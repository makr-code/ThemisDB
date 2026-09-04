/**
 * @file test_continuous_learning_client.cpp
 * @brief Unit tests for Continuous Learning Client
 */

#include <gtest/gtest.h>
#include "rag/continuous_learning_client.h"

using namespace themis::rag::judge;

class ContinuousLearningClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration
        config_.endpoint = "http://localhost:8080/metrics";
        config_.enable_logging = true;
        config_.enable_triggers = true;
        config_.faithfulness_threshold = 0.75;
        config_.relevance_threshold = 0.70;
        config_.overall_quality_threshold = 0.70;
        config_.metric_window_size = 10;
        config_.enable_batching = false;  // Disable for testing
    }
    
    ContinuousLearningClient::Config config_;
    
    QCResult createTestResult(double overall_score) {
        QCResult result;
        result.overall_score = overall_score;
        result.faithfulness_score = overall_score;
        result.relevance_score = overall_score;
        result.completeness_score = overall_score;
        result.coherence_score = overall_score;
        result.decision = overall_score >= 0.75 ? QCDecision::ACCEPT : QCDecision::WARN;
        result.mode = QCMode::BALANCED;
        result.latency = std::chrono::milliseconds(100);
        result.passed_threshold = overall_score >= 0.75;
        return result;
    }
};

// ═══════════════════════════════════════════════════════════
// Constructor Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ContinuousLearningClientTest, DefaultConstructor) {
    ContinuousLearningClient client;
    auto stats = client.getStatistics();
    
    EXPECT_EQ(stats.metrics_logged, 0);
    EXPECT_EQ(stats.metrics_sent, 0);
    EXPECT_EQ(stats.triggers_fired, 0);
}

TEST_F(ContinuousLearningClientTest, ConfigConstructor) {
    ContinuousLearningClient client(config_);
    auto stats = client.getStatistics();
    
    EXPECT_EQ(stats.metrics_logged, 0);
}

// ═══════════════════════════════════════════════════════════
// Metric Logging Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ContinuousLearningClientTest, LogQCResult) {
    ContinuousLearningClient client(config_);
    
    auto result = createTestResult(0.85);
    client.logQCResult(result);
    
    auto stats = client.getStatistics();
    EXPECT_GT(stats.metrics_logged, 0);  // Should log multiple metrics from result
}

TEST_F(ContinuousLearningClientTest, LogMultipleResults) {
    ContinuousLearningClient client(config_);
    
    for (int i = 0; i < 5; i++) {
        auto result = createTestResult(0.80);
        client.logQCResult(result);
    }
    
    auto stats = client.getStatistics();
    EXPECT_GT(stats.metrics_logged, 5);  // Multiple metrics per result
}

TEST_F(ContinuousLearningClientTest, LogIndividualMetric) {
    ContinuousLearningClient client(config_);
    
    QualityMetric metric;
    metric.type = MetricType::FAITHFULNESS;
    metric.value = 0.85;
    metric.timestamp = std::chrono::system_clock::now();
    
    client.logMetric(metric);
    
    auto stats = client.getStatistics();
    EXPECT_EQ(stats.metrics_logged, 1);
}

TEST_F(ContinuousLearningClientTest, LogMetricsBatch) {
    ContinuousLearningClient client(config_);
    
    std::vector<QualityMetric> metrics = {};

    for (int i = 0; i < 3; i++) {
        QualityMetric metric;
        metric.type = MetricType::RELEVANCE;
        metric.value = 0.75 + i * 0.05;
        metric.timestamp = std::chrono::system_clock::now();
        metrics.push_back(metric);
    }
    
    client.logMetricsBatch(metrics);
    
    auto stats = client.getStatistics();
    EXPECT_EQ(stats.metrics_logged, 3);
}

// ═══════════════════════════════════════════════════════════
// Trigger Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ContinuousLearningClientTest, LowFaithfulnessTrigger) {
    ContinuousLearningClient client(config_);
    
    // Log several low faithfulness results
    for (int i = 0; i < config_.metric_window_size; i++) {
        auto result = createTestResult(0.65);  // Below threshold
        client.logQCResult(result);
    }
    
    auto trigger = client.checkTriggers();
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->trigger_type, "low_faithfulness");
    EXPECT_LT(trigger->current_value, config_.faithfulness_threshold);
}

TEST_F(ContinuousLearningClientTest, LowRelevanceTrigger) {
    ContinuousLearningClient client(config_);
    
    // Log results with low relevance specifically
    for (int i = 0; i < config_.metric_window_size; i++) {
        QualityMetric metric;
        metric.type = MetricType::RELEVANCE;
        metric.value = 0.65;  // Below threshold
        metric.timestamp = std::chrono::system_clock::now();
        client.logMetric(metric);
    }
    
    auto trigger = client.checkTriggers();
    ASSERT_NE(trigger, nullptr);
    EXPECT_EQ(trigger->trigger_type, "low_relevance");
    EXPECT_FALSE(trigger->recommendation.empty());
}

TEST_F(ContinuousLearningClientTest, NoTriggerOnGoodQuality) {
    ContinuousLearningClient client(config_);
    
    // Log high quality results
    for (int i = 0; i < config_.metric_window_size; i++) {
        auto result = createTestResult(0.90);  // Above threshold
        client.logQCResult(result);
    }
    
    auto trigger = client.checkTriggers();
    EXPECT_EQ(trigger, nullptr);  // No trigger for good quality
}

