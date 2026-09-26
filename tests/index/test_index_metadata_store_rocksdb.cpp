/**
 * @file test_index_metadata_store_rocksdb.cpp
 * @brief Tests for IndexMetadataStore RocksDB integration
 *
 * Tests atomic transactions, version number sequencing, and rollback atomicity.
 * Test cases: ROCKSDB-01..ROCKSDB-04
 *
 * @date 2026-09-24
 */

#include <filesystem>
#include <gtest/gtest.h>

#include "index/index_metadata_store.h"
#include "index/index_manifest_v1.h"

namespace fs = std::filesystem;
using namespace themis::index;

class IndexMetadataStoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create temporary RocksDB directory for each test
    db_path_ = fs::temp_directory_path() / "test_index_metadata_store_XXXXXX";
    // Avoid using mkdtemp, just use a unique path
    db_path_ = fs::temp_directory_path() / "test_index_metadata_store_" / std::to_string(time(nullptr)) /
               std::to_string(rand());
    fs::create_directories(db_path_);
  }

  void TearDown() override {
    // Clean up temporary directory
    if (fs::exists(db_path_)) {
      fs::remove_all(db_path_);
    }
  }

  std::string db_path_str() const { return db_path_.string(); }

 private:
  fs::path db_path_;
};

/**
 * @test ROCKSDB-01: Write and read manifest round-trip
 */
TEST_F(IndexMetadataStoreTest, ROCKSDB_01_WriteReadRoundTrip) {
  auto store = IndexMetadataStore::Open(db_path_str(), "wiki-index-test");

  // Create initial manifest
  IndexManifestV1 manifest;
  manifest.index_id = "wiki-index-test";
  manifest.current_version.version_number = 1;
  manifest.current_version.embedding_model_id = "all-minilm-l6-v2-1.0";
  manifest.current_version.embedding_dim = 384;

  // Create history entry
  VersionHistoryEntry entry;
  entry.version_number = 1;
  entry.embedding_model_id = "all-minilm-l6-v2-1.0";
  entry.action = "initial_index";

  // Write atomically
  store->WriteManifestAtomic(manifest, entry);

  // Read back
  auto restored = store->LoadManifest();
  EXPECT_EQ(restored.index_id, "wiki-index-test");
  EXPECT_EQ(restored.current_version.version_number, 1);
  EXPECT_EQ(restored.current_version.embedding_model_id, "all-minilm-l6-v2-1.0");

  // Verify history entry
  auto history = store->GetVersionHistory(1);
  EXPECT_EQ(history.version_number, 1);
  EXPECT_EQ(history.embedding_model_id, "all-minilm-l6-v2-1.0");
}

/**
 * @test ROCKSDB-02: Atomic version number increment
 */
TEST_F(IndexMetadataStoreTest, ROCKSDB_02_AtomicVersionIncrement) {
  auto store = IndexMetadataStore::Open(db_path_str(), "wiki-index-test");

  // Get multiple version numbers in sequence
  uint32_t v1 = store->GetNextVersionNumber();
  uint32_t v2 = store->GetNextVersionNumber();
  uint32_t v3 = store->GetNextVersionNumber();

  // Should be sequential
  EXPECT_EQ(v1, 1);
  EXPECT_EQ(v2, 2);
  EXPECT_EQ(v3, 3);

  // Create new store (reopen DB) and verify counter persisted
  store.reset();  // Close DB
  auto store2 = IndexMetadataStore::Open(db_path_str(), "wiki-index-test");
  uint32_t v4 = store2->GetNextVersionNumber();
  EXPECT_EQ(v4, 4);  // Counter should continue from last value
}

/**
 * @test ROCKSDB-03: Version history persistence and FIFO rotation
 */
TEST_F(IndexMetadataStoreTest, ROCKSDB_03_VersionHistoryPersistence) {
  auto store = IndexMetadataStore::Open(db_path_str(), "wiki-index-test");

  IndexManifestV1 manifest;
  manifest.index_id = "wiki-index-test";

  // Write 10 versions
  for (int i = 1; i <= 10; ++i) {
    manifest.current_version.version_number = i;
    manifest.current_version.embedding_model_id = "model-" + std::to_string(i);

    VersionHistoryEntry entry;
    entry.version_number = i;
    entry.embedding_model_id = "model-" + std::to_string(i);
    entry.action = "update";

    store->WriteManifestAtomic(manifest, entry);
  }

  // List all versions
  auto history = store->ListVersionHistory();
  EXPECT_EQ(history.size(), 10);
  EXPECT_EQ(history.front().version_number, 1);
  EXPECT_EQ(history.back().version_number, 10);
}

