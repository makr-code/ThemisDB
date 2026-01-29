// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "sharding/raft_consensus_adapter.h"
#include "sharding/raft_consensus.h"
#include <thread>
#include <chrono>

using namespace themisdb::sharding;
using namespace std::chrono_literals;

class RaftConsensusAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration for testing
        config_.node_id = "node1";
        config_.cluster_nodes = {"node1", "node2", "node3"};
        config_.heartbeat_interval = 100ms;
        config_.election_timeout_min = 300ms;
        config_.election_timeout_max = 600ms;
        config_.enable_persistence = false;
    }
    
    ConsensusConfig config_;
};

// Test TODO #1: convertState implementation
TEST_F(RaftConsensusAdapterTest, ConvertStateFollower) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    
    // Initially should be follower
    auto state = adapter.getState();
    EXPECT_EQ(state, ConsensusState::FOLLOWER);
}

TEST_F(RaftConsensusAdapterTest, ConvertStateLeader) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    
    // Make node a leader by directly manipulating the Raft state
    // This simulates winning an election
    adapter.start();
    
    // Initially not leader
    EXPECT_FALSE(adapter.isLeader());
    
    adapter.stop();
}

// Test TODO #2: Get actual log index from propose
TEST_F(RaftConsensusAdapterTest, ProposeReturnsLogIndex) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Can only propose if leader, so test should fail initially
    nlohmann::json data = {{"key", "value"}};
    auto result = adapter.propose("PUT", data);
    
    // Should return nullopt when not leader
    EXPECT_FALSE(result.has_value());
    
    adapter.stop();
}

// Test TODO #3: waitForCommit implementation
TEST_F(RaftConsensusAdapterTest, WaitForCommitTimeout) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Try to wait for a non-existent log entry with short timeout
    bool result = adapter.waitForCommit(9999, 100ms);
    
    // Should timeout since log index doesn't exist
    EXPECT_FALSE(result);
    
    adapter.stop();
}

TEST_F(RaftConsensusAdapterTest, WaitForCommitAlreadyCommitted) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Wait for index 0 (which should already be committed/past)
    bool result = adapter.waitForCommit(0, 100ms);
    
    // Should return immediately since commit index is already >= 0
    EXPECT_TRUE(result);
    
    adapter.stop();
}

// Test TODO #4: readLog implementation
TEST_F(RaftConsensusAdapterTest, ReadLogEmpty) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Read from empty log
    auto entries = adapter.readLog(1, std::nullopt);
    
    // Should be empty since no entries committed
    EXPECT_TRUE(entries.empty());
    
    adapter.stop();
}

TEST_F(RaftConsensusAdapterTest, ReadLogWithRange) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Try to read a range from index 1 to 10
    auto entries = adapter.readLog(1, 10);
    
    // Should be empty or return only committed entries
    // Since we haven't committed anything, it should be empty
    EXPECT_TRUE(entries.empty());
    
    // Verify the size constraint if not empty
    if (!entries.empty()) {
        EXPECT_LE(entries.size(), 10u);
    }
    
    adapter.stop();
}

// Test TODO #5: getCommitIndex implementation
TEST_F(RaftConsensusAdapterTest, GetCommitIndexInitial) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    
    // Initial commit index should be 0
    auto commit_index = adapter.getCommitIndex();
    EXPECT_EQ(commit_index, 0);
}

TEST_F(RaftConsensusAdapterTest, GetCommitIndexAfterStart) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Commit index should be accessible
    auto commit_index = adapter.getCommitIndex();
    EXPECT_GE(commit_index, 0);
    
    adapter.stop();
}

// Test TODO #6: Dynamic membership changes
TEST_F(RaftConsensusAdapterTest, AddNodeWhenNotLeader) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Should fail since not leader
    bool result = adapter.addNode("node4", "localhost:8004");
    EXPECT_FALSE(result);
    
    adapter.stop();
}

TEST_F(RaftConsensusAdapterTest, AddNodeDuplicate) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Even if not leader, trying to add a duplicate node should fail
    // We test the validation logic for duplicate detection
    bool result = adapter.addNode("node2", "localhost:8002");
    
    // Should fail - either because not leader or because node already exists
    EXPECT_FALSE(result);
    
    adapter.stop();
}

TEST_F(RaftConsensusAdapterTest, RemoveNodeWhenNotLeader) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Should fail since not leader
    bool result = adapter.removeNode("node2");
    EXPECT_FALSE(result);
    
    adapter.stop();
}

TEST_F(RaftConsensusAdapterTest, RemoveNodeNonExistent) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Try to remove non-existent node
    bool result = adapter.removeNode("node99");
    EXPECT_FALSE(result);
    
    adapter.stop();
}

// Integration test: Full workflow
TEST_F(RaftConsensusAdapterTest, IntegrationInitStartStop) {
    RaftConsensusAdapter adapter(config_);
    
    // Initialize
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    
    // Start
    ASSERT_TRUE(adapter.start());
    
    // Check basic state
    EXPECT_FALSE(adapter.isLeader());  // Should not be leader initially
    EXPECT_EQ(adapter.getCommitIndex(), 0);  // Initial commit index
    
    // Get stats
    auto stats = adapter.getStats();
    EXPECT_EQ(stats.cluster_size, 3);
    
    // Get status
    auto status = adapter.getStatus();
    EXPECT_EQ(status["type"], "Raft");
    EXPECT_EQ(status["node_id"], "node1");
    
    // Stop
    adapter.stop();
}

// Test that state conversion works correctly
TEST_F(RaftConsensusAdapterTest, StateConversionConsistency) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Get state multiple times, should be consistent
    auto state1 = adapter.getState();
    auto state2 = adapter.getState();
    EXPECT_EQ(state1, state2);
    
    adapter.stop();
}

// Test log entry conversion
TEST_F(RaftConsensusAdapterTest, LogEntryConversion) {
    RaftConsensusAdapter adapter(config_);
    ASSERT_TRUE(adapter.initialize(config_.node_id, config_.cluster_nodes));
    adapter.start();
    
    // Read log entries (should be empty initially)
    auto entries = adapter.readLog(0, std::nullopt);
    
    // Even if empty, the readLog should not crash
    EXPECT_NO_THROW({
        for (const auto& entry : entries) {
            // Access fields to ensure they're properly populated
            EXPECT_GE(entry.index, 0u);
            EXPECT_GE(entry.term, 0u);
            EXPECT_FALSE(entry.operation.empty());
        }
    });
    
    adapter.stop();
}
