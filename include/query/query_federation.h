/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_federation.h                                 ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:41:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     320                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7811d1486a  2026-03-27  feat: Enhance backward compatibility and legacy support a... ║
    • bc061a79df  2026-03-24  feat(query): QueryFederation shard-key routing v1.9.0 ║
    • 097e8a5773  2026-03-24  feat(query,sharding): QueryFederation shard-key routing v... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "sharding/shard_router.h"
#include "sharding/urn_resolver.h"
#include "sharding/sharding_manager.h"
#include "query/query_optimizer.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <atomic>
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
        uint64_t estimated_cost;
    };
    
    /**
     * @brief Construct query federation engine
     * 
     * @param shard_router Shard router for execution
     * @param config Configuration
     */
    QueryFederation(std::shared_ptr<sharding::ShardRouter> shard_router);
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
     * @param config            Optional configuration
     */
    QueryFederation(
        std::shared_ptr<sharding::ShardRouter> shard_router,
        sharding::ShardingManager& sharding_manager
    );
    QueryFederation(
        std::shared_ptr<sharding::ShardRouter> shard_router,
        sharding::ShardingManager& sharding_manager,
        const Config& config
    );
    
    /**
     * @brief Execute federated query
     * 
     * Main entry point for federated query execution:
     * 1. Parses and analyzes query
     * 2. Creates execution plan
     * 3. Executes plan across shards
     * 4. Merges and returns results
     * 
     * @param query Query string (AQL format)
     * @return Query results
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
