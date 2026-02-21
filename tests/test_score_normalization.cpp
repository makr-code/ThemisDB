/**
 * @file test_score_normalization.cpp
 * @brief Focused unit tests for HybridSearch::normalizeScores()
 *
 * normalizeScores() is a public static method so it can be called directly
 * without requiring live index backends.
 */

#include <gtest/gtest.h>
#include "search/hybrid_search.h"
#include <limits>
#include <vector>
#include <string>

using namespace themis;

// Shorthand helpers
static HybridSearch::Result bm25Result(const std::string& id, double score) {
    HybridSearch::Result r;
    r.document_id = id;
    r.bm25_score = score;
    return r;
}

static HybridSearch::Result vecResult(const std::string& id, double score) {
    HybridSearch::Result r;
    r.document_id = id;
    r.vector_score = score;
    return r;
}

// ============================================================================
// Empty / Single-element edge cases
// ============================================================================

TEST(ScoreNormalization, EmptyInputIsNoOp_BM25) {
    std::vector<HybridSearch::Result> results;
    HybridSearch::normalizeScores(results, /*is_bm25=*/true);
    EXPECT_TRUE(results.empty());
}

TEST(ScoreNormalization, EmptyInputIsNoOp_Vector) {
    std::vector<HybridSearch::Result> results;
    HybridSearch::normalizeScores(results, /*is_bm25=*/false);
    EXPECT_TRUE(results.empty());
}

TEST(ScoreNormalization, SinglePositiveScore_BM25_ScoreBecomesOne) {
    // Single result with positive score → range==0 → normalizes to 1.0
    std::vector<HybridSearch::Result> results = {bm25Result("doc1", 3.7)};
    HybridSearch::normalizeScores(results, /*is_bm25=*/true);
    EXPECT_DOUBLE_EQ(results[0].bm25_score, 1.0);
}

TEST(ScoreNormalization, SinglePositiveScore_Vector_ScoreBecomesOne) {
    std::vector<HybridSearch::Result> results = {vecResult("doc1", 0.9)};
    HybridSearch::normalizeScores(results, /*is_bm25=*/false);
    EXPECT_DOUBLE_EQ(results[0].vector_score, 1.0);
}

TEST(ScoreNormalization, SingleZeroScore_BM25_NormalizesToZero) {
    // Single result with score 0 → range==0 and max==0 → normalizes to 0.0
    std::vector<HybridSearch::Result> results = {bm25Result("doc1", 0.0)};
    HybridSearch::normalizeScores(results, /*is_bm25=*/true);
    EXPECT_DOUBLE_EQ(results[0].bm25_score, 0.0);
}

TEST(ScoreNormalization, SingleZeroScore_Vector_NormalizesToZero) {
    std::vector<HybridSearch::Result> results = {vecResult("doc1", 0.0)};
    HybridSearch::normalizeScores(results, /*is_bm25=*/false);
    EXPECT_DOUBLE_EQ(results[0].vector_score, 0.0);
}

// ============================================================================
// Tied scores (range == 0) with multiple elements
// ============================================================================

TEST(ScoreNormalization, AllTiedPositiveScores_NormalizesToOne) {
    std::vector<HybridSearch::Result> results = {
        bm25Result("doc1", 5.0), bm25Result("doc2", 5.0), bm25Result("doc3", 5.0)
    };
    HybridSearch::normalizeScores(results, /*is_bm25=*/true);
    for (const auto& r : results) {
        EXPECT_DOUBLE_EQ(r.bm25_score, 1.0);
    }
}

TEST(ScoreNormalization, AllTiedZeroScores_NormalizesToZero) {
    std::vector<HybridSearch::Result> results = {
        vecResult("doc1", 0.0), vecResult("doc2", 0.0)
    };
    HybridSearch::normalizeScores(results, /*is_bm25=*/false);
    for (const auto& r : results) {
        EXPECT_DOUBLE_EQ(r.vector_score, 0.0);
    }
}

// ============================================================================
// Normal range normalization
// ============================================================================

TEST(ScoreNormalization, TwoScores_HighestBecomesOne_LowestBecomesZero) {
    std::vector<HybridSearch::Result> results = {
        bm25Result("low", 2.0), bm25Result("high", 8.0)
    };
    HybridSearch::normalizeScores(results, /*is_bm25=*/true);
    // min=2, max=8, range=6; low=(2-2)/6=0, high=(8-2)/6=1
    EXPECT_DOUBLE_EQ(results[0].bm25_score, 0.0); // low
    EXPECT_DOUBLE_EQ(results[1].bm25_score, 1.0); // high
}

