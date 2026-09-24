/**
 * @file test_reindex_decision_engine.cpp
 * @brief Comprehensive tests for ReindexDecisionEngine
 *
 * Tests all 4 reindex trigger conditions and simplified APIs.
 * Test cases: RINDEX-01..RINDEX-08
 *
 * @date 2026-09-24
 */

#include <gtest/gtest.h>
#include "ingestion/reindex_decision_engine.h"

using namespace themis::ingestion;

/**
 * @test RINDEX-01: Embedding model ID change triggers reindex
 * Example: Upgrade from all-minilm-l6-v2-0.9 to all-minilm-l6-v2-1.0
 */
TEST(ReindexDecisionEngineTest, RINDEX_01_EmbeddingModelChange) {
  ReindexDecisionEngine engine;

  EmbeddingMetadata old_meta;
  old_meta.embedding_model_id = "all-minilm-l6-v2-0.9";
  old_meta.embedding_model_name = "all-MiniLM-L6-v2";
  old_meta.embedding_dim = 384;

  EmbeddingMetadata new_meta;
  new_meta.embedding_model_id = "all-minilm-l6-v2-1.0";
  new_meta.embedding_model_name = "all-MiniLM-L6-v2";
  new_meta.embedding_dim = 384;

  ChunkingMetadata old_chunking, new_chunking;
  old_chunking.chunking_profile_id = "default_256_overlap_32";
  new_chunking.chunking_profile_id = "default_256_overlap_32";

  IndexSchema old_schema, new_schema;
  old_schema.schema_version = "2.0";
  new_schema.schema_version = "2.0";

  auto decision = engine.decide_reindex(old_meta, new_meta, old_chunking, new_chunking,
                                        old_schema, new_schema);

  EXPECT_TRUE(decision.reindex_required);
  EXPECT_EQ(decision.reason, "embedding_model_change");
  EXPECT_EQ(decision.estimated_duration_seconds, 3600.0);
  EXPECT_TRUE(decision.atomic_rollback_available);
}

/**
 * @test RINDEX-02: Embedding dimension change triggers reindex
 * Example: Upgrade from 384-dim to 1024-dim model
 */
TEST(ReindexDecisionEngineTest, RINDEX_02_EmbeddingDimensionChange) {
  ReindexDecisionEngine engine;

  EmbeddingMetadata old_meta;
  old_meta.embedding_model_id = "all-minilm-l6-v2-1.0";
  old_meta.embedding_dim = 384;

  EmbeddingMetadata new_meta;
  new_meta.embedding_model_id = "all-minilm-l6-v2-1.0";
  new_meta.embedding_dim = 1024;

  ChunkingMetadata old_chunking, new_chunking;
  old_chunking.chunking_profile_id = "default_256_overlap_32";
  new_chunking.chunking_profile_id = "default_256_overlap_32";

  IndexSchema old_schema, new_schema;
  old_schema.schema_version = "2.0";
  new_schema.schema_version = "2.0";

  auto decision = engine.decide_reindex(old_meta, new_meta, old_chunking, new_chunking,
                                        old_schema, new_schema);

  EXPECT_TRUE(decision.reindex_required);
  EXPECT_EQ(decision.reason, "embedding_dim_change");
  EXPECT_EQ(decision.estimated_duration_seconds, 3600.0);
}

/**
 * @test RINDEX-03: Chunking profile change triggers reindex
 * Example: Change from 256-byte chunks to 512-byte chunks
 */
TEST(ReindexDecisionEngineTest, RINDEX_03_ChunkingProfileChange) {
  ReindexDecisionEngine engine;

  EmbeddingMetadata old_meta, new_meta;
  old_meta.embedding_model_id = "all-minilm-l6-v2-1.0";
  old_meta.embedding_dim = 384;
  new_meta = old_meta;  // No embedding change

  ChunkingMetadata old_chunking;
  old_chunking.chunking_profile_id = "default_256_overlap_32";
  old_chunking.chunk_size_bytes = 256;

  ChunkingMetadata new_chunking;
  new_chunking.chunking_profile_id = "default_512_overlap_64";
  new_chunking.chunk_size_bytes = 512;

  IndexSchema old_schema, new_schema;
  old_schema.schema_version = "2.0";
  new_schema.schema_version = "2.0";

  auto decision = engine.decide_reindex(old_meta, new_meta, old_chunking, new_chunking,
                                        old_schema, new_schema);

  EXPECT_TRUE(decision.reindex_required);
  EXPECT_EQ(decision.reason, "chunking_profile_change");
  EXPECT_EQ(decision.estimated_duration_seconds, 1800.0);
}

/**
 * @test RINDEX-04: Index schema major version change triggers reindex
 * Example: Upgrade from schema 1.x to 2.0 (hybrid BM25+HNSW)
 */
