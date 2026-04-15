/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_optimizer.h                               ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:04:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     310                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>

namespace themis {

/**
 * @brief Adaptive Query Execution Statistics
 * 
 * Collects runtime statistics for query execution to enable adaptive optimization.
 * Tracks cardinality estimates, actual results, execution times, and resource usage.
 */
class AdaptiveQueryStats {
public:
    struct QueryExecution {
        std::string query_hash;          // Hash of query structure
        size_t estimated_rows = 0;       // Optimizer's cardinality estimate
        size_t actual_rows = 0;          // Actual rows returned
        double execution_time_ms = 0.0;  // Total execution time
        double selectivity = 1.0;        // actual_rows / estimated_rows
        std::chrono::system_clock::time_point timestamp;
        
        // Per-operator statistics
        struct OperatorStats {
            std::string operator_type;   // "scan", "join", "filter", "sort", etc.
            size_t estimated_rows = 0;
            size_t actual_rows = 0;
            double time_ms = 0.0;
        };
        std::vector<OperatorStats> operators;
    };
    
    /**
     * @brief Record a query execution
     */
    void recordExecution(const QueryExecution& exec);
    
    /**
     * @brief Get historical executions for a query pattern
     */
    std::vector<QueryExecution> getHistory(const std::string& query_hash, size_t limit = 10) const;
    
    /**
     * @brief Get average selectivity for a query pattern
     */
    double getAverageSelectivity(const std::string& query_hash) const;
    
    /**
     * @brief Check if cardinality estimates are consistently off
     */
    bool hasCardinalityMisestimation(const std::string& query_hash, double threshold = 2.0) const;
    
    /**
     * @brief Get average actual rows across historical executions
     * @return Average actual_rows, or 0 if no history exists
     */
    size_t getAverageActualRows(const std::string& query_hash) const;

    /**
     * @brief Get adaptive adjustment factor based on history
     * @return Multiplier for cardinality estimates (e.g., 0.5 if historically overestimated)
     */
    double getAdaptiveAdjustmentFactor(const std::string& query_hash) const;
    
    /**
     * @brief Clear old statistics (retention policy)
     */
    void pruneOldStats(std::chrono::hours retention = std::chrono::hours(24));
    
    /**
     * @brief Get total queries tracked
     */
    size_t getTotalQueries() const { return total_queries_.load(); }
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<QueryExecution>> executions_;
    std::atomic<size_t> total_queries_{0};
    static constexpr size_t MAX_HISTORY_PER_QUERY = 100;
};

/**
 * @brief Adaptive Plan Selector
 * 
 * Selects and adjusts query execution plans at runtime based on feedback.
 * Supports runtime plan switching when estimates are significantly off.
 */
class AdaptivePlanSelector {
public:
    struct PlanChoice {
        enum class Strategy {
            INDEX_SCAN,
            TABLE_SCAN,
            HASH_JOIN,
            MERGE_JOIN,
            NESTED_LOOP_JOIN,
            INDEX_INTERSECTION,
            PARALLEL_SCAN
        };
        
        Strategy strategy;
        double estimated_cost;
        std::string description;
    };
    
    /**
     * @brief Choose plan based on cardinality estimates and historical data
     */
    PlanChoice selectPlan(
        const std::vector<PlanChoice>& alternatives,
        const std::string& query_hash,
        const AdaptiveQueryStats& stats) const;
    
    /**
     * @brief Determine if plan should be switched at runtime
     * @param rows_so_far Actual rows processed so far
     * @param estimated_total Originally estimated total rows
     * @param progress Fraction of query completed (0.0-1.0)
     * @return true if plan should be switched
     */
    bool shouldSwitchPlan(
        size_t rows_so_far,
        size_t estimated_total,
        double progress,
        double misestimation_threshold = 5.0) const;
    
    /**
     * @brief Get alternative plan for runtime switching
     */
    PlanChoice getAlternativePlan(
        const PlanChoice& current_plan,
        size_t actual_rows,
        size_t estimated_rows) const;
};

/**
 * @brief Distributed Query Cost Model
 * 
 * Cost model that accounts for network latency, data locality, and shard distribution.
 */
class DistributedQueryCostModel {
public:
    struct ShardInfo {
        std::string shard_id;
        size_t estimated_rows = 0;
        double network_latency_ms = 1.0;  // Estimated network latency to this shard
        bool is_local = false;             // Is this shard local to the coordinator?
    };
    
