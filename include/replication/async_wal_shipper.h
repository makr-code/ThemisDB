// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file async_wal_shipper.h
 * @brief Async Cross-Region WAL Shipping API for ThemisDB replication.
 *
 * Implements asynchronous WAL segment shipping to remote datacenters with:
 * - Configurable lag limits (default 1 000 ms) per `WalShippingConfig`.
 * - Alert callback fired when the shipping lag exceeds the configured limit.
 * - Prometheus-format histogram export (`replication_wal_lag_ms`).
 * - Background worker thread; all public methods are thread-safe.
 *
 * ### Acceptance criteria (ROADMAP §3.1)
 * - WAL ship throughput ≥ 80 MB/s on a GbE link (measured end-to-end by
 *   caller; internal queue + dispatch overhead is benchmarked separately).
 * - Lag alert fires within 2× the configured lag window.
 * - `replication_wal_lag_ms` Prometheus histogram is wired.
 *
 * @see include/replication/geo_placement.h
 * @see src/replication/ROADMAP.md — §3.1 Async cross-region WAL shipping
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace themisdb {
namespace replication {

// ============================================================================
// WalSegment — the unit of WAL work
// ============================================================================

/**
 * @brief A single WAL segment to be shipped to a remote datacenter.
 *
 * Fields:
 * - sequence_number: monotonically increasing WAL segment sequence.
 * - data:            raw segment bytes.
 * - enqueue_time:    wall-clock time the segment was handed to AsyncWalShipper.
 * - target_dc:       destination datacenter identifier.
 */
struct WalSegment {
    uint64_t                                sequence_number = 0;
    std::string                             data;
    std::chrono::steady_clock::time_point   enqueue_time{};
    std::string                             target_dc;
};

// ============================================================================
// WalShippingConfig
// ============================================================================

/**
 * @brief Configuration for AsyncWalShipper.
 *
 * Corresponds to the `replication.wal_shipping.*` config key namespace.
 */
struct WalShippingConfig {
    /** Remote datacenter endpoint (host:port or symbolic name). */
    std::string remote_dc_endpoint;

    /** Identifier for this (local) datacenter. */
    std::string local_dc_id;

    /**
     * Maximum acceptable WAL shipping lag in milliseconds.
     * Defaults to 1 000 ms (1 s) per ROADMAP acceptance criteria.
     */
    uint32_t max_lag_ms = 1000;

    /**
     * Maximum number of segments that may queue before back-pressure is
     * applied (enqueueSegment returns false when the queue is full).
     * Defaults to 4096.
     */
    uint32_t max_queue_depth = 4096;

    /**
     * Number of histogram buckets for the `replication_wal_lag_ms`
     * Prometheus histogram.  Bucket upper-bounds are automatically
     * generated as a geometric series up to 10× max_lag_ms.
     */
    uint32_t histogram_buckets = 16;
};

// ============================================================================
// WalShippingStats
// ============================================================================

/**
 * @brief Snapshot of AsyncWalShipper runtime statistics.
 */
struct WalShippingStats {
    uint64_t segments_enqueued  = 0;  ///< Total segments accepted
    uint64_t segments_shipped   = 0;  ///< Total segments dispatched
    uint64_t segments_dropped   = 0;  ///< Segments dropped (queue full)
    uint64_t lag_alerts_fired   = 0;  ///< Times lag exceeded max_lag_ms
    int64_t  current_lag_ms     = 0;  ///< Most recently measured lag (ms)
    int64_t  max_observed_lag_ms= 0;  ///< Peak observed lag since start
    uint64_t bytes_enqueued     = 0;  ///< Total bytes enqueued
    uint64_t bytes_shipped      = 0;  ///< Total bytes dispatched
};

// ============================================================================
// AsyncWalShipper
// ============================================================================

/**
 * @brief Asynchronous cross-region WAL segment shipper.
 *
 * Accepts WAL segments via enqueueSegment() and dispatches them to the
 * configured remote datacenter from a dedicated background thread.
 *
 * ### Lag monitoring
 * The lag is defined as the wall-clock time between the moment a segment is
 * enqueued (WalSegment::enqueue_time) and the moment it is dispatched.  When
 * that delta exceeds WalShippingConfig::max_lag_ms the alert callback (if set)
 * is invoked once per offending segment.
 *
 * ### Transport contract
 * The actual byte-level transport is injected via setShipHandler().  The
 * default handler is a no-op that counts bytes without sending them (useful
 * for unit tests and benchmarks).  Production deployments must inject a real
 * transport handler (e.g. gRPC, TCP socket, or cloud-storage PUT).
 *
 * ### Thread safety
 * All public methods are thread-safe.  The background thread is started on
 * construction and stopped gracefully on destruction (or via stop()).
 *
 * ### Usage example
 * @code
 *   WalShippingConfig cfg;
 *   cfg.remote_dc_endpoint = "dc-eu-west:9876";
 *   cfg.local_dc_id        = "dc-us-east";
 *   cfg.max_lag_ms         = 500;
 *
 *   AsyncWalShipper shipper(cfg);
 *   shipper.setAlertCallback([](uint64_t lag_ms) {
 *       std::cerr << "WAL lag alert: " << lag_ms << " ms\n";
 *   });
 *
 *   WalSegment seg;
 *   seg.sequence_number = 1;
 *   seg.data            = wal_bytes;
 *   seg.enqueue_time    = std::chrono::steady_clock::now();
 *   seg.target_dc       = "dc-eu-west";
 *   shipper.enqueueSegment(std::move(seg));
 *
 *   std::cout << shipper.exportPrometheusMetrics();
 * @endcode
 */
class AsyncWalShipper {
public:
    /** Callback type for lag-limit alerts.  Receives current lag in ms. */
    using AlertCallback = std::function<void(uint64_t lag_ms)>;

    /** Transport handler type.  Receives the segment to send.
     *  Returns true on success, false on transient failure.  */
    using ShipHandler = std::function<bool(const WalSegment&)>;

    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct and start the background shipping thread.
     * @param config WalShippingConfig; copied on construction.
     * 
     * @note Thread safety: Construction is not thread-safe; ensure no concurrent
     *       access to this object until construction completes.
     * 
     * @note The background worker thread is started immediately and will begin
     *       processing queued segments.
     */
    explicit AsyncWalShipper(WalShippingConfig config);

    /**
     * @brief Stop the background thread and release resources.
     * 
     * Blocks until the background thread has exited gracefully (best-effort).
     * Attempts to drain remaining queue segments before stopping.
     * 
     * @note Thread safety: This method acquires queue_mutex_ and may be called
     *       from any thread.
     * 
     * @note Exception safety: Noexcept.
     */
    ~AsyncWalShipper();

    // Non-copyable; movable
    AsyncWalShipper(const AsyncWalShipper&)            = delete;
    AsyncWalShipper& operator=(const AsyncWalShipper&) = delete;
    AsyncWalShipper(AsyncWalShipper&&)                 = delete;
    AsyncWalShipper& operator=(AsyncWalShipper&&)      = delete;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Set the alert callback invoked when lag exceeds max_lag_ms.
     * 
     * Thread-safe; replaces any previously registered callback.
     * 
     * @param cb Callback function; nullptr to unregister.
     *           Callback receives lag in milliseconds.
     *           Callback exceptions are caught and suppressed.
     * 
     * @note Thread safety: Acquires callback_mutex_; safe to call concurrently.
     */
    void setAlertCallback(AlertCallback cb);

    /**
     * @brief Set the transport handler for WAL segment shipping.
     *
     * Must be called before the first enqueueSegment() call if a real
     * transport is required (default is no-op counting handler).
     * 
     * @param handler Transport handler function. Receives WalSegment by const ref.
     *                Returns true on success, false on transient failure.
     *                Exceptions from handler are NOT caught; handler must be
     *                exception-safe.
     * 
     * @note Thread safety: Acquires callback_mutex_; safe to call concurrently.
     * 
     * @note Lifetime: Handler is copied; caller retains ownership of any
     *       captured state.
     */
    void setShipHandler(ShipHandler handler);

    // -----------------------------------------------------------------------
    // Segment ingestion
    // -----------------------------------------------------------------------

    /**
     * @brief Enqueue a WAL segment for async shipping.
     *
     * Implements backpressure control: when the queue reaches max_queue_depth,
     * this returns false to signal the caller to back off (drop segment, wait,
     * or retry).
     * 
     * @param segment Segment to ship; moved into the internal queue.
     * @return true when the segment was accepted; false when the queue is at
     *         capacity (back-pressure signal to the caller).
     * 
     * @note Thread safety: Acquires queue_mutex_; safe to call concurrently.
     *       Condition variable is notified after enqueue.
     * 
     * @note Backpressure: When false is returned, caller must implement
     *       backpressure strategy (e.g., wait, retry, or drop).
     * 
     * @note Segment ownership: Moved into queue; original segment object
     *       left in moved-from state (safe to destroy).
     */
    bool enqueueSegment(WalSegment segment);

    // -----------------------------------------------------------------------
    // Metrics
    // -----------------------------------------------------------------------

    /**
     * @brief Return current stats snapshot.
     * 
     * Thread-safe; returns a consistent snapshot of current statistics.
     * 
     * @return WalShippingStats containing segments_enqueued, segments_shipped,
     *         segments_dropped, lag_alerts_fired, current and max lag, and
     *         bytes counters.
     * 
     * @note Thread safety: Acquires stats_mutex_; safe to call concurrently.
     */
    WalShippingStats stats() const;

    /**
     * @brief Return the current shipping lag in milliseconds.
     *
     * Defined as the wall-clock time between the moment the oldest segment
     * was enqueued (WalSegment::enqueue_time) and now. When the queue is empty,
     * returns 0. This is the primary lag value monitored against max_lag_ms.
     * 
     * @return Current lag in milliseconds (0 if queue is empty).
     * 
     * @note Thread safety: Acquires queue_mutex_; safe to call concurrently.
     * 
     * @note Timing: Lag is calculated using std::chrono::steady_clock for
     *       monotonic timing (immune to system clock adjustments).
     */
    int64_t currentLagMs() const;

    /**
     * @brief Export Prometheus-format metrics text.
     *
     * Exposes all WAL shipping metrics as Prometheus 0.0.4 format text.
     * Includes histogram (replication_wal_lag_ms), counters, and gauge metrics.
     *
     * Metrics exported:
     * - `replication_wal_lag_ms` histogram (buckets at geometric intervals)
     * - `replication_wal_segments_enqueued_total` counter
     * - `replication_wal_segments_shipped_total` counter
     * - `replication_wal_segments_dropped_total` counter
     * - `replication_wal_lag_alerts_total` counter
     * - `replication_wal_bytes_shipped_total` counter
     * 
     * Labels per metric:
     * - `local_dc`: local datacenter identifier
     * - `remote_dc`: remote datacenter endpoint
     * 
     * @return Prometheus-format string (valid metric text).
     * 
     * @note Thread safety: Acquires stats_mutex_ and histogram_mutex_;
     *       safe to call concurrently.
     * 
     * @note Format: Follows Prometheus v0.0.4 text exposition format.
     */
    std::string exportPrometheusMetrics() const;

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Gracefully stop the background thread.
     *
     * Sets stop_requested flag and notifies the worker thread. Drains the
     * remaining queue before stopping (best-effort; does not block indefinitely).
     * Safe to call multiple times.
     * 
     * @note Thread safety: Acquires queue_mutex_; safe to call concurrently.
     * 
     * @note Graceful shutdown: Worker thread will process all remaining queued
     *       segments before exiting (no forced termination).
     * 
     * @note Blocking: May block briefly while joining worker thread.
     */
    void stop();

private:
    WalShippingConfig config_;

    // -----------------------------------------------------------------------
    // Synchronization primitives and lock hierarchy
    // -----------------------------------------------------------------------
    // LOCK HIERARCHY (to prevent deadlock):
    // 1. queue_mutex_       (outermost: queue and stop flag)
    // 2. callback_mutex_    (callbacks and handler)
    // 3. stats_mutex_       (statistics)
    // 4. histogram_mutex_   (lag histogram buckets)
    // 
    // Rules:
    // - Never acquire a lower-numbered lock while holding a higher-numbered lock.
    // - When acquiring multiple locks, always acquire in order: 1 → 2 → 3 → 4.
    // - Operations in dispatchSegment acquire locks in order: histogram (4) → stats (3).
    // - Operations in enqueueSegment acquire: queue (1) → stats (3) [OK: 1 < 3].
    // - Alert callback execution unlocks state_mutex in callback to prevent deadlock.

    // Queue and lifecycle
    mutable std::mutex              queue_mutex_;
    std::condition_variable         queue_cv_;
    std::queue<WalSegment>          segment_queue_;
    std::atomic<bool>               stop_requested_{false};

    // Callbacks (protected by callback_mutex_)
    mutable std::mutex  callback_mutex_;
    AlertCallback       alert_cb_;
    ShipHandler         ship_handler_;

    // Stats
    mutable std::mutex      stats_mutex_;
    WalShippingStats        stats_;

    // Histogram for replication_wal_lag_ms
    std::vector<double>         histogram_bounds_;     ///< Bucket upper-bounds (ms)
    mutable std::mutex          histogram_mutex_;
    std::vector<uint64_t>       histogram_counts_;     ///< Per-bucket counts
    uint64_t                    histogram_sum_ms_ = 0; ///< Sum of all lag samples

    // Background worker
    std::thread worker_;

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Background thread main loop.
     * 
     * Runs in dedicated thread; dequeues segments and calls dispatchSegment().
     * Exits when stop_requested is true and queue is empty.
     */
    void workerLoop();

    /**
     * @brief Dispatch a single segment: invoke ship handler, update stats/histogram.
     * 
     * @param seg Segment to dispatch (const ref; not modified).
     * 
     * @note Exception safety: Strong (no state corruption on exception from
     *       callback; callback exceptions are caught and suppressed).
     * 
     * @note Lock order: histogram (4) → stats (3) (respects lock hierarchy).
     */
    void dispatchSegment(const WalSegment& seg);

    /**
     * @brief Record one lag sample into the histogram.
     * 
     * Updates histogram buckets and sum. Negative lags are clamped to 0.
     * 
     * @param lag_ms Lag in milliseconds.
     * 
     * @note Thread safety: Acquires histogram_mutex_; safe to call concurrently.
     */
    void recordLagSample(int64_t lag_ms);

    /**
     * @brief Build histogram bucket bounds for the given config.
     * 
     * Generates a geometric series of bucket upper bounds from 1 ms to
     * 10× max_lag_ms, divided into `buckets` intervals.
     * 
     * @param max_lag_ms Configuration maximum lag in milliseconds.
     * @param buckets Number of histogram buckets to generate.
     * @return Vector of bucket upper bounds (size == buckets).
     * 
     * @note Bucket +Inf is implicit (not stored); samples above the highest
     *       bucket are placed in the last bucket.
     */
    static std::vector<double> buildHistogramBounds(
       uint32_t max_lag_ms,
       uint32_t buckets);
};

} // namespace replication
} // namespace themisdb
