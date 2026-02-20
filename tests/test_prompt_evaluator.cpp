/**
 * @file test_prompt_evaluator.cpp
 * @brief Unit tests for PromptEvaluator
 *
 * Covers EvaluationMetrics, AggregatedMetrics, and statistical significance.
 */

#include <gtest/gtest.h>
#include "llm/prompt_evaluator.h"

using namespace themis::llm;

// ============================================================================
// Test fixture
// ============================================================================

class PromptEvaluatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.similarity_weight  = 0.5;
        config_.exact_match_weight = 0.3;
        config_.relevance_weight   = 0.2;
        config_.pass_threshold     = 0.7;
    }
    EvaluatorConfig config_;
};

// ============================================================================
// computeSemanticSimilarity
// ============================================================================

TEST_F(PromptEvaluatorTest, SemanticSimilarity_Identical) {
    double sim = PromptEvaluator::computeSemanticSimilarity("hello world", "hello world");
    EXPECT_NEAR(sim, 1.0, 1e-9);
}

TEST_F(PromptEvaluatorTest, SemanticSimilarity_NoOverlap) {
    double sim = PromptEvaluator::computeSemanticSimilarity("cat sat mat", "dog runs fast");
    EXPECT_NEAR(sim, 0.0, 1e-9);
}

TEST_F(PromptEvaluatorTest, SemanticSimilarity_PartialOverlap) {
    double sim = PromptEvaluator::computeSemanticSimilarity(
        "the quick brown fox", "the lazy brown dog");
    EXPECT_GT(sim, 0.0);
    EXPECT_LT(sim, 1.0);
}

TEST_F(PromptEvaluatorTest, SemanticSimilarity_EmptyStrings) {
    // Both empty → tokenize returns empty → early return 0.0
    double sim = PromptEvaluator::computeSemanticSimilarity("", "");
    EXPECT_DOUBLE_EQ(sim, 0.0);
}

TEST_F(PromptEvaluatorTest, SemanticSimilarity_OneEmpty) {
    double sim = PromptEvaluator::computeSemanticSimilarity("", "hello");
    EXPECT_DOUBLE_EQ(sim, 0.0);
}

// ============================================================================
// computeExactMatch
// ============================================================================

TEST_F(PromptEvaluatorTest, ExactMatch_Identical) {
    EXPECT_DOUBLE_EQ(PromptEvaluator::computeExactMatch("Hello World", "hello world"), 1.0);
}

TEST_F(PromptEvaluatorTest, ExactMatch_Different) {
    EXPECT_DOUBLE_EQ(PromptEvaluator::computeExactMatch("foo", "bar"), 0.0);
}

TEST_F(PromptEvaluatorTest, ExactMatch_CaseInsensitive) {
    EXPECT_DOUBLE_EQ(PromptEvaluator::computeExactMatch("ABC", "abc"), 1.0);
}

// ============================================================================
// computePartialMatch
// ============================================================================

TEST_F(PromptEvaluatorTest, PartialMatch_Identical) {
    EXPECT_NEAR(PromptEvaluator::computePartialMatch("hello", "hello"), 1.0, 1e-9);
}

TEST_F(PromptEvaluatorTest, PartialMatch_OneEdit) {
    // "hello" vs "helo" — edit distance 1, max_len 5 → score = 1 - 1/5 = 0.8
    double score = PromptEvaluator::computePartialMatch("hello", "helo");
    EXPECT_NEAR(score, 0.8, 1e-9);
}

TEST_F(PromptEvaluatorTest, PartialMatch_BothEmpty) {
    EXPECT_NEAR(PromptEvaluator::computePartialMatch("", ""), 1.0, 1e-9);
}

// ============================================================================
// computeRelevance
// ============================================================================

TEST_F(PromptEvaluatorTest, Relevance_AllTermsPresent) {
    double rel = PromptEvaluator::computeRelevance(
        "the capital of France is Paris",
        "France Paris");
    EXPECT_NEAR(rel, 1.0, 1e-9);
}

TEST_F(PromptEvaluatorTest, Relevance_NoTermsPresent) {
    double rel = PromptEvaluator::computeRelevance("dog runs fast", "cat mat hat");
    EXPECT_NEAR(rel, 0.0, 1e-9);
}

TEST_F(PromptEvaluatorTest, Relevance_EmptyExpected) {
    // Empty expected → trivially 1.0
    double rel = PromptEvaluator::computeRelevance("something", "");
    EXPECT_NEAR(rel, 1.0, 1e-9);
}

// ============================================================================
// evaluateSingle
// ============================================================================

TEST_F(PromptEvaluatorTest, EvaluateSingle_ScoresInRange) {
    PromptEvaluator evaluator(config_);
    auto metrics = evaluator.evaluateSingle(
        "Paris is the capital of France.",
        "Paris is the capital of France.");
    EXPECT_GE(metrics.semantic_similarity, 0.0);
    EXPECT_LE(metrics.semantic_similarity, 1.0);
    EXPECT_GE(metrics.exact_match, 0.0);
    EXPECT_LE(metrics.exact_match, 1.0);
    EXPECT_GE(metrics.partial_match, 0.0);
    EXPECT_LE(metrics.partial_match, 1.0);
    EXPECT_GE(metrics.relevance, 0.0);
    EXPECT_LE(metrics.relevance, 1.0);
}

