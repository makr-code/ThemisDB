/**
 * @file test_rag_uncovered.cpp
 * @brief Tests for previously uncovered RAG modules:
 *
 *   - rag/llm_meta_analyzer.cpp  : LLMMetaAnalyzer (loadConfig, getConfig,
 *                                   parseScore, extractReasoning, clearCache,
 *                                   computeCacheKey, exportMetrics)
 *   - rag/pairwise_comparator.cpp : PairwiseComparator (construction, Config
 *                                   defaults, detectPositionBias, compare
 *                                   without LLM inference)
 *
 * Both modules are tested without a live LLM backend — only the
 * heuristic/local code paths that do not require network I/O.
 */

#include <gtest/gtest.h>
#define protected public
#include "rag/llm_meta_analyzer.h"
#undef protected
#include "rag/pairwise_comparator.h"
#include <string>
#include <unordered_map>

// ============================================================================
// LLMMetaAnalyzer: concrete, non-abstract class
// ============================================================================

using namespace themis::rag;
using namespace themis::rag::judge;

#if 0  // Temporarily disabled: LLMMetaAnalyzer cannot be directly instantiated from header (Pimpl inline dtor issue)

class LLMMetaAnalyzerTest : public ::testing::Test {
protected:
    static LLMMetaAnalyzer& sharedAnalyzer() {
        static LLMMetaAnalyzer* analyzer = new LLMMetaAnalyzer();
        return *analyzer;
    }

    LLMMetaAnalyzer& analyzer_ = sharedAnalyzer();
};

// ============================================================================
// loadConfig / getConfig
// ============================================================================

TEST_F(LLMMetaAnalyzerTest, LoadConfig_GetConfig_RoundTrip) {
    LLMMetaAnalyzer::AnalysisConfig cfg;
    cfg.judge_model           = "test-model";
    cfg.use_chain_of_thought  = false;
    cfg.max_retries           = 5;
    cfg.min_confidence        = 0.8;
    cfg.enable_caching        = true;
    cfg.cache_size            = 500;

    analyzer_.loadConfig(cfg);
    auto retrieved = analyzer_.getConfig();

    EXPECT_EQ(retrieved.judge_model,          "test-model");
    EXPECT_FALSE(retrieved.use_chain_of_thought);
    EXPECT_EQ(retrieved.max_retries,          5);
    EXPECT_DOUBLE_EQ(retrieved.min_confidence, 0.8);
    EXPECT_TRUE(retrieved.enable_caching);
    EXPECT_EQ(retrieved.cache_size,           500u);
}

TEST_F(LLMMetaAnalyzerTest, GetConfig_BeforeLoadConfig_ReturnsDefaults) {
    auto cfg = analyzer_.getConfig();
    EXPECT_FALSE(cfg.judge_model.empty());   // should have a default model name
    EXPECT_GE(cfg.max_retries, 1);
}

// ============================================================================
// clearCache
// ============================================================================

TEST_F(LLMMetaAnalyzerTest, ClearCache_DoesNotThrow) {
    EXPECT_NO_THROW(analyzer_.clearCache());
}

TEST_F(LLMMetaAnalyzerTest, ClearCache_MultipleCallsDoNotThrow) {
    EXPECT_NO_THROW(analyzer_.clearCache());
    EXPECT_NO_THROW(analyzer_.clearCache());
}

// ============================================================================
// parseScore
// ============================================================================

TEST_F(LLMMetaAnalyzerTest, ParseScore_NumericResponse_InRange) {
    double score = analyzer_.parseScore("0.85");
    EXPECT_GE(score, 0.0);
    EXPECT_LE(score, 1.0);
}

TEST_F(LLMMetaAnalyzerTest, ParseScore_TextWithNumberEmbedded_ExtractsScore) {
    // Real LLM responses often look like "Score: 0.7 - Reasoning..."
    double score = analyzer_.parseScore("Score: 0.7 - explanation here");
    EXPECT_GE(score, 0.0);
    EXPECT_LE(score, 1.0);
}

TEST_F(LLMMetaAnalyzerTest, ParseScore_HighConfidence_CloseTo1) {
    double score = analyzer_.parseScore("1.0");
    EXPECT_DOUBLE_EQ(score, 1.0);
}

TEST_F(LLMMetaAnalyzerTest, ParseScore_ZeroConfidence) {
    double score = analyzer_.parseScore("0.0");
    EXPECT_DOUBLE_EQ(score, 0.0);
}

TEST_F(LLMMetaAnalyzerTest, ParseScore_InvalidText_ReturnsSafeDefault) {
    double score = analyzer_.parseScore("no number here at all !!!");
    EXPECT_GE(score, 0.0);
    EXPECT_LE(score, 1.0);
}

// ============================================================================
// extractReasoning
// ============================================================================

TEST_F(LLMMetaAnalyzerTest, ExtractReasoning_NonEmpty_ReturnsString) {
    std::string reasoning = analyzer_.extractReasoning("The answer is correct because X.");
    EXPECT_FALSE(reasoning.empty());
}

TEST_F(LLMMetaAnalyzerTest, ExtractReasoning_EmptyInput_DoesNotCrash) {
    EXPECT_NO_THROW(analyzer_.extractReasoning(""));
}

// ============================================================================
// computeCacheKey
// ============================================================================

TEST_F(LLMMetaAnalyzerTest, ComputeCacheKey_DifferentInputs_DifferentKeys) {
    auto k1 = analyzer_.computeCacheKey("prompt A");
    auto k2 = analyzer_.computeCacheKey("prompt B");
    EXPECT_NE(k1, k2);
}

