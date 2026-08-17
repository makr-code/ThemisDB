/**
 * @file gpu_traversal.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/graph_index.h"
#include "utils/expected.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace themis {
namespace graph {

/**
 * @brief GPU-accelerated BFS/DFS for massive graphs.
 *
 * Implements BFS and DFS over a CSR (Compressed Sparse Row) representation of
 * the graph loaded from a GraphIndexManager.  When CUDA/GPU hardware is
 * available this class dispatches to GPU kernels; without GPU hardware it falls
 * back to an efficient CPU implementation that mirrors the GPU-parallel
 * level-synchronous algorithm.
 *
 * The graph is represented with integer node IDs for cache-efficient traversal.
 * String vertex IDs are mapped to integer indices on load and translated back
 * in the result.
 *
 * Usage:
 * @code
 *   GPUGraphTraversal gpu_trav(graph_manager);
 *   GPUGraphTraversal::Config cfg;
 *   cfg.gpu_device = 0;
 *   cfg.min_vertices_for_gpu = 10000;
 *   ASSERT_TRUE(gpu_trav.load());
 *   auto result = gpu_trav.bfs("start_vertex", 5, cfg);
 *   if (result) {
 *       for (const auto& [vertex, dist] : result->distances) { ... }
 *   }
 * @endcode
 */
class GPUGraphTraversal {
public:
    // -----------------------------------------------------------------------
    // Config
    // -----------------------------------------------------------------------

    struct Config {
        /// GPU device index (0-based).  Ignored when GPU is unavailable.
        int gpu_device = 0;
        /// Minimum number of vertices in the loaded graph before GPU is
        /// preferred over the CPU path.  For smaller graphs the CPU fallback
        /// is used even if a GPU is present.
        size_t min_vertices_for_gpu = 10'000;
        /// Maximum traversal depth (0 = source only).
        int max_depth = 10;
        /// Stop collecting results after this many vertices (0 = no limit).
        size_t max_results = 0;
        /// Vertices that must not be visited.
        std::vector<std::string> forbidden_vertices;

        Config() = default;
    };

    // -----------------------------------------------------------------------
    // Result
    // -----------------------------------------------------------------------

    struct TraversalResult {
        /// Ordered list of vertices visited during the traversal.
        std::vector<std::string> visited_vertices;
        /// BFS distance (or DFS discovery order) from the start vertex for
        /// each visited vertex.  Value -1 means unreachable.
        std::unordered_map<std::string, int> distances;
        /// Total nodes explored (may exceed visited_vertices.size() when
        /// early-termination is applied).
        size_t nodes_explored = 0;
        /// Total edges traversed.
        size_t edges_traversed = 0;
        /// True when the traversal ran on the CPU fallback path.
        bool used_cpu_fallback = true;
        /// True if `max_results` caused early termination.
        bool truncated = false;
        /// Wall-clock execution time in milliseconds.
        double execution_time_ms = 0.0;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @param graph_manager  Must outlive this GPUGraphTraversal instance.
     *                       Used to enumerate vertices and their adjacency
     *                       lists during `load()`.
     */
    explicit GPUGraphTraversal(GraphIndexManager& graph_manager);
    ~GPUGraphTraversal() = default;

    // Non-copyable
    GPUGraphTraversal(const GPUGraphTraversal&) = delete;
    GPUGraphTraversal& operator=(const GPUGraphTraversal&) = delete;
    GPUGraphTraversal(GPUGraphTraversal&&) noexcept = default;
    GPUGraphTraversal& operator=(GPUGraphTraversal&&) noexcept = default;

    // -----------------------------------------------------------------------
    // Graph loading
    // -----------------------------------------------------------------------

    /**
     * @brief (Re-)load the graph from the GraphIndexManager into CSR format.
     *
     * Enumerates all vertices and their out-adjacency lists via the graph
     * manager and builds an integer-indexed CSR representation.  Must be
     * called at least once before any traversal method.
     *
     * @param vertex_ids  Explicit set of vertex IDs to include.  When empty
     *                    the method includes every vertex that appears in any
     *                    edge (source or target).
     * @return Ok(true) on success, or Err with error details on failure.
     */
    Result<bool> load(const std::vector<std::string>& vertex_ids = {});

    /// Number of vertices in the loaded CSR graph.
    size_t vertexCount() const noexcept { return vertex_count_; }
    /// Number of edges in the loaded CSR graph.
    size_t edgeCount() const noexcept { return edge_count_; }

    // -----------------------------------------------------------------------
    // Traversal
    // -----------------------------------------------------------------------

    /**
     * @brief GPU-accelerated BFS from a single source vertex.
     *
     * Performs level-synchronous BFS.  Each BFS level is expanded atomically,
     * enabling GPU-parallel frontier processing.  On hardware without GPU
     * support the same algorithm runs on the CPU.
     *
     * @param start_vertex  Source vertex string ID.
     * @return TraversalResult or an error (e.g. unknown vertex).
     */
    Result<TraversalResult> bfs(
      const std::string& start_vertex
    );

    /**
     * @brief GPU-accelerated BFS with configuration.
     *
     * Performs level-synchronous BFS with custom configuration.
     *
     * @param start_vertex  Source vertex string ID.
     * @param config        Traversal configuration.
     * @return TraversalResult or an error (e.g. unknown vertex).
     */
    Result<TraversalResult> bfs(
      const std::string& start_vertex,
      const Config& config
    );

    /**
     * @brief GPU-accelerated DFS from a single source vertex.
     *
     * Iterative DFS with an explicit stack.
     *
     * @param start_vertex  Source vertex string ID.
     * @return TraversalResult or an error (e.g. unknown vertex).
     */
    Result<TraversalResult> dfs(
      const std::string& start_vertex
    );

    /**
     * @brief GPU-accelerated DFS with configuration.
     *
     * Iterative DFS with an explicit stack.  Depth is tracked per stack frame
     * and capped at `config.max_depth`.
     *
     * @param start_vertex  Source vertex string ID.
     * @param config        Traversal configuration.
     * @return TraversalResult or an error (e.g. unknown vertex).
     */
    Result<TraversalResult> dfs(
      const std::string& start_vertex,
      const Config& config
    );

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    struct Stats {
        size_t vertex_count = 0;
        size_t edge_count   = 0;
        bool   gpu_available = false;
        int    gpu_device_used = -1;
    };
    Stats getStats() const noexcept;

private:
    GraphIndexManager& graph_manager_;

    // CSR storage
    // row_offsets_[i] .. row_offsets_[i+1] is the range of column_indices_
    // that holds the out-neighbours of vertex i.
    std::vector<uint32_t> row_offsets_;
    std::vector<uint32_t> column_indices_;

    // Bidirectional ID mapping
    std::unordered_map<std::string, uint32_t> vertex_to_id_;
    std::vector<std::string>                  id_to_vertex_;

    size_t vertex_count_ = 0;
    size_t edge_count_   = 0;

    // Whether a real GPU device is available (detected at first load).
    bool gpu_available_ = false;
    int  gpu_device_    = -1;

    // Internal helpers
    std::optional<uint32_t> findVertexId(const std::string& v) const;

    TraversalResult runBFS(uint32_t start_id, const Config& config);
    TraversalResult runDFS(uint32_t start_id, const Config& config);
};

} // namespace graph
} // namespace themis
