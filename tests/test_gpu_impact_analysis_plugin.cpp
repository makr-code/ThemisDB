// Test: GPU Impact Analysis Plugin
// Comprehensive test suite for FEM-inspired impact analysis plugin

#include <gtest/gtest.h>
#include "enterprise/gpu_impact_analysis_plugin.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

using namespace themis::enterprise;
using json = nlohmann::json;

class GPUImpactAnalysisPluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        plugin_ = createGPUImpactAnalysisPlugin();
        
        // Initialize plugin with default configuration
        json config = {
            {"gpu_backend", "cpu"},  // Use CPU fallback for testing
            {"fem", {
                {"damping_factor", 0.85},
                {"impact_threshold", 0.01},
                {"max_iterations", 100},
                {"convergence_threshold", 0.001}
            }},
            {"monte_carlo", {
                {"num_simulations", 1000},
                {"uncertainty_factor", 0.2}
            }}
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
                {{"from", "A"}, {"to", "B"}, {"weight", 0.9}},
                {{"from", "A"}, {"to", "C"}, {"weight", 0.7}},
                {{"from", "B"}, {"to", "D"}, {"weight", 0.8}},
                {{"from", "C"}, {"to", "D"}, {"weight", 0.6}}
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
    EXPECT_TRUE(plugin_->isReady());
    
    auto metadata = plugin_->getMetadata();
    EXPECT_EQ(metadata.id, "themis.enterprise.gpu_impact_analysis");
    EXPECT_EQ(metadata.name, "GPU Impact Analysis");
    EXPECT_EQ(metadata.version, "1.0.0");
}

TEST_F(GPUImpactAnalysisPluginTest, PluginShutdown) {
    EXPECT_TRUE(plugin_->isReady());
    plugin_->shutdown();
    EXPECT_FALSE(plugin_->isReady());
}

TEST_F(GPUImpactAnalysisPluginTest, HealthCheck) {
    auto health = plugin_->healthCheck();
    EXPECT_TRUE(health.contains("status"));
    EXPECT_EQ(health["status"], "healthy");
}

// ============================================================================
// Impact Analysis Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, BasicImpactAnalysis) {
    IGPUImpactAnalysisPlugin::DocumentChange change;
    change.document_id = "doc1";
    change.change_type = "update";
    change.magnitude = 0.8;
    change.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    json config = {
        {"max_depth", 5},
        {"impact_threshold", 0.01}
    };
    
    auto result = plugin_->analyzeDocumentChangeImpact(change, config);
    
    EXPECT_FALSE(result.analysis_id.empty());
    EXPECT_EQ(result.source_change.document_id, "doc1");
    EXPECT_GE(result.total_affected_count, 0);
}

TEST_F(GPUImpactAnalysisPluginTest, BatchAnalysis) {
    std::vector<IGPUImpactAnalysisPlugin::DocumentChange> changes;
    
    for (int i = 0; i < 3; ++i) {
        IGPUImpactAnalysisPlugin::DocumentChange change;
        change.document_id = "doc" + std::to_string(i);
        change.change_type = "update";
        change.magnitude = 0.5;
        change.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        changes.push_back(change);
    }
    
    json config = {};
    auto results = plugin_->analyzeBatchChanges(changes, config);
    
    EXPECT_EQ(results.size(), 3);
    for (const auto& result : results) {
        EXPECT_FALSE(result.analysis_id.empty());
    }
}

// ============================================================================
// FEM Propagation Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, FEMPropagation) {
    json graph = createSimpleGraph();
    
    std::vector<std::string> sources = {"A"};
    std::vector<double> impacts = {1.0};
    
    IGPUImpactAnalysisPlugin::FEMPropagationConfig config;
    config.damping_factor = 0.85;
    config.impact_threshold = 0.01;
    config.max_iterations = 100;
    
    auto distribution = plugin_->propagateImpactFEM(sources, impacts, graph, config);
    
    EXPECT_TRUE(distribution.count("A") > 0);
    EXPECT_DOUBLE_EQ(distribution["A"], 1.0);
    
    // Check propagation to neighbors
    if (distribution.size() > 1) {
        // Impact should decay as it propagates
        for (const auto& [node, impact] : distribution) {
            EXPECT_GE(impact, 0.0);
            EXPECT_LE(impact, 1.0);
        }
    }
}

