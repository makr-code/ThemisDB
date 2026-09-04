/**
 * @file graph_query_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

        // Node label statistics for schema-aware cost estimation.
        // node_label_counts["Person"] = number of nodes with label "Person".
        // node_label_selectivity["Person"] = fraction of all nodes with that label [0,1].
        std::unordered_map<std::string, size_t> node_label_counts;
        std::unordered_map<std::string, double> node_label_selectivity;
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

        // ── Temporal range constraints (Phase 3: Temporal Graph Optimization) ──
        /// Start of the query time window in milliseconds since epoch.
        /// When set together with time_range_end_ms, only edges whose validity
        /// period overlaps [time_range_start_ms, time_range_end_ms] are traversed.
        /// Null means unbounded past (include edges with any valid_from).
        std::optional<int64_t> time_range_start_ms;
        /// End of the query time window in milliseconds since epoch.
        /// Null means unbounded future (include edges with any valid_to).
        std::optional<int64_t> time_range_end_ms;
        /// When true, only edges whose validity is **fully contained** within
        /// [time_range_start_ms, time_range_end_ms] are traversed.
        /// When false (default), edges with any temporal overlap are included.
        bool time_range_require_containment = false;

        /// Returns true if any temporal range constraint is active.
        bool hasTemporalRange() const {
            return time_range_start_ms.has_value() || time_range_end_ms.has_value();
        }

        // -----------------------------------------------------------------------
        // Property-graph schema-aware optimizer hints
        // -----------------------------------------------------------------------

        /// Node label hints: only traverse (and include in results) nodes that
        /// carry at least one of these labels.  Labels are matched against the
        /// comma-separated "_labels" field stored on each node entity by
        /// PropertyGraphManager.  OR semantics – a node is kept when it has ANY
        /// of the listed labels.  Empty = no label filtering (default).
        std::vector<std::string> node_labels;

        /// Edge type exclusions: during cost estimation these type strings are
        /// subtracted from the effective edge fanout, reducing the estimated
        /// search space.  Types are matched against the "_type" field of edge
        /// entities.  Empty = no type exclusions (default).
        std::vector<std::string> excluded_edge_types;
        /// When true, route BFS/DFS through the GPU-accelerated traversal path
        /// (GPUGraphTraversal) for large graphs.  Automatically falls back to
        /// the CPU path when no GPU hardware is available or when the graph is
        /// smaller than GPUGraphTraversal::Config::min_vertices_for_gpu.
        bool use_gpu = false;
        /// GPU device index (0-based) used when use_gpu = true.
        int gpu_device = 0;
        
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
        std::atomic<uint64_t> plan_cache_evictions{0};

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

        /// Human-readable descriptions of active schema hints (node labels,
        /// excluded edge types, etc.) that influenced cost estimation and
        /// algorithm selection.  Populated during plan construction; empty when
        /// no schema hints are provided.
        std::vector<std::string> active_schema_hints;
        // Shard-aware plan fields (v1.8.0) – backward-compatible:
        // empty / false for single-node execution.
        bool is_distributed = false;               ///< True when query spans >1 shard
        std::vector<std::string> shard_ids;        ///< Participating shard IDs (empty = single-node)
        size_t recommended_parallelism = 1;        ///< Fan-out parallelism across shards
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
        /// Estimated execution time (ms) from the cost model before execution.
        /// Set automatically by execute* methods to enable calibration accuracy
        /// tracking.  Zero when no estimate is available.
        double estimated_cost_ms = 0.0;
    };

    // -----------------------------------------------------------------------
    // Incremental Graph Query Execution (v1.9.0)
    // -----------------------------------------------------------------------

    /**
     * @brief Represents an atomic set of graph changes (edge/vertex additions
     *        and removals) that can be applied to trigger incremental query
     *        re-execution.
     */
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

        void addEdgeAdded(std::string id, std::string from, std::string to) {
            changes.push_back({ChangeType::EDGE_ADDED, std::move(id),
                               std::move(from), std::move(to)});
        }
        void addEdgeRemoved(std::string id, std::string from, std::string to) {
            changes.push_back({ChangeType::EDGE_REMOVED, std::move(id),
                               std::move(from), std::move(to)});
        }
        void addVertexAdded(std::string id) {
            changes.push_back({ChangeType::VERTEX_ADDED, std::move(id), {}, {}});
        }
        void addVertexRemoved(std::string id) {
            changes.push_back({ChangeType::VERTEX_REMOVED, std::move(id), {}, {}});
        }
        bool empty() const { return changes.empty(); }
        size_t size() const { return changes.size(); }
    };

    /**
     * @brief Result of an incremental query re-execution: the delta between
     *        the previous result set and the newly computed result set.
     */
    struct IncrementalQueryResult {
        std::vector<std::string> added;    ///< Vertices newly reachable after the change
        std::vector<std::string> removed;  ///< Vertices no longer reachable after the change
        std::vector<std::string> current;  ///< Complete current result set
        bool reexecuted = false;           ///< True if the query was actually re-executed
        ExecutionStats stats;              ///< Execution statistics of the re-execution
    };

    /// Opaque handle returned by registerIncrementalBFS(); use it to unregister.
    using IncrementalQueryHandle = uint64_t;

    /// Callback invoked for each registered query whenever the graph changes
    /// and the query result is affected.
    using IncrementalQueryCallback = std::function<void(const IncrementalQueryResult&)>;

    /**
     * @brief Register a BFS query for incremental re-execution on graph changes.
     *
     * Executes the BFS query immediately to capture the initial result set,
     * then stores the query so that subsequent calls to `onGraphChange()` can
     * re-execute it when the affected portion of the graph changes.
     *
     * The registered callback receives an `IncrementalQueryResult` that contains:
     * - `added`: vertices newly reachable after the change
     * - `removed`: vertices no longer reachable after the change
     * - `current`: the complete updated result set
     *
     * @param start_vertex Starting vertex for BFS traversal
     * @param max_depth    Maximum BFS depth
     * @param constraints  Query constraints (edge type, forbidden vertices, etc.)
     * @param callback     Called each time the query result changes
     * @return Handle that can be passed to `unregisterIncrementalQuery()`
     */
    IncrementalQueryHandle registerIncrementalBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        IncrementalQueryCallback callback);

    /**
     * @brief Unregister a previously registered incremental query.
     *
     * After this call the callback will never be invoked again, even if
     * `onGraphChange()` is called with relevant changes.
     *
     * @param handle Handle returned by `registerIncrementalBFS()`
     */
    void unregisterIncrementalQuery(IncrementalQueryHandle handle);

    /**
     * @brief Notify the optimizer that the graph has changed.
     *
     * For each registered incremental query whose result might be affected by
     * the supplied changes, the query is re-executed and its callback is invoked
     * with the delta (`added` / `removed` vertices and the complete `current`
     * result set).
     *
     * A query is considered affected when:
     * - Any changed vertex (or edge endpoint) appears in its previous result set, or
     * - The query's start vertex is directly affected by a vertex change.
     *
     * Queries whose previous result set is completely disjoint from the changed
     * vertices are skipped to avoid unnecessary re-execution.
     *
     * @param changes Set of graph changes (edge/vertex additions and removals)
     * @return Number of registered queries that were re-executed
     */
    size_t onGraphChange(const GraphChangeSet& changes);
    /**
     * @brief Result of a subgraph isomorphism (pattern matching) query.
     *
     * Each element of `matches` is a mapping from pattern vertex label to the
     * actual vertex ID that was matched in the data graph.  For example, if the
     * pattern has vertices {"u", "v"} and the match maps u→"A", v→"B", the
     * corresponding entry is {{"u","A"}, {"v","B"}}.
     */
    struct SubgraphIsomorphismResult {
        /// All mappings pattern_vertex → data_vertex found in the graph.
        std::vector<std::unordered_map<std::string, std::string>> matches;
        /// Number of (pattern_vertex, data_vertex) candidate pairs evaluated.
        size_t candidate_pairs_checked = 0;
        /// Total execution time in milliseconds.
        double execution_time_ms = 0.0;
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
     * @brief Stream BFS traversal results for large path sets (uses default config).
     *
     * Executes BFS from `start_vertex` up to `max_depth` hops and returns
     * the discovered vertices as a lazy `ResultStream`.  Consumers can page
     * through the result set in configurable batches without loading all
     * vertices into memory at once.
     *
     * @param start_vertex  Starting vertex for the traversal
     * @param max_depth     Maximum BFS depth (inclusive)
     * @return Streaming iterator over discovered vertex IDs, or an error
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamBFS(
        std::string_view start_vertex,
        int max_depth
    );

    /**
     * @brief Stream BFS traversal results for large path sets with custom config.
     *
     * Executes BFS from `start_vertex` up to `max_depth` hops and returns
     * the discovered vertices as a lazy `ResultStream`.  Consumers can page
     * through the result set in configurable batches without loading all
     * vertices into memory at once.
     *
     * @param start_vertex  Starting vertex for the traversal
     * @param max_depth     Maximum BFS depth (inclusive)
     * @param stream_config Stream batch / buffer configuration
     * @return Streaming iterator over discovered vertex IDs, or an error
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamBFS(
        std::string_view start_vertex,
        int max_depth,
        const query::StreamConfig& stream_config
    );

    /**
     * @brief Stream BFS traversal results with constraints (uses default config).
     *
     * Executes BFS from `start_vertex` up to `max_depth` hops and returns
     * the discovered vertices as a lazy `ResultStream`.  Consumers can page
     * through the result set in configurable batches without loading all
     * vertices into memory at once.
     *
     * @param start_vertex  Starting vertex for the traversal
     * @param max_depth     Maximum BFS depth (inclusive)
     * @param constraints   Query constraints (timeout, forbidden vertices, …)
     * @return Streaming iterator over discovered vertex IDs, or an error
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints
    );

    /**
     * @brief Stream BFS traversal results with constraints and custom config.
     *
     * Executes BFS from `start_vertex` up to `max_depth` hops and returns
     * the discovered vertices as a lazy `ResultStream`.  Consumers can page
     * through the result set in configurable batches without loading all
     * vertices into memory at once.
     *
     * @param start_vertex  Starting vertex for the traversal
     * @param max_depth     Maximum BFS depth (inclusive)
     * @param constraints   Query constraints (timeout, forbidden vertices, …)
     * @param stream_config Stream batch / buffer configuration
     * @return Streaming iterator over discovered vertex IDs, or an error
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        const query::StreamConfig& stream_config
    );

    /**
     * @brief Stream DFS traversal results for large path sets (uses default config).
     *
     * Executes DFS from `start_vertex` up to `max_depth` hops and returns
     * the discovered vertices as a lazy `ResultStream`.  Consumers can page
     * through the result set in configurable batches without loading all
     * vertices into memory at once.
     *
     * @param start_vertex  Starting vertex for the traversal
     * @param max_depth     Maximum DFS depth (inclusive)
     * @return Streaming iterator over discovered vertex IDs, or an error
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamDFS(
        std::string_view start_vertex,
        int max_depth
    );

    /**
     * @brief Stream DFS traversal results for large path sets with custom config.
     *
     * Executes DFS from `start_vertex` up to `max_depth` hops and returns
     * the discovered vertices as a lazy `ResultStream`.  Consumers can page
     * through the result set in configurable batches without loading all
     * vertices into memory at once.
     *
     * @param start_vertex  Starting vertex for the traversal
     * @param max_depth     Maximum DFS depth (inclusive)
     * @param stream_config Stream batch / buffer configuration
     * @return Streaming iterator over discovered vertex IDs, or an error
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamDFS(
        std::string_view start_vertex,
        int max_depth,
        const query::StreamConfig& stream_config
    );

    /**
     * @brief Stream DFS traversal results with constraints (uses default config).
     *
     * Executes DFS from `start_vertex` up to `max_depth` hops and returns
     * the discovered vertices as a lazy `ResultStream`.  Consumers can page
     * through the result set in configurable batches without loading all
     * vertices into memory at once.
     *
     * @param start_vertex  Starting vertex for the traversal
     * @param max_depth     Maximum DFS depth (inclusive)
     * @param constraints   Query constraints (timeout, forbidden vertices, …)
     * @return Streaming iterator over discovered vertex IDs, or an error
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamDFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints
    );

    /**
     * @brief Stream DFS traversal results with constraints and custom config.
     *
     * Executes DFS from `start_vertex` up to `max_depth` hops and returns
     * the discovered vertices as a lazy `ResultStream`.  Consumers can page
     * through the result set in configurable batches without loading all
     * vertices into memory at once.
     *
     * @param start_vertex  Starting vertex for the traversal
     * @param max_depth     Maximum DFS depth (inclusive)
     * @param constraints   Query constraints (timeout, forbidden vertices, …)
     * @param stream_config Stream batch / buffer configuration
     * @return Streaming iterator over discovered vertex IDs, or an error
     */
    Result<std::shared_ptr<query::ResultStream<std::string>>> streamDFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        const query::StreamConfig& stream_config
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
     * @brief Execute a subgraph isomorphism (pattern matching) query.
     *
     * Finds all injective mappings from the pattern graph onto subgraphs of the
     * data graph.  The algorithm is a VF2-style recursive backtracking search:
     *
     *  1. The pattern is described by a list of vertex labels and directed edges
     *     (pairs of labels).
     *  2. For each candidate extension (pattern_vertex → data_vertex), the method
     *     checks structural feasibility: every edge in the pattern that connects
     *     already-mapped vertices must be present in the data graph.
     *  3. The mapping is injective: each data vertex may appear at most once.
     *
     * @param pattern_vertices  Ordered list of vertex labels in the pattern graph.
     * @param pattern_edges     Directed edges as (source_label, target_label) pairs.
     * @param stats             Optional output execution statistics.
     * @return SubgraphIsomorphismResult containing all matches, or an error if the
     *         query timed out before the first result could be produced.
     */
    Result<SubgraphIsomorphismResult> executeSubgraphIsomorphism(
        const std::vector<std::string>& pattern_vertices,
        const std::vector<std::pair<std::string, std::string>>& pattern_edges,
        ExecutionStats* stats = nullptr
    );

    /**
     * @brief Execute a subgraph isomorphism (pattern matching) query with constraints.
     *
     * Finds all injective mappings from the pattern graph onto subgraphs of the
     * data graph.  The algorithm is a VF2-style recursive backtracking search:
     *
     *  1. The pattern is described by a list of vertex labels and directed edges
     *     (pairs of labels).
     *  2. For each candidate extension (pattern_vertex → data_vertex), the method
     *     checks structural feasibility: every edge in the pattern that connects
     *     already-mapped vertices must be present in the data graph.
     *  3. The mapping is injective: each data vertex may appear at most once.
     *
     * Constraints supported via `QueryConstraints`:
     *  - `timeout_ms`: abort and return partial results after the given deadline.
     *  - `max_results`: stop after the first N matches are found.
     *  - `forbidden_vertices`: data vertices that must not appear in any match.
     *
     * @param pattern_vertices  Ordered list of vertex labels in the pattern graph.
     * @param pattern_edges     Directed edges as (source_label, target_label) pairs.
     * @param constraints       Optional execution constraints.
     * @param stats             Optional output execution statistics.
     * @return SubgraphIsomorphismResult containing all matches, or an error if the
     *         query timed out before the first result could be produced.
     */
    Result<SubgraphIsomorphismResult> executeSubgraphIsomorphism(
        const std::vector<std::string>& pattern_vertices,
        const std::vector<std::pair<std::string, std::string>>& pattern_edges,
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
     * @brief Provide per-label node counts for schema-aware cost estimation.
     *
     * Callers may supply label statistics obtained from PropertyGraphManager
     * (e.g. via `getNodesByLabel`) so that the optimizer can apply label
     * selectivity when `QueryConstraints::node_labels` is set.
     *
     * @param label_counts Map from label string to absolute node count with that label.
     *                     The optimizer derives selectivity automatically using the
     *                     current `statistics_.vertex_count`.
     */
    void setNodeLabelStats(const std::unordered_map<std::string, size_t>& label_counts);

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
     * Set the maximum number of entries in the plan cache (LRU eviction).
     * When the cache reaches this limit, the least recently used entry is
     * evicted before a new one is inserted.  Set to 0 for unlimited size.
     * Default: 0 (unlimited).
     */
    void setPlanCacheMaxSize(size_t max_size) { plan_cache_max_size_ = max_size; }

    /// Returns the configured maximum cache size (0 = unlimited).
    size_t getPlanCacheMaxSize() const { return plan_cache_max_size_; }

    /**
     * Set the time-to-live for plan cache entries.
     * Cached entries older than `ttl` are treated as expired on the next lookup
     * and will be evicted lazily.  Set to zero duration to disable TTL.
     * Default: 0 (no TTL).
     */
    void setPlanCacheTTL(std::chrono::milliseconds ttl) { plan_cache_ttl_ = ttl; }

    /// Returns the configured TTL (zero = no TTL expiry).
    std::chrono::milliseconds getPlanCacheTTL() const { return plan_cache_ttl_; }

    /// Returns the current number of entries in the plan cache.  Thread-safe.
    size_t getPlanCacheSize() const {
        std::lock_guard<std::mutex> lk(plan_cache_mutex_);
        return plan_cache_.size();
    }

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
    // Cost Model Calibration from Execution History (v1.8.0)
    // -----------------------------------------------------------------------

    /**
     * @brief Per-algorithm statistics derived from execution history.
     *
     * In addition to actual execution time statistics, records how well the
     * cost model's estimates compared to actual execution times when
     * `ExecutionStats::estimated_cost_ms` was populated during execution.
     * These accuracy fields are zero when no estimation data is available.
     */
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

    /**
     * @brief Report produced by calibrateFromHistory().
     *
     * Contains per-algorithm statistics derived from the full execution
     * history, and summary counts of how many algorithm models were updated.
     */
    struct CostModelCalibrationReport {
        /// Per-algorithm statistics from the execution history.
        std::unordered_map<TraversalAlgorithm,
                           AlgorithmCalibrationStats,
                           std::hash<TraversalAlgorithm>> algorithm_stats;

        size_t total_samples         = 0;  ///< Total execution records analysed
        size_t algorithms_calibrated = 0;  ///< Number of algorithm models re-seeded
    };

    /**
     * @brief Recalibrate per-algorithm cost models from the full execution history.
     *
     * Unlike the incremental EMA update performed by `recordExecution()`, this
     * method performs a **batch recalibration**: it groups all entries in the
     * execution history by algorithm, computes the statistical mean, standard
     * deviation, min and max of actual execution times, and re-seeds the EMA
     * cost model for each algorithm that has at least
     * `MIN_CALIBRATION_SAMPLES` observations.
     *
     * Re-seeding replaces `ema_cost_ms` with the historical mean and resets
     * the confidence to `min(1.0, sample_count / MAX_CONF_OBS)` so the model
     * immediately reflects the measured behaviour rather than waiting for the
     * EMA to converge.
     *
     * When history entries contain a non-zero `estimated_cost_ms` (populated
     * automatically by the execute* methods), the report also includes
     * cost accuracy statistics: `mean_estimated_ms`, `mean_absolute_error_ms`,
     * and `cost_ratio` in each `AlgorithmCalibrationStats`.
     *
     * Algorithms with fewer than `MIN_CALIBRATION_SAMPLES` observations are
     * left unchanged (they appear in the report with `sample_count < threshold`
     * but `algorithms_calibrated` is not incremented for them).
     *
     * @note Has no effect when `adaptive_learning_enabled_` is false; in that
     *       case the report still contains statistics but no models are updated.
     *
     * @return CostModelCalibrationReport with per-algorithm statistics and
     *         summary counts.
     */
    CostModelCalibrationReport calibrateFromHistory();

    /// Minimum number of history samples required before an algorithm's
    /// cost model is recalibrated by `calibrateFromHistory()`.
    static constexpr size_t MIN_CALIBRATION_SAMPLES = 5;

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

    // -----------------------------------------------------------------------
    // Temporal Graph Query Optimization (Phase 3)
    // -----------------------------------------------------------------------

    /**
     * @brief Generate an optimized plan for a time-ranged graph traversal.
     *
     * Produces a cost-based optimization plan for BFS/DFS traversal that
     * restricts traversed edges to those whose validity period overlaps (or is
     * fully contained in, when time_range_require_containment = true) the
     * time window specified by [constraints.time_range_start_ms,
     * constraints.time_range_end_ms].
     *
     * The cost model applies a temporal selectivity reduction relative to an
     * unconstrained traversal: fewer edges are traversed when a time range is
     * active, lowering the estimated traversal cost.
     *
     * @param start_vertex Starting node for the traversal
     * @param max_depth    Maximum BFS/DFS depth
     * @param constraints  QueryConstraints with time_range_start_ms and/or
     *                     time_range_end_ms set.  Other constraint fields
     *                     (forbidden/required vertices, edge type, etc.) are
     *                     also respected.
     * @return Optimization plan with selected algorithm, cost estimate, and
     *         explanation indicating the active temporal range.
     */
    Result<OptimizationPlan> optimizeTemporalTraversal(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints
    );

    /**
     * @brief Execute a time-range-filtered BFS traversal.
     *
     * Performs breadth-first search from `start_vertex` up to `max_depth`
     * levels, traversing **only edges whose validity period has any overlap**
     * (or full containment when time_range_require_containment = true) with
     * the time window [constraints.time_range_start_ms,
     * constraints.time_range_end_ms].
     *
     * When neither temporal bound is set in constraints, the method falls
     * back to a standard unconstrained BFS (same as `executeBFS`).
     *
     * Observability: execution statistics are recorded and observability
     * metrics are updated identically to `executeBFS`.
     *
     * @param start_vertex Starting node
     * @param max_depth    Maximum BFS depth
     * @param constraints  QueryConstraints — temporal range fields drive edge
     *                     filtering; other fields (forbidden vertices, timeout,
     *                     max_results) are also honoured.
     * @param stats        Optional output for execution statistics
     * @return Discovered reachable nodes in BFS order, or an error on failure
     */
    Result<std::vector<std::string>> executeTemporalBFS(
        std::string_view start_vertex,
        int max_depth,
        const QueryConstraints& constraints,
        ExecutionStats* stats = nullptr);

    // Analytics Module Integration (Issue #1821)
    // -----------------------------------------------------------------------

    /**
     * @brief Attach a GraphAnalytics instance to enable algorithm reuse.
     *
     * When an analytics instance is attached, the optimizer can delegate
     * complex analytics operations (e.g., k-shortest paths via Yen's
     * algorithm) to the analytics module instead of re-implementing them.
     * The caller retains ownership; the pointer must remain valid for the
     * lifetime of this optimizer or until `detachAnalytics()` is called.
     *
     * @param analytics Reference to a GraphAnalytics instance.
     */
    void attachAnalytics(GraphAnalytics& analytics);

    /**
     * @brief Detach the previously attached analytics instance.
     *
     * After calling this, `executeKShortestPaths` will return an error
     * until a new analytics instance is attached.
     */
    void detachAnalytics();

    /**
     * @brief Returns true if an analytics instance is currently attached.
     */
    bool hasAnalytics() const { return analytics_ != nullptr; }

    /**
     * @brief Execute K-Shortest Paths using the attached analytics module.
     *
     * Delegates to `GraphAnalytics::kShortestPaths` (Yen's algorithm) to
     * find the `k` shortest loopless paths from `source` to `target`.
     * This avoids duplicating the Yen's algorithm implementation inside the
     * query optimizer and reuses the production-tested analytics code.
     *
     * Execution statistics are recorded so the adaptive cost model learns
     * from k-shortest-paths workloads.
     *
     * @param source      Source vertex primary key.
     * @param target      Target vertex primary key.
     * @param k           Number of shortest paths to return (must be > 0).
     * @param constraints Query constraints; `timeout_ms` and rate limiting apply.
     * @param weight_attr Optional edge weight attribute name (empty = default `_weight`).
     * @param stats       Optional output parameter for execution statistics.
     * @return Vector of PathInfo results (at most k paths), or an error.
     *         Returns ERR_GRAPH_PATH_NOT_FOUND if no path exists.
     */
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

    /// A single cache entry: the cached plan plus its insertion timestamp.
    struct PlanCacheEntry {
        OptimizationPlan plan;
        std::chrono::steady_clock::time_point inserted_at;
    };

    /// Maximum number of cache entries (0 = unlimited).
    size_t plan_cache_max_size_ = 0;

    /// Per-entry TTL; entries older than this are expired (zero = no expiry).
    std::chrono::milliseconds plan_cache_ttl_{0};

    /// Mutex protecting plan_cache_ and plan_cache_lru_ for thread-safe
    /// concurrent access.  Declared mutable so that const lookup methods
    /// (getPlanCacheSize) can also take the lock.
    mutable std::mutex plan_cache_mutex_;

    /// LRU access-order list: front = most recently used, back = LRU victim.
    std::list<std::string> plan_cache_lru_;

    /// Plan cache: key → (entry, iterator into lru list).
    std::unordered_map<std::string,
                       std::pair<PlanCacheEntry, std::list<std::string>::iterator>>
        plan_cache_;

    /// Insert or update a plan in the cache, enforcing LRU size limit.
    /// Thread-safe: acquires plan_cache_mutex_ internally.
    void planCacheInsert(const std::string& key, const OptimizationPlan& plan);

    /// Look up a plan in the cache.  Returns the cached plan by value (empty
    /// optional when not found or expired).  Thread-safe: acquires
    /// plan_cache_mutex_ internally, so callers receive a safe copy.
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
        /// Previous result set; used to compute added/removed deltas.
        std::unordered_set<std::string> last_result;
    };

    std::unordered_map<IncrementalQueryHandle, IncrementalQueryEntry> incremental_queries_;
    std::atomic<uint64_t> next_incremental_handle_{1};

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
     * Generate cache key for plan (exact: includes vertex IDs)
     */
    std::string generatePlanCacheKey(
        QueryPattern pattern,
        std::string_view start,
        std::string_view target,
        const QueryConstraints& constraints
    ) const;

    /**
     * Generate structural cache key for plan reuse across queries with the same
     * pattern and constraints but different vertex IDs.
     *
     * Two queries are structurally similar when they share the same QueryPattern
     * and QueryConstraints, regardless of the specific start/target vertices.
     * Their optimization plans are identical (same algorithm, same cost estimates)
     * because plan selection is driven only by pattern type, depth, edge-type
     * selectivity, parallelism flags, and graph-level statistics.
     *
     * @param pattern     Query pattern type
     * @param constraints Query constraints (depth, edge type, flags, etc.)
     * @param depth_hint  Optional explicit depth override (used for K_HOP and
     *                    PATTERN_MATCH where depth comes from the call site, not
     *                    from constraints.max_depth).
     * @return Structural cache key string (prefixed with "struct:")
     */
    std::string generateStructuralCacheKey(
        QueryPattern pattern,
        const QueryConstraints& constraints,
        std::optional<size_t> depth_hint = std::nullopt
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

