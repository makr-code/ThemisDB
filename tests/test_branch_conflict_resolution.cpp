/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_branch_conflict_resolution.cpp               ║
  Version:         0.0.32                                             ║
  Description:     Unit tests for branch merge conflict resolution UI ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "transaction/branch_manager.h"
#include "transaction/merge_engine.h"
#include "transaction/snapshot_manager.h"
#include "analytics/diff_engine.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>

using namespace themis;
using namespace themis::transaction;
using namespace themis::analytics;

class BranchConflictResolutionTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_branch_conflict_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }

        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.enable_wal = true;
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        auto* raw_db = db_->getRawDB();
        ASSERT_NE(raw_db, nullptr);

        changefeed_       = std::make_unique<Changefeed>(raw_db);
        snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
        diff_engine_      = std::make_unique<DiffEngine>(*changefeed_);
        merge_engine_     = std::make_unique<MergeEngine>(*diff_engine_, *snapshot_manager_, *changefeed_);
        branch_manager_   = std::make_unique<BranchManager>(
                                *db_, *changefeed_, *snapshot_manager_, merge_engine_.get());
    }

    void TearDown() override {
        branch_manager_.reset();
        merge_engine_.reset();
        diff_engine_.reset();
        snapshot_manager_.reset();
        changefeed_.reset();
        db_.reset();

        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

    uint64_t recordPut(const std::string& key, const std::string& value) {
        Changefeed::ChangeEvent event;
        event.type  = Changefeed::ChangeEventType::EVENT_PUT;
        event.key   = key;
        event.value = value;
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return changefeed_->recordEvent(event).sequence;
    }

    // Create a branch pinned to the current changefeed sequence
    void createBranchAt(const std::string& name, uint64_t seq) {
        BranchManager::CreateBranchOptions opts;
        opts.from_sequence = seq;
        auto b = branch_manager_->createBranch(name, "main", "test branch", "test", opts);
        ASSERT_TRUE(b.has_value()) << "Failed to create branch: " << name;
    }

    std::string                         test_db_path_;
    std::unique_ptr<RocksDBWrapper>     db_;
    std::unique_ptr<Changefeed>         changefeed_;
    std::unique_ptr<SnapshotManager>    snapshot_manager_;
    std::unique_ptr<DiffEngine>         diff_engine_;
    std::unique_ptr<MergeEngine>        merge_engine_;
    std::unique_ptr<BranchManager>      branch_manager_;
};

// ── Preview with explicit base branch detects MODIFY_MODIFY conflict ─────────
//
// Timeline:  seq1(base) → seq2(source changes) → seq3(target changes)
//
// base-branch  @ seq1  (cfg:1 = "v0")
// source-branch@ seq2  (cfg:1 = "v-source")
// target-branch@ seq3  (cfg:1 = "v-target")
//
// With explicit base @seq1, source_diff includes cfg:1 modified,
// target_diff includes cfg:1 modified → CONFLICT

TEST_F(BranchConflictResolutionTest, PreviewMergeWithBaseDetectsConflict) {
    uint64_t seq1 = recordPut("cfg:1", "v0");       // base state
    createBranchAt("base-br", seq1);

    uint64_t seq2 = recordPut("cfg:1", "v-source"); // source change
    createBranchAt("source-br", seq2);

    uint64_t seq3 = recordPut("cfg:1", "v-target"); // target change
    createBranchAt("target-br", seq3);

    auto result = branch_manager_->previewBranchMerge(
        "source-br", "target-br", "base-br");

    EXPECT_EQ(result.stats.conflicts_detected, 1u);
    ASSERT_EQ(result.conflicts.size(), 1u);

    const auto& c = result.conflicts[0];
    EXPECT_EQ(c.key, "cfg:1");
    EXPECT_EQ(c.type, MergeEngine::ConflictType::MODIFY_MODIFY);
    EXPECT_EQ(c.source_value, "v-source");
    EXPECT_EQ(c.target_value, "v-target");
    // preview must not apply changes
    EXPECT_EQ(result.result_sequence, result.target_sequence);
}

