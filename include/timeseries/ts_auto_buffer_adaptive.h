/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ts_auto_buffer_adaptive.h                          ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:27:47                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     212                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 15e6e31437  2026-03-09  feat: implement all features from problem statement ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file ts_auto_buffer_adaptive.h
 * @brief Adaptive flush controller for TSAutoBuffer.
 *
 * FlushController implements a feedback-control loop that dynamically adjusts
 * the flush batch size based on observed downstream write latencies from
 * TSStore.  The goal is to prevent buffer overruns without manual tuning while
 * still meeting the configurable SLO threshold.
 *
 * ## Algorithm
 * 1. Each completed flush records its wall-clock write latency.
 * 2. An EWMA (Exponentially Weighted Moving Average) of recent latencies is
 *    maintained with a configurable smoothing factor α.
 * 3. When EWMA_latency > slo_threshold_ms → reduce batch size (backpressure).
 * 4. When EWMA_latency < slo_threshold_ms * headroom_factor → grow batch size.
 * 5. Batch size is clamped to [min_batch_size, max_batch_size].
 * 6. If write latency exceeds @p slo_threshold_ms, a `ts_autobuffer_backpressure`
 *    counter is emitted and producers are blocked until the queue drains below
 *    the configured low-water mark.
 *
 * ## Performance Targets (from FUTURE_ENHANCEMENTS.md)
 * - Sustained single-point ingest: >500k points/s per node
 * - Buffer-to-storage flush latency P99: <10ms under normal load
 * - Backpressure event rate during sustained overload: <1 event/s
 *
 * ## Integration with TSAutoBuffer
 * ```cpp
 * FlushController ctrl(config);
 * ctrl.reportFlushLatency(measured_latency_ms);
 * size_t batch_sz = ctrl.recommendedBatchSize();
 * ```
 *
 * @note Thread Safety: FlushController is thread-safe; all methods may be
 *   called concurrently from the producer and flush threads.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "timeseries/ts_auto_buffer.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// FlushControllerConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for the adaptive flush controller.
 */
struct FlushControllerConfig {
    /// SLO threshold in milliseconds. Latencies above this trigger backpressure.
    double slo_threshold_ms       = 50.0;
    /// EWMA smoothing factor α ∈ (0, 1]. Higher = more weight on recent samples.
    double ewma_alpha             = 0.25;
    /// Headroom factor: grow batch size when EWMA < slo * headroom.
    double headroom_factor        = 0.7;
    /// Step factor for growing batch size (multiplicative).
    double grow_factor            = 1.25;
    /// Step factor for shrinking batch size (multiplicative).
    double shrink_factor          = 0.75;

    /// Minimum allowed batch size.
    size_t min_batch_size         = 50;
    /// Maximum allowed batch size (must be ≤ TSAutoBufferConfig::flush_batch_size).
    size_t max_batch_size         = 5000;
    /// Initial batch size.
    size_t initial_batch_size     = 500;

    /// Queue low-water mark: block producers until buffered points drop below this.
    size_t low_water_mark         = 1000;

    /// Minimum number of flush samples before adaptation kicks in.
    size_t warmup_samples         = 5;
};

// ─────────────────────────────────────────────────────────────────────────────
// FlushControllerStats
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Runtime statistics exposed by FlushController.
 */
struct FlushControllerStats {
    double   ewma_latency_ms  = 0.0;   ///< Current EWMA write latency
    size_t   current_batch_sz = 0;     ///< Current recommended batch size
    uint64_t backpressure_events = 0;  ///< Total backpressure events emitted
    uint64_t samples          = 0;     ///< Total flush samples received
    bool     in_backpressure  = false; ///< Whether backpressure is active
};

// ─────────────────────────────────────────────────────────────────────────────
// FlushController
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Adaptive flush batch-size controller for TSAutoBuffer.
 *
 * Integrates with TSAutoBuffer by replacing the static `flush_batch_size`
 * in TSAutoBufferConfig with a dynamically adjusted value.
 */
class FlushController {
public:
    explicit FlushController(FlushControllerConfig config = {});

    // ── Feedback ──────────────────────────────────────────────────────────────

    /**
     * @brief Record the measured write latency of a completed flush.
     *
     * Updates the EWMA and adjusts the recommended batch size.
     * Emits a backpressure metric when the SLO is breached.
     *
     * @param latency_ms  Measured flush write latency in milliseconds.
     */
    void reportFlushLatency(double latency_ms);

    // ── Producer-side backpressure ────────────────────────────────────────────

    /**
     * @brief Notify the controller that @p buffered_points are queued.
     *
     * If backpressure is active, this call blocks until the queue drains
     * below the low_water_mark or the timeout expires.
     *
     * @param buffered_points  Current number of buffered data points.
     * @param timeout          Maximum time to wait before unblocking.
     * @return true if the producer may proceed; false if timed out.
     */
    bool checkBackpressure(size_t                     buffered_points,
                           std::chrono::milliseconds  timeout = std::chrono::seconds{5});

    /**
     * @brief Signal that the buffer has been partially drained.
     *
     * Call this from the flush thread after each flush completes.
     *
     * @param remaining_points  Points still in the buffer after the flush.
     */
    void notifyDrained(size_t remaining_points);

    // ── Query ─────────────────────────────────────────────────────────────────

    /**
     * @brief Return the current recommended flush batch size.
     */
    size_t recommendedBatchSize() const noexcept;

    /**
     * @brief Return the current EWMA latency in milliseconds.
     */
    double ewmaLatencyMs() const noexcept;

    /**
     * @brief Return a snapshot of all controller statistics.
     */
    FlushControllerStats stats() const noexcept;

    /**
     * @brief True when the controller is actively signalling backpressure.
     */
    bool isBackpressureActive() const noexcept;

    const FlushControllerConfig& config() const noexcept { return config_; }

private:
    FlushControllerConfig   config_;

    mutable std::mutex      mutex_;
    std::condition_variable cv_;

    double   ewma_latency_ms_  = 0.0;
    size_t   batch_size_       = 0;
    uint64_t sample_count_     = 0;
    uint64_t backpressure_events_ = 0;
    bool     backpressure_     = false;
    size_t   current_buffered_ = 0;

    void updateBatchSize();
};

} // namespace themis
