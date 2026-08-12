/**
 * @file test_cross_lingual_search.cpp
 * @brief Unit tests for CrossLingualSearch (v2.0.0)
 *
 * Tests cover: config validation (including resource-limit clamping), null-
 * index safety, empty-embedding short-circuits, setLanguageMap / setConfig,
 * LanguageHint / EmbeddingQuery / Result struct defaults, score threshold
 * filtering, and multi-embedding API surface.
 *
 * Integration with live VectorIndexManager backends is exercised separately
 * in test_hybrid_search_integration.cpp.
 */

#include <gtest/gtest.h>
#include "search/cross_lingual_search.h"
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Config validation
// ============================================================================

TEST(CrossLingualSearchConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(CrossLingualSearch(nullptr));
}

TEST(CrossLingualSearchConfig, ZeroKThrows) {
    CrossLingualSearch::Config cfg;
    cfg.k = 0;
    EXPECT_THROW(CrossLingualSearch(nullptr, cfg), std::invalid_argument);
}

TEST(CrossLingualSearchConfig, ZeroCandidatesThrows) {
    CrossLingualSearch::Config cfg;
    cfg.candidates = 0;
    EXPECT_THROW(CrossLingualSearch(nullptr, cfg), std::invalid_argument);
}

TEST(CrossLingualSearchConfig, ZeroRrfKThrows) {
    CrossLingualSearch::Config cfg;
    cfg.rrf_k = 0.0;
    EXPECT_THROW(CrossLingualSearch(nullptr, cfg), std::invalid_argument);
}

TEST(CrossLingualSearchConfig, NegativeRrfKThrows) {
    CrossLingualSearch::Config cfg;
    cfg.rrf_k = -5.0;
    EXPECT_THROW(CrossLingualSearch(nullptr, cfg), std::invalid_argument);
}

TEST(CrossLingualSearchConfig, ConfigRoundtrip) {
    CrossLingualSearch::Config cfg;
    cfg.k          = 5;
    cfg.candidates = 50;
    cfg.rrf_k      = 30.0;
    CrossLingualSearch cls(nullptr, cfg);
    EXPECT_EQ(cls.getConfig().k, 5u);
    EXPECT_EQ(cls.getConfig().candidates, 50u);
    EXPECT_DOUBLE_EQ(cls.getConfig().rrf_k, 30.0);
}