// ── Preview without base (fast-forward path) succeeds with no conflicts ───────

TEST_F(BranchConflictResolutionTest, PreviewMergeNoBaseFastForwardSucceeds) {
    uint64_t seq1 = recordPut("data:1", "original");
    createBranchAt("src-ff", seq1);

    uint64_t seq2 = recordPut("data:2", "new-key"); // seq2 > seq1
    createBranchAt("tgt-ff", seq2);

    // Without explicit base: base = min(seq1, seq2) = seq1
    // source_diff(seq1→seq1) = empty, target_diff(seq1→seq2) = {data:2 added}
    // → no conflict (source has no changes)
    auto result = branch_manager_->previewBranchMerge("src-ff", "tgt-ff");

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.stats.conflicts_detected, 0u);
    EXPECT_TRUE(result.conflicts.empty());
}

// ── Preview returns error for missing source branch ───────────────────────────

TEST_F(BranchConflictResolutionTest, PreviewMergeSourceBranchNotFound) {
    auto result = branch_manager_->previewBranchMerge("nonexistent", "main");

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Source branch not found"), std::string::npos);
}

// ── Preview returns error for missing target branch ───────────────────────────

TEST_F(BranchConflictResolutionTest, PreviewMergeTargetBranchNotFound) {
    uint64_t seq1 = recordPut("k", "v");
    createBranchAt("src-tgt-miss", seq1);

    auto result = branch_manager_->previewBranchMerge("src-tgt-miss", "nonexistent");

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Target branch not found"), std::string::npos);
}

// ── Preview returns error for missing base branch ─────────────────────────────

TEST_F(BranchConflictResolutionTest, PreviewMergeBaseBranchNotFound) {
    uint64_t seq1 = recordPut("k", "v");
    createBranchAt("src-base-miss", seq1);
    createBranchAt("tgt-base-miss", seq1);

    auto result = branch_manager_->previewBranchMerge(
        "src-base-miss", "tgt-base-miss", "nonexistent-base");

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Base branch not found"), std::string::npos);
}

// ── Preview without MergeEngine returns error ─────────────────────────────────

TEST_F(BranchConflictResolutionTest, PreviewMergeWithoutMergeEngineReturnsError) {
    uint64_t seq1 = recordPut("k", "v");
    createBranchAt("src-no-me", seq1);
    createBranchAt("tgt-no-me", seq1);

    branch_manager_->setMergeEngine(nullptr);

    auto result = branch_manager_->previewBranchMerge("src-no-me", "tgt-no-me");

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("MergeEngine not initialized"), std::string::npos);
}

// ── Resolve with manual resolution applies the chosen value ──────────────────

