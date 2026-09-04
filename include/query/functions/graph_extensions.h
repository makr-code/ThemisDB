/**
 * @file graph_extensions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "function_registry.h"
#include "graph_functions.h"
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <algorithm>
#include <cmath>

namespace themis {
namespace query {
namespace functions {

namespace {
inline int clampIterationsFromJson(const nlohmann::json& value, int fallback = 100) {
    if (!value.is_number()) {
        return fallback;
    }
    const double raw = value.get<double>();
    if (!std::isfinite(raw)) {
        return fallback;
    }
    constexpr double kMinIterations = 1.0;
    constexpr double kMaxIterations = 1'000'000.0;
    return static_cast<int>(std::clamp(raw, kMinIterations, kMaxIterations));
}
} // namespace

// ============================================================================
// ALL_SHORTEST_PATHS - Find all shortest paths between two vertices
// ============================================================================
// NOTE: Implementation pending resolution of API type dependencies (graph storage, traversal context).
/*
class AllShortestPathsFunction : public IFunction {
public:
    ~AllShortestPathsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "ALL_SHORTEST_PATHS",
            "Graph",
            "Returns all shortest paths between two vertices",
            {
                {"startVertex", ArgType::STRING, true, nullptr, "Starting vertex ID"},
                {"endVertex", ArgType::STRING, true, nullptr, "Ending vertex ID"},
                {"options", ArgType::OBJECT, false, nlohmann::json::object(), "Optional parameters"}
            },
            ArgType::ARRAY,
            true, false,
            {R"(ALL_SHORTEST_PATHS("A", "B"))"},
            FunctionCost{CostComplexity::QUADRATIC, 100.0, 1.0, true, true, "graph"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args, 
                          const FunctionContext& ctx) const override {
        if (args.size() < 2) {
          return nlohmann::json::array();
        }
        
        std::string startVertex = toString(args[0]);
        std::string endVertex = toString(args[1]);
        
        // Options parsing
        std::string edgeCollection = "_edges";
        std::string direction = "OUTBOUND";
        int maxDepth = 10;
        
        if (args.size() > 2 && args[2].is_object()) {
            auto opts = args[2];
            if (opts.contains("edgeCollection")) {
              edgeCollection = opts["edgeCollection"].get<std::string>();
            }
            if (opts.contains("direction")) {
              direction = opts["direction"].get<std::string>();
            }
            if (opts.contains("maxDepth")) {
              maxDepth = static_cast<int>(toNumber(opts["maxDepth"]));
            }
        }
        
        // BFS to find all shortest paths
        // Implementation would use graph storage
        return nlohmann::json::array();
    }
};
*/

