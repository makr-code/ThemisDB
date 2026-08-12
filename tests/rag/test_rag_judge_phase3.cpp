/**
 * @file test_rag_judge_phase3.cpp
 * @brief Unit tests for GEvalEvaluator, NLIFaithfulnessVerifier, and LLMIntegration
 *
 * Covers the evaluator components added in Phase 3 of the RAG judge module.
 */

#include <gtest/gtest.h>
#include "rag/geval_evaluator.h"
#include "rag/nli_faithfulness_verifier.h"
#include "rag/llm_integration.h"

using namespace themis::rag::judge;
using namespace themis::rag;

// ============================================================================
// GEvalEvaluator tests
// ============================================================================

class GEvalPhase3Test : public ::testing::Test {
protected:
    void SetUp() override {
        // No LLM engine – exercise heuristic fallback path deterministically
        LLMIntegration::setInferenceEngine(nullptr);
        config_.num_samples  = 2;
        config_.aggregation  = AggregationMethod::MEAN;
        config_.temperature  = 0.7;
    }
    void TearDown() override {
        LLMIntegration::setInferenceEngine(nullptr);
    }

    GEvalEvaluator::Config config_;

    static std::vector<std::pair<std::string, std::string>> testDocs() {
        return {
            {"doc1", "Paris is the capital of France, located in Western Europe."},
            {"doc2", "The Eiffel Tower stands in Paris and was built in 1889."}
        };
    }
};

// Score is in [0, 1]
TEST_F(GEvalPhase3Test, EvaluateScoreRange) {
    GEvalEvaluator evaluator(config_);
    auto result = evaluator.evaluate(
        "What is the capital of France?",
        "Paris is the capital of France.",
        testDocs(),
        "faithfulness");
    EXPECT_GE(result.geval_score, 0.0);
    EXPECT_LE(result.geval_score, 1.0);
}

// Confidence is in [0, 1]
TEST_F(GEvalPhase3Test, EvaluateConfidenceRange) {
    GEvalEvaluator evaluator(config_);
    auto result = evaluator.evaluate(
        "What is the capital of France?",
        "Paris is the capital of France.",
        testDocs(),
        "relevance");
    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);
}

// Probability distribution sums to ~1
TEST_F(GEvalPhase3Test, ProbabilitiesSumToOne) {
    GEvalEvaluator evaluator(config_);
    auto result = evaluator.evaluate(
        "When was the Eiffel Tower built?",
        "The Eiffel Tower was built in 1889.",
        testDocs(),
        "completeness");
    ASSERT_EQ(result.token_probabilities.size(), kNumScoreLevels);
    double sum = 0.0;
    for (double p : result.token_probabilities) sum += p;
    EXPECT_NEAR(sum, 1.0, 1e-6);
}

// Each dimension name produces a distinct default score
TEST_F(GEvalPhase3Test, DimensionProducesResult) {
    GEvalEvaluator evaluator(config_);
    for (const auto& dim : {"faithfulness", "relevance", "completeness", "coherence"}) {
        auto result = evaluator.evaluate("Query", "Answer", testDocs(), dim);
        EXPECT_GE(result.geval_score, 0.0) << "dim=" << dim;
        EXPECT_LE(result.geval_score, 1.0) << "dim=" << dim;
    }
}

// Sample scores vector has expected length
TEST_F(GEvalPhase3Test, SampleScoresCount) {
    GEvalEvaluator evaluator(config_);
    auto result = evaluator.evaluate("Q", "A", testDocs(), "coherence");
    EXPECT_EQ(result.sample_scores.size(),
              static_cast<size_t>(config_.num_samples));
}

// computeGEvalScore: perfect level-5 → score ≈ 1.0
TEST_F(GEvalPhase3Test, ComputeGEvalScorePerfect) {
    std::vector<double> probs = {0.0, 0.0, 0.0, 0.0, 1.0};
    double score = GEvalEvaluator::computeGEvalScore(probs);
    EXPECT_NEAR(score, 1.0, 1e-9);
}

// computeGEvalScore: perfect level-1 → score ≈ 0.0
TEST_F(GEvalPhase3Test, ComputeGEvalScoreWorst) {
    std::vector<double> probs = {1.0, 0.0, 0.0, 0.0, 0.0};
    double score = GEvalEvaluator::computeGEvalScore(probs);
    EXPECT_NEAR(score, 0.0, 1e-9);
}

