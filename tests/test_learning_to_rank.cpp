/**
 * @file test_learning_to_rank.cpp
 * @brief Unit tests for LearningToRank (v1.5.0)
 */

#include <gtest/gtest.h>
#include "search/learning_to_rank.h"
#include <cmath>
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Helper: build a candidate with given scores
// ============================================================================

static RankedResult makeCandidate(const std::string& id,
                                   double bm25, double vector, double rrf) {
    RankedResult r;
    r.document_id = id;
    r.features.bm25_score    = bm25;
    r.features.vector_score  = vector;
    r.features.rrf_score     = rrf;
    r.features.recency       = 0.5;
    r.features.click_count   = 0.0;
    r.features.popularity    = 0.0;
    return r;
}

// ============================================================================
// Config validation
// ============================================================================

TEST(LTRConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(LearningToRank{});
}

TEST(LTRConfig, ZeroLearningRateThrows) {
    LearningToRank::Config cfg;
    cfg.learning_rate = 0.0;
    EXPECT_THROW(LearningToRank{cfg}, std::invalid_argument);
}

TEST(LTRConfig, NegativeLearningRateThrows) {
    LearningToRank::Config cfg;
    cfg.learning_rate = -0.01;
    EXPECT_THROW(LearningToRank{cfg}, std::invalid_argument);
}

TEST(LTRConfig, ZeroClickBufferThrows) {
    LearningToRank::Config cfg;
    cfg.max_click_buffer = 0;
    EXPECT_THROW(LearningToRank{cfg}, std::invalid_argument);
}

TEST(LTRConfig, NegativeRegularizationThrows) {
    LearningToRank::Config cfg;
    cfg.regularization = -0.001;
    EXPECT_THROW(LearningToRank{cfg}, std::invalid_argument);
}

TEST(LTRConfig, ConfigRoundtrip) {
    LearningToRank::Config cfg;
    cfg.learning_rate = 0.05;
    LearningToRank ltr{cfg};
    EXPECT_DOUBLE_EQ(ltr.getConfig().learning_rate, 0.05);
}

// ============================================================================
// Initial weights
// ============================================================================

TEST(LTRWeights, InitialWeightsAreNonZero) {
    LearningToRank ltr;
    auto w = ltr.getWeights();
    // Primary signals start with positive weight; popularity starts at 0 (grows via training)
    EXPECT_GT(w.bm25_score, 0.0);
    EXPECT_GT(w.vector_score, 0.0);
    EXPECT_GT(w.rrf_score, 0.0);
    // Weights should sum to ~1.0 for consistent scaling
    double sum = w.bm25_score + w.vector_score + w.rrf_score
               + w.recency + w.click_count + w.popularity;
    EXPECT_NEAR(sum, 1.0, 1e-9);
}

TEST(LTRWeights, SetWeightsRoundtrip) {
    LearningToRank ltr;
    RankingFeatures w;
    w.bm25_score = 0.9;
    w.vector_score = 0.1;
    ltr.setWeights(w);
    EXPECT_DOUBLE_EQ(ltr.getWeights().bm25_score, 0.9);
    EXPECT_DOUBLE_EQ(ltr.getWeights().vector_score, 0.1);
}

// ============================================================================
// rerank()
// ============================================================================

TEST(LTRRerank, EmptyCandidatesReturnsEmpty) {
    LearningToRank ltr;
    auto result = ltr.rerank({});
    EXPECT_TRUE(result.empty());
}

TEST(LTRRerank, FinalScorePopulated) {
    LearningToRank ltr;
    std::vector<RankedResult> candidates = {
        makeCandidate("doc1", 0.8, 0.9, 0.7),
        makeCandidate("doc2", 0.2, 0.1, 0.1)
    };
    auto result = ltr.rerank(candidates);
    ASSERT_EQ(result.size(), 2u);
    for (const auto& r : result) {
        EXPECT_GT(r.final_score, 0.0);
    }
}

TEST(LTRRerank, BetterFeaturesRankedFirst) {
    LearningToRank ltr;
    // doc1 has uniformly higher features
    std::vector<RankedResult> candidates = {
        makeCandidate("doc_low",  0.1, 0.1, 0.1),
        makeCandidate("doc_high", 0.9, 0.9, 0.9)
    };
    auto result = ltr.rerank(candidates);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].document_id, "doc_high");
    EXPECT_GT(result[0].final_score, result[1].final_score);
}

TEST(LTRRerank, SingleCandidateWorks) {
    LearningToRank ltr;
    auto result = ltr.rerank({makeCandidate("only", 0.5, 0.5, 0.5)});
    ASSERT_EQ(result.size(), 1u);
    EXPECT_GT(result[0].final_score, 0.0);
}

// ============================================================================
// train()
// ============================================================================

TEST(LTRTrain, TrainWithNoClicksReturnsZero) {
    LearningToRank ltr;
    EXPECT_EQ(ltr.train(), 0u);
}

