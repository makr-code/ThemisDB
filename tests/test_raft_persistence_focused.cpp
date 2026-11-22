#include "sharding/raft_consensus.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <stdexcept>

using namespace themisdb::sharding;

namespace {

class RaftPersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        directory_ = std::filesystem::temp_directory_path() /
                     ("themis_raft_persistence_" + std::to_string(nonce));
        std::filesystem::create_directories(directory_);
        journal_path_ = directory_ / "node.wal";
    }

    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }

    std::filesystem::path directory_;
    std::filesystem::path journal_path_;
};

AppendEntriesResponse acknowledgeAppendEntries(const AppendEntriesRequest& request) {
    const auto match_index = request.entries.empty()
        ? request.prev_log_index
        : request.entries.back().index;
    return AppendEntriesResponse{request.term, true, match_index};
}

TEST_F(RaftPersistenceTest, LogAndCommitIndexRecoverAfterReopen) {
    {
        RaftLog log(journal_path_.string());
        ASSERT_EQ(log.append(LogEntry{2, 1, "first", 11}), 1u);
        ASSERT_EQ(log.append(LogEntry{2, 2, "second|with-delimiter", 12}), 2u);
        log.setCommitIndex(1);
    }

    RaftLog recovered(journal_path_.string());
    ASSERT_TRUE(recovered.isPersistenceHealthy());
    EXPECT_EQ(recovered.getCommitIndex(), 1u);
    EXPECT_EQ(recovered.getLastLogIndex(), 2u);
    const auto first = recovered.getEntry(1);
    const auto second = recovered.getEntry(2);
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first->command, "first");
    EXPECT_EQ(second->command, "second|with-delimiter");
}

TEST_F(RaftPersistenceTest, HardStateRecoversBeforeVoteDecisions) {
    RaftConfig config;
    config.node_id = "node1";
    config.cluster_members = {"node1", "node2", "node3"};
    config.persistence_path = journal_path_.string();
    {
        RaftState state(config);
        state.becomeCandidate();
        ASSERT_EQ(state.getCurrentTerm(), 1u);
        ASSERT_EQ(state.getVotedFor(), "node1");
    }

    RaftState recovered(config);
    EXPECT_EQ(recovered.getCurrentTerm(), 1u);
    EXPECT_EQ(recovered.getVotedFor(), "node1");

    VoteRequest conflicting_vote;
    conflicting_vote.term = 1;
    conflicting_vote.candidate_id = "node2";
    const auto response = recovered.handleVoteRequest(conflicting_vote);
    EXPECT_FALSE(response.vote_granted);
    EXPECT_EQ(recovered.getVotedFor(), "node1");
}

TEST_F(RaftPersistenceTest, SuccessfulSingleNodeProposalRecoversCommittedAfterRestart) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1"};
    config.raft_config.persistence_path = journal_path_.string();

    {
        RaftConsensus consensus(config);
        auto& state = consensus.getRaftState();
        state.becomeCandidate();
        state.becomeLeader();
        EXPECT_TRUE(consensus.propose("durable-operation").get());
    }

    RaftConsensus recovered(config);
    EXPECT_EQ(recovered.getCurrentTerm(), 1u);
    EXPECT_EQ(recovered.getRaftState().getLog().getCommitIndex(), 1u);
    const auto entry = recovered.getRaftState().getLog().getEntry(1);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->command, "durable-operation");
}

TEST_F(RaftPersistenceTest, SingleNodeElectionPromotesTheSelfVoteToLeader) {
    RaftConfig config;
    config.node_id = "node1";
    config.cluster_members = {"node1"};
    config.persistence_path = journal_path_.string();

    RaftState state(config);
    state.startElection();

    EXPECT_TRUE(state.isLeader());
    EXPECT_EQ(state.getLeaderId(), "node1");
}

TEST_F(RaftPersistenceTest, ProposalIndexMatchesPersistedEntry) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1"};
    config.raft_config.persistence_path = journal_path_.string();

    RaftConsensus consensus(config);
    auto& state = consensus.getRaftState();
    state.becomeCandidate();
    state.becomeLeader();

    const auto first = consensus.proposeWithIndex("first");
    const auto second = consensus.proposeWithIndex("second");
    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(*first, 1u);
    EXPECT_EQ(*second, 2u);
    const auto first_entry = state.getLog().getEntry(*first);
    const auto second_entry = state.getLog().getEntry(*second);
    ASSERT_TRUE(first_entry.has_value());
    ASSERT_TRUE(second_entry.has_value());
    EXPECT_EQ(first_entry->command, "first");
    EXPECT_EQ(second_entry->command, "second");
}

