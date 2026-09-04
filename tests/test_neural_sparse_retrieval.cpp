/**
 * @file test_neural_sparse_retrieval.cpp
 * @brief Unit tests for NeuralSparseRetrieval (v2.0.0)
 *
 * Tests cover: config validation, encoder attachment, addDocument /
 * addDocumentText / removeDocument, search with pre-computed vectors,
 * searchText, normalizeScores (public static), score threshold filtering,
 * k-cap, max_terms_per_doc truncation, empty/null-encoder safety,
 * negative-weight clamping, and score ordering invariants.
 */

#include <gtest/gtest.h>
#include "search/neural_sparse_retrieval.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

/// Simple bag-of-words encoder: splits on spaces, assigns weight 1.0 per token
static SparseVector bowEncoder(const std::string& text) {
    SparseVector sv;
    std::string token;
    for (char c : text) {
        if (c == ' ') {
            if (!token.empty()) { sv[token] += 1.0f; token.clear(); }
        } else {
            token += c;
        }
    }
    if (!token.empty()) {
      sv[token] += 1.0f;
    }
    return sv;
}

// ============================================================================
// Config validation
// ============================================================================

TEST(NeuralSparseRetrievalConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(NeuralSparseRetrieval());
}

TEST(NeuralSparseRetrievalConfig, ZeroKThrows) {
    NeuralSparseRetrieval::Config cfg;
    cfg.k = 0;
    EXPECT_THROW(NeuralSparseRetrieval(cfg), std::invalid_argument);
}

TEST(NeuralSparseRetrievalConfig, ZeroMaxTermsThrows) {
    NeuralSparseRetrieval::Config cfg;
    cfg.max_terms_per_doc = 0;
    EXPECT_THROW(NeuralSparseRetrieval(cfg), std::invalid_argument);
}

TEST(NeuralSparseRetrievalConfig, NegativeThresholdThrows) {
    NeuralSparseRetrieval::Config cfg;
    cfg.score_threshold = -0.1f;
    EXPECT_THROW(NeuralSparseRetrieval(cfg), std::invalid_argument);
}

TEST(NeuralSparseRetrievalConfig, ZeroThresholdIsValid) {
    NeuralSparseRetrieval::Config cfg;
    cfg.score_threshold = 0.0f;
    EXPECT_NO_THROW(NeuralSparseRetrieval(cfg));
}

TEST(NeuralSparseRetrievalConfig, ConfigRoundtrip) {
    NeuralSparseRetrieval::Config cfg;
    cfg.k = 5;
    cfg.max_terms_per_doc = 64;
    cfg.score_threshold = 0.1f;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    EXPECT_EQ(nsr.getConfig().k, 5u);
    EXPECT_EQ(nsr.getConfig().max_terms_per_doc, 64u);
    EXPECT_FLOAT_EQ(nsr.getConfig().score_threshold, 0.1f);
    EXPECT_FALSE(nsr.getConfig().normalize_scores);
}

// ============================================================================
// size() / clear()
// ============================================================================

TEST(NeuralSparseRetrievalSize, EmptyIndexHasSizeZero) {
    NeuralSparseRetrieval nsr;
    EXPECT_EQ(nsr.size(), 0u);
}

TEST(NeuralSparseRetrievalSize, SizeIncreasesOnAdd) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("d1", {{"a", 1.0f}});
    nsr.addDocument("d2", {{"b", 1.0f}});
    EXPECT_EQ(nsr.size(), 2u);
}

TEST(NeuralSparseRetrievalSize, ClearResetsToZero) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("d1", {{"a", 1.0f}});
    nsr.clear();
    EXPECT_EQ(nsr.size(), 0u);
}

// ============================================================================
// addDocument – edge cases
// ============================================================================

TEST(NeuralSparseRetrievalAdd, EmptyDocIdIsSkipped) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("", {{"a", 1.0f}});
    EXPECT_EQ(nsr.size(), 0u);
}

