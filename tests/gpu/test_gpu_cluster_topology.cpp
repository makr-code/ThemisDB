#include <gtest/gtest.h>
#include "themis/gpu/cluster_topology.h"
#include "themis/gpu/cluster_coordinator.h"
#include "themis/gpu/device_discovery.h"
#include "themis/gpu/stream_manager.h"

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static DeviceInfo makeCudaDevice(int idx, const std::string& name,
                                  int major = 7, int minor = 0,
                                  uint64_t free_vram = 8ULL * 1024 * 1024 * 1024)
{
    DeviceInfo d;
    d.index          = idx;
    d.device_index   = idx;
    d.name           = name;
    d.backend        = "CUDA";
    d.compute_major  = major;
    d.compute_minor  = minor;
    d.total_vram_bytes = 24ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = free_vram;
    d.is_healthy       = true;
    return d;
}

static DeviceInfo makeCPUFallback() {
    DeviceInfo d;
    d.index        = 0;
    d.device_index = 0;
    d.name         = "CPU_FALLBACK";
    d.backend      = "CPU_FALLBACK";
    d.is_healthy   = true;
    return d;
}

// ===========================================================================
// GPUClusterTopology tests
// ===========================================================================

// ---------------------------------------------------------------------------
// detect() — empty / CPU-fallback
// ---------------------------------------------------------------------------

TEST(GPUClusterTopologyTest, Detect_EmptyDevices_ReturnsEmptyTopology) {
    auto topo = GPUClusterTopology::detect({});
    EXPECT_EQ(topo.num_gpus, 0);
    EXPECT_TRUE(topo.bandwidth_matrix.empty());
    EXPECT_TRUE(topo.links.empty());
    EXPECT_FALSE(topo.has_nvlink);
    EXPECT_FALSE(topo.has_pcie_p2p);
}

TEST(GPUClusterTopologyTest, Detect_SingleDevice_HasNoLinks) {
    auto topo = GPUClusterTopology::detect({makeCudaDevice(0, "A100")});
    EXPECT_EQ(topo.num_gpus, 1);
    EXPECT_TRUE(topo.links.empty());
    ASSERT_EQ(topo.bandwidth_matrix.size(), 1u);
    EXPECT_EQ(topo.bandwidth_matrix[0][0], 0.0f);
}

TEST(GPUClusterTopologyTest, Detect_CPUFallback_HasNoLinks) {
    auto topo = GPUClusterTopology::detect({makeCPUFallback()});
    EXPECT_EQ(topo.num_gpus, 1);
    EXPECT_TRUE(topo.links.empty());
}

// ---------------------------------------------------------------------------
// detect() — two CUDA devices (CPU-fallback path since no real CUDA)
// ---------------------------------------------------------------------------

TEST(GPUClusterTopologyTest, Detect_TwoCudaDevices_BuildsBandwidthMatrix) {
    std::vector<DeviceInfo> devs = {
        makeCudaDevice(0, "A100-0"),
        makeCudaDevice(1, "A100-1"),
    };
    auto topo = GPUClusterTopology::detect(devs);

    EXPECT_EQ(topo.num_gpus, 2);
    ASSERT_EQ(topo.bandwidth_matrix.size(), 2u);
    ASSERT_EQ(topo.bandwidth_matrix[0].size(), 2u);

    // Diagonal is always 0.
    EXPECT_EQ(topo.bandwidth_matrix[0][0], 0.0f);
    EXPECT_EQ(topo.bandwidth_matrix[1][1], 0.0f);

    // At least some estimated bandwidth between devices.
    EXPECT_GT(topo.bandwidth_matrix[0][1], 0.0f);
    EXPECT_GT(topo.bandwidth_matrix[1][0], 0.0f);
}

TEST(GPUClusterTopologyTest, Detect_TwoCudaDevices_HasLinks) {
    auto topo = GPUClusterTopology::detect({
        makeCudaDevice(0, "V100-0"),
        makeCudaDevice(1, "V100-1"),
    });
    EXPECT_FALSE(topo.links.empty());
}