/**
 * @test ROCKSDB-04: Rollback to previous version atomicity
 */
TEST_F(IndexMetadataStoreTest, ROCKSDB_04_RollbackAtomicity) {
  auto store = IndexMetadataStore::Open(db_path_str(), "wiki-index-test");

  // Write version 1
  IndexManifestV1 manifest;
  manifest.index_id = "wiki-index-test";
  manifest.current_version.version_number = 1;
  manifest.current_version.embedding_model_id = "model-1.0";
  manifest.reindex_tracking.atomic_rollback_available = true;
  manifest.reindex_tracking.previous_version_accessible = true;

  VersionHistoryEntry entry1;
  entry1.version_number = 1;
  entry1.embedding_model_id = "model-1.0";
  store->WriteManifestAtomic(manifest, entry1);

  // Write version 2
  manifest.current_version.version_number = 2;
  manifest.current_version.embedding_model_id = "model-2.0";

  VersionHistoryEntry entry2;
  entry2.version_number = 2;
  entry2.embedding_model_id = "model-2.0";
  store->WriteManifestAtomic(manifest, entry2);

  // Verify current version is 2
  auto current = store->LoadManifest();
  EXPECT_EQ(current.current_version.version_number, 2);

  // Rollback to previous (version 1)
  EXPECT_TRUE(store->RollbackToPreviousVersion());

  // Verify current version is now 1
  current = store->LoadManifest();
  EXPECT_EQ(current.current_version.version_number, 1);
}

/**
 * @test Version number encoding/decoding
 */
TEST_F(IndexMetadataStoreTest, VersionNumberEncoding) {
  // Test big-endian encoding
  uint32_t version = 0x12345678;
  std::string encoded = IndexMetadataStore::EncodeVersionNumber(version);
  EXPECT_EQ(encoded.size(), 4);
  EXPECT_EQ((uint8_t)encoded[0], 0x12);
  EXPECT_EQ((uint8_t)encoded[1], 0x34);
  EXPECT_EQ((uint8_t)encoded[2], 0x56);
  EXPECT_EQ((uint8_t)encoded[3], 0x78);

  // Test decoding
  uint32_t decoded = IndexMetadataStore::DecodeVersionNumber(encoded);
  EXPECT_EQ(decoded, version);

  // Test round-trip for various values
  for (uint32_t v : {0, 1, 255, 256, 65535, 65536, 0xFFFFFFFF}) {
    auto enc = IndexMetadataStore::EncodeVersionNumber(v);
    auto dec = IndexMetadataStore::DecodeVersionNumber(enc);
    EXPECT_EQ(dec, v);
  }
}

/**
 * @test Can check rollback availability
 */
TEST_F(IndexMetadataStoreTest, CanCheckRollbackAvailability) {
  auto store = IndexMetadataStore::Open(db_path_str(), "wiki-index-test");

  // Version 1 should not exist yet
  EXPECT_FALSE(store->CanRollbackToVersion(1));

  // Write version 1
  IndexManifestV1 manifest;
  manifest.current_version.version_number = 1;
  VersionHistoryEntry entry;
  entry.version_number = 1;
  store->WriteManifestAtomic(manifest, entry);

  // Version 1 should now exist
  EXPECT_TRUE(store->CanRollbackToVersion(1));

  // Version 2 should not exist
  EXPECT_FALSE(store->CanRollbackToVersion(2));
}

/**
 * @test Multiple stores with different index IDs
 */
TEST_F(IndexMetadataStoreTest, MultipleIndexIds) {
  auto store1 = IndexMetadataStore::Open(db_path_str(), "index-1");
  auto store2 = IndexMetadataStore::Open(db_path_str(), "index-2");

  // Write to both
  IndexManifestV1 manifest1;
  manifest1.index_id = "index-1";
  manifest1.current_version.version_number = 1;

  IndexManifestV1 manifest2;
  manifest2.index_id = "index-2";
  manifest2.current_version.version_number = 10;

  VersionHistoryEntry entry1, entry2;
  entry1.version_number = 1;
  entry2.version_number = 10;

  store1->WriteManifestAtomic(manifest1, entry1);
  store2->WriteManifestAtomic(manifest2, entry2);

  // Verify both stored correctly (they share the same DB but use different indices)
  auto restored1 = store1->LoadManifest();
  auto restored2 = store2->LoadManifest();

  EXPECT_EQ(restored1.current_version.version_number, 1);
  EXPECT_EQ(restored2.current_version.version_number, 10);
}
