#include <gtest/gtest.h>
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

class MergeEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_merge_engine_test";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.enable_wal = true;
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());

        auto* txn_db = db_->getRawDB();
        ASSERT_NE(txn_db, nullptr);

        changefeed_ = std::make_unique<Changefeed>(txn_db);
        snapshot_manager_ = std::make_unique<SnapshotManager>(*db_, *changefeed_);
        diff_engine_ = std::make_unique<DiffEngine>(*changefeed_);
        merge_engine_ = std::make_unique<MergeEngine>(*diff_engine_, *snapshot_manager_, *changefeed_);
    }
    
    void TearDown() override {
        merge_engine_.reset();
        diff_engine_.reset();
        snapshot_manager_.reset();
        changefeed_.reset();
        db_.reset();
        
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }
    
    // Helper: Record a PUT event
    uint64_t recordPut(const std::string& key, const std::string& value) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = key;
        event.value = value;
        auto now = std::chrono::system_clock::now();
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        auto recorded = changefeed_->recordEvent(event);
        return recorded.sequence;
    }
    
    // Helper: Record a DELETE event
    uint64_t recordDelete(const std::string& key) {
        Changefeed::ChangeEvent event;
        event.type = Changefeed::ChangeEventType::EVENT_DELETE;
        event.key = key;
        auto now = std::chrono::system_clock::now();
        event.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        
        auto recorded = changefeed_->recordEvent(event);
        return recorded.sequence;
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<SnapshotManager> snapshot_manager_;
    std::unique_ptr<DiffEngine> diff_engine_;
    std::unique_ptr<MergeEngine> merge_engine_;
};

// Test 1: Fast-forward merge (target has no changes)
TEST_F(MergeEngineTest, FastForwardMerge) {
    // Base state
    auto base_seq = recordPut("users:1", "Alice");
    recordPut("users:2", "Bob");
    
    // Source branch: Add new user
    auto source_seq = recordPut("users:3", "Charlie");
    
    // Target: No changes from base (still at base state)
    auto target_seq = base_seq;
    
    // Should be able to fast-forward
    EXPECT_TRUE(merge_engine_->canFastForward(base_seq, source_seq, target_seq));
    
    MergeEngine::MergeOptions options;
    options.dry_run = true; // Preview mode
    
    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.stats.is_fast_forward);
    EXPECT_EQ(result.stats.conflicts_detected, 0);
    EXPECT_EQ(result.changes_applied.size(), 1);
}

// Test 2: Merge with no conflicts (non-overlapping changes)
TEST_F(MergeEngineTest, MergeNoConflicts) {
    // Base state
    auto base_seq = recordPut("users:1", "Alice");
    
    // Source branch: Modify user:1
    auto source_seq = recordPut("users:1", "Alice Updated");
    
    // Target branch: Add user:2 (different key)
    auto target_seq = recordPut("users:2", "Bob");
    
    MergeEngine::MergeOptions options;
    options.dry_run = true;
    
    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.stats.conflicts_detected, 0);
    EXPECT_GT(result.changes_applied.size(), 0);
}

// Test 3: Detect MODIFY_MODIFY conflict
TEST_F(MergeEngineTest, DetectModifyModifyConflict) {
    // Base state
    auto base_seq = recordPut("users:1", "Alice");
    
    // Source branch: Modify user:1
    auto source_seq = recordPut("users:1", "Alice from Source");
    
    // Target branch: Also modify user:1 (conflict!)
    auto target_seq = recordPut("users:1", "Alice from Target");
    
    MergeEngine::MergeOptions options;
    options.dry_run = true;
    
    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);
    
    EXPECT_EQ(result.stats.conflicts_detected, 1);
    EXPECT_FALSE(result.stats.is_fast_forward);
    ASSERT_EQ(result.conflicts.size(), 1);
    
    const auto& conflict = result.conflicts[0];
    EXPECT_EQ(conflict.type, MergeEngine::ConflictType::MODIFY_MODIFY);
    EXPECT_EQ(conflict.key, "users:1");
    EXPECT_EQ(conflict.base_value, "Alice");
    EXPECT_EQ(conflict.source_value, "Alice from Source");
    EXPECT_EQ(conflict.target_value, "Alice from Target");
}

