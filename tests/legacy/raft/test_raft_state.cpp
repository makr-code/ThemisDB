#include "sharding/raft_state.h"
#include <gtest/gtest.h>
#include <thread>

using namespace themisdb::sharding;

class RaftStateTest : public ::testing::Test {
protected:
    RaftConfig createConfig(const std::string& node_id, size_t cluster_size = 3) {
        RaftConfig config;
        config.node_id = node_id;
        
        for (size_t i = 1; i <= cluster_size; ++i) {
            config.cluster_members.push_back("node_" + std::to_string(i));
        }
        
        config.election_timeout_min_ms = 50;   // Shorter for tests
        config.election_timeout_max_ms = 100;
        config.heartbeat_interval_ms = 20;
        
        return config;
    }
};

// Test: Initial state is FOLLOWER
TEST_F(RaftStateTest, InitialStateIsFollower) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    EXPECT_EQ(raft.getState(), RaftNodeState::FOLLOWER);
    EXPECT_EQ(raft.getCurrentTerm(), 0);
    EXPECT_TRUE(raft.getLeaderId().empty());
    EXPECT_FALSE(raft.isLeader());
    EXPECT_TRUE(raft.isFollower());
    EXPECT_FALSE(raft.isCandidate());
}

// Test: Become follower
TEST_F(RaftStateTest, BecomeFollower) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    raft.becomeFollower(5);
    
    EXPECT_EQ(raft.getState(), RaftNodeState::FOLLOWER);
    EXPECT_EQ(raft.getCurrentTerm(), 5);
    EXPECT_TRUE(raft.getVotedFor().empty());
}

// Test: Become candidate
TEST_F(RaftStateTest, BecomeCandidate) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    raft.becomeCandidate();
    
    EXPECT_EQ(raft.getState(), RaftNodeState::CANDIDATE);
    EXPECT_EQ(raft.getCurrentTerm(), 1);  // Term incremented
    EXPECT_EQ(raft.getVotedFor(), "node_1");  // Voted for self
    EXPECT_EQ(raft.getVotesReceived(), 1);  // Own vote counted
}

// Test: Start election
TEST_F(RaftStateTest, StartElection) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    raft.startElection();
    
    EXPECT_TRUE(raft.isCandidate());
    EXPECT_EQ(raft.getCurrentTerm(), 1);
    EXPECT_EQ(raft.getVotedFor(), "node_1");
}

// Test: Become leader with quorum
TEST_F(RaftStateTest, BecomeLeaderWithQuorum) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    raft.becomeCandidate();
    
    // Receive vote from another node (have 2/3, which is quorum)
    raft.receiveVote("node_2", true);
    
    EXPECT_TRUE(raft.isLeader());
    EXPECT_EQ(raft.getLeaderId(), "node_1");
}

// Test: Cannot become leader without quorum
TEST_F(RaftStateTest, CannotBecomeLeaderWithoutQuorum) {
    auto config = createConfig("node_1", 5);  // 5-node cluster
    RaftState raft(config);
    
    raft.becomeCandidate();
    
    // Receive votes from 1 other node (have 2/5, need 3 for quorum)
    raft.receiveVote("node_2", true);
    
    EXPECT_FALSE(raft.isLeader());
    EXPECT_TRUE(raft.isCandidate());
    
    // Receive one more vote (now have 3/5, which is quorum)
    raft.receiveVote("node_3", true);
    
    EXPECT_TRUE(raft.isLeader());
}

// Test: Quorum calculation
TEST_F(RaftStateTest, QuorumCalculation) {
    auto config3 = createConfig("node_1", 3);
    RaftState raft3(config3);
    EXPECT_EQ(raft3.getQuorumSize(), 2);  // 3/2 + 1 = 2
    
    auto config5 = createConfig("node_1", 5);
    RaftState raft5(config5);
    EXPECT_EQ(raft5.getQuorumSize(), 3);  // 5/2 + 1 = 3
    
    auto config7 = createConfig("node_1", 7);
    RaftState raft7(config7);
    EXPECT_EQ(raft7.getQuorumSize(), 4);  // 7/2 + 1 = 4
}

