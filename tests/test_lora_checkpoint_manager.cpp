// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_lora_checkpoint_manager.cpp
 * @brief Tests for LoRACheckpointManager (Phase 3)
 *
 * Covers:
 *  - Construction and config validation
 *  - save() with atomic write and SHA-256 computation
 *  - resume() returning latest valid checkpoint
 *  - Rolling window pruning (max_checkpoints enforcement)
 *  - validate() integrity check
 *  - Auto-rollback to previous checkpoint on corruption
 *  - clearAll() removes all checkpoints and manifest
 *  - Manifest persistence across manager instances
 *  - listCheckpoints() returns entries newest-first
 */

#include <gtest/gtest.h>
#include "training/lora_checkpoint_manager.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace themis::training;

// ============================================================================
// Helpers
// ============================================================================

static std::string writeTempFile(const std::string& dir,
                                 const std::string& name,
                                 const std::string& content) {
    std::string path = dir + "/" + name;
    std::ofstream f(path, std::ios::trunc);
    f << content;
    return path;
}

static std::string tempDir() {
    return "/tmp/themis_ckpt_test";
}

static void ensureDir(const std::string& path) {
    // Create the directory (best-effort via system call)
    std::string cmd = "mkdir -p " + path;
    (void)std::system(cmd.c_str());
}

// ============================================================================
// Fixture
// ============================================================================
class LoRACheckpointManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = tempDir() + "/" + std::to_string(test_counter_++);
        ensureDir(dir_);

        cfg_.checkpoint_dir  = dir_;
        cfg_.max_checkpoints = 3;
        cfg_.validate_on_load = true;
        cfg_.auto_rollback    = true;
    }

    void TearDown() override {
        std::string cmd = "rm -rf " + dir_;
        (void)std::system(cmd.c_str());
    }

    CheckpointManagerConfig cfg_;
    std::string dir_;
    static int test_counter_;
};

int LoRACheckpointManagerTest::test_counter_ = 0;

// ============================================================================
// Construction tests
// ============================================================================

TEST_F(LoRACheckpointManagerTest, Construction_ValidConfig_Succeeds) {
    EXPECT_NO_THROW(LoRACheckpointManager mgr(cfg_));
}

TEST_F(LoRACheckpointManagerTest, Construction_EmptyDir_Throws) {
    cfg_.checkpoint_dir = "";
    EXPECT_THROW(LoRACheckpointManager mgr(cfg_), std::invalid_argument);
}

TEST_F(LoRACheckpointManagerTest, Construction_ZeroMaxCheckpoints_Throws) {
    cfg_.max_checkpoints = 0;
    EXPECT_THROW(LoRACheckpointManager mgr(cfg_), std::invalid_argument);
}

TEST_F(LoRACheckpointManagerTest, ManifestPath_ReturnsCorrectPath) {
    LoRACheckpointManager mgr(cfg_);
    EXPECT_EQ(mgr.manifestPath(), dir_ + "/" + cfg_.manifest_filename);
}

// ============================================================================
// Save tests
// ============================================================================

TEST_F(LoRACheckpointManagerTest, Save_ValidFile_ReturnsEntryWithSHA256) {
    LoRACheckpointManager mgr(cfg_);
    std::string src = writeTempFile(dir_, "weights.bin", "dummy adapter weights data");

    CheckpointManifestEntry meta;
    meta.adapter_version = "legal_v1.0";
    meta.epoch = 1; meta.step = 100; meta.loss = 0.42;

    auto entry = mgr.save(src, meta);

    EXPECT_FALSE(entry.sha256.empty());
    EXPECT_EQ(entry.sha256.size(), 64u);  // SHA-256 hex = 64 chars
    EXPECT_FALSE(entry.checkpoint_path.empty());
    EXPECT_GT(entry.saved_at, 0);
}

TEST_F(LoRACheckpointManagerTest, Save_EmptySourcePath_Throws) {
    LoRACheckpointManager mgr(cfg_);
    CheckpointManifestEntry meta;
    EXPECT_THROW(mgr.save("", meta), std::invalid_argument);
}

TEST_F(LoRACheckpointManagerTest, Save_NonExistentSource_Throws) {
    LoRACheckpointManager mgr(cfg_);
    CheckpointManifestEntry meta;
    meta.adapter_version = "v1";
    EXPECT_THROW(mgr.save("/nonexistent/path/weights.bin", meta), std::runtime_error);
}