TEST(ReindexDecisionEngineTest, RINDEX_04_SchemaMajorVersionChange) {
  ReindexDecisionEngine engine;

  EmbeddingMetadata old_meta, new_meta;
  old_meta.embedding_model_id = "all-minilm-l6-v2-1.0";
  old_meta.embedding_dim = 384;
  new_meta = old_meta;

  ChunkingMetadata old_chunking, new_chunking;
  old_chunking.chunking_profile_id = "default_256_overlap_32";
  new_chunking.chunking_profile_id = "default_256_overlap_32";

  IndexSchema old_schema;
  old_schema.schema_version = "1.0";
  old_schema.index_type = "tfidf_only";

  IndexSchema new_schema;
  new_schema.schema_version = "2.0";
  new_schema.index_type = "hybrid_bm25_hnsw";

  auto decision = engine.decide_reindex(old_meta, new_meta, old_chunking, new_chunking,
                                        old_schema, new_schema);

  EXPECT_TRUE(decision.reindex_required);
  EXPECT_EQ(decision.reason, "index_schema_major_change");
  EXPECT_EQ(decision.estimated_duration_seconds, 7200.0);
}

/**
 * @test RINDEX-05: No reindex needed when metadata unchanged
 */
TEST(ReindexDecisionEngineTest, RINDEX_05_NoChangeRequired) {
  ReindexDecisionEngine engine;

  EmbeddingMetadata old_meta, new_meta;
  old_meta.embedding_model_id = "all-minilm-l6-v2-1.0";
  old_meta.embedding_dim = 384;
  new_meta = old_meta;

  ChunkingMetadata old_chunking, new_chunking;
  old_chunking.chunking_profile_id = "default_256_overlap_32";
  new_chunking.chunking_profile_id = "default_256_overlap_32";

  IndexSchema old_schema, new_schema;
  old_schema.schema_version = "2.0";
  new_schema.schema_version = "2.0";

  auto decision = engine.decide_reindex(old_meta, new_meta, old_chunking, new_chunking,
                                        old_schema, new_schema);

  EXPECT_FALSE(decision.reindex_required);
  EXPECT_EQ(decision.reason, "no_reindex_needed");
  EXPECT_EQ(decision.estimated_duration_seconds, 0.0);
}

/**
 * @test RINDEX-06: Simplified embedding-only API detects model change
 */
TEST(ReindexDecisionEngineTest, RINDEX_06_EmbeddingOnlyAPI) {
  ReindexDecisionEngine engine;

  EmbeddingMetadata old_meta;
  old_meta.embedding_model_id = "all-minilm-l6-v2-0.9";
  old_meta.embedding_dim = 384;

  EmbeddingMetadata new_meta;
  new_meta.embedding_model_id = "all-minilm-l6-v2-1.0";
  new_meta.embedding_dim = 384;

  auto decision = engine.decide_reindex_embedding_only(old_meta, new_meta);

  EXPECT_TRUE(decision.reindex_required);
  EXPECT_EQ(decision.reason, "embedding_model_change");
}

/**
 * @test RINDEX-07: Simplified chunking-only API detects profile change
 */
TEST(ReindexDecisionEngineTest, RINDEX_07_ChunkingOnlyAPI) {
  ReindexDecisionEngine engine;

  ChunkingMetadata old_chunking;
  old_chunking.chunking_profile_id = "default_256_overlap_32";

  ChunkingMetadata new_chunking;
  new_chunking.chunking_profile_id = "default_512_overlap_64";

  auto decision = engine.decide_reindex_chunking_only(old_chunking, new_chunking);

  EXPECT_TRUE(decision.reindex_required);
  EXPECT_EQ(decision.reason, "chunking_profile_change");
}

/**
 * @test RINDEX-08: Simplified schema-only API detects version change
 */
TEST(ReindexDecisionEngineTest, RINDEX_08_SchemaOnlyAPI) {
  ReindexDecisionEngine engine;

  IndexSchema old_schema;
  old_schema.schema_version = "1.0";

  IndexSchema new_schema;
  new_schema.schema_version = "2.0";

  auto decision = engine.decide_reindex_schema_only(old_schema, new_schema);

  EXPECT_TRUE(decision.reindex_required);
  EXPECT_EQ(decision.reason, "index_schema_major_change");
}

// ============================================================================
// JSON Serialization Tests
// ============================================================================

/**
 * @test JSON serialization round-trip for EmbeddingMetadata
 */
TEST(ReindexDecisionEngineTest, EmbeddingMetadataJsonRoundTrip) {
  EmbeddingMetadata orig;
  orig.embedding_model_id = "all-minilm-l6-v2-1.0";
  orig.embedding_model_name = "all-MiniLM-L6-v2";
  orig.embedding_model_version = "1.0";
  orig.embedding_dim = 384;
  orig.embedding_hash = "e5f3e3c9d7b4a1f6";
  orig.embedding_provider = "huggingface";

  auto json = orig.to_json();
  auto restored = EmbeddingMetadata::from_json(json);

  EXPECT_EQ(restored.embedding_model_id, orig.embedding_model_id);
  EXPECT_EQ(restored.embedding_dim, orig.embedding_dim);
  EXPECT_EQ(restored.embedding_hash, orig.embedding_hash);
}

/**
 * @test IndexSchema::major_version() correctly extracts version
 */
TEST(ReindexDecisionEngineTest, SchemaMajorVersionExtraction) {
  IndexSchema schema_v1;
  schema_v1.schema_version = "1.5";
  EXPECT_EQ(schema_v1.major_version(), 1);

  IndexSchema schema_v2;
  schema_v2.schema_version = "2.0";
  EXPECT_EQ(schema_v2.major_version(), 2);

  IndexSchema schema_v10;
  schema_v10.schema_version = "10.3";
  EXPECT_EQ(schema_v10.major_version(), 10);
}