// ============================================================================
// K_SHORTEST_PATHS - Find K shortest paths (Yen's algorithm)
// ============================================================================
// NOTE: Implementation pending resolution of API type dependencies (graph storage, traversal context).
/*
class KShortestPathsFunction : public IFunction {
public:
    ~KShortestPathsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "K_SHORTEST_PATHS",
            "Graph",
            "Returns the K shortest paths between two vertices using Yen's algorithm",
            {
                {"startVertex", ArgType::STRING, true, nullptr, "Starting vertex ID"},
                {"endVertex", ArgType::STRING, true, nullptr, "Ending vertex ID"},
                {"k", ArgType::INTEGER, true, nullptr, "Number of shortest paths to find"},
                {"options", ArgType::OBJECT, false, nlohmann::json::object(), "Optional parameters (weightAttribute, etc.)"}
            },
            ArgType::ARRAY,
            true,  // deterministic
            false, // not aggregate
            {
                R"(K_SHORTEST_PATHS("A", "E", 3) // Find 3 shortest paths)",
                R"(K_SHORTEST_PATHS("A", "E", 5, {weightAttribute: "distance"}))"
            },
            FunctionCost{CostComplexity::QUADRATIC, 200.0, 10.0, true, false, "graph"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args, 
                          const FunctionContext& ctx) const override {
        if (args.size() < 3) {
            return nlohmann::json::array();
        }
        
        std::string startVertex = toString(args[0]);
        std::string endVertex = toString(args[1]);
        int k = static_cast<int>(toNumber(args[2]));
        
        if (k <= 0) {
            return nlohmann::json::array();
        }
        
        // Parse options
        // Empty string means unweighted (edge count), or use default "_weight" from graph
        std::string weightAttribute = "";
        if (args.size() > 3 && args[3].is_object()) {
            auto opts = args[3];
            if (opts.contains("weightAttribute") && opts["weightAttribute"].is_string()) {
                weightAttribute = opts["weightAttribute"].get<std::string>();
            }
        }
        
        // Get GraphAnalytics instance from context
        auto* analytics = ctx.getGraphAnalytics();
        if (!analytics) {
            // If no analytics available, return empty (graceful degradation)
            // This can happen if the function is called without proper query context
            return nlohmann::json::array();
        }
        
        // Call Yen's algorithm implementation
        auto [status, paths] = analytics->kShortestPaths(startVertex, endVertex, k, weightAttribute);
        
        if (!status.ok) {
            // Log error but return empty array (functions should not throw in query execution)
            // Error details are in status.message
            // NOTE: Consider returning error information in result structure for richer diagnostics.
            return nlohmann::json::array();
        }
        
        // Convert PathInfo results to JSON
        nlohmann::json result = nlohmann::json::array();
        for (size_t i = 0; i < paths.size(); ++i) {
            const auto& path = paths[i];
            
            nlohmann::json pathObj = nlohmann::json::object();
            pathObj["rank"] = i + 1;
            
            // Vertices array
            nlohmann::json vertices = nlohmann::json::array();
            for (const auto& v : path.vertices) {
                vertices.push_back(v);
            }
            pathObj["vertices"] = vertices;
            
            // Edges array (from/to pairs)
            nlohmann::json edges = nlohmann::json::array();
            for (const auto& edge : path.edges) {
                nlohmann::json edgeObj = nlohmann::json::object();
                edgeObj["from"] = edge.first;
                edgeObj["to"] = edge.second;
                edges.push_back(edgeObj);
            }
            pathObj["edges"] = edges;
            
            pathObj["length"] = path.hop_count;
            pathObj["distance"] = path.length;
            
            result.push_back(pathObj);
        }
        
        return result;
    }
};
*/

// ============================================================================
// WEIGHTED_SHORTEST_PATH - Dijkstra's algorithm with edge weights
// ============================================================================
// NOTE: Implementation pending resolution of API type dependencies (graph storage, traversal context).
/*
class WeightedShortestPathFunction : public IFunction {
public:
    ~WeightedShortestPathFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "WEIGHTED_SHORTEST_PATH",
            "Graph",
            "Finds the shortest weighted path using Dijkstra's algorithm",
            {
                {"startVertex", ArgType::STRING, true, nullptr, "Starting vertex ID"},
                {"endVertex", ArgType::STRING, true, nullptr, "Ending vertex ID"},
                {"weightAttribute", ArgType::STRING, true, nullptr, "Edge weight attribute name"}
            },
            ArgType::OBJECT,
            true, false,
            {R"(WEIGHTED_SHORTEST_PATH("A", "B", "distance"))"},
            FunctionCost{CostComplexity::LINEARITHMIC, 50.0, 0.5, true, false, "graph"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args, 
                          const FunctionContext& ctx) const override {
        if (args.size() < 3) {
            return nlohmann::json{
                {"vertices", nlohmann::json::array()},
                {"edges", nlohmann::json::array()},
                {"weight", 0.0}
            };
        }
        
        std::string startVertex = toString(args[0]);
        std::string endVertex = toString(args[1]);
        std::string weightAttr = toString(args[2]);
        
        // Dijkstra's algorithm implementation
        return nlohmann::json{
            {"vertices", nlohmann::json::array()},
            {"edges", nlohmann::json::array()},
            {"weight", std::numeric_limits<double>::infinity()}
        };
    }
};
*/

