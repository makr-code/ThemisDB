/*
 * ThemisDB — Ingestion Phase 4 Tests: EntityNormalizer + RelationBuilder + Sinks
 *
 * Tests for:
 *   EntityNormalizer   (canonical IDs, dedup)               EN-01..EN-08
 *   RelationBuilder    (CITES, PART_OF, CO_OCCURS, ISSUED_BY) RB-01..RB-06
 *   InMemoryGraphWriter                                      GW-01..GW-05
 *   InMemoryVectorWriter                                     VW-01..VW-05
 *   InMemoryDocWriter                                        DW-01..DW-03
 *   IngestionSinkBundle                                      SB-01..SB-03
 *
 * Acceptance criteria:
 *
 * EN-01  EntityNormalizer: CHUNK entity gets "chunk:<file_id>:<seq>" ID
 * EN-02  EntityNormalizer: LEGAL_PROVISION with §-text gets "law:<abbr>:§<n>" ID
 * EN-03  EntityNormalizer: PERSON entity gets "person:<hash>" ID
 * EN-04  EntityNormalizer: dedup_strategy=canonical_id removes duplicates by ID
 * EN-05  EntityNormalizer: dedup keeps entity with higher confidence
 * EN-06  EntityNormalizer: min_confidence filter removes low-confidence entities
 * EN-07  EntityNormalizer: source_file_id backfilled from manifest when empty
 * EN-08  EntityNormalizer: ORGANIZATION gets "org:<hash>" ID
 *
 * RB-01  RelationBuilder: entity with norm_ref_target → CITES edge
 * RB-02  RelationBuilder: entity with relation_hint=amends → AMENDS edge
 * RB-03  RelationBuilder: entity with parent_section → PART_OF edge (target must exist)
 * RB-04  RelationBuilder: build_co_occurrence=true → CO_OCCURS edges within same section
 * RB-05  RelationBuilder: no duplicate edges (idempotent on second call)
 * RB-06  RelationBuilder: ISSUED_BY edge when LEGAL_DECISION has authority property
 *
 * GW-01  InMemoryGraphWriter: writeEntities() stores nodes by ID
 * GW-02  InMemoryGraphWriter: writeEntities() merges properties on duplicate ID
 * GW-03  InMemoryGraphWriter: writeRelations() stores edges
 * GW-04  InMemoryGraphWriter: nodeCount() and edgeCount() correct
 * GW-05  InMemoryGraphWriter: write(BaseEntitySet) combines both calls
 *
 * VW-01  InMemoryVectorWriter: writeVectors() stores by chunk_id
 * VW-02  InMemoryVectorWriter: overwrite on duplicate chunk_id
 * VW-03  InMemoryVectorWriter: vectorCount() correct
 * VW-04  InMemoryVectorWriter: findByChunkId() returns nullptr for unknown ID
 * VW-05  InMemoryVectorWriter: findByChunkId() returns record for known ID
 *
 * DW-01  InMemoryDocWriter: writeDocument() returns non-empty doc ID
 * DW-02  InMemoryDocWriter: documentCount() increments
 * DW-03  InMemoryDocWriter: stored document contains source_file_id
 *
 * SB-01  IngestionSinkBundle: writeAll() calls all three sinks
 * SB-02  IngestionSinkBundle: nullptr sink is skipped (no crash)
 * SB-03  IngestionSinkBundle: empty chunks → no vector write called
 */

#include <gtest/gtest.h>

#include "ingestion/base_entity.h"
#include "ingestion/extraction_context.h"
#include "ingestion/file_manifest.h"
#include "ingestion/entity_assembler.h"
#include "ingestion/ingestion_sinks.h"

#include <memory>
#include <string>
#include <vector>

using namespace themis::ingestion;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static BaseEntity makeEntity(EntityType et, const std::string& text,
                               double conf = 1.0,
                               const std::string& id = "") {
    BaseEntity e;
    e.entity_type = et;
    e.text        = text;
    e.id          = id;
    e.provenance.confidence = conf;
    return e;
}

static ExtractionContext makeCtx(const std::string& file_id = "sha256:test") {
    ExtractionContext ctx;
    ctx.manifest.file_id = file_id;
    return ctx;
}

static BaseEntitySet makeEntitySet(const std::string& file_id = "sha256:test") {
    BaseEntitySet es;
    es.source_file_id = file_id;
    es.quality_score  = 0.8;
    return es;
}

// ─────────────────────────────────────────────────────────────────────────────
// EN — EntityNormalizer
// ─────────────────────────────────────────────────────────────────────────────