TEST(LTRTrain, TrainReturnsClickCount) {
    LearningToRank ltr;
    ltr.recordClick({"query", "doc1", 2});
    ltr.recordClick({"query", "doc2", 5});
    EXPECT_EQ(ltr.train(), 2u);
}

TEST(LTRTrain, BufferClearedAfterTrain) {
    LearningToRank ltr;
    ltr.recordClick({"q", "d", 3});
    ltr.train();
    EXPECT_EQ(ltr.train(), 0u); // buffer is empty now
}

TEST(LTRTrain, WeightsChangedAfterTrain) {
    LearningToRank ltr;
    auto weights_before = ltr.getWeights();
    // Click at position 5 → pairwise loss should fire
    for (int i = 0; i < 10; ++i) {
        ltr.recordClick({"ml database", "doc_" + std::to_string(i), 5});
    }
    ltr.train();
    auto weights_after = ltr.getWeights();
    // At least one weight component should have changed
    bool changed = (weights_before.bm25_score   != weights_after.bm25_score   ||
                    weights_before.vector_score  != weights_after.vector_score  ||
                    weights_before.click_count   != weights_after.click_count   ||
                    weights_before.popularity    != weights_after.popularity);
    EXPECT_TRUE(changed);
}

TEST(LTRTrain, ClickAtPositionZeroNoUpdate) {
    LearningToRank ltr;
    auto weights_before = ltr.getWeights();
    // Clicks at position 0 are skipped (no pairwise loss)
    for (int i = 0; i < 5; ++i) {
        ltr.recordClick({"query", "doc", 0});
    }
    ltr.train();
    auto weights_after = ltr.getWeights();
    // With regularization, weights may shrink slightly; but click_count shouldn't grow
    EXPECT_DOUBLE_EQ(weights_before.click_count, weights_after.click_count);
}

TEST(LTRTrain, MaxClickBufferEvictsOldest) {
    LearningToRank::Config cfg;
    cfg.max_click_buffer = 3;
    LearningToRank ltr{cfg};
    for (int i = 0; i < 5; ++i) {
        ltr.recordClick({"q", "d" + std::to_string(i), 2});
    }
    // Buffer capped at 3 → train returns 3
    EXPECT_EQ(ltr.train(), 3u);
}

// ============================================================================
// A/B testing
// ============================================================================

TEST(LTRVariant, NoVariantsRegisteredReturnsEmpty) {
    LearningToRank ltr;
    EXPECT_TRUE(ltr.selectVariant("request_123").empty());
}

TEST(LTRVariant, RegisteredVariantCanBeSelected) {
    LearningToRank ltr;
    LearningToRank::Variant v;
    v.name = "ltr_v2";
    v.traffic_fraction = 1.0; // send all traffic here
    v.scorer = [](const RankingFeatures& f) { return f.bm25_score * 2.0; };
    ltr.registerVariant(v);

    // With 100% traffic fraction, this request should always get "ltr_v2"
    std::string selected = ltr.selectVariant("session_abc");
    EXPECT_EQ(selected, "ltr_v2");
}

TEST(LTRVariant, UnknownVariantFallsBackToDefault) {
    LearningToRank ltr;
    std::vector<RankedResult> candidates = {makeCandidate("d1", 0.5, 0.5, 0.5)};
    // No variant named "unknown" → falls back to default linear model
    auto result = ltr.rerankWithVariant(candidates, "unknown");
    ASSERT_EQ(result.size(), 1u);
    EXPECT_GT(result[0].final_score, 0.0);
}

TEST(LTRVariant, VariantScorerUsed) {
    LearningToRank ltr;
    LearningToRank::Variant v;
    v.name = "bm25_only";
    v.traffic_fraction = 1.0;
    v.scorer = [](const RankingFeatures& f) { return f.bm25_score; };
    ltr.registerVariant(v);

    std::vector<RankedResult> candidates = {
        makeCandidate("high_bm25",   0.9, 0.1, 0.1),
        makeCandidate("high_vector", 0.1, 0.9, 0.9)
    };
    auto result = ltr.rerankWithVariant(candidates, "bm25_only");
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0].document_id, "high_bm25");
}

// ============================================================================
// RankingFeatures struct
// ============================================================================

TEST(RankingFeatures, DefaultInitialization) {
    RankingFeatures f;
    EXPECT_DOUBLE_EQ(f.bm25_score, 0.0);
    EXPECT_DOUBLE_EQ(f.vector_score, 0.0);
    EXPECT_DOUBLE_EQ(f.rrf_score, 0.0);
    EXPECT_DOUBLE_EQ(f.recency, 0.0);
    EXPECT_DOUBLE_EQ(f.click_count, 0.0);
    EXPECT_DOUBLE_EQ(f.popularity, 0.0);
}

TEST(ClickEvent, DefaultInitialization) {
    ClickEvent ev;
    EXPECT_TRUE(ev.query.empty());
    EXPECT_TRUE(ev.document_id.empty());
    EXPECT_EQ(ev.result_position, 0u);
}
