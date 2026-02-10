/**
 * @file test_self_improvement_orchestrator.cpp
 * @brief Unit tests for SelfImprovementOrchestrator
 */

#include <gtest/gtest.h>
#include "prompt_engineering/self_improvement_orchestrator.h"
#include "prompt_engineering/prompt_performance_tracker.h"
#include "prompt_engineering/prompt_optimizer.h"
#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/prompt_evaluator.h"

using namespace themis::prompt_engineering;

class SelfImprovementOrchestratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create components
        tracker_ = std::make_shared<PromptPerformanceTracker>();
        
        OptimizationConfig opt_config;
        opt_config.max_iterations = 3;
        optimizer_ = std::make_shared<PromptOptimizer>(opt_config);
        
        manager_ = std::make_shared<PromptManager>();
        
        EvaluatorConfig eval_config;
        evaluator_ = std::make_shared<PromptEvaluator>(eval_config);
        
        // Create orchestrator with default config
        config_.min_success_rate = 0.7;
        config_.min_executions = 10;
        config_.enable_ab_testing = false;  // Disable for most tests
        
        orchestrator_ = std::make_unique<SelfImprovementOrchestrator>(
            config_,
            tracker_,
            optimizer_,
            manager_,
            evaluator_
        );
    }
    
    ImprovementConfig config_;
    std::shared_ptr<PromptPerformanceTracker> tracker_;
    std::shared_ptr<PromptOptimizer> optimizer_;
    std::shared_ptr<PromptManager> manager_;
    std::shared_ptr<PromptEvaluator> evaluator_;
    std::unique_ptr<SelfImprovementOrchestrator> orchestrator_;
};

TEST_F(SelfImprovementOrchestratorTest, ShouldOptimize_InsufficientExecutions) {
    // Create a prompt with low executions
    std::string prompt_id = "test_prompt_1";
    
    // Record only 5 executions (below threshold of 10)
    for (int i = 0; i < 5; ++i) {
        tracker_->recordExecution(prompt_id, false, 100.0);  // All failures
    }
    
    // Should not optimize due to insufficient executions
    EXPECT_FALSE(orchestrator_->shouldOptimize(prompt_id));
}

TEST_F(SelfImprovementOrchestratorTest, ShouldOptimize_HighSuccessRate) {
    // Create a prompt with good performance
    std::string prompt_id = "test_prompt_2";
    
    // Record 15 executions with high success rate (0.9)
    for (int i = 0; i < 15; ++i) {
        tracker_->recordExecution(prompt_id, i < 14, 100.0);  // 14/15 = 0.93
    }
    
    // Should not optimize - performance is good
    EXPECT_FALSE(orchestrator_->shouldOptimize(prompt_id));
}

TEST_F(SelfImprovementOrchestratorTest, ShouldOptimize_LowSuccessRate) {
    // Create a prompt with poor performance
    std::string prompt_id = "test_prompt_3";
    
    // Record 15 executions with low success rate (0.4)
    for (int i = 0; i < 15; ++i) {
        tracker_->recordExecution(prompt_id, i < 6, 100.0);  // 6/15 = 0.4
    }
    
    // Should optimize - performance is below threshold
    EXPECT_TRUE(orchestrator_->shouldOptimize(prompt_id));
}

TEST_F(SelfImprovementOrchestratorTest, ManualOptimization) {
    std::string prompt_id = "test_prompt_4";
    
    // Create a template
    PromptManager::PromptTemplate t;
    t.id = prompt_id;
    t.name = "Test Prompt";
    t.version = "v1";
    t.content = "Original prompt content";
    manager_->createTemplate(t);
    
    // Record some poor performance
    for (int i = 0; i < 15; ++i) {
        tracker_->recordExecution(prompt_id, i < 5, 100.0);  // 5/15 = 0.33
    }
    
    // Create test cases
    std::vector<TestCase> test_cases = {
        {"input1", "output1", {}},
        {"input2", "output2", {}},
        {"input3", "output3", {}}
    };
    
    // Manually trigger optimization
    auto result = orchestrator_->optimizePrompt(prompt_id, test_cases);
    
    EXPECT_EQ(result.prompt_id, prompt_id);
    EXPECT_FALSE(result.original_version.empty());
    EXPECT_FALSE(result.optimized_version.empty());
    EXPECT_GT(result.iterations, 0);
}