TEST(EntityNormalizer, EN01_ChunkEntity_CanonicalId) {
    EntityNormalizer norm;
    auto ctx = makeCtx("file123");
    auto ent = makeEntity(EntityType::CHUNK, "This is a chunk of text.");
    ctx.entities.push_back(std::move(ent));

    norm.normalize(ctx);

    ASSERT_EQ(ctx.entities.size(), 1u);
    EXPECT_EQ(ctx.entities[0].id, "chunk:file123:0");
}

TEST(EntityNormalizer, EN02_LegalProvision_CanonicalId) {
    EntityNormalizerConfig cfg;
    cfg.known_law_abbreviations = {"BImSchG"};
    EntityNormalizer norm(cfg);

    auto ctx = makeCtx("sha256:abc");
    auto ent = makeEntity(EntityType::LEGAL_PROVISION, "§ 4 Abs. 1 BImSchG");
    ctx.entities.push_back(std::move(ent));

    norm.normalize(ctx);
    ASSERT_EQ(ctx.entities.size(), 1u);
    const auto& id = ctx.entities[0].id;
    // Should contain "law:" prefix and "§4"
    EXPECT_NE(id.find("law:"), std::string::npos) << "ID: " << id;
    EXPECT_NE(id.find("4"), std::string::npos) << "ID: " << id;
}

TEST(EntityNormalizer, EN03_Person_CanonicalId) {
    EntityNormalizer norm;
    auto ctx = makeCtx();
    ctx.entities.push_back(makeEntity(EntityType::PERSON, "Max Mustermann"));

    norm.normalize(ctx);
    ASSERT_EQ(ctx.entities.size(), 1u);
    EXPECT_EQ(ctx.entities[0].id.substr(0, 7), "person:");
}

TEST(EntityNormalizer, EN04_Dedup_RemovesDuplicateById) {
    EntityNormalizerConfig cfg;
    cfg.dedup_strategy = "canonical_id";
    EntityNormalizer norm(cfg);

    auto ctx = makeCtx();
    auto e1 = makeEntity(EntityType::PERSON, "Max Mustermann", 0.9, "person:abc");
    auto e2 = makeEntity(EntityType::PERSON, "Max Mustermann", 0.7, "person:abc");
    ctx.entities = {e1, e2};

    norm.normalize(ctx);
    EXPECT_EQ(ctx.entities.size(), 1u);
}

TEST(EntityNormalizer, EN05_Dedup_KeepsHigherConfidence) {
    EntityNormalizerConfig cfg;
    cfg.dedup_strategy = "canonical_id";
    EntityNormalizer norm(cfg);

    auto ctx = makeCtx();
    auto e1 = makeEntity(EntityType::PERSON, "Anna Bauer", 0.6, "person:xyz");
    auto e2 = makeEntity(EntityType::PERSON, "Anna Bauer", 0.95, "person:xyz");
    ctx.entities = {e1, e2};

    norm.normalize(ctx);
    ASSERT_EQ(ctx.entities.size(), 1u);
    EXPECT_DOUBLE_EQ(ctx.entities[0].provenance.confidence, 0.95);
}

TEST(EntityNormalizer, EN06_MinConfidenceFilter) {
    EntityNormalizerConfig cfg;
    cfg.min_confidence = 0.5;
    EntityNormalizer norm(cfg);

    auto ctx = makeCtx();
    ctx.entities.push_back(makeEntity(EntityType::DATE, "2024-01-01", 0.3));
    ctx.entities.push_back(makeEntity(EntityType::DATE, "2025-06-15", 0.8));

    norm.normalize(ctx);
    ASSERT_EQ(ctx.entities.size(), 1u);
    EXPECT_EQ(ctx.entities[0].text, "2025-06-15");
}

TEST(EntityNormalizer, EN07_SourceFileIdBackfilled) {
    EntityNormalizer norm;
    auto ctx = makeCtx("sha256:myfile");
    auto ent = makeEntity(EntityType::ORGANIZATION, "Umweltbundesamt");
    ent.source_file_id = ""; // empty
    ctx.entities.push_back(std::move(ent));

    norm.normalize(ctx);
    ASSERT_EQ(ctx.entities.size(), 1u);
    EXPECT_EQ(ctx.entities[0].source_file_id, "sha256:myfile");
}

TEST(EntityNormalizer, EN08_Organization_CanonicalId) {
    EntityNormalizer norm;
    auto ctx = makeCtx();
    ctx.entities.push_back(makeEntity(EntityType::ORGANIZATION, "Bundesministerium"));

    norm.normalize(ctx);
    ASSERT_EQ(ctx.entities.size(), 1u);
    EXPECT_EQ(ctx.entities[0].id.substr(0, 4), "org:");
}

