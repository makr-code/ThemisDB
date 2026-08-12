/**
 * @file test_rag_adaptive_retrieval.cpp
 * @brief Unit tests for AdaptiveRetrieval (adaptive retrieval depth, Phase 7).
 *
 * Test suite: AdaptiveRetrievalFocusedTests (16 tests)
 *   Group A (5) – Configuration, factory helpers
 *   Group B (5) – Complexity analysis (heuristic scoring)
 *   Group C (6) – Parameter computation and custom scorer injection
 */

#include <gtest/gtest.h>
#include "rag/adaptive_retrieval.h"

#include <cmath>
#include <limits>
#include <string>

using namespace themis::rag;

// ══════════════════════════════════════════════════════════════════════════════
// Group A – Configuration and factory helpers
// ══════════════════════════════════════════════════════════════════════════════

TEST(AdaptiveRetrievalFocusedTests, A1_DefaultConfigValues)
{
    AdaptiveRetrievalConfig cfg;
    EXPECT_EQ(cfg.base_top_k, 5u);
    EXPECT_EQ(cfg.max_top_k, 20u);
    EXPECT_DOUBLE_EQ(cfg.base_similarity_threshold, 0.75);
    EXPECT_DOUBLE_EQ(cfg.min_similarity_threshold, 0.40);
    EXPECT_DOUBLE_EQ(cfg.complexity_scaling, 1.5);
}

TEST(AdaptiveRetrievalFocusedTests, A2_SetAndGetConfig)
{
    AdaptiveRetrievalConfig cfg;
    cfg.base_top_k = 3;
    cfg.max_top_k  = 12;
    AdaptiveRetrieval ar(cfg);
    EXPECT_EQ(ar.getConfig().base_top_k, 3u);
    EXPECT_EQ(ar.getConfig().max_top_k, 12u);

    AdaptiveRetrievalConfig cfg2;
    cfg2.base_top_k = 7;
    ar.setConfig(cfg2);
    EXPECT_EQ(ar.getConfig().base_top_k, 7u);
}

TEST(AdaptiveRetrievalFocusedTests, A3_FactoryLightweight)
{
    auto ar = AdaptiveRetrievalFactory::createLightweight();
    ASSERT_NE(ar, nullptr);
    EXPECT_LT(ar->getConfig().base_top_k, 5u);
    EXPECT_LE(ar->getConfig().max_top_k, 10u);
}

TEST(AdaptiveRetrievalFocusedTests, A4_FactoryBalanced)
{
    auto ar = AdaptiveRetrievalFactory::createBalanced();
    ASSERT_NE(ar, nullptr);
    EXPECT_EQ(ar->getConfig().base_top_k, 5u);
    EXPECT_LE(ar->getConfig().max_top_k, 20u);
}

TEST(AdaptiveRetrievalFocusedTests, A5_FactoryHighRecall)
{
    auto ar = AdaptiveRetrievalFactory::createHighRecall();
    ASSERT_NE(ar, nullptr);
    EXPECT_GE(ar->getConfig().base_top_k, 5u);
    EXPECT_GE(ar->getConfig().max_top_k, 20u);
    EXPECT_LE(ar->getConfig().base_similarity_threshold, 0.70);
}

// ══════════════════════════════════════════════════════════════════════════════
// Group B – Complexity analysis
// ══════════════════════════════════════════════════════════════════════════════

TEST(AdaptiveRetrievalFocusedTests, B1_EmptyQueryIsSimple)
{
    AdaptiveRetrieval ar;
    const auto a = ar.analyzeComplexity("");
    EXPECT_EQ(a.complexity, QueryComplexity::SIMPLE);
    EXPECT_DOUBLE_EQ(a.raw_score, 0.0);
}

TEST(AdaptiveRetrievalFocusedTests, B2_SimpleOneFactQuery)
{
    AdaptiveRetrieval ar;
    const auto a = ar.analyzeComplexity("What is the capital of France?");
    // No connectives, no extra question words → SIMPLE
    EXPECT_EQ(a.complexity, QueryComplexity::SIMPLE);
    EXPECT_GE(a.raw_score, 0.0);
    EXPECT_LT(a.raw_score, 0.30);
}

TEST(AdaptiveRetrievalFocusedTests, B3_ModerateQueryWithConnective)
{
    AdaptiveRetrieval ar;
    // "and" is one connective → pushes score up
    const auto a = ar.analyzeComplexity(
        "Who invented the transistor and when was it first patented?");
    // raw_score should reflect at least one connective
    EXPECT_GE(a.raw_score, 0.10);
    EXPECT_FALSE(a.explanation.empty());
}

TEST(AdaptiveRetrievalFocusedTests, B4_ComplexQueryMultipleConnectives)
{
    AdaptiveRetrieval ar;
    // Multiple connectives → score should be >= 0.30 (MODERATE or higher)
    const auto a = ar.analyzeComplexity(
        "Why did the market crash and how did it recover although regulators "
        "were aware but failed to act while investors panicked?");
    EXPECT_GE(a.raw_score, 0.30);
    EXPECT_NE(a.complexity, QueryComplexity::SIMPLE);
}

TEST(AdaptiveRetrievalFocusedTests, B5_ScoreToComplexityThresholds)
{
    EXPECT_EQ(AdaptiveRetrieval::scoreToComplexity(0.00), QueryComplexity::SIMPLE);
    EXPECT_EQ(AdaptiveRetrieval::scoreToComplexity(0.29), QueryComplexity::SIMPLE);
    EXPECT_EQ(AdaptiveRetrieval::scoreToComplexity(0.30), QueryComplexity::MODERATE);
    EXPECT_EQ(AdaptiveRetrieval::scoreToComplexity(0.54), QueryComplexity::MODERATE);
    EXPECT_EQ(AdaptiveRetrieval::scoreToComplexity(0.55), QueryComplexity::COMPLEX);
    EXPECT_EQ(AdaptiveRetrieval::scoreToComplexity(0.74), QueryComplexity::COMPLEX);
    EXPECT_EQ(AdaptiveRetrieval::scoreToComplexity(0.75), QueryComplexity::VERY_COMPLEX);
    EXPECT_EQ(AdaptiveRetrieval::scoreToComplexity(1.00), QueryComplexity::VERY_COMPLEX);
}

