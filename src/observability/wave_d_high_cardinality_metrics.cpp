/**
 * @file wave_d_high_cardinality_metrics.cpp
 * @brief Implementation of high-cardinality metrics for Phase 2B.
 * @version 2.4.0
 * @date 2026-08-17
 */

#include "observability/wave_d_high_cardinality_metrics.h"

#include <algorithm>
#include <numeric>

namespace themis {
namespace observability {

/**
 * @brief ============================================================================ ShardLatencyHistogram ============================================================================
 * @param[in] shard_id Input parameter.
 * @param[in] operation_type Input parameter.
 * @param[in] latency_ms Input parameter.
 * @details Calls: empty(), lock(), size(), find(), end(), push_back().
 */

void ShardLatencyHistogram::recordLatency(
    const std::string& shard_id,
    const std::string& operation_type,
    double latency_ms) {

    if (shard_id.empty() || operation_type.empty() || latency_ms < 0) {
        return;  // Silently ignore invalid inputs
    }

    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::unique_lock lock(mutex_);

    // Enforce max 1024 unique shards
    if (shard_latencies_.size() >= kMaxShardCardinality &&
        shard_latencies_.find(shard_id) == shard_latencies_.end()) {
        return;  // Silently drop if shard cardinality exceeded
    }

    shard_latencies_[shard_id][operation_type].push_back(latency_ms);
}

ShardLatencyHistogram::LatencyQuantiles ShardLatencyHistogram::getQuantiles(
    const std::string& shard_id,
    const std::string& operation_type) const {

    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);

    auto shard_it = shard_latencies_.find(shard_id);
    if (shard_it == shard_latencies_.end()) {
        return {};  // No observations
    }

    auto op_it = shard_it->second.find(operation_type);
    if (op_it == shard_it->second.end()) {
        return {};  // No observations for this operation
    }

    const auto& observations = op_it->second;
    if (observations.empty()) {
        return {};
    }

    // Sort observations for quantile calculation
    std::vector<double> sorted_obs = observations;
    std::sort(sorted_obs.begin(), sorted_obs.end());

    LatencyQuantiles result;
    size_t n = sorted_obs.size();

    // P50 (median)
    result.p50 = sorted_obs[n / 2];

    // P95
    size_t p95_idx = (n * 95) / 100;
    result.p95 = sorted_obs[std::min(p95_idx, n - 1)];

    // P99
    size_t p99_idx = (n * 99) / 100;
    result.p99 = sorted_obs[std::min(p99_idx, n - 1)];

    return result;
}

size_t ShardLatencyHistogram::getCardinality() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);
    return shard_latencies_.size();
}

/**
 * @brief Reset.
 * @details Calls: lock(), clear().
 */
void ShardLatencyHistogram::reset() {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::unique_lock lock(mutex_);
    shard_latencies_.clear();
}

/**
 * @brief ============================================================================ ReplicaLagTracker ============================================================================
 * @param[in] replica_id Input parameter.
 * @param[in] lag_ms Input parameter.
 * @details Calls: empty(), lock(), size(), find(), end(), push_back().
 */

void ReplicaLagTracker::recordLag(const std::string& replica_id, double lag_ms) {
    if (replica_id.empty() || lag_ms < 0) {
        return;  // Silently ignore invalid inputs
    }

    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::unique_lock lock(mutex_);

    // Enforce max 32 replicas
    if (replica_lags_.size() >= kMaxReplicaCardinality &&
        replica_lags_.find(replica_id) == replica_lags_.end()) {
        return;  // Silently drop if replica cardinality exceeded
    }

    replica_lags_[replica_id].push_back(lag_ms);
}