// ============================================================================
// PATH_LENGTH - Get the length (number of edges) in a path
// ============================================================================
// NOTE: Implementation pending resolution of API type dependencies (graph storage, traversal context).
/*
class PathLengthFunction : public IFunction {
public:
    ~PathLengthFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_LENGTH",
            "Graph",
            "Returns the number of edges in a path",
            {{"path", ArgType::OBJECT, true, nullptr, "Path object"}},
            ArgType::NUMBER,
            true, false,
            {R"(PATH_LENGTH(path))"},
            FunctionCost{CostComplexity::CONSTANT, 1.0, 0.0, false, true, ""}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args, 
                          const FunctionContext& ctx) const override {
        if (args.empty() || !args[0].is_object()) {
          return 0;
        }
        
        auto path = args[0];
        if (path.contains("edges") && path["edges"].is_array()) {
            return static_cast<double>(path["edges"].size());
        }
        if (path.contains("vertices") && path["vertices"].is_array()) {
            size_t vcount = path["vertices"].size();
            return static_cast<double>(vcount > 0 ? vcount - 1 : 0);
        }
        
        return 0;
    }
};
*/

// ============================================================================
// PATH_VERTICES - Extract vertices from a path
// ============================================================================
// NOTE: Implementation pending resolution of API type dependencies (graph storage, traversal context).
/*
class PathVerticesFunction : public IFunction {
public:
    ~PathVerticesFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_VERTICES",
            "Graph",
            "Extracts the vertices from a path object",
            {{"path", ArgType::OBJECT, true, nullptr, "Path object"}},
            ArgType::ARRAY,
            true, false,
            {R"(PATH_VERTICES(path))"},
            FunctionCost{CostComplexity::CONSTANT, 1.0, 0.0, false, true, ""}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args, 
                          const FunctionContext& ctx) const override {
        if (args.empty() || !args[0].is_object()) {
          return nlohmann::json::array();
        }
        
        auto path = args[0];
        if (path.contains("vertices") && path["vertices"].is_array()) {
            return path["vertices"];
        }
        
        return nlohmann::json::array();
    }
};
*/

// ============================================================================
// PATH_EDGES - Extract edges from a path
// ============================================================================
// NOTE: Implementation pending resolution of API type dependencies (graph storage, traversal context).
/*
class PathEdgesFunction : public IFunction {
public:
    ~PathEdgesFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_EDGES",
            "Graph",
            "Extracts the edges from a path object",
            {{"path", ArgType::OBJECT, true, nullptr, "Path object"}},
            ArgType::ARRAY,
            true, false,
            {R"(PATH_EDGES(path))"},
            FunctionCost{CostComplexity::CONSTANT, 1.0, 0.0, false, true, ""}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args, 
                          const FunctionContext& ctx) const override {
        if (args.empty() || !args[0].is_object()) {
          return nlohmann::json::array();
        }
        
        auto path = args[0];
        if (path.contains("edges") && path["edges"].is_array()) {
            return path["edges"];
        }
        
        return nlohmann::json::array();
    }
};
*/

// ============================================================================
// PATH_WEIGHT - Calculate total weight of a path
// ============================================================================
// NOTE: Implementation pending resolution of API type dependencies (graph storage, traversal context).
/*
class PathWeightFunction : public IFunction {
public:
    ~PathWeightFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "PATH_WEIGHT",
            "Graph",
            "Calculates the total weight of edges in a path",
            {
                {"path", ArgType::OBJECT, true, nullptr, "Path object"},
                {"weightAttribute", ArgType::STRING, true, nullptr, "Weight attribute name"}
            },
            ArgType::NUMBER,
            true, false,
            {R"(PATH_WEIGHT(path, "distance"))"},
            FunctionCost{CostComplexity::LINEAR, 1.0, 0.01, false, true, ""}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args, 
                          const FunctionContext& ctx) const override {
        if (args.size() < 2 || !args[0].is_object()) {
          return 0.0;
        }
        
        auto path = args[0];
        std::string weightAttr = toString(args[1]);
        
        if (!path.contains("edges") || !path["edges"].is_array()) {
          return 0.0;
        }
        
        double totalWeight = 0.0;
        for (const auto& edge : path["edges"]) {
            if (edge.is_object() && edge.contains(weightAttr)) {
                totalWeight += toNumber(edge[weightAttr]);
            }
        }
        
        return totalWeight;
    }
};
*/