// Test 4: Detect DELETE_MODIFY conflict
TEST_F(MergeEngineTest, DetectDeleteModifyConflict) {
    // Base state
    auto base_seq = recordPut("users:1", "Alice");
    
    // Source branch: Delete user:1
    auto source_seq = recordDelete("users:1");
    
    // Target branch: Modify user:1 (conflict!)
    auto target_seq = recordPut("users:1", "Alice Updated");
    
    MergeEngine::MergeOptions options;
    options.dry_run = true;
    
    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);
    
    EXPECT_EQ(result.stats.conflicts_detected, 1);
    ASSERT_EQ(result.conflicts.size(), 1);
    
    const auto& conflict = result.conflicts[0];
    EXPECT_EQ(conflict.type, MergeEngine::ConflictType::DELETE_MODIFY);
    EXPECT_EQ(conflict.key, "users:1");
}

// Test 5: Auto-resolve DELETE_DELETE conflict
TEST_F(MergeEngineTest, AutoResolveDeleteDelete) {
    // Base state
    auto base_seq = recordPut("users:1", "Alice");
    
    // Source branch: Delete user:1
    recordDelete("users:1");
    
    // Target branch: Also delete user:1 (auto-resolvable)
    recordDelete("users:1");
    
    auto source_seq = changefeed_->getLatestSequence();
    auto target_seq = source_seq;
    
    MergeEngine::MergeOptions options;
    options.dry_run = true;
    
    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);
    
    // DELETE_DELETE is considered a conflict but auto-resolvable
    EXPECT_TRUE(result.success);
}

// Test 6: Merge with "ours" strategy
TEST_F(MergeEngineTest, MergeStrategyOurs) {
    // Base state
    auto base_seq = recordPut("users:1", "Alice");
    
    // Source branch
    auto source_seq = recordPut("users:1", "Alice from Source");
    
    // Target branch
    auto target_seq = recordPut("users:1", "Alice from Target");
    
    MergeEngine::MergeOptions options;
    options.strategy = MergeEngine::MergeStrategy::OURS;
    options.dry_run = true;
    
    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.changes_applied.size(), 0);
    
    // Should prefer target (ours)
    bool found_target_value = false;
    for (const auto& change : result.changes_applied) {
        if (change.key == "users:1" && change.new_value == "Alice from Target") {
            found_target_value = true;
            break;
        }
    }
    EXPECT_TRUE(found_target_value);
}

// Test 7: Merge with "theirs" strategy
TEST_F(MergeEngineTest, MergeStrategyTheirs) {
    // Base state
    auto base_seq = recordPut("users:1", "Alice");
    
    // Source branch
    auto source_seq = recordPut("users:1", "Alice from Source");
    
    // Target branch
    auto target_seq = recordPut("users:1", "Alice from Target");
    
    MergeEngine::MergeOptions options;
    options.strategy = MergeEngine::MergeStrategy::THEIRS;
    options.dry_run = true;
    
    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.changes_applied.size(), 0);
    
    // Should prefer source (theirs)
    bool found_source_value = false;
    for (const auto& change : result.changes_applied) {
        if (change.key == "users:1" && change.new_value == "Alice from Source") {
            found_source_value = true;
            break;
        }
    }
    EXPECT_TRUE(found_source_value);
}

// Test 8: Manual conflict resolution
TEST_F(MergeEngineTest, ManualConflictResolution) {
    // Base state
    auto base_seq = recordPut("users:1", "Alice");
    
    // Source branch
    auto source_seq = recordPut("users:1", "Alice from Source");
    
    // Target branch
    auto target_seq = recordPut("users:1", "Alice from Target");
    
    MergeEngine::MergeOptions options;
    options.strategy = MergeEngine::MergeStrategy::MANUAL;
    options.dry_run = true;
    
    // Provide manual resolution
    MergeEngine::ConflictResolution resolution;
    resolution.key = "users:1";
    resolution.resolved_value = "Alice Manually Resolved";
    options.manual_resolutions.push_back(resolution);
    
    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);
    
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.changes_applied.size(), 0);
    
    // Should use manual resolution
    bool found_manual_value = false;
    for (const auto& change : result.changes_applied) {
        if (change.key == "users:1" && change.new_value == "Alice Manually Resolved") {
            found_manual_value = true;
            break;
        }
    }
    EXPECT_TRUE(found_manual_value);
}

// Test 9: Fast-forward strategy fails on conflict
TEST_F(MergeEngineTest, FastForwardStrategyFailsOnConflict) {
    // Base state
    auto base_seq = recordPut("users:1", "Alice");
    
    // Source branch
    auto source_seq = recordPut("users:1", "Alice from Source");
    
    // Target branch (conflict)
    auto target_seq = recordPut("users:1", "Alice from Target");
    
    MergeEngine::MergeOptions options;
    options.strategy = MergeEngine::MergeStrategy::FAST_FORWARD;
    options.fail_on_conflict = true;
    
    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);
    
    EXPECT_FALSE(result.success);
    EXPECT_GT(result.stats.conflicts_detected, 0);
}

