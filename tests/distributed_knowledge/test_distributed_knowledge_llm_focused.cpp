/**
 * @file test_distributed_knowledge_llm_focused.cpp
 * @brief Group DK — FederatedRAGMerger LLM-context build and merge path tests.
 */

#include <gtest/gtest.h>
#include "distributed_knowledge/federated_rag_merger.h"

#include <string>
#include <vector>
#include <chrono>

using namespace themis::distributed_knowledge;

// ── DK1: Construct with valid config does not throw ──────────────────────────
TEST(DistributedKnowledgeLlmFocused, DK1_ConstructValidConfig_NoThrow) {
    FederatedRAGMergerConfig cfg;
    EXPECT_NO_THROW({ FederatedRAGMerger merger(cfg); });
}

// ── DK2: Construct with top_k=0 throws invalid_argument ──────────────────────
TEST(DistributedKnowledgeLlmFocused, DK2_ZeroTopK_ThrowsInvalidArgument) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 0;
    EXPECT_THROW({ FederatedRAGMerger merger(cfg); }, std::invalid_argument);
}

// ── DK3: Merge empty shard results returns empty merged result ────────────────
TEST(DistributedKnowledgeLlmFocused, DK3_MergeEmpty_ReturnsEmptyResult) {
    FederatedRAGMergerConfig cfg;
    FederatedRAGMerger merger(cfg);

    std::vector<ShardRetrievalResult> shards;
    auto merged = merger.merge(shards);
    EXPECT_TRUE(merged.documents.empty());
}

// ── DK4: Merge single shard with one document returns that document ───────────
TEST(DistributedKnowledgeLlmFocused, DK4_MergeSingleShard_ReturnsDocuments) {
    FederatedRAGMergerConfig cfg;
    FederatedRAGMerger merger(cfg);

    ShardRetrievalResult shard;
    shard.shard_id = "shard-0";
    shard.timed_out = false;
    shard.ok = true;
    RetrievedDocument doc;
    doc.doc_id = "doc-1";
    doc.content = "Some knowledge text";
    doc.shard_id = "shard-0";
    doc.relevance_score = 0.9;
    doc.rank_in_shard = 1;
    shard.documents.push_back(doc);

    auto merged = merger.merge({shard});
    ASSERT_FALSE(merged.documents.empty());
    EXPECT_EQ(merged.documents[0].doc_id, "doc-1");
}

// ── DK5: buildPromptContext with one document produces non-empty context ───────
TEST(DistributedKnowledgeLlmFocused, DK5_BuildPromptContext_ReturnsNonEmpty) {
    FederatedRAGMergerConfig cfg;
    FederatedRAGMerger merger(cfg);

    ShardRetrievalResult shard;
    shard.shard_id = "shard-0";
    shard.ok = true;
    RetrievedDocument doc;
    doc.doc_id = "doc-1";
    doc.content = "Federated knowledge fragment";
    doc.shard_id = "shard-0";
    doc.relevance_score = 0.8;
    doc.rank_in_shard = 1;
    shard.documents.push_back(doc);

    auto merged = merger.merge({shard});
    auto context = merged.buildPromptContext(5, 4096);
    EXPECT_FALSE(context.empty());
}

// ── DK6: Merge respects top_k limit ───────────────────────────────────────────
TEST(DistributedKnowledgeLlmFocused, DK6_TopKLimit_Respected) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 2;
    FederatedRAGMerger merger(cfg);

    ShardRetrievalResult shard;
    shard.shard_id = "shard-0";
    shard.ok = true;
    for (int i = 0; i < 5; ++i) {
        RetrievedDocument d;
        d.doc_id = "doc-" + std::to_string(i);
        d.content = "text-" + std::to_string(i);
        d.shard_id = "shard-0";
        d.relevance_score = static_cast<double>(5 - i) * 0.1;
        d.rank_in_shard = static_cast<size_t>(i + 1);
        shard.documents.push_back(d);
    }

    auto merged = merger.merge({shard});
    EXPECT_LE(merged.documents.size(), 2u);
}

// ── DK7: Timed-out shard is handled gracefully ───────────────────────────────
TEST(DistributedKnowledgeLlmFocused, DK7_TimedOutShard_GracefulDegradation) {
    FederatedRAGMergerConfig cfg;
    FederatedRAGMerger merger(cfg);

    ShardRetrievalResult timed_shard;
    timed_shard.shard_id = "slow-shard";
    timed_shard.timed_out = true;
    timed_shard.ok = false;

    ShardRetrievalResult ok_shard;
    ok_shard.shard_id = "fast-shard";
    ok_shard.timed_out = false;
    ok_shard.ok = true;
    RetrievedDocument d;
    d.doc_id = "doc-ok";
    d.content = "responsive content";
    d.shard_id = "fast-shard";
    d.relevance_score = 0.7;
    d.rank_in_shard = 1;
    ok_shard.documents.push_back(d);

    auto merged = merger.merge({timed_shard, ok_shard});
    bool found = false;
    for (const auto& doc : merged.documents) {
        if (doc.doc_id == "doc-ok") found = true;
    }
    EXPECT_TRUE(found);
}
