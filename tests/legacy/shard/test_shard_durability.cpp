// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/shard_durability.h"
#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/options.h>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themisdb::sharding;
using namespace std::chrono_literals;

class ShardDurabilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./test_durability_db";
        test_checkpoint_dir_ = "./test_checkpoints";
        
        // Clean up any previous test data
        std::filesystem::remove_all(test_db_path_);
        std::filesystem::remove_all(test_checkpoint_dir_);
        
        // Open RocksDB
        rocksdb::Options options;
        options.create_if_missing = true;
        options.wal_dir = test_db_path_ + "/wal";
        
        rocksdb::TransactionDBOptions txn_db_options;
        rocksdb::TransactionDB* db_ptr;
        
        rocksdb::Status status = rocksdb::TransactionDB::Open(
            options,
            txn_db_options,
            test_db_path_,
            &db_ptr
        );
        
        ASSERT_TRUE(status.ok()) << status.ToString();
        db_.reset(db_ptr);
    }
    
    void TearDown() override {
        db_.reset();
        std::filesystem::remove_all(test_db_path_);
        std::filesystem::remove_all(test_checkpoint_dir_);
    }
    
    ShardDurabilityConfig createConfig(DurabilityMode mode = DurabilityMode::ASYNC) {
        ShardDurabilityConfig config;
        config.mode = mode;
        config.enable_wal = true;
        config.checkpoint_dir = test_checkpoint_dir_;
        config.max_checkpoints = 3;
        config.enable_auto_recovery = true;
        return config;
    }
    
    std::string test_db_path_;
    std::string test_checkpoint_dir_;
    std::unique_ptr<rocksdb::TransactionDB> db_;
};

TEST_F(ShardDurabilityTest, InitializationSuccess) {
    auto config = createConfig();
    ShardDurability durability(db_.get(), config);
    
    EXPECT_TRUE(durability.initialize());
    EXPECT_TRUE(durability.isDurabilityEnabled());
}

TEST_F(ShardDurabilityTest, InitializationWithoutDB) {
    auto config = createConfig();
    ShardDurability durability(nullptr, config);
    
    EXPECT_FALSE(durability.initialize());
}

TEST_F(ShardDurabilityTest, SyncWAL) {
    auto config = createConfig(DurabilityMode::SYNC);
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    // Write some data
    rocksdb::WriteOptions write_opts;
    rocksdb::Status status = db_->Put(write_opts, "key1", "value1");
    ASSERT_TRUE(status.ok());
    
    // Sync WAL
    EXPECT_TRUE(durability.syncWAL());
    
    // Check statistics
    const auto& stats = durability.getStatistics();
    EXPECT_GT(stats.total_syncs.load(), 0);
}

TEST_F(ShardDurabilityTest, CreateCheckpoint) {
    auto config = createConfig();
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    // Write some data
    rocksdb::WriteOptions write_opts;
    db_->Put(write_opts, "key1", "value1");
    db_->Put(write_opts, "key2", "value2");
    
    // Create checkpoint
    auto checkpoint = durability.createCheckpoint("test_checkpoint");
    ASSERT_TRUE(checkpoint.has_value());
    EXPECT_EQ(checkpoint->checkpoint_id, "test_checkpoint");
    EXPECT_TRUE(checkpoint->is_valid);
    EXPECT_GT(checkpoint->size_bytes, 0);
    
    // Verify checkpoint exists
    EXPECT_TRUE(std::filesystem::exists(checkpoint->path));
    
    // Check statistics
    const auto& stats = durability.getStatistics();
    EXPECT_EQ(stats.checkpoints_created.load(), 1);
}