TEST_F(LoRACheckpointManagerTest, Save_CheckpointFileExists_AfterSave) {
    LoRACheckpointManager mgr(cfg_);
    std::string src = writeTempFile(dir_, "w.bin", "content");

    CheckpointManifestEntry meta;
    meta.adapter_version = "v1.0";
    auto entry = mgr.save(src, meta);

    std::ifstream f(entry.checkpoint_path);
    EXPECT_TRUE(f.is_open());
}

TEST_F(LoRACheckpointManagerTest, Save_ManifestFileWritten) {
    LoRACheckpointManager mgr(cfg_);
    std::string src = writeTempFile(dir_, "w.bin", "content");
    CheckpointManifestEntry meta;
    meta.adapter_version = "v1";
    mgr.save(src, meta);

    std::ifstream mf(mgr.manifestPath());
    EXPECT_TRUE(mf.is_open());
}

// ============================================================================
// Rolling window tests
// ============================================================================

TEST_F(LoRACheckpointManagerTest, Save_RollingWindow_PrunesOldest) {
    cfg_.max_checkpoints = 2;
    LoRACheckpointManager mgr(cfg_);

    for (int i = 0; i < 3; ++i) {
        std::string src = writeTempFile(dir_, "w" + std::to_string(i) + ".bin",
                                        "content " + std::to_string(i));
        CheckpointManifestEntry meta;
        meta.adapter_version = "v" + std::to_string(i);
        meta.epoch = static_cast<size_t>(i);
        mgr.save(src, meta);
    }

    auto list = mgr.listCheckpoints();
    EXPECT_EQ(list.size(), 2u);
}

TEST_F(LoRACheckpointManagerTest, ListCheckpoints_NewestFirst) {
    LoRACheckpointManager mgr(cfg_);

    for (int i = 0; i < 3; ++i) {
        std::string src = writeTempFile(dir_, "w" + std::to_string(i) + ".bin", "data");
        CheckpointManifestEntry meta;
        meta.adapter_version = "v" + std::to_string(i);
        meta.epoch = static_cast<size_t>(i);
        mgr.save(src, meta);
    }

    auto list = mgr.listCheckpoints();
    ASSERT_GE(list.size(), 2u);
    // Newest (epoch=2) should be first
    EXPECT_GE(list[0].epoch, list[1].epoch);
}

// ============================================================================
// Resume tests
// ============================================================================

TEST_F(LoRACheckpointManagerTest, Resume_NoCheckpoints_ReturnsNullopt) {
    LoRACheckpointManager mgr(cfg_);
    EXPECT_FALSE(mgr.resume().has_value());
}

TEST_F(LoRACheckpointManagerTest, Resume_AfterSave_ReturnsLatest) {
    LoRACheckpointManager mgr(cfg_);
    std::string src = writeTempFile(dir_, "w.bin", "data");
    CheckpointManifestEntry meta;
    meta.adapter_version = "v1";
    auto saved = mgr.save(src, meta);

    auto resumed = mgr.resume();
    ASSERT_TRUE(resumed.has_value());
    EXPECT_EQ(resumed->sha256, saved.sha256);
}

TEST_F(LoRACheckpointManagerTest, Resume_CorruptLatest_AutoRollback) {
    LoRACheckpointManager mgr(cfg_);

    // Save a valid checkpoint
    std::string src1 = writeTempFile(dir_, "w1.bin", "good content");
    CheckpointManifestEntry meta1;
    meta1.adapter_version = "v1"; meta1.epoch = 1;
    auto e1 = mgr.save(src1, meta1);

    // Save another valid checkpoint
    std::string src2 = writeTempFile(dir_, "w2.bin", "good content 2");
    CheckpointManifestEntry meta2;
    meta2.adapter_version = "v2"; meta2.epoch = 2;
    mgr.save(src2, meta2);

    // Corrupt the latest checkpoint file
    auto checkpoints = mgr.listCheckpoints();
    ASSERT_GE(checkpoints.size(), 1u);
    {
        std::ofstream corrupt(checkpoints[0].checkpoint_path, std::ios::trunc);
        corrupt << "CORRUPTED DATA!!!";
    }

    // Resume should fall back to the previous valid checkpoint
    auto resumed = mgr.resume();
    ASSERT_TRUE(resumed.has_value());
    // The resumed entry should be the older v1 checkpoint
    EXPECT_EQ(resumed->sha256, e1.sha256);
}

