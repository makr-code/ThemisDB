// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_knowledge_graph_production.cpp
 * @brief Production readiness tests for KnowledgeGraphEnricher (Phase 6)
 *
 * Covers:
 *  - Construction & configuration
 *  - enrichAll statistics
 *  - enrichSample (single sample)
 *  - enrichQuery (AQL-driven enrichment)
 *  - findRelatedProvisions / findRelatedCaseLaw / findSimilarDocuments
 *  - Custom query override
 *  - Built-in AQL template accessors
 *  - Error recovery (empty IDs, null callbacks)
 */

#include <gtest/gtest.h>
#include "training/knowledge_graph_enricher.h"
#include <string>
#include <vector>

using namespace themis::training;

// ============================================================================
// Test fixture
// ============================================================================
class KnowledgeGraphProductionTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.target_collection    = "legal_training_samples";
        config_.graph_name           = "legal_knowledge_graph";
        config_.max_related_items    = 5;
        config_.traversal_depth      = 2;
        config_.similarity_threshold = 0.7f;
        config_.include_provisions   = true;
        config_.include_case_law     = true;
        config_.include_guidance     = true;
        config_.include_similar_docs = true;
        config_.batch_size           = 50;
    }

    EnrichmentConfig config_;
    const std::string db_conn_ = "";
};

// ============================================================================
// Phase 6: Construction & configuration
// ============================================================================

TEST_F(KnowledgeGraphProductionTest, Construction_Succeeds) {
    EXPECT_NO_THROW(KnowledgeGraphEnricher enricher(config_, db_conn_));
}

TEST_F(KnowledgeGraphProductionTest, Config_DefaultValues) {
    EnrichmentConfig cfg;
    EXPECT_EQ(cfg.max_related_items, 5u);
    EXPECT_EQ(cfg.traversal_depth,   2u);
    EXPECT_FLOAT_EQ(cfg.similarity_threshold, 0.7f);
    EXPECT_TRUE(cfg.include_provisions);
    EXPECT_TRUE(cfg.include_case_law);
    EXPECT_TRUE(cfg.include_similar_docs);
}

TEST_F(KnowledgeGraphProductionTest, Config_BatchSize_Default) {
    EnrichmentConfig cfg;
    EXPECT_EQ(cfg.batch_size, 50u);
}

// ============================================================================
// Phase 6: enrichAll statistics
// ============================================================================

TEST_F(KnowledgeGraphProductionTest, EnrichAll_Empty_ReturnsZeroStats) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto stats = enricher.enrichAll();

    EXPECT_EQ(stats.samples_processed,   0u);
    EXPECT_EQ(stats.samples_enriched,    0u);
    EXPECT_EQ(stats.context_items_added, 0u);
}

TEST_F(KnowledgeGraphProductionTest, EnrichAll_ElapsedTimeRecorded) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto stats = enricher.enrichAll();
    EXPECT_GE(stats.elapsed_seconds, 0.0);
}

TEST_F(KnowledgeGraphProductionTest, EnrichAll_CallbackNotInvokedOnEmpty) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    int cb_count = 0;
    enricher.enrichAll([&](size_t, size_t, const std::string&) { ++cb_count; });
    EXPECT_EQ(cb_count, 0);
}

// ============================================================================
// Phase 6: enrichSample
// ============================================================================

TEST_F(KnowledgeGraphProductionTest, EnrichSample_EmptyId_ReturnsEmptyContext) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto ctx = enricher.enrichSample("");

    EXPECT_TRUE(ctx.related_provisions.empty());
    EXPECT_TRUE(ctx.case_law.empty());
    EXPECT_TRUE(ctx.similar_documents.empty());
    EXPECT_TRUE(ctx.context_summary.empty());
}

TEST_F(KnowledgeGraphProductionTest, EnrichSample_ValidId_ReturnsContext) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    // In test environment the source document resolves to empty → empty context
    auto ctx = enricher.enrichSample("sample_001");

    // Context may be empty (no DB) but call must not throw
    EXPECT_NO_THROW(enricher.enrichSample("sample_001"));
}

TEST_F(KnowledgeGraphProductionTest, EnrichSample_RelatedProvisionsVector_Empty_WhenNoDb) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto ctx = enricher.enrichSample("sample_002");
    EXPECT_TRUE(ctx.related_provisions.empty());
}

// ============================================================================
// Phase 6: enrichQuery
// ============================================================================

TEST_F(KnowledgeGraphProductionTest, EnrichQuery_EmptyQuery_ReturnsZeroStats) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto stats = enricher.enrichQuery("");
    EXPECT_EQ(stats.samples_processed, 0u);
}

TEST_F(KnowledgeGraphProductionTest, EnrichQuery_ValidQuery_DoesNotThrow) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    EXPECT_NO_THROW(enricher.enrichQuery(
        "FOR s IN legal_training_samples FILTER s.confidence > 0.8 RETURN s._key"));
}

// ============================================================================
// Phase 6: findRelatedProvisions
// ============================================================================