TEST_F(RaftPersistenceTest, HigherTermAppendEntriesResponseStepsDownAndPersists) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1", "node2", "node3"};
    config.raft_config.persistence_path = journal_path_.string();

    {
        RaftConsensus consensus(config);
        auto& state = consensus.getRaftState();
        state.becomeCandidate();
        state.receiveVote("node2", true);
        ASSERT_TRUE(state.isLeader());

        consensus.receiveAppendEntriesResponse(
            "unknown-peer", AppendEntriesResponse{9, false, 0});
        EXPECT_EQ(consensus.getCurrentTerm(), 1u);
        EXPECT_TRUE(consensus.isLeader());

        consensus.receiveAppendEntriesResponse(
            "node2", AppendEntriesResponse{2, false, 0});
        EXPECT_EQ(consensus.getCurrentTerm(), 2u);
        EXPECT_FALSE(consensus.isLeader());
    }

    RaftConsensus recovered(config);
    EXPECT_EQ(recovered.getCurrentTerm(), 2u);
    EXPECT_FALSE(recovered.isLeader());
}

TEST_F(RaftPersistenceTest, HigherTermHeartbeatResponseStopsFurtherPeerSends) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1", "node2", "node3"};
    config.raft_config.persistence_path = journal_path_.string();
    config.raft_config.election_timeout_min_ms = 10000;
    config.raft_config.election_timeout_max_ms = 10000;
    config.raft_config.heartbeat_interval_ms = 10;
    config.enable_partition_detection = false;

    RaftConsensus consensus(config);
    auto& state = consensus.getRaftState();
    state.becomeCandidate();
    state.receiveVote("node2", true);
    ASSERT_TRUE(state.isLeader());

    std::promise<void> heartbeat_received;
    auto received = heartbeat_received.get_future();
    std::atomic<int> heartbeat_calls{0};
    consensus.setHeartbeatCallback(
        [&](const std::string& peer, const Heartbeat& heartbeat) {
            if (heartbeat_calls.fetch_add(1) == 0) {
                consensus.receiveAppendEntriesResponse(
                    peer, AppendEntriesResponse{heartbeat.term + 1, false, 0});
                heartbeat_received.set_value();
            }
            return false;
        });

    consensus.start();
    ASSERT_EQ(received.wait_for(std::chrono::seconds(2)),
              std::future_status::ready);
    consensus.stop();

    EXPECT_EQ(consensus.getCurrentTerm(), 2u);
    EXPECT_FALSE(consensus.isLeader());
    EXPECT_EQ(heartbeat_calls.load(), 1);
}

TEST_F(RaftPersistenceTest, DelayedAppendEntriesAckDoesNotRegressReplicaProgress) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1", "node2", "node3"};
    config.raft_config.persistence_path = journal_path_.string();

    RaftConsensus consensus(config);
    auto& state = consensus.getRaftState();
    state.becomeCandidate();
    state.receiveVote("node2", true);
    ASSERT_TRUE(state.isLeader());

    consensus.receiveAppendEntriesResponse(
        "node2", AppendEntriesResponse{1, true, 7});
    consensus.receiveAppendEntriesResponse(
        "node2", AppendEntriesResponse{1, true, 5});
    consensus.receiveAppendEntriesResponse(
        "node2", AppendEntriesResponse{0, true, 99});

    const auto replicas = consensus.getReplicaStates();
    const auto node2 = std::find_if(
        replicas.begin(), replicas.end(),
        [](const ReplicaState& replica) { return replica.node_id == "node2"; });
    ASSERT_NE(node2, replicas.end());
    EXPECT_EQ(node2->match_index, 7u);
    EXPECT_EQ(node2->next_index, 8u);
}

