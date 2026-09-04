/**
 * ThemisDB Replication – Raft v2 / Joint Consensus Tests
 *
 * Covers:
 *  1. RaftV2ClusterConfig – basic construction, quorum calculation
 *  2. RaftV2ClusterConfig – joint consensus activation (add member)
 *  3. RaftV2ClusterConfig – joint consensus activation (remove member)
 *  4. RaftV2ClusterConfig – quorum requires majority in BOTH old and new
 *     during joint consensus
 *  5. RaftV2ClusterConfig – commitTransition collapses to new config
 *  6. RaftV2ClusterConfig – rollbackTransition restores old config
 *  7. RaftV2ClusterConfig – error on concurrent changes
 *  8. RaftV2ClusterConfig – error removing the last member
 *  9. MembershipChangeManager – proposeAdd creates JOINT entry
 * 10. MembershipChangeManager – onJointCommitted writes COMMIT entry
 * 11. MembershipChangeManager – onNewConfigCommitted finalises transition
 * 12. MembershipChangeManager – proposeRemove creates JOINT entry
 * 13. MembershipChangeManager – applyEntry drives follower config (add)
 * 14. MembershipChangeManager – applyEntry drives follower config (remove)
 * 15. MembershipChangeManager – applyEntry idempotent when already in joint
 * 16. MembershipChangeManager – concurrent change rejected
 * 17. RaftV2State – hasVoted semantics
 */

#include <gtest/gtest.h>

#include "replication/raft_v2.h"
#include "replication/replication_manager.h"

#include <filesystem>
#include <memory>
#include <set>
#include <string>

using namespace themisdb::replication;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::set<std::string> makeMembers(
    std::initializer_list<const char*> ids)
{
    std::set<std::string> s = {};

    for (const auto* id : ids) {
      s.insert(id);
    }
    return s;
}

struct TempWALDir {
    std::string path = {};
    explicit TempWALDir(const std::string& p) : path(p) {
        std::filesystem::remove_all(p);
        std::filesystem::create_directories(p);
    }
    ~TempWALDir() { std::filesystem::remove_all(path); }
};

