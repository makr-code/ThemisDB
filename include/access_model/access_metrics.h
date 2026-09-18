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

class LatencyHistogram {
public:
    explicit LatencyHistogram(std::size_t num_buckets = 1000,
                              uint64_t max_latency_us = 100000);

    /**
     * @brief Record.
     * @param[in] latency_us Input parameter.
     */
    void record(uint64_t latency_us);

    [[nodiscard]] uint64_t percentile(double p) const;

    [[nodiscard]] double mean() const;

    [[nodiscard]] double stdDev() const;

    [[nodiscard]] uint64_t count() const noexcept { return count_; }

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

struct AccessMetrics {
    uint64_t access_count = 0;    ///< Total get/put accesses
    uint64_t cache_hits = 0;      ///< Accesses satisfied from cache
    uint64_t cache_misses = 0;    ///< Accesses that missed cache
    uint64_t total_accesses = 0;  ///< Combined hit + miss counter
    uint64_t evictions = 0;       ///< Number of evictions observed
    uint64_t promotion_count = 0; ///< Number of promotions triggered
    uint64_t demotion_count = 0;  ///< Number of demotions triggered

    std::chrono::system_clock::time_point last_access_time =
        std::chrono::system_clock::now();

    std::shared_ptr<LatencyHistogram> latency_histogram;

    /**
     * @brief Record Access.
     * @param[in] latency_us Input parameter.
     */
    void recordAccess(uint64_t latency_us);

    /**
     * @brief Record Cache Hit.
     */
    void recordCacheHit();

    /**
     * @brief Record Cache Miss.
     */
    void recordCacheMiss();

    /**
     * @brief Record Eviction.
     */
    void recordEviction();

    [[nodiscard]] double cacheHitRate() const;

    [[nodiscard]] std::string describe() const;
};

// ============================================================================
// § 3  Aggregated Coordinator Metrics
// ============================================================================

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

class AccessModelMetrics {
public:
    AccessModelMetrics();


    /**
     * @brief Record Event Processing Latency.
     * @param[in] latency_us Input parameter.
     */
    void recordEventProcessingLatency(uint64_t latency_us);

    /**
     * @brief Record Tier Promotion Latency.
     * @param[in] latency_us Input parameter.
     */
    void recordTierPromotionLatency(uint64_t latency_us);

    /**
     * @brief Record Policy Decision Latency.
     * @param[in] latency_us Input parameter.
     */
    void recordPolicyDecisionLatency(uint64_t latency_us);

    // ── Queries ─────────────────────────────────────────────────────────────

    [[nodiscard]] double coordinationOverheadPercent() const;

    [[nodiscard]] std::string describe() const;

    [[nodiscard]] std::string detailedReport() const;

    // ── Public data ──────────────────────────────────────────────────────────

    AccessOperationCounters counters;

    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

    // ── Public latency histograms (directly accessible for testing/inspection) ─

    LatencyHistogram event_processing_latency_us_;

    LatencyHistogram tier_promotion_latency_us_;

    LatencyHistogram policy_decision_latency_us_;
};

}  // namespace access_model
}  // namespace themis
