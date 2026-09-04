/*
 * ThemisDB — Ingestion Phase 5+6 Tests
 *
 * Tests for:
 *   DocumentStoreSinkAdapter (Phase 5 IDocumentStore wiring)  DS-01..DS-05
 *   GesetzParser                                               LD-01..LD-04
 *   TemporalExtractor                                          LD-05..LD-07
 *   BehoerdenMapper                                            LD-08..LD-09
 *   BescheidExtractor                                          LD-10..LD-11
 *   CrossDocumentLinker                                        LD-12..LD-13
 *   LegalEntityExport                                          LD-14..LD-15
 *
 * Acceptance criteria:
 *
 * DS-01  DocumentStoreSinkAdapter: writeDocument() stores entity set in IDocumentStore
 * DS-02  DocumentStoreSinkAdapter: documentCount() increments on each write
 * DS-03  DocumentStoreSinkAdapter: duplicate source_file_id triggers update instead of error
 * DS-04  DocumentStoreSinkAdapter: null store → std::invalid_argument
 * DS-05  DocumentStoreSinkAdapter: IngestionSinkBundle::writeAll() routes to adapter
 *
 * LD-01  GesetzParser: parse() extracts §-paragraphs from statute text
 * LD-02  GesetzParser: extractParagraphs() returns paragraph nodes
 * LD-03  GesetzParser: toEntities() produces canonical "law:<norm>:§<n>" IDs
 * LD-04  GesetzParser: parse() on empty text returns error
 *
 * LD-05  TemporalExtractor: extract() finds DD.MM.YYYY "in Kraft getreten am"
 * LD-06  TemporalExtractor: normaliseDate() converts "1. Januar 2024" → "2024-01-01"
 * LD-07  TemporalExtractor: merge() gives metadata precedence over text
 *
 * LD-08  BehoerdenMapper: lookupAuthority("BImSchG") returns authority
 * LD-09  BehoerdenMapper: addMapping() overrides, setFallback() called on miss
 *
 * LD-10  BescheidExtractor: extract() finds Aktenzeichen from "Az.: XYZ"
 * LD-11  BescheidExtractor: toEntity() produces "bescheid:<aktenzeichen>" ID
 *
 * LD-12  CrossDocumentLinker: linkDocuments() creates CITES edge for matching IDs
 * LD-13  CrossDocumentLinker: linkDocumentBatch() aggregates across multiple targets
 *
 * LD-14  LegalEntityExport: exportJsonLd() contains "@context" and "@graph"
 * LD-15  LegalEntityExport: exportRdf(Turtle) starts with "@base"
 */

#include "ingestion/legal_domain.h"
#include "ingestion/ingestion_sinks.h"
#include "document/document_store.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <string>

namespace themis {
namespace ingestion {
namespace test {

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string makeTempDbPath(const std::string& stem) {
    static std::atomic<std::uint64_t> counter{0};
    const auto id = counter.fetch_add(1, std::memory_order_relaxed);
    return (fs::temp_directory_path() /
            (stem + "-" + std::to_string(id))).string();
}

/// Build a minimal BaseEntitySet for sink tests.
static BaseEntitySet makeEntitySet(const std::string& file_id = "abc123") {
    BaseEntitySet es;
    es.source_file_id = file_id;
    es.quality_score  = 0.9;

    BaseEntity e;
    e.id          = "law:testg:§1";
    e.entity_type = EntityType::LEGAL_PROVISION;
    e.text        = "§ 1 Anwendungsbereich";
    e.properties["norm"] = "TestG";
    es.nodes.push_back(std::move(e));

    EntityRelation r;
    r.from_id       = "law:testg:§1";
    r.to_id         = "law:testg:§2";
    r.relation_type = RelationType::PART_OF;
    es.edges.push_back(std::move(r));

    VectorRecord vr;
    vr.chunk_id       = "chunk-1";
    vr.source_file_id = file_id;
    vr.metadata["chunk_index"] = "0";
    vr.text_snippet   = "Dieser § regelt den Anwendungsbereich.";
    es.chunks.push_back(std::move(vr));

    return es;
}

// ─────────────────────────────────────────────────────────────────────────────
// DS — DocumentStoreSinkAdapter tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(LegalDomainTests, DS01_WriteDocumentStoresEntitySet) {
    auto store = std::make_shared<document::InMemoryDocumentStore>();
    DocumentStoreSinkAdapter adapter(store);

