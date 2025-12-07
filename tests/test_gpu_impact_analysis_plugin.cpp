// Test: GPU Impact Analysis Plugin
// Comprehensive test suite for FEM-inspired impact analysis plugin

#include <gtest/gtest.h>
#include "enterprise/gpu_impact_analysis_plugin.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <memory>

using namespace themis::enterprise;
using json = nlohmann::json;

class GPUImpactAnalysisPluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        plugin_ = createGPUImpactAnalysisPlugin();
        
        // Initialize plugin with default configuration
        json config = {
            {"plugin_name", "gpu_impact_analysis"},
            {"version", "1.0.0"},
            {"gpu_enabled", false},  // Use CPU fallback for testing
            {"gpu_backend", "cpu"},
            {"max_iterations", 100},
            {"convergence_threshold", 0.001},
            {"damping_factor", 0.85},
            {"license_key", "test-license-key"}
        };
        
        ASSERT_TRUE(plugin_->initialize(config));
    }
    
    void TearDown() override {
        if (plugin_) {
            plugin_->shutdown();
        }
    }
    
    // Helper: Create simple graph structure
    json createSimpleGraph() {
        json graph = {
            {"nodes", {
                {{"id", "A"}, {"type", "document"}},
                {{"id", "B"}, {"type", "document"}},
                {{"id", "C"}, {"type", "document"}},
                {{"id", "D"}, {"type", "document"}}
            }},
            {"edges", {
                {{"from", "A"}, {"to", "B"}, {"weight", 0.9}, {"type", "DEPENDS_ON"}},
                {{"from", "A"}, {"to", "C"}, {"weight", 0.7}, {"type", "REFERENCES"}},
                {{"from", "B"}, {"to", "D"}, {"weight", 0.8}, {"type", "DEPENDS_ON"}},
                {{"from", "C"}, {"to", "D"}, {"weight", 0.6}, {"type", "REFERENCES"}}
            }}
        };
        return graph;
    }
    
    // Helper: Create complex graph structure (legal scenario)
    json createLegalGraph() {
        json graph = {
            {"nodes", {
                {{"id", "BVerfG_1_BvR_2_24"}, {"type", "court_ruling"}, {"inertia", 0.99}},
                {{"id", "SGB_II_Para_44a"}, {"type", "law"}, {"inertia", 0.95}},
                {{"id", "AI_System_JobMatch"}, {"type", "it_system"}, {"inertia", 0.30}},
                {{"id", "AI_System_TaxRisk"}, {"type", "it_system"}, {"inertia", 0.30}},
                {{"id", "Regulation_123"}, {"type", "regulation"}, {"inertia", 0.85}}
            }},
            {"edges", {
                {{"from", "BVerfG_1_BvR_2_24"}, {"to", "SGB_II_Para_44a"}, {"weight", 0.99}, {"type", "VERWIRFT"}},
                {{"from", "SGB_II_Para_44a"}, {"to", "AI_System_JobMatch"}, {"weight", 0.95}, {"type", "LEGAL_BASIS"}},
                {{"from", "SGB_II_Para_44a"}, {"to", "Regulation_123"}, {"weight", 0.90}, {"type", "IMPLEMENTED_BY"}},
                {{"from", "Regulation_123"}, {"to", "AI_System_TaxRisk"}, {"weight", 0.88}, {"type", "APPLIES_TO"}}
            }}
        };
        return graph;
    }
    
    std::unique_ptr<IGPUImpactAnalysisPlugin> plugin_;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, PluginInitialization) {
    EXPECT_TRUE(plugin_->isInitialized());
    EXPECT_EQ(plugin_->getName(), "gpu_impact_analysis");
    EXPECT_EQ(plugin_->getVersion(), "1.0.0");
}

TEST_F(GPUImpactAnalysisPluginTest, PluginShutdown) {
    EXPECT_TRUE(plugin_->isInitialized());
    plugin_->shutdown();
    EXPECT_FALSE(plugin_->isInitialized());
}

