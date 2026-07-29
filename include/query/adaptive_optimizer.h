/**
 * @file adaptive_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: adaptive_optimizer.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace themis {
namespace query {

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
            PARALLEL_SCAN,
            BINARY_BATCH_CPU,   ///< MessagePack/custom-binary + CPU thread pool
            ARROW_GPU_VRAM,     ///< Apache Arrow IPC + GPU/VRAM parallel execution
            ARROW_CPU_PARALLEL, ///< Apache Arrow IPC + CPU thread pool
        };
        
        Strategy strategy;
        double estimated_cost = 0.0;
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
        int node_id = 0;
        size_t available_cores = 0;
        size_t memory_gb = 0;
        std::vector<int> cpu_ids;
    };
    
    struct NumaPlacement {
        int preferred_numa_node = 0;
        std::vector<int> cpu_affinity;
        bool use_local_memory = false;
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

/**
 * @brief Detects geospatial predicate patterns from query text and injects optimizer hints.
 *
 * Recognizes FILTER predicates like `ST_Within(field, @poly)` and injects
 * `GEO` index hints into optimizer metadata for downstream plan selection.
 */
class GeoPredicatePatternDetector {
public:
    struct DetectedSpatialHint {
        std::string function_name;   ///< Normalized spatial function name.
        std::string field_reference; ///< First argument field reference.
    };

    /**
     * @brief Detect geospatial FILTER patterns in query text.
     * @param query_text Raw AQL query text.
     * @return Spatial hint metadata when a supported pattern is found.
     */
    static std::optional<DetectedSpatialHint> detect(const std::string& query_text);

    /**
     * @brief Inject GEO index hints for supported spatial FILTER patterns.
     * @param query_text Raw AQL query text.
     * @param hints Mutable hint map to augment.
     * @param suggested_indexes Mutable suggested-index list to augment.
     */
    static void injectSpatialIndexHints(
        const std::string& query_text,
        std::map<std::string, std::string>& hints,
        std::vector<std::string>& suggested_indexes);
};

} // namespace query
} // namespace themis

