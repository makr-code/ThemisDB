/**
 * @file adaptive_compaction.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "storage/compaction_manager.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

namespace themis {

/**
 * @brief Adaptive compaction scheduler for the storage engine (v1.7.0).
 *
 * AdaptiveCompactionScheduler monitors read/write I/O patterns, predicts the
 * CPU impact of running a compaction pass, identifies low-load time windows
 * suitable for scheduling, and dynamically adjusts the trigger thresholds
 * passed to the underlying CompactionManager — delivering the 20-30 %
 * compaction CPU overhead reduction targeted by roadmap item 209.
 *
 * ## Key capabilities
 *
 *  • **Pattern monitoring** – recordRead() / recordWrite() accumulate I/O
 *    counts.  A background sampling thread converts them into per-sample
 *    read/write rates and maintains an Exponential Moving Average (EMA)
 *    of each rate to dampen transient spikes.
 *
 *  • **Impact prediction** – predictCompactionImpact() uses the current EMA
 *    write rate and the caller-supplied write-amplification factor to estimate
 *    compaction CPU overhead and duration without running an actual compaction.
 *
 *  • **Low-load scheduling** – isLowLoadPeriod() returns true when both the
 *    EMA write rate and the EMA read rate are below their configured thresholds.
 *    shouldTriggerCompaction() combines the low-load signal with the urgency
 *    derived from write amplification to give a single scheduling decision.
 *
 *  • **Dynamic trigger adjustment** – getAdaptedConfig() computes a
 *    CompactionManager::Config whose tombstone threshold and GC interval are
 *    derived from the current workload: aggressive during quiet periods,
 *    conservative during peaks.  applyAdaptedConfig() pushes the result
 *    straight into a live CompactionManager.
 *
 * ## Thread safety
 *
 * All public methods are thread-safe.  The background sampling thread is
 * started with startSampling() and stopped (with a join) by stopSampling()
 * or the destructor.
 *
 * ## Usage example
 *
 * @code
 * auto cm  = std::make_shared<CompactionManager>(db);
 * AdaptiveCompactionScheduler sched;
 *
 * sched.startSampling();
 * cm->startBackgroundGC();
 *
 * // In your read / write paths:
 * sched.recordWrite(batch_size);
 * sched.recordRead();
 *
 * // Periodically in a maintenance loop:
 * auto stats = cm->stats();
 * if (sched.shouldTriggerCompaction(stats.writeAmplification())) {
 *     cm->compactAll();
 * }
 * sched.applyAdaptedConfig(*cm);   // keep thresholds in sync
 * @endcode
 */
class AdaptiveCompactionScheduler {
public:
    // ── Configuration ─────────────────────────────────────────────────────

    /** Configuration options. */
    struct Config {
        // ── Pattern monitoring ──────────────────────────────────────────

        /** Number of I/O samples retained in the sliding window. */
        uint32_t window_samples = 60;

        /** Interval between background sampling ticks. */
        std::chrono::seconds sample_interval{60};

        /**
         * EMA smoothing factor α ∈ (0, 1].
         * Larger = faster response, smaller = more smoothing.
         */
        double ema_alpha = 0.2;

        // ── Low-load detection ──────────────────────────────────────────

        /** Writes/s EMA below which we consider the system "low load". */
        double low_load_write_rate = 100.0;

        /** Reads/s EMA below which we consider the system "low load". */
        double low_load_read_rate = 1000.0;

        // ── Compaction impact prediction ────────────────────────────────

        /**
         * Write-amplification factor above which compaction is considered
         * urgent regardless of current load.
         */
        double urgent_write_amp_threshold = 8.0;

        /**
         * Write-amplification factor above which compaction is desirable
         * during low-load windows.
         */
        double desired_write_amp_threshold = 3.0;

        // ── Adaptive compaction triggers ────────────────────────────────

        /** Minimum tombstone GC threshold (used during high-write periods). */
        uint64_t min_tombstone_threshold = 1'000;

        /** Maximum tombstone GC threshold (used during low-write periods). */
        uint64_t max_tombstone_threshold = 100'000;

        /** Shortest background GC interval (low load → compact aggressively). */
        std::chrono::seconds min_gc_interval{60};

        /** Longest background GC interval (high load → back off). */
        std::chrono::seconds max_gc_interval{1800};
    };

    // ── Data types ─────────────────────────────────────────────────────────

    /** A single I/O observation collected by the background sampling thread. */
    struct IOSample {
        std::chrono::steady_clock::time_point timestamp;
        double write_rate{0.0};  ///< Writes/s observed during this interval
        double read_rate{0.0};   ///< Reads/s observed during this interval
    };

    /**
     * @brief Predicted impact of running a compaction pass right now.
     *
     * All fields are estimates based on the current EMA rates and the
     * write-amplification supplied by the caller.
     */
    struct CompactionImpactPrediction {
        /** Estimated fractional CPU overhead [0.0, 1.0]. */
        double estimated_cpu_overhead{0.0};

        /** Estimated wall-clock duration of the compaction pass (seconds). */
        double estimated_duration_s{0.0};

        /** Write-amplification factor used for this prediction. */
        double write_amplification{0.0};

        /** True if compaction is urgently required (WA exceeds urgent threshold). */
        bool is_urgent{false};

        /**
         * True when current load is low enough that compaction impact will be
         * minimal and scheduling is recommended.
         */
        bool is_low_impact{false};
    };

