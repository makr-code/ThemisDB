/**
 * @file shard_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "sharding/urn_resolver.h"
#include "sharding/remote_executor.h"
#include "sharding/prometheus_metrics.h"
#include "sharding/truetime.h"
#include "sharding/distributed_transaction.h"
#include <string>
#include <atomic>
#include <vector>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/**
 * @brief Routing mode selected for a logical query.
 *
 * The router chooses one of these strategies after inspecting the query text.
 * The selected strategy determines fan-out, merge behavior, and whether the
 * router may need cross-shard coordination.
 */
enum class RoutingStrategy {
    SINGLE_SHARD,     // Query hits one shard (e.g., GET by URN)
    SCATTER_GATHER,   // Query spans all shards (e.g., full table scan)
    NAMESPACE_LOCAL,  // Query scoped to namespace (multi-shard but not all)
    CROSS_SHARD_JOIN  // Join across shards (expensive)
};

/**
 * @brief Result envelope returned for one shard invocation.
 *
 * A result may represent a successful local/remote execution or a transport,
 * timeout, or routing failure. Callers should inspect @ref success before
 * consuming @ref data.
 */
struct ShardResult {
    std::string shard_id;           ///< Owning shard that processed or rejected the request.
    nlohmann::json data;            ///< JSON payload returned by the shard on success.
    bool success = false;           ///< True when the operation completed successfully.
    std::string error_msg;          ///< Diagnostic message when @ref success is false.
    uint64_t execution_time_ms = 0; ///< Observed execution latency in milliseconds.
    uint64_t version_token = 0;     ///< Monotonic merge/version metadata for stale-read detection.
};

/**
 * @brief Routes point operations and queries to the appropriate shard set.
 *
 * The router encapsulates shard resolution, local-vs-remote dispatch,
 * scatter-gather fan-out, result merging, and optional TrueTime-backed
 * distributed transaction coordination.
 */
class ShardRouter {
public:
    /**
     * @brief Runtime configuration for @ref ShardRouter.
     */
    struct Config {
        std::string local_shard_id;      ///< Identifier of the shard hosting this router instance.
        
        uint32_t scatter_timeout_ms = 30000;     ///< Per-batch wait timeout for scatter-gather futures.
        size_t max_concurrent_shards = 10;       ///< Upper bound for concurrent shard RPCs in one batch.
        
        bool enable_query_pushdown = true;       ///< Enables predicate pushdown into shard-local execution.
        bool enable_result_caching = false;      ///< Enables router-side result caching when supported.
    };
    
    /**
     * Construct Shard Router
     * @param resolver URN resolver for shard location
     * @param executor Remote executor for shard communication
     * @param config Router configuration
     * @param metrics Optional Prometheus metrics collector
     * @param truetime Optional TrueTime for distributed transactions
     */
    ShardRouter(
        std::shared_ptr<URNResolver> resolver,
        std::shared_ptr<RemoteExecutor> executor,
        const Config& config,
        std::shared_ptr<PrometheusMetrics> metrics = nullptr,
        std::shared_ptr<TrueTime> truetime = nullptr
    );
    
    /**
     * Set TrueTime instance for distributed transactions
     * @param truetime TrueTime instance
     */
    void setTrueTime(std::shared_ptr<TrueTime> truetime);
    
    /**
     * @brief Return the optional distributed transaction coordinator.
     * @return Shared coordinator instance or nullptr when TrueTime is unavailable.
     */
    std::shared_ptr<DistributedTransactionCoordinator> getTransactionCoordinator();
    
    /**
     * Route GET request by URN
     * @param urn URN to retrieve
     * @param snapshot_timestamp Optional timestamp for snapshot reads
     * @return Data from shard, or nullopt if not found/error
     */
    std::optional<nlohmann::json> get(
        const URN& urn,
        std::optional<std::chrono::nanoseconds> snapshot_timestamp = std::nullopt
    );
    
    /**
     * Route PUT request by URN
     * @param urn URN to update
     * @param data Data to store
     * @return true if successful
     */
    bool put(const URN& urn, const nlohmann::json& data);
    
    /**
     * Route DELETE request by URN
     * @param urn URN to delete
     * @return true if successful
     */
    bool del(const URN& urn);
    
    /**
     * @brief Execute a logical query using the routing strategy inferred from its text.
     * @param query Query string, typically an AQL-style statement.
     * @return Combined JSON result from the selected shard strategy.
     * @note Falls back to scatter-gather when single-shard extraction is inconclusive.
     */
    virtual nlohmann::json executeQuery(const std::string& query);
    
    /**
     * Determine routing strategy for a query
     * Analyzes query to determine which shards to involve
     * @param query Query string
     * @return Routing strategy
     */
    RoutingStrategy analyzeQuery(const std::string& query) const;
    
    /**
     * Execute scatter-gather query
     * Sends query to all shards and merges results
     * @param query Query to execute
     * @return Merged results from all shards
     */
    virtual std::vector<ShardResult> scatterGather(const std::string& query);

    /**
     * Execute a query on a specific subset of shards.
     *
     * Behaves identically to scatterGather but only contacts the shards
     * whose IDs appear in @p shard_ids.  Unknown or unhealthy shard IDs are
     * skipped with a WARN log rather than causing an error.
     *
     * @param query     AQL query string
     * @param shard_ids Shard identifiers to target
     * @return Results from the targeted shards (success + failure entries)
     */
    virtual std::vector<ShardResult> executeOnShards(
        const std::string& query,
        const std::vector<std::string>& shard_ids
    );