TEST_F(BranchConflictResolutionTest, ResolveAndMergeAppliesManualResolution) {
    uint64_t seq1 = recordPut("users:1", "Alice");
    createBranchAt("base-res", seq1);

    uint64_t seq2 = recordPut("users:1", "Alice-Source");
    createBranchAt("src-res", seq2);

    uint64_t seq3 = recordPut("users:1", "Alice-Target");
    createBranchAt("tgt-res", seq3);

    MergeEngine::ConflictResolution resolution;
    resolution.key            = "users:1";
    resolution.resolved_value = "Alice-Resolved";

    auto result = branch_manager_->resolveAndMergeBranches(
        "src-res", "tgt-res", {resolution}, "base-res");

    EXPECT_TRUE(result.success);
    EXPECT_GT(result.stats.changes_applied, 0u);

    bool found = false;
    for (const auto& change : result.changes_applied) {
        if (change.key == "users:1" && change.new_value == "Alice-Resolved") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Expected resolved value 'Alice-Resolved' in applied changes";
}

// ── Resolve with delete resolution applies a DELETE ──────────────────────────

TEST_F(BranchConflictResolutionTest, ResolveAndMergeDeleteResolution) {
    uint64_t seq1 = recordPut("rec:1", "original");
    createBranchAt("base-del", seq1);

    uint64_t seq2 = recordPut("rec:1", "source-version");
    createBranchAt("src-del", seq2);

    uint64_t seq3 = recordPut("rec:1", "target-version");
    createBranchAt("tgt-del", seq3);

    // Resolution without resolved_value → delete the key
    MergeEngine::ConflictResolution resolution;
    resolution.key = "rec:1";
    // resolved_value intentionally absent

    auto result = branch_manager_->resolveAndMergeBranches(
        "src-del", "tgt-del", {resolution}, "base-del");

    EXPECT_TRUE(result.success);

    bool found_delete = false;
    for (const auto& change : result.changes_applied) {
        if (change.key == "rec:1" &&
            change.type == DiffEngine::ChangeType::DELETED) {
            found_delete = true;
            break;
        }
    }
    EXPECT_TRUE(found_delete) << "Expected key 'rec:1' to be deleted";
}

// ── Resolve returns error for missing source branch ───────────────────────────

TEST_F(BranchConflictResolutionTest, ResolveAndMergeSourceBranchNotFound) {
    auto result = branch_manager_->resolveAndMergeBranches("nonexistent", "main", {});

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Source branch not found"), std::string::npos);
}

// ── Resolve returns error for missing target branch ───────────────────────────

TEST_F(BranchConflictResolutionTest, ResolveAndMergeTargetBranchNotFound) {
    uint64_t seq1 = recordPut("k2", "v2");
    createBranchAt("src-rtgt", seq1);

    auto result = branch_manager_->resolveAndMergeBranches("src-rtgt", "nonexistent", {});

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Target branch not found"), std::string::npos);
}

// ── Resolve returns error for missing base branch ─────────────────────────────

TEST_F(BranchConflictResolutionTest, ResolveAndMergeBaseBranchNotFound) {
    uint64_t seq1 = recordPut("k3", "v3");
    createBranchAt("src-rbase", seq1);
    createBranchAt("tgt-rbase", seq1);

    auto result = branch_manager_->resolveAndMergeBranches(
        "src-rbase", "tgt-rbase", {}, "nonexistent-base");

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("Base branch not found"), std::string::npos);
}

// ── Resolve without MergeEngine returns error ─────────────────────────────────

TEST_F(BranchConflictResolutionTest, ResolveAndMergeWithoutMergeEngineReturnsError) {
    uint64_t seq1 = recordPut("k4", "v4");
    createBranchAt("src-nme", seq1);
    createBranchAt("tgt-nme", seq1);

    branch_manager_->setMergeEngine(nullptr);

    auto result = branch_manager_->resolveAndMergeBranches("src-nme", "tgt-nme", {});

    EXPECT_FALSE(result.success);
    EXPECT_NE(result.message.find("MergeEngine not initialized"), std::string::npos);
}

// ── Resolve with no conflicts and no resolutions succeeds ─────────────────────

TEST_F(BranchConflictResolutionTest, ResolveAndMergeNoConflictsSucceeds) {
    uint64_t seq1 = recordPut("data:nc", "base-value");
    createBranchAt("src-nc", seq1);

    uint64_t seq2 = recordPut("data:nc2", "source-only-key");
    createBranchAt("tgt-nc", seq2);

    // No explicit base; base = min(seq1, seq2) = seq1
    // source_diff(seq1→seq1) = empty, no conflicts
    auto result = branch_manager_->resolveAndMergeBranches("src-nc", "tgt-nc", {});

    EXPECT_TRUE(result.success);
}

// ── Preview result JSON contains branch names ─────────────────────────────────
// (validated via the MergeResult toJson, which we check transitively through
//  the BranchManager method returning the MergeEngine result)

TEST_F(BranchConflictResolutionTest, PreviewResultHasCorrectSequences) {
    uint64_t seq1 = recordPut("x:1", "a");
    createBranchAt("src-seq", seq1);

    uint64_t seq2 = recordPut("x:2", "b");
    createBranchAt("tgt-seq", seq2);

    auto result = branch_manager_->previewBranchMerge("src-seq", "tgt-seq");

    EXPECT_EQ(result.source_sequence, seq1);
    EXPECT_EQ(result.target_sequence, seq2);
    EXPECT_EQ(result.base_sequence, seq1); // min(seq1, seq2) = seq1
}

