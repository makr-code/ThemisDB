#pragma once

#include "index/graph_index.h"
#include "utils/expected.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <chrono>
#include <future>
#include <mutex>

namespace themis {
namespace graph {

/**
 * @brief Graph Query Optimizer for efficient graph traversal execution
 * 
 * Provides cost-based optimization for graph queries including:
 * - Multiple traversal algorithm selection (BFS, DFS, Bidirectional, A*)
 * - Query plan generation and cost estimation
 * - Index and statistics-based optimization
 * - Path optimization and early termination
 * 
 * Integrates with GraphIndexManager for graph operations and supports
 * various optimization strategies based on query patterns and graph statistics.
 */
class GraphQueryOptimizer {
public:
    /**
     * Traversal algorithm types
     */
    enum class TraversalAlgorithm {
        BFS,              // Breadth-First Search - best for shortest path, level exploration
        DFS,              // Depth-First Search - best for deep exploration
        BIDIRECTIONAL,    // Search from both ends - best for long-distance paths
        ASTAR,            // Heuristic-guided - best when heuristic available
        DIJKSTRA          // Weighted shortest path
    };

    /**
     * Query pattern types for optimization
     */
    enum class QueryPattern {
        SHORTEST_PATH,           // Single shortest path
        ALL_PATHS,              // All paths between nodes
        K_HOP_NEIGHBORS,        // k-hop neighborhood
        PATTERN_MATCH,          // Subgraph pattern matching
        REACHABILITY,           // Simple reachability check
        CONNECTED_COMPONENT     // Component analysis
    };

    /**
     * Graph statistics for cost estimation
     */
    struct GraphStatistics {
        size_t vertex_count = 0;
        size_t edge_count = 0;
        double avg_degree = 0.0;
        double avg_branching_factor = 0.0;
        size_t max_depth = 0;
        bool has_edge_index = false;
        bool has_adjacency_cache = false;
        
        // Edge type statistics
        std::unordered_map<std::string, size_t> edge_type_counts;
        std::unordered_map<std::string, double> edge_type_selectivity;
    };

    /**
     * Query constraints and hints
     */
    struct QueryConstraints {
        std::optional<int> max_depth;
        std::optional<size_t> max_results;
        std::optional<std::string> edge_type;
        std::optional<std::string> graph_id;
        bool unique_vertices = false;
        bool unique_edges = false;
        std::vector<std::string> forbidden_vertices;
        std::vector<std::string> required_vertices;
        /// Maximum allowed execution time in milliseconds (0 = no limit).
        /// When exceeded, the traversal returns `ERR_QUERY_TIMEOUT` error.
        uint32_t timeout_ms = 0;
        /// Enable parallel frontier expansion in BFS (Phase 3).
        /// When true and the graph is sufficiently large, each BFS level's
        /// neighbor lookups are dispatched to separate threads.
        bool enable_parallel = false;
        /// Maximum worker threads for parallel BFS (0 = use hardware_concurrency/2,
        /// clamped to [2, 16]).  Ignored when enable_parallel = false.
        uint32_t num_threads = 0;
        
        QueryConstraints() = default;
    };

    /**
     * @brief Aggregate observability metrics for graph query execution.
     *
     * All counters are cumulative since the optimizer was constructed.
     * Thread-safe: uses atomic operations for counter updates.
     */
    struct GraphQueryMetrics {
        std::atomic<uint64_t> total_queries{0};
        std::atomic<uint64_t> failed_queries{0};
        std::atomic<uint64_t> timed_out_queries{0};
        std::atomic<uint64_t> total_execution_time_ms{0};
        std::atomic<uint64_t> max_execution_time_ms{0};
        std::atomic<uint64_t> total_nodes_explored{0};
        std::atomic<uint64_t> total_edges_traversed{0};
        std::atomic<uint64_t> plan_cache_hits{0};
        std::atomic<uint64_t> plan_cache_misses{0};

        /// Returns average execution time in milliseconds, or 0 if no queries.
        double avgExecutionTimeMs() const {
            uint64_t n = total_queries.load(std::memory_order_relaxed);
            return n > 0 ? static_cast<double>(
                               total_execution_time_ms.load(std::memory_order_relaxed)) / n
                         : 0.0;
        }

        /// Returns the fraction of queries that failed (0.0–1.0).
        double errorRate() const {
            uint64_t n = total_queries.load(std::memory_order_relaxed);
            return n > 0 ? static_cast<double>(
                               failed_queries.load(std::memory_order_relaxed)) / n
                         : 0.0;
        }
    };

    /**
     * Optimization plan for a graph query
     */
    struct OptimizationPlan {
        TraversalAlgorithm algorithm;
        QueryPattern pattern;
        double estimated_cost;
        double estimated_time_ms;
        size_t estimated_nodes_explored;
        bool use_index = false;
        bool use_cache = false;
        bool enable_early_termination = false;
        bool enable_parallel = false;
        std::string explanation;
        
        // Alternative plans considered
        std::vector<std::pair<TraversalAlgorithm, double>> alternatives;
    };