TEST(NeuralSparseRetrievalAdd, EmptySparseVectorIsSkipped) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("doc1", {});
    EXPECT_EQ(nsr.size(), 0u);
}

TEST(NeuralSparseRetrievalAdd, AllNegativeWeightsClamped) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("doc1", {{"a", -1.0f}, {"b", -2.0f}});
    // All weights are non-positive → effectively empty → not indexed
    EXPECT_EQ(nsr.size(), 0u);
}

TEST(NeuralSparseRetrievalAdd, MixedWeightsClampsNegatives) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("doc1", {{"a", 2.0f}, {"b", -1.0f}});
    EXPECT_EQ(nsr.size(), 1u);
    // Only "a" should survive — searching for "b" should give no results
    auto results = nsr.search({{"b", 1.0f}});
    EXPECT_TRUE(results.empty());
    // Searching for "a" should find the doc
    auto results2 = nsr.search({{"a", 1.0f}});
    EXPECT_EQ(results2.size(), 1u);
}

TEST(NeuralSparseRetrievalAdd, ReaddOverwritesPreviousVector) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("doc1", {{"old_term", 1.0f}});
    nsr.addDocument("doc1", {{"new_term", 1.0f}});
    // Size remains 1 (same doc re-indexed)
    EXPECT_EQ(nsr.size(), 1u);
    // Old term no longer matches
    auto r_old = nsr.search({{"old_term", 1.0f}});
    EXPECT_TRUE(r_old.empty());
    // New term matches
    auto r_new = nsr.search({{"new_term", 1.0f}});
    EXPECT_EQ(r_new.size(), 1u);
}

TEST(NeuralSparseRetrievalAdd, MaxTermsTruncatesLargeVector) {
    NeuralSparseRetrieval::Config cfg;
    cfg.max_terms_per_doc = 2;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    // 4 terms but only top-2 by weight survive
    nsr.addDocument("doc1", {{"a", 1.0f}, {"b", 4.0f}, {"c", 2.0f}, {"d", 3.0f}});
    // top-2 are "b"(4) and "d"(3)
    auto rb = nsr.search({{"b", 1.0f}});
    EXPECT_FALSE(rb.empty());
    auto rd = nsr.search({{"d", 1.0f}});
    EXPECT_FALSE(rd.empty());
    // "a" and "c" were truncated
    auto ra = nsr.search({{"a", 1.0f}});
    EXPECT_TRUE(ra.empty());
    auto rc = nsr.search({{"c", 1.0f}});
    EXPECT_TRUE(rc.empty());
}

// ============================================================================
// removeDocument
// ============================================================================

TEST(NeuralSparseRetrievalRemove, RemoveDecreasesSize) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("d1", {{"x", 1.0f}});
    nsr.addDocument("d2", {{"y", 1.0f}});
    nsr.removeDocument("d1");
    EXPECT_EQ(nsr.size(), 1u);
}

TEST(NeuralSparseRetrievalRemove, RemovedDocNotFoundInSearch) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("d1", {{"query_term", 1.0f}});
    nsr.removeDocument("d1");
    auto results = nsr.search({{"query_term", 1.0f}});
    EXPECT_TRUE(results.empty());
}

TEST(NeuralSparseRetrievalRemove, RemoveNonexistentDocIsNoOp) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("d1", {{"x", 1.0f}});
    EXPECT_NO_THROW(nsr.removeDocument("no_such_doc"));
    EXPECT_EQ(nsr.size(), 1u);
}

// ============================================================================
// search — basic correctness
// ============================================================================

TEST(NeuralSparseRetrievalSearch, EmptyIndexReturnsEmpty) {
    NeuralSparseRetrieval nsr;
    auto results = nsr.search({{"a", 1.0f}});
    EXPECT_TRUE(results.empty());
}

TEST(NeuralSparseRetrievalSearch, EmptyQueryVectorReturnsEmpty) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("d1", {{"a", 1.0f}});
    auto results = nsr.search({});
    EXPECT_TRUE(results.empty());
}

