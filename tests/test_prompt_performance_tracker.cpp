/**
 * @file test_prompt_performance_tracker.cpp
 * @brief Unit tests for PromptPerformanceTracker
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_performance_tracker.h"

using namespace themis::prompt_engineering;

class PromptPerformanceTrackerTest : public ::testing::Test {
protected:
    void SetUp() override {
        tracker_ = std::make_unique<PromptPerformanceTracker>();
    }
    
    std::unique_ptr<PromptPerformanceTracker> tracker_;
};

TEST_F(PromptPerformanceTrackerTest, RecordAndRetrieveMetrics) {
    const std::string prompt_id = "test_prompt_1";
    
    // Record some successful executions
    tracker_->recordExecution(prompt_id, true, 100.0);
    tracker_->recordExecution(prompt_id, true, 150.0);
    tracker_->recordExecution(prompt_id, true, 125.0);
    
    // Get metrics
    auto metrics = tracker_->getMetrics(prompt_id);
    ASSERT_TRUE(metrics.has_value());
    
    EXPECT_EQ(metrics->prompt_id, prompt_id);
    EXPECT_EQ(metrics->total_executions, 3);
    EXPECT_EQ(metrics->failed_executions, 0);
    EXPECT_DOUBLE_EQ(metrics->success_rate, 1.0);
    EXPECT_NEAR(metrics->avg_latency_ms, 125.0, 0.1);
}

TEST_F(PromptPerformanceTrackerTest, SuccessRateCalculation) {
    const std::string prompt_id = "test_prompt_2";
    
    // Record mixed results: 3 success, 2 failures
    tracker_->recordExecution(prompt_id, true, 100.0);
    tracker_->recordExecution(prompt_id, false, 50.0);
    tracker_->recordExecution(prompt_id, true, 120.0);
    tracker_->recordExecution(prompt_id, false, 75.0);
    tracker_->recordExecution(prompt_id, true, 110.0);
    
    auto metrics = tracker_->getMetrics(prompt_id);
    ASSERT_TRUE(metrics.has_value());
    
    EXPECT_EQ(metrics->total_executions, 5);
    EXPECT_EQ(metrics->failed_executions, 2);
    EXPECT_DOUBLE_EQ(metrics->success_rate, 0.6); // 3/5 = 0.6
}

TEST_F(PromptPerformanceTrackerTest, UserFeedback) {
    const std::string prompt_id = "test_prompt_3";
    
    // Record with user feedback
    tracker_->recordExecution(prompt_id, true, 100.0, 0.8);
    tracker_->recordExecution(prompt_id, true, 100.0, 0.9);
    tracker_->recordExecution(prompt_id, true, 100.0, 0.7);
    
    auto metrics = tracker_->getMetrics(prompt_id);
    ASSERT_TRUE(metrics.has_value());
    
    // Average feedback: (0.8 + 0.9 + 0.7) / 3 = 0.8
    EXPECT_NEAR(metrics->user_satisfaction, 0.8, 0.01);
}

TEST_F(PromptPerformanceTrackerTest, LowPerformingPrompts) {
    // Create several prompts with different success rates
    tracker_->recordExecution("good_prompt", true, 100.0);
    tracker_->recordExecution("good_prompt", true, 100.0);
    tracker_->recordExecution("good_prompt", true, 100.0);
    tracker_->recordExecution("good_prompt", true, 100.0);
    tracker_->recordExecution("good_prompt", true, 100.0);
    tracker_->recordExecution("good_prompt", true, 100.0);
    tracker_->recordExecution("good_prompt", true, 100.0);
    tracker_->recordExecution("good_prompt", true, 100.0);
    tracker_->recordExecution("good_prompt", true, 100.0);
    tracker_->recordExecution("good_prompt", true, 100.0);  // 10 executions, 100% success
    
    // Bad prompt: 40% success rate
    tracker_->recordExecution("bad_prompt", true, 100.0);
    tracker_->recordExecution("bad_prompt", true, 100.0);
    tracker_->recordExecution("bad_prompt", true, 100.0);
    tracker_->recordExecution("bad_prompt", true, 100.0);
    tracker_->recordExecution("bad_prompt", false, 100.0);
    tracker_->recordExecution("bad_prompt", false, 100.0);
    tracker_->recordExecution("bad_prompt", false, 100.0);
    tracker_->recordExecution("bad_prompt", false, 100.0);
    tracker_->recordExecution("bad_prompt", false, 100.0);
    tracker_->recordExecution("bad_prompt", false, 100.0);  // 10 executions, 40% success
    
    // Get low performing prompts (threshold 0.7, min 10 executions)
    auto low_performers = tracker_->getLowPerformingPrompts(0.7, 10);
    
    ASSERT_EQ(low_performers.size(), 1);
    EXPECT_EQ(low_performers[0], "bad_prompt");
}

TEST_F(PromptPerformanceTrackerTest, TopPerformingPrompts) {
    // Create several prompts with different success rates
    for (int i = 0; i < 10; ++i) {
        tracker_->recordExecution("prompt_a", true, 100.0);  // 100%
    }
    
    for (int i = 0; i < 10; ++i) {
        tracker_->recordExecution("prompt_b", i < 8, 100.0);  // 80%
    }
    
    for (int i = 0; i < 10; ++i) {
        tracker_->recordExecution("prompt_c", i < 6, 100.0);  // 60%
    }
    
    auto top = tracker_->getTopPerformingPrompts(2, 10);
    
    ASSERT_EQ(top.size(), 2);
    EXPECT_EQ(top[0].first, "prompt_a");
    EXPECT_DOUBLE_EQ(top[0].second, 1.0);
    EXPECT_EQ(top[1].first, "prompt_b");
    EXPECT_DOUBLE_EQ(top[1].second, 0.8);
}

TEST_F(PromptPerformanceTrackerTest, ResetMetrics) {
    const std::string prompt_id = "test_prompt_4";
    
    tracker_->recordExecution(prompt_id, true, 100.0);
    tracker_->recordExecution(prompt_id, true, 100.0);
    
    auto metrics_before = tracker_->getMetrics(prompt_id);
    ASSERT_TRUE(metrics_before.has_value());
    
    bool reset_result = tracker_->resetMetrics(prompt_id);
    EXPECT_TRUE(reset_result);
    
    auto metrics_after = tracker_->getMetrics(prompt_id);
    EXPECT_FALSE(metrics_after.has_value());
}

TEST_F(PromptPerformanceTrackerTest, SummaryStatistics) {
    // Create a few prompts
    for (int i = 0; i < 5; ++i) {
        tracker_->recordExecution("prompt_1", true, 100.0, 0.9);
    }
    
    for (int i = 0; i < 5; ++i) {
        tracker_->recordExecution("prompt_2", i < 4, 150.0, 0.7);
    }
    
    auto summary = tracker_->getSummaryStatistics();
    
    EXPECT_EQ(summary["total_prompts"].get<size_t>(), 2);
    EXPECT_EQ(summary["total_executions"].get<size_t>(), 10);
    EXPECT_GT(summary["avg_success_rate"].get<double>(), 0.0);
    EXPECT_GT(summary["avg_latency_ms"].get<double>(), 0.0);
    EXPECT_GT(summary["avg_user_satisfaction"].get<double>(), 0.0);
}

TEST_F(PromptPerformanceTrackerTest, NonExistentPrompt) {
    auto metrics = tracker_->getMetrics("nonexistent_prompt");
    EXPECT_FALSE(metrics.has_value());
}

TEST_F(PromptPerformanceTrackerTest, ClearAllMetrics) {
    tracker_->recordExecution("prompt_1", true, 100.0);
    tracker_->recordExecution("prompt_2", true, 100.0);
    tracker_->recordExecution("prompt_3", true, 100.0);
    
    auto all_metrics_before = tracker_->getAllMetrics();
    EXPECT_EQ(all_metrics_before.size(), 3);
    
    tracker_->clearAllMetrics();
    
    auto all_metrics_after = tracker_->getAllMetrics();
    EXPECT_EQ(all_metrics_after.size(), 0);
}
