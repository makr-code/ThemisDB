# GPU Impact Analysis Plugin - ThemisDB Integration Guide

This document provides comprehensive guidance for integrating the GPU Impact Analysis Plugin with ThemisDB's core systems.

## Table of Contents

1. [Integration Architecture](#integration-architecture)
2. [AQL Function Registration](#aql-function-registration)
3. [REST API Endpoints](#rest-api-endpoints)
4. [Plugin Lifecycle Management](#plugin-lifecycle-management)
5. [Data Access Patterns](#data-access-patterns)
6. [Performance Optimization](#performance-optimization)
7. [Error Handling](#error-handling)
8. [Deployment Guide](#deployment-guide)

---

## Integration Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                        Client Layer                          │
│  ┌─────────┐  ┌─────────┐  ┌──────────┐  ┌──────────┐      │
│  │ AQL CLI │  │ REST API│  │ Web UI   │  │ SDK      │      │
│  └────┬────┘  └────┬────┘  └────┬─────┘  └────┬─────┘      │
└───────┼───────────┼────────────┼─────────────┼──────────────┘
        │           │            │             │
        └───────────┴────────────┴─────────────┘
                    │
┌───────────────────┼─────────────────────────────────────────┐
│              ThemisDB Core                                   │
│                   │                                          │
│  ┌────────────────▼────────────────────────────────┐       │
│  │           AQL Query Engine                       │       │
│  │  ┌──────────────────────────────────────────┐   │       │
│  │  │  Function Registry                        │   │       │
│  │  │  - GPU_ANALYZE_IMPACT()                  │   │       │
│  │  │  - GPU_MONTE_CARLO_RISK()                │   │       │
│  │  │  - GPU_DETECT_PATTERNS()                 │   │       │
│  │  └──────────────┬───────────────────────────┘   │       │
│  └─────────────────┼───────────────────────────────┘       │
│                    │                                        │
│  ┌─────────────────▼───────────────────────────────┐       │
│  │           Plugin Manager                         │       │
│  │  - Load/Unload plugins                          │       │
│  │  - Lifecycle management                          │       │
│  │  - License validation                            │       │
│  └─────────────────┬───────────────────────────────┘       │
│                    │                                        │
│  ┌─────────────────▼───────────────────────────────┐       │
│  │         REST API Handler                         │       │
│  │  POST /api/v1/analytics/impact                  │       │
│  │  POST /api/v1/analytics/monte-carlo             │       │
│  │  GET  /api/v1/analytics/patterns                │       │
│  └──────────────────────────────────────────────────┘       │
│                                                             │
│  ┌──────────────────────────────────────────────────┐      │
│  │      Graph Index Manager                         │      │
│  │  - Node/Edge storage                             │      │
│  │  - Graph traversal                               │      │
│  │  - Metadata access                               │      │
│  └──────────────────────────────────────────────────┘      │
│                                                             │
│  ┌──────────────────────────────────────────────────┐      │
│  │         RocksDB Storage Layer                     │      │
│  └──────────────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────────────┘
                    │
                    │ REST API calls only
                    │ (no direct RocksDB access)
                    │
┌───────────────────▼─────────────────────────────────────────┐
│           GPU Impact Analysis Plugin                         │
│  ┌──────────────────────────────────────────────────┐       │
│  │  IGPUImpactAnalysisPlugin Interface              │       │
│  │  - analyzeImpact()                               │       │
│  │  - performMonteCarloRisk()                       │       │
│  │  - detectPatterns()                              │       │
│  │  - analyzeWhatIfScenarios()                      │       │
│  │  - performSensitivityAnalysis()                  │       │
│  │  - performRootCauseAnalysis()                    │       │
│  └──────────────────────────────────────────────────┘       │
│                                                             │
│  ┌──────────────────────────────────────────────────┐       │
│  │         FEM Algorithm Engine                      │       │
│  │  - Graph propagation                             │       │
│  │  - Convergence solver                            │       │
│  │  - Metadata extraction                           │       │
│  └──────────────────────────────────────────────────┘       │
│                                                             │
│  ┌──────────────────────────────────────────────────┐       │
│  │         GPU Backend Manager                       │       │
│  │  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐         │       │
│  │  │ CUDA │  │Vulkan│  │  HIP │  │OpenCL│         │       │
│  │  └──────┘  └──────┘  └──────┘  └──────┘         │       │
│  └──────────────────────────────────────────────────┘       │
└─────────────────────────────────────────────────────────────┘
```

### Key Architectural Principles

1. **Plugin Isolation**: Plugin communicates with ThemisDB **only via REST API**
2. **No Direct Storage Access**: Plugin has **no** RocksDB access
3. **Stateless Plugin**: All state managed by ThemisDB Core
4. **License Gated**: Enterprise feature with license validation
5. **Optional Deployment**: ThemisDB works without plugin

---

## AQL Function Registration

### Registration Code (ThemisDB Core)

Add to `src/aql/function_registry.cpp`:

```cpp
#include "plugin/plugin_manager.h"
#include "enterprise/gpu_impact_analysis_plugin.h"

namespace themis {
namespace aql {

void FunctionRegistry::registerEnterprisePluginFunctions() {
    auto& plugin_mgr = PluginManager::getInstance();
    
    // Check if GPU Impact Analysis plugin is loaded
    if (!plugin_mgr.isPluginLoaded("gpu_impact_analysis")) {
        spdlog::info("GPU Impact Analysis plugin not loaded, skipping function registration");
        return;
    }
    
    auto plugin = plugin_mgr.getPlugin<enterprise::IGPUImpactAnalysisPlugin>("gpu_impact_analysis");
    if (!plugin) {
        spdlog::warn("Failed to get GPU Impact Analysis plugin interface");
        return;
    }
    
    // Register GPU_ANALYZE_IMPACT()
    registerFunction("GPU_ANALYZE_IMPACT", 
        FunctionSignature{
            .name = "GPU_ANALYZE_IMPACT",
            .description = "Analyzes impact of document changes using FEM-inspired graph propagation",
            .parameters = {
                {"change", ParameterType::Object, "Document change specification"},
                {"options", ParameterType::Object, "Analysis options (optional)"}
            },
            .return_type = ReturnType::Object,
            .implementation = [plugin](const std::vector<Value>& args) -> Value {
                // Parse arguments
                if (args.size() < 1) {
                    throw std::invalid_argument("GPU_ANALYZE_IMPACT requires at least 1 argument");
                }
                
                json change_json = args[0].toJSON();
                
                // Parse change
                enterprise::DocumentChange change;
                change.document_id = change_json["document_id"].get<std::string>();
                change.change_type = change_json.value("change_type", "modification");
                change.magnitude = change_json.value("magnitude", 1.0);
                change.timestamp = std::chrono::system_clock::now();
                
                // Parse options
                enterprise::AnalysisOptions options;
                if (args.size() >= 2) {
                    json options_json = args[1].toJSON();
                    options.max_depth = options_json.value("max_depth", 10);
                    options.impact_threshold = options_json.value("impact_threshold", 0.01);
                    options.use_fem_metadata = options_json.value("use_fem_metadata", true);
                    options.use_temporal_decay = options_json.value("use_temporal_decay", false);
                }
                
                // Get graph structure via GraphIndexManager
                auto& graph_mgr = GraphIndexManager::getInstance();
                json graph = graph_mgr.getGraphStructure(change.document_id, options.max_depth);
                
                // Call plugin
                json result = plugin->analyzeImpact(change, graph, options);
                
                return Value::fromJSON(result);
            }
        }
    );
    
    // Register GPU_MONTE_CARLO_RISK()
    registerFunction("GPU_MONTE_CARLO_RISK",
        FunctionSignature{
            .name = "GPU_MONTE_CARLO_RISK",
            .description = "Performs Monte Carlo risk assessment for document changes",
            .parameters = {
                {"change", ParameterType::Object, "Document change specification"},
                {"options", ParameterType::Object, "Risk options (optional)"}
            },
            .return_type = ReturnType::Object,
            .implementation = [plugin](const std::vector<Value>& args) -> Value {
                if (args.size() < 1) {
                    throw std::invalid_argument("GPU_MONTE_CARLO_RISK requires at least 1 argument");
                }
                
                json change_json = args[0].toJSON();
                
                enterprise::DocumentChange change;
                change.document_id = change_json["document_id"].get<std::string>();
                change.change_type = change_json.value("change_type", "modification");
                change.magnitude = change_json.value("magnitude", 1.0);
                change.timestamp = std::chrono::system_clock::now();
                
                enterprise::RiskOptions options;
                if (args.size() >= 2) {
                    json options_json = args[1].toJSON();
                    options.num_simulations = options_json.value("num_simulations", 10000);
                    options.confidence_level = options_json.value("confidence_level", 0.95);
                }
                
                auto& graph_mgr = GraphIndexManager::getInstance();
                json graph = graph_mgr.getGraphStructure(change.document_id, 10);
                
                json result = plugin->performMonteCarloRisk(change, graph, options);
                
                return Value::fromJSON(result);
            }
        }
    );
    
    // Register GPU_DETECT_PATTERNS()
    registerFunction("GPU_DETECT_PATTERNS",
        FunctionSignature{
            .name = "GPU_DETECT_PATTERNS",
            .description = "Detects recurring patterns in historical impact analysis results",
            .parameters = {
                {"historical_results", ParameterType::Array, "Array of historical analysis results"}
            },
            .return_type = ReturnType::Object,
            .implementation = [plugin](const std::vector<Value>& args) -> Value {
                if (args.size() < 1) {
                    throw std::invalid_argument("GPU_DETECT_PATTERNS requires 1 argument");
                }
                
                std::vector<json> historical_results;
                for (const auto& item : args[0].toArray()) {
                    historical_results.push_back(item.toJSON());
                }
                
                json result = plugin->detectPatterns(historical_results);
                
                return Value::fromJSON(result);
            }
        }
    );
    
    // Register GPU_DETECT_ANOMALIES()
    registerFunction("GPU_DETECT_ANOMALIES",
        FunctionSignature{
            .name = "GPU_DETECT_ANOMALIES",
            .description = "Detects anomalies in historical impact analysis results",
            .parameters = {
                {"historical_results", ParameterType::Array, "Array of historical analysis results"}
            },
            .return_type = ReturnType::Object,
            .implementation = [plugin](const std::vector<Value>& args) -> Value {
                if (args.size() < 1) {
                    throw std::invalid_argument("GPU_DETECT_ANOMALIES requires 1 argument");
                }
                
                std::vector<json> historical_results;
                for (const auto& item : args[0].toArray()) {
                    historical_results.push_back(item.toJSON());
                }
                
                json result = plugin->detectAnomalies(historical_results);
                
                return Value::fromJSON(result);
            }
        }
    );
    
    // Register GPU_WHAT_IF_SCENARIOS()
    registerFunction("GPU_WHAT_IF_SCENARIOS",
        FunctionSignature{
            .name = "GPU_WHAT_IF_SCENARIOS",
            .description = "Analyzes multiple what-if scenarios for comparison",
            .parameters = {
                {"scenarios", ParameterType::Array, "Array of scenario specifications"}
            },
            .return_type = ReturnType::Object,
            .implementation = [plugin](const std::vector<Value>& args) -> Value {
                if (args.size() < 1) {
                    throw std::invalid_argument("GPU_WHAT_IF_SCENARIOS requires 1 argument");
                }
                
                std::vector<enterprise::DocumentChange> scenarios;
                for (const auto& item : args[0].toArray()) {
                    json scenario_json = item.toJSON();
                    enterprise::DocumentChange change;
                    change.document_id = scenario_json["document_id"].get<std::string>();
                    change.change_type = scenario_json.value("change_type", "modification");
                    change.magnitude = scenario_json.value("magnitude", 1.0);
                    change.timestamp = std::chrono::system_clock::now();
                    scenarios.push_back(change);
                }
                
                auto& graph_mgr = GraphIndexManager::getInstance();
                json graph = graph_mgr.getGraphStructure(scenarios[0].document_id, 10);
                
                enterprise::AnalysisOptions options;
                json result = plugin->analyzeWhatIfScenarios(graph, scenarios, options);
                
                return Value::fromJSON(result);
            }
        }
    );
    
    spdlog::info("Registered {} GPU Impact Analysis AQL functions", 5);
}

} // namespace aql
} // namespace themis
```

### AQL Usage Examples

```sql
-- Example 1: Basic Impact Analysis
LET impact = GPU_ANALYZE_IMPACT(
  {document_id: 'docs/api/payment.md', magnitude: 0.9},
  {max_depth: 10, impact_threshold: 0.01}
)

FOR node IN impact.affected_nodes
  FILTER node.impact_score > 0.5
  SORT node.impact_score DESC
  LIMIT 20
  RETURN {
    document: node.node_id,
    impact: node.impact_score,
    type: node.node_type
  }

-- Example 2: Monte Carlo Risk with Filtering
LET risk = GPU_MONTE_CARLO_RISK(
  {document_id: 'laws/GDPR_Article_17', magnitude: 0.99},
  {num_simulations: 100000, confidence_level: 0.99}
)

RETURN {
  expected_impact: risk.expected_impact,
  worst_case: risk.value_at_risk_99,
  scenario_count: risk.num_scenarios_above_threshold
}

-- Example 3: Pattern Detection on Historical Data
LET historical = (
  FOR result IN impact_analysis_history
    FILTER result.timestamp > DATE_SUB(NOW(), 30, 'day')
    SORT result.timestamp ASC
    RETURN result
)

LET patterns = GPU_DETECT_PATTERNS(historical)

FOR pattern IN patterns.patterns
  RETURN {
    period: pattern.period_days,
    strength: pattern.correlation,
    description: pattern.description
  }

-- Example 4: What-If Scenario Comparison
LET scenarios = [
  {document_id: 'products/pricing', magnitude: 0.5, change_type: 'price_increase_5pct'},
  {document_id: 'products/pricing', magnitude: 0.7, change_type: 'price_increase_10pct'},
  {document_id: 'products/pricing', magnitude: 0.9, change_type: 'price_increase_20pct'}
]

LET comparison = GPU_WHAT_IF_SCENARIOS(scenarios)

FOR scenario IN comparison.scenarios
  RETURN {
    change_type: scenario.change_type,
    total_impact: scenario.total_impact,
    affected_customers: scenario.affected_nodes_by_type.customer,
    revenue_impact: scenario.estimated_revenue_impact
  }
```

---

## REST API Endpoints

### Endpoint Implementation (ThemisDB Core)

Add to `src/http/handlers/analytics_handler.cpp`:

```cpp
#include "http/handlers/analytics_handler.h"
#include "plugin/plugin_manager.h"
#include "enterprise/gpu_impact_analysis_plugin.h"
#include <spdlog/spdlog.h>

namespace themis {
namespace http {

void AnalyticsHandler::registerRoutes(httplib::Server& server) {
    // POST /api/v1/analytics/impact
    server.Post("/api/v1/analytics/impact", [this](const httplib::Request& req, httplib::Response& res) {
        handleImpactAnalysis(req, res);
    });
    
    // POST /api/v1/analytics/monte-carlo
    server.Post("/api/v1/analytics/monte-carlo", [this](const httplib::Request& req, httplib::Response& res) {
        handleMonteCarloRisk(req, res);
    });
    
    // POST /api/v1/analytics/patterns
    server.Post("/api/v1/analytics/patterns", [this](const httplib::Request& req, httplib::Response& res) {
        handlePatternDetection(req, res);
    });
    
    // POST /api/v1/analytics/anomalies
    server.Post("/api/v1/analytics/anomalies", [this](const httplib::Request& req, httplib::Response& res) {
        handleAnomalyDetection(req, res);
    });
    
    // POST /api/v1/analytics/what-if
    server.Post("/api/v1/analytics/what-if", [this](const httplib::Request& req, httplib::Response& res) {
        handleWhatIfScenarios(req, res);
    });
    
    spdlog::info("Registered GPU Impact Analysis REST API endpoints");
}

void AnalyticsHandler::handleImpactAnalysis(const httplib::Request& req, httplib::Response& res) {
    try {
        // Get plugin
        auto& plugin_mgr = PluginManager::getInstance();
        if (!plugin_mgr.isPluginLoaded("gpu_impact_analysis")) {
            res.status = 503;
            res.set_content(json{
                {"error", "GPU Impact Analysis plugin not loaded"},
                {"code", "PLUGIN_NOT_AVAILABLE"}
            }.dump(), "application/json");
            return;
        }
        
        auto plugin = plugin_mgr.getPlugin<enterprise::IGPUImpactAnalysisPlugin>("gpu_impact_analysis");
        
        // Parse request
        json request_json = json::parse(req.body);
        
        // Extract change
        enterprise::DocumentChange change;
        change.document_id = request_json["document_id"].get<std::string>();
        change.change_type = request_json.value("change_type", "modification");
        change.magnitude = request_json.value("magnitude", 1.0);
        change.timestamp = std::chrono::system_clock::now();
        
        // Extract options
        enterprise::AnalysisOptions options;
        if (request_json.contains("options")) {
            auto opts = request_json["options"];
            options.max_depth = opts.value("max_depth", 10);
            options.impact_threshold = opts.value("impact_threshold", 0.01);
            options.use_fem_metadata = opts.value("use_fem_metadata", true);
            options.use_temporal_decay = opts.value("use_temporal_decay", false);
        }
        
        // Get graph structure
        auto& graph_mgr = GraphIndexManager::getInstance();
        json graph = graph_mgr.getGraphStructure(change.document_id, options.max_depth);
        
        // Perform analysis
        auto start = std::chrono::high_resolution_clock::now();
        json result = plugin->analyzeImpact(change, graph, options);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        // Add metadata
        result["_metadata"] = {
            {"query_time_ms", duration_ms},
            {"graph_nodes", graph["nodes"].size()},
            {"graph_edges", graph["edges"].size()},
            {"plugin_version", plugin->getVersion()}
        };
        
        res.status = 200;
        res.set_content(result.dump(), "application/json");
        
    } catch (const std::exception& e) {
        spdlog::error("Error in handleImpactAnalysis: {}", e.what());
        res.status = 500;
        res.set_content(json{
            {"error", e.what()},
            {"code", "INTERNAL_ERROR"}
        }.dump(), "application/json");
    }
}

void AnalyticsHandler::handleMonteCarloRisk(const httplib::Request& req, httplib::Response& res) {
    try {
        auto& plugin_mgr = PluginManager::getInstance();
        auto plugin = plugin_mgr.getPlugin<enterprise::IGPUImpactAnalysisPlugin>("gpu_impact_analysis");
        
        if (!plugin) {
            res.status = 503;
            res.set_content(json{{"error", "Plugin not available"}}.dump(), "application/json");
            return;
        }
        
        json request_json = json::parse(req.body);
        
        enterprise::DocumentChange change;
        change.document_id = request_json["document_id"].get<std::string>();
        change.change_type = request_json.value("change_type", "modification");
        change.magnitude = request_json.value("magnitude", 1.0);
        change.timestamp = std::chrono::system_clock::now();
        
        enterprise::RiskOptions options;
        if (request_json.contains("options")) {
            auto opts = request_json["options"];
            options.num_simulations = opts.value("num_simulations", 10000);
            options.confidence_level = opts.value("confidence_level", 0.95);
        }
        
        auto& graph_mgr = GraphIndexManager::getInstance();
        json graph = graph_mgr.getGraphStructure(change.document_id, 10);
        
        auto start = std::chrono::high_resolution_clock::now();
        json result = plugin->performMonteCarloRisk(change, graph, options);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        result["_metadata"] = {
            {"query_time_ms", duration_ms},
            {"simulations", options.num_simulations}
        };
        
        res.status = 200;
        res.set_content(result.dump(), "application/json");
        
    } catch (const std::exception& e) {
        spdlog::error("Error in handleMonteCarloRisk: {}", e.what());
        res.status = 500;
        res.set_content(json{{"error", e.what()}}.dump(), "application/json");
    }
}

// Similar implementations for other endpoints...

} // namespace http
} // namespace themis
```

### REST API Usage Examples

```bash
# Example 1: Impact Analysis
curl -X POST http://localhost:8765/api/v1/analytics/impact \
  -H "Content-Type: application/json" \
  -d '{
    "document_id": "docs/api/payment.md",
    "magnitude": 0.9,
    "change_type": "breaking_change",
    "options": {
      "max_depth": 10,
      "impact_threshold": 0.01,
      "use_fem_metadata": true
    }
  }'

# Example 2: Monte Carlo Risk
curl -X POST http://localhost:8765/api/v1/analytics/monte-carlo \
  -H "Content-Type: application/json" \
  -d '{
    "document_id": "laws/GDPR_Article_17",
    "magnitude": 0.99,
    "change_type": "regulatory_change",
    "options": {
      "num_simulations": 100000,
      "confidence_level": 0.99
    }
  }'

# Example 3: Pattern Detection
curl -X POST http://localhost:8765/api/v1/analytics/patterns \
  -H "Content-Type: application/json" \
  -d '{
    "historical_results": [
      {"timestamp": 1, "total_impact": 1.2, "affected_count": 45},
      {"timestamp": 2, "total_impact": 1.3, "affected_count": 48},
      ...
    ]
  }'
```

---

## Plugin Lifecycle Management

### Plugin Loading (Startup)

Add to `src/plugin/plugin_manager.cpp`:

```cpp
void PluginManager::loadEnterprisePlugins() {
    spdlog::info("Loading enterprise plugins...");
    
    // Check if GPU Impact Analysis plugin exists
    std::filesystem::path plugin_path = 
        config_->getPluginDirectory() / "enterprise" / "gpu_impact_analysis";
    
    if (!std::filesystem::exists(plugin_path)) {
        spdlog::info("GPU Impact Analysis plugin not found at {}", plugin_path.string());
        return;
    }
    
    // Load plugin configuration
    std::filesystem::path config_file = plugin_path / "config.yaml";
    if (!std::filesystem::exists(config_file)) {
        spdlog::warn("Plugin config not found: {}", config_file.string());
        return;
    }
    
    YAML::Node config = YAML::LoadFile(config_file.string());
    
    // Check license
    std::string license_key = config_->getLicenseKey("gpu_impact_analysis");
    if (license_key.empty()) {
        spdlog::warn("No license key for GPU Impact Analysis plugin");
        return;
    }
    
    // Load shared library
    std::filesystem::path lib_path;
    #ifdef _WIN32
        lib_path = plugin_path / "gpu_impact_analysis.dll";
    #elif __APPLE__
        lib_path = plugin_path / "libgpu_impact_analysis.dylib";
    #else
        lib_path = plugin_path / "libgpu_impact_analysis.so";
    #endif
    
    if (!std::filesystem::exists(lib_path)) {
        spdlog::warn("Plugin library not found: {}", lib_path.string());
        return;
    }
    
    // Load library
    void* handle = dlopen(lib_path.string().c_str(), RTLD_NOW);
    if (!handle) {
        spdlog::error("Failed to load plugin library: {}", dlerror());
        return;
    }
    
    // Get factory function
    using FactoryFunc = enterprise::IGPUImpactAnalysisPlugin* (*)();
    auto create_plugin = (FactoryFunc)dlsym(handle, "createGPUImpactAnalysisPlugin");
    if (!create_plugin) {
        spdlog::error("Failed to find plugin factory function");
        dlclose(handle);
        return;
    }
    
    // Create plugin instance
    auto plugin = std::unique_ptr<enterprise::IGPUImpactAnalysisPlugin>(create_plugin());
    
    // Initialize plugin
    json init_config = {
        {"plugin_name", "gpu_impact_analysis"},
        {"version", config["version"].as<std::string>()},
        {"gpu_enabled", config["gpu"]["enabled"].as<bool>()},
        {"gpu_backend", config["gpu"]["backend"].as<std::string>()},
        {"license_key", license_key}
    };
    
    if (!plugin->initialize(init_config)) {
        spdlog::error("Failed to initialize GPU Impact Analysis plugin");
        return;
    }
    
    // Register plugin
    plugins_["gpu_impact_analysis"] = std::move(plugin);
    plugin_handles_["gpu_impact_analysis"] = handle;
    
    spdlog::info("Loaded GPU Impact Analysis plugin v{}", config["version"].as<std::string>());
}
```

### Plugin Unloading (Shutdown)

```cpp
void PluginManager::shutdown() {
    spdlog::info("Shutting down plugins...");
    
    for (auto& [name, plugin] : plugins_) {
        spdlog::info("Shutting down plugin: {}", name);
        plugin->shutdown();
    }
    
    plugins_.clear();
    
    for (auto& [name, handle] : plugin_handles_) {
        dlclose(handle);
    }
    
    plugin_handles_.clear();
}
```

---

## Data Access Patterns

### Graph Structure Retrieval

The plugin accesses graph data via GraphIndexManager:

```cpp
// In ThemisDB Core: src/index/graph_index_manager.cpp

json GraphIndexManager::getGraphStructure(
    const std::string& start_node_id,
    int max_depth
) {
    json graph;
    graph["nodes"] = json::array();
    graph["edges"] = json::array();
    
    std::unordered_set<std::string> visited_nodes;
    std::queue<std::pair<std::string, int>> queue;
    
    queue.push({start_node_id, 0});
    visited_nodes.insert(start_node_id);
    
    while (!queue.empty()) {
        auto [node_id, depth] = queue.front();
        queue.pop();
        
        if (depth > max_depth) {
            continue;
        }
        
        // Get node data
        auto node_data = getNode(node_id);
        if (node_data) {
            graph["nodes"].push_back(*node_data);
        }
        
        // Get outgoing edges
        auto edges = getOutgoingEdges(node_id);
        for (const auto& edge : edges) {
            graph["edges"].push_back(edge);
            
            std::string to_node = edge["to"].get<std::string>();
            if (visited_nodes.find(to_node) == visited_nodes.end()) {
                queue.push({to_node, depth + 1});
                visited_nodes.insert(to_node);
            }
        }
    }
    
    return graph;
}
```

### FEM Metadata Extraction

```cpp
// Automatically extract FEM metadata during ingestion
// In ThemisDB Core: src/ingestion/document_processor.cpp

void DocumentProcessor::processDocument(const json& document) {
    // ... existing ingestion logic ...
    
    // Extract FEM metadata if plugin is loaded
    auto& plugin_mgr = PluginManager::getInstance();
    if (plugin_mgr.isPluginLoaded("gpu_impact_analysis")) {
        auto plugin = plugin_mgr.getPlugin<enterprise::IGPUImpactAnalysisPlugin>("gpu_impact_analysis");
        
        // Calculate node metadata
        auto node_metadata = plugin->calculateNodeMetadata(
            document,
            getHistoricalChanges(document["id"]),
            {}
        );
        
        // Add to document
        document["_fem_metadata"] = {
            {"inertia", node_metadata.inertia},
            {"change_amplification", node_metadata.change_amplification},
            {"stability", node_metadata.stability},
            {"impact_radius", node_metadata.impact_radius}
        };
    }
    
    // ... continue ingestion ...
}

void DocumentProcessor::processEdge(const json& edge, const json& from_node, const json& to_node) {
    // ... existing edge processing ...
    
    // Extract FEM metadata for edge
    auto& plugin_mgr = PluginManager::getInstance();
    if (plugin_mgr.isPluginLoaded("gpu_impact_analysis")) {
        auto plugin = plugin_mgr.getPlugin<enterprise::IGPUImpactAnalysisPlugin>("gpu_impact_analysis");
        
        auto edge_metadata = plugin->calculateEdgeMetadata(
            edge["type"].get<std::string>(),
            from_node,
            to_node,
            edge.value("context", json{})
        );
        
        edge["_fem_metadata"] = {
            {"weight", edge_metadata.weight},
            {"damping_coefficient", edge_metadata.damping_coefficient},
            {"material_stiffness", edge_metadata.material_stiffness},
            {"criticality", edge_metadata.criticality}
        };
    }
    
    // ... continue edge processing ...
}
```

---

## Performance Optimization

### Caching Strategy

```cpp
// Cache frequently accessed graph structures
class GraphCache {
public:
    std::optional<json> getGraphStructure(const std::string& node_id, int max_depth) {
        std::string cache_key = node_id + ":" + std::to_string(max_depth);
        
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        auto it = cache_.find(cache_key);
        if (it != cache_.end()) {
            // Update LRU
            lru_list_.erase(it->second.lru_it);
            lru_list_.push_front(cache_key);
            it->second.lru_it = lru_list_.begin();
            
            cache_hits_++;
            return it->second.data;
        }
        
        cache_misses_++;
        return std::nullopt;
    }
    
    void putGraphStructure(const std::string& node_id, int max_depth, const json& data) {
        std::string cache_key = node_id + ":" + std::to_string(max_depth);
        
        std::lock_guard<std::mutex> lock(cache_mutex_);
        
        // Evict if cache full
        if (cache_.size() >= max_cache_size_) {
            std::string evict_key = lru_list_.back();
            lru_list_.pop_back();
            cache_.erase(evict_key);
        }
        
        lru_list_.push_front(cache_key);
        cache_[cache_key] = {data, lru_list_.begin()};
    }
    
private:
    struct CacheEntry {
        json data;
        std::list<std::string>::iterator lru_it;
    };
    
    std::unordered_map<std::string, CacheEntry> cache_;
    std::list<std::string> lru_list_;
    std::mutex cache_mutex_;
    size_t max_cache_size_ = 1000;
    size_t cache_hits_ = 0;
    size_t cache_misses_ = 0;
};
```

### Batch Processing

```cpp
// Process multiple impact analyses in batch
json AnalyticsHandler::handleBatchImpactAnalysis(const std::vector<DocumentChange>& changes) {
    auto plugin = PluginManager::getInstance().getPlugin<enterprise::IGPUImpactAnalysisPlugin>("gpu_impact_analysis");
    
    // Group changes by proximity in graph
    auto grouped_changes = groupChangesByProximity(changes);
    
    json results = json::array();
    
    for (const auto& group : grouped_changes) {
        // Load graph once for entire group
        json graph = loadGraphForGroup(group);
        
        // Process all changes in group
        for (const auto& change : group) {
            json result = plugin->analyzeImpact(change, graph, {});
            results.push_back(result);
        }
    }
    
    return results;
}
```

---

## Error Handling

### Plugin Error Recovery

```cpp
try {
    json result = plugin->analyzeImpact(change, graph, options);
    return result;
    
} catch (const enterprise::LicenseException& e) {
    spdlog::error("License error in GPU Impact Analysis: {}", e.what());
    throw APIException(403, "INVALID_LICENSE", "GPU Impact Analysis license invalid or expired");
    
} catch (const enterprise::GPUException& e) {
    spdlog::warn("GPU error, falling back to CPU: {}", e.what());
    // Retry with CPU backend
    options.gpu_enabled = false;
    return plugin->analyzeImpact(change, graph, options);
    
} catch (const std::exception& e) {
    spdlog::error("Unexpected error in GPU Impact Analysis: {}", e.what());
    throw APIException(500, "PLUGIN_ERROR", e.what());
}
```

---

## Deployment Guide

### Installation Steps

```bash
# 1. Install ThemisDB Core
./install-themis.sh

# 2. Install GPU Impact Analysis Plugin
cd /opt/themisdb/plugins/enterprise
tar -xzf gpu-impact-analysis-v1.0.0.tar.gz

# 3. Configure plugin
vi gpu_impact_analysis/config.yaml

# 4. Add license key
vi /opt/themisdb/config/licenses.yaml
# Add:
# gpu_impact_analysis:
#   key: "YOUR-LICENSE-KEY"
#   expires: "2025-12-31"

# 5. Restart ThemisDB
systemctl restart themisdb

# 6. Verify plugin loaded
curl http://localhost:8765/api/v1/plugins
# Should show: {"name": "gpu_impact_analysis", "version": "1.0.0", "status": "active"}
```

### Configuration

```yaml
# /opt/themisdb/plugins/enterprise/gpu_impact_analysis/config.yaml

plugin_name: gpu_impact_analysis
version: 1.0.0

gpu:
  enabled: true
  backend: cuda  # Options: cuda, vulkan, hip, opencl, cpu
  device_id: 0
  
algorithms:
  max_iterations: 100
  convergence_threshold: 0.001
  damping_factor: 0.85
  
performance:
  batch_size: 1000
  cache_size_mb: 512
  num_threads: 16
  
fem_metadata:
  auto_extract: true
  config_file: /opt/themisdb/config/fem_edge_type_defaults.yaml
```

---

## Summary

This integration guide provides:

1. ✅ **AQL Function Registration** - 5 functions (GPU_ANALYZE_IMPACT, GPU_MONTE_CARLO_RISK, GPU_DETECT_PATTERNS, GPU_DETECT_ANOMALIES, GPU_WHAT_IF_SCENARIOS)
2. ✅ **REST API Endpoints** - 5 endpoints matching AQL functions
3. ✅ **Plugin Lifecycle** - Load/unload with license validation
4. ✅ **Data Access** - Via GraphIndexManager (REST API pattern)
5. ✅ **Performance Optimization** - Caching + batch processing
6. ✅ **Error Handling** - Graceful fallbacks + recovery
7. ✅ **Deployment** - Step-by-step installation guide

**Key Integration Points:**
- Plugin loaded at ThemisDB startup via PluginManager
- AQL functions registered in FunctionRegistry
- REST endpoints added to AnalyticsHandler
- Graph data accessed via GraphIndexManager
- FEM metadata extracted during document ingestion
- All communication via REST API (no direct RocksDB access)

**Result:** Seamless integration maintaining architectural boundaries while providing enterprise-grade impact analysis capabilities.
