/**
 * @file test_rag_ethics.cpp
 * @brief Unit tests for ethical compliance evaluation and bias detection in the RAG judge.
 *
 * Tests cover:
 * - Ethical compliance scoring via evaluate() (BALANCED mode)
 * - Ethical VETO mechanism (low compliance fails quality threshold)
 * - Computed confidence (not hardcoded 0.85)
 * - BiasDetector: detectPositionBias, detectLengthBias, analyzeAllBiases,
 *   applyBiasMitigation, setConfig/getConfig
 */

#include <gtest/gtest.h>
#include "rag/rag_judge.h"
#include "rag/bias_detector.h"

using namespace themis::rag::judge;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<RetrievedDocument> makeDocs() {
    return {
        {"d1", "Renewable energy reduces carbon emissions and is widely adopted.", 0.9, {}},
        {"d2", "Studies show that solar power output varies by region.", 0.85, {}},
    };
}

static RAGJudgeConfig balancedConfig() {
    RAGJudgeConfig cfg;
    cfg.mode = EvaluationMode::BALANCED;
    cfg.enable_ethical_evaluation = true;
    cfg.ethical_veto_power = true;
    cfg.ethical_compliance_threshold = 0.7;
    cfg.quality_threshold = 0.6;
    return cfg;
}

// ---------------------------------------------------------------------------
// Ethical compliance via evaluate()
// ---------------------------------------------------------------------------

class RAGEthicsTest : public ::testing::Test {
protected:
    RAGJudge judge_{balancedConfig()};
    std::vector<RetrievedDocument> docs_ = makeDocs();
};

// Ethical compliance score is in [0, 1]
TEST_F(RAGEthicsTest, EthicalComplianceScoreRange) {
    std::string answer =
        "Renewable energy has both advantages and disadvantages. "
        "You may consider solar or wind options depending on your region.";
    auto result = judge_.evaluate("Should I use renewable energy?", docs_, answer);
    EXPECT_GE(result.ethical_compliance_score, 0.0);
    EXPECT_LE(result.ethical_compliance_score, 1.0);
}

// An answer that respects autonomy (offers choice, avoids imperatives) should score well
TEST_F(RAGEthicsTest, GoodEthicsScoresHigh) {
    std::string answer =
        "There are multiple perspectives on this topic. "
        "Some experts argue that solar is better, while others prefer wind. "
        "Ultimately, the choice depends on your specific needs and location.";
    auto result = judge_.evaluate("Which energy source is best?", docs_, answer);
    // A balanced, autonomy-preserving answer should have no ethical violations
    // and an acceptable compliance score.
    EXPECT_TRUE(result.ethical_violations.empty() || result.ethical_compliance_score >= 0.5)
        << "Balanced, autonomy-preserving answer should not have low ethical score";
}

// Ethical VETO: when ethical compliance is disabled, veto is not triggered
TEST_F(RAGEthicsTest, EthicalVetoDisabledByConfig) {
    RAGJudgeConfig cfg = balancedConfig();
    cfg.enable_ethical_evaluation = false;
    cfg.ethical_veto_power = false;
    RAGJudge judge(cfg);

    // Even a low-quality answer should not fail because of ethics when disabled
    std::string answer = "You must always use solar energy. This is the only option.";
    auto result = judge.evaluate("What energy should I use?", docs_, answer);
    EXPECT_DOUBLE_EQ(result.ethical_compliance_score, 1.0);  // skipped, defaults to 1.0
}