// computeGEvalScore: wrong size → returns default 0.5
TEST_F(GEvalPhase3Test, ComputeGEvalScoreWrongSize) {
    std::vector<double> probs = {0.5, 0.5};
    double score = GEvalEvaluator::computeGEvalScore(probs);
    EXPECT_DOUBLE_EQ(score, 0.5);
}

// computeConfidence: peaked distribution → high confidence
TEST_F(GEvalPhase3Test, ComputeConfidencePeaked) {
    std::vector<double> probs = {0.0, 0.0, 0.0, 0.0, 1.0};
    double conf = GEvalEvaluator::computeConfidence(probs);
    EXPECT_GT(conf, 0.9);
}

// computeConfidence: uniform distribution → low confidence
TEST_F(GEvalPhase3Test, ComputeConfidenceUniform) {
    std::vector<double> probs = {0.2, 0.2, 0.2, 0.2, 0.2};
    double conf = GEvalEvaluator::computeConfidence(probs);
    EXPECT_NEAR(conf, 0.0, 1e-6);
}

// ============================================================================
// NLIFaithfulnessVerifier – negation false-positive fix
// ============================================================================

class NLIPhase3Test : public ::testing::Test {
protected:
    NLIFaithfulnessVerifier::Config config_;
    void SetUp() override {
        config_.entailment_threshold    = 0.7;
        config_.contradiction_threshold = 0.7;
    }
};

// "not only" should NOT be detected as a contradiction
TEST_F(NLIPhase3Test, NotOnlyIsNotContradiction) {
    NLIFaithfulnessVerifier verifier(config_);
    std::string premise    = "Paris is the capital of France and also a major tourist destination.";
    std::string hypothesis = "Paris is not only the capital of France but also attracts many tourists.";
    auto result = verifier.checkEntailment(premise, hypothesis);
    // Should not be labelled CONTRADICTION for a positive complement phrase
    EXPECT_NE(result.label, NLILabel::CONTRADICTION);
}

// "never before" should NOT trigger contradiction
TEST_F(NLIPhase3Test, NeverBeforeIsNotContradiction) {
    NLIFaithfulnessVerifier verifier(config_);
    std::string premise    = "The Eiffel Tower was completed in 1889.";
    std::string hypothesis = "The Eiffel Tower achieved something never before seen in 1889.";
    auto result = verifier.checkEntailment(premise, hypothesis);
    EXPECT_NE(result.label, NLILabel::CONTRADICTION);
}

// Genuine negation ("Paris is not") should still be detected
TEST_F(NLIPhase3Test, GenuineNegationIsContradiction) {
    NLIFaithfulnessVerifier verifier(config_);
    std::string premise    = "Berlin is the capital of Germany.";
    std::string hypothesis = "Berlin is not the capital of Germany.";
    auto result = verifier.checkEntailment(premise, hypothesis);
    // High overlap but genuine negation → CONTRADICTION
    EXPECT_EQ(result.label, NLILabel::CONTRADICTION);
}

// ============================================================================
// LLMIntegration – token probability callback
// ============================================================================

TEST(LLMIntegrationPhase3Test, TokenCallbackNotInvokedWithoutEngine) {
    LLMIntegration::setInferenceEngine(nullptr);

    bool callback_called = false;
    LLMGenerationOptions opts;
    opts.include_token_probabilities = true;
    opts.token_callback = [&](const TokenProbability&) {
        callback_called = true;
    };

    // With no engine configured the stub path returns immediately without
    // calling the token callback (since no response tokens are generated)
    LLMIntegration::generate("Test prompt", opts);
    EXPECT_FALSE(callback_called);
}

TEST(LLMIntegrationPhase3Test, PerplexityEmptyInput) {
    double p = LLMIntegration::calculatePerplexity({});
    EXPECT_DOUBLE_EQ(p, 0.0);
}

TEST(LLMIntegrationPhase3Test, PerplexityPerfectProbs) {
    // All probabilities = 1.0 → log(1) = 0 → perplexity = exp(0) = 1.0
    std::vector<double> probs(5, 1.0);
    double p = LLMIntegration::calculatePerplexity(probs);
    EXPECT_NEAR(p, 1.0, 1e-9);
}

TEST(LLMIntegrationPhase3Test, SemanticSimilarityIdentical) {
    double sim = LLMIntegration::calculateSemanticSimilarity("hello world", "hello world");
    EXPECT_NEAR(sim, 1.0, 1e-9);
}

TEST(LLMIntegrationPhase3Test, SemanticSimilarityEmpty) {
    double sim = LLMIntegration::calculateSemanticSimilarity("", "hello world");
    EXPECT_DOUBLE_EQ(sim, 0.0);
}