TEST_F(RaftPersistenceTest, ProposalIndexIsAbsentWithoutQuorum) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1", "node2", "node3"};
    config.raft_config.persistence_path = journal_path_.string();

    {
        RaftConsensus consensus(config);
        EXPECT_FALSE(consensus.proposeWithIndex("not-leader").has_value());
        auto& state = consensus.getRaftState();
        state.becomeCandidate();
        state.receiveVote("node2", true);
        ASSERT_TRUE(state.isLeader());

        EXPECT_FALSE(consensus.proposeWithIndex("unreplicated").has_value());
        EXPECT_EQ(state.getLog().getCommitIndex(), 0u);
        ASSERT_TRUE(state.getLog().getEntry(1).has_value());
    }

    RaftConsensus recovered(config);
    EXPECT_EQ(recovered.getRaftState().getLog().getCommitIndex(), 0u);
    const auto entry = recovered.getRaftState().getLog().getEntry(1);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->command, "unreplicated");
    auto& recovered_state = recovered.getRaftState();
    recovered_state.becomeCandidate();
    recovered_state.receiveVote("node2", true);
    ASSERT_TRUE(recovered_state.isLeader());
    EXPECT_FALSE(recovered.proposeWithIndex("after-restart").has_value());
    EXPECT_FALSE(recovered_state.getLog().getEntry(2).has_value());
}

TEST_F(RaftPersistenceTest, MultiNodeProposalRejectsVolatileLeaderBeforeReplication) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1", "node2", "node3"};

    RaftConsensus consensus(config);
    auto& state = consensus.getRaftState();
    state.becomeCandidate();
    state.receiveVote("node2", true);
    ASSERT_TRUE(state.isLeader());

    int replication_calls = 0;
    consensus.setReplicationCallback([&](const std::string&,
                                        const AppendEntriesRequest& request) {
        ++replication_calls;
        return acknowledgeAppendEntries(request);
    });

    EXPECT_FALSE(consensus.proposeWithIndex("volatile-leader").has_value());
    EXPECT_EQ(replication_calls, 0);
    EXPECT_EQ(state.getLog().getLastLogIndex(), 0u);
    EXPECT_EQ(state.getLog().getCommitIndex(), 0u);
}

TEST_F(RaftPersistenceTest, ProposalRejectsAckThatDoesNotMatchTheRequestedPrefix) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1", "node2", "node3"};
    config.raft_config.persistence_path = journal_path_.string();

    RaftConsensus consensus(config);
    auto& state = consensus.getRaftState();
    state.becomeCandidate();
    state.receiveVote("node2", true);
    ASSERT_TRUE(state.isLeader());
    consensus.setReplicationCallback([](const std::string&,
                                        const AppendEntriesRequest& request) {
        return AppendEntriesResponse{request.term, true, request.prev_log_index};
    });

    EXPECT_FALSE(consensus.proposeWithIndex("missing-match-index").has_value());
    EXPECT_EQ(state.getLog().getCommitIndex(), 0u);
    EXPECT_TRUE(state.getLog().getEntry(1).has_value());
}

TEST_F(RaftPersistenceTest, ProposalCannotCommitAfterLeaderStepsDown) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1", "node2", "node3"};
    config.raft_config.persistence_path = journal_path_.string();

    {
        RaftConsensus consensus(config);
        auto& state = consensus.getRaftState();
        state.becomeCandidate();
        state.receiveVote("node2", true);
        ASSERT_TRUE(state.isLeader());
        consensus.setReplicationCallback([&](const std::string&,
                                              const AppendEntriesRequest& request) {
            return AppendEntriesResponse{request.term + 1, false, 0};
        });

        EXPECT_FALSE(consensus.proposeWithIndex("old-leader").has_value());
        EXPECT_FALSE(state.isLeader());
        EXPECT_EQ(state.getLog().getCommitIndex(), 0u);
    }

    RaftConsensus recovered(config);
    EXPECT_EQ(recovered.getCurrentTerm(), 2u);
    EXPECT_EQ(recovered.getRaftState().getLog().getCommitIndex(), 0u);
}

TEST_F(RaftPersistenceTest, SameTermStepDownAlsoFencesProposal) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1", "node2", "node3"};
    config.raft_config.persistence_path = journal_path_.string();
    RaftConsensus consensus(config);
    auto& state = consensus.getRaftState();
    state.becomeCandidate();
    state.receiveVote("node2", true);
    ASSERT_TRUE(state.isLeader());
    consensus.setReplicationCallback([&](const std::string&,
                                          const AppendEntriesRequest& request) {
        state.becomeFollower(request.term);
        return acknowledgeAppendEntries(request);
    });

    EXPECT_FALSE(consensus.proposeWithIndex("former-leader").has_value());
    EXPECT_EQ(state.getLog().getCommitIndex(), 0u);
}

