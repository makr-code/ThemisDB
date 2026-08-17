/**
 * @file intelligent_prefetcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

namespace themis {
namespace performance {

/**
 * @brief Intelligent Prefetching System (v1.8.0)
 *
 * Machine learning-based prefetcher that learns memory access patterns and
 * proactively loads data to reduce cache miss latency.
 *
 * Research basis: "Learning-based Prefetching" (MICRO'19).
 *
 * Features:
 *  - Pattern Learning:   detects sequential and strided access patterns via a
 *                        sliding window of recent addresses.
 *  - Prefetch Distance:  adaptively adjusts the number of addresses to
 *                        prefetch ahead based on observed access latency.
 *  - Confidence Scoring: only prefetches when the pattern confidence exceeds
 *                        the configured threshold.
 *  - Multi-Level:        routes prefetch requests to L1, L2, L3, or DRAM
 *                        (non-temporal) based on confidence level.
 *  - Feedback Loop:      records hit/miss outcomes to improve accuracy
 *                        estimates reported in PrefetchStats.
 *
 * Thread safety: all public methods are protected by an internal mutex and
 * are safe to call concurrently.
 */
class IntelligentPrefetcher {
public:
    // =========================================================================
    // Configuration
    // =========================================================================

    /** @brief Cache level targeted by a prefetch request. */
    enum class CacheLevel {
        L1,   ///< L1 cache – high temporal locality
        L2,   ///< L2 cache – moderate temporal locality
        L3,   ///< L3 cache – low temporal locality
        DRAM  ///< Non-temporal / streaming – bypass cache hierarchy
    };

    /**
     * @brief Configuration for IntelligentPrefetcher construction.
     *
     * All fields carry production-ready defaults that match the roadmap
     * acceptance criteria.
     */
    struct PrefetchConfig {
        /** Enable online pattern learning; when false only static prefetch
         *  hints are used (based on address arithmetic alone). */
        bool enable_learning = true;

        /** Maximum number of addresses to prefetch ahead per prediction call. */
        size_t max_prefetch_distance = 16;

        /** Confidence threshold in [0, 1].  Only patterns with confidence
         *  >= this value generate prefetch requests. */
        double confidence_threshold = 0.7;

        /** Size of the sliding window (number of addresses) used to learn
         *  access patterns. */
        size_t history_size = 1000;

        /** When true, issue actual CPU prefetch instructions via
         *  __builtin_prefetch / _mm_prefetch.  Setting false is useful for
         *  unit testing without side effects. */
        bool enable_hardware_prefetch = true;
    };

    // =========================================================================
    // Data types
    // =========================================================================

    /**
     * @brief A learned access pattern derived from the address history.
     *
     * The `addresses` vector holds the raw addresses captured in the window.
     * `stride` is the dominant difference between consecutive addresses.
     * `confidence` is in [0, 1] and reflects how consistently the stride was
     * observed.
     */
    struct AccessPattern {
        std::vector<uint64_t> addresses;
        uint64_t              timestamp  = 0;
        uint64_t              stride     = 0;
        double                confidence = 0.0;
    };

    /**
     * @brief Cumulative prefetch statistics.
     *
     * `accuracy`  = useful_prefetches / total_prefetches  (when total > 0)
     * `coverage`  = useful_prefetches / total_accesses    (when total > 0)
     */
    struct PrefetchStats {
        size_t total_prefetches  = 0;
        size_t useful_prefetches = 0;
        size_t wasted_prefetches = 0;
        double accuracy          = 0.0;
        double coverage          = 0.0;
        size_t total_accesses    = 0;
    };

    // =========================================================================
    // Construction
    // =========================================================================

    IntelligentPrefetcher();
    explicit IntelligentPrefetcher(PrefetchConfig config);
    ~IntelligentPrefetcher();

    IntelligentPrefetcher(const IntelligentPrefetcher&)            = delete;
    IntelligentPrefetcher& operator=(const IntelligentPrefetcher&) = delete;
    IntelligentPrefetcher(IntelligentPrefetcher&&)                 noexcept = default;
    IntelligentPrefetcher& operator=(IntelligentPrefetcher&&)      noexcept = default;

    // =========================================================================
    // Core API
    // =========================================================================

    /**
     * @brief Record a memory access and update the pattern model.
     *
     * This is the primary training input for the learning engine.  Each call
     * appends `address` to the sliding history window and re-fits the
     * dominant stride.  When an address matches a prior prediction the
     * useful_prefetches counter is incremented (feedback loop).
     *
     * @param address   64-bit address of the accessed memory location.
     * @param timestamp Access timestamp (nanoseconds since epoch, or any
     *                  monotonic counter).  Used to adapt prefetch distance.
     */
    void record_access(uint64_t address, uint64_t timestamp = 0);

    /**
     * @brief Predict the next `lookahead` addresses after `current_address`.
     *
     * Uses the learned stride to extrapolate future addresses.  Only
     * predictions that exceed the configured confidence threshold are
     * returned.
     *
     * @param current_address  The address from which to project.
     * @param lookahead        Maximum number of future addresses to return.
     *                         Capped at `max_prefetch_distance`.
     * @return Ordered vector of predicted addresses (may be empty when
     *         confidence is too low or the history is insufficient).
     */
    std::vector<uint64_t> predict_next_accesses(uint64_t current_address,
                                                size_t   lookahead = 8);

    /**
     * @brief Issue hardware prefetch instructions for the given addresses.
     *
     * Routes each address to the appropriate cache level:
     *  - L1  when confidence >= 0.9
     *  - L2  when confidence >= 0.75
     *  - L3  when confidence >= confidence_threshold
     *  - DRAM (non-temporal) for very low-confidence streaming patterns
     *
     * Increments `total_prefetches` for each address issued.
     *
     * When `enable_hardware_prefetch` is false the counters are still
     * updated but no CPU instruction is emitted.
     *
     * @param addresses  Addresses to prefetch (typically from
     *                   predict_next_accesses()).
     */
    void prefetch_predicted(const std::vector<uint64_t>& addresses);

    // =========================================================================
    // Pattern inspection
    // =========================================================================

    /**
     * @brief Return the currently learned access pattern.
     *
     * Reflects the state after the most recent record_access() call.
     */
    AccessPattern current_pattern() const;

    /**
     * @brief Return the adaptive prefetch distance computed from recent
     *        inter-access latency.
     *
     * Increases when latency is high (prefetch further ahead) and decreases
     * when latency is low (avoid wasteful prefetching).
     */
    size_t adaptive_prefetch_distance() const;

    // =========================================================================
    // Statistics & feedback
    // =========================================================================

    /** @brief Return a snapshot of current prefetch statistics. */
    PrefetchStats get_stats() const;

    /** @brief Reset statistics without clearing the learned pattern model. */
    void reset_stats();

    /** @brief Reset the full learned state (history + stats). */
    void reset();

    // =========================================================================
    // Config accessor
    // =========================================================================

    /** @brief Return the configuration in effect. */
    const PrefetchConfig& config() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace performance
}  // namespace themis