// Test 10: Merge by tag
TEST_F(MergeEngineTest, MergeByTag) {
    // Create base state and tag it
    auto base_seq = recordPut("users:1", "Alice");
    auto base_tag = snapshot_manager_->createTag("base", "Base state", "test");
    ASSERT_TRUE(base_tag.has_value());
    
    // Create source branch and tag it
    auto source_seq = recordPut("users:2", "Bob");
    auto source_tag = snapshot_manager_->createTag("source", "Source branch", "test");
    ASSERT_TRUE(source_tag.has_value());
    
    // Target is current
    
    MergeEngine::MergeOptions options;
    options.dry_run = true;
    
    auto result = merge_engine_->mergeByTag("base", "source", "current", options);
    
    EXPECT_TRUE(result.success);
}

// Test 11: Preview merge
TEST_F(MergeEngineTest, PreviewMerge) {
    auto base_seq = recordPut("users:1", "Alice");
    auto source_seq = recordPut("users:1", "Alice Updated");
    auto target_seq = recordPut("users:2", "Bob");
    
    auto result = merge_engine_->previewMerge(base_seq, source_seq, target_seq);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.result_sequence, target_seq); // No changes actually applied
}

// Test 12: Complex merge with multiple keys
TEST_F(MergeEngineTest, ComplexMergeMultipleKeys) {
    // Base state: 3 users
    auto base_seq = recordPut("users:1", "Alice");
    recordPut("users:2", "Bob");
    recordPut("users:3", "Charlie");
    
    // Source branch: Modify user:1, add user:4
    recordPut("users:1", "Alice from Source");
    auto source_seq = recordPut("users:4", "David");
    
    // Target branch: Modify user:2, delete user:3
    recordPut("users:2", "Bob Updated");
    auto target_seq = recordDelete("users:3");
    
    MergeEngine::MergeOptions options;
    options.strategy = MergeEngine::MergeStrategy::THEIRS;
    options.dry_run = true;
    
    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.stats.conflicts_detected, 0); // No overlapping keys
    EXPECT_GT(result.changes_applied.size(), 0);
}
// Test 13: Detect ADD_ADD conflict (same key added on both branches)
TEST_F(MergeEngineTest, DetectAddAddConflict) {
    // Base state: no key exists for users:5
    auto base_seq = recordPut("users:0", "base");

    // Source branch: add users:5
    auto source_seq = recordPut("users:5", "Eve from Source");

    // Target branch: also add users:5 with a different value (conflict!)
    auto target_seq = recordPut("users:5", "Eve from Target");

    MergeEngine::MergeOptions options;
    options.dry_run = true;

    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);

    EXPECT_EQ(result.stats.conflicts_detected, 1);
    ASSERT_EQ(result.conflicts.size(), 1);
    EXPECT_EQ(result.conflicts[0].key, "users:5");
}

// Test 14: Merge with DELETE_DELETE on same key (auto-resolvable)
TEST_F(MergeEngineTest, AutoResolveModifyModifyWithSameValue) {
    auto base_seq = recordPut("cfg:1", "v1");

    // Both branches apply the same value (idempotent) - should auto-resolve
    recordPut("cfg:1", "v2");
    auto source_seq = changefeed_->getLatestSequence();

    recordPut("cfg:1", "v2"); // same value as source
    auto target_seq = changefeed_->getLatestSequence();

    MergeEngine::MergeOptions options;
    options.strategy = MergeEngine::MergeStrategy::THEIRS;
    options.dry_run = true;

    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);

    EXPECT_TRUE(result.success);
}

// Test 15: Merge stats are populated correctly
TEST_F(MergeEngineTest, MergeStatsPopulated) {
    auto base_seq = recordPut("stat:1", "a");

    auto source_seq = recordPut("stat:1", "b");
    auto target_seq = recordPut("stat:2", "c");

    MergeEngine::MergeOptions options;
    options.strategy = MergeEngine::MergeStrategy::THEIRS;
    options.dry_run = true;

    auto result = merge_engine_->merge(base_seq, source_seq, target_seq, options);

    EXPECT_TRUE(result.success);
    // At least one change should have been applied or detected
    EXPECT_GE(result.stats.changes_applied + result.stats.conflicts_detected, 1u);
}
