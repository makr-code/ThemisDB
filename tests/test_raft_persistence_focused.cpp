#include "sharding/raft_consensus.h"
#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>
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