    /**
     * Query execution statistics
     */
    struct ExecutionStats {
        size_t nodes_explored = 0;
        size_t edges_traversed = 0;
        size_t paths_found = 0;
        double execution_time_ms = 0.0;
        size_t max_depth_reached = 0;
        double avg_branching_observed = 0.0;
        bool early_terminated = false;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        /// Algorithm that produced this execution; used by the adaptive cost model.
        TraversalAlgorithm algorithm = TraversalAlgorithm::BFS;
    };

    explicit GraphQueryOptimizer(GraphIndexManager& graph_manager);

    /**
     * Generate optimized plan for shortest path query
     */
    Result<OptimizationPlan> optimizeShortestPath(
        std::string_view start_vertex,
        std::string_view target_vertex
    );
    
    Result<OptimizationPlan> optimizeShortestPath(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints
    );

    /**
     * Generate optimized plan for k-hop neighborhood query
     */
    Result<OptimizationPlan> optimizeKHopNeighborhood(
        std::string_view start_vertex,
        int k
    );

    Result<OptimizationPlan> optimizeKHopNeighborhood(
        std::string_view start_vertex,
        int k,
        const QueryConstraints& constraints
    );

    /**
     * Generate optimized plan for pattern matching
     */
    Result<OptimizationPlan> optimizePatternMatch(
        const std::vector<std::string>& pattern_vertices,
        const std::vector<std::pair<std::string, std::string>>& pattern_edges
    );

    Result<OptimizationPlan> optimizePatternMatch(
        const std::vector<std::string>& pattern_vertices,
        const std::vector<std::pair<std::string, std::string>>& pattern_edges,
        const QueryConstraints& constraints
    );

    /**
     * Generate optimized plan for reachability check
     */
    Result<OptimizationPlan> optimizeReachability(
        std::string_view start_vertex,
        std::string_view target_vertex
    );

    Result<OptimizationPlan> optimizeReachability(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints
    );

