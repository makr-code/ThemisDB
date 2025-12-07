// Benchmark: GPU Impact Analysis Plugin Performance
// Measures performance of FEM-inspired impact analysis algorithms
// Compares CPU vs GPU backends for various graph sizes and scenarios

#include "enterprise/gpu_impact_analysis_plugin.h"
#include <benchmark/benchmark.h>
#include <nlohmann/json.hpp>
#include <random>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

using namespace themis::enterprise;
using json = nlohmann::json;

// ============================================================================
// Test Data Generation
// ============================================================================

class GraphGenerator {
public:
    static json generateRandomGraph(size_t num_nodes, size_t num_edges, int seed = 42) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> weight_dist(0.3, 1.0);
        std::uniform_int_distribution<size_t> node_dist(0, num_nodes - 1);
        
        json graph;
        graph["nodes"] = json::array();
        graph["edges"] = json::array();
        
        // Generate nodes
        for (size_t i = 0; i < num_nodes; i++) {
            graph["nodes"].push_back({
                {"id", "node_" + std::to_string(i)},
                {"type", "document"},
                {"inertia", weight_dist(rng) * 0.5},
                {"change_amplification", 1.0 + weight_dist(rng) * 0.5}
            });
        }
        
        // Generate edges
        std::set<std::pair<size_t, size_t>> edge_set;
        while (edge_set.size() < num_edges) {
            size_t from = node_dist(rng);
            size_t to = node_dist(rng);
            if (from != to) {
                edge_set.insert({from, to});
            }
        }
        
        std::vector<std::string> edge_types = {"DEPENDS_ON", "REFERENCES", "SIMILAR_TO", "DERIVED_FROM"};
        std::uniform_int_distribution<size_t> type_dist(0, edge_types.size() - 1);
        
        for (const auto& [from, to] : edge_set) {
            graph["edges"].push_back({
                {"from", "node_" + std::to_string(from)},
                {"to", "node_" + std::to_string(to)},
                {"weight", weight_dist(rng)},
                {"type", edge_types[type_dist(rng)]},
                {"damping_coefficient", 1.0 - weight_dist(rng)}
            });
        }
        
        return graph;
    }
    
    static json generateLegalGraph(size_t num_laws, size_t num_systems, int seed = 42) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> weight_dist(0.8, 0.99);
        
        json graph;
        graph["nodes"] = json::array();
        graph["edges"] = json::array();
        
        // Court rulings (highest inertia)
        for (size_t i = 0; i < num_laws / 10; i++) {
            graph["nodes"].push_back({
                {"id", "ruling_" + std::to_string(i)},
                {"type", "court_ruling"},
                {"inertia", 0.99}
            });
        }
        
        // Laws
        for (size_t i = 0; i < num_laws; i++) {
            graph["nodes"].push_back({
                {"id", "law_" + std::to_string(i)},
                {"type", "law"},
                {"inertia", 0.95}
            });
        }
        
        // IT Systems (lowest inertia)
        for (size_t i = 0; i < num_systems; i++) {
            graph["nodes"].push_back({
                {"id", "system_" + std::to_string(i)},
                {"type", "it_system"},
                {"inertia", 0.30}
            });
        }
        
        // Connect rulings to laws
        for (size_t i = 0; i < num_laws / 10; i++) {
            size_t law_idx = rng() % num_laws;
            graph["edges"].push_back({
                {"from", "ruling_" + std::to_string(i)},
                {"to", "law_" + std::to_string(law_idx)},
                {"weight", 0.99},
                {"type", "VERWIRFT"},
                {"damping_coefficient", 0.01}
            });
        }
        
        // Connect laws to systems
        for (size_t i = 0; i < num_systems; i++) {
            size_t law_idx = rng() % num_laws;
            graph["edges"].push_back({
                {"from", "law_" + std::to_string(law_idx)},
                {"to", "system_" + std::to_string(i)},
                {"weight", weight_dist(rng)},
                {"type", "LEGAL_BASIS"},
                {"damping_coefficient", 1.0 - weight_dist(rng)}
            });
        }
        
        return graph;
    }
};