// ══════════════════════════════════════════════════════════════════════════════
// Group C – Parameter computation and scorer injection
// ══════════════════════════════════════════════════════════════════════════════

TEST(AdaptiveRetrievalFocusedTests, C1_SimpleQueryGetsBaseTopK)
{
    AdaptiveRetrievalConfig cfg;
    cfg.base_top_k = 5;
    AdaptiveRetrieval ar(cfg);

    const auto params = ar.computeParams("What is X?");
    EXPECT_EQ(params.top_k, 5u);
    EXPECT_DOUBLE_EQ(params.similarity_threshold,
                     cfg.base_similarity_threshold);
}

TEST(AdaptiveRetrievalFocusedTests, C2_ComplexQueryGetsHigherTopK)
{
    AdaptiveRetrieval ar;
    const auto simple_params  = ar.computeParams("What is X?");
    const auto complex_params = ar.computeParams(
        "Why did event A happen and how did it affect B while C remained "
        "unchanged although D suggested otherwise?");

    EXPECT_GT(complex_params.top_k, simple_params.top_k);
    EXPECT_LE(complex_params.similarity_threshold,
              simple_params.similarity_threshold);
}

TEST(AdaptiveRetrievalFocusedTests, C3_TopKCappedAtMaxTopK)
{
    AdaptiveRetrievalConfig cfg;
    cfg.base_top_k       = 10;
    cfg.max_top_k        = 12;
    cfg.complexity_scaling = 3.0;
    AdaptiveRetrieval ar(cfg);

    const auto params = ar.computeParams(
        "Why and because although but while since however therefore "
        "furthermore moreover whereas unless whether");
    // Even very complex query is capped at max_top_k
    EXPECT_LE(params.top_k, cfg.max_top_k);
}

TEST(AdaptiveRetrievalFocusedTests, C4_CustomScorerOverridesHeuristic)
{
    struct AlwaysHighScorer : IComplexityScorer {
        double score(const std::string& /*query*/) override { return 0.95; }
    };

    AdaptiveRetrieval ar;
    AlwaysHighScorer scorer;
    ar.setScorer(&scorer);

    const auto params = ar.computeParams("Simple question");
    // Custom scorer forces VERY_COMPLEX regardless of query text
    EXPECT_EQ(params.analysis.complexity, QueryComplexity::VERY_COMPLEX);
    EXPECT_GT(params.top_k, 5u);
}

TEST(AdaptiveRetrievalFocusedTests, C5_NullScorerFallsBackToHeuristic)
{
    AdaptiveRetrieval ar;
    ar.setScorer(nullptr);  // Explicitly remove any scorer

    const auto params = ar.computeParams("What is the speed of light?");
    // Should still produce valid parameters via heuristic
    EXPECT_GT(params.top_k, 0u);
    EXPECT_GT(params.similarity_threshold, 0.0);
    EXPECT_LE(params.similarity_threshold, 1.0);
    EXPECT_FALSE(params.analysis.explanation.empty());
}

TEST(AdaptiveRetrievalFocusedTests, C6_InvalidConfigIsClampedToSafeBounds)
{
    AdaptiveRetrievalConfig cfg;
    cfg.base_top_k = 0;
    cfg.max_top_k = 0;
    cfg.base_similarity_threshold = std::numeric_limits<double>::infinity();
    cfg.min_similarity_threshold = 0.90;
    cfg.complexity_scaling = std::numeric_limits<double>::quiet_NaN();

    AdaptiveRetrieval ar(cfg);
    const auto ctorCfg = ar.getConfig();
    EXPECT_EQ(ctorCfg.base_top_k, 1u);
    EXPECT_EQ(ctorCfg.max_top_k, 1u);
    EXPECT_DOUBLE_EQ(ctorCfg.complexity_scaling, 1.0);
    EXPECT_DOUBLE_EQ(ctorCfg.base_similarity_threshold, 0.75);
    EXPECT_DOUBLE_EQ(ctorCfg.min_similarity_threshold, 0.75);

    AdaptiveRetrievalConfig cfg2;
    cfg2.base_top_k = 4;
    cfg2.max_top_k = 2;
    cfg2.base_similarity_threshold = -1.0;
    cfg2.min_similarity_threshold = 2.0;
    cfg2.complexity_scaling = -3.0;
    ar.setConfig(cfg2);

    const auto setCfg = ar.getConfig();
    EXPECT_EQ(setCfg.base_top_k, 4u);
    EXPECT_EQ(setCfg.max_top_k, 4u);
    EXPECT_DOUBLE_EQ(setCfg.complexity_scaling, 1.0);
    EXPECT_DOUBLE_EQ(setCfg.base_similarity_threshold, 0.0);
    EXPECT_DOUBLE_EQ(setCfg.min_similarity_threshold, 0.0);

    const auto params = ar.computeParams(
        "Why and because although but while since however therefore");
    EXPECT_GE(params.top_k, setCfg.base_top_k);
    EXPECT_LE(params.top_k, setCfg.max_top_k);
    EXPECT_GE(params.similarity_threshold, 0.0);
    EXPECT_LE(params.similarity_threshold, 1.0);
    EXPECT_TRUE(std::isfinite(params.similarity_threshold));
}