    const auto es = makeEntitySet("file-001");
    auto result = adapter.writeDocument(es, "legal");
    ASSERT_TRUE(result.has_value()) << result.error().message();

    const std::string doc_id = result.value();
    EXPECT_FALSE(doc_id.empty());

    // Document should be retrievable from the store
    auto fetched = store->get("legal", doc_id);
    ASSERT_TRUE(fetched.has_value()) << fetched.error().message();
    ASSERT_TRUE(fetched.value().has_value());
    EXPECT_EQ(fetched.value()->collection_id, "legal");
}

TEST(LegalDomainTests, DS02_DocumentCountIncrements) {
    auto store = std::make_shared<document::InMemoryDocumentStore>();
    DocumentStoreSinkAdapter adapter(store);

    EXPECT_EQ(adapter.documentCount(), 0u);
    adapter.writeDocument(makeEntitySet("f1"), "col");
    EXPECT_EQ(adapter.documentCount(), 1u);
    adapter.writeDocument(makeEntitySet("f2"), "col");
    EXPECT_EQ(adapter.documentCount(), 2u);
}

TEST(LegalDomainTests, DS03_DuplicateSourceIdTriggersUpdate) {
    auto store = std::make_shared<document::InMemoryDocumentStore>();
    DocumentStoreSinkAdapter adapter(store);

    auto es = makeEntitySet("dup-file");
    auto r1 = adapter.writeDocument(es, "col");
    ASSERT_TRUE(r1.has_value());
    // Write again with same source_file_id
    auto r2 = adapter.writeDocument(es, "col");
    // Should succeed (update path)
    EXPECT_TRUE(r2.has_value()) << (r2.has_value() ? "" : r2.error().message());
}

TEST(LegalDomainTests, DS04_NullStoreThrows) {
    EXPECT_THROW(DocumentStoreSinkAdapter{nullptr}, std::invalid_argument);
}

TEST(LegalDomainTests, DS05_SinkBundleWriteAllRoutesToAdapter) {
    auto store = std::make_shared<document::InMemoryDocumentStore>();
    auto adapter = std::make_shared<DocumentStoreSinkAdapter>(store);

    IngestionSinkBundle bundle;
    bundle.doc = adapter;

    const auto es = makeEntitySet("bundle-file");
    auto r = bundle.writeAll(es, "bundle-col");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_EQ(adapter->documentCount(), 1u);
}

TEST(LegalDomainTests, GS01_GraphStoreSinkAdapterPersistsNodesAndEdges) {
    const auto db_path = makeTempDbPath("graph-sink");
    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_blobdb = false;
    auto db = std::make_shared<RocksDBWrapper>(cfg);
    ASSERT_TRUE(db->open());

    auto graph_index = std::make_shared<GraphIndexManager>(*db);
    GraphStoreSinkAdapter adapter(db, graph_index);

    const auto es = makeEntitySet("graph-file");
    auto node_result = adapter.writeEntities(es.nodes);
    ASSERT_TRUE(node_result.has_value()) << node_result.error().message();
    auto edge_result = adapter.writeRelations(es.edges);
    ASSERT_TRUE(edge_result.has_value()) << edge_result.error().message();

    EXPECT_EQ(adapter.nodeCount(), 1u);
    EXPECT_EQ(adapter.edgeCount(), 1u);
    EXPECT_TRUE(db->get("ingestion:graph:node:" + es.nodes.front().id).has_value());

    db->close();
    std::error_code ec;
    fs::remove_all(db_path, ec);
    EXPECT_FALSE(ec) << ec.message();
}

TEST(LegalDomainTests, VS01_VectorIndexSinkAdapterPersistsVectors) {
    const auto db_path = makeTempDbPath("vector-sink");
    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_blobdb = false;
    auto db = std::make_shared<RocksDBWrapper>(cfg);
    ASSERT_TRUE(db->open());

    auto vector_index = std::make_shared<VectorIndexManager>(*db);
    VectorIndexSinkAdapter adapter(vector_index, "ingestion_chunks", 1);

    auto es = makeEntitySet("vector-file");
    es.chunks.front().embedding = {0.1f};
    auto result = adapter.writeVectors(es.chunks);
    ASSERT_TRUE(result.has_value()) << result.error().message();

    EXPECT_EQ(adapter.vectorCount(), 1u);
    ASSERT_NE(adapter.findByChunkId(es.chunks.front().chunk_id), nullptr);
    EXPECT_EQ(adapter.findByChunkId(es.chunks.front().chunk_id)->source_file_id,
              es.chunks.front().source_file_id);

    db->close();
    std::error_code ec;
    fs::remove_all(db_path, ec);
    EXPECT_FALSE(ec) << ec.message();
}

// ─────────────────────────────────────────────────────────────────────────────
// LD-01..LD-04 — GesetzParser
// ─────────────────────────────────────────────────────────────────────────────

static const std::string SAMPLE_LAW_TEXT =
    "Testgesetz (TestG)\n\n"
    "§ 1 Anwendungsbereich\n"
    "Dieses Gesetz regelt die Anforderungen.\n"
    "(1) Die Anforderungen gelten für alle Betriebe.\n"
    "(2) Ausnahmen sind zulässig.\n\n"
    "§ 2 Begriffsbestimmungen\n"
    "Im Sinne dieses Gesetzes bedeutet:\n"
    "(1) Betrieb: eine ortsfeste Einrichtung.\n\n"
    "§ 3a Übergangsregelungen\n"
    "Bestehende Betriebe haben eine Übergangsfrist von zwei Jahren.\n";

TEST(LegalDomainTests, LD01_GesetzParserExtractsParagraphs) {
    GesetzParser parser;
    auto result = parser.parse(SAMPLE_LAW_TEXT, "TestG");
    ASSERT_TRUE(result.has_value()) << result.error().message();

    const auto& hier = result.value();
    EXPECT_EQ(hier.norm_abbreviation, "TestG");

    // Gather all paragraph nodes
    int para_count = 0;
    hier.root.traverse([&](const GesetzNode& n, int) {
        if (n.type == GesetzNodeType::PARAGRAPH) {
          ++para_count;
        }
    });
    EXPECT_GE(para_count, 3); // §1, §2, §3a
}

TEST(LegalDomainTests, LD02_ExtractParagraphsReturnsParagraphNodes) {
    GesetzParser parser;
    auto paras = parser.extractParagraphs(SAMPLE_LAW_TEXT);
    ASSERT_GE(paras.size(), 3u);
    for (const auto& p : paras) {
        EXPECT_EQ(p.type, GesetzNodeType::PARAGRAPH);
        EXPECT_FALSE(p.number.empty());
    }
}

TEST(LegalDomainTests, LD03_ToEntitiesProducesCanonicalIds) {
    GesetzParser parser;
    auto result = parser.parse(SAMPLE_LAW_TEXT, "TestG");
    ASSERT_TRUE(result.has_value());

    auto entities = parser.toEntities(result.value());
    ASSERT_FALSE(entities.empty());

    bool found_para1 = false;
    for (const auto& e : entities) {
        if (e.id == "law:testg:§1") { found_para1 = true; break; }
    }
    EXPECT_TRUE(found_para1) << "Expected canonical ID law:testg:§1";
}

TEST(LegalDomainTests, LD04_ParseEmptyTextReturnsError) {
    GesetzParser parser;
    auto result = parser.parse("", "TestG");
    EXPECT_FALSE(result.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// LD-05..LD-07 — TemporalExtractor
// ─────────────────────────────────────────────────────────────────────────────

TEST(LegalDomainTests, LD05_ExtractFindsDMYDate) {
    TemporalExtractor te;
    const std::string text =
        "Das Gesetz ist in Kraft getreten am 01.03.2022 und gilt seither.";
    auto tv = te.extract(text);
    ASSERT_TRUE(tv.effective_from.has_value());
    EXPECT_EQ(tv.effective_from.value(), "2022-03-01");
}

TEST(LegalDomainTests, LD06_NormaliseDateGermanMonthName) {
    EXPECT_EQ(TemporalExtractor::normaliseDate("1. Januar 2024"), "2024-01-01");
    EXPECT_EQ(TemporalExtractor::normaliseDate("15. März 2023"),  "2023-03-15");
    EXPECT_EQ(TemporalExtractor::normaliseDate("2022-06-30"),     "2022-06-30");
}

TEST(LegalDomainTests, LD07_MergeGivesMetadataPrecedence) {
    TemporalExtractor te;
    TemporalValidity tv;
    tv.effective_from = "2020-01-01";

    nlohmann::json meta = {{"effective_from", "01.06.2021"}};
    auto merged = te.merge(tv, meta);
    ASSERT_TRUE(merged.effective_from.has_value());
    EXPECT_EQ(merged.effective_from.value(), "2021-06-01");
}

// ─────────────────────────────────────────────────────────────────────────────
// LD-08..LD-09 — BehoerdenMapper
// ─────────────────────────────────────────────────────────────────────────────

TEST(LegalDomainTests, LD08_LookupBuiltinAuthority) {
    BehoerdenMapper mapper;
    auto auth = mapper.lookupAuthority("BImSchG");
    ASSERT_TRUE(auth.has_value());
    EXPECT_FALSE(auth.value().empty());
}

TEST(LegalDomainTests, LD09_AddMappingAndFallback) {
    BehoerdenMapper mapper;
    mapper.addMapping("XYZ-Gesetz", "Testbehörde XYZ");
    EXPECT_EQ(mapper.lookupAuthority("XYZ-Gesetz").value(), "Testbehörde XYZ");

    // Unknown norm without fallback → nullopt
    EXPECT_FALSE(mapper.lookupAuthority("UNKNOWN-NORM-999").has_value());

    // With fallback
    mapper.setFallback([](const std::string& norm) -> std::string {
        if (norm == "UNKNOWN-NORM-999") {
          return "FallbackBehörde";
        }
        return "";
    });
    auto fb = mapper.lookupAuthority("UNKNOWN-NORM-999");
    ASSERT_TRUE(fb.has_value());
    EXPECT_EQ(fb.value(), "FallbackBehörde");
}

// ─────────────────────────────────────────────────────────────────────────────
// LD-10..LD-11 — BescheidExtractor
// ─────────────────────────────────────────────────────────────────────────────

static const std::string BESCHEID_TEXT =
    "Bescheid\n"
    "Az.: 2024-UMW-042\n"
    "Antragsteller: Muster GmbH\n"
    "Bescheid vom 15.04.2024\n\n"
    "Auflagen:\n"
    "1. Die Emission ist auf 50 mg/m³ zu begrenzen.\n"
    "2. Betriebszeiten sind von 6:00 bis 22:00 Uhr festgelegt.\n";

TEST(LegalDomainTests, LD10_BescheidExtractorFindsFields) {
    BescheidExtractor extractor;
    auto be = extractor.extract(BESCHEID_TEXT);
    EXPECT_EQ(be.aktenzeichen,  "2024-UMW-042");
    EXPECT_FALSE(be.antragsteller.empty());
    EXPECT_FALSE(be.bescheid_datum.empty());
    EXPECT_GE(be.auflagen.size(), 2u);
}

TEST(LegalDomainTests, LD11_BescheidToEntityHasCanonicalId) {
    BescheidExtractor extractor;
    auto be = extractor.extract(BESCHEID_TEXT);
    auto entity = extractor.toEntity(be, "bescheid-doc-1.pdf");
    EXPECT_EQ(entity.id, "bescheid:2024-UMW-042");
    EXPECT_EQ(entity.entity_type, EntityType::LEGAL_DECISION);
    EXPECT_EQ(entity.properties.at("aktenzeichen"), "2024-UMW-042");
}

// ─────────────────────────────────────────────────────────────────────────────
// LD-12..LD-13 — CrossDocumentLinker
// ─────────────────────────────────────────────────────────────────────────────

static ExtractionContext makeCtx(
    const std::string& path,
    const std::vector<std::pair<std::string, EntityType>>& entities_id_type)
{
    ExtractionContext ctx;
    ctx.manifest.original_path = path;
    for (const auto& [id, type] : entities_id_type) {
        BaseEntity e;
        e.id          = id;
        e.entity_type = type;
        e.text        = id;
        ctx.entities.push_back(e);
    }
    return ctx;
}

TEST(LegalDomainTests, LD12_LinkDocumentsCreatesCitesEdge) {
    CrossDocumentLinker linker;
    auto ctx1 = makeCtx("/docs/a.txt",
                        {{"law:testg:§1", EntityType::LEGAL_NORM_REFERENCE}});
    auto ctx2 = makeCtx("/docs/b.txt",
                        {{"law:testg:§1", EntityType::LEGAL_PROVISION},
                         {"law:testg:§2", EntityType::LEGAL_PROVISION}});

    auto edges = linker.linkDocuments(ctx1, ctx2);
    ASSERT_GE(edges.size(), 1u);
    EXPECT_EQ(edges[0].relation_type, RelationType::CITES);
    EXPECT_EQ(edges[0].from_id, "law:testg:§1");
    EXPECT_EQ(edges[0].to_id,   "law:testg:§1");
}

TEST(LegalDomainTests, LD13_LinkDocumentBatchAggregates) {
    CrossDocumentLinker linker;
    auto src  = makeCtx("/s.txt", {{"law:a:§1", EntityType::LEGAL_NORM_REFERENCE}});
    auto tgt1 = makeCtx("/t1.txt", {{"law:a:§1", EntityType::LEGAL_PROVISION}});
    auto tgt2 = makeCtx("/t2.txt", {{"law:b:§3", EntityType::LEGAL_PROVISION}});

    auto edges = linker.linkDocumentBatch(src, {tgt1, tgt2});
    // Should find at least the tgt1 CITES match
    bool found = false;
    for (const auto& e : edges) {
        if (e.from_id == "law:a:§1" && e.to_id == "law:a:§1") {
          found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ─────────────────────────────────────────────────────────────────────────────
// LD-14..LD-15 — LegalEntityExport
// ─────────────────────────────────────────────────────────────────────────────

static BaseEntitySet makeExportSet() {
    BaseEntitySet es;
    es.source_file_id = "export-test";

    BaseEntity e1;
    e1.id            = "law:testg:§1";
    e1.entity_type   = EntityType::LEGAL_PROVISION;
    e1.text          = "§ 1 Anwendungsbereich";
    e1.source_file_id = "TestG";
    e1.properties["norm"] = "TestG";
    es.nodes.push_back(e1);

    BaseEntity e2;
    e2.id            = "law:testg:§2";
    e2.entity_type   = EntityType::LEGAL_PROVISION;
    e2.text          = "§ 2 Begriffe";
    e2.source_file_id = "TestG";
    es.nodes.push_back(e2);

    EntityRelation r;
    r.from_id       = "law:testg:§1";
    r.to_id         = "law:testg:§2";
    r.relation_type = RelationType::CITES;
    es.edges.push_back(r);

    return es;
}

TEST(LegalDomainTests, LD14_ExportJsonLdHasContextAndGraph) {
    LegalEntityExport exporter;
    const auto es  = makeExportSet();
    const auto doc = exporter.exportJsonLd(es);

    EXPECT_TRUE(doc.contains("@context"));
    EXPECT_TRUE(doc.contains("@graph"));
    ASSERT_TRUE(doc["@graph"].is_array());
    EXPECT_GE(doc["@graph"].size(), 2u); // 2 nodes + 1 relation
}

TEST(LegalDomainTests, LD15_ExportRdfTurtleStartsWithBase) {
    LegalEntityExport exporter;
    const auto es  = makeExportSet();
    const auto ttl = exporter.exportRdf(es, RdfFormat::TURTLE);

    EXPECT_FALSE(ttl.empty());
    EXPECT_NE(ttl.find("@base"), std::string::npos);
    EXPECT_NE(ttl.find("@prefix"), std::string::npos);
    // Check entity IRI appears
    EXPECT_NE(ttl.find("law"), std::string::npos);
}

} // namespace test
} // namespace ingestion
} // namespace themis