// ============================================================================
// Impact Analysis Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, BasicImpactAnalysis) {
    json graph = createSimpleGraph();
    
    DocumentChange change;
    change.document_id = "A";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    options.max_depth = 10;
    options.impact_threshold = 0.01;
    options.use_temporal_decay = false;
    
    json result = plugin_->analyzeImpact(change, graph, options);
    
    ASSERT_TRUE(result.contains("affected_nodes"));
    ASSERT_TRUE(result.is_object());
    
    auto affected_nodes = result["affected_nodes"];
    ASSERT_TRUE(affected_nodes.is_array());
    EXPECT_GT(affected_nodes.size(), 0);
    
    // Verify node A has highest impact
    bool found_a = false;
    for (const auto& node : affected_nodes) {
        if (node["node_id"] == "A") {
            EXPECT_NEAR(node["impact_score"].get<double>(), 1.0, 0.01);
            found_a = true;
        }
    }
    EXPECT_TRUE(found_a);
}

TEST_F(GPUImpactAnalysisPluginTest, ImpactPropagation) {
    json graph = createSimpleGraph();
    
    DocumentChange change;
    change.document_id = "A";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    options.max_depth = 10;
    options.impact_threshold = 0.01;
    
    json result = plugin_->analyzeImpact(change, graph, options);
    auto affected_nodes = result["affected_nodes"];
    
    // Check that impact propagates to B, C, and D
    std::unordered_map<std::string, double> impacts;
    for (const auto& node : affected_nodes) {
        impacts[node["node_id"]] = node["impact_score"];
    }
    
    EXPECT_TRUE(impacts.find("A") != impacts.end());
    EXPECT_TRUE(impacts.find("B") != impacts.end());
    EXPECT_TRUE(impacts.find("C") != impacts.end());
    EXPECT_TRUE(impacts.find("D") != impacts.end());
    
    // Verify decay: A > B > D (through strong edge 0.9 * 0.8)
    EXPECT_GT(impacts["A"], impacts["B"]);
    EXPECT_GT(impacts["B"], impacts["D"]);
}

TEST_F(GPUImpactAnalysisPluginTest, LegalGraphImpact) {
    json graph = createLegalGraph();
    
    DocumentChange change;
    change.document_id = "BVerfG_1_BvR_2_24";
    change.change_type = "court_ruling";
    change.magnitude = 0.99;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    options.max_depth = 10;
    options.impact_threshold = 0.01;
    options.use_fem_metadata = true;
    
    json result = plugin_->analyzeImpact(change, graph, options);
    auto affected_nodes = result["affected_nodes"];
    
    // Verify court ruling impacts AI systems
    bool found_jobmatch = false;
    bool found_taxrisk = false;
    
    for (const auto& node : affected_nodes) {
        if (node["node_id"] == "AI_System_JobMatch") {
            found_jobmatch = true;
            EXPECT_GT(node["impact_score"].get<double>(), 0.5);
        }
        if (node["node_id"] == "AI_System_TaxRisk") {
            found_taxrisk = true;
        }
    }
    
    EXPECT_TRUE(found_jobmatch);
    EXPECT_TRUE(found_taxrisk);
}

// ============================================================================
// Monte Carlo Risk Analysis Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, MonteCarloRiskBasic) {
    json graph = createSimpleGraph();
    
    DocumentChange change;
    change.document_id = "A";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    RiskOptions options;
    options.num_simulations = 1000;  // Small for testing
    options.confidence_level = 0.95;
    
    json result = plugin_->performMonteCarloRisk(change, graph, options);
    
    ASSERT_TRUE(result.contains("expected_impact"));
    ASSERT_TRUE(result.contains("value_at_risk_95"));
    ASSERT_TRUE(result.contains("value_at_risk_99"));
    
    EXPECT_GT(result["expected_impact"].get<double>(), 0.0);
    EXPECT_GE(result["value_at_risk_95"].get<double>(), result["expected_impact"].get<double>());
}

TEST_F(GPUImpactAnalysisPluginTest, MonteCarloRiskDistribution) {
    json graph = createSimpleGraph();
    
    DocumentChange change;
    change.document_id = "A";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    RiskOptions options;
    options.num_simulations = 10000;
    options.confidence_level = 0.95;
    
    json result = plugin_->performMonteCarloRisk(change, graph, options);
    
    ASSERT_TRUE(result.contains("impact_distribution"));
    auto distribution = result["impact_distribution"];
    ASSERT_TRUE(distribution.is_array());
    EXPECT_GT(distribution.size(), 0);
}

