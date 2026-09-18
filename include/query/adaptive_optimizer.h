/**
 * @file adaptive_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
     * @brief Record Execution.
     * @param[in] exec Input parameter.
     */
    void recordExecution(const QueryExecution& exec);
    
    std::vector<QueryExecution> getHistory(const std::string& query_hash, size_t limit = 10) const;
    
    /**
     * @brief Get Average Selectivity.
     * @param[in] query_hash Input parameter.
     * @return Return value.
     */
    double getAverageSelectivity(const std::string& query_hash) const;
    
    bool hasCardinalityMisestimation(const std::string& query_hash, double threshold = 2.0) const;
    
    /**
     * @brief Get Average Actual Rows.
     * @param[in] query_hash Input parameter.
     * @return Return value.
     */
    size_t getAverageActualRows(const std::string& query_hash) const;

    /**
     * @brief Get Adaptive Adjustment Factor.
     * @param[in] query_hash Input parameter.
     * @return Return value.
     */
    double getAdaptiveAdjustmentFactor(const std::string& query_hash) const;
    
    void pruneOldStats(std::chrono::hours retention = std::chrono::hours(24));
    
    size_t getTotalQueries() const { return total_queries_.load(); }
    
private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<QueryExecution>> executions_;
    std::atomic<size_t> total_queries_{0};
    static constexpr size_t MAX_HISTORY_PER_QUERY = 100;
};

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
     * @brief Select Plan.
     * @param[in] alternatives Input parameter.
     * @param[in] query_hash Input parameter.
     * @param[in] stats Input parameter.
     * @return Return value.
     */
    PlanChoice selectPlan(
        const std::vector<PlanChoice>& alternatives,
        const std::string& query_hash,
        const AdaptiveQueryStats& stats) const;
    
    bool shouldSwitchPlan(
        size_t rows_so_far,
        size_t estimated_total,
        double progress,
        double misestimation_threshold = 5.0) const;
    
    /**
     * @brief Get Alternative Plan.
     * @param[in] current_plan Input parameter.
     * @param[in] actual_rows Input parameter.
     * @param[in] estimated_rows Input parameter.
     * @return Return value.
     */
    PlanChoice getAlternativePlan(
        const PlanChoice& current_plan,
        size_t actual_rows,
        size_t estimated_rows) const;
};

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
     * @brief Estimate Distributed Query Cost.
     * @param[in] involved_shards Input parameter.
     * @param[in] estimated_result_rows Input parameter.
     * @return Return value.
     */
    double estimateDistributedQueryCost(
        const std::vector<ShardInfo>& involved_shards,
        size_t estimated_result_rows) const;
    
    /**
     * @brief Estimate Cross Shard Join Cost.
     * @param[in] left_shard Input parameter.
     * @param[in] right_shard Input parameter.
     * @param[in] left_rows Input parameter.
     * @param[in] right_rows Input parameter.
     * @return Return value.
     */
    CrossShardJoinCost estimateCrossShardJoinCost(
        const ShardInfo& left_shard,
        const ShardInfo& right_shard,
        size_t left_rows,
        size_t right_rows) const;
    
    /**
     * @brief Should Prune Partition.
     * @param[in] shard Input parameter.
     * @param[in] total_shards Input parameter.
     * @param[in] selectivity Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldPrunePartition(
        const ShardInfo& shard,
        size_t total_shards,
        double selectivity) const;
    
    /**
     * @brief Get Optimal Parallelism.
     * @param[in] shards Input parameter.
     * @param[in] available_threads Input parameter.
     * @return Return value.
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

class MultiIndexOptimizer {
public:
    struct IndexCandidate {
        std::string index_name;
        std::string column = {};
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
     * @brief Optimize Multi Index Access.
     * @param[in] available_indexes Input parameter.
     * @param[in] table_size Input parameter.
     * @return Return value.
     */
    IntersectionPlan optimizeMultiIndexAccess(
        const std::vector<IndexCandidate>& available_indexes,
        size_t table_size) const;
    
    /**
     * @brief Should Use Index Intersection.
     * @param[in] candidates Input parameter.
     * @param[in] table_size Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldUseIndexIntersection(
        const std::vector<IndexCandidate>& candidates,
        size_t table_size) const;
    
    double getBitmapIntersectionThreshold() const { return 0.1; }
};

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
     * @brief Get Optimal Placement.
     * @param[in] data_size_bytes Input parameter.
     * @param[in] parallelism Input parameter.
     * @return Return value.
     */
    NumaPlacement getOptimalPlacement(
        size_t data_size_bytes,
        size_t parallelism) const;
    
    /**
     * @brief Is Numa Available.
     * @return True when the operation succeeds.
     */
    static bool isNumaAvailable();
    
    /**
     * @brief Get Numa Node Count.
     * @return Return value.
     */
    static size_t getNumaNodeCount();
    
    /**
     * @brief Pin Thread To Cpu.
     * @param[in] cpu_id Identifier of the cpu.
     * @return True when the operation succeeds.
     */
    static bool pinThreadToCpu(int cpu_id);
};

class GeoPredicatePatternDetector {
public:
    struct DetectedSpatialHint {
        std::string function_name;   ///< Normalized spatial function name.
        std::string field_reference; ///< First argument field reference.
    };

    /**
     * @brief Detect.
     * @param[in] query_text Input parameter.
     * @return Return value.
     */
    static std::optional<DetectedSpatialHint> detect(const std::string& query_text);

    static void injectSpatialIndexHints(
        const std::string& query_text,
        std::map<std::string, std::string>& hints,
        std::vector<std::string>& suggested_indexes);
};

} // namespace query
} // namespace themis