TEST(ScoreNormalization, ThreeScores_MiddleNormalizedCorrectly) {
    std::vector<HybridSearch::Result> results = {
        bm25Result("a", 0.0), bm25Result("b", 5.0), bm25Result("c", 10.0)
    };
    HybridSearch::normalizeScores(results, /*is_bm25=*/true);
    // min=0, max=10, range=10
    EXPECT_DOUBLE_EQ(results[0].bm25_score, 0.0);
    EXPECT_DOUBLE_EQ(results[1].bm25_score, 0.5);
    EXPECT_DOUBLE_EQ(results[2].bm25_score, 1.0);
}

TEST(ScoreNormalization, VectorScores_NormalizedInPlace) {
    std::vector<HybridSearch::Result> results = {
        vecResult("a", 0.2), vecResult("b", 0.8)
    };
    HybridSearch::normalizeScores(results, /*is_bm25=*/false);
    // min=0.2, max=0.8, range=0.6; a=(0.2-0.2)/0.6=0, b=(0.8-0.2)/0.6=1
    EXPECT_DOUBLE_EQ(results[0].vector_score, 0.0);
    EXPECT_DOUBLE_EQ(results[1].vector_score, 1.0);
}

TEST(ScoreNormalization, NormalizationPreservesOrder) {
    std::vector<HybridSearch::Result> results = {
        bm25Result("a", 10.0), bm25Result("b", 4.0), bm25Result("c", 7.0)
    };
    HybridSearch::normalizeScores(results, /*is_bm25=*/true);
    // Scores are normalized but relative order should be preserved
    EXPECT_GT(results[0].bm25_score, results[2].bm25_score); // a > c
    EXPECT_GT(results[2].bm25_score, results[1].bm25_score); // c > b
}

TEST(ScoreNormalization, AllNormalizedScoresInUnitInterval) {
    std::vector<HybridSearch::Result> results = {
        bm25Result("a", 1.0), bm25Result("b", 3.5), bm25Result("c", 7.0),
        bm25Result("d", 2.2), bm25Result("e", 9.9)
    };
    HybridSearch::normalizeScores(results, /*is_bm25=*/true);
    for (const auto& r : results) {
        EXPECT_GE(r.bm25_score, 0.0);
        EXPECT_LE(r.bm25_score, 1.0);
    }
}

TEST(ScoreNormalization, BM25NormalizationDoesNotAffectVectorScore) {
    // Calling normalizeScores(is_bm25=true) must leave vector_score unchanged
    HybridSearch::Result r;
    r.document_id = "doc1";
    r.bm25_score = 5.0;
    r.vector_score = 0.7;

    std::vector<HybridSearch::Result> results = {r};
    HybridSearch::normalizeScores(results, /*is_bm25=*/true);

    EXPECT_DOUBLE_EQ(results[0].vector_score, 0.7); // unchanged
    EXPECT_DOUBLE_EQ(results[0].bm25_score, 1.0);   // single-result → 1.0
}

TEST(ScoreNormalization, VectorNormalizationDoesNotAffectBM25Score) {
    // Calling normalizeScores(is_bm25=false) must leave bm25_score unchanged
    HybridSearch::Result r;
    r.document_id = "doc1";
    r.bm25_score = 3.3;
    r.vector_score = 0.5;

    std::vector<HybridSearch::Result> results = {r};
    HybridSearch::normalizeScores(results, /*is_bm25=*/false);

    EXPECT_DOUBLE_EQ(results[0].bm25_score, 3.3);   // unchanged
    EXPECT_DOUBLE_EQ(results[0].vector_score, 1.0);  // single-result → 1.0
}

TEST(ScoreNormalization, NegativeScoresNormalized) {
    // negative scores are valid (e.g. DOT product can produce negatives)
    // normalizeScores should still produce [0,1] output
    std::vector<HybridSearch::Result> results = {
        vecResult("a", -1.0), vecResult("b", 0.0), vecResult("c", 1.0)
    };
    HybridSearch::normalizeScores(results, /*is_bm25=*/false);
    // min=-1, max=1, range=2; a=0, b=0.5, c=1
    EXPECT_DOUBLE_EQ(results[0].vector_score, 0.0);
    EXPECT_DOUBLE_EQ(results[1].vector_score, 0.5);
    EXPECT_DOUBLE_EQ(results[2].vector_score, 1.0);
}

TEST(ScoreNormalization, LargeScaleScores) {
    // Regression: large score values should not cause overflow
    std::vector<HybridSearch::Result> results = {
        bm25Result("a", 1e10), bm25Result("b", 2e10)
    };
    HybridSearch::normalizeScores(results, /*is_bm25=*/true);
    EXPECT_DOUBLE_EQ(results[0].bm25_score, 0.0);
    EXPECT_DOUBLE_EQ(results[1].bm25_score, 1.0);
}
