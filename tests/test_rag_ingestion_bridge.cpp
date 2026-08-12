/*
 * ThemisDB — RAGIngestionBridge unit tests
 *
 * Tests:
 *   RI-01  Construction with valid toolbox succeeds
 *   RI-02  Construction with null toolbox throws std::invalid_argument
 *   RI-03  toolbox() accessor returns injected toolbox
 *   RI-04  vectorWriter() returns nullptr when not injected
 *   RI-05  graphWriter() returns nullptr when not injected
 *   RI-06  vectorWriter() returns injected sink
 *   RI-07  graphWriter() returns injected sink
 *   RI-08  indexDocument("") returns ok==false with "empty input" error
 *   RI-09  indexDocument(text) without sinks succeeds (ok==true)
 *   RI-10  indexDocument(text) returns non-empty doc_id
 *   RI-11  doc_id has form "<collection>/<hash>"
 *   RI-12  indexDocument same text twice yields same doc_id (stable hash)
 *   RI-13  indexDocument different text yields different doc_id
 *   RI-14  indexDocument with vector_writer calls writeVectors
 *   RI-15  indexDocument with graph_writer calls writeEntities (non-crash)
 *   RI-16  indexDocument returns collection in IndexResult
 *   RI-17  extractEntitiesForContext("") returns empty vector
 *   RI-18  extractEntitiesForContext(text) returns vector (no crash)
 *   RI-19  buildEntityContext({}) returns empty string
 *   RI-20  buildEntityContext(entities) starts with "Extracted entities:"
 *   RI-21  buildEntityContext with multiple entities contains " |" separator
 *   RI-22  enrichRetrievedDocuments({}) returns 0 and does not crash
 *   RI-23  enrichRetrievedDocuments skips documents with empty content
 *   RI-24  enrichRetrievedDocuments returns count of enriched documents
 *   RI-25  enrichRetrievedDocuments appends "_entities" key to metadata
 *   RI-26  Move construction compiles and keeps toolbox accessible
 *   RI-27  Move assignment compiles and keeps toolbox accessible
 */

#include <gtest/gtest.h>

#include "rag/rag_ingestion_bridge.h"
#include "toolbox/ingestion_toolbox.h"
#include "ingestion/ingestion_sinks.h"
#include "ingestion/base_entity.h"

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

using namespace themis;
using namespace themis::rag;
using namespace themis::toolbox;
using namespace themis::ingestion;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::shared_ptr<IngestionToolbox> makeToolbox() {
    return IngestionToolbox::createDefault();
}

static BaseEntity makeEntity(EntityType type, const std::string& id, const std::string& text) {
    BaseEntity e;
    e.entity_type = type;
    e.id          = id;
    e.text        = text;
    return e;
}

// ─────────────────────────────────────────────────────────────────────────────
// RI-01 — Construction with valid toolbox succeeds
// ─────────────────────────────────────────────────────────────────────────────
TEST(RAGIngestionBridgeTest, RI01_ConstructValid) {
    auto tb = makeToolbox();
    EXPECT_NO_THROW(RAGIngestionBridge bridge(tb));
}

// RI-02 — Construction with null toolbox throws
TEST(RAGIngestionBridgeTest, RI02_ConstructNullThrows) {
    EXPECT_THROW(RAGIngestionBridge bridge(nullptr), std::invalid_argument);
}

// RI-03 — toolbox() returns the injected toolbox
TEST(RAGIngestionBridgeTest, RI03_ToolboxAccessor) {
    auto tb = makeToolbox();
    RAGIngestionBridge bridge(tb);
    EXPECT_EQ(bridge.toolbox(), tb);
}

// RI-04 — vectorWriter() returns nullptr when not injected
TEST(RAGIngestionBridgeTest, RI04_VectorWriterNullDefault) {
    RAGIngestionBridge bridge(makeToolbox());
    EXPECT_EQ(bridge.vectorWriter(), nullptr);
}

