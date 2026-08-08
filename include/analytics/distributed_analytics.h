/**
 * @file distributed_analytics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: distributed_analytics.h | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 338
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4324 Implement cached health sta... (2026-03-19)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

/**
 * Abstract interface for executing an OLAP query on a single shard.
 *
 * Implementors:
 *   - LocalShardExecutor  – executes via an in-process OLAPEngine (tests /
 *                           single-node mode).
 *   - RemoteShardExecutor – serialises the query and sends it to a remote
 *                           shard over the existing RPC transport (production).
 */
class ShardQueryExecutor {
public:
    virtual ~ShardQueryExecutor() = default;

    /**
     * Execute an OLAP query on the shard and return the partial result.
     *
     * @param shard_id  Identifier of the target shard (informational).
     * @param query     The query to execute.
     * @return Partial OLAPResult for this shard's data partition.
     */
    virtual themis::analytics::OLAPResult execute(
        const std::string& shard_id,
        const themis::analytics::OLAPQuery& query) = 0;

    /**
     * Returns false if the shard is known to be unreachable, so the caller
     * can skip it without paying a timeout penalty.
     */
    virtual bool isHealthy() const { return true; }
};

// ---------------------------------------------------------------------------
// LocalShardExecutor – thin wrapper around an existing OLAPEngine
// ---------------------------------------------------------------------------

/**
 * Executes a query against an in-process OLAPEngine.
 * Useful for single-node mode and unit tests.
 */
class LocalShardExecutor final : public ShardQueryExecutor {
public:
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

/**
 * Coordinator for distributed OLAP analytics across cluster shards.
 *
 * Usage:
 * @code
 *   DistributedAnalyticsSharding das;
 *
 *   // Register a local executor (in tests or single-node mode)
 *   OLAPEngine engine_a, engine_b;
 *   das.addShard("shard_a", std::make_shared<LocalShardExecutor>(engine_a));
 *   das.addShard("shard_b", std::make_shared<LocalShardExecutor>(engine_b));
 *
 *   // Execute a query distributed across all shards
 *   OLAPQuery q;
 *   q.collection = "sales";
 *   q.dimensions  = {{"region", "", true}};
 *   q.measures    = {{"total", "amount", Measure::Function::Sum}};
 *   auto result = das.executeDistributed(q);
 * @endcode
 */
class DistributedAnalyticsSharding {
public:
    /**
     * Configuration knobs.
     */
    struct Config {
        /// Maximum number of in-flight shard requests at a time.
        /// 0 means unlimited (all shards queried concurrently).
        size_t max_parallel_shards = 0;

        /// If true, a partial result is returned even when some shards fail.
        /// The execution_info field in the result will indicate which shards
        /// were skipped.
        bool allow_partial_results = true;

        /// Maximum fraction of shards that may fail before the entire
        /// `executeDistributed()` call is considered failed.
        /// Range: [0.0, 1.0]. Default: 0.20 (tolerate up to 20 % failures).
        /// When `allow_partial_results` is false, this field is not consulted
        /// (a single failure is already fatal).
        double max_failure_rate = 0.20;

        /// Timeout per shard in milliseconds. 0 = no timeout.
        uint32_t shard_timeout_ms = 30000;

        /// Interval between background health-monitor sweeps.
        /// Default: 5 s.  Set to zero to disable the background monitor.
        std::chrono::milliseconds health_check_interval{5000};

        /// ====== SAFETY CONTROLS (Phase 2.2 Hardening) ======

        /// Enable circuit breaker pattern for failed shards.
        bool enable_circuit_breaker = true;

        /// Consecutive failure threshold before opening circuit breaker.
        /// Default: 3 failures in a row.
        uint32_t circuit_breaker_failure_threshold = 3;

        /// Initial delay (ms) before attempting recovery from OPEN state.
        /// Default: 1000 ms. Increases exponentially with backoff.
        uint32_t circuit_breaker_recovery_delay_ms = 1000;

        /// Maximum delay (ms) for recovery backoff to prevent infinite waits.
        /// Default: 30000 ms (30 seconds).
        uint32_t circuit_breaker_max_recovery_delay_ms = 30000;

        /// Maximum number of HALF_OPEN recovery attempts before returning to OPEN.
        /// Default: 2 attempts.
        uint32_t circuit_breaker_recovery_attempts = 2;

