#include <gtest/gtest.h>
#include "themis/gpu/cluster_coordinator.h"

#include <chrono>
#include <thread>

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ClusterConfig MakeStandaloneConfig() {
    ClusterConfig cfg;
    cfg.enabled = false;
    cfg.mode    = ClusterConfig::Mode::STANDALONE;
    return cfg;
}

static ClusterConfig MakeCoordinatorConfig() {
    ClusterConfig cfg;
    cfg.enabled                = true;
    cfg.mode                   = ClusterConfig::Mode::COORDINATOR;
    cfg.coordinator_address    = "192.168.1.100:9000";
    cfg.node_id                = "coordinator";
    cfg.heartbeat_interval_ms  = 1000;
    cfg.node_timeout_ms        = 3000;
    cfg.nodes = {
        {"node-1", "192.168.1.101:9000", 4},
        {"node-2", "192.168.1.102:9000", 4},
    };
    return cfg;
}

static ClusterConfig MakeWorkerConfig() {
    ClusterConfig cfg;
    cfg.enabled               = true;
    cfg.mode                  = ClusterConfig::Mode::WORKER;
    cfg.coordinator_address   = "192.168.1.100:9000";
    cfg.node_id               = "node-1";
    cfg.heartbeat_interval_ms = 1000;
    cfg.node_timeout_ms       = 3000;
    return cfg;
}

// ---------------------------------------------------------------------------
// STANDALONE mode
// ---------------------------------------------------------------------------

TEST(GPUClusterCoordinatorTest, Standalone_Initialize_Succeeds) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    EXPECT_TRUE(c.initialize(MakeStandaloneConfig()));
    EXPECT_TRUE(c.isInitialized());
}

TEST(GPUClusterCoordinatorTest, Standalone_IsCoordinator) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeStandaloneConfig());
    EXPECT_TRUE(c.isCoordinator());
}

TEST(GPUClusterCoordinatorTest, Standalone_HasOneOnlineNode) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeStandaloneConfig());
    EXPECT_EQ(c.totalNodes(), 1u);
    EXPECT_EQ(c.onlineNodeCount(), 1u);
}

TEST(GPUClusterCoordinatorTest, Standalone_SelectNode_ReturnsLocalNode) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeStandaloneConfig());
    const auto* node = c.selectNode();
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->status, GPUClusterCoordinator::NodeStatus::ONLINE);
}

// ---------------------------------------------------------------------------
// COORDINATOR mode — initialize seeds nodes as OFFLINE
// ---------------------------------------------------------------------------

TEST(GPUClusterCoordinatorTest, Coordinator_Initialize_SeedsNodes) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());
    EXPECT_EQ(c.totalNodes(), 2u);
}

TEST(GPUClusterCoordinatorTest, Coordinator_Initialize_NodesStartOffline) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());
    // All seeded nodes start OFFLINE until first heartbeat arrives.
    EXPECT_EQ(c.onlineNodeCount(), 0u);
}

TEST(GPUClusterCoordinatorTest, Coordinator_IsCoordinator_ReturnsTrue) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());
    EXPECT_TRUE(c.isCoordinator());
}

TEST(GPUClusterCoordinatorTest, Coordinator_SelectNode_NullWhenAllOffline) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());
    EXPECT_EQ(c.selectNode(), nullptr);
}

// ---------------------------------------------------------------------------
// WORKER mode
// ---------------------------------------------------------------------------

TEST(GPUClusterCoordinatorTest, Worker_Initialize_Succeeds) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    EXPECT_TRUE(c.initialize(MakeWorkerConfig()));
}

TEST(GPUClusterCoordinatorTest, Worker_IsCoordinator_ReturnsFalse) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeWorkerConfig());
    EXPECT_FALSE(c.isCoordinator());
}

// ---------------------------------------------------------------------------
// registerNode / deregisterNode
// ---------------------------------------------------------------------------

TEST(GPUClusterCoordinatorTest, RegisterNode_AppearsInCluster) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeStandaloneConfig());

    GPUClusterCoordinator::NodeInfo n;
    n.id        = "extra-node";
    n.address   = "10.0.0.5:9000";
    n.status    = GPUClusterCoordinator::NodeStatus::ONLINE;
    n.gpu_count = 2;
    c.registerNode(n);

    EXPECT_GE(c.totalNodes(), 2u);
    bool found = false;
    for (const auto& node : c.getClusterNodes()) {
        if (node.id == "extra-node") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

TEST(GPUClusterCoordinatorTest, DeregisterNode_RemovesFromCluster) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    EXPECT_TRUE(c.deregisterNode("node-1"));
    EXPECT_EQ(c.totalNodes(), 1u);
}

TEST(GPUClusterCoordinatorTest, DeregisterNode_NonExistent_ReturnsFalse) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    EXPECT_FALSE(c.deregisterNode("does-not-exist"));
}

// ---------------------------------------------------------------------------
// updateHeartbeat
// ---------------------------------------------------------------------------