TEST(NeuralSparseRetrievalSearch, NonMatchingQueryReturnsEmpty) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("d1", {{"cat", 1.0f}});
    auto results = nsr.search({{"dog", 1.0f}});
    EXPECT_TRUE(results.empty());
}

TEST(NeuralSparseRetrievalSearch, ExactMatchReturnsDocument) {
    NeuralSparseRetrieval::Config cfg;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    nsr.addDocument("d1", {{"database", 2.0f}});
    auto results = nsr.search({{"database", 3.0f}});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "d1");
    // raw score = 2.0 * 3.0 = 6.0
    EXPECT_FLOAT_EQ(results[0].raw_score, 6.0f);
}

TEST(NeuralSparseRetrievalSearch, ScoreIsInnerProduct) {
    NeuralSparseRetrieval::Config cfg;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    // d1 has terms a=1, b=2; query has a=2, b=3
    // expected score = 1*2 + 2*3 = 8
    nsr.addDocument("d1", {{"a", 1.0f}, {"b", 2.0f}});
    auto results = nsr.search({{"a", 2.0f}, {"b", 3.0f}});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_FLOAT_EQ(results[0].raw_score, 8.0f);
}

TEST(NeuralSparseRetrievalSearch, ResultsSortedByScoreDescending) {
    NeuralSparseRetrieval::Config cfg;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    nsr.addDocument("low",  {{"term", 1.0f}});
    nsr.addDocument("mid",  {{"term", 2.0f}});
    nsr.addDocument("high", {{"term", 3.0f}});
    auto results = nsr.search({{"term", 1.0f}});
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0].document_id, "high");
    EXPECT_EQ(results[1].document_id, "mid");
    EXPECT_EQ(results[2].document_id, "low");
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i - 1].raw_score, results[i].raw_score);
    }
}

TEST(NeuralSparseRetrievalSearch, KCapLimitsResults) {
    NeuralSparseRetrieval::Config cfg;
    cfg.k = 2;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    for (int i = 0; i < 5; ++i) {
        nsr.addDocument("d" + std::to_string(i), {{"t", static_cast<float>(i + 1)}});
    }
    auto results = nsr.search({{"t", 1.0f}});
    EXPECT_EQ(results.size(), 2u);
    // Top-2 by score
    EXPECT_EQ(results[0].document_id, "d4");
    EXPECT_EQ(results[1].document_id, "d3");
}

TEST(NeuralSparseRetrievalSearch, KOverrideInSearchCall) {
    NeuralSparseRetrieval::Config cfg;
    cfg.k = 10;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    for (int i = 0; i < 5; ++i) {
        nsr.addDocument("d" + std::to_string(i), {{"t", static_cast<float>(i + 1)}});
    }
    // k override = 3
    auto results = nsr.search({{"t", 1.0f}}, 3);
    EXPECT_EQ(results.size(), 3u);
}

// ============================================================================
// score threshold
// ============================================================================

TEST(NeuralSparseRetrievalSearch, ScoreThresholdFiltersLowScores) {
    NeuralSparseRetrieval::Config cfg;
    cfg.score_threshold = 2.0f;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    nsr.addDocument("low",  {{"t", 1.0f}});  // score = 1*1 = 1 < 2 → filtered
    nsr.addDocument("high", {{"t", 3.0f}});  // score = 3*1 = 3 >= 2 → kept
    auto results = nsr.search({{"t", 1.0f}});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].document_id, "high");
}

// ============================================================================
// normalizeScores — public static
// ============================================================================

TEST(NeuralSparseNormalizeScores, EmptyIsNoOp) {
    std::vector<NeuralSparseRetrieval::Result> empty;
    EXPECT_NO_THROW(NeuralSparseRetrieval::normalizeScores(empty));
    EXPECT_TRUE(empty.empty());
}

TEST(NeuralSparseNormalizeScores, SinglePositiveBecomesOne) {
    std::vector<NeuralSparseRetrieval::Result> v(1);
    v[0].score = 5.0f;
    NeuralSparseRetrieval::normalizeScores(v);
    EXPECT_FLOAT_EQ(v[0].score, 1.0f);
}

