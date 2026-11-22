#include "sharding/raft_consensus.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <thread>

using namespace themisdb::sharding;
using namespace std::chrono_literals;

// Reachable peers use the production durable AppendEntries handler; tests
// control only whether a transport attempt can reach that handler.
class RaftProposalRecoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        persistence_directory_ = std::filesystem::temp_directory_path() /
            ("themis_raft_proposal_recovery_" + std::to_string(nonce));
        std::filesystem::create_directories(persistence_directory_);
    }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(persistence_directory_, error);
    }

    RaftConsensus::Config createConsensusConfig() const {
        RaftConsensus::Config config;
        config.raft_config.node_id = "leader";
        config.raft_config.cluster_members = {"leader", "follower-a", "follower-b"};
        config.raft_config.persistence_path =
            (persistence_directory_ / "leader.wal").string();
        config.raft_config.election_timeout_min_ms = 10000;
        config.raft_config.election_timeout_max_ms = 10000;
        config.raft_config.heartbeat_interval_ms = 10;
        config.enable_partition_detection = false;
        return config;
    }

    std::unique_ptr<RaftState> createFollower(const std::string& node_id) const {
        RaftConfig config;
        config.node_id = node_id;
        config.cluster_members = {"leader", "follower-a", "follower-b"};
        config.persistence_path = (persistence_directory_ / (node_id + ".wal")).string();
        return std::make_unique<RaftState>(config);
    }

    void electLeader(RaftConsensus& consensus) {
        auto& state = consensus.getRaftState();
        state.becomeCandidate();
        state.receiveVote("follower-a", true);
        ASSERT_TRUE(state.isLeader());
    }

    std::filesystem::path persistence_directory_;
};

TEST_F(RaftProposalRecoveryTest, IdenticalCommandAfterQuorumRecoveryGetsDistinctEntry) {
    RaftConsensus consensus(createConsensusConfig());
    auto follower_a = createFollower("follower-a");
    auto follower_b = createFollower("follower-b");
    bool follower_a_available = false;
    bool follower_b_available = false;
    electLeader(consensus);

    consensus.setReplicationCallback(
        [&](const std::string& peer, const AppendEntriesRequest& request) {
            if (peer == "follower-a" && follower_a_available) {
                return follower_a->handleAppendEntries(request);
            }
            if (peer == "follower-b" && follower_b_available) {
                return follower_b->handleAppendEntries(request);
            }
            return AppendEntriesResponse{request.term, false, 0};
        });

    EXPECT_FALSE(consensus.proposeWithIndex("retained").has_value());
    EXPECT_EQ(consensus.getRaftState().getLog().getLastLogIndex(), 1);
    EXPECT_EQ(consensus.getRaftState().getLog().getCommitIndex(), 0);

    follower_a_available = true;
    const auto committed_index = consensus.proposeWithIndex("retained");

    ASSERT_TRUE(committed_index.has_value());
    EXPECT_EQ(*committed_index, 2);
    EXPECT_EQ(consensus.getRaftState().getLog().getLastLogIndex(), 2);
    EXPECT_EQ(consensus.getRaftState().getLog().getCommitIndex(), 2);
    ASSERT_TRUE(follower_a->getLog().getEntry(1).has_value());
    ASSERT_TRUE(follower_a->getLog().getEntry(2).has_value());
    EXPECT_EQ(follower_a->getLog().getEntry(1)->command, "retained");
    EXPECT_EQ(follower_a->getLog().getEntry(2)->command, "retained");
    EXPECT_EQ(follower_a->getLog().getEntry(1)->index, 1);
    EXPECT_EQ(follower_a->getLog().getEntry(2)->index, 2);
}

