/**
 * @file query_federation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "sharding/shard_router.h"
#include "sharding/urn_resolver.h"
#include "sharding/sharding_manager.h"
#include "sharding/adaptive_shard_router.h"
#include "query/query_optimizer.h"
#include "distributed_knowledge/federated_rag_merger.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis::query {

/**
 * @brief Query Federation - Advanced cross-shard query execution
 * 
 * Extends the basic shard router functionality with:
 * - Query decomposition and rewriting
 * - Parallel execution planning
 * - Result streaming and pagination
 * - Cross-shard JOIN optimization
 * - Aggregate pushdown
 * - Cost-based routing decisions
 * 
 * Query Federation Process:
 * ```
 * Client Query
 *      ↓
 * Parse & Analyze
 *      ↓
 * Decompose into sub-queries
 *      ↓
 * ┌────────┬────────┬────────┐
 * ↓        ↓        ↓        ↓
 * Shard1  Shard2  Shard3  ShardN
 * ↓        ↓        ↓        ↓
 * └────────┴────────┴────────┘
 *      ↓
 * Merge & Aggregate
 *      ↓
 * Apply Global Operations
 *      ↓
 * Return Results
 * ```
 */
class QueryFederation {
public:
    /**
     * @brief Configuration for query federation
     */
    struct Config {
        // Optimization settings
        bool enable_pushdown = true;           // Push filters to shards
        bool enable_parallel_execution = true; // Execute queries in parallel
        bool enable_result_streaming = false;  // Stream results as they arrive
        
        // Resource limits
        uint32_t max_parallel_shards = 10;     // Max concurrent shard queries
        uint64_t max_result_size_bytes = 100 * 1024 * 1024; // 100MB
        uint32_t query_timeout_ms = 60000;     // 60 seconds
        
        // Join optimization
        bool enable_broadcast_join = true;     // Broadcast small tables
        uint64_t broadcast_threshold_bytes = 10 * 1024 * 1024; // 10MB
        
        // Caching
        bool enable_result_cache = false;      // Cache federated query results
        uint32_t cache_ttl_seconds = 300;      // 5 minutes
    };
    
    /**
     * @brief Execution plan for a federated query
     */
    struct ExecutionPlan {
        enum class Strategy {
            SCATTER_GATHER,       // Send same query to all shards
            PARTITION_PRUNING,    // Send query only to relevant shards
            BROADCAST_JOIN,       // Broadcast small table to all shards
            SHUFFLE_JOIN,         // Redistribute data for join
            MAP_REDUCE            // Map phase on shards, reduce locally
        };
        
        Strategy strategy;
        std::vector<std::string> target_shards;
        std::vector<std::string> sub_queries;
        std::string merge_operation;
        uint64_t estimated_cost = 0;
    };
    
    /**
     * @brief Construct query federation engine
     * 
     * @param shard_router Shard router for execution
     */
    QueryFederation(std::shared_ptr<sharding::ShardRouter> shard_router);
    
    /**
     * @brief Construct query federation engine with configuration
     * 
     * @param shard_router Shard router for execution
     * @param config Configuration
     */
    QueryFederation(
        std::shared_ptr<sharding::ShardRouter> shard_router,
        const Config& config
    );
    
    /**
     * @brief Construct query federation engine with explicit ShardingManager
     *
     * Enables shard-key routing (point-lookup and range queries) so that
     * only the relevant shards are consulted instead of broadcasting to all.
     *
     * @param shard_router      Shard router for execution
     * @param sharding_manager  ShardingManager owning the consistent-hash ring
     */
    QueryFederation(
        std::shared_ptr<sharding::ShardRouter> shard_router,
        sharding::ShardingManager& sharding_manager
    );
    
    /**
     * @brief Construct query federation engine with explicit ShardingManager and configuration
     *
     * Enables shard-key routing (point-lookup and range queries) so that
     * only the relevant shards are consulted instead of broadcasting to all.
     *
     * @param shard_router      Shard router for execution
     * @param sharding_manager  ShardingManager owning the consistent-hash ring
     * @param config            Optional configuration
     */
    QueryFederation(
        std::shared_ptr<sharding::ShardRouter> shard_router,
        sharding::ShardingManager& sharding_manager,
        const Config& config
    );
    
    // ── DK-4: Federated RAG merge (Layer C) ─────────────────────────────────

    /**
     * @brief Inject a FederatedRAGMerger (DK-4 DI-setter).
     *
     * When set, `executeFederatedRAGQuery()` uses this merger to combine
     * per-shard retrieval results via Reciprocal Rank Fusion.
     */
    void setRAGMerger(
        std::shared_ptr<distributed_knowledge::FederatedRAGMerger> merger);