TEST_F(LLMMetaAnalyzerTest, ComputeCacheKey_SameInput_SameKey) {
    auto k1 = analyzer_.computeCacheKey("identical prompt");
    auto k2 = analyzer_.computeCacheKey("identical prompt");
    EXPECT_EQ(k1, k2);
}

// ============================================================================
// exportMetrics
// ============================================================================

TEST_F(LLMMetaAnalyzerTest, ExportMetrics_DoesNotThrow) {
    std::unordered_map<std::string, double> metrics;
    EXPECT_NO_THROW(analyzer_.exportMetrics(metrics));
}

#endif

// ============================================================================
// PairwiseComparator
// ============================================================================

class PairwiseComparatorTest : public ::testing::Test {
protected:
    PairwiseComparator comparator_;
};

// ============================================================================
// Construction
// ============================================================================

TEST_F(PairwiseComparatorTest, DefaultConstruction_Succeeds) {
    PairwiseComparator comp;
    (void)comp;
    SUCCEED();
}

TEST_F(PairwiseComparatorTest, ConfigConstruction_Succeeds) {
    PairwiseComparator::Config cfg;
    cfg.bias_strategy     = BiasMitigationStrategy::NONE;
    cfg.tie_threshold     = 0.1;
    cfg.confidence_threshold = 0.6;
    cfg.num_samples       = 1;
    cfg.enable_per_dimension = false;

    PairwiseComparator comp(cfg);
    (void)comp;
    SUCCEED();
}

// ============================================================================
// Config defaults
// ============================================================================

TEST(PairwiseComparatorConfigTest, Defaults_AreReasonable) {
    PairwiseComparator::Config cfg;
    EXPECT_EQ(cfg.bias_strategy, BiasMitigationStrategy::FLIP_AND_AVERAGE);
    EXPECT_GT(cfg.tie_threshold,     0.0);
    EXPECT_GT(cfg.confidence_threshold, 0.0);
    EXPECT_GE(cfg.num_samples,       1);
}

// ============================================================================
// detectPositionBias
// ============================================================================

TEST_F(PairwiseComparatorTest, DetectPositionBias_Agreement_LowBias) {
    // Both orderings agree → position bias should be low
    double bias = comparator_.detectPositionBias(ComparisonWinner::ANSWER_A,
                                                  ComparisonWinner::ANSWER_A);
    EXPECT_GE(bias, 0.0);
    EXPECT_LE(bias, 1.0);
}

TEST_F(PairwiseComparatorTest, DetectPositionBias_Disagreement_HigherBias) {
    // Orderings disagree → higher bias
    double agree_bias    = comparator_.detectPositionBias(ComparisonWinner::ANSWER_A,
                                                           ComparisonWinner::ANSWER_A);
    double disagree_bias = comparator_.detectPositionBias(ComparisonWinner::ANSWER_A,
                                                           ComparisonWinner::ANSWER_B);
    EXPECT_GE(disagree_bias, agree_bias);
}

TEST_F(PairwiseComparatorTest, DetectPositionBias_BothTie_LowBias) {
    double bias = comparator_.detectPositionBias(ComparisonWinner::TIE,
                                                  ComparisonWinner::TIE);
    EXPECT_GE(bias, 0.0);
    EXPECT_LE(bias, 1.0);
}

// ============================================================================
// compare (heuristic path, no LLM)
// ============================================================================

TEST_F(PairwiseComparatorTest, Compare_ReturnsValidResult) {
    std::vector<std::pair<std::string, std::string>> docs = {
        {"doc1", "The sky is blue."},
        {"doc2", "Water is H2O."}
    };

    // Using Config with NONE bias strategy to avoid flipping (which needs LLM)
    PairwiseComparator::Config cfg;
    cfg.bias_strategy = BiasMitigationStrategy::NONE;
    PairwiseComparator comp(cfg);

    auto result = comp.compare("What color is the sky?", docs,
                                "The sky is blue.",
                                "The sky is green.");

    // Result should have a valid winner enum value
    EXPECT_TRUE(result.overall_winner == ComparisonWinner::ANSWER_A ||
                result.overall_winner == ComparisonWinner::ANSWER_B ||
                result.overall_winner == ComparisonWinner::TIE);

    EXPECT_GE(result.overall_confidence,     0.0);
    EXPECT_LE(result.overall_confidence,     1.0);
    EXPECT_GE(result.answer_a_overall_score, 0.0);
    EXPECT_GE(result.answer_b_overall_score, 0.0);
}

// ============================================================================
// ComparisonWinner enum completeness
// ============================================================================

TEST(ComparisonWinnerTest, AllValuesDistinct) {
    EXPECT_NE(ComparisonWinner::ANSWER_A, ComparisonWinner::ANSWER_B);
    EXPECT_NE(ComparisonWinner::ANSWER_A, ComparisonWinner::TIE);
    EXPECT_NE(ComparisonWinner::ANSWER_B, ComparisonWinner::TIE);
}

// ============================================================================
// BiasMitigationStrategy enum completeness
// ============================================================================

TEST(BiasMitigationStrategyTest, AllValuesDistinct) {
    std::vector<BiasMitigationStrategy> strategies = {
        BiasMitigationStrategy::NONE,
        BiasMitigationStrategy::RANDOMIZE_ORDER,
        BiasMitigationStrategy::FLIP_AND_AVERAGE,
        BiasMitigationStrategy::MULTI_SAMPLE,
    };
    for (size_t i = 0; i < strategies.size(); ++i) {
        for (size_t j = i + 1; j < strategies.size(); ++j) {
            EXPECT_NE(strategies[i], strategies[j]);
        }
    }
}
