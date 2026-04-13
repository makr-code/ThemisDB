/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            intelligent_prefetcher.cpp                         ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:33:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     432                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f01d91fb8  2026-03-15  feat(performance): implement Intelligent Prefetching Syst... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2026 ThemisDB
// Licensed under MIT License

/**
 * IntelligentPrefetcher – Implementation (v1.8.0)
 *
 * Design overview
 * ───────────────
 * Pattern Learning
 *   A fixed-size circular deque (history_) stores the last `history_size`
 *   (address, timestamp) pairs.  On every record_access() call the engine
 *   recomputes the dominant stride by scanning the most-recent
 *   ANALYSIS_WINDOW entries, counting how many consecutive differences equal
 *   each candidate stride, and normalising to a [0,1] confidence score.
 *
 * Adaptive Prefetch Distance
 *   Inter-access latency (timestamp delta) is tracked via an Exponential
 *   Moving Average (EMA).  When the EMA exceeds LATENCY_HIGH_NS we
 *   increase adaptive_distance_ (more aggressive prefetch); when it drops
 *   below LATENCY_LOW_NS we reduce it (avoid wasting bandwidth).
 *
 * Confidence Scoring
 *   confidence = (consistent_stride_count) / (window_size - 1).
 *   Only predictions with confidence >= config_.confidence_threshold are
 *   returned from predict_next_accesses() and issued as prefetches.
 *
 * Multi-Level Prefetch Routing
 *   ≥ 0.90 → L1  (__builtin_prefetch(ptr, 0, 3))
 *   ≥ 0.75 → L2  (__builtin_prefetch(ptr, 0, 2))
 *   ≥ 0.60 → L3  (__builtin_prefetch(ptr, 0, 1))
 *   <  0.60 → DRAM/NTA (__builtin_prefetch(ptr, 0, 0))
 *
 * Feedback Loop
 *   pending_predictions_ holds the set of addresses that were issued as
 *   prefetches but not yet confirmed.  Each record_access() call checks
 *   whether the incoming address was in pending_predictions_; if yes,
 *   useful_prefetches is incremented and the address is removed.  Any
 *   entry that is evicted from the set without being confirmed counts as a
 *   wasted prefetch.
 */

#include "performance/intelligent_prefetcher.h"
#include "performance/prefetch_hints.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace performance {

// ─── Implementation details ───────────────────────────────────────────────────

namespace {

// Number of most-recent accesses used for stride analysis.
constexpr size_t ANALYSIS_WINDOW = 32;

// Minimum history required before a stride can be considered reliable.
constexpr size_t MIN_HISTORY_FOR_STRIDE = 4;

// Latency thresholds in nanoseconds for adaptive distance tuning.
constexpr double LATENCY_HIGH_NS = 100.0;  // > 100 ns → increase distance
constexpr double LATENCY_LOW_NS  =  20.0;  //  < 20 ns → decrease distance

// EMA smoothing factor for latency (α = 0.125 ≈ 1/8).
constexpr double LATENCY_EMA_ALPHA = 0.125;

// Maximum pending prediction set size (prevents unbounded growth).
constexpr size_t MAX_PENDING_PREDICTIONS = 4096;

}  // namespace

// ─── Impl class ───────────────────────────────────────────────────────────────

class IntelligentPrefetcher::Impl {
public:
    struct AccessEntry {
        uint64_t address;
        uint64_t timestamp;
    };

    explicit Impl(PrefetchConfig cfg)
        : config_(std::move(cfg)),
          adaptive_distance_(std::min(size_t{8}, config_.max_prefetch_distance)),
          latency_ema_ns_(0.0) {}

    // ── record_access ────────────────────────────────────────────────────────

    void record_access(uint64_t address, uint64_t timestamp) {
        std::lock_guard<std::mutex> lk(mu_);

        stats_.total_accesses++;

        // Feedback loop: check if this address was predicted.
        auto it = pending_.find(address);
        if (it != pending_.end()) {
            stats_.useful_prefetches++;
            pending_.erase(it);
        }

        // Update latency EMA (only when we have a previous entry with a real
        // timestamp to compare against).
        if (!history_.empty() && timestamp > 0 && history_.back().timestamp > 0
                && timestamp > history_.back().timestamp) {
            double delta_ns = static_cast<double>(timestamp - history_.back().timestamp);
            if (latency_ema_ns_ == 0.0) {
                latency_ema_ns_ = delta_ns;
            } else {
                latency_ema_ns_ = LATENCY_EMA_ALPHA * delta_ns
                                + (1.0 - LATENCY_EMA_ALPHA) * latency_ema_ns_;
            }
            // Adjust adaptive distance.
            if (latency_ema_ns_ > LATENCY_HIGH_NS) {
                adaptive_distance_ = std::min(adaptive_distance_ + 1,
                                             config_.max_prefetch_distance);
            } else if (latency_ema_ns_ < LATENCY_LOW_NS && adaptive_distance_ > 1) {
                adaptive_distance_--;
            }
        }

        // Append to sliding history window.
        history_.push_back({address, timestamp});
        if (history_.size() > config_.history_size) {
            history_.pop_front();
        }

        // Re-analyse if learning is enabled.
        if (config_.enable_learning) {
            analyse_pattern();
        }
    }

