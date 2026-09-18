/**
 * @file parallel_traversal.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class ParallelTraversal {
public:
    struct Config {
        int max_depth = 10;
        size_t max_results = 0;
        uint32_t num_threads = 0;
        uint32_t timeout_ms = 0;
        std::vector<std::string> forbidden_vertices;

        uint32_t fan_out_threshold = 0;

        Config() = default;
    };

    struct MultiSourceResult {
        std::vector<std::string> visited_vertices;

        std::unordered_map<std::string, std::string> vertex_to_source;

        size_t total_nodes_explored = 0;

        size_t total_edges_traversed = 0;

        double execution_time_ms = 0.0;

        bool timed_out = false;
    };

    /**
     * @brief Parallel Traversal.
     * @param[in,out] graph_manager Input/output parameter.
     * @return Return value.
     */
    explicit ParallelTraversal(GraphIndexManager& graph_manager);

    /**
     * @brief Multi Source BFS.
     * @param[in] sources Input parameter.
     * @return Return value.
     */
    Result<MultiSourceResult> multiSourceBFS(
        const std::vector<std::string>& sources
    );

    /**
     * @brief Multi Source BFS.
     * @param[in] sources Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    Result<MultiSourceResult> multiSourceBFS(
        const std::vector<std::string>& sources,
        const Config& config
    );

    /**
     * @brief Multi Source DFS.
     * @param[in] sources Input parameter.
     * @return Return value.
     */
    Result<MultiSourceResult> multiSourceDFS(
        const std::vector<std::string>& sources
    );

    /**
     * @brief Multi Source DFS.
     * @param[in] sources Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    Result<MultiSourceResult> multiSourceDFS(
        const std::vector<std::string>& sources,
        const Config& config
    );

private:
    GraphIndexManager& graph_manager_;

    /**
     * @brief Effective Thread Count.
     * @param[in] config Input parameter.
     * @param[in] num_sources Input parameter.
     * @return Return value.
     */
    static size_t effectiveThreadCount(const Config& config, size_t num_sources);

    struct SourceTraversalResult {
        std::string source = {};
        std::vector<std::string> visited;   // ordered by discovery
        size_t nodes_explored = 0;
        size_t edges_traversed = 0;
        bool timed_out = false;
    };

    /**
     * @brief Run Single BFS.
     * @param[in] source Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    SourceTraversalResult runSingleBFS(
        const std::string& source,
        const Config& config
    );

    /**
     * @brief Run Single DFS.
     * @param[in] source Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    SourceTraversalResult runSingleDFS(
        const std::string& source,
        const Config& config
    );

    /**
     * @brief Merge Results.
     * @param[in] per_source Input parameter.
     * @param[in] execution_time_ms Input parameter.
     * @return Return value.
     */
    static MultiSourceResult mergeResults(
        std::vector<SourceTraversalResult>&& per_source,
        double execution_time_ms
    );
};

} // namespace graph
} // namespace themis
