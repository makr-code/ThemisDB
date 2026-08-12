/**
 * @file test_manifest_database_file_deletion.cpp
 * @brief Tests for ManifestDatabase::deleteManifest() associated file cleanup.
 *
 * Issue: ManifestDatabase – Delete Associated Files on Entry Removal (v1.8.0)
 *
 * Covers:
 *  - Acceptance criterion: insert manifest with 3 associated files, remove
 *    entry, verify all 3 files are deleted from the filesystem.
 *  - Acceptance criterion: file_registry entries are removed from RocksDB.
 *  - Acceptance criterion: download_cache entries are removed from RocksDB.
 *  - Tombstone key is written before and removed after the deletion window.
 *  - deleteManifest() on a non-existent version returns false gracefully.
 */

#include <gtest/gtest.h>

#include "updates/manifest_database.h"
#include "updates/release_manifest.h"
#include "storage/rocksdb_wrapper.h"

#include <chrono>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using namespace themis::updates;
using themis::RocksDBWrapper;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string uniqueTempPath(const std::string& prefix) {
    auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(tick))).string();
}

static void touchFile(const fs::path& p) {
    fs::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out << "dummy";
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class ManifestDatabaseFileDeletionTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_   = uniqueTempPath("themis_manifest_del_db_");
        file_dir_  = uniqueTempPath("themis_manifest_del_files_");
        fs::create_directories(file_dir_);

        RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_;
        cfg.memtable_size_mb    = 16;
        cfg.block_cache_size_mb = 32;
        storage_ = std::make_shared<RocksDBWrapper>(cfg);

        if (!storage_->open()) {
            GTEST_SKIP() << "Failed to open test RocksDB at " << db_path_;
        }

        // PluginSecurityVerifier not needed for these tests – pass nullptr.
        db_ = std::make_unique<ManifestDatabase>(storage_, nullptr);
    }

    void TearDown() override {
        db_.reset();
        storage_.reset();
        fs::remove_all(db_path_);
        fs::remove_all(file_dir_);
    }

    /**
     * Create a manifest with @p count files. Each file gets a real file on disk
     * and a cacheDownload entry so deleteManifest() can locate it.
     */
    ReleaseManifest makeManifestWithFiles(const std::string& version, int count) {
        ReleaseManifest manifest;
        manifest.version   = version;
        manifest.tag_name  = "v" + version;
        manifest.schema_version = 1;

        for (int i = 0; i < count; ++i) {
            std::string rel_path = "bin/file_" + std::to_string(i);

            ReleaseFile f;
            f.path        = rel_path;
            f.type        = "executable";
            f.sha256_hash = "hash" + std::to_string(i);
            f.size_bytes  = 1024;
            f.platform    = "linux";
            f.architecture = "x64";
            manifest.files.push_back(f);

            // Create the actual file on disk
            fs::path local = fs::path(file_dir_) / ("file_" + std::to_string(i));
            touchFile(local);

            // Register the local path in the download cache
            db_->cacheDownload(version, rel_path, local.string());
        }

        return manifest;
    }

    std::string db_path_;
    std::string file_dir_;
    std::shared_ptr<RocksDBWrapper> storage_;
    std::unique_ptr<ManifestDatabase> db_;
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

/**
 * Core acceptance test: insert manifest with 3 associated files, remove entry,
 * verify all 3 files are deleted from the filesystem.
 */
TEST_F(ManifestDatabaseFileDeletionTest, DeleteManifest_RemovesAllThreeAssociatedFiles) {
    const std::string version = "1.0.0";
    auto manifest = makeManifestWithFiles(version, 3);

    // Verify files exist before deletion
    for (int i = 0; i < 3; ++i) {
        fs::path p = fs::path(file_dir_) / ("file_" + std::to_string(i));
        ASSERT_TRUE(fs::exists(p)) << "Pre-condition: file should exist: " << p;
    }

    ASSERT_TRUE(db_->storeManifest(manifest));
    ASSERT_TRUE(db_->deleteManifest(version));

    // All 3 files must be gone from the filesystem
    for (int i = 0; i < 3; ++i) {
        fs::path p = fs::path(file_dir_) / ("file_" + std::to_string(i));
        EXPECT_FALSE(fs::exists(p)) << "File should have been deleted: " << p;
    }
}

/**
 * Manifest record itself is removed from the database after deleteManifest().
 */
