/**
 * @file test_index_manifest_v1.cpp
 * @brief Comprehensive tests for IndexManifestV1
 *
 * Tests JSON serialization/deserialization, RocksDB persistence,
 * version history rotation, and rollback availability checks.
 * Test cases: MANIFEST-01..MANIFEST-06
 *
 * @date 2026-09-24
 */

#include <gtest/gtest.h>
#include "index/index_manifest_v1.h"

using namespace themis::index;

/**
 * @test MANIFEST-01: Serialize manifest to JSON and back
 */
TEST(IndexManifestV1Test, MANIFEST_01_JsonRoundTrip) {
  IndexManifestV1 manifest;
  manifest.manifest_version = "1.0";
  manifest.index_id = "wiki-index-main";
  manifest.created_at = "2026-09-24T05:43:22Z";
  manifest.last_updated_at = "2026-09-24T06:30:15Z";

  manifest.current_version.version_number = 42;
  manifest.current_version.timestamp = "2026-09-24T06:30:15Z";
  manifest.current_version.embedding_model_id = "all-minilm-l6-v2-1.0";
  manifest.current_version.embedding_dim = 384;
  manifest.current_version.chunking_profile_id = "default_256_overlap_32";
  manifest.current_version.document_count = 6000000;
  manifest.current_version.index_size_bytes = 4294967296;

  // Serialize to JSON
  auto json = manifest.to_json();
  EXPECT_TRUE(json.contains("manifest_version"));
  EXPECT_TRUE(json.contains("index_id"));
  EXPECT_TRUE(json.contains("current_version"));

  // Deserialize back
  auto restored = IndexManifestV1::from_json(json);
  EXPECT_EQ(restored.index_id, manifest.index_id);
  EXPECT_EQ(restored.current_version.version_number, 42);
  EXPECT_EQ(restored.current_version.embedding_model_id, "all-minilm-l6-v2-1.0");
  EXPECT_EQ(restored.current_version.document_count, 6000000);
}

/**
 * @test MANIFEST-02: RocksDB value serialization (minified JSON)
 */
TEST(IndexManifestV1Test, MANIFEST_02_RocksDBValueFormat) {
  IndexManifestV1 manifest;
  manifest.index_id = "wiki-index-main";
  manifest.current_version.version_number = 42;
  manifest.current_version.embedding_model_id = "all-minilm-l6-v2-1.0";

  // Serialize to RocksDB format
  std::string rocksdb_value = manifest.to_rocksdb_value();
  EXPECT_FALSE(rocksdb_value.empty());

  // Should be valid JSON (minified, no whitespace)
  EXPECT_TRUE(rocksdb_value.find("\"manifest_version\"") != std::string::npos);

  // Deserialize from RocksDB format
  auto restored = IndexManifestV1::from_rocksdb_value(rocksdb_value);
  EXPECT_EQ(restored.index_id, "wiki-index-main");
  EXPECT_EQ(restored.current_version.version_number, 42);
}

/**
 * @test MANIFEST-03: Version history rotation (FIFO, max 30 entries)
 */
TEST(IndexManifestV1Test, MANIFEST_03_VersionHistoryRotation) {
  IndexManifestV1 manifest;
  manifest.index_id = "wiki-index-main";

  // Add 35 entries to test rotation
  for (int i = 1; i <= 35; ++i) {
    VersionHistoryEntry entry;
    entry.version_number = i;
    entry.timestamp = "2026-09-24T00:00:00Z";
    entry.embedding_model_id = "model-" + std::to_string(i);
    entry.action = "incremental_update";
    manifest.add_version_history(entry);
  }

  // Should have rotated: 35 entries added, but only 30 kept (FIFO)
  EXPECT_EQ(manifest.version_history.size(), 30);

  // First entry should be version 6 (oldest retained after rotation)
  EXPECT_EQ(manifest.version_history.front().version_number, 6);

  // Last entry should be version 35 (most recent)
  EXPECT_EQ(manifest.version_history.back().version_number, 35);
}

/**
 * @test MANIFEST-04: Get previous version number for rollback
 */
TEST(IndexManifestV1Test, MANIFEST_04_GetPreviousVersion) {
  IndexManifestV1 manifest;

  // No history yet
  EXPECT_EQ(manifest.get_previous_version_number(), 0);

  // Add first version
  VersionHistoryEntry v1;
  v1.version_number = 40;
  manifest.add_version_history(v1);
  EXPECT_EQ(manifest.get_previous_version_number(), 0);

  // Add second version
  VersionHistoryEntry v2;
  v2.version_number = 41;
  manifest.add_version_history(v2);
  EXPECT_EQ(manifest.get_previous_version_number(), 40);

  // Add third version
  VersionHistoryEntry v3;
  v3.version_number = 42;
  manifest.add_version_history(v3);
  EXPECT_EQ(manifest.get_previous_version_number(), 41);
}

/**
 * @test MANIFEST-05: Rollback availability checks
 */