TEST(NeuralSparseNormalizeScores, SingleZeroBecomesZero) {
    std::vector<NeuralSparseRetrieval::Result> v(1);
    v[0].score = 0.0f;
    NeuralSparseRetrieval::normalizeScores(v);
    EXPECT_FLOAT_EQ(v[0].score, 0.0f);
}

TEST(NeuralSparseNormalizeScores, AllEqualPositiveBecomesOne) {
    std::vector<NeuralSparseRetrieval::Result> v(3);
    for (auto& r : v) {
      r.score = 4.0f;
    }
    NeuralSparseRetrieval::normalizeScores(v);
    for (const auto& r : v) {
      EXPECT_FLOAT_EQ(r.score, 1.0f);
    }
}

TEST(NeuralSparseNormalizeScores, AllEqualZeroBecomesZero) {
    std::vector<NeuralSparseRetrieval::Result> v(2);
    for (auto& r : v) {
      r.score = 0.0f;
    }
    NeuralSparseRetrieval::normalizeScores(v);
    for (const auto& r : v) {
      EXPECT_FLOAT_EQ(r.score, 0.0f);
    }
}

TEST(NeuralSparseNormalizeScores, RangeNormalizationIsCorrect) {
    // min=2, max=6 -> range=4
    std::vector<NeuralSparseRetrieval::Result> v(3);
    v[0].score = 2.0f; // -> 0.0
    v[1].score = 4.0f; // -> 0.5
    v[2].score = 6.0f; // -> 1.0
    NeuralSparseRetrieval::normalizeScores(v);
    EXPECT_FLOAT_EQ(v[0].score, 0.0f);
    EXPECT_FLOAT_EQ(v[1].score, 0.5f);
    EXPECT_FLOAT_EQ(v[2].score, 1.0f);
}

TEST(NeuralSparseNormalizeScores, NormalizedBoundsZeroToOne) {
    std::vector<NeuralSparseRetrieval::Result> v(4);
    v[0].score = 1.0f;
    v[1].score = 3.0f;
    v[2].score = 7.0f;
    v[3].score = 10.0f;
    NeuralSparseRetrieval::normalizeScores(v);
    for (const auto& r : v) {
        EXPECT_GE(r.score, 0.0f);
        EXPECT_LE(r.score, 1.0f);
    }
}

TEST(NeuralSparseNormalizeScores, RawScoreIsPreserved) {
    std::vector<NeuralSparseRetrieval::Result> v(2);
    v[0].raw_score = 3.0f; v[0].score = 3.0f;
    v[1].raw_score = 9.0f; v[1].score = 9.0f;
    NeuralSparseRetrieval::normalizeScores(v);
    // raw_score is not touched
    EXPECT_FLOAT_EQ(v[0].raw_score, 3.0f);
    EXPECT_FLOAT_EQ(v[1].raw_score, 9.0f);
}

// ============================================================================
// normalizeScores integrated into search()
// ============================================================================

TEST(NeuralSparseRetrievalSearch, NormalizedScoresInRange) {
    NeuralSparseRetrieval::Config cfg;
    cfg.normalize_scores = true;
    NeuralSparseRetrieval nsr(cfg);
    nsr.addDocument("a", {{"t", 1.0f}});
    nsr.addDocument("b", {{"t", 3.0f}});
    nsr.addDocument("c", {{"t", 5.0f}});
    auto results = nsr.search({{"t", 1.0f}});
    for (const auto& r : results) {
        EXPECT_GE(r.score, 0.0f);
        EXPECT_LE(r.score, 1.0f);
    }
}

TEST(NeuralSparseRetrievalSearch, NormalizeDisabledPreservesRawScore) {
    NeuralSparseRetrieval::Config cfg;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    nsr.addDocument("d1", {{"x", 4.0f}});
    auto results = nsr.search({{"x", 2.0f}});
    ASSERT_EQ(results.size(), 1u);
    EXPECT_FLOAT_EQ(results[0].score, 8.0f);
    EXPECT_FLOAT_EQ(results[0].raw_score, 8.0f);
}

