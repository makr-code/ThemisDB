/**
 * @file access_metrics.h
 * @brief Unified metrics collection for access model (cache & storage).
 *
 * ThemisDB | File: access_metrics.h | Version: 2.0.0
 * Maturity: 🟡 ALPHA (Phase 2 Implementation) | Status: Active development
 * Author: Copilot | Date: 2026-08-03
 *
 * Provides histogram-based latency tracking, per-key access counters, and
 * an aggregated coordinator metrics class.
 *
 * @see include/access_model/access_coordinator.h
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Latency Histogram
// ============================================================================

/**
 * @brief Fixed-bucket histogram for recording operation latency in microseconds.
 *
 * Thread-unsafe; callers must synchronize externally.
 */
class LatencyHistogram {
public:
    /**
     * @brief Construct a histogram with the given number of buckets and max range.
     *
     * @param num_buckets Number of fixed-width buckets
     * @param max_latency_us Maximum tracked latency in microseconds; values beyond
     *        this are placed in an overflow bucket
     */
    explicit LatencyHistogram(std::size_t num_buckets = 1000,
                              uint64_t max_latency_us = 100000);

    /**
     * @brief Record a single latency observation.
     *
     * @param latency_us Observed latency in microseconds
     */
    void record(uint64_t latency_us);

    /**
     * @brief Compute the Nth percentile latency.
     *
     * @param p Percentile (0–100), e.g. 50 for median, 95 for P95
     * @return Estimated latency at the given percentile in microseconds;
     *         returns 0 if no samples recorded
     */
    [[nodiscard]] uint64_t percentile(double p) const;

    /**
     * @brief Compute the arithmetic mean latency in microseconds.
     */
    [[nodiscard]] double mean() const;

    /**
     * @brief Compute the standard deviation of recorded latencies.
     */
    [[nodiscard]] double stdDev() const;

    /**
     * @brief Return the total number of recorded samples.
     */
    [[nodiscard]] uint64_t count() const noexcept { return count_; }

    /**
     * @brief Return a human-readable summary of histogram statistics.
     */
    [[nodiscard]] std::string describe() const;

    // Accessible to AccessModelMetrics for coordination overhead calculation
    uint64_t sum_latency_us_ = 0;

private:
    std::size_t num_buckets_;
    uint64_t max_latency_us_;
    uint64_t bucket_width_us_;
    std::vector<uint64_t> buckets_;
    uint64_t count_ = 0;
    uint64_t min_latency_us_ = UINT64_MAX;
    uint64_t max_observed_latency_us_ = 0;
};

// ============================================================================
// § 2  Per-Key / Per-Tier Access Metrics
// ============================================================================

/**
 * @brief Access statistics for a single key or tier.
 *
 * Lightweight mutable counter bag; thread-unsafe — callers must synchronize.
 */
struct AccessMetrics {
    uint64_t access_count = 0;    ///< Total get/put accesses
    uint64_t cache_hits = 0;      ///< Accesses satisfied from cache
    uint64_t cache_misses = 0;    ///< Accesses that missed cache
    uint64_t total_accesses = 0;  ///< Combined hit + miss counter
    uint64_t evictions = 0;       ///< Number of evictions observed
    uint64_t promotion_count = 0; ///< Number of promotions triggered
    uint64_t demotion_count = 0;  ///< Number of demotions triggered

    /// Timestamp of most recent access
    std::chrono::system_clock::time_point last_access_time =
        std::chrono::system_clock::now();

    /// Optional per-key latency histogram (null if not enabled)
    std::shared_ptr<LatencyHistogram> latency_histogram;

    /// Record a single access with its latency.
    void recordAccess(uint64_t latency_us);

    /// Record a cache hit.
    void recordCacheHit();

    /// Record a cache miss.
    void recordCacheMiss();

    /// Record an eviction.
    void recordEviction();

    /// Return the cache hit rate [0.0, 1.0]; returns 0 if no accesses recorded.
    [[nodiscard]] double cacheHitRate() const;

    /// Return a human-readable summary.
    [[nodiscard]] std::string describe() const;
};

// ============================================================================
// § 3  Aggregated Coordinator Metrics
// ============================================================================

/**
 * @brief Comprehensive metrics for the AccessCoordinator.
 *
 * Aggregates promotion/demotion counters and latency histograms.
 * Thread-unsafe; coordinators should protect access with their own mutex.
 */
struct AccessOperationCounters {
    uint64_t promotions_initiated = 0;          ///< Promotions queued
    uint64_t promotions_succeeded = 0;          ///< Promotions completed OK
    uint64_t promotions_failed = 0;             ///< Promotions that failed
    uint64_t demotions_initiated = 0;           ///< Demotions planned
    uint64_t demotions_succeeded = 0;           ///< Demotions completed OK
    uint64_t demotions_failed = 0;              ///< Demotions that failed
    uint64_t cache_evictions_observed = 0;      ///< Cache eviction signals received
    uint64_t storage_hot_accesses_observed = 0; ///< Hot-access signals received
};

/**
 * @brief Full coordinator metrics object.
 */
class AccessModelMetrics {
public:
    /**
     * @brief Construct with default histogram parameters.
     */
    AccessModelMetrics();

    // ── Recording ───────────────────────────────────────────────────────────

    /**
     * @brief Record the latency of one event-processing cycle.
     * @param latency_us Latency in microseconds
     */
    void recordEventProcessingLatency(uint64_t latency_us);

    /**
     * @brief Record the end-to-end latency of a tier promotion.
     * @param latency_us Latency in microseconds
     */
    void recordTierPromotionLatency(uint64_t latency_us);

    /**
     * @brief Record the time taken to reach a policy decision.
     * @param latency_us Latency in microseconds
     */
    void recordPolicyDecisionLatency(uint64_t latency_us);

    // ── Queries ─────────────────────────────────────────────────────────────

    /**
     * @brief Estimate the coordinator overhead as a percentage of application
     *        query time (simplified approximation).
     */
    [[nodiscard]] double coordinationOverheadPercent() const;

    /**
     * @brief Return a compact human-readable summary.
     */
    [[nodiscard]] std::string describe() const;

    /**
     * @brief Return a detailed, multi-section report.
     */
    [[nodiscard]] std::string detailedReport() const;

    // ── Public data ──────────────────────────────────────────────────────────

    /// Operation counters (directly mutable by coordinator)
    AccessOperationCounters counters;

    /// Snapshot timestamp
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

    // ── Public latency histograms (directly accessible for testing/inspection) ─

    /// Histogram of coordinator event-processing latencies (µs)
    LatencyHistogram event_processing_latency_us_;

    /// Histogram of end-to-end tier promotion latencies (µs)
    LatencyHistogram tier_promotion_latency_us_;

    /// Histogram of policy decision latencies (µs)
    LatencyHistogram policy_decision_latency_us_;
};

}  // namespace access_model
}  // namespace themis
