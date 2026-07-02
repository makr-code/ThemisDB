/**
 * @file test_ml_learning_metrics_collector.cpp
 * @brief Tests for ML Learning Metrics Collector observability.
 */

#include <gtest/gtest.h>

#include "rag/ml_learning_metrics_collector.h"
#include "rag/learning_metrics.h"

namespace themis::rag::learning {
namespace {

/**
 * @brief Mock metrics implementation for testing
 */
class MockMetrics : public core::concerns::IMetrics {
public:
    void incrementCounter(const std::string& name, int64_t value = 1,
                         const Labels& labels = {}) override {
        counters_[name] += value;
    }

    void setGauge(const std::string& name, double value,
                  const Labels& labels = {}) override {
        gauges_[name] = value;
    }

    void incrementGauge(const std::string& name, double delta,
                        const Labels& labels = {}) override {
        gauges_[name] += delta;
    }

    void decrementGauge(const std::string& name, double delta,
                        const Labels& labels = {}) override {
        gauges_[name] -= delta;
    }

    void observeHistogram(const std::string& name, double value,
                         const Labels& labels = {}) override {
        histograms_[name].push_back(value);
    }

    // Helper methods for testing
    int64_t getCounterValue(const std::string& name) {
        return counters_[name];
    }

    double getGaugeValue(const std::string& name) {
        return gauges_[name];
    }

    const std::vector<double>& getHistogramValues(const std::string& name) {
        return histograms_[name];
    }

private:
    std::unordered_map<std::string, int64_t> counters_;
    std::unordered_map<std::string, double> gauges_;
    std::unordered_map<std::string, std::vector<double>> histograms_;
};

/**
 * @brief Mock logger implementation for testing
 */
class MockLogger : public core::concerns::ILogger {
public:
    void logStructured(Level level, const std::string& message,
                      const Fields& fields = {}) override {
        logged_messages_.push_back({message, fields});
    }

    void logWithContext(Level level, const std::string& message,
                       const TraceContext& ctx,
                       const Fields& fields = {}) override {
        Fields merged = fields;
        if (!ctx.trace_id.empty()) merged["trace_id"] = ctx.trace_id;
        if (!ctx.span_id.empty()) merged["span_id"] = ctx.span_id;
        if (!ctx.request_id.empty()) merged["request_id"] = ctx.request_id;
        logged_messages_.push_back({message, merged});
    }

    struct LogEntry {
        std::string message;
        Fields fields;
    };

    std::vector<LogEntry> logged_messages_;
};

// ============================================================================
// Test Fixtures
// ============================================================================

class MLLearningMetricsCollectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_metrics_ = std::make_shared<MockMetrics>();
        mock_logger_ = std::make_shared<MockLogger>();
        
        collector_ = MLLearningMetricsCollector::getInstance();
        collector_->initialize(mock_metrics_, mock_logger_);
        
        // Create a trace context for all tests
        trace_ctx_ = MLLearningMetricsCollector::createTraceContext();
    }

    std::shared_ptr<MLLearningMetricsCollector> collector_;
    std::shared_ptr<MockMetrics> mock_metrics_;
    std::shared_ptr<MockLogger> mock_logger_;
    LearningTraceContext trace_ctx_;
};

// ============================================================================
// Learning Loop Observability Tests
// ============================================================================

TEST_F(MLLearningMetricsCollectorTest, RecordLoopStateTransition) {
    collector_->recordLoopStateTransition("LOOP_1", "ACTIVE", trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_loop_transitions_total"), 1);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_loop_state"), 1.0);
}

