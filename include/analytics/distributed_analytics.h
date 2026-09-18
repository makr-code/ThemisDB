/**
 * @file distributed_analytics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Distributed Analytics Sharding
 *
 * Scatter-gather OLAP execution across cluster nodes.
 *
 * Architecture:
 *   - ShardQueryExecutor: pluggable per-shard execution interface
 *   - DistributedAnalyticsSharding: coordinator that fans out an OLAPQuery
 *     to all (healthy) shards in parallel, then merges the partial results.
 *
 * Merge semantics per aggregate function:
 *   COUNT          – sum of per-shard counts
 *   SUM            – sum of per-shard sums
 *   AVG            – weighted average (sum / count, tracked internally)
 *   MIN            – minimum of per-shard minimums
 *   MAX            – maximum of per-shard maximums
 *   STDDEV         – approximation via combined variance (Chan's formula)
 *   VARIANCE       – approximation via combined variance (Chan's formula)
 *   COUNT_DISTINCT – approximate upper bound (union of per-shard counts)
 *   FIRST          – value from the first responding shard
 *   LAST           – value from the last responding shard
 *   MEDIAN         – approximation (average of per-shard medians)
 *   PERCENTILE     – approximation (average of per-shard percentiles)
 *
 * CUBE / ROLLUP / GROUPING SETS queries are supported: the query is forwarded
 * as-is to each shard and the results are merged by matching dimension values
 * and grouping_id.
 *
 * Thread-safety:
 *   - executeDistributed() is thread-safe (reads topology under shared lock,
 *     dispatches work via std::async).
 *   - addShard() / removeShard() / setExecutor() must not be called
 *     concurrently with executeDistributed().
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "analytics/olap.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace themisdb {
namespace analytics {

// ---------------------------------------------------------------------------
// ShardQueryExecutor interface
// ---------------------------------------------------------------------------

class ShardQueryExecutor {
public:
    /**
     * @brief Shard Query Executor.
     * @return Return value.
     */
    virtual ~ShardQueryExecutor() = default;

    /**
     * @brief Execute.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    virtual themis::analytics::OLAPResult execute(
        const std::string& shard_id,
        const themis::analytics::OLAPQuery& query) = 0;

    virtual bool isHealthy() const { return true; }
};

// ---------------------------------------------------------------------------
// LocalShardExecutor – thin wrapper around an existing OLAPEngine
// ---------------------------------------------------------------------------

class LocalShardExecutor final : public ShardQueryExecutor {
public:
    /**
     * @brief Local Shard Executor.
     * @param[in,out] engine Input/output parameter.
     * @return Return value.
     */
    explicit LocalShardExecutor(themis::analytics::OLAPEngine& engine)
        : engine_(engine) {}

    themis::analytics::OLAPResult execute(
        const std::string& /*shard_id*/,
        const themis::analytics::OLAPQuery& query) override {
        return engine_.execute(query);
    }

private:
    themis::analytics::OLAPEngine& engine_;
};

// ---------------------------------------------------------------------------
// DistributedAnalyticsSharding
// ---------------------------------------------------------------------------

class DistributedAnalyticsSharding {
public:
    struct Config {
        size_t max_parallel_shards = 0;

        bool allow_partial_results = true;

        double max_failure_rate = 0.20;

        uint32_t shard_timeout_ms = 30000;

        uint32_t shard_execution_timeout_ms = 30000;

        std::chrono::milliseconds health_check_interval{5000};


        bool enable_circuit_breaker = true;

        uint32_t circuit_breaker_failure_threshold = 3;

        uint32_t circuit_breaker_recovery_delay_ms = 1000;

        uint32_t circuit_breaker_max_recovery_delay_ms = 30000;

        uint32_t circuit_breaker_recovery_attempts = 2;

        uint32_t max_queued_requests_per_shard = 100;

        uint32_t queue_enqueue_timeout_ms = 100;

        // Wave-A AN1: per-shard retry with exponential backoff
        struct RetryConfig {
            uint32_t max_retries   = 2;
            uint32_t base_delay_ms = 50;
            uint32_t max_delay_ms  = 500;
        };
        RetryConfig retry_config;
    };

    enum class CircuitBreakerState : uint8_t {
        CLOSED = 0,    ///< Normal operation, requests processed.
        OPEN = 1,      ///< Too many failures, requests rejected.
        HALF_OPEN = 2  ///< Attempting recovery, limited requests sent.
    };

    struct CircuitBreakerInfo {
        CircuitBreakerState state = CircuitBreakerState::CLOSED;
        uint32_t consecutive_failures = 0;
        uint32_t recovery_attempts = 0;
        std::chrono::steady_clock::time_point opened_at;
        std::chrono::steady_clock::time_point next_recovery_at;
        uint64_t state_changes = 0;
        std::string last_error;
    };

    struct ShardExecutionInfo {
        std::string shard_id;
        bool success = false;
        std::string error;
        double execution_time_ms = 0.0;
        CircuitBreakerState circuit_state = CircuitBreakerState::CLOSED;
        uint32_t circuit_consecutive_failures = 0;
    };