TEST_F(RaftPersistenceTest, ConcurrentProposalsReturnTheirOwnIndices) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1", "node2", "node3"};
    config.raft_config.persistence_path = journal_path_.string();

    RaftConsensus consensus(config);
    auto& state = consensus.getRaftState();
    state.becomeCandidate();
    state.receiveVote("node2", true);
    ASSERT_TRUE(state.isLeader());

    std::promise<void> first_in_callback;
    auto entered = first_in_callback.get_future();
    std::promise<void> release_first;
    auto released = release_first.get_future();
    consensus.setReplicationCallback([&](const std::string&,
                                          const AppendEntriesRequest& request) {
        if (request.entries.back().command == "first") {
            first_in_callback.set_value();
            released.wait();
        }
        return acknowledgeAppendEntries(request);
    });

    auto first = std::async(std::launch::async, [&] {
        return consensus.proposeWithIndex("first");
    });
    const auto entered_status = entered.wait_for(std::chrono::seconds(5));
    if (entered_status != std::future_status::ready) {
        release_first.set_value();
        FAIL() << "first proposal did not enter the callback";
    }
    const auto second_while_first_is_uncommitted = consensus.proposeWithIndex("second");
    release_first.set_value();
    const auto first_index = first.get();
    ASSERT_TRUE(first_index.has_value());
    EXPECT_FALSE(second_while_first_is_uncommitted.has_value());
    EXPECT_EQ(*first_index, 1u);
    const auto second = consensus.proposeWithIndex("second");
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(*second, 2u);
    const auto first_entry = state.getLog().getEntry(*first_index);
    const auto second_entry = state.getLog().getEntry(*second);
    ASSERT_TRUE(first_entry.has_value());
    ASSERT_TRUE(second_entry.has_value());
    EXPECT_EQ(first_entry->command, "first");
    EXPECT_EQ(second_entry->command, "second");
}

TEST_F(RaftPersistenceTest, FailedEarlierProposalBlocksCommitAcrossUnreplicatedTail) {
    RaftConsensus::Config config;
    config.raft_config.node_id = "node1";
    config.raft_config.cluster_members = {"node1", "node2", "node3"};
    config.raft_config.persistence_path = journal_path_.string();

    {
        RaftConsensus consensus(config);
        auto& state = consensus.getRaftState();
        state.becomeCandidate();
        state.receiveVote("node2", true);
        ASSERT_TRUE(state.isLeader());

        std::promise<void> first_in_callback;
        auto entered = first_in_callback.get_future();
        std::promise<void> release_first;
        auto released = release_first.get_future();
        consensus.setReplicationCallback([&](const std::string& node,
                                              const AppendEntriesRequest& request) {
            if (request.entries.back().command == "first") {
                if (node == "node2") {
                    first_in_callback.set_value();
                    released.wait();
                }
                return AppendEntriesResponse{request.term, false, 0};
            }
            return acknowledgeAppendEntries(request);
        });

        auto first = std::async(std::launch::async, [&] {
            return consensus.proposeWithIndex("first");
        });
        const auto entered_status = entered.wait_for(std::chrono::seconds(5));
        if (entered_status != std::future_status::ready) {
            release_first.set_value();
            FAIL() << "first proposal did not enter the callback";
        }
        const auto second_while_first_is_uncommitted = consensus.proposeWithIndex("second");
        release_first.set_value();
        EXPECT_FALSE(first.get().has_value());
        EXPECT_FALSE(second_while_first_is_uncommitted.has_value());
        EXPECT_FALSE(consensus.proposeWithIndex("second").has_value());
        EXPECT_EQ(state.getLog().getCommitIndex(), 0u);
        EXPECT_TRUE(state.getLog().getEntry(1).has_value());
        EXPECT_FALSE(state.getLog().getEntry(2).has_value());
        EXPECT_FALSE(consensus.isReadOnly());
    }

    RaftConsensus recovered(config);
    EXPECT_EQ(recovered.getRaftState().getLog().getCommitIndex(), 0u);
    const auto entry = recovered.getRaftState().getLog().getEntry(1);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->command, "first");
    auto& recovered_state = recovered.getRaftState();
    recovered_state.becomeCandidate();
    recovered_state.receiveVote("node2", true);
    ASSERT_TRUE(recovered_state.isLeader());
    EXPECT_FALSE(recovered.proposeWithIndex("after-restart").has_value());
    EXPECT_FALSE(recovered_state.getLog().getEntry(2).has_value());
}

