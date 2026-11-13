#include <gtest/gtest.h>
#include "rag/rag_retrieval_helpers.h"
#include "llm/rag_helpers.h"

#include <limits>

using namespace themis;

TEST(RAGRetrievalHelpers, ExtractsUsableTextWithoutInventingContent) {
    const auto candidate = rag::makeRAGCandidate(
        "d1", "docs", {{"content", " "}, {"text", "real evidence"}}, 0.8f, "retrieve:vector");
    ASSERT_TRUE(candidate);
    EXPECT_EQ(candidate->content, "real evidence");
    EXPECT_FALSE(rag::makeRAGCandidate("missing", "docs", {{"_key", "missing"}}, 1, "retrieve:graph"));
    EXPECT_FALSE(rag::makeRAGCandidate("empty", "docs", {{"content", ""}}, 1, "retrieve:vector"));
}

TEST(RAGRetrievalHelpers, SerializesRealStructuredRows) {
    const nlohmann::json row = {
        {"name", "Berlin"},
        {"population", 3500000},
        {"_key", "berlin"},
        {"source_ref_type", "content_chunk"},
        {"source_ref_collection", "places"},
        {"source_ref_key", "chunk:berlin"},
        {"source_ref_provenance", "content_manager"},
        {"source_ref_version", "2026-11-11"},
        {"metadata.owner", "internal"},
        {"embedding", {0.1, 0.2}}
    };
    const auto candidate = rag::makeRAGCandidate("berlin", "cities", row, 1, "retrieve:relational");
    ASSERT_TRUE(candidate);
    EXPECT_EQ(nlohmann::json::parse(candidate->content),
              (nlohmann::json{{"name", "Berlin"}, {"population", 3500000}}));
    ASSERT_TRUE(candidate->source_reference.has_value());
    EXPECT_EQ(candidate->source_reference->key, "chunk:berlin");
}

TEST(RAGRetrievalHelpers, RejectsInvalidIdentityAndScores) {
    const nlohmann::json entity = {{"content", "text"}};
    EXPECT_FALSE(rag::makeRAGCandidate("", "docs", entity, 1, "retrieve:vector"));
    EXPECT_FALSE(rag::makeRAGCandidate("d1", "docs", nullptr, 1, "retrieve:vector"));
    EXPECT_FALSE(rag::makeRAGCandidate(
        "d1", "docs", entity, std::numeric_limits<float>::quiet_NaN(), "retrieve:vector"));
    EXPECT_FALSE(rag::makeRAGCandidate(
        "d1", "docs", entity, std::numeric_limits<float>::infinity(), "retrieve:vector"));
}

TEST(RAGRetrievalHelpers, LLMConversionPreservesIdentityAndProvenance) {
    const auto candidate = rag::makeRAGCandidate(
        "d1", "docs", {{"content", "evidence"}}, -0.5f, "retrieve:vector");
    ASSERT_TRUE(candidate);
    const auto documents = llm::toRAGDocuments({*candidate});
    ASSERT_EQ(documents.size(), 1);
    EXPECT_EQ(documents.front().source, "d1");
    EXPECT_EQ(documents.front().content, "evidence");
    EXPECT_FLOAT_EQ(documents.front().relevance_score, -0.5f);
    EXPECT_EQ(documents.front().metadata["collection"], "docs");
    EXPECT_EQ(documents.front().metadata["provenance"][0], "retrieve:vector");
}