// ============================================================================
// Temporal Analysis Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, TemporalAnalysisBasic) {
    json graph = createSimpleGraph();
    
    // Create historical results (simulated time series)
    std::vector<json> historical_results;
    for (int i = 0; i < 30; i++) {
        json result = {
            {"timestamp", i},
            {"total_impact", 1.0 + 0.1 * std::sin(i * 0.3)},
            {"affected_count", 4}
        };
        historical_results.push_back(result);
    }
    
    TemporalOptions options;
    options.forecast_periods = 10;
    options.include_trend = true;
    
    json result = plugin_->analyzeTemporalImpact(graph, historical_results, options);
    
    ASSERT_TRUE(result.contains("forecast"));
    auto forecast = result["forecast"];
    ASSERT_TRUE(forecast.is_array());
    EXPECT_EQ(forecast.size(), 10);
}

// ============================================================================
// Pattern Detection Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, PatternDetectionBasic) {
    // Create historical results with patterns
    std::vector<json> historical_results;
    for (int i = 0; i < 100; i++) {
        json result = {
            {"timestamp", i},
            {"total_impact", 1.0 + 0.5 * std::sin(i * 0.1)},  // Pattern with period ~60
            {"affected_count", 3 + static_cast<int>(2 * std::cos(i * 0.1))}
        };
        historical_results.push_back(result);
    }
    
    json result = plugin_->detectPatterns(historical_results);
    
    ASSERT_TRUE(result.contains("patterns"));
    auto patterns = result["patterns"];
    ASSERT_TRUE(patterns.is_array());
}

// ============================================================================
// Anomaly Detection Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, AnomalyDetectionBasic) {
    std::vector<json> historical_results;
    
    // Normal data points
    for (int i = 0; i < 90; i++) {
        json result = {
            {"timestamp", i},
            {"total_impact", 1.0 + 0.1 * (std::rand() % 100) / 100.0},
            {"affected_count", 3}
        };
        historical_results.push_back(result);
    }
    
    // Anomalous data points
    for (int i = 90; i < 100; i++) {
        json result = {
            {"timestamp", i},
            {"total_impact", 5.0 + 0.5 * (std::rand() % 100) / 100.0},  // Much higher
            {"affected_count", 15}  // Much higher
        };
        historical_results.push_back(result);
    }
    
    json result = plugin_->detectAnomalies(historical_results);
    
    ASSERT_TRUE(result.contains("anomalies"));
    auto anomalies = result["anomalies"];
    ASSERT_TRUE(anomalies.is_array());
    EXPECT_GT(anomalies.size(), 0);
}

// ============================================================================
// What-If Scenario Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, WhatIfScenarioComparison) {
    json graph = createSimpleGraph();
    
    std::vector<DocumentChange> scenarios;
    
    DocumentChange scenario1;
    scenario1.document_id = "A";
    scenario1.change_type = "modification";
    scenario1.magnitude = 0.5;
    scenario1.timestamp = std::chrono::system_clock::now();
    scenarios.push_back(scenario1);
    
    DocumentChange scenario2;
    scenario2.document_id = "A";
    scenario2.change_type = "modification";
    scenario2.magnitude = 1.0;
    scenario2.timestamp = std::chrono::system_clock::now();
    scenarios.push_back(scenario2);
    
    AnalysisOptions options;
    options.max_depth = 10;
    
    json result = plugin_->analyzeWhatIfScenarios(graph, scenarios, options);
    
    ASSERT_TRUE(result.contains("scenarios"));
    auto scenario_results = result["scenarios"];
    ASSERT_TRUE(scenario_results.is_array());
    EXPECT_EQ(scenario_results.size(), 2);
    
    // Scenario 2 should have higher impact
    EXPECT_GT(
        scenario_results[1]["total_impact"].get<double>(),
        scenario_results[0]["total_impact"].get<double>()
    );
}

// ============================================================================
// Sensitivity Analysis Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, SensitivityAnalysisBasic) {
    json graph = createSimpleGraph();
    
    DocumentChange change;
    change.document_id = "A";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    std::vector<std::string> parameters = {"damping_factor", "max_depth"};
    
    json result = plugin_->performSensitivityAnalysis(change, graph, parameters);
    
    ASSERT_TRUE(result.contains("sensitivity"));
    auto sensitivity = result["sensitivity"];
    ASSERT_TRUE(sensitivity.is_object());
    
    EXPECT_TRUE(sensitivity.contains("damping_factor"));
    EXPECT_TRUE(sensitivity.contains("max_depth"));
}

