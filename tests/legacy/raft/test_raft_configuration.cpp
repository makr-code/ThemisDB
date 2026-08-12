#include <gtest/gtest.h>
#include "sharding/raft_configuration.h"

using themis::sharding::RaftConfiguration;
using themis::sharding::ConfigurationEntry;

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(RaftConfiguration, DefaultConstructorNotInJointConsensus) {
    RaftConfiguration cfg;
    EXPECT_FALSE(cfg.isJointConsensus());
    EXPECT_FALSE(cfg.isInTransition());
    EXPECT_TRUE(cfg.getMembers().empty());
}

TEST(RaftConfiguration, ConstructWithMembers) {
    RaftConfiguration cfg({"node1", "node2", "node3"});
    EXPECT_FALSE(cfg.isJointConsensus());
    EXPECT_EQ(cfg.getMembers().size(), 3u);
    EXPECT_TRUE(cfg.isMember("node1"));
    EXPECT_TRUE(cfg.isMember("node2"));
    EXPECT_TRUE(cfg.isMember("node3"));
    EXPECT_FALSE(cfg.isMember("node4"));
}

// ---------------------------------------------------------------------------
// addNode – joint consensus phase
// ---------------------------------------------------------------------------

TEST(RaftConfiguration, AddNodeEntersJointConsensus) {
    RaftConfiguration cfg({"node1", "node2", "node3"});
    cfg.addNode("node4");

    EXPECT_TRUE(cfg.isJointConsensus());
    EXPECT_TRUE(cfg.isInTransition());
}

TEST(RaftConfiguration, AddNodeNewMemberVisible) {
    RaftConfiguration cfg({"node1", "node2", "node3"});
    cfg.addNode("node4");

    // isMember must return true for both old and new members
    EXPECT_TRUE(cfg.isMember("node1"));
    EXPECT_TRUE(cfg.isMember("node4"));

    // getNewMembers includes the added node
    auto new_members = cfg.getNewMembers();
    EXPECT_GT(new_members.count("node4"), 0u);

    // getOldMembers does NOT include the added node
    auto old_members = cfg.getOldMembers();
    EXPECT_EQ(old_members.count("node4"), 0u);
    EXPECT_EQ(old_members.count("node1"), 1u);
}

TEST(RaftConfiguration, AddNodeGetMembersReturnsUnion) {
    RaftConfiguration cfg({"node1", "node2"});
    cfg.addNode("node3");

    auto all = cfg.getMembers();
    EXPECT_EQ(all.size(), 3u);
    EXPECT_GT(all.count("node1"), 0u);
    EXPECT_GT(all.count("node2"), 0u);
    EXPECT_GT(all.count("node3"), 0u);
}

// ---------------------------------------------------------------------------
// removeNode – joint consensus phase
// ---------------------------------------------------------------------------

TEST(RaftConfiguration, RemoveNodeEntersJointConsensus) {
    RaftConfiguration cfg({"node1", "node2", "node3"});
    cfg.removeNode("node3");

    EXPECT_TRUE(cfg.isJointConsensus());
}

TEST(RaftConfiguration, RemoveNodeStillMemberDuringTransition) {
    RaftConfiguration cfg({"node1", "node2", "node3"});
    cfg.removeNode("node3");

    // During joint consensus the removed node is still a member (in old config)
    EXPECT_TRUE(cfg.isMember("node3"));

    // But it is absent from the new configuration
    auto new_members = cfg.getNewMembers();
    EXPECT_EQ(new_members.count("node3"), 0u);
}

// ---------------------------------------------------------------------------
// Concurrent-change guard
// ---------------------------------------------------------------------------

TEST(RaftConfiguration, SecondAddNodeWhileInTransitionThrows) {
    RaftConfiguration cfg({"node1", "node2", "node3"});
    cfg.addNode("node4");
    EXPECT_THROW(cfg.addNode("node5"), std::runtime_error);
}

TEST(RaftConfiguration, RemoveNodeWhileInTransitionThrows) {
    RaftConfiguration cfg({"node1", "node2", "node3"});
    cfg.addNode("node4");
    EXPECT_THROW(cfg.removeNode("node1"), std::runtime_error);
}

