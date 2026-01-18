/**
 * @file graph_extensions.h
 * @brief Extended Graph Functions for ThemisDB AQL
 * 
 * Provides advanced graph algorithms and path operations:
 * - All shortest paths
 * - K shortest paths
 * - Weighted shortest path
 * - Path utilities (length, vertices, edges, weight)
 * - Community detection (Louvain)
 * - Centrality measures (Betweenness, Closeness)
 */

#pragma once

#include "function_registry.h"
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <algorithm>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// ALL_SHORTEST_PATHS - Find all shortest paths between two vertices
// ============================================================================
// TODO: Implement - currently a stub using undefined API types
/*
class AllShortestPathsFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "ALL_SHORTEST_PATHS",
            {ParamType::STRING, ParamType::STRING},  // startVertex, endVertex
            ParamType::ARRAY,
            2, 3,  // optional options object
            "Returns all shortest paths between two vertices",
            FunctionCost{CostComplexity::QUADRATIC, 100.0, 1.0, true, true, "graph"}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.size() < 2) return JsonValue::array();
        
        std::string startVertex = args[0].as_string();
        std::string endVertex = args[1].as_string();
        
        // Options parsing
        std::string edgeCollection = "_edges";
        std::string direction = "OUTBOUND";
        int maxDepth = 10;
        
        if (args.size() > 2 && args[2].is_object()) {
            auto opts = args[2].as_object();
            if (opts.count("edgeCollection")) edgeCollection = opts["edgeCollection"].as_string();
            if (opts.count("direction")) direction = opts["direction"].as_string();
            if (opts.count("maxDepth")) maxDepth = static_cast<int>(opts["maxDepth"].as_number());
        }
        
        // BFS to find all shortest paths
        // Implementation would use graph storage
        return JsonValue::array();
    }
};
*/

// ============================================================================
// K_SHORTEST_PATHS - Find K shortest paths (Yen's algorithm)
// ============================================================================
// TODO: Implement - currently a stub using undefined API types
/*
class KShortestPathsFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "K_SHORTEST_PATHS",
            {ParamType::STRING, ParamType::STRING, ParamType::NUMBER},  // start, end, k
            ParamType::ARRAY,
            3, 4,  // optional options
            "Returns the K shortest paths between two vertices (Yen's algorithm)",
            FunctionCost{CostComplexity::QUADRATIC, 200.0, 10.0, true, false, "graph"}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.size() < 3) return JsonValue::array();
        
        std::string startVertex = args[0].as_string();
        std::string endVertex = args[1].as_string();
        int k = static_cast<int>(args[2].as_number());
        
        if (k <= 0) return JsonValue::array();
        
        // Yen's algorithm implementation would go here
        // Uses graph storage to find paths
        return JsonValue::array();
    }
};
*/

// ============================================================================
// WEIGHTED_SHORTEST_PATH - Dijkstra's algorithm with edge weights
// ============================================================================
// TODO: Implement - currently a stub using undefined API types
/*
class WeightedShortestPathFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "WEIGHTED_SHORTEST_PATH",
            {ParamType::STRING, ParamType::STRING, ParamType::STRING},  // start, end, weightAttr
            ParamType::OBJECT,  // {vertices, edges, weight}
            3, 4,
            "Finds the shortest weighted path using Dijkstra's algorithm",
            FunctionCost{CostComplexity::LINEARITHMIC, 50.0, 0.5, true, false, "graph"}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.size() < 3) {
            return JsonValue::object({
                {"vertices", JsonValue::array()},
                {"edges", JsonValue::array()},
                {"weight", 0.0}
            });
        }
        
        std::string startVertex = args[0].as_string();
        std::string endVertex = args[1].as_string();
        std::string weightAttr = args[2].as_string();
        
        // Dijkstra's algorithm implementation
        return JsonValue::object({
            {"vertices", JsonValue::array()},
            {"edges", JsonValue::array()},
            {"weight", std::numeric_limits<double>::infinity()}
        });
    }
};
*/

// ============================================================================
// PATH_LENGTH - Get the length (number of edges) in a path
// ============================================================================
// TODO: Implement - currently a stub using undefined API types
/*
class PathLengthFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "PATH_LENGTH",
            {ParamType::OBJECT},  // path object
            ParamType::NUMBER,
            1, 1,
            "Returns the number of edges in a path",
            FunctionCost{CostComplexity::CONSTANT, 1.0, 0.0, false, true, ""}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.empty() || !args[0].is_object()) return JsonValue(0);
        
        auto path = args[0].as_object();
        if (path.count("edges") && path["edges"].is_array()) {
            return JsonValue(static_cast<double>(path["edges"].as_array().size()));
        }
        if (path.count("vertices") && path["vertices"].is_array()) {
            size_t vcount = path["vertices"].as_array().size();
            return JsonValue(static_cast<double>(vcount > 0 ? vcount - 1 : 0));
        }
        
        return JsonValue(0);
    }
};
*/