// ─────────────────────────────────────────────────────────────────────────────
// RB — RelationBuilder
// ─────────────────────────────────────────────────────────────────────────────

TEST(RelationBuilder, RB01_NormRefTarget_CITESEdge) {
    RelationBuilderConfig cfg;
    cfg.relation_types = {"CITES"};
    RelationBuilder builder(cfg);

    auto ctx = makeCtx();
    auto e = makeEntity(EntityType::LEGAL_PROVISION, "§ 4 gilt gemäß § 5.");
    e.id = "law:BImSchG:§4";
    e.properties["norm_ref_target"] = "law:BImSchG:§5";
    ctx.entities.push_back(std::move(e));

    builder.build(ctx);
    ASSERT_EQ(ctx.relations.size(), 1u);
    EXPECT_EQ(ctx.relations[0].relation_type, RelationType::CITES);
    EXPECT_EQ(ctx.relations[0].from_id, "law:BImSchG:§4");
    EXPECT_EQ(ctx.relations[0].to_id,   "law:BImSchG:§5");
}

TEST(RelationBuilder, RB02_RelationHintAmends_AMENDSEdge) {
    RelationBuilder builder;
    auto ctx = makeCtx();
    auto e = makeEntity(EntityType::LEGAL_PROVISION, "§ 4 neu (amends §4 alt)");
    e.id = "law:X:§4:new";
    e.properties["relation_hint"]   = "amends";
    e.properties["relation_target"] = "law:X:§4:old";
    ctx.entities.push_back(std::move(e));

    builder.build(ctx);
    bool found = false;
    for (const auto& r : ctx.relations) {
        if (r.relation_type == RelationType::AMENDS) { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(RelationBuilder, RB03_ParentSection_PARTOFEdge) {
    RelationBuilderConfig cfg;
    cfg.relation_types = {"PART_OF"};
    RelationBuilder builder(cfg);

    auto ctx = makeCtx();
    BaseEntity parent = makeEntity(EntityType::LEGAL_PROVISION, "§ 4");
    parent.id = "law:ABC:§4";
    BaseEntity child = makeEntity(EntityType::LEGAL_PROVISION, "§ 4 Abs. 1");
    child.id = "law:ABC:§4:Abs1";
    child.properties["parent_section"] = "law:ABC:§4";

    ctx.entities = {parent, child};
    builder.build(ctx);

    bool found = false;
    for (const auto& r : ctx.relations) {
        if (r.relation_type == RelationType::PART_OF
            && r.from_id == "law:ABC:§4:Abs1"
            && r.to_id   == "law:ABC:§4") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST(RelationBuilder, RB04_CoOccurrence_SameSection_Edges) {
    RelationBuilderConfig cfg;
    cfg.build_co_occurrence = true;
    RelationBuilder builder(cfg);

    auto ctx = makeCtx();
    auto e1 = makeEntity(EntityType::PERSON, "Max");
    e1.id = "person:001"; e1.properties["section_ref"] = "§ 3";
    auto e2 = makeEntity(EntityType::ORGANIZATION, "BMU");
    e2.id = "org:002"; e2.properties["section_ref"] = "§ 3";
    auto e3 = makeEntity(EntityType::DATE, "2024");
    e3.id = "date:003"; e3.properties["section_ref"] = "§ 5"; // different section

    ctx.entities = {e1, e2, e3};
    builder.build(ctx);

    bool found_cooccur = false;
    for (const auto& r : ctx.relations) {
        if (r.relation_type == RelationType::CO_OCCURS
            && ((r.from_id == "person:001" && r.to_id == "org:002")
                || (r.from_id == "org:002" && r.to_id == "person:001"))) {
            found_cooccur = true;
        }
    }
    EXPECT_TRUE(found_cooccur);

    // e3 is in a different section → no CO_OCCURS with e1 or e2
    for (const auto& r : ctx.relations) {
        if (r.relation_type == RelationType::CO_OCCURS) {
            EXPECT_NE(r.from_id, "date:003");
            EXPECT_NE(r.to_id,   "date:003");
        }
    }
}

TEST(RelationBuilder, RB05_Idempotent_NoDuplicateEdges) {
    RelationBuilder builder;
    auto ctx = makeCtx();
    auto e = makeEntity(EntityType::LEGAL_PROVISION, "§ 4");
    e.id = "law:X:§4";
    e.properties["norm_ref_target"] = "law:X:§5";
    ctx.entities.push_back(std::move(e));

    builder.build(ctx);
    const std::size_t count_after_first = ctx.relations.size();
    builder.build(ctx); // second call
    EXPECT_EQ(ctx.relations.size(), count_after_first) << "No duplicate edges expected";
}

TEST(RelationBuilder, RB06_IssuedBy_LegalDecision_Authority) {
    RelationBuilderConfig cfg;
    cfg.relation_types = {"ISSUED_BY"};
    RelationBuilder builder(cfg);

    auto ctx = makeCtx();
    BaseEntity auth = makeEntity(EntityType::LEGAL_AUTHORITY, "Umweltbundesamt");
    auth.id = "org:uba";
    BaseEntity decision = makeEntity(EntityType::LEGAL_DECISION, "Bescheid 42/24");
    decision.id = "bescheid:42_24";
    decision.properties["authority"] = "Umweltbundesamt";

    ctx.entities = {auth, decision};
    builder.build(ctx);

    bool found = false;
    for (const auto& r : ctx.relations) {
        if (r.relation_type == RelationType::ISSUED_BY
            && r.from_id == "bescheid:42_24"
            && r.to_id   == "org:uba") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ─────────────────────────────────────────────────────────────────────────────
// GW — InMemoryGraphWriter
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryGraphWriter, GW01_WriteEntities_StoredById) {
    InMemoryGraphWriter gw;
    auto e = makeEntity(EntityType::PERSON, "Hans", 1.0, "person:001");
    gw.writeEntities({e});
    EXPECT_EQ(gw.nodeCount(), 1u);
    EXPECT_NE(gw.nodes().find("person:001"), gw.nodes().end());
}

TEST(InMemoryGraphWriter, GW02_WriteEntities_MergesPropertiesOnDuplicate) {
    InMemoryGraphWriter gw;
    auto e1 = makeEntity(EntityType::PERSON, "Hans", 0.9, "person:001");
    e1.properties["role"] = "Antragsteller";
    auto e2 = makeEntity(EntityType::PERSON, "Hans", 0.7, "person:001");
    e2.properties["title"] = "Dr.";

    gw.writeEntities({e1, e2});
    EXPECT_EQ(gw.nodeCount(), 1u);
    const auto& n = gw.nodes().at("person:001");
    EXPECT_EQ(n.properties.at("role"), "Antragsteller");
    EXPECT_EQ(n.properties.at("title"), "Dr.");
}

TEST(InMemoryGraphWriter, GW03_WriteRelations_StoredEdges) {
    InMemoryGraphWriter gw;
    EntityRelation r;
    r.from_id = "law:X:§4"; r.to_id = "law:X:§5"; r.relation_type = RelationType::CITES;
    gw.writeRelations({r});
    EXPECT_EQ(gw.edgeCount(), 1u);
}

TEST(InMemoryGraphWriter, GW04_NodeAndEdgeCount) {
    InMemoryGraphWriter gw;
    gw.writeEntities({makeEntity(EntityType::DATE, "2024", 1.0, "date:1"),
                      makeEntity(EntityType::DATE, "2025", 1.0, "date:2")});
    EntityRelation r;
    r.from_id = "date:1"; r.to_id = "date:2";
    r.relation_type = RelationType::CO_OCCURS;
    gw.writeRelations({r});
    EXPECT_EQ(gw.nodeCount(), 2u);
    EXPECT_EQ(gw.edgeCount(), 1u);
}

TEST(InMemoryGraphWriter, GW05_WriteEntitySet_CallsBoth) {
    auto gw = std::make_shared<InMemoryGraphWriter>();
    auto es = makeEntitySet("f1");
    es.nodes.push_back(makeEntity(EntityType::PERSON, "A", 1.0, "person:a"));
    EntityRelation r;
    r.from_id = "person:a"; r.to_id = "person:b"; r.relation_type = RelationType::CO_OCCURS;
    es.edges.push_back(r);

    auto result = gw->write(es);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(gw->nodeCount(), 1u);
    EXPECT_EQ(gw->edgeCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// VW — InMemoryVectorWriter
// ─────────────────────────────────────────────────────────────────────────────

static VectorRecord makeVecRec(const std::string& chunk_id,
                                const std::string& file_id = "f1") {
    VectorRecord v;
    v.chunk_id      = chunk_id;
    v.source_file_id = file_id;
    v.text_snippet  = "snippet";
    v.embedding     = {0.1f, 0.2f, 0.3f};
    return v;
}

TEST(InMemoryVectorWriter, VW01_WriteVectors_StoredByChunkId) {
    InMemoryVectorWriter vw;
    vw.writeVectors({makeVecRec("chunk:1")});
    EXPECT_EQ(vw.vectorCount(), 1u);
}

TEST(InMemoryVectorWriter, VW02_Overwrite_DuplicateChunkId) {
    InMemoryVectorWriter vw;
    vw.writeVectors({makeVecRec("chunk:1")});
    auto v2 = makeVecRec("chunk:1");
    v2.text_snippet = "updated";
    vw.writeVectors({v2});
    EXPECT_EQ(vw.vectorCount(), 1u);
    EXPECT_EQ(vw.findByChunkId("chunk:1")->text_snippet, "updated");
}

TEST(InMemoryVectorWriter, VW03_VectorCount) {
    InMemoryVectorWriter vw;
    vw.writeVectors({makeVecRec("a"), makeVecRec("b"), makeVecRec("c")});
    EXPECT_EQ(vw.vectorCount(), 3u);
}

TEST(InMemoryVectorWriter, VW04_FindByChunkId_NotFound_ReturnsNullptr) {
    InMemoryVectorWriter vw;
    EXPECT_EQ(vw.findByChunkId("nonexistent"), nullptr);
}

TEST(InMemoryVectorWriter, VW05_FindByChunkId_Found_ReturnsRecord) {
    InMemoryVectorWriter vw;
    vw.writeVectors({makeVecRec("chunk:42")});
    const auto* rec = vw.findByChunkId("chunk:42");
    ASSERT_NE(rec, nullptr);
    EXPECT_EQ(rec->chunk_id, "chunk:42");
}

// ─────────────────────────────────────────────────────────────────────────────
// DW — InMemoryDocWriter
// ─────────────────────────────────────────────────────────────────────────────

TEST(InMemoryDocWriter, DW01_WriteDocument_ReturnsNonEmptyId) {
    InMemoryDocWriter dw;
    auto es = makeEntitySet("sha256:abc");
    auto result = dw.writeDocument(es, "legal");
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result->empty());
}

TEST(InMemoryDocWriter, DW02_DocumentCount_Increments) {
    InMemoryDocWriter dw;
    auto es = makeEntitySet("sha256:abc");
    dw.writeDocument(es, "ingested");
    dw.writeDocument(es, "ingested");
    EXPECT_EQ(dw.documentCount(), 2u);
}

TEST(InMemoryDocWriter, DW03_StoredDocument_ContainsSourceFileId) {
    InMemoryDocWriter dw;
    auto es = makeEntitySet("sha256:myfile");
    auto r = dw.writeDocument(es, "ingested");
    ASSERT_TRUE(r.has_value());
    const std::string doc = dw.getDocument(*r);
    EXPECT_NE(doc.find("sha256:myfile"), std::string::npos);
}

// ─────────────────────────────────────────────────────────────────────────────
// SB — IngestionSinkBundle
// ─────────────────────────────────────────────────────────────────────────────

TEST(IngestionSinkBundle, SB01_WriteAll_CallsAllThreeSinks) {
    auto gw = std::make_shared<InMemoryGraphWriter>();
    auto vw = std::make_shared<InMemoryVectorWriter>();
    auto dw = std::make_shared<InMemoryDocWriter>();

    IngestionSinkBundle bundle{gw, vw, dw};

    auto es = makeEntitySet("f1");
    es.nodes.push_back(makeEntity(EntityType::PERSON, "X", 1.0, "person:x"));
    VectorRecord vr = makeVecRec("chunk:1");
    es.chunks.push_back(vr);

    auto result = bundle.writeAll(es, "test");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(gw->nodeCount(), 1u);
    EXPECT_EQ(vw->vectorCount(), 1u);
    EXPECT_EQ(dw->documentCount(), 1u);
}

TEST(IngestionSinkBundle, SB02_NullSink_Skipped_NoError) {
    auto gw = std::make_shared<InMemoryGraphWriter>();
    IngestionSinkBundle bundle{gw, nullptr, nullptr};

    auto es = makeEntitySet("f1");
    es.nodes.push_back(makeEntity(EntityType::DATE, "2024", 1.0, "date:1"));

    auto result = bundle.writeAll(es);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(gw->nodeCount(), 1u);
}

TEST(IngestionSinkBundle, SB03_EmptyChunks_NoVectorWrite) {
    auto gw = std::make_shared<InMemoryGraphWriter>();
    auto vw = std::make_shared<InMemoryVectorWriter>();
    IngestionSinkBundle bundle{gw, vw, nullptr};

    auto es = makeEntitySet("f1");
    // No chunks in es

    auto result = bundle.writeAll(es);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(vw->vectorCount(), 0u); // no vectors written
}
