/**
 * @file test_rag_ingestion_bridge_hardening_focused.cpp
 * @brief Focused regression tests for ingestion bridge fail-closed behavior
 *        and context-hydration robustness.
 *
 * Roadmap Item: Ingestion bridge and context-hydration hardening for fail-closed
 *               retrieval inputs (Target: Q3 2026)
 *
 * Test Suite: RagIngestionBridgeHardeningFocusedTests
 *   Group A – Fail-closed on malformed input
 *   Group B – Missing metadata handling
 *   Group C – Empty retrieval handling
 *   Group D – Deterministic hydration
 *   Group E – Error recovery and diagnostics
 */

#include <gtest/gtest.h>

#include "ingestion/ingestion_sinks.h"
#include "rag/rag_ingestion_bridge.h"
#include "toolbox/ingestion_toolbox.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::ingestion;
using namespace themis::rag;
using namespace themis::toolbox;

namespace {

constexpr std::size_t kMaxDocumentChars = 5u * 1024u * 1024u;
constexpr std::size_t kMaxCollectionChars = 256u;
constexpr std::size_t kMaxFilenameChars = 512u;
constexpr std::size_t kMaxChunkSnippetChars = 128u * 1024u;
constexpr std::size_t kMaxMetadataValueChars = 16u * 1024u;

std::shared_ptr<IngestionToolbox> makeToolbox()
{
    return IngestionToolbox::createDefault();
}

std::vector<std::string> sortedChunkIds(const InMemoryVectorWriter& writer)
{
    std::vector<std::string> chunk_ids;
    chunk_ids.reserve(writer.records().size());
    for (const auto& [chunk_id, record] : writer.records()) {
        static_cast<void>(record);
        chunk_ids.push_back(chunk_id);
    }
    std::sort(chunk_ids.begin(), chunk_ids.end());
    return chunk_ids;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Group A – Fail-closed on malformed input
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagIngestionBridgeHardeningFocusedTests, A1_RejectedIfNullToolbox) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    auto graph_writer = std::make_shared<InMemoryGraphWriter>();

    EXPECT_THROW(
        {
            RAGIngestionBridge bridge(nullptr, vector_writer, graph_writer);
        },
        std::invalid_argument);
}

TEST(RagIngestionBridgeHardeningFocusedTests, A2_EmptyDocumentHandledSafely) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    auto graph_writer = std::make_shared<InMemoryGraphWriter>();
    RAGIngestionBridge bridge(makeToolbox(), vector_writer, graph_writer);

    const auto result = bridge.indexDocument("", "test-coll");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "empty input");
    EXPECT_TRUE(result.doc_id.empty());
    EXPECT_EQ(result.vector_count, 0u);
    EXPECT_EQ(result.entity_count, 0u);
    EXPECT_EQ(vector_writer->vectorCount(), 0u);
    EXPECT_EQ(graph_writer->nodeCount(), 0u);
    EXPECT_EQ(graph_writer->edgeCount(), 0u);
}

TEST(RagIngestionBridgeHardeningFocusedTests, A3_ExcessivelyLargeDocumentRejected) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    RAGIngestionBridge bridge(makeToolbox(), vector_writer);

    const auto result =
        bridge.indexDocument(std::string(kMaxDocumentChars + 1u, 'x'), "oversized-coll");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "input too large");
    EXPECT_TRUE(result.doc_id.empty());
    EXPECT_EQ(vector_writer->vectorCount(), 0u);
}

TEST(RagIngestionBridgeHardeningFocusedTests, A4_InvalidCollectionNameHandledFailClosed) {
    RAGIngestionBridge bridge(makeToolbox());

    const auto too_long =
        bridge.indexDocument("valid text", std::string(kMaxCollectionChars + 1u, 'c'));
    EXPECT_FALSE(too_long.ok);
    EXPECT_EQ(too_long.error, "invalid collection");

    const auto with_control =
        bridge.indexDocument("valid text", std::string("bad\001collection", 14));
    EXPECT_FALSE(with_control.ok);
    EXPECT_EQ(with_control.error, "invalid collection");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group B – Missing metadata handling
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagIngestionBridgeHardeningFocusedTests, B1_MissingSourceDocIDHandledGracefully) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    RAGIngestionBridge bridge(makeToolbox(), vector_writer);

    const auto result = bridge.indexDocument("Canonical retrieval body", "canonical-coll");

    ASSERT_TRUE(result.ok);
    ASSERT_GT(vector_writer->vectorCount(), 0u);
    for (const auto& [chunk_id, record] : vector_writer->records()) {
        static_cast<void>(chunk_id);
        ASSERT_TRUE(record.metadata.contains("source"));
        ASSERT_TRUE(record.metadata.contains("content"));
        ASSERT_TRUE(record.metadata.contains("text"));
        ASSERT_TRUE(record.metadata.contains("body"));
        EXPECT_FALSE(record.metadata.at("source").empty());
        EXPECT_FALSE(record.metadata.at("content").empty());
        EXPECT_EQ(record.metadata.at("text"), record.metadata.at("content"));
        EXPECT_EQ(record.metadata.at("body"), record.metadata.at("content"));
    }
}