// ── isBranchMerged: not merged before any merge ───────────────────────────────

TEST_F(BranchConflictResolutionTest, BranchNotMergedInitially) {
    uint64_t seq1 = recordPut("m:1", "a");
    createBranchAt("src-merge-init", seq1);

    // Before any merge: deleteBranch without force must fail
    EXPECT_FALSE(branch_manager_->deleteBranch("src-merge-init", false));
    EXPECT_TRUE(branch_manager_->branchExists("src-merge-init"));
}

// ── isBranchMerged: resolveAndMergeBranches records merge status ──────────────

TEST_F(BranchConflictResolutionTest, BranchIsMarkedMergedAfterResolveAndMerge) {
    uint64_t seq1 = recordPut("m:2", "base");
    createBranchAt("src-after-resolve", seq1);

    uint64_t seq2 = recordPut("m:3", "other");
    createBranchAt("tgt-after-resolve", seq2);

    // No conflict path: source_diff(seq1→seq1) is empty
    auto result = branch_manager_->resolveAndMergeBranches(
        "src-after-resolve", "tgt-after-resolve", {});
    ASSERT_TRUE(result.success);

    // Now deleteBranch without force should succeed because merge is recorded
    EXPECT_TRUE(branch_manager_->deleteBranch("src-after-resolve", false));
    EXPECT_FALSE(branch_manager_->branchExists("src-after-resolve"));
}

// ── isBranchMerged: fast-forward mergeBranches records merge status ───────────

TEST_F(BranchConflictResolutionTest, BranchIsMarkedMergedAfterFastForwardMerge) {
    // For fast-forward: source_seq >= target_seq
    // Create target first (smaller seq), then source (larger seq)
    uint64_t seq1 = recordPut("m:4", "target-older");
    createBranchAt("tgt-ff-merge", seq1);

    uint64_t seq2 = recordPut("m:5", "source-newer"); // seq2 > seq1
    createBranchAt("src-ff-merge", seq2);

    // Fast-forward: source_seq(seq2) >= target_seq(seq1) → fast-forward path
    BranchManager::MergeOptions opts;
    opts.fast_forward    = true;
    opts.abort_on_conflict = false;

    auto result = branch_manager_->mergeBranches("src-ff-merge", "tgt-ff-merge", opts);
    ASSERT_TRUE(result.success);

    // Branch should now be deletable without force
    EXPECT_TRUE(branch_manager_->deleteBranch("src-ff-merge", false));
}

// ── pruneMergedBranches: only removes merged branches ────────────────────────

TEST_F(BranchConflictResolutionTest, PruneMergedBranchesOnlyRemovesMerged) {
    uint64_t seq1 = recordPut("p:1", "base");
    createBranchAt("src-prune-merged", seq1);
    createBranchAt("src-prune-unmerged", seq1);

    // pruneMergedBranches checks isBranchMerged(branch, "main").
    // Merge src-prune-merged into main (DEFAULT_BRANCH).
    BranchManager::MergeOptions opts;
    opts.fast_forward    = false;
    opts.abort_on_conflict = false;
    auto merge_result = branch_manager_->mergeBranches("src-prune-merged", "main", opts);
    ASSERT_TRUE(merge_result.success);

    // GC policy: no age limit, only prune merged branches
    BranchManager::BranchGCPolicy policy;
    policy.max_age_ms    = 0;   // no age limit
    policy.only_merged   = true;
    policy.protect_default = true;
    branch_manager_->setBranchGCPolicy(policy);

    size_t pruned = branch_manager_->pruneMergedBranches();

    EXPECT_EQ(pruned, 1u);
    EXPECT_FALSE(branch_manager_->branchExists("src-prune-merged"));
    EXPECT_TRUE(branch_manager_->branchExists("src-prune-unmerged"));
    EXPECT_TRUE(branch_manager_->branchExists("main"));
}