TEST_F(PromptEvaluatorTest, EvaluateSingle_IdenticalHighScores) {
    PromptEvaluator evaluator(config_);
    auto metrics = evaluator.evaluateSingle("hello world", "hello world");
    EXPECT_NEAR(metrics.semantic_similarity, 1.0, 1e-9);
    EXPECT_NEAR(metrics.exact_match, 1.0, 1e-9);
    EXPECT_NEAR(metrics.partial_match, 1.0, 1e-9);
}

TEST_F(PromptEvaluatorTest, EvaluateSingle_DetailsPopulated) {
    PromptEvaluator evaluator(config_);
    auto metrics = evaluator.evaluateSingle("foo bar", "foo baz");
    EXPECT_TRUE(metrics.details.contains("semantic_similarity"));
    EXPECT_TRUE(metrics.details.contains("exact_match"));
    EXPECT_TRUE(metrics.details.contains("partial_match"));
    EXPECT_TRUE(metrics.details.contains("relevance"));
}

// ============================================================================
// evaluateBatch
// ============================================================================

TEST_F(PromptEvaluatorTest, EvaluateBatch_EmptyInputs) {
    PromptEvaluator evaluator(config_);
    auto agg = evaluator.evaluateBatch({}, {});
    EXPECT_DOUBLE_EQ(agg.overall_score, 0.0);
}

TEST_F(PromptEvaluatorTest, EvaluateBatch_SizeMismatch) {
    PromptEvaluator evaluator(config_);
    auto agg = evaluator.evaluateBatch({"a", "b"}, {"a"});
    EXPECT_DOUBLE_EQ(agg.overall_score, 0.0);
}

TEST_F(PromptEvaluatorTest, EvaluateBatch_ScoresInRange) {
    PromptEvaluator evaluator(config_);
    std::vector<std::string> outputs  = {"Paris", "Berlin", "Rome"};
    std::vector<std::string> expected = {"Paris", "Berlin", "Paris"};
    auto agg = evaluator.evaluateBatch(outputs, expected);
    EXPECT_GE(agg.overall_score, 0.0);
    EXPECT_LE(agg.overall_score, 1.0);
    EXPECT_GE(agg.mean_similarity, 0.0);
    EXPECT_GE(agg.std_similarity, 0.0);
    EXPECT_EQ(agg.num_exact_matches, 2u);   // First two entries match exactly
}

TEST_F(PromptEvaluatorTest, EvaluateBatch_PassRate) {
    PromptEvaluator evaluator(config_);
    // All identical → all pass
    std::vector<std::string> outputs  = {"hello world", "hello world"};
    std::vector<std::string> expected = {"hello world", "hello world"};
    auto agg = evaluator.evaluateBatch(outputs, expected);
    EXPECT_NEAR(agg.pass_rate, 1.0, 1e-9);
}

TEST_F(PromptEvaluatorTest, EvaluateBatch_PerCaseMetrics) {
    PromptEvaluator evaluator(config_);
    std::vector<std::string> outputs  = {"a", "b"};
    std::vector<std::string> expected = {"a", "b"};
    auto agg = evaluator.evaluateBatch(outputs, expected);
    EXPECT_TRUE(agg.per_case_metrics.is_array());
    EXPECT_EQ(agg.per_case_metrics.size(), 2u);
}

// ============================================================================
// isStatisticallySignificant
// ============================================================================

TEST_F(PromptEvaluatorTest, StatSig_ClearImprovement) {
    std::vector<double> baseline = {0.5, 0.5, 0.5, 0.5, 0.5};
    std::vector<double> improved = {0.8, 0.8, 0.8, 0.8, 0.8};
    EXPECT_TRUE(PromptEvaluator::isStatisticallySignificant(baseline, improved));
}

TEST_F(PromptEvaluatorTest, StatSig_NoImprovement) {
    std::vector<double> baseline = {0.8, 0.8, 0.8};
    std::vector<double> same     = {0.8, 0.8, 0.8};
    EXPECT_FALSE(PromptEvaluator::isStatisticallySignificant(baseline, same));
}

TEST_F(PromptEvaluatorTest, StatSig_EmptyInputs) {
    EXPECT_FALSE(PromptEvaluator::isStatisticallySignificant({}, {}));
}

TEST_F(PromptEvaluatorTest, StatSig_SmallDifference) {
    // <5% improvement → not significant
    std::vector<double> baseline = {0.80, 0.80, 0.80};
    std::vector<double> slight   = {0.82, 0.82, 0.82};
    EXPECT_FALSE(PromptEvaluator::isStatisticallySignificant(baseline, slight));
}

// ============================================================================
// setConfig / getConfig-style via constructor
// ============================================================================

TEST_F(PromptEvaluatorTest, ConfigPassThreshold) {
    EvaluatorConfig strict_cfg;
    strict_cfg.pass_threshold = 0.99;
    PromptEvaluator strict_evaluator(strict_cfg);

    // Two completely different strings should not pass with 0.99 threshold
    std::vector<std::string> outputs  = {"foo"};
    std::vector<std::string> expected = {"bar"};
    auto agg = strict_evaluator.evaluateBatch(outputs, expected);
    EXPECT_NEAR(agg.pass_rate, 0.0, 1e-9);
}