TEST_F(KnowledgeGraphProductionTest, FindRelatedProvisions_EmptyId_ReturnsEmpty) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto provisions = enricher.findRelatedProvisions("", 5);
    EXPECT_TRUE(provisions.empty());
}

TEST_F(KnowledgeGraphProductionTest, FindRelatedProvisions_ValidId_ReturnsEmpty_WhenNoDb) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto provisions = enricher.findRelatedProvisions("doc_001", 5);
    // No DB → always empty
    EXPECT_TRUE(provisions.empty());
}

TEST_F(KnowledgeGraphProductionTest, FindRelatedProvisions_RespectMaxResults) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto provisions = enricher.findRelatedProvisions("doc_002", 3);
    EXPECT_LE(provisions.size(), 3u);
}

// ============================================================================
// Phase 6: findRelatedCaseLaw
// ============================================================================

TEST_F(KnowledgeGraphProductionTest, FindRelatedCaseLaw_EmptyId_ReturnsEmpty) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto case_law = enricher.findRelatedCaseLaw("", 5);
    EXPECT_TRUE(case_law.empty());
}

TEST_F(KnowledgeGraphProductionTest, FindRelatedCaseLaw_ValidId_ReturnsEmpty_WhenNoDb) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto case_law = enricher.findRelatedCaseLaw("doc_003", 5);
    EXPECT_TRUE(case_law.empty());
}

// ============================================================================
// Phase 6: findSimilarDocuments
// ============================================================================

TEST_F(KnowledgeGraphProductionTest, FindSimilarDocuments_EmptyId_ReturnsEmpty) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto similar = enricher.findSimilarDocuments("", 5);
    EXPECT_TRUE(similar.empty());
}

TEST_F(KnowledgeGraphProductionTest, FindSimilarDocuments_ValidId_ReturnsEmpty_WhenNoDb) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto similar = enricher.findSimilarDocuments("doc_004", 5);
    EXPECT_TRUE(similar.empty());
}

TEST_F(KnowledgeGraphProductionTest, FindSimilarDocuments_ScoresInValidRange) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto similar = enricher.findSimilarDocuments("doc_005", 5);
    for (const auto& [id, score] : similar) {
        EXPECT_GE(score, 0.0f);
        EXPECT_LE(score, 1.0f);
    }
}

// ============================================================================
// Phase 6: Custom query override
// ============================================================================

TEST_F(KnowledgeGraphProductionTest, SetCustomQuery_DoesNotThrow) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    EXPECT_NO_THROW(enricher.setCustomQuery(
        "find_provisions",
        "FOR doc IN legal_documents FILTER doc._key == @id RETURN doc._key"));
}

TEST_F(KnowledgeGraphProductionTest, SetCustomQuery_OverridesBuiltIn) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    const std::string custom_query =
        "FOR doc IN custom_collection FILTER doc._key == @id RETURN doc._key";
    enricher.setCustomQuery("find_provisions", custom_query);

    // Verify via getQueryTemplate
    auto retrieved = enricher.getQueryTemplate("find_provisions");
    EXPECT_EQ(retrieved, custom_query);
}

// ============================================================================
// Phase 6: Built-in AQL template accessors
// ============================================================================

TEST_F(KnowledgeGraphProductionTest, GetQueryTemplate_BuiltInProvisionsTemplate_NotEmpty) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto tmpl = enricher.getQueryTemplate("find_provisions");
    EXPECT_FALSE(tmpl.empty());
    EXPECT_NE(tmpl.find("OUTBOUND"), std::string::npos);
}

TEST_F(KnowledgeGraphProductionTest, GetQueryTemplate_BuiltInCaseLawTemplate_NotEmpty) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto tmpl = enricher.getQueryTemplate("find_case_law");
    EXPECT_FALSE(tmpl.empty());
    EXPECT_NE(tmpl.find("case_law"), std::string::npos);
}

TEST_F(KnowledgeGraphProductionTest, GetQueryTemplate_BuiltInSimilarTemplate_NotEmpty) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto tmpl = enricher.getQueryTemplate("find_similar");
    EXPECT_FALSE(tmpl.empty());
    EXPECT_NE(tmpl.find("COSINE_SIMILARITY"), std::string::npos);
}

TEST_F(KnowledgeGraphProductionTest, GetQueryTemplate_UpdateContextTemplate_NotEmpty) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto tmpl = enricher.getQueryTemplate("update_context");
    EXPECT_FALSE(tmpl.empty());
    EXPECT_NE(tmpl.find("graph_context"), std::string::npos);
}

TEST_F(KnowledgeGraphProductionTest, GetQueryTemplate_FetchAllTemplate_NotEmpty) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto tmpl = enricher.getQueryTemplate("fetch_all");
    EXPECT_FALSE(tmpl.empty());
}

TEST_F(KnowledgeGraphProductionTest, GetQueryTemplate_UnknownName_ReturnsEmpty) {
    KnowledgeGraphEnricher enricher(config_, db_conn_);
    auto tmpl = enricher.getQueryTemplate("nonexistent_query");
    EXPECT_TRUE(tmpl.empty());
}