TEST(RagIngestionBridgeHardeningFocusedTests, B2_NullMetadataValuesBoundedSafely) {
    RAGIngestionBridge bridge(makeToolbox());

    std::vector<judge::RetrievedDocument> docs(1);
    docs[0].id = "doc-safe";
    docs[0].content = "Canonical content for hydration";
    docs[0].metadata["source"] = "   \t";
    docs[0].metadata["content"] = " \n\r ";

    const auto enriched = bridge.enrichRetrievedDocuments(docs);

    EXPECT_LE(enriched, docs.size());
    ASSERT_TRUE(docs[0].metadata.contains("source"));
    ASSERT_TRUE(docs[0].metadata.contains("content"));
    EXPECT_EQ(docs[0].metadata.at("source"), "doc-safe");
    EXPECT_EQ(docs[0].metadata.at("content"), "Canonical content for hydration");
}

TEST(RagIngestionBridgeHardeningFocusedTests, B3_ControlCharactersInMetadataDetected) {
    RAGIngestionBridge bridge(makeToolbox());

    const auto invalid_mime =
        bridge.indexDocument("valid text", "valid-coll", std::string("text/\001plain", 11));
    EXPECT_FALSE(invalid_mime.ok);
    EXPECT_EQ(invalid_mime.error, "invalid mime");

    const auto invalid_filename =
        bridge.indexDocument("valid text", "valid-coll", "text/plain", std::string("bad\001name.txt", 12));
    EXPECT_FALSE(invalid_filename.ok);
    EXPECT_EQ(invalid_filename.error, "invalid filename");
}

// ─────────────────────────────────────────────────────────────────────────────
// Group C – Empty retrieval handling
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagIngestionBridgeHardeningFocusedTests, C1_EmptyRetrievalResultsHandledCorrectly) {
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs;

    const auto enriched = bridge.enrichRetrievedDocuments(docs);

    EXPECT_EQ(enriched, 0u);
    EXPECT_TRUE(docs.empty());
}

TEST(RagIngestionBridgeHardeningFocusedTests, C2_MissingChunkContentNotCrashing) {
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs(1);
    docs[0].id = "doc-empty";
    docs[0].content = " \n\t ";

    const auto enriched = bridge.enrichRetrievedDocuments(docs);

    EXPECT_EQ(enriched, 0u);
    EXPECT_TRUE(docs[0].metadata.empty());
}