// ============================================================================
// BETWEENNESS_CENTRALITY - Calculate betweenness centrality for vertices
// ============================================================================
// NOTE: Implementation pending resolution of API type dependencies (graph storage, traversal context).
/*
class BetweennessCentralityFunction : public IFunction {
public:
    ~BetweennessCentralityFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "BETWEENNESS_CENTRALITY",
            "Graph",
            "Calculates betweenness centrality for all vertices",
            {{"graphName", ArgType::STRING, true, nullptr, "Graph name or edge collection"}},
            ArgType::OBJECT,
            true, false,
            {R"(BETWEENNESS_CENTRALITY("myGraph"))"},
            FunctionCost{CostComplexity::QUADRATIC, 1000.0, 10.0, true, true, "graph"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args, 
                          const FunctionContext& ctx) const override {
        if (args.empty()) {
          return nlohmann::json::object();
        }
        
        // Brandes' algorithm implementation would go here
        return nlohmann::json::object();
    }
};
*/

// ============================================================================
// CLOSENESS_CENTRALITY - Calculate closeness centrality for vertices
// ============================================================================
// NOTE: Implementation pending resolution of API type dependencies (graph storage, traversal context).
/*
class ClosenessCentralityFunction : public IFunction {
public:
    ~ClosenessCentralityFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "CLOSENESS_CENTRALITY",
            "Graph",
            "Calculates closeness centrality for all vertices",
            {{"graphName", ArgType::STRING, true, nullptr, "Graph name or edge collection"}},
            ArgType::OBJECT,
            true, false,
            {R"(CLOSENESS_CENTRALITY("myGraph"))"},
            FunctionCost{CostComplexity::QUADRATIC, 800.0, 8.0, true, true, "graph"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args, 
                          const FunctionContext& ctx) const override {
        if (args.empty()) {
          return nlohmann::json::object();
        }
        
        // BFS-based closeness calculation
        return nlohmann::json::object();
    }
};
*/

// ============================================================================
// BETWEENNESS_CENTRALITY - Brandes' algorithm on edge arrays
// ============================================================================

/**
 * @brief BETWEENNESS_CENTRALITY(edges, options?) - Betweenness centrality via Brandes algorithm
 *
 * Computes betweenness centrality for all vertices in the graph defined by an
 * array of edge documents. Uses Brandes' O(V*E) algorithm for unweighted graphs.
 *
 * @param edges   Array of edge documents (each with _from and _to fields)
 * @param options Optional object: {normalize: bool} (default false)
 * @return Object mapping each vertex ID to its betweenness score
 */
