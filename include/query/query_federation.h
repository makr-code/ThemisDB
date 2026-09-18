/**
 * @file query_federation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class QueryFederation {
public:
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
    
    QueryFederation(std::shared_ptr<sharding::ShardRouter> shard_router);
    
    QueryFederation(
        std::shared_ptr<sharding::ShardRouter> shard_router,
        const Config& config
    );
    
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
     * @brief ── DK-4: Federated RAG merge (Layer C) ─────────────────────────────────
     * @param[in] merger Input parameter.
     */

    void setRAGMerger(
        std::shared_ptr<distributed_knowledge::FederatedRAGMerger> merger);

    /**
     * @brief Set Shard Router.
     * @param[in] router Input parameter.
     */
    void setShardRouter(
        std::shared_ptr<sharding::AdaptiveShardRouter> router);

    [[nodiscard]] distributed_knowledge::MergedRAGContext mergeRAGResults(
        const std::vector<distributed_knowledge::ShardRetrievalResult>& shard_results
    ) const;

    [[nodiscard]] distributed_knowledge::MergedRAGContext executeFederatedRAGQuery(
        const std::string& query,
        distributed_knowledge::AdapterDomainType domain =
            distributed_knowledge::AdapterDomainType::GENERAL
    );

    /**
     * @brief ── Standard execution ───────────────────────────────────────────────────
     * @param[in] query Input parameter.
     * @return Return value.
     */

    nlohmann::json execute(const std::string& query);
    
    /**
     * @brief Create Execution Plan.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    ExecutionPlan createExecutionPlan(const std::string& query);
    
    /**
     * @brief Execute Join.
     * @param[in] left_collection Input parameter.
     * @param[in] right_collection Input parameter.
     * @param[in] join_condition Input parameter.
     * @return Return value.
     */
    nlohmann::json executeJoin(
        const std::string& left_collection,
        const std::string& right_collection,
        const std::string& join_condition
    );
    
    /**
     * @brief Execute Aggregation.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    nlohmann::json executeAggregation(const std::string& query);
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
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
     * @brief Analyze Query.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    QueryMetadata analyzeQuery(const std::string& query);
    /**
     * @brief Determine Relevant Shards.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    std::vector<std::string> determineRelevantShards(const QueryMetadata& metadata);
    
    /**
     * @brief Rewrite Query For Shard.
     * @param[in] query Input parameter.
     * @param[in] shard_id Identifier of the shard.
     * @return Return value.
     */
    std::string rewriteQueryForShard(const std::string& query, const std::string& shard_id);
    
    /**
     * @brief Merge Results.
     * @param[in] results Input parameter.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    nlohmann::json mergeResults(
        const std::vector<sharding::ShardResult>& results,
        const QueryMetadata& metadata
    );
    
    /**
     * @brief Apply Global Operations.
     * @param[in] merged Input parameter.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    nlohmann::json applyGlobalOperations(
        const nlohmann::json& merged,
        const QueryMetadata& metadata
    );
    
    /**
     * @brief Estimate Collection Size.
     * @param[in] collection Input parameter.
     * @return Return value.
     */
    uint64_t estimateCollectionSize(const std::string& collection);
};

} // namespace themis::query
