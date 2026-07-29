/**
 * @file test_federated_rag_merger_focused.cpp
 * @brief Focused unit tests for cross-shard RAG result merge orchestration.
 *
 * Tests the core distributed knowledge RAG merge paths against the actual
 * production API surface:
 * - FederatedRAGMerger construction and configuration
 * - MergeStrategy enumeration (RECIPROCAL_RANK_FUSION, SCORE_WEIGHTED, ROUND_ROBIN)
 * - merge() with various shard results including failures and timeouts
 * - deduplication and top-K truncation contracts
 *
 * Target: Production readiness validation for RAG merge layer.
 * Q3 2026 Hardening: merge determinism, partial-failure contracts, dedup semantics.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <memory>

#include "distributed_knowledge/federated_rag_merger.h"

using namespace themis::distributed_knowledge;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixtures
// ─────────────────────────────────────────────────────────────────────────────

class FederatedRAGMergerTest : public ::testing::Test {
protected:
    void SetUp() override {
        cfg_.strategy    = MergeStrategy::RECIPROCAL_RANK_FUSION;
        cfg_.top_k       = 10;
        cfg_.deduplicate = true;
    }

    FederatedRAGMergerConfig cfg_;

    /// Build a ShardRetrievalResult with n documents.
    static ShardRetrievalResult makeShardResult(const std::string& shard_id, size_t n_docs,
                                                 bool ok = true) {
        ShardRetrievalResult r;
        r.shard_id  = shard_id;
        r.ok        = ok;
        r.latency_ms = 5;
        for (size_t i = 0; i < n_docs; ++i) {
            RetrievedDocument doc;
            doc.doc_id          = shard_id + "_doc" + std::to_string(i);
            doc.content         = "content " + std::to_string(i);
            doc.shard_id        = shard_id;
            doc.relevance_score = 1.0 - (0.1 * static_cast<double>(i));
            doc.rank_in_shard   = i + 1;
            r.documents.push_back(doc);
        }
        return r;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Config and Strategy Tests (FRM-01..FRM-03)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FRM-01: FederatedRAGMergerConfig validation — valid config.
 *
 * Verifies that a properly constructed FederatedRAGMergerConfig reports
 * isValid() == true.
 */
TEST_F(FederatedRAGMergerTest, ValidConfigReportsValid) {
    EXPECT_TRUE(cfg_.isValid());
}

/**
 * @test FRM-02: MergeStrategy enumeration values.
 *
 * Verifies that all three merge strategy values are accessible.
 */
TEST_F(FederatedRAGMergerTest, MergeStrategyEnumeration) {
    // All three strategies must be distinct values.
    EXPECT_NE(static_cast<int>(MergeStrategy::RECIPROCAL_RANK_FUSION),
              static_cast<int>(MergeStrategy::SCORE_WEIGHTED));
    EXPECT_NE(static_cast<int>(MergeStrategy::SCORE_WEIGHTED),
              static_cast<int>(MergeStrategy::ROUND_ROBIN));
    EXPECT_NE(static_cast<int>(MergeStrategy::RECIPROCAL_RANK_FUSION),
              static_cast<int>(MergeStrategy::ROUND_ROBIN));
}

/**
 * @test FRM-03: Invalid config (top_k == 0) is rejected.
 *
 * Verifies that a config with top_k == 0 is rejected by isValid().
 */
TEST_F(FederatedRAGMergerTest, InvalidConfigTopKZeroRejected) {
    cfg_.top_k = 0;
    EXPECT_FALSE(cfg_.isValid());
}

// ─────────────────────────────────────────────────────────────────────────────
// Merge Correctness Tests (FRM-04..FRM-06)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FRM-04: merge() with two healthy shards returns correct document counts.
 *
 * Verifies that merging results from two shards (5 docs each) produces a
 * MergedRAGContext with the expected shards_queried and shards_responded values.
 */
TEST_F(FederatedRAGMergerTest, MergeWithTwoHealthyShardsCorrectCounts) {
    FederatedRAGMerger merger(cfg_);
    std::vector<ShardRetrievalResult> results = {
        makeShardResult("shard-001", 5),
        makeShardResult("shard-002", 5),
    };

    MergedRAGContext ctx = merger.merge(results);

    EXPECT_EQ(ctx.shards_queried,   2u);
    EXPECT_EQ(ctx.shards_responded, 2u);
    EXPECT_LE(ctx.documents.size(), cfg_.top_k);
}

/**
 * @test FRM-05: merge() skips failed shard (ok == false) gracefully.
 *
 * Verifies that a shard with ok == false is excluded from the merged context
 * but the merge succeeds with the remaining responding shards.
 */
TEST_F(FederatedRAGMergerTest, PartialShardFailureSkipsFailedShard) {
    FederatedRAGMerger merger(cfg_);
    ShardRetrievalResult failed = makeShardResult("shard-003", 5, /*ok=*/false);
    failed.error_message = "shard-003 unavailable";

    std::vector<ShardRetrievalResult> results = {
        makeShardResult("shard-001", 5),
        makeShardResult("shard-002", 5),
        failed,
    };

    MergedRAGContext ctx = merger.merge(results);

    EXPECT_EQ(ctx.shards_queried,   3u);
    EXPECT_EQ(ctx.shards_responded, 2u);
    EXPECT_FALSE(ctx.documents.empty());
}

/**
 * @test FRM-06: merge() with shard_timeout_ms == 0 throws when all shards time out.
 *
 * Verifies the documented contract: when shard_timeout_ms == 0, merge() throws
 * "all shards timed out" immediately.
 */