        /// Bounded queue: maximum number of queued requests per shard.
        /// 0 means unbounded. Default: 100 requests per shard.
        uint32_t max_queued_requests_per_shard = 100;

        /// Timeout (ms) for enqueuing a request when the queue is full.
        /// 0 means non-blocking (drop if full). Default: 100 ms.
        uint32_t queue_enqueue_timeout_ms = 100;
    };

    /**
     * Circuit breaker states for shard-level fault tolerance.
     *
     * CLOSED: Normal operation. Requests are processed.
     * OPEN: Shard has failed too many times. Requests are rejected immediately.
     * HALF_OPEN: Attempting to recover. Limited requests are sent to probe shard health.
     */
    enum class CircuitBreakerState : uint8_t {
        CLOSED = 0,    ///< Normal operation, requests processed.
        OPEN = 1,      ///< Too many failures, requests rejected.
        HALF_OPEN = 2  ///< Attempting recovery, limited requests sent.
    };

    /**
     * Per-shard circuit breaker state and diagnostics.
     * Tracks consecutive failures, recovery attempts, and state transitions.
     */
    struct CircuitBreakerInfo {
        CircuitBreakerState state = CircuitBreakerState::CLOSED;
        uint32_t consecutive_failures = 0;
        uint32_t recovery_attempts = 0;
        std::chrono::steady_clock::time_point opened_at;
        std::chrono::steady_clock::time_point next_recovery_at;
        uint64_t state_changes = 0;
        std::string last_error;
    };

    /**
     * Per-shard execution information attached to the merged result.
     * Includes circuit breaker state for diagnostics.
     */
    struct ShardExecutionInfo {
        std::string shard_id;
        bool success = false;
        std::string error;
        double execution_time_ms = 0.0;
        CircuitBreakerState circuit_state = CircuitBreakerState::CLOSED;
        uint32_t circuit_consecutive_failures = 0;
    };

    /**
     * Extended result that includes per-shard diagnostics.
     */
    struct DistributedResult {
        themis::analytics::OLAPResult merged;
        std::vector<ShardExecutionInfo> shard_info;
        size_t successful_shards = 0;
        size_t total_shards = 0;
    };

    // ------------------------------------------------------------------
    // Construction / destruction
    // ------------------------------------------------------------------

    DistributedAnalyticsSharding();
    explicit DistributedAnalyticsSharding(const Config& cfg);
    ~DistributedAnalyticsSharding();

    // ------------------------------------------------------------------
    // Shard management
    // ------------------------------------------------------------------

    /**
     * Register a shard and its executor.
     * Overwrites any previously registered executor for the same shard_id.
     *
     * @param shard_id      Unique shard identifier.
     * @param executor      Per-shard query executor.
     * @param tenant_id     Optional tenant this shard exclusively serves.
     *                      Empty string means the shard is accessible to all
     *                      tenants (or tenant isolation is not required).
     */
    void addShard(const std::string& shard_id,
                  std::shared_ptr<ShardQueryExecutor> executor,
                  const std::string& tenant_id = {});

    /**
     * Deregister a shard.
     */
    void removeShard(const std::string& shard_id);

    /** Total number of registered shards. */
    size_t getShardCount() const;

    /**
     * Number of registered shards whose background health monitor last
     * reported as healthy.  Reads a cached atomic flag — does not perform
     * any network I/O; completes in ≤ 2 µs.
     */
    size_t getHealthyShardCount() const;

    /**
     * Asynchronously query live health for all registered shards.
     *
     * Unlike getHealthyShardCount(), this performs real isHealthy() calls
     * without holding the shard registry lock, so it never blocks addShard()
     * or removeShard().  The result is delivered via the returned future.
     */
    std::future<size_t> getHealthyShardCountAsync() const;

    /** Returns all registered shard IDs. */
    std::vector<std::string> getShardIds() const;

    // ------------------------------------------------------------------
    // Query execution
    // ------------------------------------------------------------------

    /**
     * Execute an OLAP query across all healthy shards and merge results.
     *
     * The query is fanned-out to every healthy shard concurrently.
     * Partial results are aggregated using the merge semantics documented in
     * the file header.
     *
     * @param query  The query to execute on each shard.
     * @return Merged DistributedResult.
     */
    DistributedResult executeDistributed(
        const themis::analytics::OLAPQuery& query);

