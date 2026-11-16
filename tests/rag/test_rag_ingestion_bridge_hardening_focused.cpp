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
#include "ingestion/builtin_step_factories.h"
#include "ingestion/inference_backend.h"
#include "ingestion/workflow_engine.h"
#include "rag/rag_ingestion_bridge.h"
#include "toolbox/ingestion_toolbox.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
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
    std::vector<std::string> chunk_ids = {};

    chunk_ids.reserve(writer.records().size());
    for (const auto& [chunk_id, record] : writer.records()) {
        static_cast<void>(record);
        chunk_ids.push_back(chunk_id);
    }
    std::sort(chunk_ids.begin(), chunk_ids.end());
    return chunk_ids;
}

class PopulateChunkStep final : public IIngestionStep
{
public:
    const char* getName() const override { return "test.populate_chunk"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool initialize(const char*) override { return true; }
    void shutdown() override {}
    void* getInstance() override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& ctx, const StepConfig&) override
    {
        TextChunk chunk;
        chunk.seq = 0;
        chunk.text = ctx.raw_text;
        ctx.chunks.push_back(std::move(chunk));

        BaseEntity first;
        first.id = "entity-a:" + ctx.manifest.file_id;
        first.source_file_id = ctx.manifest.file_id;
        first.text = ctx.raw_text;
        ctx.entities.push_back(std::move(first));

        BaseEntity second;
        second.id = "entity-b:" + ctx.manifest.file_id;
        second.source_file_id = ctx.manifest.file_id;
        second.text = ctx.raw_text;
        ctx.entities.push_back(std::move(second));

        EntityRelation relation;
        relation.from_id = ctx.entities[0].id;
        relation.to_id = ctx.entities[1].id;
        relation.relation_type = RelationType::CO_OCCURS;
        ctx.relations.push_back(std::move(relation));
        return {};
    }
};

class DeterministicEmbeddingBackend final : public IEmbeddingBackend
{
public:
    std::vector<float> embed(const std::string&) override { return {1.0f, 0.0f}; }
    int dimensions() const override { return 2; }
    bool isAvailable() const override { return true; }
    std::string description() const override { return "deterministic test backend"; }
};

class FailingVectorWriter final : public IVectorWriter
{
public:
    Result<void> writeVectors(const std::vector<VectorRecord>&) override
    {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "injected vector failure");
    }

    std::size_t vectorCount() const override { return 0; }
    const VectorRecord* findByChunkId(const std::string&) const override
    {
        return nullptr;
    }
};

class FaultInjectingGraphWriter final : public IGraphWriter
{
public:
    explicit FaultInjectingGraphWriter(bool fail_entities, bool fail_relations)
        : fail_entities_(fail_entities), fail_relations_(fail_relations)
    {}

    Result<void> writeEntities(const std::vector<BaseEntity>& nodes) override
    {
        if (fail_entities_) {
            return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                           "injected graph entity failure");
        }
        node_count_ += nodes.size();
        return {};
    }

    Result<void> writeRelations(const std::vector<EntityRelation>& edges) override
    {
        ++relation_calls_;
        if (fail_relations_) {
            return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                           "injected graph relation failure");
        }
        edge_count_ += edges.size();
        return {};
    }

    std::size_t nodeCount() const override { return node_count_; }
    std::size_t edgeCount() const override { return edge_count_; }
    std::size_t relationCalls() const { return relation_calls_; }

private:
    bool fail_entities_;
    bool fail_relations_;
    std::size_t node_count_{0};
    std::size_t edge_count_{0};
    std::size_t relation_calls_{0};
};

std::shared_ptr<IngestionToolbox> makeWorkflowToolbox()
{
    auto toolbox = makeToolbox();
    auto engine = std::make_shared<WorkflowEngine>();

    auto chunk_step_result = engine->stepRegistry().registerStep(
        "test.populate_chunk", std::make_shared<PopulateChunkStep>());
    EXPECT_TRUE(chunk_step_result)
        << (chunk_step_result ? "" : chunk_step_result.error().message());

    auto embed_step_result = engine->stepRegistry().registerStep(
        "builtin.chunk_embed",
        builtin::createChunkEmbedStep(std::make_shared<DeterministicEmbeddingBackend>()));
    EXPECT_TRUE(embed_step_result)
        << (embed_step_result ? "" : embed_step_result.error().message());

    const auto profile_path =
        std::filesystem::temp_directory_path() /
        ("themis_rag_ingestion_profile_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
         ".json");
    {
        std::ofstream profile(profile_path);
        profile
            << R"({"name":"default","steps":[{"name":"populate","plugin":"test.populate_chunk"},{"name":"embed","plugin":"builtin.chunk_embed","config":{"skip_when_unavailable":false}}]})";
    }
    auto profile_result = engine->loadProfile(profile_path.string());
    EXPECT_TRUE(profile_result)
        << (profile_result ? "" : profile_result.error().message());
    std::error_code remove_error;
    std::filesystem::remove(profile_path, remove_error);

    toolbox->setWorkflowEngine(std::move(engine));
    return toolbox;
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

TEST(RagIngestionBridgeHardeningFocusedTests, D4_WorkflowChunkIdsAreUniqueAcrossDocuments) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    RAGIngestionBridge bridge(makeWorkflowToolbox(), vector_writer);

    const auto first = bridge.indexDocument("first workflow document", "workflow-coll");
    const auto second = bridge.indexDocument("second workflow document", "workflow-coll");

    ASSERT_TRUE(first.ok);
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(first.status, IndexStatus::COMPLETE);
    EXPECT_EQ(second.status, IndexStatus::COMPLETE);
    EXPECT_FALSE(first.used_fallback);
    EXPECT_FALSE(second.used_fallback);
    EXPECT_NE(first.doc_id, second.doc_id);
    EXPECT_EQ(first.doc_id.size(), std::string("workflow-coll/").size() + 64u);
    EXPECT_NE(vector_writer->findByChunkId(first.doc_id + ":0"), nullptr);
    EXPECT_NE(vector_writer->findByChunkId(second.doc_id + ":0"), nullptr);
    EXPECT_EQ(vector_writer->vectorCount(), 2u);
}