    /**
     * @brief Dynamically adjusted CompactionManager configuration derived
     *        from the most recent workload observation.
     */
    struct AdaptedConfig {
        uint64_t tombstone_gc_threshold{10'000};
        std::chrono::seconds bg_gc_interval{300};
        bool enable_full_compaction{false};
    };

    /** Statistics snapshot returned by stats(). */
    struct Stats {
        uint64_t total_reads{0};           ///< Cumulative recorded reads
        uint64_t total_writes{0};          ///< Cumulative recorded writes
        double   ema_read_rate{0.0};       ///< Current EMA reads/s
        double   ema_write_rate{0.0};      ///< Current EMA writes/s
        uint64_t compaction_schedules{0};  ///< Times shouldTriggerCompaction() returned true
        uint64_t trigger_adjustments{0};   ///< Times thresholds were updated
        uint32_t sample_count{0};          ///< Samples currently in the window
    };

    // ── Construction ───────────────────────────────────────────────────────

    /** Construct with default configuration. */
    AdaptiveCompactionScheduler();

    /** Construct with custom configuration. */
    explicit AdaptiveCompactionScheduler(const Config& config);

    ~AdaptiveCompactionScheduler();

    // Not copyable or movable after construction.
    AdaptiveCompactionScheduler(const AdaptiveCompactionScheduler&) = delete;
    AdaptiveCompactionScheduler& operator=(const AdaptiveCompactionScheduler&) = delete;

    // ── Pattern monitoring ────────────────────────────────────────────────

    /** Record @p count read operations. */
    void recordRead(uint64_t count = 1);

    /** Record @p count write operations. */
    void recordWrite(uint64_t count = 1);

    // ── Prediction ────────────────────────────────────────────────────────

    /**
     * @brief Predict the impact of running a compaction pass now.
     *
     * @param current_write_amp  Current write-amplification factor; pass 0.0
     *                           if unknown (the prediction is still useful but
     *                           less precise).
     */
    CompactionImpactPrediction predictCompactionImpact(
        double current_write_amp = 0.0) const;

    // ── Scheduling decisions ──────────────────────────────────────────────

    /**
     * @brief Return true when the current EMA I/O rates are below the
     *        configured low-load thresholds.
     */
    bool isLowLoadPeriod() const;

    /**
     * @brief Return true when a compaction should be triggered.
     *
     * Returns true if:
     *  - write amplification is above the urgent threshold, OR
     *  - write amplification is above the desired threshold AND the system is
     *    in a low-load period.
     *
     * A "yes" increments the internal compaction_schedules counter.
     *
     * @param current_write_amp  Write-amplification factor from CompactionManager::Stats.
     */
    bool shouldTriggerCompaction(double current_write_amp = 0.0);

    // ── Dynamic trigger adjustment ────────────────────────────────────────

    /**
     * @brief Compute a CompactionManager::Config adjusted for the current
     *        workload without applying it.
     */
    AdaptedConfig getAdaptedConfig() const;

    /**
     * @brief Apply the dynamically computed configuration to @p mgr.
     *
     * This adjusts the tombstone GC threshold and background GC interval of
     * the live CompactionManager based on the current workload.
     *
     * @note This restarts the background GC thread inside mgr if it was
     *       already running, so the new interval takes effect immediately.
     */
    void applyAdaptedConfig(CompactionManager& mgr);

    // ── Background sampling ───────────────────────────────────────────────

    /**
     * @brief Start the background sampling thread.
     *
     * The thread wakes every config.sample_interval, converts the
     * accumulated read/write counts into per-second rates, updates the EMA,
     * and appends an IOSample to the sliding window.
     *
     * Does nothing if the thread is already running.
     */
    void startSampling();

    /** Stop and join the background sampling thread. */
    void stopSampling();

    /** Return true if the background sampling thread is running. */
    bool isSamplingRunning() const;

    // ── Metrics ───────────────────────────────────────────────────────────

    /** Return a snapshot of current scheduler statistics. */
    Stats stats() const;

private:
    void samplingLoop();
    void collectSample();
    void updateEMA(double new_value, double& ema) noexcept;
    AdaptedConfig computeAdaptedConfig() const;

    Config config_;

    // ── Counters (per-window; reset after each sample) ───────────────────
    std::atomic<uint64_t> window_reads_{0};
    std::atomic<uint64_t> window_writes_{0};

    // ── Cumulative totals ────────────────────────────────────────────────
    std::atomic<uint64_t> total_reads_{0};
    std::atomic<uint64_t> total_writes_{0};

    // ── EMA rates (protected by rates_mutex_) ────────────────────────────
    mutable std::mutex rates_mutex_;
    double ema_read_rate_{0.0};
    double ema_write_rate_{0.0};
    std::chrono::steady_clock::time_point last_sample_time_;

    // ── Sliding window samples ────────────────────────────────────────────
    mutable std::mutex samples_mutex_;
    std::deque<IOSample> samples_;

    // ── Stats counters ────────────────────────────────────────────────────
    std::atomic<uint64_t> compaction_schedules_{0};
    std::atomic<uint64_t> trigger_adjustments_{0};

    // ── Background sampling thread ────────────────────────────────────────
    std::thread sample_thread_;
    std::atomic<bool> sample_stop_{false};
    mutable std::mutex sample_mutex_;
    std::condition_variable sample_cv_;
};

} // namespace themis