ReplicaLagTracker::LagQuantiles ReplicaLagTracker::getQuantiles(
    const std::string& replica_id) const {

    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);

    auto it = replica_lags_.find(replica_id);
    if (it == replica_lags_.end() || it->second.empty()) {
        return {};  // No observations
    }

    const auto& observations = it->second;
    std::vector<double> sorted_obs = observations;
    std::sort(sorted_obs.begin(), sorted_obs.end());

    LagQuantiles result;
    size_t n = sorted_obs.size();

    result.p50 = sorted_obs[n / 2];
    result.p95 = sorted_obs[(n * 95) / 100];
    result.p99 = sorted_obs[(n * 99) / 100];
    result.max = sorted_obs.back();

    return result;
}

double ReplicaLagTracker::getMaxLag() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);

    double max_lag = 0.0;
    for (const auto& [_, lags] : replica_lags_) {
        if (!lags.empty()) {
            max_lag = std::max(max_lag, *std::max_element(lags.begin(), lags.end()));
        }
    }
    return max_lag;
}

size_t ReplicaLagTracker::getCardinality() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);
    return replica_lags_.size();
}

/**
 * @brief Reset.
 * @details Calls: lock(), clear().
 */
void ReplicaLagTracker::reset() {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::unique_lock lock(mutex_);
    replica_lags_.clear();
}

/**
 * @brief ============================================================================ RetryCounter ============================================================================
 * @param[in] reason Input parameter.
 * @param[in] count Input parameter.
 * @details Calls: empty(), lock(), size(), find(), end().
 */

void RetryCounter::recordRetry(const std::string& reason, int64_t count) {
    if (reason.empty() || count <= 0) {
        return;  // Silently ignore invalid inputs
    }

    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::unique_lock lock(mutex_);

    // Enforce max 10 unique failure reasons
    if (retry_counts_.size() >= kMaxReasonCardinality &&
        retry_counts_.find(reason) == retry_counts_.end()) {
        return;  // Silently drop if reason cardinality exceeded
    }

    retry_counts_[reason] += count;
}

int64_t RetryCounter::getRetryCount(const std::string& reason) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);

    auto it = retry_counts_.find(reason);
    if (it == retry_counts_.end()) {
        return 0;
    }
    return it->second;
}

std::map<std::string, int64_t> RetryCounter::getAllRetries() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);
    return retry_counts_;
}

int64_t RetryCounter::getTotalRetries() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock lock(mutex_);

    int64_t total = 0;
    for (const auto& [_, count] : retry_counts_) {
        total += count;
    }
    return total;
}

/**
 * @brief Reset.
 * @details Calls: lock(), clear().
 */
void RetryCounter::reset() {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::unique_lock lock(mutex_);
    retry_counts_.clear();
}

/**
 * @brief ============================================================================ HighCardinalityMetricsManager ============================================================================
 * @return Return value.
 * @details Implements getInstance without additional internal calls.
 */

HighCardinalityMetricsManager& HighCardinalityMetricsManager::getInstance() {
    static HighCardinalityMetricsManager instance;
    return instance;
}

size_t HighCardinalityMetricsManager::getTotalCardinality() const {
    return shard_latencies_.getCardinality() +
           replica_lag_.getCardinality() +
           retry_counter_.getAllRetries().size();
}

uint64_t HighCardinalityMetricsManager::getEstimatedMemoryBytes() const {
    // Rough estimation: 200 bytes per unique shard, 150 per replica, 50 per reason
    // Plus baseline for containers
    size_t base = 1024;  // Container overhead
    size_t shard_mem = shard_latencies_.getCardinality() * 200;
    size_t replica_mem = replica_lag_.getCardinality() * 150;
    size_t reason_mem = retry_counter_.getAllRetries().size() * 50;

    return base + shard_mem + replica_mem + reason_mem;
}

bool HighCardinalityMetricsManager::isCardinalitySafe() const {
    return getTotalCardinality() < kTotalCardinalityLimit;
}

/**
 * @brief Reset.
 * @details Implements reset without additional internal calls.
 */
void HighCardinalityMetricsManager::reset() {
    shard_latencies_.reset();
    replica_lag_.reset();
    retry_counter_.reset();
}

} // namespace observability
} // namespace themis