TEST(GPUClusterCoordinatorTest, UpdateHeartbeat_BringsNodeOnline) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    c.updateHeartbeat("node-1", 8ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(c.onlineNodeCount(), 1u);
}

TEST(GPUClusterCoordinatorTest, UpdateHeartbeat_UpdatesFreeVRAM) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    const uint64_t vram = 16ULL * 1024 * 1024 * 1024;
    c.updateHeartbeat("node-1", vram);

    for (const auto& n : c.getClusterNodes()) {
        if (n.id == "node-1") {
            EXPECT_EQ(n.free_vram_bytes, vram);
            break;
        }
    }
}

TEST(GPUClusterCoordinatorTest, UpdateHeartbeat_UnknownNode_NoOp) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    const size_t before = c.onlineNodeCount();
    c.updateHeartbeat("ghost-node", 1024);
    EXPECT_EQ(c.onlineNodeCount(), before);
}

// ---------------------------------------------------------------------------
// markNodeOffline
// ---------------------------------------------------------------------------

TEST(GPUClusterCoordinatorTest, MarkNodeOffline_SetsStatus) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    c.updateHeartbeat("node-1", 8ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(c.onlineNodeCount(), 1u);

    c.markNodeOffline("node-1");
    EXPECT_EQ(c.onlineNodeCount(), 0u);
}

// ---------------------------------------------------------------------------
// selectNode — LEAST_LOADED strategy
// ---------------------------------------------------------------------------

TEST(GPUClusterCoordinatorTest, SelectNode_PicksNodeWithMostFreeVRAM) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    c.updateHeartbeat("node-1", 4ULL  * 1024 * 1024 * 1024);
    c.updateHeartbeat("node-2", 16ULL * 1024 * 1024 * 1024);

    const auto* best = c.selectNode();
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->id, "node-2");
}

TEST(GPUClusterCoordinatorTest, SelectNode_SkipsOfflineNodes) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    c.updateHeartbeat("node-1", 4ULL  * 1024 * 1024 * 1024);
    c.updateHeartbeat("node-2", 16ULL * 1024 * 1024 * 1024);
    c.markNodeOffline("node-2");

    const auto* best = c.selectNode();
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->id, "node-1");
}

TEST(GPUClusterCoordinatorTest, SelectNode_RequiredVRAM_SkipsInsufficientNodes) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    c.updateHeartbeat("node-1", 2ULL * 1024 * 1024 * 1024);
    c.updateHeartbeat("node-2", 8ULL * 1024 * 1024 * 1024);

    const uint64_t required = 4ULL * 1024 * 1024 * 1024;
    const auto* best = c.selectNode(required);
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->id, "node-2");
}

TEST(GPUClusterCoordinatorTest, SelectNode_RequiredVRAM_NullWhenNoneSuffice) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    c.updateHeartbeat("node-1", 1ULL * 1024 * 1024 * 1024);
    c.updateHeartbeat("node-2", 1ULL * 1024 * 1024 * 1024);

    const uint64_t required = 8ULL * 1024 * 1024 * 1024;
    EXPECT_EQ(c.selectNode(required), nullptr);
}

// ---------------------------------------------------------------------------
// expireStaleNodes
// ---------------------------------------------------------------------------

TEST(GPUClusterCoordinatorTest, ExpireStaleNodes_MarksTimedOutNodesOffline) {
    ClusterConfig cfg = MakeCoordinatorConfig();
    cfg.node_timeout_ms = 50;   // very short timeout for test speed

    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(cfg);

    c.updateHeartbeat("node-1", 8ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(c.onlineNodeCount(), 1u);

    // Wait for the heartbeat to expire.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    c.expireStaleNodes();

    EXPECT_EQ(c.onlineNodeCount(), 0u);
}

TEST(GPUClusterCoordinatorTest, ExpireStaleNodes_FreshHeartbeat_StaysOnline) {
    ClusterConfig cfg = MakeCoordinatorConfig();
    cfg.node_timeout_ms = 5000;   // generous timeout

    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(cfg);

    c.updateHeartbeat("node-1", 8ULL * 1024 * 1024 * 1024);
    c.expireStaleNodes();

    EXPECT_EQ(c.onlineNodeCount(), 1u);
}

// ---------------------------------------------------------------------------
// getOnlineNodes
// ---------------------------------------------------------------------------

TEST(GPUClusterCoordinatorTest, GetOnlineNodes_ReturnsOnlyOnlineNodes) {
    GPUClusterCoordinator& c = GPUClusterCoordinator::GetInstance();
    c.initialize(MakeCoordinatorConfig());

    c.updateHeartbeat("node-1", 4ULL * 1024 * 1024 * 1024);
    // node-2 remains OFFLINE

    const auto online = c.getOnlineNodes();
    ASSERT_EQ(online.size(), 1u);
    EXPECT_EQ(online[0].id, "node-1");
}