    /**
     * Execute optimized BFS traversal
     */
    Result<std::vector<std::string>> executeBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    /**
     * Execute optimized DFS traversal
     */
    Result<std::vector<std::string>> executeDFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    /**
     * Execute optimized Dijkstra shortest path
     */
    Result<GraphIndexManager::PathResult> executeDijkstra(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    /**
     * Execute optimized A* search
     */
    Result<GraphIndexManager::PathResult> executeAStar(
        std::string_view start_vertex,
        std::string_view target_vertex,
        std::function<double(const std::string&)> heuristic,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    /**
     * Execute optimized bidirectional search
     */
    Result<GraphIndexManager::PathResult> executeBidirectional(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    /**
     * Collect and update graph statistics
     */
    Result<GraphStatistics> collectStatistics(
        std::optional<std::string_view> graph_id = std::nullopt
    );

    /**
     * Get current graph statistics
     */
    const GraphStatistics& getStatistics() const { return statistics_; }

    /**
     * Estimate selectivity for edge type
     */
    double estimateEdgeTypeSelectivity(std::string_view edge_type) const;

    /**
     * Generate query execution plan explanation
     */
    std::string explainPlan(const OptimizationPlan& plan) const;

    /**
     * Enable/disable plan caching
     */
    void setPlanCachingEnabled(bool enabled) { plan_caching_enabled_ = enabled; }

    /**
     * Clear plan cache
     */
    void clearPlanCache();

    /**
     * Get execution statistics history
     */
    const std::vector<ExecutionStats>& getExecutionHistory() const { 
        return execution_history_; 
    }

    /**
     * @brief Return cumulative observability metrics for this optimizer instance.
     *
     * Metrics cover all queries executed since construction, including counts,
     * timings, cache efficiency, and error rates. Useful for Prometheus export
     * or admin dashboards.
     */
    const GraphQueryMetrics& getQueryMetrics() const { return metrics_; }

    // -----------------------------------------------------------------------
    // Adaptive Cost Model (v1.7.0)
    // -----------------------------------------------------------------------

    /**
     * @brief Per-algorithm learned cost model entry.
     *
     * Tracks an exponential-moving-average (EMA) of observed execution times
     * and a confidence level that grows towards 1.0 as more observations
     * accumulate (saturates at 100 executions).
     */
    struct AlgorithmCostModel {
        double ema_cost_ms = 0.0;   ///< EMA of observed execution durations (ms)
        uint32_t exec_count = 0;    ///< Number of observations so far
        double confidence = 0.0;    ///< [0, 1] – blended into cost estimates

        static constexpr double LEARNING_RATE = 0.1;  ///< EMA alpha
        static constexpr uint32_t MAX_CONF_OBS = 100; ///< Observations for confidence = 1.0

        /// Update EMA with a new observation.
        void update(double observed_ms) {
            if (exec_count == 0) {
                ema_cost_ms = observed_ms;
            } else {
                ema_cost_ms = LEARNING_RATE * observed_ms +
                              (1.0 - LEARNING_RATE) * ema_cost_ms;
            }
            ++exec_count;
            confidence = std::min(1.0, static_cast<double>(exec_count) / MAX_CONF_OBS);
        }
    };

    /**
     * @brief Enable or disable adaptive cost-model learning.
     *
     * When enabled (the default), each call to `recordExecution` that carries
     * a known algorithm tag updates the per-algorithm EMA cost.  The learned
     * cost is then blended into `estimateCost` proportional to its confidence
     * level so that initial plans are still theory-driven, but converge towards
     * actual observed behaviour over time.
     */
    void enableAdaptiveLearning(bool enable) { adaptive_learning_enabled_ = enable; }

    /// Returns whether adaptive learning is currently enabled.
    bool isAdaptiveLearningEnabled() const { return adaptive_learning_enabled_; }

    /**
     * @brief Export the current learned cost model as a JSON string.
     *
     * The returned string is a JSON object mapping algorithm names to their
     * current `ema_cost_ms`, `exec_count`, and `confidence`.  It can be saved
     * to disk and reloaded via `importCostModel` to seed a new optimizer
     * instance with pre-learned data.
     *
     * @return JSON string representation of all per-algorithm cost models.
     */
    std::string exportCostModel() const;

    /**
     * @brief Import a previously exported cost model from a JSON string.
     *
     * Unknown algorithm names or malformed JSON are silently ignored so that
     * a model trained on one version can be loaded safely on a newer version.
     *
     * @param json_model JSON string as returned by `exportCostModel`.
     * @return true if the string parsed successfully; false on JSON errors.
     */
    bool importCostModel(std::string_view json_model);

    /**
     * @brief Return a snapshot of all per-algorithm learned cost models.
     */
    const std::unordered_map<TraversalAlgorithm, AlgorithmCostModel, std::hash<int>>&
        getAlgorithmCostModels() const { return algo_cost_models_; }
    
    /**
     * @brief Optimize constrained path query using PathConstraints
     * 
     * Generates an optimization plan for path finding with complex constraints.
     * This method bridges PathConstraints with the query optimizer to select
     * the best traversal strategy based on constraint types.
     * 
     * @param start_vertex Starting node
     * @param end_vertex Target node
     * @param constraints PathConstraints object with all constraint specifications
     * @return Optimization plan with recommended algorithm and cost estimates
     */
    Result<OptimizationPlan> optimizeConstrainedPath(
        std::string_view start_vertex,
        std::string_view end_vertex,
        const class PathConstraints& constraints
    );

    /**
     * @brief Dry-run explain for a constrained path query (no execution).
     *
     * Identical to `optimizeConstrainedPath` but guaranteed to never execute a
     * traversal. Use this to inspect the chosen algorithm, cost estimate, and
     * constraint summary before committing to actual graph traversal.
     *
     * The returned plan's `explanation` field includes:
     * - Selected algorithm and reason
     * - Active constraint count and types
     * - Estimated cost and time in milliseconds
     *
     * @param start_vertex Starting node
     * @param end_vertex   Target node
     * @param constraints  PathConstraints with all constraint specifications
     * @return OptimizationPlan that can be inspected via `explainPlan()`
     */
    Result<OptimizationPlan> explainConstrainedPath(
        std::string_view start_vertex,
        std::string_view end_vertex,
        const class PathConstraints& constraints
    );

private:
    GraphIndexManager& graph_manager_;
    GraphStatistics statistics_;
    bool plan_caching_enabled_ = true;
    
    // Plan cache: query signature -> plan
    std::unordered_map<std::string, OptimizationPlan> plan_cache_;
    
    // Execution history for adaptive optimization
    std::vector<ExecutionStats> execution_history_;
    static constexpr size_t MAX_HISTORY_SIZE = 1000;

    // Cumulative observability metrics
    mutable GraphQueryMetrics metrics_;

    // Adaptive cost model: per-algorithm EMA cost tracking
    bool adaptive_learning_enabled_ = true;
    std::unordered_map<TraversalAlgorithm, AlgorithmCostModel, std::hash<int>> algo_cost_models_;

    /**
     * Estimate cost for traversal algorithm
     */
    double estimateCost(
        TraversalAlgorithm algorithm,
        size_t estimated_depth,
        const QueryConstraints& constraints
    ) const;

    /**
     * Select best algorithm based on query pattern and statistics
     */
    TraversalAlgorithm selectAlgorithm(
        QueryPattern pattern,
        size_t estimated_depth,
        const QueryConstraints& constraints
    ) const;

    /**
     * Estimate depth for query
     */
    size_t estimateDepth(
        QueryPattern pattern,
        const QueryConstraints& constraints
    ) const;

    /**
     * Generate cache key for plan
     */
    std::string generatePlanCacheKey(
        QueryPattern pattern,
        std::string_view start,
        std::string_view target,
        const QueryConstraints& constraints
    ) const;

    /**
     * Check if parallel execution is beneficial
     */
    bool shouldUseParallel(
        TraversalAlgorithm algorithm,
        size_t estimated_nodes
    ) const;

    /**
     * Record execution statistics
     */
    void recordExecution(const ExecutionStats& stats);
};

} // namespace graph
} // namespace themis