TEST_F(RaftPersistenceTest, AppendEntriesAcknowledgesOnlyTheVerifiedDurablePrefix) {
    RaftConfig config;
    config.node_id = "node2";
    config.cluster_members = {"node1", "node2", "node3"};
    config.persistence_path = journal_path_.string();

    {
        RaftState follower(config);
        AppendEntriesRequest append;
        append.term = 1;
        append.leader_id = "node1";
        append.entries = {LogEntry{1, 1, "first"}, LogEntry{1, 2, "second"}};
        append.leader_commit = 1;

        const auto append_response = follower.handleAppendEntries(append);
        ASSERT_TRUE(append_response.success);
        EXPECT_EQ(append_response.match_index, 2u);
        EXPECT_EQ(follower.getLog().getCommitIndex(), 1u);

        AppendEntriesRequest heartbeat;
        heartbeat.term = 1;
        heartbeat.leader_id = "node1";
        heartbeat.prev_log_index = 1;
        heartbeat.prev_log_term = 1;
        heartbeat.leader_commit = 1;

        const auto heartbeat_response = follower.handleAppendEntries(heartbeat);
        ASSERT_TRUE(heartbeat_response.success);
        EXPECT_EQ(heartbeat_response.match_index, 1u);
        EXPECT_NE(heartbeat_response.match_index, follower.getLog().getLastLogIndex());
    }

    RaftState recovered(config);
    EXPECT_EQ(recovered.getLog().getCommitIndex(), 1u);
    EXPECT_TRUE(recovered.getLog().getEntry(1).has_value());
    EXPECT_TRUE(recovered.getLog().getEntry(2).has_value());
}

TEST_F(RaftPersistenceTest, AppendEntriesRejectsNonMemberLeaderBeforeChangingTerm) {
    RaftConfig config;
    config.node_id = "node2";
    config.cluster_members = {"node1", "node2", "node3"};
    config.persistence_path = journal_path_.string();

    RaftState follower(config);
    AppendEntriesRequest request;
    request.term = 3;
    request.leader_id = "unconfigured-node";
    request.entries = {LogEntry{3, 1, "must-not-append"}};

    const auto response = follower.handleAppendEntries(request);

    EXPECT_FALSE(response.success);
    EXPECT_EQ(follower.getCurrentTerm(), 0u);
    EXPECT_EQ(follower.getLog().getLastLogIndex(), 0u);

    request.leader_id.clear();
    request.term = 4;
    const auto empty_leader_response = follower.handleAppendEntries(request);
    EXPECT_FALSE(empty_leader_response.success);
    EXPECT_EQ(follower.getCurrentTerm(), 0u);
    EXPECT_EQ(follower.getLog().getLastLogIndex(), 0u);
}

TEST(RaftAppendEntriesTest, VolatileFollowerDoesNotAcknowledgeReplication) {
    RaftConfig config;
    config.node_id = "node2";
    config.cluster_members = {"node1", "node2", "node3"};

    RaftState follower(config);
    AppendEntriesRequest request;
    request.term = 1;
    request.leader_id = "node1";
    request.entries = {LogEntry{1, 1, "not-durable"}};

    const auto response = follower.handleAppendEntries(request);

    EXPECT_FALSE(response.success);
    EXPECT_EQ(response.term, 0u);
    EXPECT_EQ(follower.getLog().getLastLogIndex(), 0u);
}

