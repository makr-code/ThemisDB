/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_flush_controller.h                        ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-13 04:21:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     306                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0c584eba98  2026-04-09  feat(timeseries): implement AdaptiveFlushController (PERF... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file adaptive_flush_controller.h
 * @brief AdaptiveFlushController — buffered, asynchronous timeseries writer.
 *
 * AdaptiveFlushController buffers incoming TSStore::DataPoint writes and
 * flushes them asynchronously in batches, with two flush triggers:
 *
 *  1. **Watermark trigger** – when the buffer reaches `watermark_ratio`
 *     (default 80 %) of `buffer_capacity` a flush is initiated immediately.
 *  2. **Timeout trigger**  – if no watermark flush has occurred within
 *     `flush_interval` (default 100 ms) a periodic flush is triggered.
 *
 * Additionally:
 *  - **Backpressure** – producers that call add() or addBatch() are blocked
 *    (or return ERR_API_RESOURCE_EXHAUSTED) when the buffer is at or above
 *    the watermark threshold (80 % default).
 *  - **Overdue flush alerting** – if any data has been held in the buffer for
 *    longer than `flush_interval * overdue_flush_multiplier` (default 2×) a
 *    WARN log is emitted and the overdue counter is incremented.
 *
 * ## Public API
 * ```cpp
 * AdaptiveFlushControllerConfig cfg;
 * cfg.buffer_capacity  = 10000;
 * cfg.flush_interval   = std::chrono::milliseconds{100};
 * cfg.watermark_ratio  = 0.80;
 *
 * AdaptiveFlushController afc(tsstore, cfg);
 * afc.start();
 *
 * afc.add(point);                        // single-point add
 * afc.addBatch({p1, p2, p3});            // batch add
 * afc.flush();                           // manual flush
 * auto stats = afc.getStats();           // statistics snapshot
 * bool bp     = afc.isBackpressured();   // check backpressure
 *
 * afc.stop();   // flush remaining points and stop background thread
 * ```
 *
 * @note Thread Safety: all public methods are thread-safe.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "timeseries/tsstore.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace themis {

class TimeSeriesMetrics; // forward declaration

// ─────────────────────────────────────────────────────────────────────────────
// AdaptiveFlushControllerConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for AdaptiveFlushController.
 */
struct AdaptiveFlushControllerConfig {
    /// Total buffer capacity in data points.  Default: 10 000.
    size_t buffer_capacity = 10'000;

    /// Periodic flush interval.  A flush is triggered if no watermark flush
    /// has occurred within this duration.  Default: 100 ms.
    std::chrono::milliseconds flush_interval{100};

    /// Fraction of `buffer_capacity` at which a watermark flush is triggered
    /// and backpressure is applied to producers.  Default: 0.80 (80 %).
    double watermark_ratio = 0.80;

    /// A buffer is considered overdue when its oldest data point has been
    /// held for longer than `flush_interval * overdue_flush_multiplier`.
    /// An overdue event emits a WARN log and increments the stats counter.
    /// Default: 2 (= 2× the flush interval).
    unsigned overdue_flush_multiplier = 2;

    /// Number of data points written to TSStore per flush operation.
    size_t flush_batch_size = 500;

    /// When true (default) flushing runs in a background thread so that
    /// add() / addBatch() never block waiting for storage I/O.
    bool async_flush = true;

    /// Optional metrics sink.  When non-null, backpressure and overdue-flush
    /// events are reported via TimeSeriesMetrics::recordBackpressure() and
    /// TimeSeriesMetrics::recordOverdueFlush().  Not owned by this class.
    TimeSeriesMetrics* metrics = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// AdaptiveFlushControllerStats
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Runtime statistics for AdaptiveFlushController.
 */
struct AdaptiveFlushControllerStats {
    uint64_t points_buffered{0};             ///< Total data points passed to add()/addBatch()
    uint64_t points_flushed{0};              ///< Total data points written to TSStore
    uint64_t flush_count{0};                 ///< Total flush operations performed
    uint64_t watermark_triggered_flushes{0}; ///< Flushes triggered by the watermark
    uint64_t timeout_triggered_flushes{0};   ///< Flushes triggered by the periodic timeout
    uint64_t backpressure_events{0};         ///< Times a producer was blocked by backpressure
    uint64_t overdue_flush_events{0};        ///< Times an overdue flush was detected and alerted

    size_t current_buffer_size{0};           ///< Points currently in the buffer
    double buffer_utilization{0.0};          ///< buffer_size / buffer_capacity  (0.0 – 1.0)

    std::chrono::steady_clock::time_point last_flush_time; ///< Wall clock of last flush
};

// ─────────────────────────────────────────────────────────────────────────────
// AdaptiveFlushController
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Buffered, asynchronous timeseries write controller.
 *
 * Implements the PERF-D1-A acceptance criteria:
 *  - Buffer capacity configurable (default 10 000 points)
 *  - Flush triggers: watermark (80 % fill) OR timeout (100 ms)
 *  - Async flush worker — add()/addBatch() are non-blocking
 *  - Backpressure at 80 % buffer fill
 *  - Overdue flush alerts when data held > 2× flush_interval
 *  - Metrics/statistics for flushes and buffer utilisation
 *  - No memory leaks; full lifecycle (start/stop) is safe
 */
class AdaptiveFlushController {
public:
    /**
     * @brief Construct controller.
     * @param tsstore  TSStore backend (not owned; must outlive this object).
     * @param config   Configuration.
     */
    explicit AdaptiveFlushController(TSStore* tsstore,
                                     AdaptiveFlushControllerConfig config = {});

    ~AdaptiveFlushController();

    AdaptiveFlushController(const AdaptiveFlushController&)            = delete;
    AdaptiveFlushController& operator=(const AdaptiveFlushController&) = delete;
    AdaptiveFlushController(AdaptiveFlushController&&)                 = delete;
    AdaptiveFlushController& operator=(AdaptiveFlushController&&)      = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /** @brief Start the background flush thread (no-op if already running). */
    void start();

    /**
     * @brief Stop the background flush thread.
     *
     * Flushes all remaining buffered points synchronously before returning.
     * Unblocks any producers waiting on backpressure.
     */
    void stop();

    /** @brief True while the controller is running. */
    bool isRunning() const { return running_.load(std::memory_order_relaxed); }

    // ── Write API ─────────────────────────────────────────────────────────────

    /**
     * @brief Buffer a single data point.
     *
     * If backpressure is active (buffer ≥ 80 % full) the call blocks until
     * the buffer drains below the watermark or the controller is stopped.
     * After stop() any blocked producer receives ERR_API_RESOURCE_EXHAUSTED.
     *
     * @param point  Data point to buffer.
     * @return Result<void> — success, or an error code.
     */
    Result<void> add(const TSStore::DataPoint& point);

    /**
     * @brief Buffer a batch of data points.
     *
     * Equivalent to calling add() for each point in order, but more
     * efficient (single lock acquisition for the whole batch).
     *
     * @param points  Data points to buffer.
     * @return Result<size_t> — number of points accepted, or an error.
     */
    Result<size_t> addBatch(const std::vector<TSStore::DataPoint>& points);

    /**
     * @brief Flush all currently buffered points to TSStore immediately.
     *
     * Safe to call from any thread while the controller is running or stopped.
     *
     * @return Number of points flushed.
     */
    size_t flush();

    // ── Query API ─────────────────────────────────────────────────────────────

    /** @brief Return a snapshot of all statistics. */
    AdaptiveFlushControllerStats getStats() const;

    /**
     * @brief True when the buffer is at or above the backpressure watermark.
     *
     * This is a lock-free, instantaneous check.  Use it to poll backpressure
     * state without blocking.
     */
    bool isBackpressured() const noexcept;

    /** @brief Return the current configuration. */
    const AdaptiveFlushControllerConfig& getConfig() const { return config_; }

private:
    // ── Internal helpers ──────────────────────────────────────────────────────

    /// Background flush loop (runs in flush_thread_).
    void flushThread();

    /// Perform one flush cycle: drain up to flush_batch_size points per call.
    /// @return number of points written to TSStore.
    size_t flushInternal();

    /// Watermark threshold in absolute point count.
    size_t watermarkThreshold() const noexcept;

    /// True when buffer_size_ >= watermark threshold.
    bool watermarkReached() const noexcept;

    /// True when the oldest buffered point has been held > overdue threshold.
    bool isOverdue() const;

    /// Validate a single DataPoint; returns non-null error string on failure.
    static const char* validatePoint(const TSStore::DataPoint& p) noexcept;

    // ── State ─────────────────────────────────────────────────────────────────

    TSStore*                       tsstore_;
    AdaptiveFlushControllerConfig  config_;

    /// Primary data buffer (protected by buffer_mutex_).
    std::deque<TSStore::DataPoint> buffer_;
    mutable std::mutex             buffer_mutex_;

    /// Lock-free buffer-size counter for backpressure checks.
    std::atomic<size_t>            buffer_size_{0};

    /// Set to true when buffer_size_ >= watermark (lock-free read for isBackpressured()).
    std::atomic<bool>              backpressure_{false};

    /// Time of the oldest point currently in the buffer (protected by buffer_mutex_).
    std::chrono::steady_clock::time_point oldest_point_time_;
    bool                           has_oldest_point_{false};

    // ── Background flush thread ───────────────────────────────────────────────

    std::atomic<bool>              running_{false};
    std::thread                    flush_thread_;
    std::condition_variable        flush_cv_;
    std::mutex                     flush_mutex_;

    // ── Backpressure wait for producers ──────────────────────────────────────

    std::condition_variable        bp_cv_;
    std::mutex                     bp_mutex_;

    // ── Statistics (atomics so that getStats() needs no lock) ─────────────────

    std::atomic<uint64_t>          stat_points_buffered_{0};
    std::atomic<uint64_t>          stat_points_flushed_{0};
    std::atomic<uint64_t>          stat_flush_count_{0};
    std::atomic<uint64_t>          stat_watermark_triggered_{0};
    std::atomic<uint64_t>          stat_timeout_triggered_{0};
    std::atomic<uint64_t>          stat_backpressure_events_{0};
    std::atomic<uint64_t>          stat_overdue_flush_events_{0};
    std::atomic<int64_t>           stat_last_flush_ns_{0}; ///< steady_clock epoch nanoseconds
};

} // namespace themis
