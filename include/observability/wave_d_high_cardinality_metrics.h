/**
 * @file wave_d_high_cardinality_metrics.h
 * @brief Wave D Phase 2B: High-cardinality metrics collection with bounded memory.
 * @version 2.4.0
 * @date 2026-08-17
 *
 * Extends MetricsCollector with shard histograms, replica-lag quantiles,
 * and retry counters. Enforces cardinality bounds to prevent OOM under
 * high-volume metric emission.
 *
 * Wave D Phase 2B Gate: W4A-METRICS-01 (memory bounded, no OOM at 50k dimensions)
 *
 * ## Cardinality Bounds
 *
 * - `shard_latency{shard_id}`: max dimensions = cluster topology size (≤ 1024)
 * - `replica_lag_ms{replica_id}`: max dimensions = replica set size (≤ 32)
 * - `retry_counter{reason}`: max dimensions = enumerated failure reasons (≤ 10)
 * - Total high-cardinality metrics: ≤ 50k unique label combinations
 * - Memory budget: ≤ 512 MB at 50k cardinality
 *
 * @see tests/observability/test_high_cardinality_metrics.cpp
 */

#pragma once

#include "observability/metrics_collector.h"

#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <memory>
#include <shared_mutex>

namespace themis {
namespace observability {

/**
 * @brief Shard-level latency histogram with bounded cardinality.
 *
 * Records per-shard operation latencies (e.g., write latency, query latency).
 * Cardinality is bounded by the number of shards in the cluster.
 */
class ShardLatencyHistogram {
public:
    /**
     * @brief Record a latency observation for a shard operation.
     *
     * @param shard_id Shard identifier (e.g., "shard_001").
     * @param operation_type Type of operation (e.g., "write", "read", "scan").
     * @param latency_ms Observed latency in milliseconds.
     *
     * @note Thread-safe; multiple threads may record observations concurrently.
     * @note Shard cardinality is enforced: max 1024 unique shard IDs.
     * @note Silent drop if cardinality limit exceeded.
     */
    void recordLatency(
        const std::string& shard_id,
        const std::string& operation_type,
        double latency_ms);

    /**
     * @brief Get quantile latencies for a specific shard and operation.
     *
     * Returns the p50, p95, p99 latencies observed for the given shard and operation.
     *
     * @param shard_id Shard identifier.
     * @param operation_type Operation type.
     * @return Struct with p50, p95, p99 fields (in milliseconds). Returns zeros if no observations.
     */
    struct LatencyQuantiles {
        double p50{0.0};
        double p95{0.0};
        double p99{0.0};
    };

    LatencyQuantiles getQuantiles(
        const std::string& shard_id,
        const std::string& operation_type) const;

    /**
     * @brief Get the cardinality (unique shard IDs observed).
     * @return Number of unique shards in the histogram.
     */
    size_t getCardinality() const;

    /**
     * @brief Reset all collected histogram data.
     */
    void reset();

private:
    mutable std::shared_mutex mutex_;

    // Per-shard-operation histograms: map<shard_id, map<operation_type, vector<observations>>>
    std::map<std::string, std::map<std::string, std::vector<double>>> shard_latencies_;

    // Cardinality limit (max unique shards)
    static constexpr size_t kMaxShardCardinality = 1024;
};

/**
 * @brief Replica-level lag tracker with quantile reporting.
 *
 * Tracks replication lag between primary and replicas. Useful for monitoring
 * cross-region replication delay and failover readiness.
 */
class ReplicaLagTracker {
public:
    /**
     * @brief Record a replication lag observation.
     *
     * @param replica_id Replica identifier (e.g., "replica_001", "replica_us_west_2").
     * @param lag_ms Observed replication lag in milliseconds.
     *
     * @note Thread-safe.
     * @note Replica cardinality is enforced: max 32 replicas.
     * @note Silent drop if cardinality limit exceeded.
     */
    void recordLag(const std::string& replica_id, double lag_ms);

    /**
     * @brief Get lag quantiles for a specific replica.
     *
     * @param replica_id Replica identifier.
     * @return Struct with p50, p95, p99 lag values (in milliseconds).
     */
    struct LagQuantiles {
        double p50{0.0};
        double p95{0.0};
        double p99{0.0};
        double max{0.0};
    };

    LagQuantiles getQuantiles(const std::string& replica_id) const;

    /**
     * @brief Get the maximum observed lag across all replicas.
     * @return Maximum lag in milliseconds.
     */
    double getMaxLag() const;

    /**
     * @brief Get the cardinality (unique replica IDs observed).
     * @return Number of unique replicas tracked.
     */
    size_t getCardinality() const;

