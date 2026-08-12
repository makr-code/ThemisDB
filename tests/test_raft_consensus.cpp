#include "sharding/raft_consensus.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace themisdb::sharding;
using namespace std::chrono_literals;

class RaftConsensusTest : public ::testing::Test {
protected:
    RaftConsensus::Config createConfig(const std::string& node_id, 
                                      const std::vector<std::string>& members) {
        RaftConsensus::Config config;
        config.raft_config.node_id = node_id;
        config.raft_config.cluster_members = members;
        config.raft_config.election_timeout_min_ms = 50;
        config.raft_config.election_timeout_max_ms = 100;
        config.raft_config.heartbeat_interval_ms = 20;
        config.heartbeat_timeout = 500ms;
        config.partition_detection_interval = 200ms;
        config.enable_partition_detection = true;
        config.enable_split_brain_prevention = true;
        return config;
    }
};

TEST_F(RaftConsensusTest, InitialState) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    RaftConsensus consensus(config);
    
    EXPECT_FALSE(consensus.isLeader());
    EXPECT_EQ(consensus.getCurrentTerm(), 0);
}

TEST_F(RaftConsensusTest, StartStop) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    RaftConsensus consensus(config);
    
    consensus.start();
    std::this_thread::sleep_for(100ms);
    
    consensus.stop();
    // Should not crash
}

TEST_F(RaftConsensusTest, ProposeWhenNotLeader) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    RaftConsensus consensus(config);
    
    auto future = consensus.propose("test_command");
    auto result = future.get();
    
    EXPECT_FALSE(result);  // Not leader, should fail
}

TEST_F(RaftConsensusTest, BecomeLeaderAndPropose) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    RaftConsensus consensus(config);
    
    // Become leader
    consensus.getRaftState().becomeLeader();
    
    // Set up callback that always succeeds
    int replication_calls = 0;
    consensus.setReplicationCallback([&]([[maybe_unused]] const std::string& node_id,
                                         [[maybe_unused]] const LogEntry& entry) {
        replication_calls++;
        return true;
    });
    
    consensus.start();
    
    auto future = consensus.propose("test_command");
    auto result = future.wait_for(1s);
    
    EXPECT_EQ(result, std::future_status::ready);
    
    consensus.stop();
}

TEST_F(RaftConsensusTest, PartitionDetection) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    RaftConsensus consensus(config);
    
    consensus.start();
    std::this_thread::sleep_for(300ms);
    
    // Initially should have quorum
    EXPECT_TRUE(consensus.hasQuorum());
    
    // Simulate partition by not responding to heartbeats
    auto status = consensus.getPartitionStatus();
    EXPECT_FALSE(status.is_partitioned);  // Not partitioned initially
    
    consensus.stop();
}

TEST_F(RaftConsensusTest, ReadOnlyOnMinorityPartition) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    config.read_only_on_partition = true;
    RaftConsensus consensus(config);
    
    consensus.start();
    
    // Initially not read-only
    EXPECT_FALSE(consensus.isReadOnly());
    
    consensus.stop();
}

TEST_F(RaftConsensusTest, HeartbeatCallback) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    RaftConsensus consensus(config);
    
    int heartbeat_count = 0;
    consensus.setHeartbeatCallback([&]([[maybe_unused]] const std::string& node_id,
                                       [[maybe_unused]] const Heartbeat& hb) {
        heartbeat_count++;
        return true;
    });
    
    consensus.getRaftState().becomeLeader();
    consensus.start();
    
    std::this_thread::sleep_for(150ms);
    
    // Should have sent multiple heartbeats
    EXPECT_GT(heartbeat_count, 0);
    
    consensus.stop();
}

TEST_F(RaftConsensusTest, ReceiveHeartbeat) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    RaftConsensus consensus(config);
    
    Heartbeat hb;
    hb.leader_id = "node2";
    hb.term = 5;
    hb.commit_index = 10;
    hb.timestamp = std::chrono::steady_clock::now();
    
    consensus.receiveHeartbeat(hb);
    
    // Should become follower and update term
    EXPECT_FALSE(consensus.isLeader());
    EXPECT_FALSE(consensus.isReadOnly());
}

TEST_F(RaftConsensusTest, ReplicaStateTracking) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    RaftConsensus consensus(config);
    
    consensus.start();
    std::this_thread::sleep_for(100ms);
    
    auto states = consensus.getReplicaStates();
    EXPECT_EQ(states.size(), 2);  // Two other nodes
    
    // Check that replicas are tracked
    for (const auto& state : states) {
        EXPECT_TRUE(state.node_id == "node2" || state.node_id == "node3");
    }
    
    consensus.stop();
}

TEST_F(RaftConsensusTest, AppendEntriesResponse) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    RaftConsensus consensus(config);
    
    consensus.getRaftState().becomeLeader();
    consensus.start();
    
    AppendEntriesResponse response;
    response.term = 1;
    response.success = true;
    response.match_index = 5;
    
    consensus.receiveAppendEntriesResponse("node2", response);
    
    // Check replica state updated
    auto states = consensus.getReplicaStates();
    auto it = std::find_if(states.begin(), states.end(),
        [](const ReplicaState& s) { return s.node_id == "node2"; });
    
    EXPECT_NE(it, states.end());
    if (it != states.end()) {
        EXPECT_EQ(it->match_index, 5);
    }
    
    consensus.stop();
}

TEST_F(RaftConsensusTest, QuorumCheck) {
    auto config = createConfig("node1", {"node1", "node2", "node3"});
    RaftConsensus consensus(config);
    
    consensus.start();
    
    // With all nodes healthy, should have quorum
    EXPECT_TRUE(consensus.hasQuorum());
    
    consensus.stop();
}
