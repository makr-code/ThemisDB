#pragma once

#include "index/graph_index.h"
#include "utils/expected.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <functional>

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
    };

    explicit GraphQueryOptimizer(GraphIndexManager& graph_manager);

    /**
     * Generate optimized plan for shortest path query
     */
    Result<OptimizationPlan> optimizeShortestPath(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints = {}
    );

    /**
     * Generate optimized plan for k-hop neighborhood query
     */
    Result<OptimizationPlan> optimizeKHopNeighborhood(
        std::string_view start_vertex,
        int k,
        const QueryConstraints& constraints = {}
    );

    /**
     * Generate optimized plan for pattern matching
     */
    Result<OptimizationPlan> optimizePatternMatch(
        const std::vector<std::string>& pattern_vertices,
        const std::vector<std::pair<std::string, std::string>>& pattern_edges,
        const QueryConstraints& constraints = {}
    );

    /**
     * Generate optimized plan for reachability check
     */
    Result<OptimizationPlan> optimizeReachability(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints = {}
    );

    /**
     * Execute optimized BFS traversal
     */
    Result<std::vector<std::string>> executeBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints = {},
        ExecutionStats* stats = nullptr
    );

    /**
     * Execute optimized DFS traversal
     */
    Result<std::vector<std::string>> executeDFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints = {},
        ExecutionStats* stats = nullptr
    );

    /**
     * Execute optimized Dijkstra shortest path
     */
    Result<GraphIndexManager::PathResult> executeDijkstra(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints = {},
        ExecutionStats* stats = nullptr
    );

    /**
     * Execute optimized A* search
     */
    Result<GraphIndexManager::PathResult> executeAStar(
        std::string_view start_vertex,
        std::string_view target_vertex,
        std::function<double(const std::string&)> heuristic,
        const QueryConstraints& constraints = {},
        ExecutionStats* stats = nullptr
    );

    /**
     * Execute optimized bidirectional search
     */
    Result<GraphIndexManager::PathResult> executeBidirectional(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints = {},
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

private:
    GraphIndexManager& graph_manager_;
    GraphStatistics statistics_;
    bool plan_caching_enabled_ = true;
    
    // Plan cache: query signature -> plan
    std::unordered_map<std::string, OptimizationPlan> plan_cache_;
    
    // Execution history for adaptive optimization
    std::vector<ExecutionStats> execution_history_;
    static constexpr size_t MAX_HISTORY_SIZE = 1000;

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