TEST_F(ContinuousLearningClientTest, TriggerStatistics) {
    ContinuousLearningClient client(config_);
    
    // Log low quality results to trigger
    for (int i = 0; i < config_.metric_window_size; i++) {
        auto result = createTestResult(0.60);
        client.logQCResult(result);
    }
    
    // Check triggers multiple times
    client.checkTriggers();
    client.checkTriggers();
    
    auto stats = client.getStatistics();
    EXPECT_GT(stats.triggers_fired, 0);
}

// ═══════════════════════════════════════════════════════════
// Callback Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ContinuousLearningClientTest, TriggerCallback) {
    ContinuousLearningClient client(config_);
    
    bool callback_called = false;
    std::string trigger_type;
    
    client.setTriggerCallback([&](const OptimizationTrigger& trigger) {
        callback_called = true;
        trigger_type = trigger.trigger_type;
    });
    
    // Log low quality results
    for (int i = 0; i < config_.metric_window_size; i++) {
        auto result = createTestResult(0.60);
        client.logQCResult(result);
    }
    
    EXPECT_TRUE(callback_called);
    EXPECT_FALSE(trigger_type.empty());
}

// ═══════════════════════════════════════════════════════════
// Utility Function Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ContinuousLearningClientTest, QCResultToMetrics) {
    auto result = createTestResult(0.85);
    auto metrics = cl_utils::qcResultToMetrics(result);
    
    EXPECT_FALSE(metrics.empty());
    
    // Should have metrics for each dimension
    bool has_faithfulness = false;
    bool has_relevance = false;
    bool has_overall = false;
    
    for (const auto& metric : metrics) {
        if (metric.type == MetricType::FAITHFULNESS) {
          has_faithfulness = true;
        }
        if (metric.type == MetricType::RELEVANCE) {
          has_relevance = true;
        }
        if (metric.type == MetricType::OVERALL_QUALITY) {
          has_overall = true;
        }
    }
    
    EXPECT_TRUE(has_faithfulness);
    EXPECT_TRUE(has_relevance);
    EXPECT_TRUE(has_overall);
}

TEST_F(ContinuousLearningClientTest, GenerateRecommendation) {
    auto result = createTestResult(0.60);
    result.faithfulness_score = 0.65;
    result.relevance_score = 0.65;
    
    auto recommendation = cl_utils::generateRecommendation(result);
    
    EXPECT_FALSE(recommendation.empty());
    EXPECT_NE(recommendation.find("retrieval"), std::string::npos);
}

TEST_F(ContinuousLearningClientTest, MetricTypeToString) {
    EXPECT_EQ(cl_utils::metricTypeToString(MetricType::FAITHFULNESS), "faithfulness");
    EXPECT_EQ(cl_utils::metricTypeToString(MetricType::RELEVANCE), "relevance");
    EXPECT_EQ(cl_utils::metricTypeToString(MetricType::OVERALL_QUALITY), "overall_quality");
}

// ═══════════════════════════════════════════════════════════
// Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ContinuousLearningClientTest, DisabledLogging) {
    config_.enable_logging = false;
    ContinuousLearningClient client(config_);
    
    auto result = createTestResult(0.85);
    client.logQCResult(result);
    
    auto stats = client.getStatistics();
    EXPECT_EQ(stats.metrics_logged, 0);  // Logging disabled
}

TEST_F(ContinuousLearningClientTest, DisabledTriggers) {
    config_.enable_triggers = false;
    ContinuousLearningClient client(config_);
    
    // Log low quality results
    for (int i = 0; i < config_.metric_window_size; i++) {
        auto result = createTestResult(0.60);
        client.logQCResult(result);
    }
    
    auto trigger = client.checkTriggers();
    EXPECT_EQ(trigger, nullptr);  // Triggers disabled
}

// ═══════════════════════════════════════════════════════════
// Flush Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ContinuousLearningClientTest, FlushPendingMetrics) {
    config_.enable_batching = true;
    config_.batch_size = 100;  // Large batch
    
    ContinuousLearningClient client(config_);
    
    // Log some metrics
    for (int i = 0; i < 5; i++) {
        auto result = createTestResult(0.85);
        client.logQCResult(result);
    }
    
    // Flush should send immediately
    client.flush();
    
    auto stats = client.getStatistics();
    EXPECT_GT(stats.metrics_logged, 0);
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ContinuousLearningClientTest, EndToEndWorkflow) {
    ContinuousLearningClient client(config_);
    
    int trigger_count = 0;
    client.setTriggerCallback([&](const OptimizationTrigger& trigger) {
        trigger_count++;
        EXPECT_FALSE(trigger.recommendation.empty());
    });
    
    // Simulate a quality degradation scenario
    // Start with good quality
    for (int i = 0; i < 5; i++) {
        auto result = createTestResult(0.85);
        client.logQCResult(result);
    }
    
    // Quality degrades
    for (int i = 0; i < config_.metric_window_size; i++) {
        auto result = createTestResult(0.65);
        client.logQCResult(result);
    }
    
    // Should trigger optimization
    EXPECT_GT(trigger_count, 0);
    
    auto stats = client.getStatistics();
    EXPECT_GT(stats.metrics_logged, 0);
    EXPECT_GT(stats.triggers_fired, 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