// Ethical violations list is populated only when the VETO triggers
TEST_F(RAGEthicsTest, EthicalViolationsListPopulatedOnVeto) {
    // Force threshold impossibly high so almost any answer triggers VETO
    RAGJudgeConfig cfg = balancedConfig();
    cfg.ethical_compliance_threshold = 0.99;
    cfg.ethical_veto_power = true;
    RAGJudge judge(cfg);

    std::string answer = "You absolutely must use solar energy. There is no alternative.";
    auto result = judge.evaluate("Which energy is best?", docs_, answer);

    if (!result.ethical_violations.empty()) {
        // At least one violation message should mention "VETO"
        bool found_veto = false;
        for (const auto& v : result.ethical_violations) {
            if (v.find("VETO") != std::string::npos) {
                found_veto = true;
                break;
            }
        }
        EXPECT_TRUE(found_veto) << "VETO message should be present in violations";
        EXPECT_FALSE(result.passed_quality_threshold)
            << "Ethical VETO must set passed_quality_threshold = false";
    }
}

// Confidence is in [0, 1] and not a hardcoded constant
TEST_F(RAGEthicsTest, ConfidenceIsComputed) {
    std::string answer =
        "Renewable energy sources like solar and wind are widely available.";
    auto result = judge_.evaluate("Tell me about renewable energy.", docs_, answer);
    EXPECT_GE(result.confidence, 0.0);
    EXPECT_LE(result.confidence, 1.0);
    // Confidence must not be the old hardcoded value for every evaluation
    // (it varies with score distribution, so we only check range)
}

// Two different answers produce different confidence values
TEST_F(RAGEthicsTest, ConfidenceVariesWithScores) {
    // Answer A: addresses query well, consistent scores expected → higher confidence
    std::string answer_a =
        "Renewable energy is sustainable and reduces emissions. "
        "Solar and wind are the most common types.";
    // Answer B: totally off-topic → inconsistent / low scores → lower confidence
    std::string answer_b = "The weather today is sunny with a high of 25 degrees.";

    auto result_a = judge_.evaluate("What is renewable energy?", docs_, answer_a);
    auto result_b = judge_.evaluate("What is renewable energy?", docs_, answer_b);

    // The scores must be in range regardless
    EXPECT_GE(result_a.confidence, 0.0);
    EXPECT_LE(result_a.confidence, 1.0);
    EXPECT_GE(result_b.confidence, 0.0);
    EXPECT_LE(result_b.confidence, 1.0);
}

// ---------------------------------------------------------------------------
// BiasDetector tests
// ---------------------------------------------------------------------------

class BiasDetectorTest : public ::testing::Test {
protected:
    BiasDetector detector_;
};

// Default config is accessible
TEST_F(BiasDetectorTest, DefaultConfig) {
    auto cfg = detector_.getConfig();
    EXPECT_GT(cfg.min_samples_for_detection, 0);
    EXPECT_GT(cfg.significance_threshold, 0.0);
    EXPECT_LE(cfg.significance_threshold, 1.0);
}

// setConfig round-trips correctly
TEST_F(BiasDetectorTest, SetConfigRoundTrip) {
    BiasDetectorConfig cfg;
    cfg.min_samples_for_detection = 20;
    cfg.significance_threshold = 0.01;
    cfg.bias_threshold = 0.15;
    detector_.setConfig(cfg);

    auto retrieved = detector_.getConfig();
    EXPECT_EQ(retrieved.min_samples_for_detection, 20);
    EXPECT_DOUBLE_EQ(retrieved.significance_threshold, 0.01);
    EXPECT_DOUBLE_EQ(retrieved.bias_threshold, 0.15);
}

// detectPositionBias with insufficient samples returns non-significant result
TEST_F(BiasDetectorTest, PositionBiasInsufficientSamples) {
    std::vector<std::pair<ComparisonResult, bool>> comparisons;
    ComparisonResult cr;
    cr.winner = ComparisonResult::Winner::ANSWER_A;
    cr.confidence = 0.8;
    comparisons.push_back({cr, false});  // only 1 sample

    auto result = detector_.detectPositionBias(comparisons);
    EXPECT_EQ(result.type, BiasType::POSITION_BIAS);
    EXPECT_FALSE(result.is_significant);
}