static std::shared_ptr<WALManager> makeWAL(const std::string& dir) {
    ReplicationConfig cfg;
    cfg.wal_directory = dir;
    return std::make_shared<WALManager>(cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// 1. RaftV2ClusterConfig – basic construction and quorum
// ─────────────────────────────────────────────────────────────────────────────

class RaftV2ConfigBasicTest : public ::testing::Test {};

TEST_F(RaftV2ConfigBasicTest, ConstructionSetsMembers) {
    RaftV2ClusterConfig cfg(makeMembers({"A", "B", "C"}));
    EXPECT_FALSE(cfg.isInJointConsensus());
    EXPECT_TRUE(cfg.isMember("A"));
    EXPECT_TRUE(cfg.isMember("B"));
    EXPECT_FALSE(cfg.isMember("X"));
    EXPECT_EQ(cfg.getAllMembers().size(), 3u);
}

TEST_F(RaftV2ConfigBasicTest, QuorumSizeThreeNode) {
    RaftV2ClusterConfig cfg(makeMembers({"A", "B", "C"}));
    EXPECT_EQ(cfg.quorumSize(), 2u);  // (3/2)+1 = 2
}

TEST_F(RaftV2ConfigBasicTest, QuorumSizeFiveNode) {
    RaftV2ClusterConfig cfg(makeMembers({"A", "B", "C", "D", "E"}));
    EXPECT_EQ(cfg.quorumSize(), 3u);  // (5/2)+1 = 3
}

TEST_F(RaftV2ConfigBasicTest, StableQuorumGrantedWhenMajorityVotes) {
    RaftV2ClusterConfig cfg(makeMembers({"A", "B", "C"}));
    EXPECT_TRUE(cfg.hasQuorum({"A", "B"}));
}

TEST_F(RaftV2ConfigBasicTest, StableQuorumDeniedWhenMinorityVotes) {
    RaftV2ClusterConfig cfg(makeMembers({"A", "B", "C"}));
    EXPECT_FALSE(cfg.hasQuorum({"A"}));
}

TEST_F(RaftV2ConfigBasicTest, EmptyConfigQuorumIsOne) {
    RaftV2ClusterConfig cfg(makeMembers({"A"}));
    EXPECT_EQ(cfg.quorumSize(), 1u);
    EXPECT_TRUE(cfg.hasQuorum({"A"}));
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Joint consensus: add member
// ─────────────────────────────────────────────────────────────────────────────

class RaftV2JointConsensusAddTest : public ::testing::Test {
protected:
    RaftV2ClusterConfig cfg_{makeMembers({"A", "B", "C"})};
};

TEST_F(RaftV2JointConsensusAddTest, BeginAddEntersJointConsensus) {
    cfg_.beginAddMember("D");
    EXPECT_TRUE(cfg_.isInJointConsensus());
}

TEST_F(RaftV2JointConsensusAddTest, NewMemberIsVisible) {
    cfg_.beginAddMember("D");
    EXPECT_TRUE(cfg_.isMember("D"));
    EXPECT_EQ(cfg_.getNewMembers().size(), 4u);
}

TEST_F(RaftV2JointConsensusAddTest, OldMembersPreservedDuringTransition) {
    cfg_.beginAddMember("D");
    auto old = cfg_.getOldMembers();
    EXPECT_EQ(old.size(), 3u);
    EXPECT_TRUE(old.count("A"));
    EXPECT_TRUE(old.count("B"));
    EXPECT_TRUE(old.count("C"));
}

TEST_F(RaftV2JointConsensusAddTest, AllMembersUnionDuringJoint) {
    cfg_.beginAddMember("D");
    auto all = cfg_.getAllMembers();
    EXPECT_EQ(all.size(), 4u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Joint consensus: remove member
// ─────────────────────────────────────────────────────────────────────────────

class RaftV2JointConsensusRemoveTest : public ::testing::Test {
protected:
    RaftV2ClusterConfig cfg_{makeMembers({"A", "B", "C", "D", "E"})};
};

TEST_F(RaftV2JointConsensusRemoveTest, BeginRemoveEntersJointConsensus) {
    cfg_.beginRemoveMember("E");
    EXPECT_TRUE(cfg_.isInJointConsensus());
}

TEST_F(RaftV2JointConsensusRemoveTest, RemovedMemberAbsentFromNewConfig) {
    cfg_.beginRemoveMember("E");
    EXPECT_EQ(cfg_.getNewMembers().size(), 4u);
    EXPECT_FALSE(cfg_.getNewMembers().count("E"));
}

TEST_F(RaftV2JointConsensusRemoveTest, RemovedMemberStillInOldConfig) {
    cfg_.beginRemoveMember("E");
    EXPECT_TRUE(cfg_.getOldMembers().count("E"));
}

TEST_F(RaftV2JointConsensusRemoveTest, RemovedMemberStillReachableViaMember) {
    // During joint consensus the removed node can still vote in old config
    cfg_.beginRemoveMember("E");
    EXPECT_TRUE(cfg_.isMember("E"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Joint consensus quorum requires majority in BOTH configurations
// ─────────────────────────────────────────────────────────────────────────────

class RaftV2JointQuorumTest : public ::testing::Test {
protected:
    RaftV2ClusterConfig cfg_{makeMembers({"A", "B", "C"})};

    void SetUp() override {
        cfg_.beginAddMember("D");
        // old: {A,B,C}  new: {A,B,C,D}
    }
};

TEST_F(RaftV2JointQuorumTest, QuorumGrantedWithBothMajorities) {
    // old majority: 2/3 (A,B) AND new majority: 3/4 (A,B,D)
    EXPECT_TRUE(cfg_.hasQuorum({"A", "B", "D"}));
}

TEST_F(RaftV2JointQuorumTest, QuorumDeniedWithOldMajorityMissingNewMajority) {
    // old majority: A,B but new needs 3 of 4 → only A,B = 2 < 3 needed
    EXPECT_FALSE(cfg_.hasQuorum({"A", "B"}));
}

TEST_F(RaftV2JointQuorumTest, QuorumDeniedWithNewMajorityMissingOldMajority) {
    // new majority: A,B,C,D has A,B,D = 3 ≥ 3, but old has only A = 1 < 2
    EXPECT_FALSE(cfg_.hasQuorum({"A", "D"}));
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. Commit transition
// ─────────────────────────────────────────────────────────────────────────────

TEST(RaftV2CommitTransitionTest, CommitCollapsesToNewConfig) {
    RaftV2ClusterConfig cfg(makeMembers({"A", "B", "C"}));
    cfg.beginAddMember("D");
    cfg.commitTransition();

    EXPECT_FALSE(cfg.isInJointConsensus());
    EXPECT_EQ(cfg.getNewMembers().size(), 4u);
    EXPECT_TRUE(cfg.getOldMembers().empty());
}

TEST(RaftV2CommitTransitionTest, CommitWithoutTransitionThrows) {
    RaftV2ClusterConfig cfg(makeMembers({"A", "B", "C"}));
    EXPECT_THROW(cfg.commitTransition(), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. Rollback transition
// ─────────────────────────────────────────────────────────────────────────────

TEST(RaftV2RollbackTransitionTest, RollbackRestoresOldConfig) {
    RaftV2ClusterConfig cfg(makeMembers({"A", "B", "C"}));
    cfg.beginAddMember("D");
    cfg.rollbackTransition();

    EXPECT_FALSE(cfg.isInJointConsensus());
    EXPECT_EQ(cfg.getNewMembers().size(), 3u);
    EXPECT_FALSE(cfg.isMember("D"));
}

TEST(RaftV2RollbackTransitionTest, RollbackWithoutTransitionThrows) {
    RaftV2ClusterConfig cfg(makeMembers({"A", "B", "C"}));
    EXPECT_THROW(cfg.rollbackTransition(), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. Concurrent change rejected
// ─────────────────────────────────────────────────────────────────────────────

TEST(RaftV2ErrorTest, ConcurrentChangeRejected) {
    RaftV2ClusterConfig cfg(makeMembers({"A", "B", "C"}));
    cfg.beginAddMember("D");
    EXPECT_THROW(cfg.beginAddMember("E"), std::runtime_error);
    EXPECT_THROW(cfg.beginRemoveMember("A"), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. Cannot remove the last member
// ─────────────────────────────────────────────────────────────────────────────

TEST(RaftV2ErrorTest, CannotRemoveLastMember) {
    RaftV2ClusterConfig cfg(makeMembers({"A"}));
    EXPECT_THROW(cfg.beginRemoveMember("A"), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. MembershipChangeManager – proposeAdd
// ─────────────────────────────────────────────────────────────────────────────

class MCMTest : public ::testing::Test {
protected:
    TempWALDir wd_{"/tmp/themis_raft_v2_test"};
    std::shared_ptr<RaftV2ClusterConfig> config_;
    std::unique_ptr<MembershipChangeManager> mgr_;

    void SetUp() override {
        config_ = std::make_shared<RaftV2ClusterConfig>(
            makeMembers({"node1", "node2", "node3"}));
        auto wal = makeWAL(wd_.path);
        mgr_ = std::make_unique<MembershipChangeManager>(
            config_, "node1", std::move(wal));
    }
};

TEST_F(MCMTest, ProposeAddCreatesJointEntry) {
    auto entry = mgr_->proposeAdd("node4");
    EXPECT_EQ(entry.phase, MembershipChangeEntry::Phase::JOINT);
    EXPECT_TRUE(entry.old_members.count("node1"));
    EXPECT_TRUE(entry.new_members.count("node4"));
    EXPECT_TRUE(mgr_->isChangeInProgress());
}

TEST_F(MCMTest, ProposeAddActivatesJointConsensus) {
    mgr_->proposeAdd("node4");
    EXPECT_TRUE(config_->isInJointConsensus());
}

// ─────────────────────────────────────────────────────────────────────────────
// 10. MembershipChangeManager – onJointCommitted
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MCMTest, OnJointCommittedWritesCommitEntry) {
    auto joint_entry = mgr_->proposeAdd("node4");
    mgr_->onJointCommitted(joint_entry.log_index);

    auto pending = mgr_->pendingEntry();
    ASSERT_TRUE(pending.has_value());
    EXPECT_EQ(pending->phase, MembershipChangeEntry::Phase::COMMIT);
}

TEST_F(MCMTest, OnJointCommittedIgnoresMismatch) {
    mgr_->proposeAdd("node4");
    mgr_->onJointCommitted(9999u);  // wrong index – ignored
    auto pending = mgr_->pendingEntry();
    ASSERT_TRUE(pending.has_value());
    EXPECT_EQ(pending->phase, MembershipChangeEntry::Phase::JOINT);
}

// ─────────────────────────────────────────────────────────────────────────────
// 11. MembershipChangeManager – onNewConfigCommitted
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MCMTest, OnNewConfigCommittedFinalisesTransition) {
    auto joint = mgr_->proposeAdd("node4");
    mgr_->onJointCommitted(joint.log_index);
    mgr_->onNewConfigCommitted();

    EXPECT_FALSE(mgr_->isChangeInProgress());
    EXPECT_FALSE(config_->isInJointConsensus());
    EXPECT_TRUE(config_->isMember("node4"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 12. MembershipChangeManager – proposeRemove
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MCMTest, ProposeRemoveCreatesJointEntry) {
    auto entry = mgr_->proposeRemove("node3");
    EXPECT_EQ(entry.phase, MembershipChangeEntry::Phase::JOINT);
    EXPECT_TRUE(entry.old_members.count("node3"));
    EXPECT_FALSE(entry.new_members.count("node3"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 13. MembershipChangeManager – applyEntry drives follower config
// ─────────────────────────────────────────────────────────────────────────────

class MCMFollowerTest : public ::testing::Test {
protected:
    TempWALDir wd_{"/tmp/themis_raft_v2_follower_test"};
    std::shared_ptr<RaftV2ClusterConfig> config_;
    std::unique_ptr<MembershipChangeManager> follower_mgr_;

    void SetUp() override {
        config_ = std::make_shared<RaftV2ClusterConfig>(
            makeMembers({"node1", "node2", "node3"}));
        auto wal = makeWAL(wd_.path);
        follower_mgr_ = std::make_unique<MembershipChangeManager>(
            config_, "node2", std::move(wal));
    }
};

TEST_F(MCMFollowerTest, ApplyJointEntryActivatesJointConsensus) {
    // Simulate a JOINT entry arriving from the leader
    MembershipChangeEntry entry;
    entry.phase       = MembershipChangeEntry::Phase::JOINT;
    entry.old_members = makeMembers({"node1", "node2", "node3"});
    entry.new_members = makeMembers({"node1", "node2", "node3", "node4"});
    entry.log_index   = 42;

    follower_mgr_->applyEntry(entry);

    // After applying the JOINT entry the config must be in joint consensus
    EXPECT_TRUE(config_->isInJointConsensus());
    // The new member must be visible
    EXPECT_TRUE(config_->isMember("node4"));
    // Change must be tracked as in-progress
    EXPECT_TRUE(follower_mgr_->isChangeInProgress());
    auto pending = follower_mgr_->pendingEntry();
    ASSERT_TRUE(pending.has_value());
    EXPECT_EQ(pending->phase, MembershipChangeEntry::Phase::JOINT);
}

TEST_F(MCMFollowerTest, ApplyCommitEntryFinalisesTransition) {
    // First apply JOINT to enter joint consensus
    MembershipChangeEntry joint;
    joint.phase       = MembershipChangeEntry::Phase::JOINT;
    joint.old_members = makeMembers({"node1", "node2", "node3"});
    joint.new_members = makeMembers({"node1", "node2", "node3", "node4"});
    joint.log_index   = 42;
    follower_mgr_->applyEntry(joint);

    // Now apply COMMIT to finalise
    MembershipChangeEntry commit;
    commit.phase       = MembershipChangeEntry::Phase::COMMIT;
    commit.old_members = joint.old_members;
    commit.new_members = joint.new_members;
    commit.log_index   = 43;
    follower_mgr_->applyEntry(commit);

    // After COMMIT the joint consensus should be resolved
    EXPECT_FALSE(config_->isInJointConsensus());
    EXPECT_FALSE(follower_mgr_->isChangeInProgress());
    // node4 remains a member in the new stable config
    EXPECT_TRUE(config_->isMember("node4"));
    // Old members still present
    EXPECT_TRUE(config_->isMember("node1"));
}

TEST_F(MCMFollowerTest, ApplyJointEntryForRemove) {
    // Simulate follower receiving a JOINT entry for a remove operation
    MembershipChangeEntry entry;
    entry.phase       = MembershipChangeEntry::Phase::JOINT;
    entry.old_members = makeMembers({"node1", "node2", "node3"});
    entry.new_members = makeMembers({"node1", "node2"});
    entry.log_index   = 55;

    follower_mgr_->applyEntry(entry);

    EXPECT_TRUE(config_->isInJointConsensus());
    // node3 must be visible as old member during transition
    EXPECT_TRUE(config_->isMember("node3"));
    // but absent from new config
    EXPECT_FALSE(config_->getNewMembers().count("node3"));
}

TEST_F(MCMFollowerTest, ApplyEntryIdempotentWhenAlreadyInJoint) {
    MembershipChangeEntry entry;
    entry.phase       = MembershipChangeEntry::Phase::JOINT;
    entry.old_members = makeMembers({"node1", "node2", "node3"});
    entry.new_members = makeMembers({"node1", "node2", "node3", "node4"});
    entry.log_index   = 42;

    follower_mgr_->applyEntry(entry);
    // Applying the same JOINT entry again must not throw
    EXPECT_NO_THROW(follower_mgr_->applyEntry(entry));
    // Still in joint consensus (no double-transition)
    EXPECT_TRUE(config_->isInJointConsensus());
}

// ─────────────────────────────────────────────────────────────────────────────
// 14. MembershipChangeManager – concurrent change rejected
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MCMTest, ConcurrentProposalRejected) {
    mgr_->proposeAdd("node4");
    EXPECT_THROW(mgr_->proposeAdd("node5"), std::runtime_error);
    EXPECT_THROW(mgr_->proposeRemove("node1"), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// 15. RaftV2State
// ─────────────────────────────────────────────────────────────────────────────

TEST(RaftV2StateTest, HasVotedReturnsFalseWhenVotedForEmpty) {
    RaftV2State state;
    EXPECT_FALSE(state.hasVoted());
}

TEST(RaftV2StateTest, HasVotedReturnsTrueAfterVote) {
    RaftV2State state;
    state.voted_for = "node1";
    EXPECT_TRUE(state.hasVoted());
}

TEST(RaftV2StateTest, DefaultStateIsZero) {
    RaftV2State state;
    EXPECT_EQ(state.current_term, 0u);
    EXPECT_EQ(state.commit_index, 0u);
    EXPECT_EQ(state.last_applied, 0u);
}