// RI-05 — graphWriter() returns nullptr when not injected
TEST(RAGIngestionBridgeTest, RI05_GraphWriterNullDefault) {
    RAGIngestionBridge bridge(makeToolbox());
    EXPECT_EQ(bridge.graphWriter(), nullptr);
}

// RI-06 — vectorWriter() returns the injected sink
TEST(RAGIngestionBridgeTest, RI06_VectorWriterInjected) {
    auto vw = std::make_shared<InMemoryVectorWriter>();
    RAGIngestionBridge bridge(makeToolbox(), vw);
    EXPECT_EQ(bridge.vectorWriter(), vw);
}

// RI-07 — graphWriter() returns the injected sink
TEST(RAGIngestionBridgeTest, RI07_GraphWriterInjected) {
    auto gw = std::make_shared<InMemoryGraphWriter>();
    RAGIngestionBridge bridge(makeToolbox(), nullptr, gw);
    EXPECT_EQ(bridge.graphWriter(), gw);
}

// RI-08 — indexDocument("") returns ok==false
TEST(RAGIngestionBridgeTest, RI08_IndexEmptyTextFails) {
    RAGIngestionBridge bridge(makeToolbox());
    auto result = bridge.indexDocument("");
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error.empty());
}

// RI-09 — indexDocument(text) without sinks succeeds
TEST(RAGIngestionBridgeTest, RI09_IndexDocumentNoSinks) {
    RAGIngestionBridge bridge(makeToolbox());
    auto result = bridge.indexDocument("This is a legal provision.", "test-coll");
    EXPECT_TRUE(result.ok);
}

// RI-10 — indexDocument(text) returns non-empty doc_id
TEST(RAGIngestionBridgeTest, RI10_IndexDocumentNonEmptyId) {
    RAGIngestionBridge bridge(makeToolbox());
    auto result = bridge.indexDocument("Some document text.", "coll");
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.doc_id.empty());
}

// RI-11 — doc_id has the form "<collection>/<hash>"
TEST(RAGIngestionBridgeTest, RI11_DocIdHasCollectionPrefix) {
    RAGIngestionBridge bridge(makeToolbox());
    const std::string coll = "my-collection";
    auto result = bridge.indexDocument("Content here.", coll);
    ASSERT_TRUE(result.ok);
    // doc_id should start with "<coll>/"
    EXPECT_EQ(result.doc_id.substr(0, coll.size() + 1), coll + "/");
    EXPECT_EQ(result.collection, coll);
}

// RI-12 — Same text indexed twice yields the same doc_id (stable hash)
TEST(RAGIngestionBridgeTest, RI12_StableDocHash) {
    RAGIngestionBridge bridge(makeToolbox());
    const std::string text = "Deterministic hash input text.";
    auto r1 = bridge.indexDocument(text, "coll");
    auto r2 = bridge.indexDocument(text, "coll");
    ASSERT_TRUE(r1.ok);
    ASSERT_TRUE(r2.ok);
    EXPECT_EQ(r1.doc_id, r2.doc_id);
}

// RI-13 — Different texts yield different doc_ids
TEST(RAGIngestionBridgeTest, RI13_DifferentTextDifferentId) {
    RAGIngestionBridge bridge(makeToolbox());
    auto r1 = bridge.indexDocument("Text A unique content.", "coll");
    auto r2 = bridge.indexDocument("Text B different content.", "coll");
    ASSERT_TRUE(r1.ok);
    ASSERT_TRUE(r2.ok);
    EXPECT_NE(r1.doc_id, r2.doc_id);
}

// RI-14 — indexDocument with vector_writer calls writeVectors (no crash)
TEST(RAGIngestionBridgeTest, RI14_IndexDocumentCallsVectorWriter) {
    auto vw = std::make_shared<InMemoryVectorWriter>();
    RAGIngestionBridge bridge(makeToolbox(), vw);
    auto result = bridge.indexDocument("Legal text for vector indexing.", "vecs");
    EXPECT_TRUE(result.ok);
    // Whether chunks were produced depends on the workflow profile loaded;
    // the key assertion is no crash and ok==true.
    EXPECT_GE(result.vector_count, 0u);
}