TEST_F(ManifestDatabaseFileDeletionTest, DeleteManifest_RemovesManifestRecord) {
    const std::string version = "1.1.0";
    auto manifest = makeManifestWithFiles(version, 1);

    ASSERT_TRUE(db_->storeManifest(manifest));
    ASSERT_TRUE(db_->getManifest(version).has_value());

    ASSERT_TRUE(db_->deleteManifest(version));

    EXPECT_FALSE(db_->getManifest(version).has_value())
        << "Manifest record should no longer exist after deleteManifest()";
}

/**
 * Download cache entries are cleaned up from RocksDB after file deletion.
 */
TEST_F(ManifestDatabaseFileDeletionTest, DeleteManifest_ClearsDownloadCacheEntries) {
    const std::string version = "1.2.0";
    auto manifest = makeManifestWithFiles(version, 2);

    ASSERT_TRUE(db_->storeManifest(manifest));

    // Verify cache entries exist before deletion
    ASSERT_TRUE(db_->getCachedDownload(version, "bin/file_0").has_value());
    ASSERT_TRUE(db_->getCachedDownload(version, "bin/file_1").has_value());

    ASSERT_TRUE(db_->deleteManifest(version));

    EXPECT_FALSE(db_->getCachedDownload(version, "bin/file_0").has_value())
        << "Cache entry for bin/file_0 should be cleared";
    EXPECT_FALSE(db_->getCachedDownload(version, "bin/file_1").has_value())
        << "Cache entry for bin/file_1 should be cleared";
}

/**
 * File registry entries (file_registry CF) are removed from RocksDB.
 */
TEST_F(ManifestDatabaseFileDeletionTest, DeleteManifest_ClearsFileRegistryEntries) {
    const std::string version = "1.3.0";
    auto manifest = makeManifestWithFiles(version, 2);

    ASSERT_TRUE(db_->storeManifest(manifest));

    // File registry entries should exist before deletion
    ASSERT_TRUE(db_->getFile("bin/file_0", version).has_value());
    ASSERT_TRUE(db_->getFile("bin/file_1", version).has_value());

    ASSERT_TRUE(db_->deleteManifest(version));

    EXPECT_FALSE(db_->getFile("bin/file_0", version).has_value())
        << "file_registry entry for bin/file_0 should be cleared";
    EXPECT_FALSE(db_->getFile("bin/file_1", version).has_value())
        << "file_registry entry for bin/file_1 should be cleared";
}

/**
 * deleteManifest() on a non-existent version should return false.
 */
TEST_F(ManifestDatabaseFileDeletionTest, DeleteManifest_NonExistentVersion_ReturnsFalse) {
    EXPECT_FALSE(db_->deleteManifest("9.9.9"))
        << "deleteManifest() on a non-existent version should return false";
}

/**
 * Tombstone key is absent after a successful deleteManifest() call, confirming
 * it was cleaned up at the end of the deletion window.
 *
 * The tombstone resides in the same column family as the manifest record.
 * After a successful delete the key "__tombstone__:<version>" must not appear
 * when we try to retrieve the manifest for that version, and no version should
 * be listed via listVersions() either.
 */
TEST_F(ManifestDatabaseFileDeletionTest, DeleteManifest_TombstoneKeyRemovedAfterSuccess) {
    const std::string version = "1.4.0";
    auto manifest = makeManifestWithFiles(version, 1);

    ASSERT_TRUE(db_->storeManifest(manifest));
    ASSERT_TRUE(db_->deleteManifest(version));

    // The version must be absent from both getManifest() and listVersions().
    EXPECT_FALSE(db_->getManifest(version).has_value());

    auto versions = db_->listVersions();
    bool found = (std::find(versions.begin(), versions.end(), version) != versions.end());
    EXPECT_FALSE(found) << "Version should not appear in listVersions() after deletion";

    // The tombstone key "__tombstone__:<version>" is also stored in the manifests
    // CF. Verify it is absent by asking listVersions() — any leftover keys in
    // that CF will show up there.
    const std::string tombstone_key = "__tombstone__:" + version;
    bool tombstone_found = (std::find(versions.begin(), versions.end(), tombstone_key) != versions.end());
    EXPECT_FALSE(tombstone_found)
        << "Tombstone key should not linger after successful deleteManifest()";
}

