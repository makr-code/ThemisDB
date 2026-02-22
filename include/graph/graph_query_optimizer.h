/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graph_query_optimizer.h                            ║
  Version:         0.0.28                                             ║
  Last Modified:   2026-02-22 11:29:20                                ║
  Author:          copilot-swe-agent[bot]                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     661                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c97d719  2026-02-22  Add parallel multi-source BFS/DFS implementation (graph/p... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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

        /**
         * @brief Fixed-bucket latency histogram for percentile computation.
         *
         * 10 buckets with upper bounds (ms): 1, 5, 10, 25, 50, 100, 250, 500, 1000, +Inf.
         * Each bucket counts queries whose execution time fell into that bucket.
         * Used to compute approximate p50/p95/p99 latencies for Prometheus export.
         */
        struct LatencyHistogram {
            static constexpr size_t kBucketCount = 10;
            /// Upper-bound (inclusive, ms) for each of the first 9 buckets; bucket 9 is +Inf.
            static constexpr uint64_t kBounds[9] = {1, 5, 10, 25, 50, 100, 250, 500, 1000};

            std::atomic<uint64_t> counts[kBucketCount]{};

            /// Record one query with the given execution duration.
            void record(uint64_t latency_ms) {
                for (size_t i = 0; i < 9; ++i) {
                    if (latency_ms <= kBounds[i]) {
                        counts[i].fetch_add(1, std::memory_order_relaxed);
                        return;
                    }
                }
                counts[9].fetch_add(1, std::memory_order_relaxed);
            }

            /**
             * @brief Compute approximate p-th percentile latency in milliseconds.
             * @param p Percentile in [0.0, 1.0] (e.g. 0.99 for p99).
             * @return Approximate percentile latency in ms, or 0.0 if no data.
             */
            double percentileMs(double p) const {
                uint64_t total = 0;
                for (size_t i = 0; i < kBucketCount; ++i) {
                    total += counts[i].load(std::memory_order_relaxed);
                }
                if (total == 0) return 0.0;

                const uint64_t target = static_cast<uint64_t>(p * static_cast<double>(total));
                uint64_t cumulative = 0;
                for (size_t i = 0; i < kBucketCount; ++i) {
                    uint64_t bc = counts[i].load(std::memory_order_relaxed);
                    if (cumulative + bc > target) {
                        // Interpolate within this bucket
                        const double lower = (i == 0) ? 0.0
                                           : static_cast<double>(kBounds[i - 1]);
                        const double upper = (i < 9) ? static_cast<double>(kBounds[i])
                                           : static_cast<double>(kBounds[8]) * 2.0;
                        if (bc == 0) return lower;
                        const double frac =
                            static_cast<double>(target - cumulative) /
                            static_cast<double>(bc);
                        return lower + frac * (upper - lower);
                    }
                    cumulative += bc;
                }
                return static_cast<double>(kBounds[8]) * 2.0;
            }
        } latency_histogram;

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
     * @brief Token-window query rate limiter (per-second sliding window).
     *
     * When `max_qps > 0`, `allowQuery()` tracks how many queries have been
     * issued in the current 1-second epoch and rejects excess calls by
     * returning `false`.  The window resets atomically when the clock advances
     * to a new second.
     *
     * Thread-safe: all state managed via atomics.
     */
    struct QueryRateLimiter {
        uint32_t max_qps = 0;  ///< 0 = no limit

        /// Returns true if the query is within the rate budget, false otherwise.
        bool allowQuery() {
            if (max_qps == 0) return true;

            const uint64_t now_s = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now().time_since_epoch()
                ).count()
            );

            uint64_t window = window_epoch_s_.load(std::memory_order_relaxed);
            if (now_s != window) {
                // Try to advance the epoch and reset the counter for this new second.
                if (window_epoch_s_.compare_exchange_strong(
                        window, now_s,
                        std::memory_order_acq_rel,
                        std::memory_order_relaxed)) {
                    query_count_.store(1, std::memory_order_release);
                    return true;  // first query of this second
                }
                // Another thread advanced the window; fall through to count check.
            }

            const uint64_t count =
                query_count_.fetch_add(1, std::memory_order_acq_rel);
            return count < static_cast<uint64_t>(max_qps);
        }

    private:
        std::atomic<uint64_t> window_epoch_s_{0};
        std::atomic<uint64_t> query_count_{0};
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
    const std::unordered_map<TraversalAlgorithm, AlgorithmCostModel, std::hash<TraversalAlgorithm>>&
        getAlgorithmCostModels() const { return algo_cost_models_; }

    // -----------------------------------------------------------------------
    // Query Rate Limiter (v1.7.0)
    // -----------------------------------------------------------------------

    /**
     * @brief Set the maximum number of graph queries allowed per second.
     *
     * When set to a non-zero value, each execute* call is checked against the
     * rate budget before execution starts.  Queries that exceed the limit
     * return `ERR_GRAPH_RATE_LIMIT_EXCEEDED` (6406) immediately.
     *
     * @param max_qps Maximum queries per second (0 = no limit, the default).
     */
    void setMaxQueriesPerSecond(uint32_t max_qps) { rate_limiter_.max_qps = max_qps; }

    /// Returns the current max-QPS setting (0 = no limit).
    uint32_t getMaxQueriesPerSecond() const { return rate_limiter_.max_qps; }
    
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
    std::unordered_map<TraversalAlgorithm, AlgorithmCostModel, std::hash<TraversalAlgorithm>> algo_cost_models_;

    // Query rate limiter
    QueryRateLimiter rate_limiter_;

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