TEST(RagIngestionBridgeHardeningFocusedTests, C3_AllChunksTruncatedStillUsable) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    RAGIngestionBridge bridge(makeToolbox(), vector_writer);

    const std::string text(200'000u, 'x');
    const auto result = bridge.indexDocument(text, "bounded-coll");

    ASSERT_TRUE(result.ok);
    ASSERT_GT(vector_writer->vectorCount(), 0u);
    for (const auto& [chunk_id, record] : vector_writer->records()) {
        static_cast<void>(chunk_id);
        EXPECT_LE(record.text_snippet.size(), kMaxChunkSnippetChars);
        ASSERT_TRUE(record.metadata.contains("content"));
        EXPECT_LE(record.metadata.at("content").size(), kMaxMetadataValueChars);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Group D – Deterministic hydration
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagIngestionBridgeHardeningFocusedTests, D1_SameInputProducesDeterministicID) {
    RAGIngestionBridge bridge(makeToolbox());
    const std::string text = "Deterministic input for document hashing";

    const auto first = bridge.indexDocument(text, "deterministic-coll");
    const auto second = bridge.indexDocument(text, "deterministic-coll");

    ASSERT_TRUE(first.ok);
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(first.doc_id, second.doc_id);
    EXPECT_EQ(first.collection, second.collection);
}

TEST(RagIngestionBridgeHardeningFocusedTests, D2_EntityExtractionDeterministic) {
    RAGIngestionBridge bridge(makeToolbox());
    const std::string text = "Alice from Example Corp visited Berlin on Tuesday.";

    const auto first = bridge.extractEntitiesForContext(text);
    const auto second = bridge.extractEntitiesForContext(text);

    EXPECT_EQ(first.size(), second.size());
    EXPECT_EQ(RAGIngestionBridge::buildEntityContext(first),
              RAGIngestionBridge::buildEntityContext(second));
}

TEST(RagIngestionBridgeHardeningFocusedTests, D3_VectorChunkOrderingDeterministic) {
    auto first_writer = std::make_shared<InMemoryVectorWriter>();
    auto second_writer = std::make_shared<InMemoryVectorWriter>();
    RAGIngestionBridge first_bridge(makeToolbox(), first_writer);
    RAGIngestionBridge second_bridge(makeToolbox(), second_writer);

    const std::string text =
        "Stable chunk ordering matters for repeatable retrieval hydration checks.";

    const auto first = first_bridge.indexDocument(text, "order-coll");
    const auto second = second_bridge.indexDocument(text, "order-coll");

    ASSERT_TRUE(first.ok);
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(first.doc_id, second.doc_id);
    EXPECT_EQ(sortedChunkIds(*first_writer), sortedChunkIds(*second_writer));
}

// ─────────────────────────────────────────────────────────────────────────────
// Group E – Error recovery and diagnostics
// ─────────────────────────────────────────────────────────────────────────────

TEST(RagIngestionBridgeHardeningFocusedTests, E1_InvalidMimeReturnedAsError) {
    RAGIngestionBridge bridge(makeToolbox());

    const auto result = bridge.indexDocument("valid text", "diag-coll", "");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "invalid mime");
}

TEST(RagIngestionBridgeHardeningFocusedTests, E2_InvalidFilenameReturnedAsError) {
    RAGIngestionBridge bridge(makeToolbox());

    const auto result = bridge.indexDocument(
        "valid text",
        "diag-coll",
        "text/plain",
        std::string(kMaxFilenameChars + 1u, 'f'));

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "invalid filename");
}

TEST(RagIngestionBridgeHardeningFocusedTests, E3_RejectedInputDoesNotWritePartialState) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    auto graph_writer = std::make_shared<InMemoryGraphWriter>();
    RAGIngestionBridge bridge(makeToolbox(), vector_writer, graph_writer);

    const auto result = bridge.indexDocument("valid text", " \t ");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, "invalid collection");
    EXPECT_EQ(vector_writer->vectorCount(), 0u);
    EXPECT_EQ(graph_writer->nodeCount(), 0u);
    EXPECT_EQ(graph_writer->edgeCount(), 0u);
}

TEST(RagIngestionBridgeHardeningFocusedTests, E4_RecoveryPathOptionalGraphWriter) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    RAGIngestionBridge bridge(makeToolbox(), vector_writer, nullptr);

    const auto result = bridge.indexDocument("Vector-only indexing remains supported.", "vector-only");

    ASSERT_TRUE(result.ok);
    EXPECT_GT(vector_writer->vectorCount(), 0u);
    EXPECT_EQ(result.vector_count, vector_writer->vectorCount());
}

TEST(RagIngestionBridgeHardeningFocusedTests, E5_ErrorMessageNotEmpty) {
    RAGIngestionBridge bridge(makeToolbox());

    const auto result = bridge.indexDocument("");

    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error.empty());
    EXPECT_NE(result.error.find("empty"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration coverage
// ─────────────────────────────────────────────────────────────────────────────

class RagIngestionBridgeIntegrationTests : public ::testing::Test {
protected:
    std::shared_ptr<IngestionToolbox> toolbox = makeToolbox();
    std::shared_ptr<InMemoryVectorWriter> vector_writer =
        std::make_shared<InMemoryVectorWriter>();
    std::shared_ptr<InMemoryGraphWriter> graph_writer =
        std::make_shared<InMemoryGraphWriter>();
};

TEST_F(RagIngestionBridgeIntegrationTests, IntegrationA_FullIndexingWorkflow) {
    RAGIngestionBridge bridge(toolbox, vector_writer, graph_writer);

    const auto result = bridge.indexDocument(
        "ACME signed a supply agreement with Berlin Logistics GmbH.",
        "integration-coll");

    ASSERT_TRUE(result.ok);
    EXPECT_FALSE(result.doc_id.empty());
    EXPECT_EQ(result.collection, "integration-coll");
    EXPECT_GT(vector_writer->vectorCount(), 0u);
}
