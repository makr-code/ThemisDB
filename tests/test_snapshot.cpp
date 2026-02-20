// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for BackupManager::createSnapshot / restoreFromSnapshot / verifySnapshot / listSnapshots
// using the real RocksDB Checkpoint API (no stubs).

#include <gtest/gtest.h>
#include "storage/backup_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class SnapshotTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto ts = std::chrono::system_clock::now().time_since_epoch().count();
        base_  = fs::temp_directory_path() / ("themis_snap_test_" + std::to_string(ts));
        db_dir_ = base_ / "db";
        fs::create_directories(db_dir_);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = db_dir_.string();
        cfg.enable_wal = true;
        db_   = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open DB";
        mgr_ = std::make_unique<BackupManager>(db_);
    }

    void TearDown() override {
        mgr_.reset();
        if (db_) { db_->close(); db_.reset(); }
        fs::remove_all(base_);
    }

    // Put a few key-value pairs into the DB
    void seedData(int n = 5) {
        for (int i = 0; i < n; ++i) {
            ASSERT_TRUE(db_->put("key_" + std::to_string(i),
                                 std::string_view("val_" + std::to_string(i))));
        }
    }

    fs::path                         base_;
    fs::path                         db_dir_;
    std::shared_ptr<RocksDBWrapper>  db_;
    std::unique_ptr<BackupManager>   mgr_;
};

// ─────────────────────────────────────────────────────────────────────────────
// createSnapshot
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SnapshotTest, CreateSnapshot_ReturnsSnapshotPath) {
    seedData();
    auto r = mgr_->createSnapshot("test_snap");
    ASSERT_TRUE(r.has_value()) << r.error().message();
    EXPECT_FALSE(r->empty());
    EXPECT_TRUE(fs::exists(*r)) << "Snapshot directory should exist: " << *r;
}

TEST_F(SnapshotTest, CreateSnapshot_DirectoryContainsManifest) {
    seedData();
    auto r = mgr_->createSnapshot("with_manifest");
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(fs::exists(fs::path(*r) / "snapshot_manifest.json"))
        << "manifest should exist inside snapshot dir";
}

TEST_F(SnapshotTest, CreateSnapshot_DirectoryContainsRocksDBFiles) {
    seedData();
    auto r = mgr_->createSnapshot("has_rocksdb");
    ASSERT_TRUE(r.has_value());
    // A valid RocksDB checkpoint must contain a CURRENT file
    EXPECT_TRUE(fs::exists(fs::path(*r) / "CURRENT"))
        << "CURRENT file must exist in checkpoint";
}

TEST_F(SnapshotTest, CreateSnapshot_SequenceNumberInManifest) {
    seedData(10);
    auto r = mgr_->createSnapshot("seq_check");
    ASSERT_TRUE(r.has_value());

    std::ifstream mf(fs::path(*r) / "snapshot_manifest.json");
    ASSERT_TRUE(mf.is_open());
    nlohmann::json manifest;
    ASSERT_NO_THROW(manifest = nlohmann::json::parse(mf));

    EXPECT_TRUE(manifest.contains("sequence_number"));
    EXPECT_GT(manifest["sequence_number"].get<uint64_t>(), 0u)
        << "Sequence number should be > 0 after writes";
}

