/*
 * ThemisDB — Ingestion BaseEntity / ExtractionContext / FileManifest Tests
 *
 * Tests for:
 *   FileManifest helper methods                FM-01..FM-03
 *   ExtractionContext state management         EC-01..EC-07
 *   BaseEntity helper methods                  BA-01..BA-04
 *   EntityRelation construction                BA-05..BA-06
 *   VectorRecord                               BA-07..BA-08
 *   BaseEntitySet                              BA-09..BA-10
 */

#include <gtest/gtest.h>

#include "ingestion/base_entity.h"
#include "ingestion/extraction_context.h"
#include "ingestion/file_manifest.h"

using namespace themis::ingestion;

// ─────────────────────────────────────────────────────────────────────────────
// FM — FileManifest
// ─────────────────────────────────────────────────────────────────────────────

TEST(IngestionFileManifest, FM01_HasKnownFormat) {
    FileManifest m;
    EXPECT_FALSE(m.hasKnownFormat());
    m.detected_format = FileFormat::PDF;
    EXPECT_TRUE(m.hasKnownFormat());
}

TEST(IngestionFileManifest, FM02_HasMime) {
    FileManifest m;
    EXPECT_FALSE(m.hasMime());
    m.detected_mime = "application/pdf";
    EXPECT_TRUE(m.hasMime());
}

TEST(IngestionFileManifest, FM03_ExifOr) {
    FileManifest m;
    const std::string def = "default_val";
    EXPECT_EQ(m.exifOr("Author", def), def);
    m.exif["Author"] = "Max Mustermann";
    EXPECT_EQ(m.exifOr("Author", def), "Max Mustermann");
}

// ─────────────────────────────────────────────────────────────────────────────
// EC — ExtractionContext
// ─────────────────────────────────────────────────────────────────────────────

TEST(IngestionExtractionContext, EC01_DefaultAllFalse) {
    ExtractionContext ctx;
    EXPECT_FALSE(ctx.hasText());
    EXPECT_FALSE(ctx.hasChunks());
    EXPECT_FALSE(ctx.hasEntities());
    EXPECT_FALSE(ctx.hasEmbeddings());
    EXPECT_FALSE(ctx.hasGeoFeatures());
    EXPECT_FALSE(ctx.hasTableRows());
}

TEST(IngestionExtractionContext, EC02_HasTextAfterAssign) {
    ExtractionContext ctx;
    ctx.raw_text = "hello";
    EXPECT_TRUE(ctx.hasText());
}

TEST(IngestionExtractionContext, EC03_HasChunksAfterAdd) {
    ExtractionContext ctx;
    TextChunk c;
    c.text = "chunk text";
    ctx.chunks.push_back(c);
    EXPECT_TRUE(ctx.hasChunks());
}

TEST(IngestionExtractionContext, EC04_HasEntitiesAfterAdd) {
    ExtractionContext ctx;
    ctx.entities.push_back(BaseEntity{});
    EXPECT_TRUE(ctx.hasEntities());
}

TEST(IngestionExtractionContext, EC05_HasEmbeddingsAfterAdd) {
    ExtractionContext ctx;
    ctx.embeddings.push_back(VectorRecord{"c1", "f1", "text", {1.f, 2.f}});
    EXPECT_TRUE(ctx.hasEmbeddings());
}

TEST(IngestionExtractionContext, EC06_HasGeoFeaturesAfterAdd) {
    ExtractionContext ctx;
    GeoFeature gf;
    gf.id = "gf1";
    gf.geometry_type = "Point";
    ctx.geo_features.push_back(gf);
    EXPECT_TRUE(ctx.hasGeoFeatures());
}

TEST(IngestionExtractionContext, EC07_ExtraOr) {
    ExtractionContext ctx;
    EXPECT_EQ(ctx.extraOr("missing", "def"), "def");
    ctx.extra["key"] = "value";
    EXPECT_EQ(ctx.extraOr("key", "def"), "value");
}

// ─────────────────────────────────────────────────────────────────────────────
// BA — BaseEntity
// ─────────────────────────────────────────────────────────────────────────────

TEST(IngestionBaseEntity, BA01_DefaultEntityTypeUnknown) {
    BaseEntity e;
    EXPECT_EQ(e.entity_type, EntityType::UNKNOWN);
}

TEST(IngestionBaseEntity, BA02_PropertyOrDefaultWhenAbsent) {
    BaseEntity e;
    EXPECT_EQ(e.propertyOr("missing", "fallback"), "fallback");
}

TEST(IngestionBaseEntity, BA03_PropertyOrReturnsValue) {
    BaseEntity e;
    e.properties["section_ref"] = "§ 4 Abs. 1";
    EXPECT_EQ(e.propertyOr("section_ref", ""), "§ 4 Abs. 1");
}

TEST(IngestionBaseEntity, BA04_HasEmbedding) {
    BaseEntity e;
    EXPECT_FALSE(e.hasEmbedding());
    e.embeddings = {1.f, 2.f, 3.f};
    EXPECT_TRUE(e.hasEmbedding());
}

TEST(IngestionBaseEntity, BA05_DefaultRelationTypeUnknown) {
    EntityRelation r;
    EXPECT_EQ(r.relation_type, RelationType::UNKNOWN);
}

TEST(IngestionBaseEntity, BA06_RelationFromToRoundtrip) {
    EntityRelation r;
    r.from_id = "law:BImSchG:§4";
    r.to_id   = "law:BImSchG:§3";
    r.relation_type = RelationType::CITES;
    EXPECT_EQ(r.from_id, "law:BImSchG:§4");
    EXPECT_EQ(r.to_id,   "law:BImSchG:§3");
    EXPECT_EQ(r.relation_type, RelationType::CITES);
}

TEST(IngestionBaseEntity, BA07_VectorRecordRoundtrip) {
    VectorRecord v;
    v.chunk_id = "chunk:sha256:aabb:0";
    v.source_file_id = "sha256:aabb";
    v.text_snippet   = "Das Unternehmen muss ...";
    v.embedding      = {0.1f, 0.2f, 0.3f};
    EXPECT_EQ(v.chunk_id,       "chunk:sha256:aabb:0");
    EXPECT_EQ(v.source_file_id, "sha256:aabb");
    EXPECT_EQ(v.embedding.size(), 3u);
}

TEST(IngestionBaseEntity, BA08_VectorRecordMetadata) {
    VectorRecord v;
    v.metadata["section_ref"] = "§ 4";
    v.metadata["language"]    = "de";
    EXPECT_EQ(v.metadata["section_ref"], "§ 4");
    EXPECT_EQ(v.metadata["language"],    "de");
}

TEST(IngestionBaseEntity, BA09_BaseEntitySetDefaultQualityScore) {
    BaseEntitySet s;
    EXPECT_DOUBLE_EQ(s.quality_score, 0.0);
}

TEST(IngestionBaseEntity, BA10_BaseEntitySetPopulated) {
    BaseEntitySet s;
    s.nodes.push_back(BaseEntity{});
    s.nodes.push_back(BaseEntity{});
    s.edges.push_back(EntityRelation{});
    s.chunks.push_back(VectorRecord{});
    s.quality_score = 0.85;
    EXPECT_EQ(s.nodes.size(),  2u);
    EXPECT_EQ(s.edges.size(),  1u);
    EXPECT_EQ(s.chunks.size(), 1u);
    EXPECT_DOUBLE_EQ(s.quality_score, 0.85);
}