// ============================================================================
// Encoder attachment and addDocumentText / searchText
// ============================================================================

TEST(NeuralSparseRetrievalEncoder, NoEncoderAddDocumentTextIsNoOp) {
    NeuralSparseRetrieval nsr;
    nsr.addDocumentText("doc1", "hello world");
    EXPECT_EQ(nsr.size(), 0u);
}

TEST(NeuralSparseRetrievalEncoder, NoEncoderSearchTextReturnsEmpty) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("d1", {{"hello", 1.0f}});
    auto results = nsr.searchText("hello");
    EXPECT_TRUE(results.empty());
}

TEST(NeuralSparseRetrievalEncoder, SetEncoderAndAddDocumentText) {
    NeuralSparseRetrieval nsr;
    nsr.setEncoder(bowEncoder);
    nsr.addDocumentText("doc1", "fast database engine");
    EXPECT_EQ(nsr.size(), 1u);
}

TEST(NeuralSparseRetrievalEncoder, SearchTextFindsIndexedDoc) {
    NeuralSparseRetrieval::Config cfg;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    nsr.setEncoder(bowEncoder);
    nsr.addDocumentText("doc1", "database engine performance");
    nsr.addDocumentText("doc2", "neural sparse retrieval splade");

    // "database" appears only in doc1
    auto r1 = nsr.searchText("database");
    ASSERT_EQ(r1.size(), 1u);
    EXPECT_EQ(r1[0].document_id, "doc1");

    // "neural" appears only in doc2
    auto r2 = nsr.searchText("neural");
    ASSERT_EQ(r2.size(), 1u);
    EXPECT_EQ(r2[0].document_id, "doc2");
}

TEST(NeuralSparseRetrievalEncoder, SetEncoderNullptrRemovesBackend) {
    NeuralSparseRetrieval nsr;
    nsr.setEncoder(bowEncoder);
    nsr.setEncoder(nullptr);
    nsr.addDocumentText("doc1", "hello");
    EXPECT_EQ(nsr.size(), 0u);
}

TEST(NeuralSparseRetrievalEncoder, SearchTextWithThrowingEncoderReturnsEmpty) {
    NeuralSparseRetrieval nsr;
    nsr.addDocument("d1", {{"x", 1.0f}});
    nsr.setEncoder([](const std::string&) -> SparseVector {
        throw std::runtime_error("encoder error");
    });
    // searchText should catch the exception and return empty
    EXPECT_NO_THROW({
        auto results = nsr.searchText("x");
        EXPECT_TRUE(results.empty());
    });
}

// ============================================================================
// Multiple documents / multi-term queries
// ============================================================================

TEST(NeuralSparseRetrievalSearch, MultiTermQueryAccumulatesScores) {
    NeuralSparseRetrieval::Config cfg;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    // doc1 matches both terms, doc2 matches only one
    nsr.addDocument("doc1", {{"engine", 2.0f}, {"fast", 3.0f}});
    nsr.addDocument("doc2", {{"engine", 1.0f}});

    // query: engine=1, fast=1
    // doc1: 2*1 + 3*1 = 5
    // doc2: 1*1 = 1
    auto results = nsr.search({{"engine", 1.0f}, {"fast", 1.0f}});
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].document_id, "doc1");
    EXPECT_FLOAT_EQ(results[0].raw_score, 5.0f);
    EXPECT_EQ(results[1].document_id, "doc2");
    EXPECT_FLOAT_EQ(results[1].raw_score, 1.0f);
}

TEST(NeuralSparseRetrievalSearch, ZeroWeightQueryTermSkipped) {
    NeuralSparseRetrieval::Config cfg;
    cfg.normalize_scores = false;
    NeuralSparseRetrieval nsr(cfg);
    nsr.addDocument("d1", {{"term", 2.0f}});
    // query weight = 0 → no contribution
    auto results = nsr.search({{"term", 0.0f}});
    EXPECT_TRUE(results.empty());
}