TEST_F(FederatedRAGMergerTest, AllShardsTimedOutThrows) {
    cfg_.shard_timeout_ms = 0;
    FederatedRAGMerger merger(cfg_);

    ShardRetrievalResult t1 = makeShardResult("shard-001", 3);
    t1.timed_out = true;
    t1.ok        = false;
    ShardRetrievalResult t2 = makeShardResult("shard-002", 3);
    t2.timed_out = true;
    t2.ok        = false;

    EXPECT_THROW(merger.merge({t1, t2}), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// Deduplication and Top-K Tests (FRM-07..FRM-08)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FRM-07: Deduplication removes documents with identical doc_ids.
 *
 * Verifies that when two shards return the same doc_id, only one copy
 * appears in the merged output.
 */
TEST_F(FederatedRAGMergerTest, DeduplicationRemovesDuplicateDocIds) {
    cfg_.deduplicate = true;
    FederatedRAGMerger merger(cfg_);

    // Both shards return the same doc_id for the first document.
    auto r1 = makeShardResult("shard-001", 3);
    auto r2 = makeShardResult("shard-002", 3);
    r2.documents[0].doc_id = r1.documents[0].doc_id;  // duplicate

    MergedRAGContext ctx = merger.merge({r1, r2});

    // Total candidates = 6, but after dedup one duplicate is removed
    EXPECT_LT(ctx.unique_doc_count, ctx.total_candidate_count);
}

/**
 * @test FRM-08: Top-K truncation is respected.
 *
 * Verifies that the merged output never exceeds top_k documents.
 */
TEST_F(FederatedRAGMergerTest, TopKTruncationRespected) {
    cfg_.top_k = 3;
    FederatedRAGMerger merger(cfg_);

    std::vector<ShardRetrievalResult> results = {
        makeShardResult("shard-001", 5),
        makeShardResult("shard-002", 5),
    };

    MergedRAGContext ctx = merger.merge(results);
    EXPECT_LE(ctx.documents.size(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stateless Merger and Serialization Tests (FRM-09..FRM-12)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @test FRM-09: FederatedRAGMerger exposes its config correctly.
 *
 * Verifies that the config passed to the constructor is accessible via config().
 */
TEST_F(FederatedRAGMergerTest, MergerExposesConfig) {
    FederatedRAGMerger merger(cfg_);
    EXPECT_EQ(merger.config().top_k,    cfg_.top_k);
    EXPECT_EQ(merger.config().strategy, cfg_.strategy);
}

/**
 * @test FRM-10: merge() with empty shard result list returns empty context.
 *
 * Verifies that an empty input produces a zero-document MergedRAGContext.
 */
TEST_F(FederatedRAGMergerTest, MergeEmptyShardListReturnsEmptyContext) {
    FederatedRAGMerger merger(cfg_);
    MergedRAGContext ctx = merger.merge({});

    EXPECT_EQ(ctx.shards_queried,   0u);
    EXPECT_EQ(ctx.shards_responded, 0u);
    EXPECT_TRUE(ctx.documents.empty());
}

/**
 * @test FRM-11: Merger is stateless — successive calls are independent.
 *
 * Verifies that calling merge() twice on the same merger instance
 * produces independent contexts (stateless contract).
 */
TEST_F(FederatedRAGMergerTest, MergerIsStateless) {
    FederatedRAGMerger merger(cfg_);
    auto results = std::vector<ShardRetrievalResult>{
        makeShardResult("shard-001", 3),
        makeShardResult("shard-002", 3),
    };

    MergedRAGContext ctx1 = merger.merge(results);
    MergedRAGContext ctx2 = merger.merge(results);

    EXPECT_EQ(ctx1.documents.size(), ctx2.documents.size());
    EXPECT_EQ(ctx1.shards_responded, ctx2.shards_responded);
}

/**
 * @test FRM-12: RetrievedDocument JSON serialization.
 *
 * Verifies that a RetrievedDocument serialises to JSON with the expected fields.
 */
TEST_F(FederatedRAGMergerTest, RetrievedDocumentSerialization) {
    RetrievedDocument doc;
    doc.doc_id          = "doc-42";
    doc.content         = "test content";
    doc.shard_id        = "shard-001";
    doc.relevance_score = 0.95;
    doc.rank_in_shard   = 1;

    json j = doc.toJson();

    EXPECT_EQ(j["doc_id"],          "doc-42");
    EXPECT_EQ(j["shard_id"],        "shard-001");
    EXPECT_DOUBLE_EQ(j["relevance_score"].get<double>(), 0.95);
    EXPECT_EQ(j["rank_in_shard"].get<size_t>(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Summary
// ─────────────────────────────────────────────────────────────────────────────

/*
 * Test Coverage Summary (FRM-01..FRM-12):
 *
 * FRM-01: Valid FederatedRAGMergerConfig reports isValid()
 * FRM-02: MergeStrategy enumeration — all three values are distinct
 * FRM-03: Invalid config (top_k==0) rejected by isValid()
 * FRM-04: merge() with two healthy shards returns correct counts
 * FRM-05: Partial shard failure (ok==false) skipped gracefully
 * FRM-06: All-shards-timed-out throws with shard_timeout_ms==0
 * FRM-07: Deduplication removes documents with identical doc_ids
 * FRM-08: Top-K truncation respected in merged output
 * FRM-09: Merger exposes its config correctly
 * FRM-10: merge() with empty shard list returns empty context
 * FRM-11: Merger is stateless — successive calls are independent
 * FRM-12: RetrievedDocument JSON serialization
 *
 * Target: Q3 2026 Hardening - merge determinism, partial-failure contracts.
 * Status: Focused unit test suite for federated RAG merge layer.
 */
