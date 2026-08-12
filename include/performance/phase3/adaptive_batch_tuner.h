/**
 * @file adaptive_batch_tuner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Adaptive Batch Size Tuning for LLM Inference
// Performance Module - Phase 3 (Issue #1996)
//
// Dynamically adjusts the LLM inference batch size based on measured
// throughput and latency, using hardware cycle counters for precise
// timing.  Works without GPU hardware present (CPU-only deployment).
//
// Algorithm:
//   1. Measure tokens-per-second throughput for the current batch size
//      using HardwareCycleCounter.
//   2. After every `window_size` batches, evaluate whether a larger or
//      smaller batch size yields better throughput.
//   3. Increase by `step_up` when throughput is improving; decrease by
//      `step_down` after a latency breach or throughput regression.
//   4. Exponential moving average smooths noisy measurements.
//
// Integration:
//   - Feature flag: Phase3FeatureFlags::adaptive_batch_tuner_enabled()
//   - Cycle metrics: HardwareCycleCounter / ScopedCycleTimer
//   - Namespace: themis::performance::phase3
//
// Expected gain: 15-40% throughput improvement for variable-length
// LLM inference workloads vs. a fixed batch size.

#pragma once

#include "performance/cycle_metrics.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace performance {
namespace phase3 {

/**
 * @brief Adaptive batch size tuner for LLM inference workloads.
 *
 * Measures per-batch inference throughput using hardware cycle counters
 * and adjusts the recommended batch size to maximise tokens/second while
 * staying within a configurable latency budget.
 *
 * Thread-safety: all public methods are safe to call concurrently.
 *
 * Typical usage:
 * @code
 *   LLMBatchTuner tuner;
 *
 *   // At the start of each inference batch:
 *   size_t batch_size = tuner.recommendedBatchSize();
 *   {
 *       auto guard = tuner.beginBatch(batch_size, total_tokens);
 *       // ... run inference for batch_size requests ...
 *   } // guard records the measurement and may trigger a tuning step
 * @endcode
 */
class LLMBatchTuner {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        /// Minimum batch size (inclusive).
        size_t min_batch_size = 1;
        /// Maximum batch size (inclusive).
        size_t max_batch_size = 64;
        /// Starting batch size (clamped to [min, max] on construction).
        size_t initial_batch_size = 4;

        /// Step size when increasing batch size.
        size_t step_up = 2;
        /// Step size when decreasing batch size.
        size_t step_down = 1;

        /// Number of batches to observe before re-evaluating the batch size.
        size_t window_size = 8;

        /// Maximum allowed P99 latency per token (ms).  0 = disabled.
        double max_latency_ms_per_token = 0.0;

        /// Smoothing factor for exponential moving average (0 < alpha <= 1).
        /// Smaller values give more weight to older measurements.
        float ema_alpha = 0.2f;

