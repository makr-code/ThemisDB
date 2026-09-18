/**
 * @file graph_query_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/graph_index.h"
#include "index/graph_analytics.h"
#include "utils/expected.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <functional>
#include <atomic>
#include <chrono>
#include <future>
#include <mutex>

// Forward declaration to break circular dependency with query module
namespace themis {
namespace query {
template<typename T> class ResultStream;
struct StreamConfig;  // Forward declare StreamConfig for streamBFS/streamDFS parameters
} // namespace query
} // namespace themis

namespace themis {
namespace graph {

class GraphQueryOptimizer {
public:
    enum class TraversalAlgorithm {
        BFS,              // Breadth-First Search - best for shortest path, level exploration
        DFS,              // Depth-First Search - best for deep exploration
        BIDIRECTIONAL,    // Search from both ends - best for long-distance paths
        ASTAR,            // Heuristic-guided - best when heuristic available
        DIJKSTRA          // Weighted shortest path
    };

    enum class QueryPattern {
        SHORTEST_PATH,           // Single shortest path
        ALL_PATHS,              // All paths between nodes
        K_HOP_NEIGHBORS,        // k-hop neighborhood
        PATTERN_MATCH,          // Subgraph pattern matching
        REACHABILITY,           // Simple reachability check
        CONNECTED_COMPONENT     // Component analysis
    };

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

        // Node label statistics for schema-aware cost estimation.
        // node_label_counts["Person"] = number of nodes with label "Person".
        // node_label_selectivity["Person"] = fraction of all nodes with that label [0,1].
        std::unordered_map<std::string, size_t> node_label_counts;
        std::unordered_map<std::string, double> node_label_selectivity;
    };

    struct QueryConstraints {
        std::optional<int> max_depth;
        std::optional<size_t> max_results;
        std::optional<std::string> edge_type;
        std::optional<std::string> graph_id;
        bool unique_vertices = false;
        bool unique_edges = false;
        std::vector<std::string> forbidden_vertices;
        std::vector<std::string> required_vertices;
        uint32_t timeout_ms = 0;
        bool enable_parallel = false;
        uint32_t num_threads = 0;

        // ── Temporal range constraints (Phase 3: Temporal Graph Optimization) ──
        std::optional<int64_t> time_range_start_ms;
        std::optional<int64_t> time_range_end_ms;
        bool time_range_require_containment = false;

        bool hasTemporalRange() const {
            return time_range_start_ms.has_value() || time_range_end_ms.has_value();
        }

        // -----------------------------------------------------------------------
        // Property-graph schema-aware optimizer hints
        // -----------------------------------------------------------------------

        std::vector<std::string> node_labels;

        std::vector<std::string> excluded_edge_types;
        bool use_gpu = false;
        int gpu_device = 0;
        
        QueryConstraints() = default;
    };

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
        std::atomic<uint64_t> plan_cache_evictions{0};

        struct LatencyHistogram {
            static constexpr size_t kBucketCount = 10;
            static constexpr uint64_t kBounds[9] = {1, 5, 10, 25, 50, 100, 250, 500, 1000};

            std::atomic<uint64_t> counts[kBucketCount]{};

            /**
             * @brief Record.
             * @param[in] latency_ms Input parameter.
             * @details Calls: fetch_add().
             */
            void record(uint64_t latency_ms) {
                for (size_t i = 0; i < 9; ++i) {
                    if (latency_ms <= kBounds[i]) {
                        counts[i].fetch_add(1, std::memory_order_relaxed);
                        return;
                    }
                }
                counts[9].fetch_add(1, std::memory_order_relaxed);
            }

            double percentileMs(double p) const {
                uint64_t total = 0;
                for (size_t i = 0; i < kBucketCount; ++i) {
                    total += counts[i].load(std::memory_order_relaxed);
                }
                if (total == 0) {
                  return 0.0;
                }

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
                        if (bc == 0) {
                          return lower;
                        }
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

        double avgExecutionTimeMs() const {
            uint64_t n = total_queries.load(std::memory_order_relaxed);
            return n > 0 ? static_cast<double>(
                               total_execution_time_ms.load(std::memory_order_relaxed)) / n
                         : 0.0;
        }

        double errorRate() const {
            uint64_t n = total_queries.load(std::memory_order_relaxed);
            return n > 0 ? static_cast<double>(
                               failed_queries.load(std::memory_order_relaxed)) / n
                         : 0.0;
        }
    };

    struct QueryRateLimiter {
        uint32_t max_qps = 0;  ///< 0 = no limit

        /**
         * @brief Allow Query.
         * @return True when the operation succeeds.
         * @details Calls: std::chrono::steady_clock::now(), time_since_epoch(), count(), load(), compare_exchange_strong(), store(), fetch_add().
         */
        bool allowQuery() {
            if (max_qps == 0) {
              return true;
            }

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

        std::vector<std::string> active_schema_hints;
        // Shard-aware plan fields (v1.8.0) – backward-compatible:
        // empty / false for single-node execution.
        bool is_distributed = false;               ///< True when query spans >1 shard
        std::vector<std::string> shard_ids;        ///< Participating shard IDs (empty = single-node)
        size_t recommended_parallelism = 1;        ///< Fan-out parallelism across shards
    };

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
        TraversalAlgorithm algorithm = TraversalAlgorithm::BFS;
        double estimated_cost_ms = 0.0;
    };

    // -----------------------------------------------------------------------
    // Incremental Graph Query Execution (v1.9.0)
    // -----------------------------------------------------------------------

    struct GraphChangeSet {
        enum class ChangeType {
            EDGE_ADDED,
            EDGE_REMOVED,
            VERTEX_ADDED,
            VERTEX_REMOVED
        };

        struct Change {
            ChangeType type;
            std::string id;    ///< Edge ID or vertex ID
            std::string from;  ///< Source vertex (EDGE_ADDED / EDGE_REMOVED only)
            std::string to;    ///< Target vertex (EDGE_ADDED / EDGE_REMOVED only)
        };

        std::vector<Change> changes;

        /**
         * @brief Add Edge Added.
         * @param[in] id Input parameter.
         * @param[in] from Input parameter.
         * @param[in] to Input parameter.
         * @details Calls: push_back(), std::move().
         */
        void addEdgeAdded(std::string id, std::string from, std::string to) {
            changes.push_back({ChangeType::EDGE_ADDED, std::move(id),
                               std::move(from), std::move(to)});
        }
        /**
         * @brief Add Edge Removed.
         * @param[in] id Input parameter.
         * @param[in] from Input parameter.
         * @param[in] to Input parameter.
         * @details Calls: push_back(), std::move().
         */
        void addEdgeRemoved(std::string id, std::string from, std::string to) {
            changes.push_back({ChangeType::EDGE_REMOVED, std::move(id),
                               std::move(from), std::move(to)});
        }
        /**
         * @brief Add Vertex Added.
         * @param[in] id Input parameter.
         * @details Calls: push_back(), std::move().
         */
        void addVertexAdded(std::string id) {
            changes.push_back({ChangeType::VERTEX_ADDED, std::move(id), {}, {}});
        }
        /**
         * @brief Add Vertex Removed.
         * @param[in] id Input parameter.
         * @details Calls: push_back(), std::move().
         */
        void addVertexRemoved(std::string id) {
            changes.push_back({ChangeType::VERTEX_REMOVED, std::move(id), {}, {}});
        }
        bool empty() const { return changes.empty(); }
        size_t size() const { return changes.size(); }
    };

    struct IncrementalQueryResult {
        std::vector<std::string> added;    ///< Vertices newly reachable after the change
        std::vector<std::string> removed;  ///< Vertices no longer reachable after the change
        std::vector<std::string> current;  ///< Complete current result set
        bool reexecuted = false;           ///< True if the query was actually re-executed
        ExecutionStats stats;              ///< Execution statistics of the re-execution
    };

    using IncrementalQueryHandle = uint64_t;

    using IncrementalQueryCallback = std::function<void(const IncrementalQueryResult&)>;

    /**
     * @brief Register Incremental BFS.
     * @param[in] start_vertex Input parameter.
     * @param[in] max_depth Input parameter.
     * @param[in] constraints Input parameter.
     * @param[in] callback Input parameter.
     * @return Return value.
     */
    IncrementalQueryHandle registerIncrementalBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        IncrementalQueryCallback callback);

    /**
     * @brief Unregister Incremental Query.
     * @param[in] handle Input parameter.
     */
    void unregisterIncrementalQuery(IncrementalQueryHandle handle);

    /**
     * @brief On Graph Change.
     * @param[in] changes Input parameter.
     * @return Return value.
     */
    size_t onGraphChange(const GraphChangeSet& changes);
    struct SubgraphIsomorphismResult {
        std::vector<std::unordered_map<std::string, std::string>> matches;
        size_t candidate_pairs_checked = 0;
        double execution_time_ms = 0.0;
    };

    /**
     * @brief Graph Query Optimizer.
     * @param[in,out] graph_manager Input/output parameter.
     * @return Return value.
     */
    explicit GraphQueryOptimizer(GraphIndexManager& graph_manager);

    /**
     * @brief Optimize Shortest Path.
     * @param[in] start_vertex Input parameter.
     * @param[in] target_vertex Input parameter.
     * @return Return value.
     */
    Result<OptimizationPlan> optimizeShortestPath(
        std::string_view start_vertex,
        std::string_view target_vertex
    );
    
    /**
     * @brief Optimize Shortest Path.
     * @param[in] start_vertex Input parameter.
     * @param[in] target_vertex Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    Result<OptimizationPlan> optimizeShortestPath(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints
    );

    /**
     * @brief Optimize KHop Neighborhood.
     * @param[in] start_vertex Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    Result<OptimizationPlan> optimizeKHopNeighborhood(
        std::string_view start_vertex,
        int k
    );

    /**
     * @brief Optimize KHop Neighborhood.
     * @param[in] start_vertex Input parameter.
     * @param[in] k Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    Result<OptimizationPlan> optimizeKHopNeighborhood(
        std::string_view start_vertex,
        int k,
        const QueryConstraints& constraints
    );

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
     * @brief Optimize Reachability.
     * @param[in] start_vertex Input parameter.
     * @param[in] target_vertex Input parameter.
     * @return Return value.
     */
    Result<OptimizationPlan> optimizeReachability(
        std::string_view start_vertex,
        std::string_view target_vertex
    );

    /**
     * @brief Optimize Reachability.
     * @param[in] start_vertex Input parameter.
     * @param[in] target_vertex Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    Result<OptimizationPlan> optimizeReachability(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints
    );

    Result<std::vector<std::string>> executeBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    Result<std::vector<std::string>> executeDFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    Result<GraphIndexManager::PathResult> executeDijkstra(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    /**
     * @brief Stream BFS.
     * @param[in] start_vertex Input parameter.
     * @param[in] max_depth Input parameter.
     * @return Return value.
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamBFS(
        std::string_view start_vertex,
        int max_depth
    );

    /**
     * @brief Stream BFS.
     * @param[in] start_vertex Input parameter.
     * @param[in] max_depth Input parameter.
     * @param[in] stream_config Input parameter.
     * @return Return value.
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamBFS(
        std::string_view start_vertex,
        int max_depth,
        const query::StreamConfig& stream_config
    );

    /**
     * @brief Stream BFS.
     * @param[in] start_vertex Input parameter.
     * @param[in] max_depth Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints
    );

    /**
     * @brief Stream BFS.
     * @param[in] start_vertex Input parameter.
     * @param[in] max_depth Input parameter.
     * @param[in] constraints Input parameter.
     * @param[in] stream_config Input parameter.
     * @return Return value.
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        const query::StreamConfig& stream_config
    );

    /**
     * @brief Stream DFS.
     * @param[in] start_vertex Input parameter.
     * @param[in] max_depth Input parameter.
     * @return Return value.
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamDFS(
        std::string_view start_vertex,
        int max_depth
    );

    /**
     * @brief Stream DFS.
     * @param[in] start_vertex Input parameter.
     * @param[in] max_depth Input parameter.
     * @param[in] stream_config Input parameter.
     * @return Return value.
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamDFS(
        std::string_view start_vertex,
        int max_depth,
        const query::StreamConfig& stream_config
    );

    /**
     * @brief Stream DFS.
     * @param[in] start_vertex Input parameter.
     * @param[in] max_depth Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamDFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints
    );

    /**
     * @brief Stream DFS.
     * @param[in] start_vertex Input parameter.
     * @param[in] max_depth Input parameter.
     * @param[in] constraints Input parameter.
     * @param[in] stream_config Input parameter.
     * @return Return value.
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamDFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        const query::StreamConfig& stream_config
    );

    Result<GraphIndexManager::PathResult> executeAStar(
        std::string_view start_vertex,
        std::string_view target_vertex,
        std::function<double(const std::string&)> heuristic,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    Result<GraphIndexManager::PathResult> executeBidirectional(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    Result<SubgraphIsomorphismResult> executeSubgraphIsomorphism(
        const std::vector<std::string>& pattern_vertices,
        const std::vector<std::pair<std::string, std::string>>& pattern_edges,
        ExecutionStats* stats = nullptr
    );

    Result<SubgraphIsomorphismResult> executeSubgraphIsomorphism(
        const std::vector<std::string>& pattern_vertices,
        const std::vector<std::pair<std::string, std::string>>& pattern_edges,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr
    );

    Result<GraphStatistics> collectStatistics(
        std::optional<std::string_view> graph_id = std::nullopt
    );

    const GraphStatistics& getStatistics() const { return statistics_; }

    void setNodeLabelStats(const std::unordered_map<std::string, size_t>& label_counts);

    /**
     * @brief Estimate Edge Type Selectivity.
     * @param[in] edge_type Input parameter.
     * @return Return value.
     */
    double estimateEdgeTypeSelectivity(std::string_view edge_type) const;

    /**
     * @brief Explain Plan.
     * @param[in] plan Input parameter.
     * @return Return value.
     */
    std::string explainPlan(const OptimizationPlan& plan) const;

    /**
     * @brief Set Plan Caching Enabled.
     * @param[in] enabled Input parameter.
     * @details Implements setPlanCachingEnabled without additional internal calls.
     */
    void setPlanCachingEnabled(bool enabled) { plan_caching_enabled_ = enabled; }

    /**
     * @brief Set Plan Cache Max Size.
     * @param[in] max_size Input parameter.
     * @details Implements setPlanCacheMaxSize without additional internal calls.
     */
    void setPlanCacheMaxSize(size_t max_size) { plan_cache_max_size_ = max_size; }

    size_t getPlanCacheMaxSize() const { return plan_cache_max_size_; }

    /**
     * @brief Set Plan Cache TTL.
     * @param[in] ttl Input parameter.
     * @details Implements setPlanCacheTTL without additional internal calls.
     */
    void setPlanCacheTTL(std::chrono::milliseconds ttl) { plan_cache_ttl_ = ttl; }

    std::chrono::milliseconds getPlanCacheTTL() const { return plan_cache_ttl_; }

    size_t getPlanCacheSize() const {
        /**
         * @brief Lk.
         * @param[in] plan_cache_mutex_ Input parameter.
         * @return Return value.
         */
        std::lock_guard<std::mutex> lk(plan_cache_mutex_);
        return plan_cache_.size();
    }

    /**
     * @brief Clear Plan Cache.
     */
    void clearPlanCache();

    const std::vector<ExecutionStats>& getExecutionHistory() const { 
        return execution_history_; 
    }

    const GraphQueryMetrics& getQueryMetrics() const { return metrics_; }

    // -----------------------------------------------------------------------
    // Adaptive Cost Model (v1.7.0)
    // -----------------------------------------------------------------------

    struct AlgorithmCostModel {
        double ema_cost_ms = 0.0;   ///< EMA of observed execution durations (ms)
        uint32_t exec_count = 0;    ///< Number of observations so far
        double confidence = 0.0;    ///< [0, 1] – blended into cost estimates

        static constexpr double LEARNING_RATE = 0.1;  ///< EMA alpha
        static constexpr uint32_t MAX_CONF_OBS = 100; ///< Observations for confidence = 1.0

        /**
         * @brief Update.
         * @param[in] observed_ms Input parameter.
         * @details Calls: std::min().
         */
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
     * @brief Enable Adaptive Learning.
     * @param[in] enable Input parameter.
     * @details Implements enableAdaptiveLearning without additional internal calls.
     */
    void enableAdaptiveLearning(bool enable) { adaptive_learning_enabled_ = enable; }

    bool isAdaptiveLearningEnabled() const { return adaptive_learning_enabled_; }

    /**
     * @brief Export Cost Model.
     * @return Return value.
     */
    std::string exportCostModel() const;

    /**
     * @brief Import Cost Model.
     * @param[in] json_model Input parameter.
     * @return True when the operation succeeds.
     */
    bool importCostModel(std::string_view json_model);

    const std::unordered_map<TraversalAlgorithm, AlgorithmCostModel, std::hash<TraversalAlgorithm>>&
        getAlgorithmCostModels() const { return algo_cost_models_; }

    // -----------------------------------------------------------------------
    // Cost Model Calibration from Execution History (v1.8.0)
    // -----------------------------------------------------------------------

    struct AlgorithmCalibrationStats {
        double mean_execution_ms   = 0.0;  ///< Mean actual execution time (ms)
        double stddev_execution_ms = 0.0;  ///< Standard deviation of execution times (ms)
        double min_execution_ms    = 0.0;  ///< Minimum observed execution time (ms)
        double max_execution_ms    = 0.0;  ///< Maximum observed execution time (ms)
        size_t sample_count        = 0;    ///< Number of observations used

        // Cost estimation accuracy fields (populated when estimated_cost_ms > 0)
        double mean_estimated_ms        = 0.0;  ///< Mean of pre-execution estimated times (ms)
        double mean_absolute_error_ms   = 0.0;  ///< Mean |actual - estimated| (ms)
        double cost_ratio               = 0.0;  ///< mean_estimated_ms / mean_execution_ms; >1 = over-estimated, <1 = under-estimated; 0 when no estimate data
        size_t estimation_sample_count  = 0;    ///< Records that had estimated_cost_ms > 0
    };

    struct CostModelCalibrationReport {
        std::unordered_map<TraversalAlgorithm,
                           AlgorithmCalibrationStats,
                           std::hash<TraversalAlgorithm>> algorithm_stats;

        size_t total_samples         = 0;  ///< Total execution records analysed
        size_t algorithms_calibrated = 0;  ///< Number of algorithm models re-seeded
    };

    /**
     * @brief Calibrate From History.
     * @return Return value.
     */
    CostModelCalibrationReport calibrateFromHistory();

    static constexpr size_t MIN_CALIBRATION_SAMPLES = 5;

    /**
     * @brief ----------------------------------------------------------------------- Query Rate Limiter (v1.
     * @param[in] max_qps Input parameter.
     * @details 7.0) ----------------------------------------------------------------------- Implements setMaxQueriesPerSecond without additional internal calls.
     */

    void setMaxQueriesPerSecond(uint32_t max_qps) { rate_limiter_.max_qps = max_qps; }

    uint32_t getMaxQueriesPerSecond() const { return rate_limiter_.max_qps; }
    
    /**
     * @brief Optimize Constrained Path.
     * @param[in] start_vertex Input parameter.
     * @param[in] end_vertex Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    Result<OptimizationPlan> optimizeConstrainedPath(
        std::string_view start_vertex,
        std::string_view end_vertex,
        const class PathConstraints& constraints
    );

    /**
     * @brief Explain Constrained Path.
     * @param[in] start_vertex Input parameter.
     * @param[in] end_vertex Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    Result<OptimizationPlan> explainConstrainedPath(
        std::string_view start_vertex,
        std::string_view end_vertex,
        const class PathConstraints& constraints
    );

    /**
     * @brief ----------------------------------------------------------------------- Temporal Graph Query Optimization (Phase 3) -----------------------------------------------------------------------
     * @param[in] start_vertex Input parameter.
     * @param[in] max_depth Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */

    Result<OptimizationPlan> optimizeTemporalTraversal(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints
    );

    Result<std::vector<std::string>> executeTemporalBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr);

    /**
     * @brief Analytics Module Integration (Issue #1821) -----------------------------------------------------------------------
     * @param[in,out] analytics Input/output parameter.
     */

    void attachAnalytics(GraphAnalytics& analytics);

    /**
     * @brief Detach Analytics.
     */
    void detachAnalytics();

    bool hasAnalytics() const { return analytics_ != nullptr; }

    Result<std::vector<GraphAnalytics::PathInfo>> executeKShortestPaths(
        std::string_view source,
        std::string_view target,
        int k,
        const QueryConstraints& constraints,
        std::string_view weight_attr = "",
        ExecutionStats* stats = nullptr
    );

