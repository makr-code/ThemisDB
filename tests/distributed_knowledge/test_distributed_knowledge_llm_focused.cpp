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
    cfg.top_k = 10;
    EXPECT_NO_THROW({ FederatedRAGMerger merger(cfg); });
}

// ── DK2: Construct with max_merged_docs=0 throws invalid_argument ─────────────
TEST(DistributedKnowledgeLlmFocused, DK2_ZeroMaxDocs_ThrowsInvalidArgument) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 0;
    EXPECT_THROW({ FederatedRAGMerger merger(cfg); }, std::invalid_argument);
}

// ── DK3: Merge empty shard results returns empty merged result ────────────────
TEST(DistributedKnowledgeLlmFocused, DK3_MergeEmpty_ReturnsEmptyResult) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 10;
    FederatedRAGMerger merger(cfg);

    std::vector<ShardRetrievalResult> shards;
    auto merged = merger.merge(shards);
    EXPECT_TRUE(merged.documents.empty());
}

// ── DK4: Merge single shard with one document returns that document ───────────
TEST(DistributedKnowledgeLlmFocused, DK4_MergeSingleShard_ReturnsDocuments) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 10;
    FederatedRAGMerger merger(cfg);

    ShardRetrievalResult shard;
    shard.shard_id = "shard-0";
    shard.timed_out = false;
    RetrievedDocument doc;
    doc.doc_id     = "doc-1";
    doc.relevance_score = 0.9;
    doc.content         = "Some knowledge text";
    shard.documents.push_back(doc);

    auto merged = merger.merge({shard});
    ASSERT_FALSE(merged.documents.empty());
    EXPECT_EQ(merged.documents[0].doc_id, "doc-1");
}

// ── DK5: buildPromptContext with one document produces non-empty context ───────
TEST(DistributedKnowledgeLlmFocused, DK5_BuildPromptContext_ReturnsNonEmpty) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 5;
    FederatedRAGMerger merger(cfg);

    ShardRetrievalResult shard;
    shard.shard_id = "shard-0";
    RetrievedDocument doc;
    doc.doc_id = "doc-1";
    doc.relevance_score = 0.8;
    doc.content         = "Federated knowledge fragment";
    shard.documents.push_back(doc);

    auto merged  = merger.merge({shard});
    auto context = merged.buildPromptContext();
    EXPECT_FALSE(context.empty());
}

// ── DK6: Merge respects max_merged_docs limit ────────────────────────────────
TEST(DistributedKnowledgeLlmFocused, DK6_MaxDocsLimit_Respected) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 2;
    FederatedRAGMerger merger(cfg);

    ShardRetrievalResult shard;
    shard.shard_id = "shard-0";
    for (int i = 0; i < 5; ++i) {
        RetrievedDocument d;
        d.doc_id = "doc-" + std::to_string(i);
        d.relevance_score = static_cast<double>(5 - i) * 0.1;
        d.content         = "text-" + std::to_string(i);
        shard.documents.push_back(d);
    }

    auto merged = merger.merge({shard});
    EXPECT_LE(merged.documents.size(), 2u);
}

// ── DK7: Timed-out shard is handled gracefully ───────────────────────────────
TEST(DistributedKnowledgeLlmFocused, DK7_TimedOutShard_GracefulDegradation) {
    FederatedRAGMergerConfig cfg;
    cfg.top_k = 10;
    FederatedRAGMerger merger(cfg);

    ShardRetrievalResult timed_shard;
    timed_shard.shard_id = "slow-shard";
    timed_shard.timed_out = true;

    ShardRetrievalResult ok_shard;
    ok_shard.shard_id = "fast-shard";
    ok_shard.timed_out = false;
    RetrievedDocument d;
    d.doc_id = "doc-ok";
    d.relevance_score = 0.7;
    d.content         = "responsive content";
    ok_shard.documents.push_back(d);

    auto merged = merger.merge({timed_shard, ok_shard});
    // At least the ok shard document should appear
    bool found = false;
    for (const auto& doc : merged.documents) {
        if (doc.doc_id == "doc-ok") found = true;
    }
    EXPECT_TRUE(found);
}