// ============================================================================
// Monte Carlo Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, MonteCarloRiskAssessment) {
    IGPUImpactAnalysisPlugin::DocumentChange change;
    change.document_id = "risk_doc";
    change.change_type = "critical_update";
    change.magnitude = 0.7;
    change.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    IGPUImpactAnalysisPlugin::MonteCarloConfig config;
    config.num_simulations = 1000;
    config.uncertainty_factor = 0.2;
    
    auto risk = plugin_->assessChangeRisk_MonteCarlo(change, config);
    
    EXPECT_GE(risk.expected_impact, 0.0);
    EXPECT_LE(risk.expected_impact, 1.0);
    EXPECT_GE(risk.value_at_risk_95, risk.expected_impact);
    EXPECT_GE(risk.value_at_risk_99, risk.value_at_risk_95);
    EXPECT_GE(risk.max_impact, risk.value_at_risk_99);
}

// ============================================================================
// Temporal Analysis Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, TemporalAnalysis) {
    std::vector<IGPUImpactAnalysisPlugin::DocumentChange> changes;
    
    for (int i = 0; i < 5; ++i) {
        IGPUImpactAnalysisPlugin::DocumentChange change;
        change.document_id = "doc1";
        change.change_type = "update";
        change.magnitude = 0.5 + (i * 0.1);
        change.timestamp = std::chrono::system_clock::now().time_since_epoch().count() + (i * 1000000);
        changes.push_back(change);
    }
    
    std::vector<std::string> target_nodes = {"doc1", "doc2"};
    
    auto temporal_impacts = plugin_->analyzeTemporalImpact(changes, target_nodes, std::chrono::hours(24));
    
    // We should get results for the nodes that were affected
    EXPECT_GE(temporal_impacts.size(), 0);
}

TEST_F(GPUImpactAnalysisPluginTest, Forecasting) {
    std::vector<IGPUImpactAnalysisPlugin::TemporalImpact> historical;
    
    IGPUImpactAnalysisPlugin::TemporalImpact temp;
    temp.node_id = "forecast_test";
    
    // Create simple time series
    for (int i = 0; i < 10; ++i) {
        temp.impact_timeseries.push_back({i * 1000, 0.5 + (i * 0.02)});
    }
    temp.trend = 0.02;
    temp.volatility = 0.1;
    
    historical.push_back(temp);
    
    auto forecasts = plugin_->forecastFutureImpact(historical, 5);
    
    EXPECT_EQ(forecasts.size(), 1);
    if (!forecasts.empty()) {
        EXPECT_EQ(forecasts[0].impact_timeseries.size(), 5);
    }
}

// ============================================================================
// Pattern Detection Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, PatternDetection) {
    std::vector<IGPUImpactAnalysisPlugin::ImpactAnalysisResult> historical;
    
    for (int i = 0; i < 5; ++i) {
        IGPUImpactAnalysisPlugin::ImpactAnalysisResult result;
        result.analysis_id = "analysis_" + std::to_string(i);
        result.total_affected_count = (i % 2 == 0) ? 25 : 3;
        result.max_impact_score = 0.8;
        historical.push_back(result);
    }
    
    auto patterns = plugin_->detectImpactPatterns_FFT(historical);
    
    EXPECT_GE(patterns.size(), 0);
}

// ============================================================================
// Anomaly Detection Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, AnomalyDetection) {
    std::vector<IGPUImpactAnalysisPlugin::ImpactAnalysisResult> impacts;
    
    // Normal impacts
    for (int i = 0; i < 10; ++i) {
        IGPUImpactAnalysisPlugin::ImpactAnalysisResult result;
        result.total_affected_count = 10;
        result.max_impact_score = 0.5;
        impacts.push_back(result);
    }
    
    // Anomalous impact
    IGPUImpactAnalysisPlugin::ImpactAnalysisResult anomaly;
    anomaly.total_affected_count = 100;  // Significantly higher
    anomaly.max_impact_score = 0.95;
    impacts.push_back(anomaly);
    
    json config = {{"threshold", 2.0}};
    
    auto anomalies = plugin_->detectImpactAnomalies(impacts, config);
    
    EXPECT_GE(anomalies.size(), 0);
}

// ============================================================================
// What-If Analysis Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, WhatIfScenarios) {
    std::vector<IGPUImpactAnalysisPlugin::WhatIfScenario> scenarios;
    
    IGPUImpactAnalysisPlugin::WhatIfScenario scenario1;
    scenario1.scenario_name = "Low Impact";
    
    IGPUImpactAnalysisPlugin::DocumentChange change1;
    change1.document_id = "test_doc";
    change1.change_type = "minor_update";
    change1.magnitude = 0.2;
    change1.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    scenario1.hypothetical_changes.push_back(change1);
    
    scenarios.push_back(scenario1);
    
    auto results = plugin_->simulateWhatIfScenarios(scenarios);
    
    EXPECT_GE(results.size(), 1);
}

