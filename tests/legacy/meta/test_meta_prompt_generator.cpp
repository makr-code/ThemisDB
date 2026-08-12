/**
 * @file test_meta_prompt_generator.cpp
 * @brief Unit tests for MetaPromptGenerator
 */

#include <gtest/gtest.h>
#include "prompt_engineering/meta_prompt_generator.h"

using namespace themis::prompt_engineering;

class MetaPromptGeneratorTest : public ::testing::Test {
protected:
    MetaPromptConfig config_;
    
    void SetUp() override {
        config_.improvement_strategy = "iterative";
        config_.include_examples = true;
        config_.include_constraints = true;
        config_.max_prompt_length = 2000;
    }
};

TEST_F(MetaPromptGeneratorTest, BasicImprovementPrompt) {
    MetaPromptGenerator generator(config_);
    
    std::string original = "Summarize the text.";
    std::string feedback = "Output is too brief.";
    double score = 0.6;
    
    auto result = generator.generateImprovementPrompt(
        original, feedback, score
    );
    
    EXPECT_FALSE(result.meta_prompt.empty());
    EXPECT_FALSE(result.improvement_suggestion.empty());
    EXPECT_GT(result.key_insights.size(), 0);
}

TEST_F(MetaPromptGeneratorTest, MetaPromptContainsOriginal) {
    MetaPromptGenerator generator(config_);
    
    std::string original = "Classify sentiment";
    auto result = generator.generateImprovementPrompt(
        original, "Needs examples", 0.5
    );
    
    EXPECT_NE(result.meta_prompt.find(original), std::string::npos);
}

TEST_F(MetaPromptGeneratorTest, ScoreInfluencesSuggestions) {
    MetaPromptGenerator generator(config_);
    
    // Low score
    auto result_low = generator.generateImprovementPrompt(
        "Do task", "Poor performance", 0.3
    );
    
    // High score
    auto result_high = generator.generateImprovementPrompt(
        "Do task well", "Good performance", 0.9
    );
    
    // Different insights for different score ranges
    EXPECT_NE(result_low.improvement_suggestion, result_high.improvement_suggestion);
}

TEST_F(MetaPromptGeneratorTest, AnalysisPromptGeneration) {
    MetaPromptGenerator generator(config_);
    
    std::vector<std::pair<std::string, std::string>> examples = {
        {"input1", "output1"},
        {"input2", "output2"}
    };
    
    std::string analysis = generator.generateAnalysisPrompt(
        "Test prompt", examples
    );
    
    EXPECT_FALSE(analysis.empty());
    EXPECT_NE(analysis.find("input1"), std::string::npos);
    EXPECT_NE(analysis.find("output1"), std::string::npos);
}

TEST_F(MetaPromptGeneratorTest, ImprovementSuggestionsByWeakness) {
    MetaPromptGenerator generator(config_);
    
    auto suggestions_clarity = generator.generateImprovementSuggestions(
        "Prompt", "unclear instructions"
    );
    
    auto suggestions_examples = generator.generateImprovementSuggestions(
        "Prompt", "needs more examples"
    );
    
    EXPECT_GT(suggestions_clarity.size(), 0);
    EXPECT_GT(suggestions_examples.size(), 0);
    EXPECT_NE(suggestions_clarity, suggestions_examples);
}

TEST_F(MetaPromptGeneratorTest, SuccessPatternExtraction) {
    MetaPromptGenerator generator(config_);
    
    std::vector<std::pair<std::string, double>> successful = {
        {"Example: Do this\nStep 1: First\nFormat: JSON", 0.9},
        {"Example: Do that\nStep 1: Begin\nOutput: CSV", 0.85},
        {"Simple prompt", 0.5}  // Below threshold
    };
    
    auto patterns = generator.extractSuccessPatterns(successful);
    
    EXPECT_GT(patterns.size(), 0);
    // Should identify that high-scoring prompts have examples and steps
}

TEST_F(MetaPromptGeneratorTest, DifferentStrategies) {
    // Iterative strategy
    config_.improvement_strategy = "iterative";
    MetaPromptGenerator gen1(config_);
    auto result1 = gen1.generateImprovementPrompt("Test", "feedback", 0.5);
    
    // Analytical strategy
    config_.improvement_strategy = "analytical";
    MetaPromptGenerator gen2(config_);
    auto result2 = gen2.generateImprovementPrompt("Test", "feedback", 0.5);
    
    // Creative strategy
    config_.improvement_strategy = "creative";
    MetaPromptGenerator gen3(config_);
    auto result3 = gen3.generateImprovementPrompt("Test", "feedback", 0.5);
    
    // Different strategies should produce different instructions
    EXPECT_NE(result1.meta_prompt, result2.meta_prompt);
    EXPECT_NE(result2.meta_prompt, result3.meta_prompt);
}

TEST_F(MetaPromptGeneratorTest, ConstraintsInclusion) {
    config_.include_constraints = true;
    MetaPromptGenerator gen1(config_);
    auto result1 = gen1.generateImprovementPrompt("Test", "feedback", 0.5);
    
    config_.include_constraints = false;
    MetaPromptGenerator gen2(config_);
    auto result2 = gen2.generateImprovementPrompt("Test", "feedback", 0.5);
    
    EXPECT_GT(result1.meta_prompt.length(), result2.meta_prompt.length());
}

TEST_F(MetaPromptGeneratorTest, ExamplesInclusion) {
    config_.include_examples = true;
    MetaPromptGenerator gen1(config_);
    auto result1 = gen1.generateImprovementPrompt("Test", "feedback", 0.5);
    
    config_.include_examples = false;
    MetaPromptGenerator gen2(config_);
    auto result2 = gen2.generateImprovementPrompt("Test", "feedback", 0.5);
    
    EXPECT_NE(result1.meta_prompt.find("Example"), std::string::npos);
}

TEST_F(MetaPromptGeneratorTest, TaskDescriptionInclusion) {
    MetaPromptGenerator generator(config_);
    
    auto result = generator.generateImprovementPrompt(
        "Original", "feedback", 0.5, "Classify text sentiment"
    );
    
    EXPECT_NE(result.meta_prompt.find("Classify text sentiment"), std::string::npos);
}

TEST_F(MetaPromptGeneratorTest, MetadataPopulation) {
    MetaPromptGenerator generator(config_);
    
    auto result = generator.generateImprovementPrompt(
        "Test prompt", "feedback", 0.7
    );
    
    EXPECT_TRUE(result.metadata.contains("score"));
    EXPECT_TRUE(result.metadata.contains("strategy"));
    EXPECT_TRUE(result.metadata.contains("original_length"));
    EXPECT_DOUBLE_EQ(result.metadata["score"].get<double>(), 0.7);
}

TEST_F(MetaPromptGeneratorTest, ConfigurationUpdate) {
    MetaPromptGenerator generator(config_);
    
    EXPECT_EQ(generator.getConfig().improvement_strategy, "iterative");
    
    MetaPromptConfig new_config;
    new_config.improvement_strategy = "analytical";
    generator.setConfig(new_config);
    
    EXPECT_EQ(generator.getConfig().improvement_strategy, "analytical");
}

TEST_F(MetaPromptGeneratorTest, EmptyPromptHandling) {
    MetaPromptGenerator generator(config_);
    
    auto result = generator.generateImprovementPrompt("", "feedback", 0.5);
    
    // Should still generate meta-prompt
    EXPECT_FALSE(result.meta_prompt.empty());
}
