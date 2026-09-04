/**
 * @file adaptive_compaction.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "storage/adaptive_compaction.h"
#include "utils/logger.h"
#include "utils/thread_join_utils.h"

#include <algorithm>
#include <cmath>

namespace themis {

// ──────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ──────────────────────────────────────────────────────────────────────────────

AdaptiveCompactionScheduler::AdaptiveCompactionScheduler()
    : AdaptiveCompactionScheduler(Config{}) {}

AdaptiveCompactionScheduler::AdaptiveCompactionScheduler(const Config& config)
    : config_(config),
      last_sample_time_(std::chrono::steady_clock::now()) {}

AdaptiveCompactionScheduler::~AdaptiveCompactionScheduler() {
    stopSampling();
}

// ──────────────────────────────────────────────────────────────────────────────
// Pattern monitoring
// ──────────────────────────────────────────────────────────────────────────────

void AdaptiveCompactionScheduler::recordRead(uint64_t count) {
    total_reads_.fetch_add(count, std::memory_order_relaxed);
    window_reads_.fetch_add(count, std::memory_order_relaxed);
}

void AdaptiveCompactionScheduler::recordWrite(uint64_t count) {
    total_writes_.fetch_add(count, std::memory_order_relaxed);
    window_writes_.fetch_add(count, std::memory_order_relaxed);
}

// ──────────────────────────────────────────────────────────────────────────────
// Internal EMA helper
// ──────────────────────────────────────────────────────────────────────────────

void AdaptiveCompactionScheduler::updateEMA(double new_value, double& ema) noexcept {
    ema = config_.ema_alpha * new_value + (1.0 - config_.ema_alpha) * ema;
}

// ──────────────────────────────────────────────────────────────────────────────
// Background sampling
// ──────────────────────────────────────────────────────────────────────────────

void AdaptiveCompactionScheduler::startSampling() {
    std::lock_guard<std::mutex> lock(sample_mutex_);
    if (sample_thread_.joinable()) return;  // already running
    sample_stop_.store(false, std::memory_order_relaxed);
    sample_thread_ = std::thread([this] { samplingLoop(); });
}

void AdaptiveCompactionScheduler::stopSampling() {
    {
        std::lock_guard<std::mutex> lock(sample_mutex_);
        sample_stop_.store(true, std::memory_order_relaxed);
        sample_cv_.notify_all();
    }
    if (sample_thread_.joinable() &&
        !utils::joinThreadWithin(sample_thread_)) {
        THEMIS_WARN("AdaptiveCompactionScheduler: sampling thread exceeded shutdown timeout");
    }
}

bool AdaptiveCompactionScheduler::isSamplingRunning() const {
    std::lock_guard<std::mutex> lock(sample_mutex_);
    return sample_thread_.joinable() &&
           !sample_stop_.load(std::memory_order_relaxed);
}

void AdaptiveCompactionScheduler::samplingLoop() {
    while (!sample_stop_.load(std::memory_order_relaxed)) {
        std::unique_lock<std::mutex> lock(sample_mutex_);
        // lock_in_loop scanner alert (line 90): cv::wait_for() semantics require
        // holding the unique_lock for the duration of the wait; this is the correct
        // condition_variable pattern — the lock is never re-acquired on each pass
        // without waiting — false positive.
        sample_cv_.wait_for(lock, config_.sample_interval,
                            [this] { return sample_stop_.load(std::memory_order_relaxed); });
        lock.unlock();

        if (sample_stop_.load(std::memory_order_relaxed)) {
          break;
        }

        collectSample();
    }
}

void AdaptiveCompactionScheduler::collectSample() {
    auto now = std::chrono::steady_clock::now();

    // Drain per-window counters atomically.
    uint64_t reads  = window_reads_.exchange(0, std::memory_order_relaxed);
    uint64_t writes = window_writes_.exchange(0, std::memory_order_relaxed);

    double elapsed_s = 0.0;
    {
        std::lock_guard<std::mutex> lock(rates_mutex_);
        auto delta = std::chrono::duration<double>(now - last_sample_time_).count();
        elapsed_s = (delta > 0.0) ? delta : 1.0;
        last_sample_time_ = now;

        double read_rate  = static_cast<double>(reads)  / elapsed_s;
        double write_rate = static_cast<double>(writes) / elapsed_s;

        updateEMA(read_rate,  ema_read_rate_);
        updateEMA(write_rate, ema_write_rate_);
    }

    // Append to sliding window (drop oldest sample when full).
    {
        std::lock_guard<std::mutex> lock(samples_mutex_);
        IOSample s;
        s.timestamp   = now;
        s.read_rate   = static_cast<double>(reads)  / elapsed_s;
        s.write_rate  = static_cast<double>(writes) / elapsed_s;
        samples_.push_back(s);
        while (static_cast<int>(samples_.size()) > config_.window_samples) {
            samples_.pop_front();
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// Prediction
// ──────────────────────────────────────────────────────────────────────────────

AdaptiveCompactionScheduler::CompactionImpactPrediction
AdaptiveCompactionScheduler::predictCompactionImpact(double current_write_amp) const {
    CompactionImpactPrediction pred;
    pred.write_amplification = current_write_amp;

    double ema_write = 0.0;
    double ema_read  = 0.0;
    {
        std::lock_guard<std::mutex> lock(rates_mutex_);
        ema_write = ema_write_rate_;
        ema_read  = ema_read_rate_;
    }

    // CPU overhead estimate: scales with write rate and write amplification.
    // Normalised against the low-load threshold so that baseline light load
    // maps to ~0.1 overhead and saturation maps to ~1.0.
    double write_factor = (config_.low_load_write_rate > 0.0)
                              ? ema_write / config_.low_load_write_rate
                              : 0.0;
    double wa_factor = (current_write_amp > 1.0)
                           ? std::log(current_write_amp) / std::log(config_.urgent_write_amp_threshold)
                           : 0.0;

    pred.estimated_cpu_overhead =
        std::min(1.0, 0.1 + 0.5 * write_factor + 0.4 * wa_factor);

    // Duration estimate: proportional to write rate (more pending bytes → longer).
    // Base of 5 s at zero load; grows up to ~300 s under heavy write pressure.
    pred.estimated_duration_s = 5.0 + 295.0 * std::min(1.0, write_factor);

    pred.is_urgent =
        (current_write_amp >= config_.urgent_write_amp_threshold);

    // Low impact = low estimated CPU overhead AND low current load.
    bool low_write = (ema_write < config_.low_load_write_rate);
    bool low_read  = (ema_read  < config_.low_load_read_rate);
    pred.is_low_impact = low_write && low_read;

    return pred;
}

// ──────────────────────────────────────────────────────────────────────────────
// Scheduling decisions
// ──────────────────────────────────────────────────────────────────────────────

bool AdaptiveCompactionScheduler::isLowLoadPeriod() const {
    std::lock_guard<std::mutex> lock(rates_mutex_);
    return (ema_write_rate_ < config_.low_load_write_rate) &&
           (ema_read_rate_  < config_.low_load_read_rate);
}

bool AdaptiveCompactionScheduler::shouldTriggerCompaction(double current_write_amp) {
    auto pred = predictCompactionImpact(current_write_amp);

    bool trigger = pred.is_urgent ||
                   (current_write_amp >= config_.desired_write_amp_threshold && pred.is_low_impact);

    if (trigger) {
        compaction_schedules_.fetch_add(1, std::memory_order_relaxed);
    }
    return trigger;
}

// ──────────────────────────────────────────────────────────────────────────────
// Dynamic trigger adjustment
// ──────────────────────────────────────────────────────────────────────────────

AdaptiveCompactionScheduler::AdaptedConfig
AdaptiveCompactionScheduler::computeAdaptedConfig() const {
    double ema_write = 0.0;
    double ema_read  = 0.0;
    {
        std::lock_guard<std::mutex> lock(rates_mutex_);
        ema_write = ema_write_rate_;
        ema_read  = ema_read_rate_;
    }

    AdaptedConfig cfg;

    // Normalise write rate to [0, 1] relative to the low-load threshold.
    double write_pressure =
        std::min(1.0, (config_.low_load_write_rate > 0.0)
                          ? ema_write / config_.low_load_write_rate
                          : 0.0);

    // Normalise read rate similarly.
    double read_pressure =
        std::min(1.0, (config_.low_load_read_rate > 0.0)
                          ? ema_read / config_.low_load_read_rate
                          : 0.0);

    // Combined load pressure (write-heavy factor has more weight).
    double pressure = 0.7 * write_pressure + 0.3 * read_pressure;

    // Tombstone threshold: raise it under load (delay GC), lower it when idle
    // (compact eagerly).  Linear interpolation between min and max.
    auto min_t = static_cast<double>(config_.min_tombstone_threshold);
    auto max_t = static_cast<double>(config_.max_tombstone_threshold);
    cfg.tombstone_gc_threshold =
        static_cast<uint64_t>(min_t + pressure * (max_t - min_t));

    // GC interval: longer under load, shorter when idle.
    double min_s = static_cast<double>(config_.min_gc_interval.count());
    double max_s = static_cast<double>(config_.max_gc_interval.count());
    cfg.bg_gc_interval =
        std::chrono::seconds(static_cast<std::chrono::seconds::rep>(min_s + pressure * (max_s - min_s)));

    // Full compaction is only enabled during low-load windows.
    cfg.enable_full_compaction = (pressure < 0.2);

    return cfg;
}

AdaptiveCompactionScheduler::AdaptedConfig
AdaptiveCompactionScheduler::getAdaptedConfig() const {
    return computeAdaptedConfig();
}

void AdaptiveCompactionScheduler::applyAdaptedConfig(CompactionManager& mgr) {
    auto adapted = computeAdaptedConfig();

    CompactionManager::Config new_cfg = mgr.getConfig();
    new_cfg.tombstone_gc_threshold = adapted.tombstone_gc_threshold;
    new_cfg.bg_gc_interval         = adapted.bg_gc_interval;
    new_cfg.enable_full_compaction = adapted.enable_full_compaction;
    mgr.setConfig(new_cfg);

    trigger_adjustments_.fetch_add(1, std::memory_order_relaxed);
}

// ──────────────────────────────────────────────────────────────────────────────
// Metrics
// ──────────────────────────────────────────────────────────────────────────────

AdaptiveCompactionScheduler::Stats AdaptiveCompactionScheduler::stats() const {
    Stats s;
    s.total_reads  = total_reads_.load(std::memory_order_relaxed);
    s.total_writes = total_writes_.load(std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(rates_mutex_);
        s.ema_read_rate  = ema_read_rate_;
        s.ema_write_rate = ema_write_rate_;
    }
    s.compaction_schedules = compaction_schedules_.load(std::memory_order_relaxed);
    s.trigger_adjustments  = trigger_adjustments_.load(std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(samples_mutex_);
        s.sample_count = static_cast<uint32_t>(samples_.size());
    }
    return s;
}

} // namespace themis