    /**
     * @brief Inject an AdaptiveShardRouter for per-shard accuracy-delta lookup
     *        (DK-4 DI-setter).
     *
     * When set, `executeFederatedRAGQuery()` enriches each
     * `ShardRetrievalResult` with `adapter_accuracy_delta` from the router
     * so that specialised shards are boosted during RRF merge.
     *
     * @param router  Typed AdaptiveShardRouter — provides
     *                `getAdapterAccuracyDelta(shard_id, domain)`.
     */
    void setShardRouter(
        std::shared_ptr<sharding::AdaptiveShardRouter> router);

    /**
     * @brief Merge pre-built per-shard retrieval results via the injected
     *        FederatedRAGMerger.
     *
     * Exposed publicly so unit tests can bypass the fan-out and verify merge
     * logic directly without a running shard cluster.
     *
     * @throws std::logic_error when no RAGMerger has been injected.
     */
    [[nodiscard]] distributed_knowledge::MergedRAGContext mergeRAGResults(
        const std::vector<distributed_knowledge::ShardRetrievalResult>& shard_results
    ) const;

    /**
     * @brief Execute a RAG-aware federated query.
     *
     * Fan-out to all shards via `shard_router_->scatterGather()`, convert
     * each `ShardResult` to `ShardRetrievalResult` (including
     * `adapter_accuracy_delta` when an AdaptiveShardRouter is injected),
     * then merge via `FederatedRAGMerger`.
     *
     * Timeout shards (`success == false`) are marked `ok = false` and
     * skipped by the merger automatically.
     *
     * @throws std::logic_error when no RAGMerger has been injected.
     * @param domain  Domain type used for accuracy-delta lookup (default: GENERAL).
     */
    [[nodiscard]] distributed_knowledge::MergedRAGContext executeFederatedRAGQuery(
        const std::string& query,
        distributed_knowledge::AdapterDomainType domain =
            distributed_knowledge::AdapterDomainType::GENERAL
    );

    // ── Standard execution ───────────────────────────────────────────────────

    /**
     * @brief Execute federated query
     * 
     * Main entry point for federated query execution:
     * 1. Parses and analyzes query
     * 2. Creates execution plan
     * 3. Executes plan across shards
     * 4. Merges and returns results
     *
     * **Exception Safety (Wave A §13):**
     *   This is a propagating boundary: exceptions from shard routers and
     *   query execution are caught, audited (including exception type and affected
     *   clusters), logged with full context, and then re-thrown to the caller.
     *   This provides observability without exception swallowing.
     *
     *   Both std::exception and unknown exceptions are audited before propagation.
     *   Audit logs include:
     *     - Event: "federation_failure"
     *     - Exception type (typeid name)
     *     - Original exception message
     *     - Affected cluster count
     *     - Timestamp
     *
     * @param query Query string (AQL format)
     * @return Query results as JSON
     * 
     * @throws Any exception from shard router or query execution; wrapped with
     *         audit context. Guaranteed never to swallow exceptions.
     *         - std::exception and subclasses: Full type and message logged
     *         - Unknown exceptions: Audited as "unknown exception" then re-thrown
     */
    nlohmann::json execute(const std::string& query);
    
    /**
     * @brief Create execution plan for a query
     * 
     * Analyzes query and determines optimal execution strategy
     * 
     * @param query Query string
     * @return Execution plan
     */
    ExecutionPlan createExecutionPlan(const std::string& query);
    
    /**
     * @brief Execute cross-shard JOIN operation
     * 
     * Optimized JOIN execution:
     * - Broadcast join for small tables
     * - Shuffle join for large tables
     * - Semi-join reduction when possible
     * 
     * @param left_collection Left side of join
     * @param right_collection Right side of join
     * @param join_condition Join condition
     * @return Joined results
     */
    nlohmann::json executeJoin(
        const std::string& left_collection,
        const std::string& right_collection,
        const std::string& join_condition
    );
    
    /**
     * @brief Execute aggregation query
     * 
     * Pushes partial aggregation to shards when possible:
     * - COUNT, SUM: Partial aggregation on each shard
     * - AVG: Compute SUM and COUNT on shards, combine locally
     * - MIN/MAX: Compute on each shard, take min/max
     * - GROUP BY: Partial grouping on shards, final grouping locally
     * 
     * @param query Aggregation query
     * @return Aggregated results
     */
    nlohmann::json executeAggregation(const std::string& query);
    
    /**
     * @brief Get query statistics
     * 
     * @return Statistics including execution counts, latencies
     */
    nlohmann::json getStatistics() const;

public:
    std::shared_ptr<sharding::ShardRouter> shard_router_;
    // Non-owning pointer; nullptr when no ShardingManager was injected.
    sharding::ShardingManager* sharding_manager_ = nullptr;
    Config config_;
    