    /**
     * @brief Reset all collected lag data.
     */
    void reset();

private:
    mutable std::shared_mutex mutex_;

    // Per-replica lag observations: map<replica_id, vector<observations>>
    std::map<std::string, std::vector<double>> replica_lags_;

    // Cardinality limit (max unique replicas)
    static constexpr size_t kMaxReplicaCardinality = 32;
};

/**
 * @brief Retry counter with failure reason tracking.
 *
 * Tracks retry attempts by failure reason (e.g., "timeout", "connection_reset", "byzantine").
 * Useful for identifying which failure modes are most common.
 */
class RetryCounter {
public:
    /**
     * @brief Increment retry counter for a specific failure reason.
     *
     * @param reason Failure reason (e.g., "timeout", "connection_reset", "byzantine").
     * @param count Number of retry attempts (default 1).
     *
     * @note Thread-safe.
     * @note Reason cardinality is enforced: max 10 unique failure reasons.
     * @note Silent drop if cardinality limit exceeded.
     */
    void recordRetry(const std::string& reason, int64_t count = 1);

    /**
     * @brief Get total retry count for a specific reason.
     *
     * @param reason Failure reason.
     * @return Total number of retries observed for this reason.
     */
    int64_t getRetryCount(const std::string& reason) const;

    /**
     * @brief Get all retry counts (map of reason -> count).
     * @return Map of failure reasons to retry counts.
     */
    std::map<std::string, int64_t> getAllRetries() const;

    /**
     * @brief Get the total number of retries across all reasons.
     * @return Sum of all retry attempts.
     */
    int64_t getTotalRetries() const;

    /**
     * @brief Reset all retry counters.
     */
    void reset();

private:
    mutable std::shared_mutex mutex_;

    // Per-reason retry counters: map<reason, count>
    std::map<std::string, int64_t> retry_counts_;

    // Cardinality limit (max unique failure reasons)
    static constexpr size_t kMaxReasonCardinality = 10;
};

/**
 * @brief High-cardinality metrics manager.
 *
 * Aggregates shard latencies, replica lag, and retry counters with
 * bounded memory and cardinality enforcement.
 *
 * Wave D Phase 2B: Provides metrics collection for observability gates
 * W4A-METRICS-01 (50k cardinality, 512 MB memory bound).
 */
class HighCardinalityMetricsManager {
public:
    /**
     * @brief Get the singleton instance.
     * @return Reference to the high-cardinality metrics manager.
     */
    static HighCardinalityMetricsManager& getInstance();

    // Prevent copying
    HighCardinalityMetricsManager(const HighCardinalityMetricsManager&) = delete;
    HighCardinalityMetricsManager& operator=(const HighCardinalityMetricsManager&) = delete;

    /**
     * @brief Get the shard latency histogram.
     * @return Reference to the ShardLatencyHistogram.
     */
    ShardLatencyHistogram& shardLatencies() { return shard_latencies_; }

    /**
     * @brief Get the replica lag tracker.
     * @return Reference to the ReplicaLagTracker.
     */
    ReplicaLagTracker& replicaLag() { return replica_lag_; }

    /**
     * @brief Get the retry counter.
     * @return Reference to the RetryCounter.
     */
    RetryCounter& retryCounter() { return retry_counter_; }

    /**
     * @brief Get the total cardinality (sum of all unique label combinations).
     * @return Total cardinality across all high-cardinality metrics.
     */
    size_t getTotalCardinality() const;

    /**
     * @brief Get estimated memory usage (bytes).
     *
     * Rough estimate based on number of observations and cardinality.
     * Useful for detecting memory growth patterns.
     *
     * @return Estimated memory usage in bytes.
     */
    uint64_t getEstimatedMemoryBytes() const;

    /**
     * @brief Check if cardinality is within safe bounds.
     *
     * Returns true if total cardinality < 50k (no OOM risk).
     *
     * @return true if cardinality is safe, false if near/at limit.
     */
    bool isCardinalitySafe() const;

    /**
     * @brief Reset all high-cardinality metrics.
     */
    void reset();

private:
    HighCardinalityMetricsManager() = default;
    ~HighCardinalityMetricsManager() = default;

    ShardLatencyHistogram shard_latencies_;
    ReplicaLagTracker replica_lag_;
    RetryCounter retry_counter_;

    // Cardinality limit for the entire high-cardinality metrics subsystem
    static constexpr size_t kTotalCardinalityLimit = 50000;

    // Memory budget
    static constexpr uint64_t kMemoryBudgetBytes = 512 * 1024 * 1024;  // 512 MB
};

} // namespace observability
} // namespace themis
