/**
 * @file access_metrics.h
 * @brief Unified metrics collection for access model (cache & storage).
 *
 * ThemisDB | File: access_metrics.h | Version: 1.0.0
 * Maturity: 🟡 ALPHA (Phase 1 API Definition) | Status: Frozen for v1.x
 * Author: Copilot | Date: 2026-08-03
 *
 * @see include/access_model/access_coordinator.h
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace access_model {

/**
 * @brief Counters for promotion/demotion operations.
 */
struct AccessOperationCounters {
    /// Number of promotion operations initiated
    uint64_t promotions_initiated = 0;

    /// Number of promotions completed successfully
    uint64_t promotions_succeeded = 0;

    /// Number of promotions that failed
    uint64_t promotions_failed = 0;

    /// Number of demotion operations initiated
    uint64_t demotions_initiated = 0;

    /// Number of demotions completed successfully
    uint64_t demotions_succeeded = 0;

    /// Number of demotions that failed
    uint64_t demotions_failed = 0;

    /// Number of eviction signals from cache
    uint64_t cache_evictions_observed = 0;

    /// Number of hot-access signals from storage
    uint64_t storage_hot_accesses_observed = 0;
};

/**
 * @brief Latency statistics for access operations.
 */
struct AccessLatencyStats {
    /// Minimum latency observed (µs for cache, ms for storage)
    std::chrono::microseconds min_latency;

    /// Maximum latency observed
    std::chrono::microseconds max_latency;

    /// Average latency (arithmetic mean)
    std::chrono::microseconds avg_latency;

    /// P50 latency (median)
    std::chrono::microseconds p50_latency;

    /// P95 latency
    std::chrono::microseconds p95_latency;

    /// P99 latency
    std::chrono::microseconds p99_latency;

    /// Number of samples in statistics
    uint64_t sample_count = 0;
};

/**
 * @brief Comprehensive metrics for the access model.
 *
 * Aggregates statistics across all tiers and operations.
 */
struct AccessModelMetrics {
    /// Snapshot timestamp
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

    /// Operation counters
    AccessOperationCounters counters;

    /// Promotion latency statistics
    AccessLatencyStats promotion_latencies;

    /// Demotion latency statistics
    AccessLatencyStats demotion_latencies;

    /// Cache eviction signal latency (eviction to coordinator processing)
    AccessLatencyStats eviction_signal_latencies;

    /// Storage access signal latency (access detection to coordinator notification)
    AccessLatencyStats storage_signal_latencies;

    /// Correlation between cache eviction and storage demotion latency (ms)
    double eviction_demotion_correlation = 0.0;

    /// Overall system overhead (% of application query time)
    double coordinator_overhead_percent = 0.0;

    /// Number of keys currently tracked
    uint64_t tracked_keys_count = 0;

    /// Total bytes managed across all tiers
    uint64_t total_managed_bytes = 0;
};

}  // namespace access_model
}  // namespace themis

#endif  // THEMISDB_INCLUDE_ACCESS_MODEL_ACCESS_METRICS_H