    // ── predict_next_accesses ────────────────────────────────────────────────

    std::vector<uint64_t> predict_next_accesses(uint64_t current_address,
                                                size_t   lookahead) {
        std::lock_guard<std::mutex> lk(mu_);

        if (pattern_.confidence < config_.confidence_threshold) {
            return {};
        }
        if (pattern_.stride == 0) {
            return {};
        }

        size_t count = std::min(lookahead, adaptive_distance_);
        count = std::min(count, config_.max_prefetch_distance);

        std::vector<uint64_t> predictions;
        predictions.reserve(count);
        auto addr = static_cast<int64_t>(current_address);
        for (size_t i = 0; i < count; ++i) {
            addr += signed_stride_;
            predictions.push_back(static_cast<uint64_t>(addr));
        }
        return predictions;
    }

    // ── prefetch_predicted ───────────────────────────────────────────────────

    void prefetch_predicted(const std::vector<uint64_t>& addresses) {
        std::lock_guard<std::mutex> lk(mu_);

        for (uint64_t addr : addresses) {
            stats_.total_prefetches++;

            // Determine target cache level from current pattern confidence.
            CacheLevel level = confidence_to_level(pattern_.confidence);

            if (config_.enable_hardware_prefetch) {
                issue_prefetch(addr, level);
            }

            // Track in pending set for the feedback loop.
            if (pending_.size() < MAX_PENDING_PREDICTIONS) {
                pending_.insert(addr);
            } else {
                // Set is full – evict one arbitrary entry as wasted.
                stats_.wasted_prefetches++;
                pending_.erase(pending_.begin());
                pending_.insert(addr);
            }
        }

        update_derived_stats();
    }

    // ── current_pattern ──────────────────────────────────────────────────────

    AccessPattern current_pattern() const {
        std::lock_guard<std::mutex> lk(mu_);
        return pattern_;
    }

    // ── adaptive_prefetch_distance ───────────────────────────────────────────

    size_t adaptive_prefetch_distance() const {
        std::lock_guard<std::mutex> lk(mu_);
        return adaptive_distance_;
    }

    // ── stats ────────────────────────────────────────────────────────────────

    PrefetchStats get_stats() const {
        std::lock_guard<std::mutex> lk(mu_);
        PrefetchStats s = stats_;
        if (s.total_prefetches > 0) {
            s.accuracy = static_cast<double>(s.useful_prefetches) /
                         static_cast<double>(s.total_prefetches);
        }
        if (s.total_accesses > 0) {
            s.coverage = static_cast<double>(s.useful_prefetches) /
                         static_cast<double>(s.total_accesses);
        }
        return s;
    }

    void reset_stats() {
        std::lock_guard<std::mutex> lk(mu_);
        stats_ = {};
        pending_.clear();
    }

    void reset() {
        std::lock_guard<std::mutex> lk(mu_);
        stats_            = {};
        pattern_          = {};
        history_.clear();
        pending_.clear();
        latency_ema_ns_   = 0.0;
        adaptive_distance_ = std::min(size_t{8}, config_.max_prefetch_distance);
    }

    const PrefetchConfig& config() const noexcept { return config_; }

private:
    // ── Internals ─────────────────────────────────────────────────────────────

