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
     */
    explicit AsyncWalShipper(WalShippingConfig config);

    /**
     * @brief Stop the background thread and release resources.
     * Blocks until the background thread has exited.
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
     * Thread-safe; replaces any previously registered callback.
     */
    void setAlertCallback(AlertCallback cb);

    /**
     * @brief Set the transport handler.
     *
     * Must be called before the first enqueueSegment() call if a real
     * transport is required.  Thread-safe.
     */
    void setShipHandler(ShipHandler handler);

    // -----------------------------------------------------------------------
    // Segment ingestion
    // -----------------------------------------------------------------------

    /**
     * @brief Enqueue a WAL segment for async shipping.
     *
     * @param segment Segment to ship; moved into the internal queue.
     * @return true when the segment was accepted; false when the queue is at
     *         capacity (back-pressure signal to the caller).
     */
    bool enqueueSegment(WalSegment segment);

    // -----------------------------------------------------------------------
    // Metrics
    // -----------------------------------------------------------------------

    /**
     * @brief Return current stats snapshot.
     * Thread-safe.
     */
    WalShippingStats stats() const;

    /**
     * @brief Return the current shipping lag in milliseconds.
     *
     * Defined as the age of the oldest queued segment, or 0 when the queue
     * is empty.  This is the primary lag value monitored against max_lag_ms.
     */
    int64_t currentLagMs() const;

    /**
     * @brief Export Prometheus-format metrics text.
     *
     * Exposes:
     * - `replication_wal_lag_ms` histogram (buckets at geometric intervals)
     * - `replication_wal_segments_enqueued_total` counter
     * - `replication_wal_segments_shipped_total` counter
     * - `replication_wal_segments_dropped_total` counter
     * - `replication_wal_lag_alerts_total` counter
     * - `replication_wal_bytes_shipped_total` counter
     */
    std::string exportPrometheusMetrics() const;

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Gracefully stop the background thread.
     *
     * Drains the remaining queue before stopping (best-effort; does not
     * block indefinitely).  Safe to call multiple times.
     */
    void stop();

private:
    WalShippingConfig config_;

    // Queue
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

    /// Background thread main loop.
    void workerLoop();

    /// Dispatch a single segment: invoke ship handler, update stats/histogram.
    void dispatchSegment(const WalSegment& seg);

    /// Record one lag sample into the histogram.
    void recordLagSample(int64_t lag_ms);

    /// Build histogram bucket bounds for the given config.
    static std::vector<double> buildHistogramBounds(
        uint32_t max_lag_ms,
        uint32_t buckets);
};

} // namespace replication
} // namespace themisdb