TEST(IndexManifestV1Test, MANIFEST_05_RollbackAvailability) {
  IndexManifestV1 manifest;

  // Initially cannot rollback (no history)
  EXPECT_FALSE(manifest.can_rollback_to_previous());

  // Add one entry (still can't rollback, need at least 2)
  VersionHistoryEntry v1;
  v1.version_number = 40;
  manifest.add_version_history(v1);
  EXPECT_FALSE(manifest.can_rollback_to_previous());

  // Add second entry
  VersionHistoryEntry v2;
  v2.version_number = 41;
  manifest.add_version_history(v2);

  // Can rollback when: history >= 2 AND atomic_rollback_available AND previous_accessible
  manifest.reindex_tracking.atomic_rollback_available = true;
  manifest.reindex_tracking.previous_version_accessible = true;
  EXPECT_TRUE(manifest.can_rollback_to_previous());

  // Cannot rollback if atomic rollback not available
  manifest.reindex_tracking.atomic_rollback_available = false;
  EXPECT_FALSE(manifest.can_rollback_to_previous());

  // Cannot rollback if previous version not accessible
  manifest.reindex_tracking.atomic_rollback_available = true;
  manifest.reindex_tracking.previous_version_accessible = false;
  EXPECT_FALSE(manifest.can_rollback_to_previous());
}

/**
 * @test MANIFEST-06: Reindex tracking information persistence
 */
TEST(IndexManifestV1Test, MANIFEST_06_ReindexTrackingPersistence) {
  IndexManifestV1 manifest;
  manifest.index_id = "wiki-index-main";

  // Set reindex tracking info
  manifest.reindex_tracking.last_reindex_reason = "embedding_model_upgrade";
  manifest.reindex_tracking.last_reindex_start = "2026-09-19T18:00:00Z";
  manifest.reindex_tracking.last_reindex_end = "2026-09-20T02:00:00Z";
  manifest.reindex_tracking.last_reindex_status = "success";
  manifest.reindex_tracking.atomic_rollback_available = true;
  manifest.reindex_tracking.previous_version_accessible = true;

  // Serialize and deserialize
  std::string rocksdb_value = manifest.to_rocksdb_value();
  auto restored = IndexManifestV1::from_rocksdb_value(rocksdb_value);

  // Verify reindex tracking was preserved
  EXPECT_EQ(restored.reindex_tracking.last_reindex_reason, "embedding_model_upgrade");
  EXPECT_EQ(restored.reindex_tracking.last_reindex_status, "success");
  EXPECT_TRUE(restored.reindex_tracking.atomic_rollback_available);
}

// ============================================================================
// Component Serialization Tests
// ============================================================================

/**
 * @test IndexVersionInfo JSON round-trip
 */
TEST(IndexManifestV1Test, IndexVersionInfoJsonRoundTrip) {
  IndexVersionInfo orig;
  orig.version_number = 42;
  orig.timestamp = "2026-09-24T06:30:15Z";
  orig.embedding_model_id = "all-minilm-l6-v2-1.0";
  orig.embedding_dim = 384;
  orig.chunking_profile_id = "default_256_overlap_32";
  orig.schema_version = "2.0";
  orig.document_count = 6000000;
  orig.index_size_bytes = 4294967296;
  orig.last_indexed_at = "2026-09-24T06:30:15Z";

  auto json = orig.to_json();
  auto restored = IndexVersionInfo::from_json(json);

  EXPECT_EQ(restored.version_number, 42);
  EXPECT_EQ(restored.embedding_model_id, "all-minilm-l6-v2-1.0");
  EXPECT_EQ(restored.embedding_dim, 384);
  EXPECT_EQ(restored.document_count, 6000000);
}

/**
 * @test VersionHistoryEntry JSON round-trip
 */
TEST(IndexManifestV1Test, VersionHistoryEntryJsonRoundTrip) {
  VersionHistoryEntry orig;
  orig.version_number = 40;
  orig.timestamp = "2026-09-20T02:00:00Z";
  orig.embedding_model_id = "all-minilm-l6-v2-0.9";
  orig.chunking_profile_id = "default_256_overlap_32";
  orig.schema_version = "2.0";
  orig.action = "reindex_complete";
  orig.reason = "embedding_model_upgrade";
  orig.documents_added = 100000;
  orig.documents_removed = 50000;

  auto json = orig.to_json();
  auto restored = VersionHistoryEntry::from_json(json);

  EXPECT_EQ(restored.version_number, 40);
  EXPECT_EQ(restored.action, "reindex_complete");
  EXPECT_EQ(restored.reason, "embedding_model_upgrade");
  EXPECT_EQ(restored.documents_added, 100000);
}

/**
 * @test ReindexTrackingInfo JSON round-trip
 */
TEST(IndexManifestV1Test, ReindexTrackingInfoJsonRoundTrip) {
  ReindexTrackingInfo orig;
  orig.last_reindex_reason = "embedding_model_upgrade";
  orig.last_reindex_start = "2026-09-19T18:00:00Z";
  orig.last_reindex_end = "2026-09-20T02:00:00Z";
  orig.last_reindex_status = "success";
  orig.atomic_rollback_available = true;
  orig.previous_version_accessible = true;

  auto json = orig.to_json();
  auto restored = ReindexTrackingInfo::from_json(json);

  EXPECT_EQ(restored.last_reindex_reason, "embedding_model_upgrade");
  EXPECT_EQ(restored.last_reindex_status, "success");
  EXPECT_TRUE(restored.atomic_rollback_available);
}