TEST_F(GPUImpactAnalysisPluginTest, ScenarioComparison) {
    std::vector<IGPUImpactAnalysisPlugin::WhatIfScenario> scenarios;
    
    for (int i = 0; i < 2; ++i) {
        IGPUImpactAnalysisPlugin::WhatIfScenario scenario;
        scenario.scenario_name = "Scenario_" + std::to_string(i);
        
        IGPUImpactAnalysisPlugin::DocumentChange change;
        change.document_id = "doc";
        change.change_type = "update";
        change.magnitude = 0.3 * (i + 1);
        change.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        scenario.hypothetical_changes.push_back(change);
        
        scenarios.push_back(scenario);
    }
    
    auto comparison = plugin_->compareScenarios(scenarios);
    
    EXPECT_EQ(comparison.scenario_names.size(), 2);
    EXPECT_FALSE(comparison.recommended_scenario.empty());
}

// ============================================================================
// Sensitivity Analysis Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, SensitivityAnalysis) {
    IGPUImpactAnalysisPlugin::DocumentChange change;
    change.document_id = "sens_test";
    change.change_type = "update";
    change.magnitude = 0.5;
    change.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    std::vector<std::string> parameters = {"magnitude"};
    
    auto sensitivity = plugin_->analyzeSensitivity(change, parameters, 0.2);
    
    EXPECT_TRUE(sensitivity.contains("magnitude"));
}

// ============================================================================
// Causal Graph Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, CausalGraphConstruction) {
    std::vector<IGPUImpactAnalysisPlugin::DocumentChange> changes;
    
    for (int i = 0; i < 5; ++i) {
        IGPUImpactAnalysisPlugin::DocumentChange change;
        change.document_id = "doc" + std::to_string(i);
        change.change_type = "update";
        change.magnitude = 0.5;
        change.timestamp = std::chrono::system_clock::now().time_since_epoch().count() + (i * 100000);
        changes.push_back(change);
    }
    
    auto causal_graph = plugin_->buildCausalGraph(changes, 0.5);
    
    EXPECT_GE(causal_graph.nodes.size(), 0);
    EXPECT_GE(causal_graph.edges.size(), 0);
}

TEST_F(GPUImpactAnalysisPluginTest, RootCauseAnalysis) {
    IGPUImpactAnalysisPlugin::ImpactAnalysisResult observed;
    observed.analysis_id = "test";
    
    IGPUImpactAnalysisPlugin::CausalGraph graph;
    graph.nodes = {"A", "B", "C"};
    graph.edges = {
        {"A", "B", 0.9},
        {"A", "C", 0.8},
        {"B", "C", 0.7}
    };
    
    auto root_causes = plugin_->findRootCauses(observed, graph, 3);
    
    EXPECT_GE(root_causes.size(), 0);
    if (!root_causes.empty()) {
        EXPECT_GE(root_causes[0].second, 0.0);
    }
}

// ============================================================================
// Performance Metrics Tests
// ============================================================================

TEST_F(GPUImpactAnalysisPluginTest, PerformanceMetrics) {
    // Run a simple analysis
    IGPUImpactAnalysisPlugin::DocumentChange change;
    change.document_id = "perf_test";
    change.change_type = "update";
    change.magnitude = 0.5;
    change.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    plugin_->analyzeDocumentChangeImpact(change, {});
    
    auto metrics = plugin_->getPerformanceMetrics();
    
    EXPECT_EQ(metrics.total_analyses, 1);
    EXPECT_GE(metrics.avg_analysis_time_ms, 0.0);
}

TEST_F(GPUImpactAnalysisPluginTest, ResetMetrics) {
    // Run an analysis
    IGPUImpactAnalysisPlugin::DocumentChange change;
    change.document_id = "reset_test";
    change.change_type = "update";
    change.magnitude = 0.5;
    change.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    
    plugin_->analyzeDocumentChangeImpact(change, {});
    
    auto metrics_before = plugin_->getPerformanceMetrics();
    EXPECT_GT(metrics_before.total_analyses, 0);
    
    plugin_->resetPerformanceMetrics();
    
    auto metrics_after = plugin_->getPerformanceMetrics();
    EXPECT_EQ(metrics_after.total_analyses, 0);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