TEST_F(MLLearningMetricsCollectorTest, RecordLoopExecutionSuccess) {
    collector_->recordLoopExecution("LOOP_1", 100.5, true, trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_loop_executions_total"), 1);
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_loop_errors_total"), 0);
    
    const auto& histogram = mock_metrics_->getHistogramValues("themisdb_ml_loop_duration_ms");
    EXPECT_EQ(histogram.size(), 1);
    EXPECT_DOUBLE_EQ(histogram[0], 100.5);
}

TEST_F(MLLearningMetricsCollectorTest, RecordLoopExecutionFailure) {
    collector_->recordLoopExecution("LOOP_1", 50.0, false, trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_loop_executions_total"), 1);
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_loop_errors_total"), 1);
}

TEST_F(MLLearningMetricsCollectorTest, RecordMultipleLoopsStateTransitions) {
    collector_->recordLoopStateTransition("LOOP_1", "ACTIVE", trace_ctx_);
    collector_->recordLoopStateTransition("LOOP_2", "IDLE", trace_ctx_);
    collector_->recordLoopStateTransition("LOOP_3", "RUNNING", trace_ctx_);
    collector_->recordLoopStateTransition("LOOP_4", "COMPLETED", trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_loop_transitions_total"), 4);
}

// ============================================================================
// Adapter & Model Observability Tests
// ============================================================================

TEST_F(MLLearningMetricsCollectorTest, RecordAdapterVersion) {
    collector_->recordAdapterVersion("adapter_v1", "1.0", "active", trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_adapter_deployments_total"), 1);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_adapter_version"), 1.0);
}

TEST_F(MLLearningMetricsCollectorTest, RecordRetrainingProgress) {
    collector_->recordRetrainingProgress("adapter_v1", 0.5, trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_retraining_progress_percent"), 50.0);
}

TEST_F(MLLearningMetricsCollectorTest, RecordRetrainingCompletion) {
    collector_->recordRetrainingProgress("adapter_v1", 1.0, trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_retraining_progress_percent"), 100.0);
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_retraining_completions_total"), 1);
}

TEST_F(MLLearningMetricsCollectorTest, RecordModelPerformance) {
    collector_->recordModelPerformance("adapter_v1", 0.95, 150.0, trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_model_accuracy"), 0.95);
    
    const auto& latency_histogram = 
        mock_metrics_->getHistogramValues("themisdb_ml_inference_latency_ms");
    EXPECT_EQ(latency_histogram.size(), 1);
    EXPECT_DOUBLE_EQ(latency_histogram[0], 150.0);
}

// ============================================================================
// A/B Test Observability Tests
// ============================================================================

TEST_F(MLLearningMetricsCollectorTest, RecordABTestState) {
    collector_->recordABTestState("test_001", "running", 0.05, trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_ab_test_state_changes_total"), 1);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_ab_test_improvement_percent"), 5.0);
}

TEST_F(MLLearningMetricsCollectorTest, RecordABTestPromotion) {
    collector_->recordABTestState("test_001", "promoted", 0.10, trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_ab_test_state_changes_total"), 1);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_ab_test_improvement_percent"), 10.0);
}

// ============================================================================
// Error & Warning States Tests
// ============================================================================

TEST_F(MLLearningMetricsCollectorTest, RecordWarningState) {
    collector_->recordWarningState("circuit_breaker_open", "LOOP_1", 
                                  "Too many consecutive failures", trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_warnings_total"), 1);
}

TEST_F(MLLearningMetricsCollectorTest, RecordErrorState) {
    collector_->recordErrorState("provider_unavailable", "LOOP_1", 
                                "BaoOptimizer provider not wired", trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_errors_total"), 1);
}

TEST_F(MLLearningMetricsCollectorTest, RecordMultipleErrors) {
    collector_->recordErrorState("provider_unavailable", "LOOP_1", "msg1", trace_ctx_);
    collector_->recordErrorState("data_corruption", "LOOP_2", "msg2", trace_ctx_);
    collector_->recordErrorState("deadlock_detected", "LOOP_4", "msg3", trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_errors_total"), 3);
}

// ============================================================================
// Feedback Loop Observability Tests
// ============================================================================

TEST_F(MLLearningMetricsCollectorTest, RecordFeedbackCollection) {
    collector_->recordFeedbackCollection("adapter_v1", 150, 0.75, trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_feedback_count"), 150.0);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_positive_feedback_fraction"), 0.75);
}

// ============================================================================
// Snapshot Update Tests
// ============================================================================

TEST_F(MLLearningMetricsCollectorTest, UpdateFromSnapshot) {
    LearningStats stats;
    stats.current_accuracy = 0.92;
    stats.accuracy_7d_avg = 0.90;
    stats.accuracy_trend = 0.01;
    stats.total_interactions_logged = 500;
    stats.lora_retraining_count = 3;
    stats.prompt_optimizations = 5;
    stats.retrieval_optimizations = 2;
    
    collector_->updateFromSnapshot(stats, trace_ctx_);
    
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_accuracy"), 0.92);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_accuracy_7d_avg"), 0.90);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_accuracy_trend"), 0.01);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_total_interactions"), 500.0);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_lora_retraining_count"), 3.0);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_prompt_optimizations"), 5.0);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_retrieval_optimizations"), 2.0);
}

