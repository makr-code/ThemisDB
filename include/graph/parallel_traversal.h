/**
 * @file parallel_traversal.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/graph_index.h"
#include "utils/expected.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <atomic>

namespace themis {
namespace graph {

/**
 * @brief Parallel multi-source BFS/DFS for large graphs.
 *
 * Implements concurrent traversal from multiple source vertices, running one
 * independent search per source vertex.  Each search executes in its own
 * std::async task so that large fan-out workloads scale across available CPU
 * cores.  The individual per-source results are merged after all tasks
 * complete: if a vertex is reachable from more than one source, the result
 * records the source that discovered it first (non-deterministic when two
 * sources reach the vertex in the same wall-clock instant).
 *
 * Thread safety: all mutable state lives inside per-task closures; the
 * GraphIndexManager is accessed read-only (outNeighbors / outAdjacency), which
 * is safe to call concurrently per its documented contract.
 *
 * Usage:
 * @code
 *   ParallelTraversal pt(graph_manager);
 *   ParallelTraversal::Config cfg;
 *   cfg.max_depth = 5;
 *   cfg.num_threads = 4;
 *   auto result = pt.multiSourceBFS({"A", "B", "C"}, cfg);
 *   if (result) {
 *       for (const auto& v : result->visited_vertices) { ... }
 *   }
 * @endcode
 */
class ParallelTraversal {
public:
    /**
     * @brief Configuration for a multi-source traversal.
     */
    struct Config {
        /// Maximum depth each individual source traversal may reach.
        int max_depth = 10;
        /// Stop collecting results once this many vertices have been found
        /// across all sources (0 = no limit).
        size_t max_results = 0;
        /// Maximum number of concurrent source traversals (0 = auto, clamped
        /// to [2, hardware_concurrency]).
        uint32_t num_threads = 0;
        /// Per-source traversal timeout in milliseconds (0 = no limit).
        /// If a single source traversal exceeds this budget it is aborted and
        /// its partial results are included in the merged output.
        uint32_t timeout_ms = 0;
        /// Vertices that must never be visited by any source traversal.
        std::vector<std::string> forbidden_vertices;

        /// Minimum frontier size that triggers parallel fan-out expansion
        /// within a single source's BFS.  When the BFS frontier at any level
        /// reaches this threshold, neighbor lookups are dispatched to multiple
        /// threads (using at most `num_threads` workers, or auto if 0).
        /// 0 = never parallelize fan-out (sources still run in parallel
        /// with each other as usual).
        uint32_t fan_out_threshold = 0;

        Config() = default;
    };

    /**
     * @brief Aggregated result returned by multiSourceBFS / multiSourceDFS.
     */
    struct MultiSourceResult {
        /// Ordered list of all distinct vertices visited across all sources.
        /// Vertices reachable from multiple sources appear only once (the
        /// source that first claimed them wins).
        std::vector<std::string> visited_vertices;

        /// Maps each visited vertex to the source that first reached it.
        std::unordered_map<std::string, std::string> vertex_to_source;

        /// Total nodes explored (summed across all per-source traversals,
        /// including duplicates before de-duplication).
        size_t total_nodes_explored = 0;

        /// Total edges traversed (summed across all per-source traversals).
        size_t total_edges_traversed = 0;

        /// Wall-clock time from start to end of multiSourceBFS/DFS (ms).
        double execution_time_ms = 0.0;

        /// True if at least one per-source traversal was aborted due to
        /// timeout_ms being exceeded.
        bool timed_out = false;
    };

    /**
     * @brief Construct with a reference to the graph storage backend.
     * @param graph_manager Must outlive this ParallelTraversal instance.
     */
    explicit ParallelTraversal(GraphIndexManager& graph_manager);

    /**
     * @brief Run BFS from each source vertex in parallel and merge results.
     *
     * Each source vertex gets its own BFS executed in a separate thread.  The
     * per-source BFS uses level-by-level frontier expansion.  Results are
     * merged once all threads have completed.
     *
     * @param sources  Non-empty list of source vertex IDs.
     * @return Merged MultiSourceResult, or an error if sources is empty.
     */
    Result<MultiSourceResult> multiSourceBFS(
        const std::vector<std::string>& sources
    );

    /**
     * @brief Run BFS from each source vertex in parallel with custom configuration.
     *
     * Each source vertex gets its own BFS executed in a separate thread.  The
     * per-source BFS uses level-by-level frontier expansion.  Results are
     * merged once all threads have completed.
     *
     * @param sources  Non-empty list of source vertex IDs.
     * @param config   Traversal configuration.
     * @return Merged MultiSourceResult, or an error if sources is empty.
     */
    Result<MultiSourceResult> multiSourceBFS(
        const std::vector<std::string>& sources,
        const Config& config
    );

    /**
     * @brief Run DFS from each source vertex in parallel and merge results.
     *
     * Each source vertex gets its own iterative DFS executed in a separate
     * thread.  Results are merged once all threads have completed.
     *
     * @param sources  Non-empty list of source vertex IDs.
     * @return Merged MultiSourceResult, or an error if sources is empty.
     */
    Result<MultiSourceResult> multiSourceDFS(
        const std::vector<std::string>& sources
    );

    /**
     * @brief Run DFS from each source vertex in parallel with custom configuration.
     *
     * Each source vertex gets its own iterative DFS executed in a separate
     * thread.  Results are merged once all threads have completed.
     *
     * @param sources  Non-empty list of source vertex IDs.
     * @param config   Traversal configuration.
     * @return Merged MultiSourceResult, or an error if sources is empty.
     */
    Result<MultiSourceResult> multiSourceDFS(
        const std::vector<std::string>& sources,
        const Config& config
    );

private:
    GraphIndexManager& graph_manager_;

    /// Compute effective thread count from config and available hardware.
    static size_t effectiveThreadCount(const Config& config, size_t num_sources);

    /// Per-source BFS result (before merging).
    struct SourceTraversalResult {
        std::string source;
        std::vector<std::string> visited;   // ordered by discovery
        size_t nodes_explored = 0;
        size_t edges_traversed = 0;
        bool timed_out = false;
    };

    /// Run a single-source BFS; called from within an async task.
    SourceTraversalResult runSingleBFS(
        const std::string& source,
        const Config& config
    );

    /// Run a single-source DFS; called from within an async task.
    SourceTraversalResult runSingleDFS(
        const std::string& source,
        const Config& config
    );

    /// Merge per-source results into a single MultiSourceResult.
    static MultiSourceResult mergeResults(
        std::vector<SourceTraversalResult>&& per_source,
        double execution_time_ms
    );
};

} // namespace graph
} // namespace themis
