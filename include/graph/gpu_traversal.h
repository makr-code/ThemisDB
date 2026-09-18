/**
 * @file gpu_traversal.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
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

class GPUGraphTraversal {
public:
    // -----------------------------------------------------------------------
    // Config
    // -----------------------------------------------------------------------

    struct Config {
        int gpu_device = 0;
        size_t min_vertices_for_gpu = 10'000;
        int max_depth = 10;
        size_t max_results = 0;
        std::vector<std::string> forbidden_vertices;

        Config() = default;
    };

    // -----------------------------------------------------------------------
    // Result
    // -----------------------------------------------------------------------

    struct TraversalResult {
        std::vector<std::string> visited_vertices;
        std::unordered_map<std::string, int> distances;
        size_t nodes_explored = 0;
        size_t edges_traversed = 0;
        bool used_cpu_fallback = true;
        bool truncated = false;
        double execution_time_ms = 0.0;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief GPUGraph Traversal.
     * @param[in,out] graph_manager Input/output parameter.
     * @return Return value.
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

    Result<bool> load(const std::vector<std::string>& vertex_ids = {});

    size_t vertexCount() const noexcept { return vertex_count_; }
    size_t edgeCount() const noexcept { return edge_count_; }

    // -----------------------------------------------------------------------
    // Traversal
    // -----------------------------------------------------------------------

    /**
     * @brief Bfs.
     * @param[in] start_vertex Input parameter.
     * @return Return value.
     */
    Result<TraversalResult> bfs(
      const std::string& start_vertex
    );

    /**
     * @brief Bfs.
     * @param[in] start_vertex Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    Result<TraversalResult> bfs(
      const std::string& start_vertex,
      const Config& config
    );

    /**
     * @brief Dfs.
     * @param[in] start_vertex Input parameter.
     * @return Return value.
     */
    Result<TraversalResult> dfs(
      const std::string& start_vertex
    );

    /**
     * @brief Dfs.
     * @param[in] start_vertex Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
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
    /**
     * @brief Get Stats.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
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
    /**
     * @brief Find Vertex Id.
     * @param[in] v Input parameter.
     * @return Return value.
     */
    std::optional<uint32_t> findVertexId(const std::string& v) const;

    /**
     * @brief Run BFS.
     * @param[in] start_id Identifier of the start.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    TraversalResult runBFS(uint32_t start_id, const Config& config);
    /**
     * @brief Run DFS.
     * @param[in] start_id Identifier of the start.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    TraversalResult runDFS(uint32_t start_id, const Config& config);
};

} // namespace graph
} // namespace themis