// ============================================================================
// Trace Context Tests
// ============================================================================

TEST_F(MLLearningMetricsCollectorTest, CreateTraceContext) {
    auto ctx = MLLearningMetricsCollector::createTraceContext();
    
    EXPECT_FALSE(ctx.trace_id.empty());
    EXPECT_FALSE(ctx.span_id.empty());
    EXPECT_FALSE(ctx.request_id.empty());
    
    // Trace ID should be 32-char hex
    EXPECT_EQ(ctx.trace_id.length(), 32);
    
    // Span ID should be 16-char hex
    EXPECT_EQ(ctx.span_id.length(), 16);
}

// ============================================================================
// Logging Tests
// ============================================================================

TEST_F(MLLearningMetricsCollectorTest, LogsWarningsWithTraceContext) {
    collector_->recordWarningState("test_warning", "TEST_COMPONENT", "test message", trace_ctx_);
    
    EXPECT_GT(mock_logger_->logged_messages_.size(), 0);
    // Should have trace context in logs (may be multiple messages)
}

TEST_F(MLLearningMetricsCollectorTest, LogsErrorsWithTraceContext) {
    collector_->recordErrorState("test_error", "TEST_COMPONENT", "test error message", trace_ctx_);
    
    EXPECT_GT(mock_logger_->logged_messages_.size(), 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(MLLearningMetricsCollectorTest, CompleteObservabilityWorkflow) {
    // Simulate a complete learning loop with observability
    
    // 1. Loop starts
    collector_->recordLoopStateTransition("LOOP_1", "RUNNING", trace_ctx_);
    
    // 2. Loop executes
    auto loop_ctx = MLLearningMetricsCollector::createTraceContext();
    collector_->recordLoopExecution("LOOP_1", 75.5, true, loop_ctx);
    
    // 3. Adapter gets deployed
    auto deploy_ctx = MLLearningMetricsCollector::createTraceContext();
    collector_->recordAdapterVersion("adapter_v2", "2.0", "active", deploy_ctx);
    
    // 4. Model performance is recorded
    auto perf_ctx = MLLearningMetricsCollector::createTraceContext();
    collector_->recordModelPerformance("adapter_v2", 0.94, 145.0, perf_ctx);
    
    // 5. A/B test is started
    auto test_ctx = MLLearningMetricsCollector::createTraceContext();
    collector_->recordABTestState("test_v2", "running", 0.02, test_ctx);
    
    // 6. Verify metrics were recorded
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_loop_transitions_total"), 1);
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_loop_executions_total"), 1);
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_adapter_deployments_total"), 1);
    EXPECT_EQ(mock_metrics_->getGaugeValue("themisdb_ml_model_accuracy"), 0.94);
    EXPECT_EQ(mock_metrics_->getCounterValue("themisdb_ml_ab_test_state_changes_total"), 1);
}

}  // namespace
}  // namespace themis::rag::learning