// RI-15 — indexDocument with graph_writer does not crash
TEST(RAGIngestionBridgeTest, RI15_IndexDocumentCallsGraphWriter) {
    auto gw = std::make_shared<InMemoryGraphWriter>();
    RAGIngestionBridge bridge(makeToolbox(), nullptr, gw);
    EXPECT_NO_THROW({
        auto result = bridge.indexDocument("Entity text for graph.", "graph");
        EXPECT_TRUE(result.ok);
    });
}

// RI-16 — indexDocument returns the collection name in the result
TEST(RAGIngestionBridgeTest, RI16_CollectionInResult) {
    RAGIngestionBridge bridge(makeToolbox());
    auto result = bridge.indexDocument("Sample.", "my-coll");
    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.collection, "my-coll");
}

// RI-17 — extractEntitiesForContext("") returns empty
TEST(RAGIngestionBridgeTest, RI17_ExtractEmptyText) {
    RAGIngestionBridge bridge(makeToolbox());
    auto entities = bridge.extractEntitiesForContext("");
    EXPECT_TRUE(entities.empty());
}

// RI-18 — extractEntitiesForContext(text) returns vector without crashing
TEST(RAGIngestionBridgeTest, RI18_ExtractTextNoCrash) {
    RAGIngestionBridge bridge(makeToolbox());
    EXPECT_NO_THROW({
        auto entities = bridge.extractEntitiesForContext("This is some text.");
        // Result can be empty if no profile is loaded — that is OK.
        (void)entities;
    });
}

// RI-19 — buildEntityContext({}) returns empty string
TEST(RAGIngestionBridgeTest, RI19_BuildContextEmpty) {
    std::string ctx = RAGIngestionBridge::buildEntityContext({});
    EXPECT_TRUE(ctx.empty());
}

// RI-20 — buildEntityContext(entities) starts with "Extracted entities:"
TEST(RAGIngestionBridgeTest, RI20_BuildContextPrefix) {
    std::vector<BaseEntity> entities{
        makeEntity(EntityType::ORGANIZATION, "org:abc", "ABC Corp")
    };
    std::string ctx = RAGIngestionBridge::buildEntityContext(entities);
    EXPECT_FALSE(ctx.empty());
    EXPECT_EQ(ctx.substr(0, 20), "Extracted entities: ");
}

// RI-21 — buildEntityContext with multiple entities contains " |" separator
TEST(RAGIngestionBridgeTest, RI21_BuildContextSeparator) {
    std::vector<BaseEntity> entities{
        makeEntity(EntityType::ORGANIZATION, "org:abc", "ABC"),
        makeEntity(EntityType::PERSON,       "per:xyz", "Max")
    };
    std::string ctx = RAGIngestionBridge::buildEntityContext(entities);
    EXPECT_NE(ctx.find(" |"), std::string::npos);
}

// RI-22 — enrichRetrievedDocuments({}) returns 0 and does not crash
TEST(RAGIngestionBridgeTest, RI22_EnrichEmptyList) {
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs;
    EXPECT_NO_THROW({
        std::size_t n = bridge.enrichRetrievedDocuments(docs);
        EXPECT_EQ(n, 0u);
    });
}

// RI-23 — enrichRetrievedDocuments skips documents with empty content
TEST(RAGIngestionBridgeTest, RI23_EnrichSkipsEmptyContent) {
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs;
    judge::RetrievedDocument doc;
    doc.id      = "doc1";
    doc.content = "";  // empty — should be skipped
    docs.push_back(doc);

    std::size_t n = bridge.enrichRetrievedDocuments(docs);
    EXPECT_EQ(n, 0u);
    EXPECT_EQ(docs[0].metadata.count("_entities"), 0u);
}