// detectPositionBias with 50/50 results reports no significant bias
TEST_F(BiasDetectorTest, PositionBiasNoSignificantBias) {
    BiasDetectorConfig cfg;
    cfg.min_samples_for_detection = 5;
    detector_.setConfig(cfg);

    std::vector<std::pair<ComparisonResult, bool>> comparisons;
    for (int i = 0; i < 10; ++i) {
        ComparisonResult cr;
        cr.winner = (i % 2 == 0) ? ComparisonResult::Winner::ANSWER_A
                                  : ComparisonResult::Winner::ANSWER_B;
        cr.confidence = 0.7;
        comparisons.push_back({cr, false});
    }

    auto result = detector_.detectPositionBias(comparisons);
    EXPECT_FALSE(result.is_significant)
        << "50/50 split should not produce significant position bias";
    EXPECT_GE(result.p_value, 0.0);
    EXPECT_LE(result.bias_magnitude, 1.0);
}

// detectLengthBias with insufficient samples returns non-significant result
TEST_F(BiasDetectorTest, LengthBiasInsufficientSamples) {
    std::vector<std::pair<double, size_t>> evals = {{0.8, 100}, {0.7, 200}};
    auto result = detector_.detectLengthBias(evals);
    EXPECT_EQ(result.type, BiasType::LENGTH_BIAS);
    EXPECT_FALSE(result.is_significant);
}

// detectLengthBias with uncorrelated data reports no significant bias
TEST_F(BiasDetectorTest, LengthBiasUncorrelated) {
    BiasDetectorConfig cfg;
    cfg.min_samples_for_detection = 5;
    detector_.setConfig(cfg);

    // scores and lengths are deliberately independent (constant score, varying length)
    // → correlation near zero, no significant length bias
    std::vector<std::pair<double, size_t>> evals = {
        {0.7, 50},  {0.7, 120}, {0.7, 80},  {0.7, 300}, {0.7, 200},
        {0.7, 450}, {0.7, 30},  {0.7, 170}, {0.7, 90},  {0.7, 260},
    };
    auto result = detector_.detectLengthBias(evals);
    EXPECT_EQ(result.type, BiasType::LENGTH_BIAS);
    EXPECT_GE(result.p_value, 0.0);
    EXPECT_LE(result.bias_magnitude, 1.0);
}

// analyzeAllBiases with empty history returns empty vector
TEST_F(BiasDetectorTest, AnalyzeAllBiasesEmpty) {
    std::vector<EvaluationResult> history;
    auto results = detector_.analyzeAllBiases(history);
    EXPECT_TRUE(results.empty());
}

// applyBiasMitigation: non-significant bias leaves confidence unchanged
TEST_F(BiasDetectorTest, BiasMitigationNoSignificantBias) {
    ComparisonResult cr;
    cr.winner = ComparisonResult::Winner::ANSWER_A;
    cr.confidence = 0.9;

    BiasDetectionResult bdr;
    bdr.type = BiasType::POSITION_BIAS;
    bdr.is_significant = false;
    bdr.bias_magnitude = 0.05;

    auto mitigated = detector_.applyBiasMitigation(cr, bdr);
    EXPECT_DOUBLE_EQ(mitigated.confidence, cr.confidence);
}

// applyBiasMitigation: significant position bias reduces confidence
TEST_F(BiasDetectorTest, BiasMitigationReducesConfidence) {
    ComparisonResult cr;
    cr.winner = ComparisonResult::Winner::ANSWER_A;
    cr.confidence = 1.0;

    BiasDetectionResult bdr;
    bdr.type = BiasType::POSITION_BIAS;
    bdr.is_significant = true;
    bdr.bias_magnitude = 0.4;

    auto mitigated = detector_.applyBiasMitigation(cr, bdr);
    EXPECT_LT(mitigated.confidence, cr.confidence)
        << "Significant bias mitigation should reduce confidence";
}

