/**
 * @file test_prompt_engineering_metrics.cpp
 * @brief Unit tests for PromptEngineeringMetrics
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_engineering_metrics.h"
#include <string>

using namespace themis::prompt_engineering;

class PromptEngineeringMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        PromptEngineeringMetrics::Config config;
        config.enabled = true;
        metrics = std::make_unique<PromptEngineeringMetrics>(config);
    }

    void TearDown() override {
        metrics.reset();
    }

    std::unique_ptr<PromptEngineeringMetrics> metrics;
};

TEST_F(PromptEngineeringMetricsTest, RecordOptimizationMetrics) {
    // Record some optimization attempts
    metrics->recordOptimizationAttempt("prompt1");
    metrics->recordOptimizationSuccess("prompt1", 0.15);
    metrics->recordOptimizationDuration("prompt1", 125.5);
    metrics->recordOptimizationIterations("prompt1", 3);

    // Export and verify metrics are present
    std::string exported = metrics->exportMetrics();
    
    EXPECT_TRUE(exported.find("themis_prompt_engineering_optimization_attempts_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_optimization_successes_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_optimization_duration_ms_avg") != std::string::npos);
}

TEST_F(PromptEngineeringMetricsTest, RecordABTestMetrics) {
    // Record A/B test operations
    metrics->recordABTestStart("test1", "prompt1");
    metrics->recordABTestObservation("test1", "a", true);
    metrics->recordABTestObservation("test1", "b", true);
    metrics->recordActiveABTests(1);

    std::string exported = metrics->exportMetrics();
    
    EXPECT_TRUE(exported.find("themis_prompt_engineering_ab_test_starts_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_ab_test_observations_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_ab_tests_active") != std::string::npos);
}

TEST_F(PromptEngineeringMetricsTest, RecordPerformanceMetrics) {
    // Record prompt executions
    metrics->recordPromptExecution("prompt1", true, 120.5);
    metrics->recordPromptExecution("prompt1", true, 110.3);
    metrics->recordPromptExecution("prompt1", false, 150.0);

    std::string exported = metrics->exportMetrics();
    
    EXPECT_TRUE(exported.find("themis_prompt_engineering_prompt_executions_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_prompt_success_rate") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_prompt_latency_ms_avg") != std::string::npos);
}

TEST_F(PromptEngineeringMetricsTest, RecordFeedbackMetrics) {
    // Record feedback
    metrics->recordFeedback("prompt1", "USER_POSITIVE");
    metrics->recordFeedback("prompt1", "USER_POSITIVE");
    metrics->recordFeedback("prompt1", "USER_NEGATIVE");
    metrics->recordHallucinationDetection("prompt1");

    std::string exported = metrics->exportMetrics();
    
    EXPECT_TRUE(exported.find("themis_prompt_engineering_feedback_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_feedback_positive_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_hallucination_detections_total") != std::string::npos);
}

TEST_F(PromptEngineeringMetricsTest, RecordVersionControlMetrics) {
    // Record version control operations
    metrics->recordVersionCommit("prompt1", "main");
    metrics->recordBranchCreation("prompt1");
    metrics->recordMergeOperation("prompt1", "auto", true);
    metrics->recordVersionRollback("prompt1");

    std::string exported = metrics->exportMetrics();
    
    EXPECT_TRUE(exported.find("themis_prompt_engineering_version_commits_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_branch_creations_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_merge_operations_total") != std::string::npos);
}

TEST_F(PromptEngineeringMetricsTest, RecordIntegrationMetrics) {
    // Record integration operations
    metrics->recordIntegrationExecution(true);  // before
    metrics->recordIntegrationExecution(false); // after
    metrics->recordBackgroundWorkerCycle();
    metrics->recordBackgroundWorkerDuration(250.0);

    std::string exported = metrics->exportMetrics();
    
    EXPECT_TRUE(exported.find("themis_prompt_engineering_integration_before_calls_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_integration_after_calls_total") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_background_worker_cycles_total") != std::string::npos);
}

TEST_F(PromptEngineeringMetricsTest, ResetMetrics) {
    // Record some metrics
    metrics->recordOptimizationAttempt("prompt1");
    metrics->recordPromptExecution("prompt1", true, 100.0);

    // Reset
    metrics->reset();

    // Export should show zeros
    std::string exported = metrics->exportMetrics();
    EXPECT_TRUE(exported.find("themis_prompt_engineering_optimization_attempts_total 0") != std::string::npos);
    EXPECT_TRUE(exported.find("themis_prompt_engineering_prompt_executions_total 0") != std::string::npos);
}

TEST_F(PromptEngineeringMetricsTest, PrometheusFormat) {
    metrics->recordOptimizationAttempt("prompt1");
    
    std::string exported = metrics->exportMetrics();
    
    // Check Prometheus format
    EXPECT_TRUE(exported.find("# HELP") != std::string::npos);
    EXPECT_TRUE(exported.find("# TYPE") != std::string::npos);
    EXPECT_TRUE(exported.find("counter") != std::string::npos);
    EXPECT_TRUE(exported.find("gauge") != std::string::npos);
}

TEST_F(PromptEngineeringMetricsTest, DisabledMetrics) {
    // Create metrics with disabled config
    PromptEngineeringMetrics::Config config;
    config.enabled = false;
    auto disabled_metrics = std::make_unique<PromptEngineeringMetrics>(config);

    // Record metrics (should be no-op)
    disabled_metrics->recordOptimizationAttempt("prompt1");

    // Export should still work but with zeros
    std::string exported = disabled_metrics->exportMetrics();
    EXPECT_TRUE(exported.find("themis_prompt_engineering_optimization_attempts_total 0") != std::string::npos);
}