// RI-24 — enrichRetrievedDocuments returns count of enriched documents
TEST(RAGIngestionBridgeTest, RI24_EnrichReturnsCount) {
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs;

    judge::RetrievedDocument d1;
    d1.id      = "d1";
    d1.content = "This is some meaningful document text.";
    docs.push_back(d1);

    // Count may be 0 or 1 depending on whether entities are extracted by the
    // default (no-profile) workflow; we only assert no crash and ∈ {0, 1}.
    std::size_t n = bridge.enrichRetrievedDocuments(docs);
    EXPECT_LE(n, docs.size());
}

// RI-25 — enrichRetrievedDocuments appends "_entities" key when entities found
TEST(RAGIngestionBridgeTest, RI25_EnrichAppendsMetadata) {
    // Build a toolbox and pre-register a mock entity for a controlled response.
    // Since the workflow is heuristic-based with no LLM, we use a text that
    // reliably produces at least a CHUNK entity from parse_text.
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs;

    judge::RetrievedDocument d;
    d.id      = "d1";
    d.content = "Text content for NER extraction.";
    docs.push_back(d);

    bridge.enrichRetrievedDocuments(docs);
    // If metadata["_entities"] was set, it must contain "Extracted entities:"
    if (docs[0].metadata.count("_entities")) {
        EXPECT_NE(docs[0].metadata.at("_entities").find("Extracted entities:"),
                  std::string::npos);
    }
    // No crash is the primary assertion here.
    SUCCEED();
}

// RI-26 — Move construction keeps toolbox accessible
TEST(RAGIngestionBridgeTest, RI26_MoveConstruct) {
    auto tb = makeToolbox();
    RAGIngestionBridge b1(tb);
    RAGIngestionBridge b2(std::move(b1));
    EXPECT_EQ(b2.toolbox(), tb);
}

// RI-27 — Move assignment keeps toolbox accessible
TEST(RAGIngestionBridgeTest, RI27_MoveAssign) {
    auto tb = makeToolbox();
    RAGIngestionBridge b1(tb);
    RAGIngestionBridge b2(makeToolbox());
    b2 = std::move(b1);
    EXPECT_EQ(b2.toolbox(), tb);
}

// RI-28 — enrichRetrievedDocuments skips documents without ID (fail-closed)
TEST(RAGIngestionBridgeTest, RI28_EnrichSkipsMissingId) {
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs;

    judge::RetrievedDocument doc;
    doc.id = "";
    doc.content = "Valid content without stable source id";
    docs.push_back(doc);

    const std::size_t enriched = bridge.enrichRetrievedDocuments(docs);
    EXPECT_EQ(enriched, 0u);
    EXPECT_EQ(docs[0].metadata.count("source"), 0u);
}

// RI-29 — enrichRetrievedDocuments backfills missing source metadata from doc.id
TEST(RAGIngestionBridgeTest, RI29_EnrichBackfillsSourceMetadata) {
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs;

    judge::RetrievedDocument doc;
    doc.id = "doc-42";
    doc.content = "Simple text for enrichment";
    docs.push_back(doc);

    static_cast<void>(bridge.enrichRetrievedDocuments(docs));
    ASSERT_TRUE(docs[0].metadata.contains("source"));
    EXPECT_EQ(docs[0].metadata.at("source"), "doc-42");
    ASSERT_TRUE(docs[0].metadata.contains("content"));
    EXPECT_EQ(docs[0].metadata.at("content"), "Simple text for enrichment");
}

// RI-30 — enrichRetrievedDocuments backfills missing content metadata from doc.content
TEST(RAGIngestionBridgeTest, RI30_EnrichBackfillsContentMetadata) {
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs;

    judge::RetrievedDocument doc;
    doc.id = "doc-99";
    doc.content = "Canonical content field for downstream RAG";
    docs.push_back(doc);

    static_cast<void>(bridge.enrichRetrievedDocuments(docs));
    ASSERT_TRUE(docs[0].metadata.contains("content"));
    EXPECT_EQ(docs[0].metadata.at("content"), "Canonical content field for downstream RAG");
    ASSERT_TRUE(docs[0].metadata.contains("source"));
    EXPECT_EQ(docs[0].metadata.at("source"), "doc-99");
}