class BetweennessCentralityExtFunction : public IFunction {
public:
    ~BetweennessCentralityExtFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "BETWEENNESS_CENTRALITY",
            "Graph",
            "Calculates betweenness centrality for all vertices using Brandes' algorithm",
            {
                {"edges", ArgType::ANY, true, nullptr, "Array of edge documents"},
                {"options", ArgType::OBJECT, false, nlohmann::json::object(),
                 "Optional: {normalize: bool} — divide scores by (n-1)*(n-2) (default: false)"}
            },
            ArgType::OBJECT,
            true,
            false,
            {
                R"(BETWEENNESS_CENTRALITY(edges))",
                R"(BETWEENNESS_CENTRALITY(edges, {normalize: true}))"
            },
            FunctionCost{CostComplexity::QUADRATIC, 1000.0, 10.0, false, false, ""}
        };
    }

    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || !args[0].is_array()) {
            return nlohmann::json::object();
        }

        auto graph = graph_helpers::buildGraph(args[0]);
        const auto& vertices = graph.vertices();
        if (vertices.empty()) {
          return nlohmann::json::object();
        }

        bool normalize = false;
        if (args.size() > 1 && args[1].is_object() && args[1].contains("normalize")) {
            if (args[1]["normalize"].is_boolean()) {
                normalize = args[1]["normalize"].get<bool>();
            }
        }

        std::vector<std::string> node_list(vertices.begin(), vertices.end());

        // Initialize betweenness scores
        std::unordered_map<std::string, double> betweenness;
        for (const auto& v : node_list) {
            betweenness[v] = 0.0;
        }

        // Brandes algorithm: O(V*E) for unweighted graphs
        for (const auto& source : node_list) {
            std::queue<std::string> q;
            std::unordered_map<std::string, std::vector<std::string>> predecessors;
            std::unordered_map<std::string, int> distance;
            std::unordered_map<std::string, int> sigma;
            std::unordered_map<std::string, double> delta;

            for (const auto& v : node_list) {
                distance[v] = -1;
                sigma[v] = 0;
                delta[v] = 0.0;
            }

            distance[source] = 0;
            sigma[source] = 1;
            q.push(source);

            std::vector<std::string> stack;

            // Forward BFS: discover shortest paths
            while (!q.empty()) {
                std::string v = q.front();
                q.pop();
                stack.push_back(v);

                for (const auto& [w, weight] : graph.outNeighbors(v)) {
                    // graph.vertices() contains all endpoints of all edges, so every
                    // neighbor w is guaranteed to be in node_list and pre-initialized
                    // in `distance` with -1 (never default-inserted as 0).
                    if (distance[w] < 0) {
                        distance[w] = distance[v] + 1;
                        q.push(w);
                    }
                    if (distance[w] == distance[v] + 1) {
                        sigma[w] += sigma[v];
                        predecessors[w].push_back(v);
                    }
                }
            }

            // Backward accumulation: propagate dependencies
            while (!stack.empty()) {
                std::string w = stack.back();
                stack.pop_back();

                auto pred_it = predecessors.find(w);
                if (pred_it != predecessors.end()) {
                    for (const auto& v : pred_it->second) {
                        if (sigma[w] > 0) {
                            delta[v] += (static_cast<double>(sigma[v]) / sigma[w]) *
                                        (1.0 + delta[w]);
                        }
                    }
                }

                if (w != source) {
                    betweenness[w] += delta[w];
                }
            }
        }

        // Optional normalization for directed graphs: divide by (n-1)*(n-2)
        const size_t n = node_list.size();
        if (normalize && n > 2) {
            const double scale =
                1.0 / (static_cast<double>(n - 1) * static_cast<double>(n - 2));
            for (auto& [v, bc] : betweenness) {
                bc *= scale;
            }
        }

        // Convert to JSON
        nlohmann::json result = nlohmann::json::object();
        for (const auto& [v, bc] : betweenness) {
            result[v] = bc;
        }
        return result;
    }
};

// ============================================================================
// LOUVAIN_COMMUNITIES (enhanced) - Rich community object format
// ============================================================================

/**
 * @brief LOUVAIN_COMMUNITIES(edges, options?) - Louvain community detection (rich output)
 *
 * Detects communities using the Louvain greedy modularity algorithm and returns
 * a detailed result object containing per-community metrics.
 *
 * @param edges   Array of edge documents (each with _from and _to fields)
 * @param options Optional object: {min_modularity_gain: number} (default: 0.000001)
 * @return Object: {communities: [{id, members, size, modularity, density}],
 *                  num_communities, overall_modularity}
 */
class LouvainCommunitiesExtFunction : public IFunction {
private:
    static constexpr int MAX_LOUVAIN_ITERATIONS = 100;

