#include <gtest/gtest.h>
#include "utils/consistent_hash.h"

#include <set>
#include <string>

using namespace themis::utils;

// ============================================================================
// Basic API
// ============================================================================

TEST(ConsistentHashRing, EmptyRingReturnsEmptyOnGetNode) {
    ConsistentHashRing ring;
    EXPECT_TRUE(ring.getNode("key").empty());
}

TEST(ConsistentHashRing, SingleNodeReturnsThatNode) {
    ConsistentHashRing ring;
    ring.addNode("node1");
    EXPECT_EQ(ring.getNode("anything"), "node1");
}

TEST(ConsistentHashRing, AddAndRemoveNode) {
    ConsistentHashRing ring;
    ring.addNode("node1");
    ring.addNode("node2");
    EXPECT_EQ(ring.nodeCount(), 2u);
    ring.removeNode("node1");
    EXPECT_EQ(ring.nodeCount(), 1u);
    EXPECT_EQ(ring.getNode("key"), "node2");
}

TEST(ConsistentHashRing, SameKeyAlwaysMapsToSameNode) {
    ConsistentHashRing ring;
    ring.addNode("nodeA");
    ring.addNode("nodeB");
    ring.addNode("nodeC");

    const std::string key = "some_routing_key";
    const std::string first = ring.getNode(key);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(ring.getNode(key), first);
    }
}

TEST(ConsistentHashRing, GetNNodesReturnsDistinctNodes) {
    ConsistentHashRing ring;
    ring.addNode("n1");
    ring.addNode("n2");
    ring.addNode("n3");

    auto nodes = ring.getNodes("key", 3);
    ASSERT_EQ(nodes.size(), 3u);
    std::set<std::string> unique(nodes.begin(), nodes.end());
    EXPECT_EQ(unique.size(), 3u);
}

TEST(ConsistentHashRing, GetNNodesCappedAtPhysicalNodeCount) {
    ConsistentHashRing ring;
    ring.addNode("n1");
    ring.addNode("n2");

    // Requesting more nodes than available should return only 2
    auto nodes = ring.getNodes("key", 5);
    EXPECT_EQ(nodes.size(), 2u);
}

TEST(ConsistentHashRing, EmptyCheck) {
    ConsistentHashRing ring;
    EXPECT_TRUE(ring.empty());
    ring.addNode("n1");
    EXPECT_FALSE(ring.empty());
    ring.removeNode("n1");
    EXPECT_TRUE(ring.empty());
}