// ---------------------------------------------------------------------------
// bandwidthBetween
// ---------------------------------------------------------------------------

TEST(GPUClusterTopologyTest, BandwidthBetween_SameDevice_ReturnsZero) {
    auto topo = GPUClusterTopology::detect({
        makeCudaDevice(0, "A100"),
        makeCudaDevice(1, "A100"),
    });
    EXPECT_EQ(topo.bandwidthBetween(0, 0), 0.0f);
    EXPECT_EQ(topo.bandwidthBetween(1, 1), 0.0f);
}

TEST(GPUClusterTopologyTest, BandwidthBetween_OutOfRange_ReturnsZero) {
    auto topo = GPUClusterTopology::detect({makeCudaDevice(0, "A100")});
    EXPECT_EQ(topo.bandwidthBetween(0, 5), 0.0f);
    EXPECT_EQ(topo.bandwidthBetween(-1, 0), 0.0f);
}

// ---------------------------------------------------------------------------
// bestNVLinkPair — without real NVLink hardware returns {-1, -1}
// ---------------------------------------------------------------------------

TEST(GPUClusterTopologyTest, BestNVLinkPair_WithoutNVLink_ReturnsMinusOne) {
    auto topo = GPUClusterTopology::detect({});
    auto pair = topo.bestNVLinkPair();
    EXPECT_EQ(pair.first,  -1);
    EXPECT_EQ(pair.second, -1);
}

TEST(GPUClusterTopologyTest, BestNVLinkPair_WithManualNVLinkLink_ReturnsCorrectPair) {
    GPUClusterTopology topo;
    topo.num_gpus = 2;
    topo.bandwidth_matrix.assign(2, std::vector<float>(2, 0.0f));

    TopologyLink lnk;
    lnk.type              = InterconnectType::NVLINK;
    lnk.bandwidth_gbps    = 300.0f;
    lnk.latency_us        = 1.0f;
    lnk.src_device_index  = 0;
    lnk.dst_device_index  = 1;
    topo.addLink(lnk);   // addLink() must update bandwidth_matrix for intra-node links
    topo.has_nvlink        = true;

    // Verify the matrix was updated by addLink().
    EXPECT_FLOAT_EQ(topo.bandwidthBetween(0, 1), 300.0f);

    auto pair = topo.bestNVLinkPair();
    EXPECT_EQ(pair.first,  0);
    EXPECT_EQ(pair.second, 1);
}

TEST(GPUClusterTopologyTest, AddLink_IntraNode_UpdatesBandwidthMatrix) {
    GPUClusterTopology topo;
    topo.num_gpus = 2;
    topo.bandwidth_matrix.assign(2, std::vector<float>(2, 0.0f));

    TopologyLink lnk;
    lnk.type              = InterconnectType::PCIE_P2P;
    lnk.bandwidth_gbps    = 16.0f;
    lnk.src_device_index  = 0;
    lnk.dst_device_index  = 1;
    topo.addLink(lnk);

    EXPECT_FLOAT_EQ(topo.bandwidthBetween(0, 1), 16.0f);
    EXPECT_FLOAT_EQ(topo.bandwidthBetween(1, 0), 0.0f);  // directed: only 0→1 was set
}

// ---------------------------------------------------------------------------
// Node management
// ---------------------------------------------------------------------------

TEST(GPUClusterTopologyTest, AddNode_RegistersNode) {
    GPUClusterTopology topo;
    ClusterNode n;
    n.node_id = "node0";
    n.rank    = 0;
    topo.addNode(n);

    EXPECT_EQ(topo.nodes.size(), 1u);
    EXPECT_EQ(topo.getNode("node0").node_id, "node0");
}