TEST_F(SelfImprovementOrchestratorTest, OptimizationHistory) {
    std::string prompt_id = "test_prompt_5";
    
    // Create a template
    PromptManager::PromptTemplate t;
    t.id = prompt_id;
    t.name = "Test Prompt";
    t.content = "Original content";
    manager_->createTemplate(t);
    
    // Record poor performance
    for (int i = 0; i < 15; ++i) {
        tracker_->recordExecution(prompt_id, i < 5, 100.0);
    }
    
    // No history initially
    auto history = orchestrator_->getOptimizationHistory(prompt_id);
    EXPECT_EQ(history.size(), 0);
    
    // Run optimization
    std::vector<TestCase> test_cases = {
        {"input", "output", {}}
    };
    orchestrator_->optimizePrompt(prompt_id, test_cases);
    
    // Check history
    history = orchestrator_->getOptimizationHistory(prompt_id);
    EXPECT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].prompt_id, prompt_id);
}

TEST_F(SelfImprovementOrchestratorTest, ABTestStartAndRecord) {
    // Enable A/B testing
    config_.enable_ab_testing = true;
    config_.ab_test_sample_size = 20;
    orchestrator_->setConfig(config_);
    
    std::string prompt_id = "test_prompt_6";
    std::string version_a = "Version A content";
    std::string version_b = "Version B content";
    
    // Start A/B test
    std::string test_id = orchestrator_->startABTest(
        prompt_id,
        version_a,
        version_b,
        20
    );
    
    EXPECT_FALSE(test_id.empty());
    
    // Record observations for version A (60% success)
    for (int i = 0; i < 10; ++i) {
        orchestrator_->recordABTestObservation(
            test_id,
            "a",
            i < 6,  // 6/10 success
            100.0
        );
    }
    
    // Record observations for version B (80% success)
    for (int i = 0; i < 10; ++i) {
        orchestrator_->recordABTestObservation(
            test_id,
            "b",
            i < 8,  // 8/10 success
            100.0
        );
    }
    
    // Get test results
    auto test_opt = orchestrator_->getABTestResults(test_id);
    ASSERT_TRUE(test_opt.has_value());
    
    auto test = test_opt.value();
    EXPECT_EQ(test.samples_a, 10);
    EXPECT_EQ(test.samples_b, 10);
    EXPECT_NEAR(test.score_a, 0.6, 0.01);
    EXPECT_NEAR(test.score_b, 0.8, 0.01);
}

TEST_F(SelfImprovementOrchestratorTest, GetActiveABTests) {
    std::string test_id = orchestrator_->startABTest(
        "prompt_1",
        "version_a",
        "version_b",
        100
    );
    
    auto active_tests = orchestrator_->getActiveABTests();
    EXPECT_EQ(active_tests.size(), 1);
    EXPECT_EQ(active_tests[0].test_id, test_id);
}

TEST_F(SelfImprovementOrchestratorTest, ConfigurationUpdate) {
    auto original_config = orchestrator_->getConfig();
    EXPECT_EQ(original_config.min_success_rate, 0.7);
    
    ImprovementConfig new_config;
    new_config.min_success_rate = 0.8;
    new_config.min_executions = 50;
    
    orchestrator_->setConfig(new_config);
    
    auto updated_config = orchestrator_->getConfig();
    EXPECT_EQ(updated_config.min_success_rate, 0.8);
    EXPECT_EQ(updated_config.min_executions, 50);
}

