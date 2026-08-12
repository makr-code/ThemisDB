/**
 * @file test_prompt_optimizer.cpp
 * @brief Unit tests for PromptOptimizer
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_optimizer.h"

using namespace themis::prompt_engineering;

class PromptOptimizerTest : public ::testing::Test {
protected:
    OptimizationConfig config_;
    
    // Constants for mock evaluation
    static constexpr double BASE_SCORE_DENOMINATOR = 200.0;
    static constexpr double TASK_BONUS = 0.2;
    static constexpr double EXAMPLE_BONUS = 0.15;
    static constexpr double GUIDELINES_BONUS = 0.1;
    
    void SetUp() override {
        config_.max_iterations = 3;
        config_.min_improvement = 0.05;
        config_.target_score = 0.9;
        config_.num_test_cases = 5;
    }
    
    // Mock evaluation function
    static double mockEvaluator(
        const std::string& prompt,
        const std::vector<TestCase>& test_cases
    ) {
        // Simple mock: longer prompts score better (up to a point)
        double base_score = std::min(0.5, prompt.length() / BASE_SCORE_DENOMINATOR);
        
        // Bonus for having "Task" or "Example"
        if (prompt.find("Task") != std::string::npos) {
            base_score += TASK_BONUS;
        }
        if (prompt.find("Example") != std::string::npos) {
            base_score += EXAMPLE_BONUS;
        }
        if (prompt.find("Guidelines") != std::string::npos) {
            base_score += GUIDELINES_BONUS;
        }
        
        return std::min(1.0, base_score);
    }
};

TEST_F(PromptOptimizerTest, BasicOptimization) {
    PromptOptimizer optimizer(config_);
    
    std::string initial_prompt = "Do the task.";
    std::vector<TestCase> test_cases = {
        {"input1", "output1", {}},
        {"input2", "output2", {}}
    };
    
    auto result = optimizer.optimize(
        initial_prompt,
        test_cases,
        mockEvaluator
    );
    
    EXPECT_GT(result.final_score, 0.0);
    EXPECT_GT(result.iterations, 0);
    EXPECT_EQ(result.score_history.size(), result.iterations + 1);
    EXPECT_EQ(result.prompt_history.size(), result.iterations + 1);
}

TEST_F(PromptOptimizerTest, ImprovementTracking) {
    PromptOptimizer optimizer(config_);
    
    std::string initial_prompt = "Short prompt.";
    std::vector<TestCase> test_cases = {{"in", "out", {}}};
    
    auto result = optimizer.optimize(
        initial_prompt,
        test_cases,
        mockEvaluator
    );
    
    // Check that score improves over iterations
    EXPECT_GE(result.final_score, result.score_history[0]);
    
    // Metadata should be populated
    EXPECT_TRUE(result.metadata.contains("initial_score"));
    EXPECT_TRUE(result.metadata.contains("improvement"));
}

TEST_F(PromptOptimizerTest, EarlyTermination) {
    config_.target_score = 0.3; // Low target for quick termination
    PromptOptimizer optimizer(config_);
    
    std::string initial_prompt = "This is a longer prompt with Task and Example keywords.";
    std::vector<TestCase> test_cases = {{"in", "out", {}}};
    
    auto result = optimizer.optimize(
        initial_prompt,
        test_cases,
        mockEvaluator
    );
    
    // Should terminate before max iterations due to target reached
    EXPECT_LE(result.iterations, config_.max_iterations);
    EXPECT_GE(result.final_score, config_.target_score);
}

TEST_F(PromptOptimizerTest, HistoryTracking) {
    config_.enable_version_control = true;
    PromptOptimizer optimizer(config_);
    
    std::string initial_prompt = "Initial.";
    std::vector<TestCase> test_cases = {{"in", "out", {}}};
    
    EXPECT_EQ(optimizer.getHistory().size(), 0);
    
    optimizer.optimize(initial_prompt, test_cases, mockEvaluator);
    
    auto history = optimizer.getHistory();
    EXPECT_GT(history.size(), 0);
    
    optimizer.clearHistory();
    EXPECT_EQ(optimizer.getHistory().size(), 0);
}

TEST_F(PromptOptimizerTest, FeedbackGeneration) {
    PromptOptimizer optimizer(config_);
    
    std::string prompt = "Test prompt";
    double score = 0.6;
    std::vector<TestCase> test_cases = {{"in", "out", {}}};
    
    std::string feedback = optimizer.generateFeedback(prompt, score, test_cases);
    
    EXPECT_FALSE(feedback.empty());
    EXPECT_NE(feedback.find("0.6"), std::string::npos); // Should mention the score
}

TEST_F(PromptOptimizerTest, CustomImprovementFunction) {
    PromptOptimizer optimizer(config_);
    
    auto custom_improver = [](const std::string& prompt, double score, const std::string& feedback) {
        return prompt + " [IMPROVED]";
    };
    
    std::vector<TestCase> test_cases = {{"in", "out", {}}};
    
    auto result = optimizer.optimize(
        "Initial",
        test_cases,
        mockEvaluator,
        custom_improver
    );
    
    // Should use our custom improver
    EXPECT_NE(result.optimized_prompt.find("[IMPROVED]"), std::string::npos);
}

TEST_F(PromptOptimizerTest, EmptyTestCases) {
    PromptOptimizer optimizer(config_);
    
    std::vector<TestCase> empty_test_cases;
    
    auto result = optimizer.optimize(
        "Initial",
        empty_test_cases,
        mockEvaluator
    );
    
    // Should return empty result
    EXPECT_EQ(result.iterations, 0);
}

TEST_F(PromptOptimizerTest, ConfigurationUpdate) {
    PromptOptimizer optimizer(config_);
    
    EXPECT_EQ(optimizer.getConfig().max_iterations, 3);
    
    OptimizationConfig new_config;
    new_config.max_iterations = 10;
    optimizer.setConfig(new_config);
    
    EXPECT_EQ(optimizer.getConfig().max_iterations, 10);
}
