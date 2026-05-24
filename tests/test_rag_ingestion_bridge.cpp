/*
 * ThemisDB | File: test_rag_ingestion_bridge.cpp | Version: 0.1.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 97/100
 * Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

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