// ---------------------------------------------------------------------------
// applyConfiguration – finalise joint consensus
// ---------------------------------------------------------------------------

TEST(RaftConfiguration, ApplyConfigurationFinalisesToCNew) {
    RaftConfiguration cfg({"node1", "node2", "node3"});
    cfg.addNode("node4");

    ConfigurationEntry c_new;
    c_new.old_members       = {};
    c_new.new_members       = {"node1", "node2", "node3", "node4"};
    c_new.is_joint_consensus = false;
    cfg.applyConfiguration(c_new);

    EXPECT_FALSE(cfg.isJointConsensus());
    EXPECT_TRUE(cfg.isMember("node4"));
    EXPECT_EQ(cfg.getMembers().size(), 4u);
}

TEST(RaftConfiguration, ApplyConfigurationCanSetJointConsensus) {
    RaftConfiguration cfg({"node1", "node2", "node3"});

    ConfigurationEntry joint;
    joint.old_members        = {"node1", "node2", "node3"};
    joint.new_members        = {"node1", "node2", "node3", "node4"};
    joint.is_joint_consensus = true;
    cfg.applyConfiguration(joint);

    EXPECT_TRUE(cfg.isJointConsensus());
    EXPECT_TRUE(cfg.isMember("node4"));
}

// ---------------------------------------------------------------------------
// hasQuorum – simple configuration
// ---------------------------------------------------------------------------

TEST(RaftConfiguration, HasQuorumSimpleMajority) {
    RaftConfiguration cfg({"node1", "node2", "node3"});  // quorum = 2

    EXPECT_TRUE(cfg.hasQuorum({"node1", "node2"}));
    EXPECT_TRUE(cfg.hasQuorum({"node1", "node2", "node3"}));
    EXPECT_FALSE(cfg.hasQuorum({"node1"}));
    EXPECT_FALSE(cfg.hasQuorum({}));
}

TEST(RaftConfiguration, HasQuorumSingleNode) {
    RaftConfiguration cfg({"node1"});  // quorum = 1
    EXPECT_TRUE(cfg.hasQuorum({"node1"}));
    EXPECT_FALSE(cfg.hasQuorum({}));
}

// ---------------------------------------------------------------------------
// hasQuorum – joint consensus requires majority in BOTH configs
// ---------------------------------------------------------------------------

TEST(RaftConfiguration, HasQuorumJointConsensusRequiresBothMajorities) {
    // C_old = {n1, n2, n3}, quorum_old = 2
    // C_new = {n1, n2, n3, n4}, quorum_new = 3
    RaftConfiguration cfg({"node1", "node2", "node3"});
    cfg.addNode("node4");

    // Only old quorum met (n1, n2) – fails because new quorum (3) not met
    EXPECT_FALSE(cfg.hasQuorum({"node1", "node2"}));

    // Both quorums met (n1, n2, n3)
    EXPECT_TRUE(cfg.hasQuorum({"node1", "node2", "node3"}));

    // All four – still true
    EXPECT_TRUE(cfg.hasQuorum({"node1", "node2", "node3", "node4"}));
}

// ---------------------------------------------------------------------------
// getQuorumSize
// ---------------------------------------------------------------------------

TEST(RaftConfiguration, QuorumSizeThreeNodes) {
    RaftConfiguration cfg({"node1", "node2", "node3"});
    EXPECT_EQ(cfg.getQuorumSize(), 2u);  // (3/2)+1 = 2
}

TEST(RaftConfiguration, QuorumSizeFiveNodes) {
    RaftConfiguration cfg({"n1", "n2", "n3", "n4", "n5"});
    EXPECT_EQ(cfg.getQuorumSize(), 3u);  // (5/2)+1 = 3
}

TEST(RaftConfiguration, QuorumSizeJointConsensusIsMax) {
    // C_old = 3 nodes (quorum=2), C_new = 5 nodes (quorum=3) → max = 3
    RaftConfiguration cfg({"node1", "node2", "node3"});
    cfg.addNode("node4");
    cfg.applyConfiguration({
        {"node1", "node2", "node3"},
        {"node1", "node2", "node3", "node4", "node5"},
        true
    });
    EXPECT_EQ(cfg.getQuorumSize(), 3u);  // max(2, 3) = 3
}

