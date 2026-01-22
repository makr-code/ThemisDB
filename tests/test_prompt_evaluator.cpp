/**
 * @file test_prompt_evaluator.cpp
 * @brief Unit tests for PromptEvaluator
 */

#include <gtest/gtest.h>
#include "llm/prompt_evaluator.h"

using namespace themis::llm;

class PromptEvaluatorTest : public ::testing::Test {
protected:
    EvaluatorConfig config_;
    
    void SetUp() override {
        config_.similarity_weight = 0.5;
        config_.exact_match_weight = 0.3;
        config_.relevance_weight = 0.2;
        config_.pass_threshold = 0.7;
    }
};

TEST_F(PromptEvaluatorTest, ExactMatchDetection) {
    PromptEvaluator evaluator(config_);
    
    std::string output = "Hello world";
    std::string expected = "Hello world";
    
    double score = PromptEvaluator::computeExactMatch(output, expected);
    EXPECT_DOUBLE_EQ(score, 1.0);
    
    // Case and whitespace normalized
    output = "  hello   WORLD  ";
    score = PromptEvaluator::computeExactMatch(output, expected);
    EXPECT_DOUBLE_EQ(score, 1.0);
    
    // Not exact
    output = "Hello there";
    score = PromptEvaluator::computeExactMatch(output, expected);
    EXPECT_DOUBLE_EQ(score, 0.0);
}

TEST_F(PromptEvaluatorTest, SemanticSimilarity) {
    std::string s1 = "The quick brown fox jumps";
    std::string s2 = "The quick brown fox leaps";
    
    double score = PromptEvaluator::computeSemanticSimilarity(s1, s2);
    EXPECT_GT(score, 0.5); // High overlap
    EXPECT_LT(score, 1.0); // Not identical
    
    s1 = "cat dog bird";
    s2 = "fish snake lizard";
    score = PromptEvaluator::computeSemanticSimilarity(s1, s2);
    EXPECT_LT(score, 0.3); // Low overlap
}

TEST_F(PromptEvaluatorTest, PartialMatch) {
    std::string output = "Hello world";
    std::string expected = "Hello world";
    
    double score = PromptEvaluator::computePartialMatch(output, expected);
    EXPECT_DOUBLE_EQ(score, 1.0);
    
    output = "Hell world"; // One character different
    score = PromptEvaluator::computePartialMatch(output, expected);
    EXPECT_GT(score, 0.8); // High but not perfect
    
    output = "Completely different";
    score = PromptEvaluator::computePartialMatch(output, expected);
    EXPECT_LT(score, 0.5); // Low similarity
}

TEST_F(PromptEvaluatorTest, RelevanceScoring) {
    std::string output = "The answer is 42 because it explains everything";
    std::string expected = "The answer is 42";
    
    double score = PromptEvaluator::computeRelevance(output, expected);
    EXPECT_DOUBLE_EQ(score, 1.0); // All expected words present
    
    output = "The result is 99";
    score = PromptEvaluator::computeRelevance(output, expected);
    EXPECT_GT(score, 0.0); // Some words match
    EXPECT_LT(score, 1.0); // Not all match
}

TEST_F(PromptEvaluatorTest, SingleEvaluation) {
    PromptEvaluator evaluator(config_);
    
    std::string output = "The cat sat on the mat";
    std::string expected = "The cat sat on the mat";
    
    auto metrics = evaluator.evaluateSingle(output, expected);
    
    EXPECT_DOUBLE_EQ(metrics.exact_match, 1.0);
    EXPECT_DOUBLE_EQ(metrics.semantic_similarity, 1.0);
    EXPECT_DOUBLE_EQ(metrics.partial_match, 1.0);
    EXPECT_DOUBLE_EQ(metrics.relevance, 1.0);
}

TEST_F(PromptEvaluatorTest, BatchEvaluation) {
    PromptEvaluator evaluator(config_);
    
    std::vector<std::string> outputs = {
        "Hello world",
        "Goodbye world",
        "Hello universe"
    };
    
    std::vector<std::string> expected = {
        "Hello world",
        "Goodbye world",
        "Hello cosmos"
    };
    
    auto agg = evaluator.evaluateBatch(outputs, expected);
    
    EXPECT_EQ(agg.num_exact_matches, 2); // First two are exact
    EXPECT_GT(agg.overall_score, 0.7); // High overall quality
    EXPECT_GT(agg.mean_similarity, 0.7);
    EXPECT_EQ(agg.per_case_metrics.size(), 3);
}

TEST_F(PromptEvaluatorTest, PassRateCalculation) {
    config_.pass_threshold = 0.8;
    PromptEvaluator evaluator(config_);
    
    // Create outputs with varying quality
    std::vector<std::string> outputs = {
        "Perfect match",      // High score
        "Perfect match",      // High score
        "Completely wrong"    // Low score
    };
    
    std::vector<std::string> expected = {
        "Perfect match",
        "Perfect match",
        "Perfect match"
    };
    
    auto agg = evaluator.evaluateBatch(outputs, expected);
    
    // 2 out of 3 should pass
    EXPECT_DOUBLE_EQ(agg.pass_rate, 2.0 / 3.0);
}

TEST_F(PromptEvaluatorTest, StatisticalSignificance) {
    std::vector<double> baseline = {0.5, 0.52, 0.51, 0.53, 0.50};
    std::vector<double> improved = {0.6, 0.62, 0.61, 0.63, 0.60};
    
    bool significant = PromptEvaluator::isStatisticallySignificant(baseline, improved);
    EXPECT_TRUE(significant); // 20% improvement should be significant
    
    std::vector<double> marginally_better = {0.51, 0.52, 0.53, 0.52, 0.51};
    significant = PromptEvaluator::isStatisticallySignificant(baseline, marginally_better);
    EXPECT_FALSE(significant); // Small improvement not significant
}

TEST_F(PromptEvaluatorTest, EmptyInputHandling) {
    PromptEvaluator evaluator(config_);
    
    std::vector<std::string> empty_outputs;
    std::vector<std::string> empty_expected;
    
    auto agg = evaluator.evaluateBatch(empty_outputs, empty_expected);
    
    EXPECT_DOUBLE_EQ(agg.overall_score, 0.0);
    EXPECT_DOUBLE_EQ(agg.mean_similarity, 0.0);
    EXPECT_EQ(agg.num_exact_matches, 0);
}

TEST_F(PromptEvaluatorTest, ConfigurationUpdate) {
    PromptEvaluator evaluator(config_);
    
    EXPECT_DOUBLE_EQ(evaluator.getConfig().pass_threshold, 0.7);
    
    EvaluatorConfig new_config;
    new_config.pass_threshold = 0.85;
    evaluator.setConfig(new_config);
    
    EXPECT_DOUBLE_EQ(evaluator.getConfig().pass_threshold, 0.85);
}

TEST_F(PromptEvaluatorTest, LevenshteinDistance) {
    // Same strings
    size_t dist = PromptEvaluator::levenshteinDistance("hello", "hello");
    EXPECT_EQ(dist, 0);
    
    // One insertion
    dist = PromptEvaluator::levenshteinDistance("hello", "helloX");
    EXPECT_EQ(dist, 1);
    
    // One deletion
    dist = PromptEvaluator::levenshteinDistance("hello", "hell");
    EXPECT_EQ(dist, 1);
    
    // One substitution
    dist = PromptEvaluator::levenshteinDistance("hello", "hallo");
    EXPECT_EQ(dist, 1);
    
    // Completely different
    dist = PromptEvaluator::levenshteinDistance("abc", "xyz");
    EXPECT_EQ(dist, 3);
}