// Test: Vote request with lower term rejected
TEST_F(RaftStateTest, VoteRequestLowerTermRejected) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    raft.becomeFollower(5);
    
    VoteRequest request;
    request.term = 3;  // Lower than current term
    request.candidate_id = "node_2";
    request.last_log_index = 0;
    request.last_log_term = 0;
    
    auto response = raft.handleVoteRequest(request);
    
    EXPECT_FALSE(response.vote_granted);
    EXPECT_EQ(response.term, 5);
}

// Test: Vote request with higher term updates term
TEST_F(RaftStateTest, VoteRequestHigherTermUpdatesTerm) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    raft.becomeFollower(3);
    
    VoteRequest request;
    request.term = 5;  // Higher than current term
    request.candidate_id = "node_2";
    request.last_log_index = 0;
    request.last_log_term = 0;
    
    auto response = raft.handleVoteRequest(request);
    
    EXPECT_TRUE(response.vote_granted);
    EXPECT_EQ(response.term, 5);
    EXPECT_EQ(raft.getCurrentTerm(), 5);
    EXPECT_TRUE(raft.isFollower());
}

// Test: Can only vote once per term
TEST_F(RaftStateTest, CanOnlyVoteOncePerTerm) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    VoteRequest request1;
    request1.term = 1;
    request1.candidate_id = "node_2";
    request1.last_log_index = 0;
    request1.last_log_term = 0;
    
    auto response1 = raft.handleVoteRequest(request1);
    EXPECT_TRUE(response1.vote_granted);
    EXPECT_EQ(raft.getVotedFor(), "node_2");
    
    // Try to vote for different candidate in same term
    VoteRequest request2;
    request2.term = 1;
    request2.candidate_id = "node_3";
    request2.last_log_index = 0;
    request2.last_log_term = 0;
    
    auto response2 = raft.handleVoteRequest(request2);
    EXPECT_FALSE(response2.vote_granted);
}

// Test: Can vote for same candidate multiple times
TEST_F(RaftStateTest, CanVoteForSameCandidateMultipleTimes) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    VoteRequest request1;
    request1.term = 1;
    request1.candidate_id = "node_2";
    request1.last_log_index = 0;
    request1.last_log_term = 0;
    
    auto response1 = raft.handleVoteRequest(request1);
    EXPECT_TRUE(response1.vote_granted);
    
    // Vote for same candidate again
    auto response2 = raft.handleVoteRequest(request1);
    EXPECT_TRUE(response2.vote_granted);
}

// Test: Election timeout
TEST_F(RaftStateTest, ElectionTimeout) {
    auto config = createConfig("node_1");
    config.election_timeout_min_ms = 10;
    config.election_timeout_max_ms = 20;
    RaftState raft(config);
    
    EXPECT_FALSE(raft.isElectionTimeout());
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_TRUE(raft.isElectionTimeout());
}

// Test: Reset election timeout
TEST_F(RaftStateTest, ResetElectionTimeout) {
    auto config = createConfig("node_1");
    config.election_timeout_min_ms = 10;
    config.election_timeout_max_ms = 20;
    RaftState raft(config);
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    
    // Reset timeout
    raft.resetElectionTimeout();
    
    // Should not be timed out yet
    EXPECT_FALSE(raft.isElectionTimeout());
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_TRUE(raft.isElectionTimeout());
}

// Test: Leader does not have election timeout
TEST_F(RaftStateTest, LeaderNoElectionTimeout) {
    auto config = createConfig("node_1");
    config.election_timeout_min_ms = 10;
    config.election_timeout_max_ms = 20;
    RaftState raft(config);
    
    raft.becomeCandidate();
    raft.receiveVote("node_2", true);  // Become leader
    
    EXPECT_TRUE(raft.isLeader());
    
    // Wait for what would be election timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Leader should not timeout
    EXPECT_FALSE(raft.isElectionTimeout());
}