TEST_F(RaftPersistenceTest, AppendEntriesRejectsMalformedGapsAndCommittedConflicts) {
    RaftConfig config;
    config.node_id = "node2";
    config.cluster_members = {"node1", "node2", "node3"};
    config.persistence_path = journal_path_.string();

    RaftState follower(config);
    AppendEntriesRequest malformed;
    malformed.term = 1;
    malformed.leader_id = "node1";
    malformed.entries = {LogEntry{1, 1, "first"}, LogEntry{1, 3, "gap"}};
    EXPECT_FALSE(follower.handleAppendEntries(malformed).success);
    EXPECT_EQ(follower.getLog().getLastLogIndex(), 0u);

    AppendEntriesRequest initial;
    initial.term = 1;
    initial.leader_id = "node1";
    initial.entries = {LogEntry{1, 1, "committed"}};
    initial.leader_commit = 1;
    ASSERT_TRUE(follower.handleAppendEntries(initial).success);

    AppendEntriesRequest same_term_conflict = initial;
    same_term_conflict.entries = {LogEntry{1, 1, "different-command"}};
    EXPECT_FALSE(follower.handleAppendEntries(same_term_conflict).success);
    EXPECT_EQ(follower.getLog().getEntry(1)->command, "committed");

    AppendEntriesRequest committed_conflict = initial;
    committed_conflict.term = 2;
    committed_conflict.entries = {LogEntry{2, 1, "conflicting-term"}};
    EXPECT_FALSE(follower.handleAppendEntries(committed_conflict).success);
    EXPECT_EQ(follower.getLog().getCommitIndex(), 1u);
    EXPECT_EQ(follower.getLog().getEntry(1)->command, "committed");
}

TEST_F(RaftPersistenceTest, AppendEntriesRejectsRegressingEntryTerms) {
    RaftConfig config;
    config.node_id = "node2";
    config.cluster_members = {"node1", "node2", "node3"};
    config.persistence_path = journal_path_.string();

    {
        RaftState follower(config);
        AppendEntriesRequest initial;
        initial.term = 1;
        initial.leader_id = "node1";
        initial.entries = {LogEntry{1, 1, "existing"}};
        initial.leader_commit = 1;
        ASSERT_TRUE(follower.handleAppendEntries(initial).success);

        AppendEntriesRequest invalid_empty_anchor;
        invalid_empty_anchor.term = 2;
        invalid_empty_anchor.leader_id = "node1";
        invalid_empty_anchor.prev_log_term = 1;
        EXPECT_FALSE(follower.handleAppendEntries(invalid_empty_anchor).success);

        AppendEntriesRequest below_previous_term;
        below_previous_term.term = 2;
        below_previous_term.leader_id = "node1";
        below_previous_term.prev_log_index = 1;
        below_previous_term.prev_log_term = 1;
        below_previous_term.entries = {LogEntry{0, 2, "term-regression"}};
        EXPECT_FALSE(follower.handleAppendEntries(below_previous_term).success);
        EXPECT_EQ(follower.getLog().getLastLogIndex(), 1u);

        AppendEntriesRequest within_batch_regression;
        within_batch_regression.term = 2;
        within_batch_regression.leader_id = "node1";
        within_batch_regression.prev_log_index = 1;
        within_batch_regression.prev_log_term = 1;
        within_batch_regression.entries = {
            LogEntry{2, 2, "valid-term"}, LogEntry{1, 3, "term-regression"}};
        EXPECT_FALSE(follower.handleAppendEntries(within_batch_regression).success);
        EXPECT_EQ(follower.getLog().getLastLogIndex(), 1u);
        EXPECT_EQ(follower.getLog().getCommitIndex(), 1u);
    }

    RaftState recovered(config);
    EXPECT_EQ(recovered.getLog().getLastLogIndex(), 1u);
    EXPECT_EQ(recovered.getLog().getCommitIndex(), 1u);
    EXPECT_FALSE(recovered.getLog().getEntry(2).has_value());
}

TEST_F(RaftPersistenceTest, SnapshotCompactionRecoversAnchorAndCommitIndex) {
    {
        RaftLog log(journal_path_.string());
        for (uint64_t index = 1; index <= 3; ++index) {
            log.append(LogEntry{1, index, "entry", index});
        }
        log.setCommitIndex(3);
        log.compactUpTo(2, 1);
    }

    RaftLog recovered(journal_path_.string());
    EXPECT_EQ(recovered.getSnapshotIndex(), 2u);
    EXPECT_EQ(recovered.getSnapshotTerm(), 1u);
    EXPECT_EQ(recovered.getCommitIndex(), 3u);
    EXPECT_TRUE(recovered.hasEntry(2, 1));
    EXPECT_FALSE(recovered.getEntry(1).has_value());
    EXPECT_TRUE(recovered.getEntry(3).has_value());
}

