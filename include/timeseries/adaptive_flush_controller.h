/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_flush_controller.h                        ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file adaptive_flush_controller.h
 * @brief High-level adaptive flush controller for TSStore write path.
 *
 * AdaptiveFlushController is a facade over TSAutoBuffer + FlushController
 * that provides a simplified add/addBatch/flush/getStats/isBackpressured
 * API for integration into TSStore's write path (PERF-D1-B).
 *
 * ## Design
 * - Wraps TSAutoBuffer with adaptive flush enabled
 * - add()      → TSAutoBuffer::add()      (blocking with backpressure)
 * - addBatch() → TSAutoBuffer::add() per-point (routes each point through buffer)
 * - flush()    → TSAutoBuffer::flush()
 * - getStats() → TSAutoBuffer stats converted to BufferStats
 * - isBackpressured() → buffer size vs. high-watermark
 *
 * ## Integration with TSStore
 * ```cpp
 * AdaptiveFlushController::Config ctrl_cfg;
 * auto ctrl = std::make_shared<AdaptiveFlushController>(tsstore.get(), ctrl_cfg);
 * ctrl->start();
 * tsstore->setAdaptiveFlushController(ctrl.get());
 *
 * // putDataPoint() and putDataPoints() now route via ctrl
 * tsstore->putDataPoint(point);
 * ```
 *
 * @note Thread Safety: all public methods are thread-safe.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "timeseries/tsstore.h"
#include "timeseries/ts_auto_buffer.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace themis {

class TimeSeriesMetrics; // forward declaration

/**
 * @brief Configuration for AdaptiveFlushController.
 */
struct AdaptiveFlushControllerConfig {
    /// Maximum total buffered data points across all metric:entity pairs.
    size_t capacity = 10000;

    /// Background flush interval.
    std::chrono::milliseconds flush_interval{100};

    /// Fraction of capacity at which the high-watermark backpressure threshold is set.
    /// E.g. 0.80 means backpressure engages when 80% of capacity is filled.
    double watermark_ratio = 0.80;

    /// An overdue buffer (first-point age > flush_interval * overdue_flush_multiplier)
    /// triggers an immediate flush and a metrics warning.
    unsigned overdue_flush_multiplier = 2;

    /// Enable adaptive batch-size control based on observed write latencies.
    bool enable_adaptive_flush = true;

    /// Write-latency SLO in milliseconds; backpressure engages when EWMA exceeds this.
    double backpressure_slo_ms = 50.0;

    /// EWMA smoothing factor α ∈ (0, 1].
    double ewma_alpha = 0.25;

    /// Minimum adaptive flush batch size.
    size_t adaptive_batch_min = 50;

    /// Maximum adaptive flush batch size.
    size_t adaptive_batch_max = 5000;

    /// Initial (and maximum non-adaptive) flush batch size.
    size_t flush_batch_size = 500;

    /// Run flush on a background thread (recommended for production).
    bool async_flush = true;

    /// Optional metrics integration (not owned; must outlive this controller).
    TimeSeriesMetrics* metrics = nullptr;
};

/**
 * @brief Buffer and flush statistics exposed by AdaptiveFlushController.
 */
struct AdaptiveFlushStats {
    uint64_t points_buffered  = 0;  ///< Total points ever buffered
    uint64_t points_flushed   = 0;  ///< Total points successfully written to TSStore
    uint64_t flush_count      = 0;  ///< Total flush operations completed
    uint64_t backpressure_events = 0; ///< Total times producers were blocked
    size_t   current_buffer_size = 0; ///< Points currently in buffer
    double   current_ewma_latency_ms = 0.0; ///< Latest EWMA write latency
    size_t   current_adaptive_batch_size = 0; ///< Current adaptive batch size
};

/**
 * @brief Adaptive flush controller for the TSStore write path.
 *
 * Routes single-point and batch inserts through an adaptive TSAutoBuffer,
 * providing backpressure signalling and EWMA-controlled batch sizing.
 * Integrates with TSStore via setAdaptiveFlushController().
 */
class AdaptiveFlushController {
public:
    /**
     * @brief Construct the controller.
     * @param store  TSStore instance (not owned; must outlive this controller).
     * @param config Controller configuration.
     */
    explicit AdaptiveFlushController(TSStore* store,
                                     AdaptiveFlushControllerConfig config = {});

    ~AdaptiveFlushController();

    // Non-copyable, non-movable (contains threads via TSAutoBuffer)
    AdaptiveFlushController(const AdaptiveFlushController&) = delete;
    AdaptiveFlushController& operator=(const AdaptiveFlushController&) = delete;
    AdaptiveFlushController(AdaptiveFlushController&&) = delete;
    AdaptiveFlushController& operator=(AdaptiveFlushController&&) = delete;

    /**
     * @brief Start the background flush thread.
     *
     * Must be called before any add/addBatch calls if async_flush is enabled.
     */
    void start();

    /**
     * @brief Stop the background flush thread and flush remaining points.
     */
    void stop();

    /**
     * @brief Buffer a single data point.
     *
     * If adaptive backpressure is active and the buffer exceeds the
     * high-watermark, the caller blocks until the queue drains.
     *
     * @param point Data point to buffer.
     * @return Result<void> — success, or error (e.g. stopped during backpressure wait).
     */
    Result<void> add(const TSStore::DataPoint& point);

    /**
     * @brief Buffer a batch of data points.
     *
     * Each point in the batch is routed through add() so that backpressure
     * and EWMA control apply uniformly.
     *
     * @param points Data points to buffer.
     * @return Result<void> — success, or first error encountered.
     */
    Result<void> addBatch(const std::vector<TSStore::DataPoint>& points);

    /**
     * @brief Immediately flush all buffered points to TSStore.
     * @return Number of points flushed.
     */
    size_t flush();

    /**
     * @brief Return a snapshot of buffer and flush statistics.
     */
    AdaptiveFlushStats getStats() const;

    /**
     * @brief True when the buffer is at or above the high-watermark threshold.
     *
     * This provides a non-blocking way to probe backpressure state without
     * calling add().
     */
    bool isBackpressured() const;

    /**
     * @brief Return whether the background flush thread is running.
     */
    bool isRunning() const;

    /**
     * @brief Return the current configuration.
     */
    const AdaptiveFlushControllerConfig& getConfig() const { return config_; }

private:
    AdaptiveFlushControllerConfig config_;
    std::unique_ptr<TSAutoBuffer> buffer_;

    /// Derived high-watermark (points): capacity * watermark_ratio
    size_t high_watermark_;
};

} // namespace themis