TEST(CrossLingualSearchConfig, KClampedToMaxK) {
    CrossLingualSearch::Config cfg;
    cfg.k     = 20'000;
    cfg.max_k = 5'000;
    CrossLingualSearch cls(nullptr, cfg);
    EXPECT_EQ(cls.getConfig().k, 5'000u);
}

TEST(CrossLingualSearchConfig, CandidatesClampedToMaxCandidates) {
    CrossLingualSearch::Config cfg;
    cfg.candidates     = 20'000;
    cfg.max_candidates = 5'000;
    CrossLingualSearch cls(nullptr, cfg);
    EXPECT_EQ(cls.getConfig().candidates, 5'000u);
}

TEST(CrossLingualSearchConfig, ScoreThresholdStoredCorrectly) {
    CrossLingualSearch::Config cfg;
    cfg.score_threshold = 0.75;
    CrossLingualSearch cls(nullptr, cfg);
    EXPECT_DOUBLE_EQ(cls.getConfig().score_threshold, 0.75);
}

// ============================================================================
// search() — null index / empty embedding
// ============================================================================

TEST(CrossLingualSearchTest, NullIndexReturnsEmpty) {
    CrossLingualSearch cls(nullptr);
    std::vector<float> emb = {0.1f, 0.2f, 0.3f};
    auto results = cls.search(emb);
    EXPECT_TRUE(results.empty());
}

TEST(CrossLingualSearchTest, EmptyEmbeddingReturnsEmpty) {
    CrossLingualSearch cls(nullptr);
    auto results = cls.search({});
    EXPECT_TRUE(results.empty());
}

TEST(CrossLingualSearchTest, SearchWithHintsNullIndexReturnsEmpty) {
    CrossLingualSearch cls(nullptr);
    std::vector<float> emb = {0.5f, 0.5f};
    std::vector<CrossLingualSearch::LanguageHint> hints = {{"en", 1.5}};
    EXPECT_NO_THROW(cls.search(emb, hints));
    EXPECT_TRUE(cls.search(emb, hints).empty());
}

// ============================================================================
// searchMultiEmbedding() — null index / empty queries
// ============================================================================

TEST(CrossLingualSearchTest, MultiEmbeddingEmptyQueriesReturnsEmpty) {
    CrossLingualSearch cls(nullptr);
    auto results = cls.searchMultiEmbedding({});
    EXPECT_TRUE(results.empty());
}

TEST(CrossLingualSearchTest, MultiEmbeddingNullIndexReturnsEmpty) {
    CrossLingualSearch cls(nullptr);
    CrossLingualSearch::EmbeddingQuery q;
    q.embedding = {0.1f, 0.2f, 0.3f};
    q.weight    = 1.0;
    auto results = cls.searchMultiEmbedding({q});
    EXPECT_TRUE(results.empty());
}

TEST(CrossLingualSearchTest, MultiEmbeddingEmptyEmbeddingSkipped) {
    CrossLingualSearch cls(nullptr);
    CrossLingualSearch::EmbeddingQuery q;
    q.embedding = {};  // empty → silently skipped
    q.weight    = 1.0;
    EXPECT_NO_THROW(cls.searchMultiEmbedding({q}));
    EXPECT_TRUE(cls.searchMultiEmbedding({q}).empty());
}

TEST(CrossLingualSearchTest, MultiEmbeddingWithHintsNullIndexReturnsEmpty) {
    CrossLingualSearch cls(nullptr);
    CrossLingualSearch::EmbeddingQuery q;
    q.embedding = {0.5f, 0.5f};
    std::vector<CrossLingualSearch::LanguageHint> hints = {{"de", 0.8}};
    EXPECT_NO_THROW(cls.searchMultiEmbedding({q}, hints));
    EXPECT_TRUE(cls.searchMultiEmbedding({q}, hints).empty());
}

TEST(CrossLingualSearchTest, ZeroWeightQueryDoesNotCrash) {
    // zero weight is clamped to 1.0 internally
    CrossLingualSearch cls(nullptr);
    CrossLingualSearch::EmbeddingQuery q;
    q.embedding = {0.1f, 0.2f};
    q.weight    = 0.0;
    EXPECT_NO_THROW(cls.searchMultiEmbedding({q}));
}

// ============================================================================
// setLanguageMap / setConfig
// ============================================================================

TEST(CrossLingualSearchTest, SetLanguageMapDoesNotThrow) {
    CrossLingualSearch cls(nullptr);
    EXPECT_NO_THROW(
        cls.setLanguageMap({{"doc1", "en"}, {"doc2", "de"}, {"doc3", "fr"}}));
}

TEST(CrossLingualSearchTest, SetConfigUpdatesK) {
    CrossLingualSearch cls(nullptr);
    CrossLingualSearch::Config cfg;
    cfg.k = 7;
    cls.setConfig(cfg);
    EXPECT_EQ(cls.getConfig().k, 7u);
}

TEST(CrossLingualSearchTest, SetConfigUpdatesScoreThreshold) {
    CrossLingualSearch cls(nullptr);
    CrossLingualSearch::Config cfg;
    cfg.score_threshold = 0.5;
    cls.setConfig(cfg);
    EXPECT_DOUBLE_EQ(cls.getConfig().score_threshold, 0.5);
}

// ============================================================================
// Struct default initialisation
// ============================================================================

TEST(CrossLingualSearchStructs, LanguageHintDefaults) {
    CrossLingualSearch::LanguageHint hint;
    EXPECT_TRUE(hint.language_code.empty());
    EXPECT_DOUBLE_EQ(hint.boost, 1.0);
}

TEST(CrossLingualSearchStructs, EmbeddingQueryDefaults) {
    CrossLingualSearch::EmbeddingQuery q;
    EXPECT_TRUE(q.embedding.empty());
    EXPECT_DOUBLE_EQ(q.weight, 1.0);
}

TEST(CrossLingualSearchStructs, ResultDefaults) {
    CrossLingualSearch::Result r;
    EXPECT_TRUE(r.document_id.empty());
    EXPECT_DOUBLE_EQ(r.score, 0.0);
    EXPECT_TRUE(r.language.empty());
}

// ============================================================================
// k cap is respected (constructor)
// ============================================================================

TEST(CrossLingualSearchTest, KCapRespected) {
    CrossLingualSearch::Config cfg;
    cfg.k = 3;
    CrossLingualSearch cls(nullptr, cfg);
    EXPECT_EQ(cls.getConfig().k, 3u);
}

// ============================================================================
// Multiple embeddings with only empty ones → empty
// ============================================================================

TEST(CrossLingualSearchTest, MultiEmbeddingAllEmptyReturnsEmpty) {
    CrossLingualSearch cls(nullptr);
    CrossLingualSearch::EmbeddingQuery q1, q2;
    q1.embedding = {};
    q2.embedding = {};
    auto results = cls.searchMultiEmbedding({q1, q2});
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// Language hint with zero boost is ignored (not stored)
// ============================================================================

TEST(CrossLingualSearchTest, ZeroBoostHintDoesNotCrash) {
    CrossLingualSearch cls(nullptr);
    cls.setLanguageMap({{"doc1", "en"}});
    std::vector<float> emb = {0.5f};
    std::vector<CrossLingualSearch::LanguageHint> hints = {{"en", 0.0}};
    // zero-boost hints are skipped silently; should not crash
    EXPECT_NO_THROW(cls.search(emb, hints));
}
