/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_lora_checkpoint_manager.cpp                   ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:55:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     367                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ce712594b0  2026-03-09  feat(training): Phase 3 enhancements - checkpoint manager... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
    std::string saved_sha256;

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