TEST_F(LoRACheckpointManagerTest, Resume_MissingChecksumInManifest_FailsClosed) {
    // Create a manifest with an entry that has an empty sha256 field.
    const std::string manifest_path = dir_ + "/" + cfg_.manifest_filename;
    {
        std::ofstream mf(manifest_path, std::ios::trunc);
        ASSERT_TRUE(mf.is_open());
        mf << "checkpoint_path=" << dir_ << "/orphan.ckpt\n";
        mf << "sha256=\n";
        mf << "base_model_hash=\n";
        mf << "adapter_version=v_missing_sha\n";
        mf << "epoch=1\n";
        mf << "step=10\n";
        mf << "loss=0.5\n";
        mf << "accuracy=0.8\n";
        mf << "saved_at=1700000000\n";
        mf << "---\n";
    }

    LoRACheckpointManager mgr(cfg_);
    auto resumed = mgr.resume();
    EXPECT_FALSE(resumed.has_value());
}

// ============================================================================
// Validate tests
// ============================================================================

TEST_F(LoRACheckpointManagerTest, Validate_ValidEntry_ReturnsTrue) {
    LoRACheckpointManager mgr(cfg_);
    std::string src = writeTempFile(dir_, "w.bin", "content");
    CheckpointManifestEntry meta;
    meta.adapter_version = "v1";
    auto entry = mgr.save(src, meta);

    EXPECT_TRUE(mgr.validate(entry));
}

TEST_F(LoRACheckpointManagerTest, Validate_CorruptFile_ReturnsFalse) {
    LoRACheckpointManager mgr(cfg_);
    std::string src = writeTempFile(dir_, "w.bin", "content");
    CheckpointManifestEntry meta;
    meta.adapter_version = "v1";
    auto entry = mgr.save(src, meta);

    // Corrupt the file
    {
        std::ofstream f(entry.checkpoint_path, std::ios::trunc);
        f << "corrupted";
    }
    EXPECT_FALSE(mgr.validate(entry));
}

TEST_F(LoRACheckpointManagerTest, Validate_EmptyPaths_ReturnsFalse) {
    LoRACheckpointManager mgr(cfg_);
    CheckpointManifestEntry entry;
    EXPECT_FALSE(mgr.validate(entry));
}

// ============================================================================
// ClearAll tests
// ============================================================================

TEST_F(LoRACheckpointManagerTest, ClearAll_RemovesAllEntries) {
    LoRACheckpointManager mgr(cfg_);
    std::string src = writeTempFile(dir_, "w.bin", "data");
    CheckpointManifestEntry meta;
    meta.adapter_version = "v1";
    mgr.save(src, meta);

    mgr.clearAll();

    EXPECT_TRUE(mgr.listCheckpoints().empty());
    EXPECT_FALSE(mgr.resume().has_value());
}

// ============================================================================
// Manifest persistence test
// ============================================================================

TEST_F(LoRACheckpointManagerTest, Manifest_PersistedAcrossInstances) {
    std::string src = writeTempFile(dir_, "w.bin", "data");
    CheckpointManifestEntry meta;
    meta.adapter_version = "v1";
    meta.epoch = 5;
    std::string saved_sha256 = {};

    {
        LoRACheckpointManager mgr(cfg_);
        auto entry = mgr.save(src, meta);
        saved_sha256 = entry.sha256;
    }

    // Create a new manager instance pointing to the same directory
    LoRACheckpointManager mgr2(cfg_);
    auto list = mgr2.listCheckpoints();
    ASSERT_FALSE(list.empty());
    EXPECT_EQ(list[0].sha256, saved_sha256);
    EXPECT_EQ(list[0].epoch,  5u);
}

// ============================================================================
// Struct default values
// ============================================================================

TEST_F(LoRACheckpointManagerTest, CheckpointManifestEntry_DefaultValues) {
    CheckpointManifestEntry entry;
    EXPECT_TRUE(entry.checkpoint_path.empty());
    EXPECT_TRUE(entry.sha256.empty());
    EXPECT_EQ(entry.epoch, 0u);
    EXPECT_EQ(entry.step, 0u);
    EXPECT_EQ(entry.loss, 0.0);
    EXPECT_EQ(entry.accuracy, 0.0);
}