    struct DistributedResult {
        themis::analytics::OLAPResult merged;
        std::vector<ShardExecutionInfo> shard_info;
        size_t successful_shards = 0;
        size_t total_shards = 0;
        std::string operation_id;
        std::string correlation_id;
        std::string failure_class = "none";
        double total_execution_ms = 0.0;
        double merge_duration_ms = 0.0;
        std::vector<std::string> operator_hints;
    };

    // ------------------------------------------------------------------
    // Construction / destruction
    // ------------------------------------------------------------------

    DistributedAnalyticsSharding();
    /**
     * @brief Distributed Analytics Sharding.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */
    explicit DistributedAnalyticsSharding(const Config& cfg);
    ~DistributedAnalyticsSharding();

    // ------------------------------------------------------------------
    // Shard management
    // ------------------------------------------------------------------

    void addShard(const std::string& shard_id,
                  std::shared_ptr<ShardQueryExecutor> executor,
                  const std::string& tenant_id = {});

    /**
     * @brief Remove Shard.
     * @param[in] shard_id Identifier of the shard.
     */
    void removeShard(const std::string& shard_id);

    /**
     * @brief Get Shard Count.
     * @return Return value.
     */
    size_t getShardCount() const;

    /**
     * @brief Get Healthy Shard Count.
     * @return Return value.
     */
    size_t getHealthyShardCount() const;

    /**
     * @brief Get Healthy Shard Count Async.
     * @return Return value.
     */
    std::future<size_t> getHealthyShardCountAsync() const;

    /**
     * @brief Get Shard Ids.
     * @return Return value.
     */
    std::vector<std::string> getShardIds() const;

    // ------------------------------------------------------------------
    // Query execution
    // ------------------------------------------------------------------

    /**
     * @brief Execute Distributed.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    DistributedResult executeDistributed(
        const themis::analytics::OLAPQuery& query);

    /**
     * @brief Execute.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    themis::analytics::OLAPResult execute(
        const themis::analytics::OLAPQuery& query);

    /**
     * @brief ------------------------------------------------------------------ Result merging (exposed for testing / custom pipelines) ------------------------------------------------------------------
     * @param[in] partials Input parameter.
     * @param[in] query Input parameter.
     * @return Return value.
     */

    static themis::analytics::OLAPResult mergeResults(
        const std::vector<themis::analytics::OLAPResult>& partials,
        const themis::analytics::OLAPQuery& query);

private:
    // Forward declaration for private helpers that take ShardEntry parameters.
    struct ShardEntry;
    // ---------------------------------------------------------------
    // Internal helpers
    // ---------------------------------------------------------------

    /**
     * @brief Row Group Key.
     * @param[in] row Input parameter.
     * @param[in] dims Input parameter.
     * @param[in] grouping_id Identifier of the grouping.
     * @return Return value.
     */
    static std::string rowGroupKey(
        const themis::analytics::OLAPResult::Row& row,
        const std::vector<themis::analytics::Dimension>& dims,
        int64_t grouping_id);


    /**
     * @brief On Shard Success.
     * @param[in,out] entry Input/output parameter.
     */
    void onShardSuccess(ShardEntry& entry);

    /**
     * @brief On Shard Failure.
     * @param[in,out] entry Input/output parameter.
     * @param[in] error_msg Input parameter.
     * @return True when the operation succeeds.
     */
    bool onShardFailure(ShardEntry& entry, const std::string& error_msg);

    /**
     * @brief Update Circuit Breaker State.
     * @param[in,out] entry Input/output parameter.
     * @return Return value.
     */
    CircuitBreakerState updateCircuitBreakerState(ShardEntry& entry);

    bool tryEnqueueRequest(ShardEntry& entry, std::function<void()> task);

    /**
     * @brief Process Queued Requests.
     * @param[in,out] entry Input/output parameter.
     */
    void processQueuedRequests(ShardEntry& entry);

    /**
     * @brief Start Health Monitor.
     */
    void startHealthMonitor();

    /**
     * @brief Run Health Monitor.
     */
    void runHealthMonitor();

    // ---------------------------------------------------------------
    // State
    // ---------------------------------------------------------------

    Config config_;
    mutable std::mutex mutex_;

    struct ShardEntry {
        std::string shard_id;
        std::shared_ptr<ShardQueryExecutor> executor;
        std::string allowed_tenant_id;
        std::shared_ptr<std::atomic<bool>> cached_healthy =
            std::make_shared<std::atomic<bool>>(true);

        std::shared_ptr<std::mutex> circuit_breaker_mutex = std::make_shared<std::mutex>();
        std::shared_ptr<CircuitBreakerInfo> circuit_breaker_info = std::make_shared<CircuitBreakerInfo>();
        std::shared_ptr<std::queue<std::function<void()>>> request_queue = std::make_shared<std::queue<std::function<void()>>>();
        std::shared_ptr<std::mutex> queue_mutex = std::make_shared<std::mutex>();
        std::shared_ptr<std::condition_variable> queue_cv = std::make_shared<std::condition_variable>();
        std::shared_ptr<std::atomic<uint32_t>> in_flight_requests = std::make_shared<std::atomic<uint32_t>>(0);
    };

    std::vector<ShardEntry> shards_;

    // Background health-monitor
    std::atomic<bool>       stopping_{false};
    std::mutex              health_monitor_mutex_;
    std::condition_variable health_monitor_cv_;
    std::thread             health_monitor_thread_;
};

} // namespace analytics
} // namespace themisdb