    /**
     * @brief Access the resolver used for key-to-shard lookup decisions.
     * @return Mutable reference to the configured resolver.
     */
    URNResolver& getResolver() { return *resolver_; }

    /**
     * @brief Access the resolver used for key-to-shard lookup decisions.
     * @return Const reference to the configured resolver.
     */
    const URNResolver& getResolver() const { return *resolver_; }
    
    /**
     * Execute cross-shard join (simplified two-phase approach)
     * Phase 1: Fetch from first collection
     * Phase 2: Lookup in second collection
     * @param query Query string
     * @param join_field Field to join on
     * @return Joined results with monotonic mergeVersion/version_token metadata so
     *         callers can detect stale merged snapshots across shards.
     */
    nlohmann::json executeCrossShardJoin(
        const std::string& query,
        const std::string& join_field
    );
    
    /**
     * @brief Return a snapshot of aggregate routing counters.
     * @return JSON object containing total, local, remote, scatter, and error counters.
     */
    nlohmann::json getStatistics() const;
    
    /**
     * Route request to appropriate shard
     * Handles both local and remote execution
     * @param urn URN to route
     * @param method HTTP method (non-empty required)
     * @param path Request path (non-empty required)
     * @param body Optional request body
     * @return Result from shard
     * @note Rejects empty method or path fail-closed
     */
    ShardResult routeRequest(
        const URN& urn,
        const std::string& method,
        const std::string& path,
        const std::optional<nlohmann::json>& body = std::nullopt
    );
    
    /**
     * Execute request locally (this shard)
     * @param method HTTP method (non-empty required)
     * @param path Request path (non-empty required)
     * @param body Optional request body
     * @return Result from local execution
     * @note Rejects empty method or path fail-closed
     */
    ShardResult executeLocal(
        const std::string& method,
        const std::string& path,
        const std::optional<nlohmann::json>& body = std::nullopt
    );

    /**
     * @brief Execute multi-shard exact consistency query with deterministic fallback.
     *
     * Executes a query across multiple shards with exact consistency guarantees.
     * If some shards fail, falls back to healthy shards while maintaining consistency.
     *
     * Guarantees:
     * - Quorum-based validation (majority of shards must agree)
     * - Deterministic fallback to remaining healthy shards
     * - Version token tracking for consistency verification
     *
     * @param query Query text.
     * @param shard_ids Target shard identifiers.
     * @return Per-shard execution records; check validateMultiShardExactConsistency().
     */
    std::vector<ShardResult> executeMultiShardExactConsistency(
        const std::string& query,
        const std::vector<std::string>& shard_ids);

    /**
     * @brief Validate multi-shard results for exact consistency.
     *
     * Checks that results from multiple shards are consistent and
     * can be safely merged for exact consistency reads.
     *
     * Validation criteria:
     * - Quorum of shards must have succeeded
     * - Version tokens must be within expected consistency window
     * - No causal ordering violations
     *
     * @param results Vector of shard results.
     * @return true if results pass exact consistency validation, false otherwise.
     */
    bool validateMultiShardExactConsistency(
        const std::vector<ShardResult>& results);

private:
    std::shared_ptr<URNResolver> resolver_;   ///< Resolves ownership and health information for shards.
    std::shared_ptr<RemoteExecutor> executor_; ///< Performs remote shard RPCs when the target is non-local.
    std::shared_ptr<PrometheusMetrics> metrics_; ///< Optional metrics sink for routing observability.
    std::shared_ptr<TrueTime> truetime_;      ///< Optional bounded-time source for distributed coordination.
    std::shared_ptr<DistributedTransactionCoordinator> txn_coordinator_; ///< Optional TrueTime-backed txn coordinator.
    Config config_;                           ///< Immutable router configuration captured at construction.
    
    /// @brief Thread-safe diagnostic counters using relaxed memory order (no synchronization required).
    /// These are read-only metrics for observability; they don't protect critical state.
    /// Memory order: std::memory_order_relaxed (sufficient for diagnostic counters).
    mutable std::atomic<uint64_t> total_requests_{0};          ///< Total number of routed operations.
    mutable std::atomic<uint64_t> local_requests_{0};          ///< Requests executed on the local shard.
    mutable std::atomic<uint64_t> remote_requests_{0};         ///< Requests dispatched to a remote shard.
    mutable std::atomic<uint64_t> scatter_gather_requests_{0}; ///< Scatter-gather style requests executed.
    mutable std::atomic<uint64_t> errors_{0};                  ///< Routing or execution failures observed.
    
    /**
     * Merge results from multiple shards
     * Combines data arrays and handles errors
     * @param results Results from shards
     * @return Merged result with mergeVersion/version_token metadata derived from
     *         shard payload versions or a local monotonic clock fallback.
     */
    nlohmann::json mergeResults(const std::vector<ShardResult>& results);
    
    /**
     * Apply LIMIT/OFFSET across shards
     * For scatter-gather queries
     * @param merged Merged results
     * @param offset Offset to apply
     * @param limit Limit to apply
     * @return Paginated results
     */
    nlohmann::json applyPagination(
        const nlohmann::json& merged,
        size_t offset,
        size_t limit
    );
    
    /**
     * Extract URN from query (if present)
     * Simple pattern matching for URN-based queries
     * @param query Query string
     * @return URN if found
     */
    std::optional<URN> extractURN(const std::string& query) const;
    
    /**
     * Extract namespace from query (if present)
     * @param query Query string
     * @return Namespace if found
     */
    std::optional<std::string> extractNamespace(const std::string& query) const;
};

} // namespace themis::sharding