// ============================================================================
// PATH_VERTICES - Extract vertices from a path
// ============================================================================
// TODO: Implement - currently a stub using undefined API types
/*
class PathVerticesFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "PATH_VERTICES",
            {ParamType::OBJECT},
            ParamType::ARRAY,
            1, 1,
            "Extracts the vertices from a path object",
            FunctionCost{CostComplexity::CONSTANT, 1.0, 0.0, false, true, ""}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.empty() || !args[0].is_object()) return JsonValue::array();
        
        auto path = args[0].as_object();
        if (path.count("vertices") && path["vertices"].is_array()) {
            return path["vertices"];
        }
        
        return JsonValue::array();
    }
};
*/

// ============================================================================
// PATH_EDGES - Extract edges from a path
// ============================================================================
// TODO: Implement - currently a stub using undefined API types
/*
class PathEdgesFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "PATH_EDGES",
            {ParamType::OBJECT},
            ParamType::ARRAY,
            1, 1,
            "Extracts the edges from a path object",
            FunctionCost{CostComplexity::CONSTANT, 1.0, 0.0, false, true, ""}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.empty() || !args[0].is_object()) return JsonValue::array();
        
        auto path = args[0].as_object();
        if (path.count("edges") && path["edges"].is_array()) {
            return path["edges"];
        }
        
        return JsonValue::array();
    }
};
*/

// ============================================================================
// PATH_WEIGHT - Calculate total weight of a path
// ============================================================================
// TODO: Implement - currently a stub using undefined API types
/*
class PathWeightFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "PATH_WEIGHT",
            {ParamType::OBJECT, ParamType::STRING},  // path, weightAttribute
            ParamType::NUMBER,
            2, 2,
            "Calculates the total weight of edges in a path",
            FunctionCost{CostComplexity::LINEAR, 1.0, 0.01, false, true, ""}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.size() < 2 || !args[0].is_object()) return JsonValue(0.0);
        
        auto path = args[0].as_object();
        std::string weightAttr = args[1].as_string();
        
        if (!path.count("edges") || !path["edges"].is_array()) return JsonValue(0.0);
        
        double totalWeight = 0.0;
        for (const auto& edge : path["edges"].as_array()) {
            if (edge.is_object()) {
                auto edgeObj = edge.as_object();
                if (edgeObj.count(weightAttr)) {
                    totalWeight += edgeObj[weightAttr].as_number();
                }
            }
        }
        
        return JsonValue(totalWeight);
    }
};
*/

// ============================================================================
// LOUVAIN_COMMUNITIES - Community detection using Louvain algorithm
// ============================================================================

class LouvainCommunitiesFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "LOUVAIN_COMMUNITIES",
            "Graph",
            "Detects communities using the Louvain algorithm",
            {
                {"edges", ArgType::ARRAY, true, nullptr, "Array of edge documents"},
                {"options", ArgType::OBJECT, false, nlohmann::json::object(), "Options: min_modularity_gain (default: 0.000001)"}
            },
            ArgType::ARRAY,
            true,
            false,
            {"LOUVAIN_COMMUNITIES([edges])"},
            FunctionCost{CostComplexity::QUADRATIC, 500.0, 5.0, true, true, "graph"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          const FunctionContext&) const override {
        if (args.empty() || !args[0].is_array()) {
            return nlohmann::json::array();
        }
        
        const auto& edges = args[0];
        double min_modularity_gain = 0.000001;
        
        // Parse options
        if (args.size() > 1 && args[1].is_object()) {
            if (args[1].contains("min_modularity_gain")) {
                min_modularity_gain = args[1]["min_modularity_gain"].get<double>();
            }
        }
        
        // Extract nodes and build adjacency
        std::unordered_set<std::string> node_set;
        std::unordered_map<std::string, std::vector<std::string>> adjacency;
        
        for (const auto& edge : edges) {
            if (!edge.is_object() || !edge.contains("_from") || !edge.contains("_to")) {
                continue;
            }
            
            std::string from = edge["_from"].get<std::string>();
            std::string to = edge["_to"].get<std::string>();
            
            node_set.insert(from);
            node_set.insert(to);
            adjacency[from].push_back(to);
            adjacency[to].push_back(from);  // Treat as undirected
        }
        
        if (node_set.empty()) {
            return nlohmann::json::array();
        }
        
        std::vector<std::string> nodes(node_set.begin(), node_set.end());
        
        // Initialize: each node in its own community
        std::unordered_map<std::string, int> node_to_comm;
        int next_comm_id = 0;
        for (const auto& node : nodes) {
            node_to_comm[node] = next_comm_id++;
        }
        
        // Count total edges
        double m = edges.size();
        if (m == 0.0) m = 1.0;
        
        // Compute node degrees
        std::unordered_map<std::string, double> node_degree;
        for (const auto& node : nodes) {
            node_degree[node] = adjacency[node].size();
        }
        
        // Louvain optimization
        bool improved = true;
        int iteration = 0;
        const int MAX_ITERATIONS = 100;
        
        while (improved && iteration < MAX_ITERATIONS) {
            improved = false;
            iteration++;
            
            for (const auto& node : nodes) {
                int current_comm = node_to_comm[node];
                
                // Collect neighboring communities
                std::unordered_map<int, double> comm_edges;
                
                for (const auto& neighbor : adjacency[node]) {
                    comm_edges[node_to_comm[neighbor]] += 1.0;
                }
                
                if (comm_edges.empty()) continue;
                
                // Try each neighboring community
                int best_comm = current_comm;
                double best_delta_q = 0.0;
                
                for (const auto& [candidate_comm, edges_to_comm] : comm_edges) {
                    if (candidate_comm == current_comm) continue;
                    
                    double delta_q = edges_to_comm / m;
                    
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
        
        // Group nodes by community
        std::unordered_map<int, std::vector<std::string>> communities;
        for (const auto& [node, comm] : node_to_comm) {
            communities[comm].push_back(node);
        }
        
        // Calculate modularity for each community
        nlohmann::json result = nlohmann::json::array();
        int comm_id = 0;
        double overall_modularity = 0.0;
        
        for (const auto& [_, members] : communities) {
            // Calculate internal edges
            int internal_edges = 0;
            for (const auto& node : members) {
                for (const auto& neighbor : adjacency[node]) {
                    if (node_to_comm[neighbor] == node_to_comm[node]) {
                        internal_edges++;
                    }
                }
            }
            internal_edges /= 2;  // Each edge counted twice
            
            // Calculate expected edges
            double total_degree = 0.0;
            for (const auto& node : members) {
                total_degree += node_degree[node];
            }
            
            double expected = (total_degree * total_degree) / (4.0 * m);
            double modularity = (internal_edges / m) - expected / (2.0 * m);
            overall_modularity += modularity;
            
            // Calculate density
            double max_edges = members.size() * (members.size() - 1) / 2.0;
            double density = max_edges > 0 ? internal_edges / max_edges : 0.0;
            
            nlohmann::json comm_obj = {
                {"id", comm_id++},
                {"members", members},
                {"size", members.size()},
                {"modularity", modularity},
                {"density", density}
            };
            
            result.push_back(comm_obj);
        }
        
        // Wrap in result object
        return nlohmann::json{
            {"communities", result},
            {"overall_modularity", overall_modularity},
            {"num_communities", communities.size()}
        };
    }
};

// ============================================================================
// LABEL_PROPAGATION_COMMUNITIES - Fast community detection
// ============================================================================

class LabelPropagationCommunitiesFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "LABEL_PROPAGATION_COMMUNITIES",
            "Graph",
            "Fast community detection using label propagation",
            {
                {"edges", ArgType::ARRAY, true, nullptr, "Array of edge documents"},
                {"options", ArgType::OBJECT, false, nlohmann::json::object(), "Options: max_iterations (default: 100)"}
            },
            ArgType::ARRAY,
            true,
            false,
            {"LABEL_PROPAGATION_COMMUNITIES([edges])"},
            FunctionCost{CostComplexity::LINEAR, 200.0, 2.0, true, true, "graph"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          const FunctionContext&) const override {
        if (args.empty() || !args[0].is_array()) {
            return nlohmann::json::array();
        }
        
        const auto& edges = args[0];
        int max_iterations = 100;
        
        // Parse options
        if (args.size() > 1 && args[1].is_object()) {
            if (args[1].contains("max_iterations")) {
                max_iterations = args[1]["max_iterations"].get<int>();
            }
        }
        
        // Extract nodes and build adjacency
        std::unordered_set<std::string> node_set;
        std::unordered_map<std::string, std::vector<std::string>> adjacency;
        
        for (const auto& edge : edges) {
            if (!edge.is_object() || !edge.contains("_from") || !edge.contains("_to")) {
                continue;
            }
            
            std::string from = edge["_from"].get<std::string>();
            std::string to = edge["_to"].get<std::string>();
            
            node_set.insert(from);
            node_set.insert(to);
            adjacency[from].push_back(to);
            adjacency[to].push_back(from);  // Treat as undirected
        }
        
        if (node_set.empty()) {
            return nlohmann::json::array();
        }
        
        std::vector<std::string> nodes(node_set.begin(), node_set.end());
        
        // Initialize: each node gets unique label
        std::unordered_map<std::string, int> labels;
        int next_label = 0;
        for (const auto& node : nodes) {
            labels[node] = next_label++;
        }
        
        // Iterative label propagation
        bool changed = true;
        int iteration = 0;
        
        while (changed && iteration < max_iterations) {
            changed = false;
            iteration++;
            
            for (const auto& node : nodes) {
                // Count labels among neighbors
                std::unordered_map<int, int> label_count;
                
                for (const auto& neighbor : adjacency[node]) {
                    label_count[labels[neighbor]]++;
                }
                
                if (label_count.empty()) continue;
                
                // Find most frequent label
                int best_label = labels[node];
                int best_count = 0;
                
                for (const auto& [label, count] : label_count) {
                    if (count > best_count) {
                        best_count = count;
                        best_label = label;
                    }
                }
                
                // Update label if changed
                if (best_label != labels[node]) {
                    labels[node] = best_label;
                    changed = true;
                }
            }
        }
        
        // Group nodes by community (label)
        std::unordered_map<int, std::vector<std::string>> communities;
        for (const auto& [node, label] : labels) {
            communities[label].push_back(node);
        }
        
        // Format result
        nlohmann::json result = nlohmann::json::array();
        int comm_id = 0;
        
        for (const auto& [_, members] : communities) {
            nlohmann::json comm_obj = {
                {"id", comm_id++},
                {"members", members},
                {"size", members.size()}
            };
            
            result.push_back(comm_obj);
        }
        
        return nlohmann::json{
            {"communities", result},
            {"num_communities", communities.size()}
        };
    }
};

// ============================================================================
// BETWEENNESS_CENTRALITY - Calculate betweenness centrality for vertices
// ============================================================================
// TODO: Implement - currently a stub using undefined API types
/*
class BetweennessCentralityFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "BETWEENNESS_CENTRALITY",
            {ParamType::STRING},  // graph name or edge collection
            ParamType::OBJECT,    // {vertex: score, ...}
            1, 2,
            "Calculates betweenness centrality for all vertices",
            FunctionCost{CostComplexity::QUADRATIC, 1000.0, 10.0, true, true, "graph"}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.empty()) return JsonValue::object();
        
        // Brandes' algorithm implementation would go here
        return JsonValue::object();
    }
};
*/

// ============================================================================
// CLOSENESS_CENTRALITY - Calculate closeness centrality for vertices
// ============================================================================
// TODO: Implement - currently a stub using undefined API types
/*
class ClosenessCentralityFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "CLOSENESS_CENTRALITY",
            {ParamType::STRING},
            ParamType::OBJECT,
            1, 2,
            "Calculates closeness centrality for all vertices",
            FunctionCost{CostComplexity::QUADRATIC, 800.0, 8.0, true, true, "graph"}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.empty()) return JsonValue::object();
        
        // BFS-based closeness calculation
        return JsonValue::object();
    }
};
*/

// ============================================================================
// Registration
// ============================================================================

inline void registerGraphExtensions(FunctionRegistry& registry) {
    // Community detection functions (implemented)
    registry.registerFunction(std::make_unique<LouvainCommunitiesFunction>());
    registry.registerFunction(std::make_unique<LabelPropagationCommunitiesFunction>());
    
    // TODO: Implement these stub functions
    // registry.registerFunction(std::make_unique<AllShortestPathsFunction>());
    // registry.registerFunction(std::make_unique<KShortestPathsFunction>());
    // registry.registerFunction(std::make_unique<WeightedShortestPathFunction>());
    // registry.registerFunction(std::make_unique<PathLengthFunction>());
    // registry.registerFunction(std::make_unique<PathVerticesFunction>());
    // registry.registerFunction(std::make_unique<PathEdgesFunction>());
    // registry.registerFunction(std::make_unique<PathWeightFunction>());
    // registry.registerFunction(std::make_unique<BetweennessCentralityFunction>());
    // registry.registerFunction(std::make_unique<ClosenessCentralityFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