TEST(GPUClusterTopologyTest, AddNode_DuplicateIgnored) {
    GPUClusterTopology topo;
    ClusterNode n;
    n.node_id = "n0";
    topo.addNode(n);
    topo.addNode(n);
    EXPECT_EQ(topo.nodes.size(), 1u);
}

TEST(GPUClusterTopologyTest, RemoveNode_RemovesNodeAndLinks) {
    GPUClusterTopology topo;
    ClusterNode a; a.node_id = "a";
    ClusterNode b; b.node_id = "b";
    topo.addNode(a);
    topo.addNode(b);

    TopologyLink lnk;
    lnk.type          = InterconnectType::INFINIBAND;
    lnk.bandwidth_gbps = 25.0f;
    lnk.src_node_id   = "a";
    lnk.dst_node_id   = "b";
    topo.addLink(lnk);

    topo.removeNode("a");
    EXPECT_EQ(topo.nodes.size(), 1u);
    EXPECT_EQ(topo.getNode("a").node_id, "");
    // All links referencing "a" should be gone.
    for (const auto& l : topo.links) {
        EXPECT_NE(l.src_node_id, "a");
        EXPECT_NE(l.dst_node_id, "a");
    }
}

TEST(GPUClusterTopologyTest, NodeIds_ReturnsAllIds) {
    GPUClusterTopology topo;
    ClusterNode n0; n0.node_id = "x";
    ClusterNode n1; n1.node_id = "y";
    topo.addNode(n0);
    topo.addNode(n1);
    auto ids = topo.nodeIds();
    EXPECT_EQ(ids.size(), 2u);
}

// ---------------------------------------------------------------------------
// InfiniBand links
// ---------------------------------------------------------------------------

TEST(GPUClusterTopologyTest, AddLink_InfiniBand_SetsFlag) {
    GPUClusterTopology topo;
    ClusterNode a; a.node_id = "a";
    ClusterNode b; b.node_id = "b";
    topo.addNode(a);
    topo.addNode(b);

    EXPECT_FALSE(topo.has_infiniband);

    TopologyLink lnk;
    lnk.type          = InterconnectType::INFINIBAND;
    lnk.bandwidth_gbps = 25.0f;
    lnk.src_node_id   = "a";
    lnk.dst_node_id   = "b";
    topo.addLink(lnk);

    EXPECT_TRUE(topo.has_infiniband);
}

TEST(GPUClusterTopologyTest, BestInfiniBandPair_NoLinks_ReturnsEmpty) {
    GPUClusterTopology topo;
    auto p = topo.bestInfiniBandPair();
    EXPECT_TRUE(p.first.empty());
    EXPECT_TRUE(p.second.empty());
}

TEST(GPUClusterTopologyTest, BestInfiniBandPair_PicksHighestBandwidth) {
    GPUClusterTopology topo;
    ClusterNode a; a.node_id = "a";
    ClusterNode b; b.node_id = "b";
    ClusterNode c; c.node_id = "c";
    topo.addNode(a); topo.addNode(b); topo.addNode(c);

    auto addIB = [&](const std::string& src, const std::string& dst, float bw) {
        TopologyLink l;
        l.type = InterconnectType::INFINIBAND;
        l.bandwidth_gbps = bw;
        l.src_node_id = src;
        l.dst_node_id = dst;
        topo.addLink(l);
    };
    addIB("a", "b", 25.0f);
    addIB("a", "c", 50.0f);  // higher bandwidth

    auto p = topo.bestInfiniBandPair();
    EXPECT_EQ(p.first,  "a");
    EXPECT_EQ(p.second, "c");
}

// ---------------------------------------------------------------------------
// interconnectTypeName
// ---------------------------------------------------------------------------