    struct CrossShardJoinCost {
        double total_cost = 0.0;
        double network_cost = 0.0;
        double compute_cost = 0.0;
        std::string recommended_strategy;  // "broadcast", "repartition", "semi_join"
    };
    
    /**
     * @brief Estimate cost for distributed query execution
     */
    double estimateDistributedQueryCost(
        const std::vector<ShardInfo>& involved_shards,
        size_t estimated_result_rows) const;
    
    /**
     * @brief Estimate cost for cross-shard join
     */
    CrossShardJoinCost estimateCrossShardJoinCost(
        const ShardInfo& left_shard,
        const ShardInfo& right_shard,
        size_t left_rows,
        size_t right_rows) const;
    
    /**
     * @brief Determine if partition pruning is beneficial
     */
    bool shouldPrunePartition(
        const ShardInfo& shard,
        size_t total_shards,
        double selectivity) const;
    
    /**
     * @brief Get optimal parallelism degree for distributed query
     */
    size_t getOptimalParallelism(
        const std::vector<ShardInfo>& shards,
        size_t available_threads) const;
    
private:
    // Cost constants (tunable)
    static constexpr double NETWORK_TRANSFER_COST_PER_ROW = 0.01;  // ms per row
    static constexpr double CROSS_SHARD_JOIN_OVERHEAD = 10.0;      // ms base overhead
    static constexpr double LOCAL_ROW_PROCESSING_COST = 0.001;     // ms per row
};

/**
 * @brief Multi-Index Intersection Optimizer
 * 
 * Optimizes queries that can benefit from intersecting multiple indexes.
 */
class MultiIndexOptimizer {
public:
    struct IndexCandidate {
        std::string index_name;
        std::string column;
        size_t estimated_selectivity = 0;  // Number of rows passing this index
        double access_cost = 0.0;           // Cost to access this index
        bool is_covering = false;           // Does index cover all required columns?
    };
    
    struct IntersectionPlan {
        std::vector<std::string> indexes_to_use;  // Ordered by selectivity
        double estimated_cost = 0.0;
        size_t estimated_result_rows = 0;
        bool use_bitmap_intersection = false;      // Use bitmap for intersection?
    };
    
    /**
     * @brief Generate optimal multi-index access plan
     */
    IntersectionPlan optimizeMultiIndexAccess(
        const std::vector<IndexCandidate>& available_indexes,
        size_t table_size) const;
    
    /**
     * @brief Determine if index intersection is beneficial
     */
    bool shouldUseIndexIntersection(
        const std::vector<IndexCandidate>& candidates,
        size_t table_size) const;
    
    /**
     * @brief Get bitmap intersection threshold
     * @return Minimum selectivity for bitmap intersection to be worthwhile
     */
    double getBitmapIntersectionThreshold() const { return 0.1; }
};

/**
 * @brief NUMA-Aware Query Optimizer
 * 
 * Optimizes query execution for NUMA architectures.
 */
class NumaAwareOptimizer {
public:
    struct NumaNode {
        int node_id;
        size_t available_cores;
        size_t memory_gb;
        std::vector<int> cpu_ids;
    };
    
    struct NumaPlacement {
        int preferred_numa_node;
        std::vector<int> cpu_affinity;
        bool use_local_memory;
    };
    
    /**
     * @brief Get optimal NUMA placement for query
     */
    NumaPlacement getOptimalPlacement(
        size_t data_size_bytes,
        size_t parallelism) const;
    
    /**
     * @brief Check if NUMA optimizations are available
     */
    static bool isNumaAvailable();
    
    /**
     * @brief Get number of NUMA nodes
     */
    static size_t getNumaNodeCount();
    
    /**
     * @brief Pin thread to specific CPU cores
     */
    static bool pinThreadToCpu(int cpu_id);
};

} // namespace themis
