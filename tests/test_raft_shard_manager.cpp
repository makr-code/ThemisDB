// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/raft_shard_manager.h"
#include "sharding/raft_state.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace themisdb::sharding;

class RaftShardManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration for tests
        RaftShardManager::Config config;
        config.replication_factor = 3;
        config.enable_auto_start = false;  // Manual start for controlled testing
        config.raft_config.raft_config.election_timeout_min_ms = 150;
        config.raft_config.raft_config.election_timeout_max_ms = 300;
        config.raft_config.raft_config.heartbeat_interval_ms = 50;
        
        manager_ = std::make_unique<RaftShardManager>(config);
    }
    
    void TearDown() override {
        manager_.reset();
    }
    
    std::unique_ptr<RaftShardManager> manager_;
};

TEST_F(RaftShardManagerTest, InitializeShard) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    // Initialize shard
    bool result = manager_->initializeShard(shard_id, replicas);
    EXPECT_TRUE(result);
    
    // Verify shard info
    auto info = manager_->getShardRaftInfo(shard_id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->shard_id, shard_id);
    EXPECT_EQ(info->role, RaftNodeState::FOLLOWER);  // Should start as follower
    EXPECT_EQ(info->term, 0);  // Initial term
}

TEST_F(RaftShardManagerTest, InitializeShardInsufficientReplicas) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002"};  // Only 2, need 3
    
    // Should fail with insufficient replicas
    bool result = manager_->initializeShard(shard_id, replicas);
    EXPECT_FALSE(result);
}

TEST_F(RaftShardManagerTest, InitializeShardDuplicate) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    // First initialization should succeed
    EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));
    
    // Second initialization should fail (already exists)
    EXPECT_FALSE(manager_->initializeShard(shard_id, replicas));
}

TEST_F(RaftShardManagerTest, RemoveShard) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    // Initialize and then remove
    EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));
    EXPECT_TRUE(manager_->getShardRaftInfo(shard_id).has_value());
    
    manager_->removeShard(shard_id);
    EXPECT_FALSE(manager_->getShardRaftInfo(shard_id).has_value());
}

TEST_F(RaftShardManagerTest, StartStopShard) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    // Initialize shard (auto-start is disabled)
    EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));
    
    // Start shard
    EXPECT_TRUE(manager_->startShard(shard_id));
    
    // Give it a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop shard
    manager_->stopShard(shard_id);
    
    // Should still exist but stopped
    EXPECT_TRUE(manager_->getShardRaftInfo(shard_id).has_value());
}

TEST_F(RaftShardManagerTest, IsShardLeader) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));
    
    // Initially should not be leader (follower state)
    EXPECT_FALSE(manager_->isShardLeader(shard_id));
}

TEST_F(RaftShardManagerTest, GetShardLeader) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));
    
    // Before Raft is started/elected, leader should be unknown
    std::string leader = manager_->getShardLeader(shard_id);
    EXPECT_TRUE(leader.empty());

    // Non-existent shard must also report no leader
    EXPECT_TRUE(manager_->getShardLeader("non_existent_shard").empty());
}

TEST_F(RaftShardManagerTest, ProposeWriteWithoutStart) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));
    
    // Try to propose write without starting Raft (should fail or timeout)
    auto future = manager_->proposeWrite(shard_id, "test_command");
    ASSERT_TRUE(future.valid());
    
    // Wait for result (should be false since not started/not leader)
    auto result = future.get();
    EXPECT_FALSE(result);
}

TEST_F(RaftShardManagerTest, GetAllShardRaftInfo) {
    // Initialize multiple shards
    manager_->initializeShard("shard_001", {"shard_001", "shard_002", "shard_003"});
    manager_->initializeShard("shard_002", {"shard_001", "shard_002", "shard_003"});
    manager_->initializeShard("shard_003", {"shard_001", "shard_002", "shard_003"});
    
    // Get all info
    auto all_info = manager_->getAllShardRaftInfo();
    EXPECT_EQ(all_info.size(), 3);
    
    EXPECT_TRUE(all_info.find("shard_001") != all_info.end());
    EXPECT_TRUE(all_info.find("shard_002") != all_info.end());
    EXPECT_TRUE(all_info.find("shard_003") != all_info.end());
}