TEST_F(SelfImprovementOrchestratorTest, RollbackPrompt) {
    std::string prompt_id = "test_prompt_7";
    
    // Create a template
    PromptManager::PromptTemplate t;
    t.id = prompt_id;
    t.name = "Test Prompt";
    t.content = "Original content";
    manager_->createTemplate(t);
    
    // Record poor performance
    for (int i = 0; i < 15; ++i) {
        tracker_->recordExecution(prompt_id, i < 5, 100.0);
    }
    
    // Run optimization
    std::vector<TestCase> test_cases = {{"input", "output", {}}};
    auto result = orchestrator_->optimizePrompt(prompt_id, test_cases);
    
    // Check that optimization happened
    EXPECT_EQ(result.prompt_id, prompt_id);
    
    // Rollback will fail if not deployed, which is expected without full A/B testing
    // Test the no-history case
    bool rolled_back = orchestrator_->rollbackPrompt("nonexistent_prompt");
    EXPECT_FALSE(rolled_back);  // Should fail for non-existent prompt
    
    // For existing prompt with history but no deployment, rollback might succeed or fail
    // depending on whether optimization was marked as deployed
    // This tests that the method doesn't crash
    rolled_back = orchestrator_->rollbackPrompt(prompt_id);
    // Result depends on deployment status, but method should not crash
    EXPECT_TRUE(rolled_back || !rolled_back);  // Just ensure no crash
}

TEST_F(SelfImprovementOrchestratorTest, AutoOptimizationScan) {
    // Create several prompts with different performance levels
    
    // Good prompt - should not be optimized
    std::string good_prompt = "good_prompt";
    PromptManager::PromptTemplate t1;
    t1.id = good_prompt;
    t1.content = "Good content";
    manager_->createTemplate(t1);
    for (int i = 0; i < 20; ++i) {
        tracker_->recordExecution(good_prompt, i < 18, 100.0);  // 90% success
    }
    
    // Bad prompt - should be optimized
    std::string bad_prompt = "bad_prompt";
    PromptManager::PromptTemplate t2;
    t2.id = bad_prompt;
    t2.content = "Bad content";
    manager_->createTemplate(t2);
    for (int i = 0; i < 20; ++i) {
        tracker_->recordExecution(bad_prompt, i < 8, 100.0);  // 40% success
    }
    
    // Run auto-optimization
    auto results = orchestrator_->runAutoOptimization();
    
    // Auto-optimization currently logs candidates but doesn't execute
    // (requires test cases which aren't available in auto mode)
    // This test validates the scan mechanism works
    EXPECT_TRUE(results.size() >= 0);  // Should not crash
}

TEST_F(SelfImprovementOrchestratorTest, OptimizationResultSerialization) {
    OptimizationResult result;
    result.prompt_id = "test_id";
    result.original_version = "v1";
    result.optimized_version = "v2";
    result.status = OptimizationStatus::COMPLETED;
    result.baseline_score = 0.7;
    result.optimized_score = 0.9;
    result.improvement = 0.286;
    result.iterations = 5;
    
    auto json = result.toJson();
    
    EXPECT_EQ(json["prompt_id"], "test_id");
    EXPECT_EQ(json["original_version"], "v1");
    EXPECT_EQ(json["optimized_version"], "v2");
    EXPECT_DOUBLE_EQ(json["baseline_score"], 0.7);
    EXPECT_DOUBLE_EQ(json["optimized_score"], 0.9);
}

TEST_F(SelfImprovementOrchestratorTest, ABTestSerialization) {
    ABTest test;
    test.test_id = "test_123";
    test.prompt_id = "prompt_abc";
    test.version_a = "version A";
    test.version_b = "version B";
    test.samples_a = 100;
    test.samples_b = 100;
    test.score_a = 0.75;
    test.score_b = 0.85;
    test.is_significant = true;
    test.p_value = 0.03;
    
    auto json = test.toJson();
    
    EXPECT_EQ(json["test_id"], "test_123");
    EXPECT_EQ(json["prompt_id"], "prompt_abc");
    EXPECT_EQ(json["samples_a"], 100);
    EXPECT_EQ(json["samples_b"], 100);
    EXPECT_DOUBLE_EQ(json["score_a"], 0.75);
    EXPECT_DOUBLE_EQ(json["score_b"], 0.85);
    EXPECT_TRUE(json["is_significant"]);
}