TEST_F(LoRACheckpointManagerTest, CheckpointManagerConfig_DefaultValues) {
    CheckpointManagerConfig cfg;
    EXPECT_EQ(cfg.max_checkpoints, 3u);
    EXPECT_TRUE(cfg.validate_on_load);
    EXPECT_TRUE(cfg.auto_rollback);
    EXPECT_EQ(cfg.manifest_filename, "checkpoint_manifest.json");
}

// ============================================================================
// Manifest integrity validation (#5414 – model_integrity_gap fix)
// ============================================================================

// Saving a checkpoint produces a manifest entry that survives a reload via a
// fresh manager instance (baseline: existing behaviour still holds).
TEST_F(LoRACheckpointManagerTest, ManifestIntegrity_ValidEntry_LoadedCorrectly) {
    std::string src = writeTempFile(dir_, "v2.bin", "weights");
    CheckpointManifestEntry meta;
    meta.adapter_version = "v2";
    meta.epoch = 1;

    {
        LoRACheckpointManager mgr(cfg_);
        mgr.save(src, meta);
    }

    LoRACheckpointManager mgr2(cfg_);
    auto list = mgr2.listCheckpoints();
    ASSERT_FALSE(list.empty());
    EXPECT_EQ(list[0].adapter_version, "v2");
}

// A manifest file that contains a path-traversal in checkpoint_path must not
// produce any entries when loaded.
TEST_F(LoRACheckpointManagerTest, ManifestIntegrity_PathTraversal_EntryRejected) {
    // Write a manifest file directly that contains a traversal sequence.
    std::string manifest_path = dir_ + "/" + cfg_.manifest_filename;
    std::ofstream mf(manifest_path);
    mf << "checkpoint_path=../../etc/passwd\n"
       << "sha256=aabbccdd\n"
       << "adapter_version=evil\n"
       << "epoch=0\n"
       << "step=0\n"
       << "loss=0\n"
       << "accuracy=0\n"
       << "saved_at=0\n"
       << "---\n";
    mf.close();

    LoRACheckpointManager mgr(cfg_);
    EXPECT_TRUE(mgr.listCheckpoints().empty())
        << "path-traversal entry must be rejected by parseManifest";
}

// A manifest entry with a malformed SHA-256 (too short) must be dropped.
TEST_F(LoRACheckpointManagerTest, ManifestIntegrity_MalformedSha256_EntryRejected) {
    std::string manifest_path = dir_ + "/" + cfg_.manifest_filename;
    std::ofstream mf(manifest_path);
    // sha256 is only 8 chars instead of 64 — should be rejected.
    mf << "checkpoint_path=valid_checkpoint.bin\n"
       << "sha256=deadbeef\n"
       << "adapter_version=v3\n"
       << "epoch=0\n"
       << "step=0\n"
       << "loss=0\n"
       << "accuracy=0\n"
       << "saved_at=0\n"
       << "---\n";
    mf.close();

    LoRACheckpointManager mgr(cfg_);
    EXPECT_TRUE(mgr.listCheckpoints().empty())
        << "manifest entry with malformed SHA-256 must be rejected";
}

// A manifest entry with an empty sha256 field (not present) is acceptable.
TEST_F(LoRACheckpointManagerTest, ManifestIntegrity_EmptySha256_EntryAccepted) {
    std::string manifest_path = dir_ + "/" + cfg_.manifest_filename;
    // Create the dummy checkpoint file so validate() has something to read.
    writeTempFile(dir_, "ok.bin", "data");
    std::ofstream mf(manifest_path);
    mf << "checkpoint_path=" << dir_ << "/ok.bin\n"
       << "sha256=\n"
       << "adapter_version=v4\n"
       << "epoch=0\n"
       << "step=0\n"
       << "loss=0\n"
       << "accuracy=0\n"
       << "saved_at=0\n"
       << "---\n";
    mf.close();

    LoRACheckpointManager mgr(cfg_);
    auto list = mgr.listCheckpoints();
    EXPECT_FALSE(list.empty())
        << "entry with empty sha256 (not yet computed) must be accepted";
}