    static nlohmann::json emptyResult() {
        nlohmann::json r;
        r["communities"] = nlohmann::json::array();
        r["num_communities"] = 0;
        r["overall_modularity"] = 0.0;
        return r;
    }

public:
    ~LouvainCommunitiesExtFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "LOUVAIN_COMMUNITIES",
            "Graph",
            "Detect communities using Louvain algorithm with detailed community metrics",
            {
                {"edges", ArgType::ANY, true, nullptr, "Array of edge documents"},
                {"options", ArgType::OBJECT, false, nlohmann::json::object(),
                 "Optional: {min_modularity_gain: number} (default: 0.000001)"}
            },
            ArgType::OBJECT,
            true,
            false,
            {
                "LOUVAIN_COMMUNITIES(edges)",
                "LOUVAIN_COMMUNITIES(edges, {min_modularity_gain: 0.001})"
            },
            FunctionCost{CostComplexity::QUADRATIC, 500.0, 5.0, false, false, ""}
        };
    }

    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || !args[0].is_array()) {
          return emptyResult();
        }

        auto graph = graph_helpers::buildGraph(args[0]);
        const auto& vertices = graph.vertices();
        if (vertices.empty()) {
          return emptyResult();
        }

        // Parse options: accept either an object {min_modularity_gain: N} or a scalar N
        double min_modularity_gain = 0.000001;
        if (args.size() > 1) {
            if (args[1].is_object() && args[1].contains("min_modularity_gain")) {
                min_modularity_gain = args[1]["min_modularity_gain"].get<double>();
            } else if (args[1].is_number()) {
                min_modularity_gain = args[1].get<double>();
            }
        }

        std::vector<std::string> node_list(vertices.begin(), vertices.end());

        // Initialize: each node in its own community
        std::unordered_map<std::string, int> node_to_comm;
        int next_comm_id = 0;
        for (const auto& node : node_list) {
            node_to_comm[node] = next_comm_id++;
        }

        // Count total unique undirected edges
        std::set<std::pair<std::string, std::string>> unique_edges;
        for (const auto& node : node_list) {
            for (const auto& [neighbor, weight] : graph.outNeighbors(node)) {
                unique_edges.insert(std::minmax(node, neighbor));
            }
        }

        if (unique_edges.empty()) {
            // No edges: each node becomes its own community
            nlohmann::json communities_array = nlohmann::json::array();
            for (int idx = 0; idx < static_cast<int>(node_list.size()); ++idx) {
                nlohmann::json comm;
                comm["id"] = idx;
                comm["members"] = nlohmann::json::array({node_list[idx]});
                comm["size"] = 1;
                comm["modularity"] = 0.0;
                comm["density"] = 0.0;
                communities_array.push_back(comm);
            }
            nlohmann::json result;
            result["communities"] = communities_array;
            result["num_communities"] = static_cast<int>(node_list.size());
            result["overall_modularity"] = 0.0;
            return result;
        }

        const double m = static_cast<double>(unique_edges.size());

        // Also compute total directed edges for per-community modularity metrics
        double m_directed = 0.0;
        for (const auto& node : node_list) {
            m_directed += static_cast<double>(graph.outNeighbors(node).size());
        }
        if (m_directed == 0.0) {
          m_directed = 1.0;
        }

        // Louvain optimization: greedy modularity maximization
        bool improved = true;
        int iteration = 0;

        while (improved && iteration < MAX_LOUVAIN_ITERATIONS) {
            improved = false;
            iteration++;

            for (const auto& node : node_list) {
                const int current_comm = node_to_comm[node];
                std::unordered_map<int, double> neighbor_community_weights;

                for (const auto& [nb, weight] : graph.outNeighbors(node)) {
                    auto it = node_to_comm.find(nb);
                    if (it != node_to_comm.end()) {
                        neighbor_community_weights[it->second] += weight;
                    }
                }
                for (const auto& [nb, weight] : graph.inNeighbors(node)) {
                    auto it = node_to_comm.find(nb);
                    if (it != node_to_comm.end()) {
                        neighbor_community_weights[it->second] += weight;
                    }
                }

                if (neighbor_community_weights.empty()) {
                  continue;
                }

                int best_comm = current_comm;
                double best_delta_q = 0.0;

                for (const auto& [candidate_comm, edge_weight] : neighbor_community_weights) {
                    if (candidate_comm == current_comm) {
                      continue;
                    }
                    double delta_q = edge_weight / m;
                    if (delta_q > best_delta_q) {
                        best_delta_q = delta_q;
                        best_comm = candidate_comm;
                    }
                }

                if (best_delta_q > min_modularity_gain && best_comm != current_comm) {
                    node_to_comm[node] = best_comm;
                    improved = true;
                }
            }
        }

        // Group nodes by community ID
        std::unordered_map<int, std::vector<std::string>> communities_map;
        for (const auto& [node, comm] : node_to_comm) {
            communities_map[comm].push_back(node);
        }

        // Build rich output with per-community metrics
        int new_id = 0;
        double overall_modularity = 0.0;
        nlohmann::json communities_array = nlohmann::json::array();

        for (auto& [old_comm, members] : communities_map) {
            std::unordered_set<std::string> member_set(members.begin(), members.end());

            // Count directed internal edges (each directed edge A→B counted once
            // via A's outNeighbors; inNeighbors loop does NOT add to internal_edges).
            double internal_edges = 0.0;
            double total_out_degree = 0.0;
            double total_in_degree = 0.0;
            for (const auto& node : members) {
                for (const auto& [nb, weight] : graph.outNeighbors(node)) {
                    if (member_set.count(nb)) {
                      internal_edges += weight;
                    }
                    total_out_degree += weight;
                }
                for (const auto& [nb, weight] : graph.inNeighbors(node)) {
                    total_in_degree += weight;
                }
            }

            // Density: internal directed edges / max possible directed edges within community
            const size_t n_c = members.size();
            double density = 0.0;
            if (n_c > 1) {
                density = internal_edges / static_cast<double>(n_c * (n_c - 1));
                if (density > 1.0) {
                  density = 1.0;
                }
            }

            // Directed modularity contribution (Newman 2016):
            //   Q_c = e_c/m - (k_c^out * k_c^in) / m^2
            const double comm_modularity =
                (internal_edges / m_directed) -
                (total_out_degree * total_in_degree) /
                    (m_directed * m_directed);
            overall_modularity += comm_modularity;

            nlohmann::json comm_obj;
            comm_obj["id"] = new_id;
            comm_obj["members"] = members;
            comm_obj["size"] = static_cast<int>(n_c);
            comm_obj["modularity"] = comm_modularity;
            comm_obj["density"] = density;
            communities_array.push_back(comm_obj);
            ++new_id;
        }

        nlohmann::json result;
        result["communities"] = communities_array;
        result["num_communities"] = new_id;
        result["overall_modularity"] = overall_modularity;
        return result;
    }
};

