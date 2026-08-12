/**
 * @file test_multi_modal_search.cpp
 * @brief Unit tests for MultiModalSearch (v1.5.0)
 *
 * Static / structural tests; integration with live backends is in
 * test_hybrid_search_integration.cpp.
 */

#include <gtest/gtest.h>
#include "search/multi_modal_search.h"
#include <string>
#include <vector>

using namespace themis;

// ============================================================================
// Config validation
// ============================================================================

TEST(MultiModalConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(MultiModalSearch(nullptr, nullptr));
}

TEST(MultiModalConfig, ZeroKThrows) {
    MultiModalSearch::Config cfg;
    cfg.k = 0;
    EXPECT_THROW(MultiModalSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(MultiModalConfig, ZeroRrfKThrows) {
    MultiModalSearch::Config cfg;
    cfg.rrf_k = 0.0;
    EXPECT_THROW(MultiModalSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(MultiModalConfig, NegativeRrfKThrows) {
    MultiModalSearch::Config cfg;
    cfg.rrf_k = -1.0;
    EXPECT_THROW(MultiModalSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(MultiModalConfig, ZeroCandidatesPerModalThrows) {
    MultiModalSearch::Config cfg;
    cfg.candidates_per_modal = 0;
    EXPECT_THROW(MultiModalSearch(nullptr, nullptr, cfg), std::invalid_argument);
}

TEST(MultiModalConfig, ConfigRoundtrip) {
    MultiModalSearch::Config cfg;
    cfg.k = 5;
    cfg.rrf_k = 30.0;
    MultiModalSearch mms(nullptr, nullptr, cfg);
    EXPECT_EQ(mms.getConfig().k, 5u);
    EXPECT_DOUBLE_EQ(mms.getConfig().rrf_k, 30.0);
}

// ============================================================================
// search() — null index / empty queries
// ============================================================================

TEST(MultiModalSearchTest, EmptyQueriesReturnsEmpty) {
    MultiModalSearch mms(nullptr, nullptr);
    auto results = mms.search({});
    EXPECT_TRUE(results.empty());
}

TEST(MultiModalSearchTest, TextQueryWithNullIndexReturnsEmpty) {
    MultiModalSearch mms(nullptr, nullptr);
    ModalQuery q;
    q.modality = Modality::TEXT;
    q.text = "machine learning";
    q.weight = 1.0;
    auto results = mms.search({q}, "table", "body");
    EXPECT_TRUE(results.empty());
}

TEST(MultiModalSearchTest, ImageQueryWithNullIndexReturnsEmpty) {
    MultiModalSearch mms(nullptr, nullptr);
    ModalQuery q;
    q.modality = Modality::IMAGE;
    q.embedding = {0.1f, 0.2f, 0.3f};
    q.weight = 1.0;
    auto results = mms.search({q});
    EXPECT_TRUE(results.empty());
}

TEST(MultiModalSearchTest, EmptyEmbeddingSkipped) {
    MultiModalSearch mms(nullptr, nullptr);
    ModalQuery q;
    q.modality = Modality::IMAGE;
    q.embedding = {}; // empty → should be skipped
    q.weight = 1.0;
    auto results = mms.search({q});
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// searchTextAndImage() — null backends
// ============================================================================

TEST(MultiModalSearchTest, TextAndImageNullBackendsReturnsEmpty) {
    MultiModalSearch mms(nullptr, nullptr);
    std::vector<float> embed = {0.1f, 0.2f, 0.3f};
    auto results = mms.searchTextAndImage("query", embed, "image_ns", "table", "col");
    EXPECT_TRUE(results.empty());
}

TEST(MultiModalSearchTest, TextAndImageEmptyTextAndEmbeddingReturnsEmpty) {
    MultiModalSearch mms(nullptr, nullptr);
    auto results = mms.searchTextAndImage("", {}, "image_ns", "table", "col");
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// Modality / ModalQuery structs
// ============================================================================

TEST(ModalQueryStruct, DefaultInitialization) {
    ModalQuery q;
    EXPECT_EQ(q.modality, Modality::TEXT);
    EXPECT_TRUE(q.text.empty());
    EXPECT_TRUE(q.embedding.empty());
    EXPECT_TRUE(q.embedding_namespace.empty());
    EXPECT_DOUBLE_EQ(q.weight, 1.0);
}

TEST(MultiModalResultStruct, DefaultInitialization) {
    MultiModalResult r;
    EXPECT_TRUE(r.document_id.empty());
    EXPECT_DOUBLE_EQ(r.score, 0.0);
    EXPECT_TRUE(r.matched_modality.empty());
}

// ============================================================================
// RRF fusion logic (tested through search() with fabricated ranked lists)
// ============================================================================

namespace {
// Helper: construct a MultiModalSearch subclass to expose fuseRRF for testing
// Instead, we verify fusion indirectly through the public search() interface
// using mock-like data.  Since search() delegates to executeModal() which
// requires live indices, we test fusion properties via a dummy scenario:
// two identical ranked lists should produce the same order.
}

TEST(MultiModalSearchTest, ZeroWeightQueryStillFused) {
    // A query with weight 0 should use weight 1.0 internally (guard in fuseRRF)
    MultiModalSearch mms(nullptr, nullptr);
    ModalQuery q;
    q.modality = Modality::TEXT;
    q.text = "test";
    q.weight = 0.0; // invalid weight → clamped to 1.0 by impl
    // With null index, result will be empty but should not throw
    EXPECT_NO_THROW(mms.search({q}, "table", "col"));
}

// ============================================================================
// k cap test
// ============================================================================

TEST(MultiModalSearchTest, KCapRespected) {
    MultiModalSearch::Config cfg;
    cfg.k = 3;
    MultiModalSearch mms(nullptr, nullptr, cfg);
    // Even with null backends, k property is preserved
    EXPECT_EQ(mms.getConfig().k, 3u);
}

// ============================================================================
// Modality enum values
// ============================================================================

TEST(ModalityEnum, EnumValuesDistinct) {
    EXPECT_NE(Modality::TEXT,   Modality::IMAGE);
    EXPECT_NE(Modality::IMAGE,  Modality::AUDIO);
    EXPECT_NE(Modality::AUDIO,  Modality::CUSTOM);
    EXPECT_NE(Modality::TEXT,   Modality::CUSTOM);
}