// Test: Heartbeat sending
TEST_F(RaftStateTest, HeartbeatSending) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    raft.becomeCandidate();
    raft.receiveVote("node_2", true);  // Become leader
    
    EXPECT_TRUE(raft.isLeader());
    
    // Should not need to send heartbeat immediately
    EXPECT_FALSE(raft.shouldSendHeartbeat());
    
    // Wait for heartbeat interval
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    
    EXPECT_TRUE(raft.shouldSendHeartbeat());
    
    // Send heartbeat
    raft.sendHeartbeat();
    
    // Should not need to send again immediately
    EXPECT_FALSE(raft.shouldSendHeartbeat());
}

// Test: Follower cannot send heartbeat
TEST_F(RaftStateTest, FollowerCannotSendHeartbeat) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    EXPECT_TRUE(raft.isFollower());
    EXPECT_FALSE(raft.shouldSendHeartbeat());
    
    // Try to send heartbeat (should be no-op)
    raft.sendHeartbeat();
}

// Test: Rejected votes don't count toward quorum
TEST_F(RaftStateTest, RejectedVotesDontCount) {
    auto config = createConfig("node_1", 5);
    RaftState raft(config);
    
    raft.becomeCandidate();
    
    // Receive some rejected votes
    raft.receiveVote("node_2", false);
    raft.receiveVote("node_3", false);
    
    EXPECT_FALSE(raft.isLeader());
    
    // Receive granted votes (1 self + 2 granted = 3/5, quorum)
    raft.receiveVote("node_4", true);
    raft.receiveVote("node_5", true);
    
    EXPECT_TRUE(raft.isLeader());
}

// Test: Config getters
TEST_F(RaftStateTest, ConfigGetters) {
    auto config = createConfig("node_1", 3);
    RaftState raft(config);
    
    EXPECT_EQ(raft.getNodeId(), "node_1");
    
    auto members = raft.getClusterMembers();
    EXPECT_EQ(members.size(), 3);
    EXPECT_EQ(members[0], "node_1");
    EXPECT_EQ(members[1], "node_2");
    EXPECT_EQ(members[2], "node_3");
}

// Test: Term increment on candidate transition
TEST_F(RaftStateTest, TermIncrementOnCandidate) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    EXPECT_EQ(raft.getCurrentTerm(), 0);
    
    raft.becomeCandidate();
    EXPECT_EQ(raft.getCurrentTerm(), 1);
    
    // Lose election, become follower, then candidate again
    raft.becomeFollower(1);
    raft.becomeCandidate();
    
    EXPECT_EQ(raft.getCurrentTerm(), 2);
}

// Test: Candidate receiving vote for different term ignored
TEST_F(RaftStateTest, VoteForDifferentTermIgnored) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    raft.becomeCandidate();  // Term = 1
    
    // Become follower with higher term
    raft.becomeFollower(2);
    
    // Receive vote for old term (should be ignored)
    raft.receiveVote("node_2", true);
    
    EXPECT_TRUE(raft.isFollower());
    EXPECT_FALSE(raft.isLeader());
}

// Test: Thread safety basic
TEST_F(RaftStateTest, ThreadSafetyBasic) {
    auto config = createConfig("node_1");
    RaftState raft(config);
    
    // Run multiple threads accessing state
    std::thread t1([&raft]() {
        for (int i = 0; i < 100; ++i) {
            raft.getState();
            raft.getCurrentTerm();
            raft.isLeader();
        }
    });
    
    std::thread t2([&raft]() {
        for (int i = 0; i < 100; ++i) {
            raft.resetElectionTimeout();
            raft.isElectionTimeout();
        }
    });
    
    t1.join();
    t2.join();
    
    // Should not crash
    EXPECT_TRUE(true);
}


