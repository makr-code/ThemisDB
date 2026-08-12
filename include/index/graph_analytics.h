/**
 * @file graph_analytics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// GraphAnalytics
/// Provides graph analysis algorithms for centrality measures and importance ranking.
/// 
/// Algorithms:
/// - Degree Centrality: Simple in/out-degree counting
/// - PageRank: Iterative power method for node importance
/// - Betweenness Centrality: Shortest-path-based centrality (Brandes algorithm)
///
/// All algorithms operate on the graph topology managed by GraphIndexManager.
class GraphAnalytics {
public:
    explicit GraphAnalytics(GraphIndexManager& graphMgr);

    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    /// Degree Centrality
    /// Returns in-degree, out-degree, and total degree for all nodes in the graph.
    struct DegreeResult {
        int in_degree = 0;
        int out_degree = 0;
        int total_degree = 0;
    };
    
    std::pair<Status, std::map<std::string, DegreeResult>> degreeCentrality(
        const std::vector<std::string>& node_pks
    ) const;

    /// PageRank
    /// Computes importance scores using the iterative PageRank algorithm.
    /// 
    /// @param node_pks: List of all node primary keys to analyze
    /// @param damping: Damping factor (probability of random jump), typically 0.85
    /// @param max_iterations: Maximum number of iterations
    /// @param tolerance: Convergence tolerance (delta sum between iterations)
    /// @return Map of node_pk -> PageRank score (sum of all scores ≈ 1.0)
    std::pair<Status, std::map<std::string, double>> pageRank(
        const std::vector<std::string>& node_pks,
        double damping = 0.85,
        int max_iterations = 100,
        double tolerance = 1e-6
    ) const;

    /// Betweenness Centrality
    /// Measures how often a node lies on shortest paths between other nodes.
    /// Uses Brandes algorithm for efficiency O(V*E) for unweighted graphs.
    /// 
    /// @param node_pks: List of all node primary keys to analyze
    /// @return Map of node_pk -> betweenness score
    std::pair<Status, std::map<std::string, double>> betweennessCentrality(
        const std::vector<std::string>& node_pks
    ) const;

    /// Closeness Centrality
    /// Measures how close a node is to all other nodes (inverse of average distance).
    /// Higher values indicate more central positions in the graph.
    /// 
    /// @param node_pks: List of all node primary keys to analyze
    /// @return Map of node_pk -> closeness score (0 for isolated nodes)
    std::pair<Status, std::map<std::string, double>> closenessCentrality(
        const std::vector<std::string>& node_pks
    ) const;

    /// Community Detection - Louvain Algorithm
    /// Detects communities by optimizing modularity using the Louvain method.
    /// Multi-level greedy optimization: local moves + aggregation.
    /// 
    /// @param node_pks: List of all node primary keys to analyze
    /// @param min_modularity_gain: Minimum modularity gain to continue optimization (default: 0.000001)
    /// @return Map of node_pk -> community_id (int)
    std::pair<Status, std::map<std::string, int>> louvainCommunities(
        const std::vector<std::string>& node_pks,
        double min_modularity_gain = 0.000001
    ) const;

    /// Community Detection - Label Propagation
    /// Fast community detection by iteratively propagating labels.
    /// Each node adopts the most frequent label among its neighbors.
    /// 
    /// @param node_pks: List of all node primary keys to analyze
    /// @param max_iterations: Maximum number of propagation iterations (default: 100)
    /// @return Map of node_pk -> community_id (int)
    std::pair<Status, std::map<std::string, int>> labelPropagationCommunities(
        const std::vector<std::string>& node_pks,
        int max_iterations = 100
    ) const;

    /// K-Shortest Paths - Yen's Algorithm
    /// Finds K shortest loopless paths from source to target.
    /// Uses Dijkstra's algorithm iteratively with edge removal.
    /// 
    /// @param source: Source node primary key
    /// @param target: Target node primary key
    /// @param k: Number of shortest paths to find
    /// @param weight_attr: Optional edge weight attribute (empty = unweighted)
    /// @return Vector of paths, each path containing vertices, edges, and total length/weight
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