private:
    // Pointer to an optional analytics instance for algorithm reuse (not owned).
    GraphAnalytics* analytics_ = nullptr;
    GraphIndexManager& graph_manager_;
    GraphStatistics statistics_;
    bool plan_caching_enabled_ = true;

    // -----------------------------------------------------------------------
    // Plan cache with LRU eviction and TTL expiry
    // -----------------------------------------------------------------------

    struct PlanCacheEntry {
        OptimizationPlan plan;
        std::chrono::steady_clock::time_point inserted_at;
    };

    size_t plan_cache_max_size_ = 0;

    std::chrono::milliseconds plan_cache_ttl_{0};

    mutable std::mutex plan_cache_mutex_;

    std::list<std::string> plan_cache_lru_;

    std::unordered_map<std::string,
                       std::pair<PlanCacheEntry, std::list<std::string>::iterator>>
        plan_cache_;

    /**
     * @brief Plan Cache Insert.
     * @param[in] key Input parameter.
     * @param[in] plan Input parameter.
     */
    void planCacheInsert(const std::string& key, const OptimizationPlan& plan);

    /**
     * @brief Plan Cache Lookup.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::optional<OptimizationPlan> planCacheLookup(const std::string& key);
    
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

    // -----------------------------------------------------------------------
    // Incremental query execution state
    // -----------------------------------------------------------------------

    struct IncrementalQueryEntry {
        IncrementalQueryHandle handle;
        std::string start_vertex;
        int max_depth = 0;
        QueryConstraints constraints;
        IncrementalQueryCallback callback;
        std::unordered_set<std::string> last_result;
    };

    std::unordered_map<IncrementalQueryHandle, IncrementalQueryEntry> incremental_queries_;
    std::atomic<uint64_t> next_incremental_handle_{1};

    /**
     * @brief Estimate Cost.
     * @param[in] algorithm Input parameter.
     * @param[in] estimated_depth Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    double estimateCost(
        TraversalAlgorithm algorithm,
        size_t estimated_depth,
        const QueryConstraints& constraints
    ) const;

    /**
     * @brief Select Algorithm.
     * @param[in] pattern Input parameter.
     * @param[in] estimated_depth Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    TraversalAlgorithm selectAlgorithm(
        QueryPattern pattern,
        size_t estimated_depth,
        const QueryConstraints& constraints
    ) const;

    /**
     * @brief Estimate Depth.
     * @param[in] pattern Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    size_t estimateDepth(
        QueryPattern pattern,
        const QueryConstraints& constraints
    ) const;

    /**
     * @brief Generate Plan Cache Key.
     * @param[in] pattern Input parameter.
     * @param[in] start Input parameter.
     * @param[in] target Input parameter.
     * @param[in] constraints Input parameter.
     * @return Return value.
     */
    std::string generatePlanCacheKey(
        QueryPattern pattern,
        std::string_view start,
        std::string_view target,
        const QueryConstraints& constraints
    ) const;

    std::string generateStructuralCacheKey(
        QueryPattern pattern,
        const QueryConstraints& constraints,
        std::optional<size_t> depth_hint = std::nullopt
    ) const;

    /**
     * @brief Should Use Parallel.
     * @param[in] algorithm Input parameter.
     * @param[in] estimated_nodes Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldUseParallel(
        TraversalAlgorithm algorithm,
        size_t estimated_nodes
    ) const;

    /**
     * @brief Record Execution.
     * @param[in] stats Input parameter.
     */
    void recordExecution(const ExecutionStats& stats);
};

} // namespace graph
} // namespace themis

