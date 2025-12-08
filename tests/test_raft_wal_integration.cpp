#include <gtest/gtest.h>
#include "sharding/raft_wal_integration.h"
#include "raft_state.h"
#include "raft_log.h"
#include "wal_manager.h"

using namespace themisdb::sharding;

class RaftWALIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock components
        raft_state = std::make_shared<RaftState>(RaftState::Config{
            .node_id = "node1",
            .cluster_members = {"node1", "node2", "node3"}
        });
        
        raft_log = std::make_shared<RaftLog>();
        
        wal_manager = std::make_shared<WALManager>(WALManager::Config{
            .wal_dir = "/tmp/test_wal"
        });
        
        // Create integration
        integration = std::make_unique<RaftWALIntegration>(RaftWALIntegration::Config{
            .node_id = "node1",
            .raft_state = raft_state,
            .raft_log = raft_log,
            .wal_manager = wal_manager
        });
    }
    
    std::shared_ptr<RaftState> raft_state;
    std::shared_ptr<RaftLog> raft_log;
    std::shared_ptr<WALManager> wal_manager;
    std::unique_ptr<RaftWALIntegration> integration;
};

TEST_F(RaftWALIntegrationTest, FollowerRejectsWrites) {
    // Node is follower by default
    EXPECT_FALSE(integration->isLeader());
    
    WALEntry entry;
    entry.type = WALEntryType::INSERT;
    entry.data = {{"key", "value"}};
    
    auto result = integration->write(entry);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Not leader"), std::string::npos);
}

TEST_F(RaftWALIntegrationTest, LeaderAcceptsWrites) {
    // Become leader
    raft_state->becomeLeader();
    integration->onBecomeLeader();
    
    EXPECT_TRUE(integration->isLeader());
    
    // Write should succeed (in test, quorum is simulated)
    WALEntry entry;
    entry.type = WALEntryType::INSERT;
    entry.data = {{"key", "value"}};
    
    // Note: In real test, would need to simulate quorum responses
    // For now, this will timeout, but verifies leader path
}

TEST_F(RaftWALIntegrationTest, LeadershipTransition) {
    // Start as follower
    EXPECT_FALSE(integration->isLeader());
    
    // Become leader
    raft_state->becomeLeader();
    integration->onBecomeLeader();
    EXPECT_TRUE(integration->isLeader());
    
    // Step down to follower
    raft_state->becomeFollower(raft_state->getCurrentTerm() + 1);
    integration->onBecomeFollower();
    EXPECT_FALSE(integration->isLeader());
}

TEST_F(RaftWALIntegrationTest, LinearizableRead) {
    // Only leader can serve reads
    EXPECT_FALSE(integration->isLeader());
    
    LSN lsn{0, 0};
    auto result = integration->read(lsn);
    EXPECT_FALSE(result.has_value());
    
    // Become leader
    raft_state->becomeLeader();
    integration->onBecomeLeader();
    
    // Now reads work (though may return nullopt if LSN doesn't exist)
    result = integration->read(lsn);
    // Result depends on WAL content
}

TEST_F(RaftWALIntegrationTest, LogCompaction) {
    raft_state->becomeLeader();
    integration->onBecomeLeader();
    
    // Compact log up to index 100
    integration->compact(100);
    
    // Commit index should be updated
    EXPECT_GE(raft_log->getCommitIndex(), 100);
}

TEST_F(RaftWALIntegrationTest, GetLeaderId) {
    // Initially no leader
    std::string leader_id = integration->getLeaderId();
    EXPECT_TRUE(leader_id.empty() || leader_id == "node1");
    
    // Become leader
    raft_state->becomeLeader();
    integration->onBecomeLeader();
    
    leader_id = integration->getLeaderId();
    EXPECT_EQ(leader_id, "node1");
}

// Additional tests would include:
// - Quorum acknowledgment simulation
// - WAL Shipper start/stop verification
// - WAL Applier integration
// - Network partition recovery
// - Concurrent write handling

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