// ============================================================================
// LABEL_PROPAGATION_COMMUNITIES (enhanced) - Rich community object format
// ============================================================================

/**
 * @brief LABEL_PROPAGATION_COMMUNITIES(edges, options?) - Label propagation (rich output)
 *
 * Detects communities using iterative label propagation and returns a detailed
 * result object containing per-community membership.
 *
 * @param edges   Array of edge documents (each with _from and _to fields)
 * @param options Optional object: {max_iterations: number} (default: 100)
 * @return Object: {communities: [{id, members, size}], num_communities}
 */
class LabelPropagationCommunitiesExtFunction : public IFunction {
private:
    static nlohmann::json emptyResult() {
        nlohmann::json r;
        r["communities"] = nlohmann::json::array();
        r["num_communities"] = 0;
        return r;
    }

public:
    ~LabelPropagationCommunitiesExtFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "LABEL_PROPAGATION_COMMUNITIES",
            "Graph",
            "Fast community detection using label propagation with detailed community metrics",
            {
                {"edges", ArgType::ANY, true, nullptr, "Array of edge documents"},
                {"options", ArgType::OBJECT, false, nlohmann::json::object(),
                 "Optional: {max_iterations: number} (default: 100)"}
            },
            ArgType::OBJECT,
            true,
            false,
            {
                "LABEL_PROPAGATION_COMMUNITIES(edges)",
                "LABEL_PROPAGATION_COMMUNITIES(edges, {max_iterations: 50})"
            },
            FunctionCost{CostComplexity::LINEAR, 200.0, 2.0, false, false, ""}
        };
    }

    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || !args[0].is_array()) {
          return emptyResult();
        }

        auto graph = graph_helpers::buildGraph(args[0]);
        const auto& vertices = graph.vertices();
        if (vertices.empty()) {
          return emptyResult();
        }

        // Parse options: accept either an object {max_iterations: N} or a scalar N
        int max_iterations = 100;
        if (args.size() > 1) {
            if (args[1].is_object() && args[1].contains("max_iterations")) {
                max_iterations = clampIterationsFromJson(args[1]["max_iterations"], max_iterations);
            } else if (args[1].is_number()) {
                max_iterations = clampIterationsFromJson(args[1], max_iterations);
            }
        }

        std::vector<std::string> node_list(vertices.begin(), vertices.end());

        // Initialize: each node gets a unique label
        std::unordered_map<std::string, int> labels;
        int next_label = 0;
        for (const auto& node : node_list) {
            labels[node] = next_label++;
        }

        // Iterative label propagation
        bool changed = true;
        int iteration = 0;

        while (changed && iteration < max_iterations) {
            changed = false;
            iteration++;

            for (const auto& node : node_list) {
                std::unordered_map<int, double> label_votes;

                for (const auto& [nb, weight] : graph.outNeighbors(node)) {
                    auto it = labels.find(nb);
                    if (it != labels.end()) {
                      label_votes[it->second] += weight;
                    }
                }
                for (const auto& [nb, weight] : graph.inNeighbors(node)) {
                    auto it = labels.find(nb);
                    if (it != labels.end()) {
                      label_votes[it->second] += weight;
                    }
                }

                if (label_votes.empty()) {
                  continue;
                }

                int best_label = labels[node];
                double best_votes = 0.0;
                for (const auto& [label, votes] : label_votes) {
                    if (votes > best_votes) {
                        best_votes = votes;
                        best_label = label;
                    }
                }

                if (best_label != labels[node]) {
                    labels[node] = best_label;
                    changed = true;
                }
            }
        }

        // Group nodes by final label
        std::unordered_map<int, std::vector<std::string>> communities_map;
        for (const auto& [node, label] : labels) {
            communities_map[label].push_back(node);
        }

        // Build rich output
        int new_id = 0;
        nlohmann::json communities_array = nlohmann::json::array();

        for (auto& [old_label, members] : communities_map) {
            nlohmann::json comm_obj;
            comm_obj["id"] = new_id++;
            comm_obj["members"] = members;
            comm_obj["size"] = static_cast<int>(members.size());
            communities_array.push_back(comm_obj);
        }

        nlohmann::json result;
        result["communities"] = communities_array;
        result["num_communities"] = new_id;
        return result;
    }
};

// ============================================================================
// Registration
// ============================================================================

inline void registerGraphExtensions(FunctionRegistry& registry) {
    // Advanced centrality: Betweenness (Brandes algorithm)
    registry.registerFunction(std::make_unique<BetweennessCentralityExtFunction>());

    // Community detection: Louvain and Label Propagation (rich output format)
    registry.registerFunction(std::make_unique<LouvainCommunitiesExtFunction>());
    registry.registerFunction(std::make_unique<LabelPropagationCommunitiesExtFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