TEST_F(RaftProposalRecoveryTest, OversizedMultiNodeCommandIsRejectedBeforeAppend) {
    RaftConsensus consensus(createConsensusConfig());
    auto follower_a = createFollower("follower-a");
    electLeader(consensus);

    std::size_t replication_calls = 0;
    consensus.setReplicationCallback(
        [&](const std::string&, const AppendEntriesRequest& request) {
            ++replication_calls;
            return follower_a->handleAppendEntries(request);
        });

    const std::string oversized(
        RaftAppendEntriesLimits::max_command_bytes + 1, 'x');
    EXPECT_FALSE(consensus.proposeWithIndex(oversized).has_value());
    EXPECT_EQ(consensus.getRaftState().getLog().getLastLogIndex(), 0);
    EXPECT_EQ(consensus.getRaftState().getLog().size(), 0);
    EXPECT_EQ(replication_calls, 0);

    const auto accepted_index = consensus.proposeWithIndex("within-limit");
    ASSERT_TRUE(accepted_index.has_value());
    EXPECT_EQ(*accepted_index, 1);
    EXPECT_EQ(replication_calls, 1);
}

TEST_F(RaftProposalRecoveryTest, NewCurrentTermEntryAdvancesRetainedPrefix) {
    RaftConsensus consensus(createConsensusConfig());
    auto follower_a = createFollower("follower-a");
    bool follower_a_available = false;
    electLeader(consensus);

    consensus.setReplicationCallback(
        [&](const std::string& peer, const AppendEntriesRequest& request) {
            if (peer == "follower-a" && follower_a_available) {
                return follower_a->handleAppendEntries(request);
            }
            return AppendEntriesResponse{request.term, false, 0};
        });

    EXPECT_FALSE(consensus.proposeWithIndex("first").has_value());
    EXPECT_EQ(consensus.getRaftState().getLog().getCommitIndex(), 0);

    consensus.getRaftState().becomeCandidate();
    consensus.getRaftState().receiveVote("follower-a", true);
    ASSERT_TRUE(consensus.isLeader());
    follower_a_available = true;
    const auto committed_index = consensus.proposeWithIndex("second");

    ASSERT_TRUE(committed_index.has_value());
    EXPECT_EQ(*committed_index, 2);
    EXPECT_EQ(consensus.getRaftState().getLog().getLastLogIndex(), 2);
    EXPECT_EQ(consensus.getRaftState().getLog().getCommitIndex(), 2);
    ASSERT_TRUE(follower_a->getLog().getEntry(1).has_value());
    ASSERT_TRUE(follower_a->getLog().getEntry(2).has_value());
    EXPECT_EQ(follower_a->getLog().getEntry(1)->command, "first");
    EXPECT_EQ(follower_a->getLog().getEntry(2)->command, "second");
    EXPECT_EQ(follower_a->getLog().getEntry(1)->term, 1);
    EXPECT_EQ(follower_a->getLog().getEntry(2)->term, 2);
}

TEST_F(RaftProposalRecoveryTest, HeartbeatCatchesUpLaggingDurableFollower) {
    RaftConsensus consensus(createConsensusConfig());
    auto follower_a = createFollower("follower-a");
    auto follower_b = createFollower("follower-b");
    bool follower_b_available = false;
    electLeader(consensus);

    consensus.setReplicationCallback(
        [&](const std::string& peer, const AppendEntriesRequest& request) {
            if (peer == "follower-a") {
                return follower_a->handleAppendEntries(request);
            }
            if (peer == "follower-b" && follower_b_available) {
                return follower_b->handleAppendEntries(request);
            }
            return AppendEntriesResponse{request.term, false, 0};
        });

    const auto first_index = consensus.proposeWithIndex("one");
    const auto second_index = consensus.proposeWithIndex("two");
    ASSERT_TRUE(first_index.has_value());
    ASSERT_TRUE(second_index.has_value());
    EXPECT_EQ(*first_index, 1);
    EXPECT_EQ(*second_index, 2);
    EXPECT_EQ(follower_b->getLog().getLastLogIndex(), 0);

    follower_b_available = true;
    consensus.start();
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (follower_b->getLog().getLastLogIndex() < 2 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(5ms);
    }
    consensus.stop();

    ASSERT_EQ(follower_b->getLog().getLastLogIndex(), 2);
    ASSERT_TRUE(follower_b->getLog().getEntry(1).has_value());
    ASSERT_TRUE(follower_b->getLog().getEntry(2).has_value());
    EXPECT_EQ(follower_b->getLog().getEntry(1)->command, "one");
    EXPECT_EQ(follower_b->getLog().getEntry(2)->command, "two");
    EXPECT_EQ(follower_b->getLog().getCommitIndex(), 2);
}