// ============================================================================
// Root Cause Analysis Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, RootCauseAnalysisBasic) {
    json graph = createSimpleGraph();
    
    // Create multiple changes
    std::vector<DocumentChange> changes;
    
    DocumentChange change1;
    change1.document_id = "A";
    change1.change_type = "modification";
    change1.magnitude = 1.0;
    change1.timestamp = std::chrono::system_clock::now() - std::chrono::hours(2);
    changes.push_back(change1);
    
    DocumentChange change2;
    change2.document_id = "B";
    change2.change_type = "modification";
    change2.magnitude = 0.5;
    change2.timestamp = std::chrono::system_clock::now() - std::chrono::hours(1);
    changes.push_back(change2);
    
    std::string target_node = "D";
    
    json result = plugin_->performRootCauseAnalysis(graph, changes, target_node);
    
    ASSERT_TRUE(result.contains("root_causes"));
    auto root_causes = result["root_causes"];
    ASSERT_TRUE(root_causes.is_array());
}

// ============================================================================
// FEM Metadata Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, FEMMetadataRespected) {
    json graph = createLegalGraph();
    
    DocumentChange change;
    change.document_id = "BVerfG_1_BvR_2_24";
    change.change_type = "court_ruling";
    change.magnitude = 0.99;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options_with_fem;
    options_with_fem.max_depth = 10;
    options_with_fem.use_fem_metadata = true;
    
    AnalysisOptions options_without_fem;
    options_without_fem.max_depth = 10;
    options_without_fem.use_fem_metadata = false;
    
    json result_with_fem = plugin_->analyzeImpact(change, graph, options_with_fem);
    json result_without_fem = plugin_->analyzeImpact(change, graph, options_without_fem);
    
    // Results should differ when FEM metadata is used
    EXPECT_NE(
        result_with_fem["total_impact"].get<double>(),
        result_without_fem["total_impact"].get<double>()
    );
}

// ============================================================================
// Performance and Edge Cases
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, LargeGraphPerformance) {
    // Create larger graph
    json graph;
    graph["nodes"] = json::array();
    graph["edges"] = json::array();
    
    // Create 100 nodes
    for (int i = 0; i < 100; i++) {
        graph["nodes"].push_back({
            {"id", "node_" + std::to_string(i)},
            {"type", "document"}
        });
    }
    
    // Create edges (chain structure)
    for (int i = 0; i < 99; i++) {
        graph["edges"].push_back({
            {"from", "node_" + std::to_string(i)},
            {"to", "node_" + std::to_string(i + 1)},
            {"weight", 0.8},
            {"type", "DEPENDS_ON"}
        });
    }
    
    DocumentChange change;
    change.document_id = "node_0";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    options.max_depth = 50;
    
    auto start = std::chrono::high_resolution_clock::now();
    json result = plugin_->analyzeImpact(change, graph, options);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time (< 1 second for CPU fallback)
    EXPECT_LT(duration.count(), 1000);
    
    ASSERT_TRUE(result.contains("affected_nodes"));
}

TEST_F(GPUImpactAnalysisPluginTest, EmptyGraphHandling) {
    json empty_graph = {
        {"nodes", json::array()},
        {"edges", json::array()}
    };
    
    DocumentChange change;
    change.document_id = "A";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    
    json result = plugin_->analyzeImpact(change, empty_graph, options);
    
    ASSERT_TRUE(result.contains("affected_nodes"));
    EXPECT_EQ(result["affected_nodes"].size(), 0);
}

TEST_F(GPUImpactAnalysisPluginTest, NonExistentNodeHandling) {
    json graph = createSimpleGraph();
    
    DocumentChange change;
    change.document_id = "NonExistent";
    change.change_type = "modification";
    change.magnitude = 1.0;
    change.timestamp = std::chrono::system_clock::now();
    
    AnalysisOptions options;
    
    json result = plugin_->analyzeImpact(change, graph, options);
    
    ASSERT_TRUE(result.contains("affected_nodes"));
    EXPECT_EQ(result["affected_nodes"].size(), 0);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