TEST_F(SnapshotTest, CreateSnapshot_NullDB_ReturnsError) {
    BackupManager bad(nullptr);
    auto r = bad.createSnapshot("should_fail");
    EXPECT_FALSE(r.has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// verifySnapshot
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SnapshotTest, VerifySnapshot_ValidSnapshot_OK) {
    seedData();
    auto snap = mgr_->createSnapshot("verify_ok");
    ASSERT_TRUE(snap.has_value());
    auto v = mgr_->verifySnapshot(*snap);
    EXPECT_TRUE(v.has_value()) << v.error().message();
}

TEST_F(SnapshotTest, VerifySnapshot_NonExistentDir_Error) {
    auto v = mgr_->verifySnapshot("/nonexistent/path_xyz");
    EXPECT_FALSE(v.has_value());
}

TEST_F(SnapshotTest, VerifySnapshot_MissingManifest_Error) {
    seedData();
    auto snap = mgr_->createSnapshot("no_manifest");
    ASSERT_TRUE(snap.has_value());

    // Remove the manifest
    fs::remove(fs::path(*snap) / "snapshot_manifest.json");

    auto v = mgr_->verifySnapshot(*snap);
    EXPECT_FALSE(v.has_value()) << "Should fail without manifest";
}

TEST_F(SnapshotTest, VerifySnapshot_CorruptManifest_Error) {
    seedData();
    auto snap = mgr_->createSnapshot("corrupt_manifest");
    ASSERT_TRUE(snap.has_value());

    // Overwrite manifest with garbage
    std::ofstream f(fs::path(*snap) / "snapshot_manifest.json");
    f << "this is not json {{{{";

    auto v = mgr_->verifySnapshot(*snap);
    EXPECT_FALSE(v.has_value()) << "Should fail with corrupt manifest";
}

// ─────────────────────────────────────────────────────────────────────────────
// listSnapshots
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SnapshotTest, ListSnapshots_Empty_ReturnsEmptyVector) {
    auto r = mgr_->listSnapshots();
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r->empty());
}

TEST_F(SnapshotTest, ListSnapshots_AfterCreate_ReturnsOne) {
    seedData();
    ASSERT_TRUE(mgr_->createSnapshot("ls_test_1").has_value());
    auto r = mgr_->listSnapshots();
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->size(), 1u);
}

TEST_F(SnapshotTest, ListSnapshots_MultipleSnapshots_ReturnsSorted) {
    seedData();
    ASSERT_TRUE(mgr_->createSnapshot("snap_a").has_value());
    ASSERT_TRUE(mgr_->createSnapshot("snap_b").has_value());
    ASSERT_TRUE(mgr_->createSnapshot("snap_c").has_value());

    auto r = mgr_->listSnapshots();
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->size(), 3u);
    // Verify the list is sorted and contains distinct paths
    EXPECT_TRUE(std::is_sorted(r->begin(), r->end()));
    // All paths must be unique
    for (size_t i = 0; i + 1 < r->size(); ++i) {
        EXPECT_NE((*r)[i], (*r)[i + 1]) << "Snapshot paths must be distinct";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// restoreFromSnapshot
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(SnapshotTest, RestoreFromSnapshot_DataSurvivesRoundTrip) {
    // Seed data and snapshot
    seedData(5);
    auto snap = mgr_->createSnapshot("restore_test");
    ASSERT_TRUE(snap.has_value());

    // Write more data AFTER the snapshot (should NOT appear after restore)
    for (int i = 100; i < 105; ++i) {
        db_->put("post_snap_" + std::to_string(i),
                 std::string_view("post_" + std::to_string(i)));
    }

    // Restore
    auto rv = mgr_->restoreFromSnapshot(*snap);
    ASSERT_TRUE(rv.has_value()) << rv.error().message();

    // DB is now re-opened at the snapshot point.
    // Keys written before snapshot should still exist.
    for (int i = 0; i < 5; ++i) {
        std::string val;
        EXPECT_TRUE(db_->get("key_" + std::to_string(i), val))
            << "key_" << i << " should exist after restore";
        EXPECT_EQ(val, "val_" + std::to_string(i));
    }

    // Keys written AFTER the snapshot should NOT exist
    for (int i = 100; i < 105; ++i) {
        std::string val;
        EXPECT_FALSE(db_->get("post_snap_" + std::to_string(i), val))
            << "post-snapshot key should not exist after restore";
    }
}

TEST_F(SnapshotTest, RestoreFromSnapshot_InvalidPath_ReturnsError) {
    auto rv = mgr_->restoreFromSnapshot("/nonexistent/snap");
    EXPECT_FALSE(rv.has_value());
}

TEST_F(SnapshotTest, RestoreFromSnapshot_EmptyPath_ReturnsError) {
    auto rv = mgr_->restoreFromSnapshot("");
    EXPECT_FALSE(rv.has_value());
}
