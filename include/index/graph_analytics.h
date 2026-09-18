/**
 * @file graph_analytics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/graph_index.h"
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <unordered_map>

namespace themis {

class GraphAnalytics {
public:
    /**
     * @brief Graph Analytics.
     * @param[in,out] graphMgr Input/output parameter.
     * @return Return value.
     */
    explicit GraphAnalytics(GraphIndexManager& graphMgr);

    struct Status {
        bool ok = true;
        std::string message;
        /**
         * @brief OK.
         * @return Return value.
         * @details Implements OK without additional internal calls.
         */
        static Status OK() { return {}; }
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    struct DegreeResult {
        int in_degree = 0;
        int out_degree = 0;
        int total_degree = 0;
    };
    
    std::pair<Status, std::map<std::string, DegreeResult>> degreeCentrality(
        const std::vector<std::string>& node_pks
    ) const;

    std::pair<Status, std::map<std::string, double>> pageRank(
        const std::vector<std::string>& node_pks,
        double damping = 0.85,
        int max_iterations = 100,
        double tolerance = 1e-6
    ) const;

    std::pair<Status, std::map<std::string, double>> betweennessCentrality(
        const std::vector<std::string>& node_pks
    ) const;

    std::pair<Status, std::map<std::string, double>> closenessCentrality(
        const std::vector<std::string>& node_pks
    ) const;

    std::pair<Status, std::map<std::string, int>> louvainCommunities(
        const std::vector<std::string>& node_pks,
        double min_modularity_gain = 0.000001
    ) const;

    std::pair<Status, std::map<std::string, int>> labelPropagationCommunities(
        const std::vector<std::string>& node_pks,
        int max_iterations = 100
    ) const;

    struct PathInfo {
        std::vector<std::string> vertices;
        std::vector<std::pair<std::string, std::string>> edges;  // pairs of (from, to)
        double length;  // number of edges or total weight
        int hop_count;  // number of edges
    };
    
    std::pair<Status, std::vector<PathInfo>> kShortestPaths(
        const std::string& source,
        const std::string& target,
        int k,
        const std::string& weight_attr = ""
    ) const;

private:
    GraphIndexManager& graphMgr_;

    // Helper: Build adjacency structure for algorithms
    struct GraphTopology {
        std::unordered_map<std::string, std::vector<std::string>> outgoing;
        std::unordered_map<std::string, std::vector<std::string>> incoming;
    };
    
    std::pair<Status, GraphTopology> buildTopology(
        const std::vector<std::string>& node_pks
    ) const;
};

} // namespace themis