TEST(RagIngestionBridgeHardeningFocusedTests, D5_ShortFallbackDocumentsUseDistinctSha256Ids) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    RAGIngestionBridge bridge(makeToolbox(), vector_writer);

    const auto first = bridge.indexDocument("short A", "fallback-coll");
    const auto second = bridge.indexDocument("short B", "fallback-coll");

    ASSERT_TRUE(first.ok);
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(first.status, IndexStatus::DEGRADED);
    EXPECT_EQ(second.status, IndexStatus::DEGRADED);
    EXPECT_TRUE(first.used_fallback);
    EXPECT_TRUE(second.used_fallback);
    EXPECT_NE(first.doc_id, second.doc_id);
    EXPECT_EQ(first.doc_id.size(), std::string("fallback-coll/").size() + 64u);
    EXPECT_NE(vector_writer->findByChunkId(first.doc_id + "#0"), nullptr);
    EXPECT_NE(vector_writer->findByChunkId(second.doc_id + "#0"), nullptr);
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

TEST(RagIngestionBridgeHardeningFocusedTests, E6_VectorWriteFailureIsPartialAndObservable) {
    auto graph_writer = std::make_shared<InMemoryGraphWriter>();
    RAGIngestionBridge bridge(
        makeWorkflowToolbox(),
        std::make_shared<FailingVectorWriter>(),
        graph_writer);

    const auto result = bridge.indexDocument("vector write failure", "failure-coll");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.status, IndexStatus::PARTIAL);
    EXPECT_FALSE(result.used_fallback);
    EXPECT_EQ(result.vector_count, 0u);
    EXPECT_GT(result.graph_entity_count, 0u);
    EXPECT_NE(result.error.find("vector write failed"), std::string::npos);
    EXPECT_GT(graph_writer->nodeCount(), 0u);
    EXPECT_GT(graph_writer->edgeCount(), 0u);
}

TEST(RagIngestionBridgeHardeningFocusedTests, E7_GraphEntityFailureIsReportedAndRelationsSkipped) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    auto graph_writer = std::make_shared<FaultInjectingGraphWriter>(true, false);
    RAGIngestionBridge bridge(makeWorkflowToolbox(), vector_writer, graph_writer);

    const auto result = bridge.indexDocument("graph entity write failure", "failure-coll");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.status, IndexStatus::PARTIAL);
    EXPECT_GT(result.vector_count, 0u);
    EXPECT_EQ(result.graph_entity_count, 0u);
    EXPECT_EQ(result.graph_relation_count, 0u);
    EXPECT_EQ(graph_writer->relationCalls(), 0u);
    EXPECT_NE(result.error.find("graph entity write failed"), std::string::npos);
    EXPECT_NE(result.error.find("graph relation write skipped"), std::string::npos);
}

TEST(RagIngestionBridgeHardeningFocusedTests, E8_GraphRelationFailureIsReported) {
    auto vector_writer = std::make_shared<InMemoryVectorWriter>();
    auto graph_writer = std::make_shared<FaultInjectingGraphWriter>(false, true);
    RAGIngestionBridge bridge(makeWorkflowToolbox(), vector_writer, graph_writer);

    const auto result = bridge.indexDocument("graph relation write failure", "failure-coll");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.status, IndexStatus::PARTIAL);
    EXPECT_GT(result.vector_count, 0u);
    EXPECT_GT(result.graph_entity_count, 0u);
    EXPECT_EQ(result.graph_relation_count, 0u);
    EXPECT_EQ(graph_writer->relationCalls(), 1u);
    EXPECT_NE(result.error.find("graph relation write failed"), std::string::npos);
}

TEST(RagIngestionBridgeHardeningFocusedTests, E9_VectorFailureWithoutOtherSinkIsFailed) {
    RAGIngestionBridge bridge(
        makeWorkflowToolbox(),
        std::make_shared<FailingVectorWriter>());

    const auto result = bridge.indexDocument("unconfirmed vector write", "failure-coll");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.status, IndexStatus::FAILED);
    EXPECT_EQ(result.vector_count, 0u);
    EXPECT_NE(result.error.find("vector write failed"), std::string::npos);
}

TEST(RagIngestionBridgeHardeningFocusedTests, E10_GraphEntityFailureWithoutOtherSinkIsFailed) {
    auto graph_writer = std::make_shared<FaultInjectingGraphWriter>(true, false);
    RAGIngestionBridge bridge(makeWorkflowToolbox(), nullptr, graph_writer);

    const auto result = bridge.indexDocument("unconfirmed graph write", "failure-coll");

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.status, IndexStatus::FAILED);
    EXPECT_EQ(result.graph_entity_count, 0u);
    EXPECT_EQ(result.graph_relation_count, 0u);
    EXPECT_EQ(graph_writer->relationCalls(), 0u);
    EXPECT_NE(result.error.find("graph entity write failed"), std::string::npos);
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