    // Statistics
    std::atomic<uint64_t> total_queries_{0};
    std::atomic<uint64_t> scatter_gather_queries_{0};
    std::atomic<uint64_t> partition_pruned_queries_{0};
    std::atomic<uint64_t> broadcast_joins_{0};
    std::atomic<uint64_t> shuffle_joins_{0};
    mutable std::mutex routing_mutex_;

    // ── DK-4: Federated RAG merge ────────────────────────────────────────────
    std::shared_ptr<distributed_knowledge::FederatedRAGMerger> rag_merger_;
    std::shared_ptr<sharding::AdaptiveShardRouter>             adaptive_router_;
    
    /**
     * @brief Analyze query to extract metadata
     * 
     * @param query Query string
     * @return Query metadata (tables, predicates, etc.)
     */
    struct QueryMetadata {
        // ── Shard-key predicate ──────────────────────────────────────────────
        // Populated by analyzeQuery() when it detects a _key == <value> or
        // _key >= <min> AND _key <= <max> predicate, enabling partition pruning.
        struct ShardKeyPredicate {
            enum class Kind { POINT, RANGE };
            Kind kind;
            std::string collection;
            std::string key_value;   // used when kind == POINT
            std::string key_min;     // used when kind == RANGE
            std::string key_max;     // used when kind == RANGE
        };

        std::vector<std::string> tables;
        std::vector<std::string> predicates;
        std::vector<std::string> projections;
        std::vector<std::string> aggregations;
        std::vector<std::string> joins;
        std::optional<std::string> order_by;
        std::optional<uint64_t> limit;
        std::optional<uint64_t> offset;

        // Shard-key routing hint (nullopt → no routing hint, use scatter-gather)
        std::optional<ShardKeyPredicate> shard_key_predicate;
        
        // Extended for adaptive capability-based routing
        std::string query_text;               // Original query text
        std::vector<float> embeddings;        // Query embeddings for semantic matching

        // Shard-key routing fields (populated by analyzeQuery)
        // Set when the query contains an equality predicate on _key:
        //   FILTER doc._key == "<value>"
        std::optional<std::string> point_lookup_key;
        // Set when the query contains a range predicate on _key:
        //   FILTER doc._key >= "<min>" AND doc._key <= "<max>"
        std::optional<std::pair<std::string, std::string>> key_range;
    };
    
    /**
     * @brief Analyze query to extract metadata for execution planning.
     *
     * Parses query text to identify:
     *   - Tables referenced
     *   - Join conditions
     *   - Aggregations
     *   - LIMIT/OFFSET clauses
     *   - Shard-key predicates
     *
     * **Exception Safety (Wave A §13):**
     *   Strong exception safety: never propagates exceptions from regex or
     *   numeric parsing. Parsing failures are logged with full context and
     *   gracefully degrade (e.g., failed LIMIT parsing resets to std::nullopt).
     *   This ensures analyzis failures never corrupt query execution.
     *
     * @param query Query string to analyze
     * @return QueryMetadata with parsed information; unset fields indicate
     *         parse failures (safe, non-throwing degradation)
     *
     * @throws Never. All exceptions caught and logged; caller receives
     *         partial metadata with failed fields unset.
     */
    QueryMetadata analyzeQuery(const std::string& query);
    /**
     * @brief Determine which shards are relevant for a query
     * 
     * @param metadata Query metadata
     * @return List of shard IDs
     */
    std::vector<std::string> determineRelevantShards(const QueryMetadata& metadata);
    
    /**
     * @brief Rewrite query for execution on a specific shard
     * 
     * @param query Original query
     * @param shard_id Target shard
     * @return Rewritten query
     */
    std::string rewriteQueryForShard(const std::string& query, const std::string& shard_id);
    
    /**
     * @brief Merge results from multiple shards
     * 
     * @param results Results from each shard
     * @param metadata Query metadata
     * @return Merged results
     */
    nlohmann::json mergeResults(
        const std::vector<sharding::ShardResult>& results,
        const QueryMetadata& metadata
    );
    
    /**
     * @brief Apply global operations (ORDER BY, LIMIT)
     * 
     * @param merged Merged results
     * @param metadata Query metadata
     * @return Results with global operations applied
     */
    nlohmann::json applyGlobalOperations(
        const nlohmann::json& merged,
        const QueryMetadata& metadata
    );
    
    /**
     * @brief Estimate table size for join optimization
     * 
     * @param collection Collection name
     * @return Estimated size in bytes
     */
    uint64_t estimateCollectionSize(const std::string& collection);
};

} // namespace themis::query