    /**
     * Convenience overload returning only the merged OLAPResult.
     */
    themis::analytics::OLAPResult execute(
        const themis::analytics::OLAPQuery& query);

    // ------------------------------------------------------------------
    // Result merging (exposed for testing / custom pipelines)
    // ------------------------------------------------------------------

    /**
     * Merge a collection of partial OLAPResults into a single result.
     *
     * @param partials   Partial results from individual shards.
     * @param query      Original query (used to determine aggregate semantics).
     * @return Merged OLAPResult.
     */
    static themis::analytics::OLAPResult mergeResults(
        const std::vector<themis::analytics::OLAPResult>& partials,
        const themis::analytics::OLAPQuery& query);

private:
    // ---------------------------------------------------------------
    // Internal helpers
    // ---------------------------------------------------------------

    /**
     * Compute a stable string key for a result row's dimension values.
     * The key encodes grouping_id so that CUBE/ROLLUP subtotals are kept
     * separate from detail rows.
     */
    static std::string rowGroupKey(
        const themis::analytics::OLAPResult::Row& row,
        const std::vector<themis::analytics::Dimension>& dims,
        int64_t grouping_id);

    /// ====== SAFETY CONTROL HELPERS (Phase 2.2) ======

    /**
     * Handle a successful shard execution: reset circuit breaker state to CLOSED.
     * Called after a shard request completes successfully.
     */
    void onShardSuccess(ShardEntry& entry);

    /**
     * Handle a failed shard execution: increment failure count, possibly opening circuit.
     * Called after a shard request fails.
     *
     * @param entry The shard entry.
     * @param error_msg The error message for diagnostics.
     * @return true if the shard is still usable (circuit not OPEN), false if circuit opened.
     */
    bool onShardFailure(ShardEntry& entry, const std::string& error_msg);

    /**
     * Check and update circuit breaker state based on recovery timing.
     * Transitions OPEN → HALF_OPEN if recovery delay has elapsed.
     *
     * @param entry The shard entry.
     * @return Current circuit breaker state after potential transition.
     */
    CircuitBreakerState updateCircuitBreakerState(ShardEntry& entry);

    /**
     * Attempt to enqueue a request for a shard with bounded queue enforcement.
     * Returns true if enqueued, false if queue full and timeout exceeded.
     *
     * @param entry The shard entry.
     * @param task The task to enqueue.
     * @return true if successfully enqueued, false if queue full and timeout expired.
     */
    bool tryEnqueueRequest(ShardEntry& entry, std::function<void()> task);

    /**
     * Process queued requests for a shard after a request completes.
     *
     * @param entry The shard entry.
     */
    void processQueuedRequests(ShardEntry& entry);

    /** Start the background health-monitor thread (if interval > 0). */
    void startHealthMonitor();

    /** Entry-point for the background health-monitor thread. */
    void runHealthMonitor();

    // ---------------------------------------------------------------
    // State
    // ---------------------------------------------------------------

    Config config_;
    mutable std::mutex mutex_;

    struct ShardEntry {
        std::string shard_id;
        std::shared_ptr<ShardQueryExecutor> executor;
        /// If non-empty, only queries whose `tenant_id` matches are allowed
        /// on this shard.  Empty = accessible to all tenants.
        std::string allowed_tenant_id;
        /// Cached health flag updated by the background monitor.
        /// Initialised to true (optimistic) when a shard is first added.
        std::shared_ptr<std::atomic<bool>> cached_healthy =
            std::make_shared<std::atomic<bool>>(true);

        /// ====== SAFETY CONTROLS: Circuit Breaker State ======
        /// Tracks shard-level fault tolerance state and recovery.
        std::shared_ptr<std::mutex> circuit_breaker_mutex = std::make_shared<std::mutex>();
        std::shared_ptr<CircuitBreakerInfo> circuit_breaker_info = std::make_shared<CircuitBreakerInfo>();
        /// Bounded queue: pending requests waiting to be executed.
        std::shared_ptr<std::queue<std::function<void()>>> request_queue = std::make_shared<std::queue<std::function<void()>>>();
        /// Queue synchronization
        std::shared_ptr<std::mutex> queue_mutex = std::make_shared<std::mutex>();
        std::shared_ptr<std::condition_variable> queue_cv = std::make_shared<std::condition_variable>();
        /// Current in-flight request count
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