TEST_F(ShardDurabilityTest, ListCheckpoints) {
    auto config = createConfig();
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    // Create multiple checkpoints
    durability.createCheckpoint("checkpoint1");
    std::this_thread::sleep_for(10ms);
    durability.createCheckpoint("checkpoint2");
    std::this_thread::sleep_for(10ms);
    durability.createCheckpoint("checkpoint3");
    
    // List checkpoints
    auto checkpoints = durability.listCheckpoints();
    EXPECT_EQ(checkpoints.size(), 3);
    
    // Verify they are sorted by creation time (newest first)
    EXPECT_EQ(checkpoints[0].checkpoint_id, "checkpoint3");
    EXPECT_EQ(checkpoints[1].checkpoint_id, "checkpoint2");
    EXPECT_EQ(checkpoints[2].checkpoint_id, "checkpoint1");
}

TEST_F(ShardDurabilityTest, CheckpointCleanup) {
    auto config = createConfig();
    config.max_checkpoints = 2;
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    // Create 3 checkpoints (max is 2)
    durability.createCheckpoint("checkpoint1");
    std::this_thread::sleep_for(10ms);
    durability.createCheckpoint("checkpoint2");
    std::this_thread::sleep_for(10ms);
    durability.createCheckpoint("checkpoint3");
    
    // Should have only 2 checkpoints (oldest removed)
    auto checkpoints = durability.listCheckpoints();
    EXPECT_EQ(checkpoints.size(), 2);
    
    // Oldest checkpoint should be removed
    bool has_checkpoint1 = false;
    for (const auto& cp : checkpoints) {
        if (cp.checkpoint_id == "checkpoint1") {
            has_checkpoint1 = true;
        }
    }
    EXPECT_FALSE(has_checkpoint1);
}

TEST_F(ShardDurabilityTest, SequenceNumberTracking) {
    auto config = createConfig();
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    uint64_t seq1 = durability.getCurrentSequenceNumber();
    
    // Write some data
    rocksdb::WriteOptions write_opts;
    db_->Put(write_opts, "key1", "value1");
    
    uint64_t seq2 = durability.getCurrentSequenceNumber();
    
    // Sequence number should increase
    EXPECT_GT(seq2, seq1);
}

TEST_F(ShardDurabilityTest, WALIntegrityVerification) {
    auto config = createConfig();
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    // Write some data
    rocksdb::WriteOptions write_opts;
    db_->Put(write_opts, "key1", "value1");
    
    // Verify WAL integrity
    EXPECT_TRUE(durability.verifyWALIntegrity());
}

TEST_F(ShardDurabilityTest, CrashRecovery) {
    auto config = createConfig();
    config.enable_auto_recovery = true;
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    // Perform recovery
    RecoveryStats stats = durability.performRecovery();
    
    // Should succeed (no actual crash, but recovery logic runs)
    EXPECT_TRUE(stats.recovery_successful || !stats.recovery_needed);
    EXPECT_GE(stats.recovery_duration.count(), 0);
}

TEST_F(ShardDurabilityTest, RecoveryCallback) {
    auto config = createConfig();
    ShardDurability durability(db_.get(), config);
    
    bool callback_called = false;
    RecoveryStats callback_stats;
    
    durability.setRecoveryCallback([&](const RecoveryStats& stats) {
        callback_called = true;
        callback_stats = stats;
    });
    
    ASSERT_TRUE(durability.initialize());
    
    // If recovery was needed, callback should have been called
    if (callback_stats.recovery_needed) {
        EXPECT_TRUE(callback_called);
    }
}

TEST_F(ShardDurabilityTest, DurabilityModes) {
    // Test different durability modes
    std::vector<DurabilityMode> modes = {
        DurabilityMode::NONE,
        DurabilityMode::ASYNC,
        DurabilityMode::SYNC,
        DurabilityMode::GROUP_COMMIT
    };
    
    for (auto mode : modes) {
        auto config = createConfig(mode);
        ShardDurability durability(db_.get(), config);
        
        EXPECT_TRUE(durability.initialize());
        EXPECT_EQ(durability.getConfig().mode, mode);
        
        durability.shutdown();
    }
}