// ============================================================================
// Plugin Setup Helper
// ============================================================================

class PluginFixture {
public:
    static std::unique_ptr<IGPUImpactAnalysisPlugin> createPlugin(bool gpu_enabled = false) {
        auto plugin = createGPUImpactAnalysisPlugin();
        
        json config = {
            {"plugin_name", "gpu_impact_analysis"},
            {"version", "1.0.0"},
            {"gpu_enabled", gpu_enabled},
            {"gpu_backend", gpu_enabled ? "cuda" : "cpu"},
            {"max_iterations", 100},
            {"convergence_threshold", 0.001},
            {"damping_factor", 0.85},
            {"license_key", "benchmark-license-key"}
        };
        
        if (!plugin->initialize(config)) {
            return nullptr;
        }
        
        return plugin;
    }
};

// ============================================================================
// Basic Impact Analysis Benchmarks
// ============================================================================

static void BM_ImpactAnalysis_SmallGraph(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    json graph = GraphGenerator::generateRandomGraph(10, 20);
    
    DocumentChange change;
    change.document_id = "node_0";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    options.max_depth = 10;
    options.impact_threshold = 0.01;
    
    for (auto _ : state) {
        json result = plugin->analyzeImpact(change, graph, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 10);  // 10 nodes
}
BENCHMARK(BM_ImpactAnalysis_SmallGraph);

static void BM_ImpactAnalysis_MediumGraph(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    const size_t num_nodes = state.range(0);
    const size_t num_edges = num_nodes * 2;
    
    json graph = GraphGenerator::generateRandomGraph(num_nodes, num_edges);
    
    DocumentChange change;
    change.document_id = "node_0";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    options.max_depth = 10;
    options.impact_threshold = 0.01;
    
    for (auto _ : state) {
        json result = plugin->analyzeImpact(change, graph, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * num_nodes);
}
BENCHMARK(BM_ImpactAnalysis_MediumGraph)->Range(100, 1000);

static void BM_ImpactAnalysis_LargeGraph(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    const size_t num_nodes = state.range(0);
    const size_t num_edges = num_nodes * 3;
    
    json graph = GraphGenerator::generateRandomGraph(num_nodes, num_edges);
    
    DocumentChange change;
    change.document_id = "node_0";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    options.max_depth = 20;
    options.impact_threshold = 0.01;
    
    for (auto _ : state) {
        json result = plugin->analyzeImpact(change, graph, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * num_nodes);
}
BENCHMARK(BM_ImpactAnalysis_LargeGraph)->Range(1000, 10000);

// ============================================================================
// FEM-Specific Benchmarks
// ============================================================================

static void BM_ImpactAnalysis_WithFEM(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    const size_t num_nodes = state.range(0);
    json graph = GraphGenerator::generateRandomGraph(num_nodes, num_nodes * 2);
    
    DocumentChange change;
    change.document_id = "node_0";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    options.max_depth = 10;
    options.impact_threshold = 0.01;
    options.use_fem_metadata = true;
    
    for (auto _ : state) {
        json result = plugin->analyzeImpact(change, graph, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * num_nodes);
}
BENCHMARK(BM_ImpactAnalysis_WithFEM)->Range(100, 1000);

static void BM_ImpactAnalysis_WithoutFEM(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    const size_t num_nodes = state.range(0);
    json graph = GraphGenerator::generateRandomGraph(num_nodes, num_nodes * 2);
    
    DocumentChange change;
    change.document_id = "node_0";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    options.max_depth = 10;
    options.impact_threshold = 0.01;
    options.use_fem_metadata = false;
    
    for (auto _ : state) {
        json result = plugin->analyzeImpact(change, graph, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * num_nodes);
}
BENCHMARK(BM_ImpactAnalysis_WithoutFEM)->Range(100, 1000);

// ============================================================================
// Monte Carlo Risk Benchmarks
// ============================================================================

static void BM_MonteCarloRisk_1K_Simulations(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    json graph = GraphGenerator::generateRandomGraph(50, 100);
    
    DocumentChange change;
    change.document_id = "node_0";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    RiskOptions options;
    options.num_simulations = 1000;
    options.confidence_level = 0.95;
    
    for (auto _ : state) {
        json result = plugin->performMonteCarloRisk(change, graph, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_MonteCarloRisk_1K_Simulations);

static void BM_MonteCarloRisk_10K_Simulations(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    json graph = GraphGenerator::generateRandomGraph(50, 100);
    
    DocumentChange change;
    change.document_id = "node_0";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    RiskOptions options;
    options.num_simulations = 10000;
    options.confidence_level = 0.95;
    
    for (auto _ : state) {
        json result = plugin->performMonteCarloRisk(change, graph, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_MonteCarloRisk_10K_Simulations);

static void BM_MonteCarloRisk_100K_Simulations(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    json graph = GraphGenerator::generateRandomGraph(50, 100);
    
    DocumentChange change;
    change.document_id = "node_0";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    RiskOptions options;
    options.num_simulations = 100000;
    options.confidence_level = 0.95;
    
    for (auto _ : state) {
        json result = plugin->performMonteCarloRisk(change, graph, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 100000);
}
BENCHMARK(BM_MonteCarloRisk_100K_Simulations);

// ============================================================================
// Legal Scenario Benchmarks
// ============================================================================

static void BM_LegalScenario_CourtRuling(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    const size_t num_laws = state.range(0);
    const size_t num_systems = num_laws * 10;
    
    json graph = GraphGenerator::generateLegalGraph(num_laws, num_systems);
    
    DocumentChange change;
    change.document_id = "ruling_0";
    change.change_type = "court_ruling";
    change.magnitude = 0.99;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    options.max_depth = 20;
    options.impact_threshold = 0.01;
    options.use_fem_metadata = true;
    
    for (auto _ : state) {
        json result = plugin->analyzeImpact(change, graph, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * (num_laws + num_systems));
}
BENCHMARK(BM_LegalScenario_CourtRuling)->Range(10, 100);

static void BM_LegalScenario_MonteCarloRisk(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    json graph = GraphGenerator::generateLegalGraph(50, 500);
    
    DocumentChange change;
    change.document_id = "ruling_0";
    change.change_type = "court_ruling";
    change.magnitude = 0.99;
    change.timestamp = std::chrono::system_clock::now();
    
    RiskOptions options;
    options.num_simulations = state.range(0);
    options.confidence_level = 0.99;
    
    for (auto _ : state) {
        json result = plugin->performMonteCarloRisk(change, graph, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
}
BENCHMARK(BM_LegalScenario_MonteCarloRisk)->Range(1000, 100000);

// ============================================================================
// Temporal Analysis Benchmarks
// ============================================================================

static void BM_TemporalAnalysis_ShortHistory(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    json graph = GraphGenerator::generateRandomGraph(50, 100);
    
    std::vector<json> historical_results;
    for (int i = 0; i < 30; i++) {
        historical_results.push_back({
            {"timestamp", i},
            {"total_impact", 1.0 + 0.1 * std::sin(i * 0.3)},
            {"affected_count", 10}
        });
    }
    
    TemporalOptions options;
    options.forecast_periods = 10;
    options.include_trend = true;
    
    for (auto _ : state) {
        json result = plugin->analyzeTemporalImpact(graph, historical_results, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 30);
}
BENCHMARK(BM_TemporalAnalysis_ShortHistory);

static void BM_TemporalAnalysis_LongHistory(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    json graph = GraphGenerator::generateRandomGraph(50, 100);
    
    const size_t history_length = state.range(0);
    std::vector<json> historical_results;
    for (size_t i = 0; i < history_length; i++) {
        historical_results.push_back({
            {"timestamp", i},
            {"total_impact", 1.0 + 0.1 * std::sin(i * 0.1)},
            {"affected_count", 10 + static_cast<int>(5 * std::cos(i * 0.1))}
        });
    }
    
    TemporalOptions options;
    options.forecast_periods = 20;
    options.include_trend = true;
    
    for (auto _ : state) {
        json result = plugin->analyzeTemporalImpact(graph, historical_results, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * history_length);
}
BENCHMARK(BM_TemporalAnalysis_LongHistory)->Range(100, 1000);

// ============================================================================
// Pattern Detection Benchmarks
// ============================================================================

static void BM_PatternDetection(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    const size_t history_length = state.range(0);
    std::vector<json> historical_results;
    for (size_t i = 0; i < history_length; i++) {
        historical_results.push_back({
            {"timestamp", i},
            {"total_impact", 1.0 + 0.5 * std::sin(i * 0.1)},
            {"affected_count", 10}
        });
    }
    
    for (auto _ : state) {
        json result = plugin->detectPatterns(historical_results);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * history_length);
}
BENCHMARK(BM_PatternDetection)->Range(100, 10000);

// ============================================================================
// Anomaly Detection Benchmarks
// ============================================================================

static void BM_AnomalyDetection(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    const size_t history_length = state.range(0);
    std::vector<json> historical_results;
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 0.2);
    
    for (size_t i = 0; i < history_length; i++) {
        double impact = 1.0 + dist(rng);
        if (i % 100 == 99) {  // Add anomalies
            impact = 5.0 + dist(rng);
        }
        
        historical_results.push_back({
            {"timestamp", i},
            {"total_impact", impact},
            {"affected_count", impact < 2.0 ? 10 : 50}
        });
    }
    
    for (auto _ : state) {
        json result = plugin->detectAnomalies(historical_results);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * history_length);
}
BENCHMARK(BM_AnomalyDetection)->Range(100, 10000);

// ============================================================================
// What-If Scenario Benchmarks
// ============================================================================

static void BM_WhatIfScenarios(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    json graph = GraphGenerator::generateRandomGraph(100, 200);
    
    const size_t num_scenarios = state.range(0);
    std::vector<DocumentChange> scenarios;
    
    for (size_t i = 0; i < num_scenarios; i++) {
        DocumentChange change;
        change.document_id = "node_0";
        change.change_type = "modification";
        change.magnitude = 0.5 + (i * 0.5 / num_scenarios);
        change.timestamp = std::chrono::system_clock::now();
        scenarios.push_back(change);
    }
    
    AnalysisOptions options;
    options.max_depth = 10;
    
    for (auto _ : state) {
        json result = plugin->analyzeWhatIfScenarios(graph, scenarios, options);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * num_scenarios);
}
BENCHMARK(BM_WhatIfScenarios)->Range(2, 20);

// ============================================================================
// Sensitivity Analysis Benchmarks
// ============================================================================

static void BM_SensitivityAnalysis(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    json graph = GraphGenerator::generateRandomGraph(100, 200);
    
    DocumentChange change;
    change.document_id = "node_0";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    std::vector<std::string> parameters = {"damping_factor", "max_depth", "impact_threshold"};
    
    for (auto _ : state) {
        json result = plugin->performSensitivityAnalysis(change, graph, parameters);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * parameters.size());
}
BENCHMARK(BM_SensitivityAnalysis);

// ============================================================================
// Root Cause Analysis Benchmarks
// ============================================================================

static void BM_RootCauseAnalysis(benchmark::State& state) {
    auto plugin = PluginFixture::createPlugin(false);
    if (!plugin) {
        state.SkipWithError("Failed to initialize plugin");
        return;
    }
    
    json graph = GraphGenerator::generateRandomGraph(100, 200);
    
    const size_t num_changes = state.range(0);
    std::vector<DocumentChange> changes;
    
    auto now = std::chrono::system_clock::now();
    for (size_t i = 0; i < num_changes; i++) {
        DocumentChange change;
        change.document_id = "node_" + std::to_string(i % 10);
        change.change_type = "modification";
        change.magnitude = 0.5 + (i * 0.5 / num_changes);
        change.timestamp = now - std::chrono::hours(num_changes - i);
        changes.push_back(change);
    }
    
    std::string target_node = "node_50";
    
    for (auto _ : state) {
        json result = plugin->performRootCauseAnalysis(graph, changes, target_node);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * num_changes);
}
BENCHMARK(BM_RootCauseAnalysis)->Range(5, 50);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