TEST_F(RaftPersistenceTest, SnapshotPayloadAndCompactedJournalRecoverTogether) {
    RaftSnapshotManager::Config snapshot_config;
    snapshot_config.snapshot_directory = (directory_ / "snapshots").string();
    const std::vector<uint8_t> snapshot_payload{'s', 't', 'a', 't', 'e'};
    {
        RaftLog log(journal_path_.string());
        log.append(LogEntry{1, 1, "entry-1", 1});
        log.append(LogEntry{1, 2, std::string(4096, 'x'), 2});
        log.setCommitIndex(2);
        RaftSnapshotManager snapshots(snapshot_config);
        ASSERT_TRUE(snapshots.createAndInstall(log, 1, 1, snapshot_payload));
    }

    RaftLog recovered(journal_path_.string());
    EXPECT_EQ(recovered.getSnapshotIndex(), 1u);
    EXPECT_EQ(recovered.getCommitIndex(), 2u);
    EXPECT_FALSE(recovered.getEntry(1).has_value());
    ASSERT_TRUE(recovered.getEntry(2).has_value());
    EXPECT_EQ(recovered.getEntry(2)->command.size(), 4096u);

    RaftSnapshotManager recovered_snapshots(snapshot_config);
    const auto snapshot = recovered_snapshots.loadLatestSnapshot();
    ASSERT_TRUE(snapshot.has_value());
    EXPECT_EQ(snapshot->snapshot_index, 1u);
    EXPECT_EQ(snapshot->uncompressed_size, snapshot_payload.size());
    EXPECT_EQ(snapshot->checksum,
              "4ba69735ca53765ed6a709edb56c6ea236b7193a3b29a6b390c346f0f4340e4e");
}

TEST_F(RaftPersistenceTest, SnapshotMetadataDoesNotCompactUntilCompactionIsRequested) {
    {
        RaftLog log(journal_path_.string());
        log.append(LogEntry{1, 1, "entry-1", 1});
        log.append(LogEntry{1, 2, "entry-2", 2});
        log.setCommitIndex(2);
        log.setSnapshotMeta(1, 1);
        ASSERT_TRUE(log.getEntry(1).has_value());
    }

    RaftLog recovered(journal_path_.string());
    EXPECT_EQ(recovered.getSnapshotIndex(), 1u);
    EXPECT_TRUE(recovered.getEntry(1).has_value());
}

TEST_F(RaftPersistenceTest, TruncationRecoversWithoutDiscardingCommittedEntries) {
    {
        RaftLog log(journal_path_.string());
        log.append(LogEntry{1, 1, "committed", 1});
        log.append(LogEntry{1, 2, "obsolete", 2});
        log.setCommitIndex(1);
        log.truncateFrom(2);
    }

    RaftLog recovered(journal_path_.string());
    EXPECT_EQ(recovered.getCommitIndex(), 1u);
    EXPECT_TRUE(recovered.getEntry(1).has_value());
    EXPECT_FALSE(recovered.getEntry(2).has_value());
    EXPECT_THROW(recovered.truncateFrom(1), std::runtime_error);
}

TEST_F(RaftPersistenceTest, JournalHasExclusiveProcessOwnership) {
    auto owner = std::make_unique<RaftLog>(journal_path_.string());
    EXPECT_THROW(RaftLog(journal_path_.string()), std::runtime_error);
    owner.reset();
    EXPECT_NO_THROW(RaftLog(journal_path_.string()));
}

TEST_F(RaftPersistenceTest, CorruptJournalFailsClosed) {
    {
        std::ofstream corrupt(journal_path_, std::ios::binary);
        corrupt << "E|1|1|0|636d64|0000\n";
    }

    EXPECT_THROW(RaftLog(journal_path_.string()), std::runtime_error);
}

TEST_F(RaftPersistenceTest, IncompleteFinalRecordFailsClosed) {
    {
        std::ofstream incomplete(journal_path_, std::ios::binary);
        incomplete << "E|1|1|0|636d64";
    }

    EXPECT_THROW(RaftLog(journal_path_.string()), std::runtime_error);
}

TEST_F(RaftPersistenceTest, JournalPathThatIsDirectoryFailsClosed) {
    EXPECT_THROW(RaftLog(directory_.string()), std::runtime_error);
}

} // namespace