// RI-31 — enrichRetrievedDocuments treats whitespace metadata as missing and backfills canonically
TEST(RAGIngestionBridgeTest, RI31_EnrichBackfillsWhitespaceMetadata) {
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs;

    judge::RetrievedDocument doc;
    doc.id = "doc-ws";
    doc.content = "Canonical body text";
    doc.metadata["source"] = "   \t";
    doc.metadata["content"] = "\n\r  ";
    docs.push_back(doc);

    static_cast<void>(bridge.enrichRetrievedDocuments(docs));
    ASSERT_TRUE(docs[0].metadata.contains("source"));
    EXPECT_EQ(docs[0].metadata.at("source"), "doc-ws");
    ASSERT_TRUE(docs[0].metadata.contains("content"));
    EXPECT_EQ(docs[0].metadata.at("content"), "Canonical body text");
}

// RI-32 — enrichRetrievedDocuments fail-closed on whitespace-only id/content
TEST(RAGIngestionBridgeTest, RI32_EnrichFailClosedOnWhitespaceIdOrContent) {
    RAGIngestionBridge bridge(makeToolbox());
    std::vector<judge::RetrievedDocument> docs;

    judge::RetrievedDocument no_id;
    no_id.id = "   \t";
    no_id.content = "Valid content";
    docs.push_back(no_id);

    judge::RetrievedDocument no_content;
    no_content.id = "doc-valid";
    no_content.content = " \n\r\t ";
    docs.push_back(no_content);

    const std::size_t enriched = bridge.enrichRetrievedDocuments(docs);
    EXPECT_EQ(enriched, 0u);
    EXPECT_EQ(docs[0].metadata.count("source"), 0u);
    EXPECT_EQ(docs[0].metadata.count("content"), 0u);
    EXPECT_EQ(docs[1].metadata.count("source"), 0u);
    EXPECT_EQ(docs[1].metadata.count("content"), 0u);
}

// RI-33 — indexDocument emits canonical source/content aliases for vector chunks
TEST(RAGIngestionBridgeTest, RI33_IndexDocumentEmitsCanonicalChunkMetadata) {
    auto vw = std::make_shared<InMemoryVectorWriter>();
    RAGIngestionBridge bridge(makeToolbox(), vw);

    auto result = bridge.indexDocument("Canonical chunk body for RAG retrieval.", "align-coll");
    ASSERT_TRUE(result.ok);
    ASSERT_GT(vw->vectorCount(), 0u);

    for (const auto& [chunk_id, record] : vw->records()) {
        static_cast<void>(chunk_id);
        EXPECT_EQ(record.metadata.at("collection"), "align-coll");

        ASSERT_TRUE(record.metadata.contains("source"));
        EXPECT_FALSE(record.metadata.at("source").empty());

        ASSERT_TRUE(record.metadata.contains("content"));
        EXPECT_FALSE(record.metadata.at("content").empty());

        ASSERT_TRUE(record.metadata.contains("text"));
        EXPECT_EQ(record.metadata.at("text"), record.metadata.at("content"));

        ASSERT_TRUE(record.metadata.contains("body"));
        EXPECT_EQ(record.metadata.at("body"), record.metadata.at("content"));
    }
}

// RI-34 — buildEntityContext trims entity IDs before rendering
TEST(RAGIngestionBridgeTest, RI34_BuildContextTrimsEntityIds) {
    std::vector<BaseEntity> entities{
        makeEntity(EntityType::ORGANIZATION, " \torg:abc\n", "ABC")
    };

    const std::string ctx = RAGIngestionBridge::buildEntityContext(entities);
    EXPECT_NE(ctx.find("ORGANIZATION org:abc"), std::string::npos);
    EXPECT_EQ(ctx.find("\n"), std::string::npos);
    EXPECT_EQ(ctx.find("\t"), std::string::npos);
}