TEST(GPUClusterTopologyTest, InterconnectTypeName_ReturnsExpectedStrings) {
    EXPECT_STREQ(interconnectTypeName(InterconnectType::NVLINK),     "NVLink");
    EXPECT_STREQ(interconnectTypeName(InterconnectType::INFINIBAND), "InfiniBand");
    EXPECT_STREQ(interconnectTypeName(InterconnectType::PCIE_P2P),   "PCIe_P2P");
    EXPECT_STREQ(interconnectTypeName(InterconnectType::CPU),        "CPU");
    EXPECT_STREQ(interconnectTypeName(InterconnectType::XGMI),       "XGMI");
    EXPECT_STREQ(interconnectTypeName(InterconnectType::ETHERNET),   "Ethernet");
}

// ===========================================================================
// GPUClusterCoordinator tests
// ===========================================================================

class GPUClusterCoordinatorTopologyTest : public ::testing::Test {
protected:
    void SetUp() override {
        devices_ = {
            makeCudaDevice(0, "A100-0"),
            makeCudaDevice(1, "A100-1"),
        };
    }
    std::vector<DeviceInfo> devices_;
};

// ---------------------------------------------------------------------------
// Single-node mode (empty ClusterConfig)
// ---------------------------------------------------------------------------

TEST_F(GPUClusterCoordinatorTopologyTest, Initialize_SingleNodeMode) {
    GPUClusterCoordinator coord;
    ClusterConfig cfg;  // default: world_size == 1
    coord.initialize(cfg, devices_);
    EXPECT_TRUE(coord.isInitialized());
    EXPECT_EQ(coord.clusterConfig().world_size, 1);
}

TEST_F(GPUClusterCoordinatorTopologyTest, SelectDevice_SingleNode_ReturnsValidIndex) {
    GPUClusterCoordinator coord;
    coord.initialize(ClusterConfig{}, devices_);
    auto p = coord.selectDevice();
    EXPECT_GE(p.device_index, -1);
}

TEST_F(GPUClusterCoordinatorTopologyTest, SelectDevice_NVLink_NoEligibleDevice_ReturnsCPUFallback) {
    // Build two unhealthy devices so no device qualifies during NVLink selection.
    DeviceInfo d0 = makeCudaDevice(0, "A100-0");
    d0.is_healthy = false;
    DeviceInfo d1 = makeCudaDevice(1, "A100-1");
    d1.is_healthy = false;

    GPUClusterCoordinator coord;
    ClusterConfig cfg;
    cfg.enable_nvlink = true;
    coord.initialize(cfg, {d0, d1});

    // Manually inject NVLink topology so the NVLink branch is exercised.
    // (Without hardware the topology won't have has_nvlink=true from detect().)
    // We directly call selectDevice; because detect() will not set has_nvlink
    // on CI, this effectively tests the first-healthy-device fallback with no
    // healthy devices → must return device_index == -1 (CPU fallback), not 0.
    auto p = coord.selectDevice();
    // Either NVLink path or regular fallback: both should not return 0 when
    // all devices are unhealthy.
    EXPECT_EQ(p.device_index, -1);
    EXPECT_EQ(p.route, InterconnectType::CPU);
}

TEST_F(GPUClusterCoordinatorTopologyTest, ClusterHealth_SingleNode) {
    GPUClusterCoordinator coord;
    coord.initialize(ClusterConfig{}, devices_);
    auto h = coord.clusterHealth();
    EXPECT_EQ(h.total_gpus, 2);
}

// ---------------------------------------------------------------------------
// Multi-node mode
// ---------------------------------------------------------------------------

TEST_F(GPUClusterCoordinatorTopologyTest, Initialize_MultiNodeMode) {
    GPUClusterCoordinator coord;
    ClusterConfig cfg;
    cfg.enabled    = true;
    cfg.node_id    = "node0";
    cfg.rank       = 0;
    cfg.world_size = 2;
    cfg.ib_device  = "mlx5_0";
    cfg.ib_port    = "1";
    coord.initialize(cfg, devices_);
    EXPECT_TRUE(coord.isInitialized());
    EXPECT_TRUE(coord.clusterConfig().is_multi_node());
}