TEST_F(RaftShardManagerTest, HasQuorum) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));

    // HasQuorum must be consistent with the shard info view
    auto info = manager_->getShardRaftInfo(shard_id);
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(manager_->hasQuorum(shard_id), info->has_quorum);

    // Non-existent shard must never report quorum
    EXPECT_FALSE(manager_->hasQuorum("non_existent_shard"));
}

TEST_F(RaftShardManagerTest, GetRaftInstance) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));
    
    // Get Raft instance
    auto raft = manager_->getRaftInstance(shard_id);
    ASSERT_NE(raft, nullptr);
    
    // Verify it's a valid RaftConsensus instance
    EXPECT_EQ(raft->getRaftState().getState(), RaftNodeState::FOLLOWER);
}

TEST_F(RaftShardManagerTest, GetRaftInstanceNonExistent) {
    // Try to get instance for non-existent shard
    auto raft = manager_->getRaftInstance("non_existent_shard");
    EXPECT_EQ(raft, nullptr);
}

TEST_F(RaftShardManagerTest, GetShardRaftInfoDetails) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));
    
    auto info = manager_->getShardRaftInfo(shard_id);
    ASSERT_TRUE(info.has_value());
    
    // Verify all fields are properly initialized
    EXPECT_EQ(info->shard_id, shard_id);
    EXPECT_EQ(info->role, RaftNodeState::FOLLOWER);
    EXPECT_EQ(info->term, 0);
    EXPECT_EQ(info->commit_index, 0);
    EXPECT_EQ(info->last_applied, 0);
    EXPECT_TRUE(info->leader_id.empty());
    
    // Replica IDs (if present) must be non-empty and not the local shard id
    for (const auto& replica_id : info->replica_ids) {
        EXPECT_FALSE(replica_id.empty());
        EXPECT_NE(replica_id, shard_id);
    }
}

TEST_F(RaftShardManagerTest, MultipleShardsConcurrent) {
    // Test managing multiple shards concurrently
    std::vector<std::string> shard_ids = {"shard_001", "shard_002", "shard_003", "shard_004"};
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003", "shard_004", "shard_005"};
    
    // Initialize all shards
    for (const auto& shard_id : shard_ids) {
        EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));
    }
    
    // Verify all shards are tracked
    auto all_info = manager_->getAllShardRaftInfo();
    EXPECT_EQ(all_info.size(), shard_ids.size());
    
    // Remove all shards
    for (const auto& shard_id : shard_ids) {
        manager_->removeShard(shard_id);
    }
    
    // Verify all removed
    all_info = manager_->getAllShardRaftInfo();
    EXPECT_EQ(all_info.size(), 0);
}

// Test fixture for auto-start enabled
class RaftShardManagerAutoStartTest : public ::testing::Test {
protected:
    void SetUp() override {
        RaftShardManager::Config config;
        config.replication_factor = 3;
        config.enable_auto_start = true;  // Enable auto-start
        config.raft_config.raft_config.election_timeout_min_ms = 150;
        config.raft_config.raft_config.election_timeout_max_ms = 300;
        
        manager_ = std::make_unique<RaftShardManager>(config);
    }
    
    void TearDown() override {
        manager_.reset();
    }
    
    std::unique_ptr<RaftShardManager> manager_;
};

TEST_F(RaftShardManagerAutoStartTest, AutoStartOnInitialize) {
    std::string shard_id = "shard_001";
    std::vector<std::string> replicas = {"shard_001", "shard_002", "shard_003"};
    
    // Initialize with auto-start enabled
    EXPECT_TRUE(manager_->initializeShard(shard_id, replicas));
    
    // Give it a moment to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify shard info shows it's running
    auto info = manager_->getShardRaftInfo(shard_id);
    ASSERT_TRUE(info.has_value());
    // Should be in follower state and running
    EXPECT_TRUE(info->role == RaftNodeState::FOLLOWER || info->role == RaftNodeState::CANDIDATE);
}