    /**
     * Recompute pattern_.stride and pattern_.confidence from the most-recent
     * ANALYSIS_WINDOW entries.
     *
     * Algorithm:
     *  1. Gather up to ANALYSIS_WINDOW addresses from the tail of history_.
     *  2. Compute differences between consecutive addresses.
     *  3. Find the modal (most-frequent) difference → candidate stride.
     *  4. Confidence = (count of differences == stride) / (window - 1).
     *
     * A stride of 0 (duplicate addresses) or very rare strides are treated
     * as no-pattern (confidence = 0).
     */
    void analyse_pattern() {
        if (history_.size() < MIN_HISTORY_FOR_STRIDE) {
            pattern_.confidence = 0.0;
            pattern_.stride     = 0;
            return;
        }

        // Collect the tail of the history window.
        size_t start = (history_.size() > ANALYSIS_WINDOW)
                           ? history_.size() - ANALYSIS_WINDOW
                           : 0;
        size_t window = history_.size() - start;  // ≥ MIN_HISTORY_FOR_STRIDE

        // Compute signed differences (cast to int64 to handle backward strides).
        std::unordered_map<int64_t, size_t> stride_counts;
        for (size_t i = start + 1; i < history_.size(); ++i) {
            int64_t diff = static_cast<int64_t>(history_[i].address) -
                           static_cast<int64_t>(history_[i - 1].address);
            stride_counts[diff]++;
        }

        // Find modal stride.
        int64_t best_stride = 0;
        size_t  best_count  = 0;
        for (auto& [stride, count] : stride_counts) {
            if (count > best_count && stride != 0) {
                best_count  = count;
                best_stride = stride;
            }
        }

        double denominator = static_cast<double>(window - 1);
        double confidence  = (denominator > 0.0)
                                 ? static_cast<double>(best_count) / denominator
                                 : 0.0;

        // Store uint64 magnitude; direction is embedded in the extrapolation.
        pattern_.stride     = static_cast<uint64_t>(std::abs(best_stride));
        pattern_.confidence = confidence;
        // Store the latest address set (for inspection).
        pattern_.addresses.clear();
        for (size_t i = start; i < history_.size(); ++i) {
            pattern_.addresses.push_back(history_[i].address);
        }
        if (!history_.empty()) {
            pattern_.timestamp = history_.back().timestamp;
        }

        // Preserve stride sign in predict_next_accesses by storing the signed
        // stride directly.  We use a signed helper field via stride_ (int64).
        signed_stride_ = best_stride;
    }

    static CacheLevel confidence_to_level(double confidence) noexcept {
        if (confidence >= 0.90) return CacheLevel::L1;
        if (confidence >= 0.75) return CacheLevel::L2;
        if (confidence >= 0.60) return CacheLevel::L3;
        return CacheLevel::DRAM;
    }

    static void issue_prefetch(uint64_t addr, CacheLevel level) noexcept {
        const void* ptr = reinterpret_cast<const void*>(static_cast<uintptr_t>(addr));
        switch (level) {
            case CacheLevel::L1:
                prefetch(ptr, PrefetchHint::T0);
                break;
            case CacheLevel::L2:
                prefetch(ptr, PrefetchHint::T1);
                break;
            case CacheLevel::L3:
                prefetch(ptr, PrefetchHint::T2);
                break;
            case CacheLevel::DRAM:
                prefetch(ptr, PrefetchHint::NTA);
                break;
        }
    }

    void update_derived_stats() {
        if (stats_.total_prefetches > 0) {
            stats_.accuracy = static_cast<double>(stats_.useful_prefetches) /
                              static_cast<double>(stats_.total_prefetches);
        }
        if (stats_.total_accesses > 0) {
            stats_.coverage = static_cast<double>(stats_.useful_prefetches) /
                              static_cast<double>(stats_.total_accesses);
        }
    }

    // ── Members ───────────────────────────────────────────────────────────────

    mutable std::mutex mu_;

    PrefetchConfig config_;

    std::deque<AccessEntry> history_;
    AccessPattern           pattern_;
    int64_t                 signed_stride_ = 0;  ///< Signed stride for direction

    size_t adaptive_distance_;
    double latency_ema_ns_;

    PrefetchStats                       stats_;
    std::unordered_set<uint64_t>        pending_;  ///< Outstanding predictions
};

// ─── IntelligentPrefetcher public interface ───────────────────────────────────

IntelligentPrefetcher::IntelligentPrefetcher()
    : impl_(std::make_unique<Impl>(PrefetchConfig{})) {}

IntelligentPrefetcher::IntelligentPrefetcher(PrefetchConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

IntelligentPrefetcher::~IntelligentPrefetcher() = default;

void IntelligentPrefetcher::record_access(uint64_t address, uint64_t timestamp) {
    impl_->record_access(address, timestamp);
}

std::vector<uint64_t> IntelligentPrefetcher::predict_next_accesses(
    uint64_t current_address, size_t lookahead) {
    return impl_->predict_next_accesses(current_address, lookahead);
}

void IntelligentPrefetcher::prefetch_predicted(const std::vector<uint64_t>& addresses) {
    impl_->prefetch_predicted(addresses);
}

IntelligentPrefetcher::AccessPattern IntelligentPrefetcher::current_pattern() const {
    return impl_->current_pattern();
}

size_t IntelligentPrefetcher::adaptive_prefetch_distance() const {
    return impl_->adaptive_prefetch_distance();
}

IntelligentPrefetcher::PrefetchStats IntelligentPrefetcher::get_stats() const {
    return impl_->get_stats();
}

void IntelligentPrefetcher::reset_stats() {
    impl_->reset_stats();
}

void IntelligentPrefetcher::reset() {
    impl_->reset();
}

const IntelligentPrefetcher::PrefetchConfig& IntelligentPrefetcher::config() const noexcept {
    return impl_->config();
}

}  // namespace performance
}  // namespace themis