TEST_F(GPUClusterCoordinatorTopologyTest, RegisterNode_AddsNodeToTopology) {
    GPUClusterCoordinator coord;
    ClusterConfig cfg;
    cfg.enabled    = true;
    cfg.node_id    = "node0";
    cfg.rank       = 0;
    cfg.world_size = 2;
    cfg.ib_device  = "mlx5_0";
    coord.initialize(cfg, devices_);

    ClusterNode peer;
    peer.node_id = "node1";
    peer.rank    = 1;
    coord.registerNode(peer, 25.0f);

    const auto& topo = coord.topology();
    auto ids = topo.nodeIds();
    bool found = false;
    for (const auto& id : ids) { if (id == "node1") found = true; }
    EXPECT_TRUE(found);
}

TEST_F(GPUClusterCoordinatorTopologyTest, RegisterNode_WithIB_CreatesInfiniBandLinks) {
    GPUClusterCoordinator coord;
    ClusterConfig cfg;
    cfg.enabled       = true;
    cfg.node_id       = "nodeA";
    cfg.rank          = 0;
    cfg.world_size    = 2;
    cfg.ib_device     = "mlx5_0";
    cfg.enable_infiniband = true;
    coord.initialize(cfg, devices_);

    ClusterNode peer;
    peer.node_id = "nodeB";
    coord.registerNode(peer, 25.0f);

    EXPECT_TRUE(coord.topology().has_infiniband);
}

TEST_F(GPUClusterCoordinatorTopologyTest, RegisterNode_DefaultIBBandwidth_Is25GBps) {
    // Validate the default IB bandwidth is 25 GB/s (HDR IB 200 Gb/s ÷ 8).
    GPUClusterCoordinator coord;
    ClusterConfig cfg;
    cfg.enabled       = true;
    cfg.node_id       = "n0";
    cfg.world_size    = 2;
    cfg.ib_device     = "mlx5_0";
    cfg.enable_infiniband = true;
    coord.initialize(cfg, devices_);

    ClusterNode peer;
    peer.node_id = "n1";
    coord.registerNode(peer, 0.0f);  // 0 = use default

    // The default InfiniBand link bandwidth must be 25 GB/s.
    const auto& topo = coord.topology();
    float max_bw = 0.0f;
    for (const auto& lnk : topo.links) {
        if (lnk.type == InterconnectType::INFINIBAND &&
            lnk.src_node_id == "n0" && lnk.dst_node_id == "n1") {
            max_bw = lnk.bandwidth_gbps;
        }
    }
    EXPECT_FLOAT_EQ(max_bw, 25.0f);
}

TEST_F(GPUClusterCoordinatorTopologyTest, SelectNodeForTransfer_WithIB_ReturnsPeer) {
    GPUClusterCoordinator coord;
    ClusterConfig cfg;
    cfg.enabled       = true;
    cfg.node_id       = "n0";
    cfg.world_size    = 2;
    cfg.ib_device     = "mlx5_0";
    cfg.enable_infiniband = true;
    coord.initialize(cfg, devices_);

    ClusterNode peer;
    peer.node_id = "n1";
    coord.registerNode(peer, 25.0f);

    auto p = coord.selectNodeForTransfer("n0");
    EXPECT_EQ(p.node_id, "n1");
    EXPECT_EQ(p.route, InterconnectType::INFINIBAND);
}

TEST_F(GPUClusterCoordinatorTopologyTest, RemoveNode_RemovesFromTopology) {
    GPUClusterCoordinator coord;
    ClusterConfig cfg;
    cfg.enabled    = true;
    cfg.node_id    = "n0";
    cfg.world_size = 2;
    cfg.ib_device  = "mlx5_0";
    coord.initialize(cfg, devices_);

    ClusterNode peer;
    peer.node_id = "n1";
    coord.registerNode(peer);
    coord.removeNode("n1");

    auto ids = coord.topology().nodeIds();
    bool found = false;
    for (const auto& id : ids) { if (id == "n1") found = true; }
    EXPECT_FALSE(found);
}