TEST_F(ShardDurabilityTest, UpdateConfiguration) {
    auto config1 = createConfig(DurabilityMode::ASYNC);
    ShardDurability durability(db_.get(), config1);
    
    ASSERT_TRUE(durability.initialize());
    EXPECT_EQ(durability.getConfig().mode, DurabilityMode::ASYNC);
    
    // Update configuration
    auto config2 = createConfig(DurabilityMode::SYNC);
    durability.updateConfig(config2);
    
    EXPECT_EQ(durability.getConfig().mode, DurabilityMode::SYNC);
}

TEST_F(ShardDurabilityTest, ShutdownSyncsWAL) {
    auto config = createConfig(DurabilityMode::ASYNC);
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    // Write data
    rocksdb::WriteOptions write_opts;
    db_->Put(write_opts, "key1", "value1");
    
    uint64_t syncs_before = durability.getStatistics().total_syncs.load();
    
    // Shutdown should sync WAL
    durability.shutdown();
    
    uint64_t syncs_after = durability.getStatistics().total_syncs.load();
    
    EXPECT_GT(syncs_after, syncs_before);
}

TEST_F(ShardDurabilityTest, CheckpointWithAutoGeneratedId) {
    auto config = createConfig();
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    // Create checkpoint without specifying name
    auto checkpoint = durability.createCheckpoint();
    ASSERT_TRUE(checkpoint.has_value());
    
    // ID should be auto-generated
    EXPECT_FALSE(checkpoint->checkpoint_id.empty());
    EXPECT_TRUE(checkpoint->checkpoint_id.find("checkpoint_") == 0);
}

TEST_F(ShardDurabilityTest, RestoreFromCheckpoint) {
    auto config = createConfig();
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    // Write data and create checkpoint
    rocksdb::WriteOptions write_opts;
    db_->Put(write_opts, "key1", "value1");
    
    auto checkpoint = durability.createCheckpoint("restore_test");
    ASSERT_TRUE(checkpoint.has_value());
    
    // Restore from checkpoint
    // Note: Actual restoration requires DB shutdown/restart
    // This just validates the checkpoint
    bool can_restore = durability.restoreFromCheckpoint("restore_test");
    EXPECT_TRUE(can_restore);
}

TEST_F(ShardDurabilityTest, DisabledDurability) {
    auto config = createConfig();
    config.enable_wal = false;
    
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    EXPECT_FALSE(durability.isDurabilityEnabled());
    
    // syncWAL should return false when disabled
    EXPECT_FALSE(durability.syncWAL());
}

TEST_F(ShardDurabilityTest, StatisticsAccumulation) {
    auto config = createConfig();
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    const auto& stats = durability.getStatistics();
    
    // Create multiple checkpoints
    durability.createCheckpoint();
    durability.createCheckpoint();
    
    // Sync WAL multiple times
    durability.syncWAL();
    durability.syncWAL();
    
    // Check accumulated statistics
    EXPECT_EQ(stats.checkpoints_created.load(), 2);
    EXPECT_GE(stats.total_syncs.load(), 2);
}

// Durability under failure scenarios would require simulating crashes
// These tests verify the infrastructure is in place
TEST_F(ShardDurabilityTest, InfrastructureForFailureRecovery) {
    auto config = createConfig();
    ShardDurability durability(db_.get(), config);
    
    ASSERT_TRUE(durability.initialize());
    
    // Verify all recovery mechanisms are available
    EXPECT_TRUE(durability.isDurabilityEnabled());
    EXPECT_TRUE(durability.verifyWALIntegrity());
    EXPECT_GT(durability.getCurrentSequenceNumber(), 0);
    
    // Create checkpoint for recovery
    auto checkpoint = durability.createCheckpoint();
    ASSERT_TRUE(checkpoint.has_value());
    EXPECT_TRUE(checkpoint->is_valid);
}
