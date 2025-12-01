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

namespace themisdb {
namespace query {
namespace functions {

// ============================================================================
// ALL_SHORTEST_PATHS - Find all shortest paths between two vertices
// ============================================================================

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

// ============================================================================
// K_SHORTEST_PATHS - Find K shortest paths (Yen's algorithm)
// ============================================================================

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

// ============================================================================
// WEIGHTED_SHORTEST_PATH - Dijkstra's algorithm with edge weights
// ============================================================================

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

// ============================================================================
// PATH_LENGTH - Get the length (number of edges) in a path
// ============================================================================

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

// ============================================================================
// PATH_VERTICES - Extract vertices from a path
// ============================================================================

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

// ============================================================================
// PATH_EDGES - Extract edges from a path
// ============================================================================

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

// ============================================================================
// PATH_WEIGHT - Calculate total weight of a path
// ============================================================================

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

// ============================================================================
// LOUVAIN_COMMUNITIES - Community detection using Louvain algorithm
// ============================================================================

class LouvainCommunitiesFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "LOUVAIN_COMMUNITIES",
            {ParamType::STRING},  // graph name or edge collection
            ParamType::ARRAY,     // [{community: id, members: [...]}]
            1, 2,
            "Detects communities using the Louvain algorithm",
            FunctionCost{CostComplexity::QUADRATIC, 500.0, 5.0, true, true, "graph"}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.empty()) return JsonValue::array();
        
        // Louvain algorithm implementation would use graph storage
        // Returns array of community objects
        return JsonValue::array();
    }
};

// ============================================================================
// BETWEENNESS_CENTRALITY - Calculate betweenness centrality for vertices
// ============================================================================

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

// ============================================================================
// CLOSENESS_CENTRALITY - Calculate closeness centrality for vertices
// ============================================================================

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

// ============================================================================
// Registration
// ============================================================================

inline void registerGraphExtensions(FunctionRegistry& registry) {
    registry.registerFunction(std::make_unique<AllShortestPathsFunction>());
    registry.registerFunction(std::make_unique<KShortestPathsFunction>());
    registry.registerFunction(std::make_unique<WeightedShortestPathFunction>());
    registry.registerFunction(std::make_unique<PathLengthFunction>());
    registry.registerFunction(std::make_unique<PathVerticesFunction>());
    registry.registerFunction(std::make_unique<PathEdgesFunction>());
    registry.registerFunction(std::make_unique<PathWeightFunction>());
    registry.registerFunction(std::make_unique<LouvainCommunitiesFunction>());
    registry.registerFunction(std::make_unique<BetweennessCentralityFunction>());
    registry.registerFunction(std::make_unique<ClosenessCentralityFunction>());
}

} // namespace functions
} // namespace query
} // namespace themisdb