        Config() noexcept = default;
    };

    // -----------------------------------------------------------------------
    // Per-batch measurement record
    // -----------------------------------------------------------------------

    struct BatchRecord {
        size_t   batch_size   = 0;    ///< Number of sequences in the batch
        size_t   total_tokens = 0;    ///< Total tokens processed
        uint64_t cycles       = 0;    ///< CPU cycles elapsed (RDTSCP)
        double   latency_ms   = 0.0;  ///< Wall-clock duration (ms)
        double   tokens_per_s = 0.0;  ///< Derived throughput
    };

    // -----------------------------------------------------------------------
    // Statistics snapshot
    // -----------------------------------------------------------------------

    struct Stats {
        size_t  current_batch_size  = 0;
        double  ema_throughput      = 0.0;  ///< EMA tokens/s
        double  avg_latency_ms      = 0.0;  ///< Mean latency over recorded window
        double  p99_latency_ms      = 0.0;  ///< P99 latency over recorded window
        size_t  total_batches       = 0;    ///< Batches recorded since construction
        size_t  tuning_events       = 0;    ///< Number of batch-size adjustments
    };

    // -----------------------------------------------------------------------
    // RAII batch timer
    // -----------------------------------------------------------------------

    /**
     * @brief RAII guard that records a single batch measurement.
     *
     * Created by LLMBatchTuner::beginBatch(); records throughput on
     * destruction (or when end() is called explicitly).
     */
    class BatchGuard {
    public:
        BatchGuard(LLMBatchTuner& tuner,
                   size_t         batch_size,
                   size_t         total_tokens) noexcept;
        ~BatchGuard() noexcept;

        BatchGuard(const BatchGuard&)            = delete;
        BatchGuard& operator=(const BatchGuard&) = delete;
        BatchGuard(BatchGuard&&) noexcept;
        BatchGuard& operator=(BatchGuard&&)      = delete;

        /**
         * @brief Finalise the measurement explicitly.
         *
         * Safe to call even if already ended; subsequent calls are no-ops.
         */
        void end() noexcept;

    private:
        LLMBatchTuner*                        tuner_;
        size_t                                batch_size_;
        size_t                                total_tokens_;
        uint64_t                              start_cycles_;
        std::chrono::steady_clock::time_point start_wall_;
        bool                                  ended_;
    };

    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    LLMBatchTuner();
    explicit LLMBatchTuner(Config config);
    ~LLMBatchTuner();

    LLMBatchTuner(const LLMBatchTuner&)            = delete;
    LLMBatchTuner& operator=(const LLMBatchTuner&) = delete;

    // -----------------------------------------------------------------------
    // Main interface
    // -----------------------------------------------------------------------

    /**
     * @brief Return the currently recommended batch size.
     *
     * Safe to call from multiple threads; the value is updated after
     * every tuning cycle.
     */
    size_t recommendedBatchSize() const noexcept;

    /**
     * @brief Start timing an inference batch.
     *
     * @param batch_size    Number of sequences in this batch.
     * @param total_tokens  Total token count across all sequences.
     * @return RAII guard – call guard.end() or let it destruct.
     */
    [[nodiscard]]
    BatchGuard beginBatch(size_t batch_size, size_t total_tokens) noexcept;

    /**
     * @brief Record a completed batch directly (without RAII guard).
     *
     * Useful when the caller measures latency through other means.
     *
     * @param batch_size    Number of sequences processed.
     * @param total_tokens  Total tokens processed.
     * @param latency_ms    Wall-clock duration of the batch (ms).
     */
    void recordBatch(size_t batch_size,
                     size_t total_tokens,
                     double latency_ms) noexcept;

    // -----------------------------------------------------------------------
    // Observation
    // -----------------------------------------------------------------------

    /** @brief Aggregate statistics over the current measurement window. */
    Stats getStats() const;

    /** @brief Total batches recorded since construction. */
    size_t totalBatches() const noexcept;

    /**
     * @brief Return the most recent records (up to @p limit).
     * @param limit Maximum number of records to return.
     */
    std::vector<BatchRecord> getRecentRecords(size_t limit = 32) const;

    /** @brief Reset all accumulated state; batch size reverts to initial. */
    void reset() noexcept;

    /** @brief Human-readable summary for logging / diagnostics. */
    std::string summary() const;

private:
    friend class BatchGuard;

    /// Called from BatchGuard::end() after a batch completes.
    void pushRecord(BatchRecord record) noexcept;

    /// Re-evaluate and possibly update current_batch_size_.
    /// Must be called with mutex_ held.
    void maybeTune() noexcept;

    Config config_;

    mutable std::mutex   mutex_;
    std::vector<BatchRecord> records_;  ///< Rolling window

    size_t   current_batch_size_;
    double   ema_throughput_    = 0.0;
    double   prev_ema_          = 0.0;  ///< EMA at last tuning event

    std::atomic<size_t> total_batches_{0};
    std::atomic<size_t> tuning_events_{0};
};

} // namespace phase3
} // namespace performance
} // namespace themis