TEST_F(GPUClusterCoordinatorTopologyTest, UpdateTopology_DoesNotCrash) {
    GPUClusterCoordinator coord;
    ClusterConfig cfg;
    cfg.node_id    = "n0";
    cfg.world_size = 1;
    coord.initialize(cfg, devices_);
    EXPECT_NO_THROW(coord.updateTopology());
}

// ---------------------------------------------------------------------------
// ClusterConfig helpers
// ---------------------------------------------------------------------------

TEST(ClusterConfigTest, DefaultIsSingleNode) {
    ClusterConfig cfg;
    EXPECT_FALSE(cfg.is_multi_node());
    EXPECT_FALSE(cfg.has_infiniband());
}

TEST(ClusterConfigTest, MultiNodeFlag) {
    ClusterConfig cfg;
    cfg.enabled    = true;
    cfg.world_size = 4;
    EXPECT_TRUE(cfg.is_multi_node());
}

TEST(ClusterConfigTest, InfiniBandFlag_RequiresEnabledAndDeviceAndEnable) {
    ClusterConfig cfg;
    cfg.enabled           = true;
    cfg.enable_infiniband = true;
    EXPECT_FALSE(cfg.has_infiniband());  // no ib_device yet
    cfg.ib_device = "mlx5_0";
    EXPECT_TRUE(cfg.has_infiniband());
    // Disabling enabled flag should also clear has_infiniband.
    cfg.enabled = false;
    EXPECT_FALSE(cfg.has_infiniband());
}

// ---------------------------------------------------------------------------
// GPUStreamManager::createCudaStream
// ---------------------------------------------------------------------------

TEST(GPUStreamManagerCudaStreamTest, CreateCudaStream_SucceedsForNewName) {
    themis::gpu::GPUStreamManager sm;
    themis::gpu::GPUStreamManager::StreamConfig cfg;
    cfg.name = "cuda_stream_0";
    EXPECT_TRUE(sm.createCudaStream(cfg, 0));
    EXPECT_TRUE(sm.hasStream("cuda_stream_0"));
}

TEST(GPUStreamManagerCudaStreamTest, CreateCudaStream_FailsForDuplicateName) {
    themis::gpu::GPUStreamManager sm;
    themis::gpu::GPUStreamManager::StreamConfig cfg;
    cfg.name = "dup_cuda";
    ASSERT_TRUE(sm.createCudaStream(cfg, 0));
    EXPECT_FALSE(sm.createCudaStream(cfg, 0));
}

TEST(GPUStreamManagerCudaStreamTest, CreateCudaStream_FailsForEmptyName) {
    themis::gpu::GPUStreamManager sm;
    themis::gpu::GPUStreamManager::StreamConfig cfg;
    cfg.name = "";
    EXPECT_FALSE(sm.createCudaStream(cfg, 0));
}

TEST(GPUStreamManagerCudaStreamTest, CreateCudaStream_StreamIsSubmittable) {
    themis::gpu::GPUStreamManager sm;
    themis::gpu::GPUStreamManager::StreamConfig cfg;
    cfg.name = "submittable_cuda";
    sm.createCudaStream(cfg, 0);

    themis::gpu::GPULauncher::WorkItem item;
    item.kernel_id = "noop";
    auto fut = sm.submit("submittable_cuda", item);
    auto res = fut.get();
    // Result may succeed (CUDA available) or succeed via CPU fallback.
    // We just verify the future resolves without deadlock.
    (void)res;
}

TEST(GPUStreamManagerCudaStreamTest, CreateCudaStream_DestroySucceeds) {
    themis::gpu::GPUStreamManager sm;
    themis::gpu::GPUStreamManager::StreamConfig cfg;
    cfg.name = "destroy_cuda";
    sm.createCudaStream(cfg, 0);
    EXPECT_TRUE(sm.destroyStream("destroy_cuda"));
    EXPECT_FALSE(sm.hasStream("destroy_cuda"));
}
