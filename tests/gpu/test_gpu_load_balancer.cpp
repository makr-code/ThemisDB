#include <gtest/gtest.h>
#include "themis/gpu/load_balancer.h"
#include "themis/gpu/cluster_topology.h"
#include "themis/gpu/device_discovery.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::gpu;

// Build a fake device list.
static std::vector<DeviceInfo> makeDevices(int n,
                                            uint64_t vram_each = 8ULL * 1024 * 1024 * 1024) {
    std::vector<DeviceInfo> devs;
    for (int i = 0; i < n; ++i) {
        DeviceInfo d;
        d.index            = i;
        d.name             = "GPU_" + std::to_string(i);
        d.backend          = "CUDA";
        d.total_vram_bytes = vram_each;
        d.free_vram_bytes  = vram_each;
        d.is_healthy       = true;
        devs.push_back(d);
    }
    return devs;
}

// ---------------------------------------------------------------------------
// Construction / updateDevices
// ---------------------------------------------------------------------------

TEST(GPULoadBalancerTest, Construct_NoDevices_HealthyIsZero) {
    GPULoadBalancer lb;
    EXPECT_EQ(lb.totalDevices(), 0u);
    EXPECT_EQ(lb.healthyDevices(), 0u);
}

TEST(GPULoadBalancerTest, UpdateDevices_SetsDevices) {
    GPULoadBalancer lb;
    lb.updateDevices(makeDevices(3));
    EXPECT_EQ(lb.totalDevices(), 3u);
    EXPECT_EQ(lb.healthyDevices(), 3u);
}

TEST(GPULoadBalancerTest, UpdateDevices_ReplacesExisting) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::ROUND_ROBIN, makeDevices(4));
    lb.updateDevices(makeDevices(2));
    EXPECT_EQ(lb.totalDevices(), 2u);
}

// ---------------------------------------------------------------------------
// selectDevice — FIRST_HEALTHY
// ---------------------------------------------------------------------------

TEST(GPULoadBalancerTest, FirstHealthy_ReturnsFirstDevice) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::FIRST_HEALTHY, makeDevices(3));
    const auto* d = lb.selectDevice();
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->index, 0);
}

TEST(GPULoadBalancerTest, FirstHealthy_NoDevices_ReturnsNull) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::FIRST_HEALTHY);
    EXPECT_EQ(lb.selectDevice(), nullptr);
}

// ---------------------------------------------------------------------------
// selectDevice — ROUND_ROBIN
// ---------------------------------------------------------------------------

TEST(GPULoadBalancerTest, RoundRobin_CyclesThroughDevices) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::ROUND_ROBIN, makeDevices(3));
    std::vector<int> selected;
    for (int i = 0; i < 6; ++i) {
        const auto* d = lb.selectDevice();
        ASSERT_NE(d, nullptr);
        selected.push_back(d->index);
    }
    // Should see 0,1,2,0,1,2.
    EXPECT_EQ(selected[0], 0);
    EXPECT_EQ(selected[1], 1);
    EXPECT_EQ(selected[2], 2);
    EXPECT_EQ(selected[3], 0);
}

TEST(GPULoadBalancerTest, RoundRobin_SkipsFailedDevice) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::ROUND_ROBIN, makeDevices(3));
    lb.markDeviceFailed(1, "lost");
    for (int i = 0; i < 6; ++i) {
        const auto* d = lb.selectDevice();
        ASSERT_NE(d, nullptr);
        EXPECT_NE(d->index, 1);  // Device 1 must never be selected.
    }
}

// ---------------------------------------------------------------------------
// selectDevice — LEAST_LOADED
// ---------------------------------------------------------------------------

TEST(GPULoadBalancerTest, LeastLoaded_SelectsDeviceWithMostFreeVRAM) {
    auto devs = makeDevices(3);
    devs[0].free_vram_bytes = 2ULL * 1024 * 1024 * 1024;
    devs[1].free_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    devs[2].free_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    GPULoadBalancer lb(GPULoadBalancer::Strategy::LEAST_LOADED, devs);
    const auto* d = lb.selectDevice();
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->index, 1);  // device 1 has most free VRAM
}

TEST(GPULoadBalancerTest, LeastLoaded_ExcludesDevicesWithInsufficientVRAM) {
    auto devs = makeDevices(2);
    devs[0].free_vram_bytes = 512ULL * 1024 * 1024;   // 512 MB free
    devs[1].free_vram_bytes = 4ULL  * 1024 * 1024 * 1024;  // 4 GB free
    GPULoadBalancer lb(GPULoadBalancer::Strategy::LEAST_LOADED, devs);
    const uint64_t req = 1ULL * 1024 * 1024 * 1024;  // need 1 GB
    const auto* d = lb.selectDevice(req);
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->index, 1);
}

TEST(GPULoadBalancerTest, LeastLoaded_NoEligibleDevice_ReturnsNull) {
    auto devs = makeDevices(2);
    devs[0].free_vram_bytes = 100;
    devs[1].free_vram_bytes = 100;
    GPULoadBalancer lb(GPULoadBalancer::Strategy::LEAST_LOADED, devs);
    EXPECT_EQ(lb.selectDevice(1ULL * 1024 * 1024 * 1024), nullptr);
}

// ---------------------------------------------------------------------------
// markDeviceFailed / resetDevice
// ---------------------------------------------------------------------------

TEST(GPULoadBalancerTest, MarkFailed_ReducesHealthyCount) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::ROUND_ROBIN, makeDevices(3));
    lb.markDeviceFailed(2, "device error");
    EXPECT_EQ(lb.healthyDevices(), 2u);
}

TEST(GPULoadBalancerTest, ResetDevice_RestoresHealthyCount) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::ROUND_ROBIN, makeDevices(3));
    lb.markDeviceFailed(0);
    EXPECT_EQ(lb.healthyDevices(), 2u);
    lb.resetDevice(0);
    EXPECT_EQ(lb.healthyDevices(), 3u);
}

TEST(GPULoadBalancerTest, AllDevicesFailed_SelectReturnsNull) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::ROUND_ROBIN, makeDevices(2));
    lb.markDeviceFailed(0);
    lb.markDeviceFailed(1);
    EXPECT_EQ(lb.selectDevice(), nullptr);
}

// ---------------------------------------------------------------------------
// recordAllocation / recordDeallocation
// ---------------------------------------------------------------------------

TEST(GPULoadBalancerTest, RecordAlloc_DecreasesDeviceFreeVRAM) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::LEAST_LOADED, makeDevices(1));
    lb.recordAllocation(0, 2ULL * 1024 * 1024 * 1024);
    const auto loads = lb.getDeviceLoads();
    ASSERT_EQ(loads.size(), 1u);
    EXPECT_EQ(loads[0].free_vram_bytes,
              8ULL * 1024 * 1024 * 1024 - 2ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(loads[0].tracked_alloc_bytes, 2ULL * 1024 * 1024 * 1024);
}

TEST(GPULoadBalancerTest, RecordDealloc_IncreasesDeviceFreeVRAM) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::LEAST_LOADED, makeDevices(1));
    lb.recordAllocation(0, 4ULL * 1024 * 1024 * 1024);
    lb.recordDeallocation(0, 2ULL * 1024 * 1024 * 1024);
    const auto loads = lb.getDeviceLoads();
    EXPECT_EQ(loads[0].tracked_alloc_bytes, 2ULL * 1024 * 1024 * 1024);
}

// ---------------------------------------------------------------------------
// getDeviceLoads
// ---------------------------------------------------------------------------

TEST(GPULoadBalancerTest, GetDeviceLoads_ContainsAllDevices) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::ROUND_ROBIN, makeDevices(3));
    EXPECT_EQ(lb.getDeviceLoads().size(), 3u);
}

TEST(GPULoadBalancerTest, GetDeviceLoads_ReflectsFailureReason) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::ROUND_ROBIN, makeDevices(2));
    lb.markDeviceFailed(1, "timeout");
    const auto loads = lb.getDeviceLoads();
    for (const auto& dl : loads) {
        if (dl.index == 1) {
            EXPECT_FALSE(dl.is_healthy);
            EXPECT_EQ(dl.failure_reason, "timeout");
        }
    }
}

// ---------------------------------------------------------------------------
// CPU_FALLBACK device excluded from selection
// ---------------------------------------------------------------------------

TEST(GPULoadBalancerTest, CPUFallback_ExcludedFromSelection) {
    std::vector<DeviceInfo> devs;
    DeviceInfo cpu;
    cpu.index      = -1;
    cpu.name       = "CPU Fallback";
    cpu.backend    = "CPU_FALLBACK";
    cpu.is_healthy = true;
    devs.push_back(cpu);

    GPULoadBalancer lb(GPULoadBalancer::Strategy::LEAST_LOADED, devs);
    EXPECT_EQ(lb.healthyDevices(), 0u);  // CPU fallback doesn't count
    EXPECT_EQ(lb.selectDevice(), nullptr);
}

// ---------------------------------------------------------------------------
// Concurrent selectDevice — no data race
// ---------------------------------------------------------------------------

TEST(GPULoadBalancerTest, Concurrent_Select_NoDataRace) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::ROUND_ROBIN, makeDevices(4));
    constexpr int THREADS = 8, OPS_PER_THREAD = 50;
    std::atomic<int> non_null{0};

    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&lb, &non_null]() {
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                if (lb.selectDevice() != nullptr) non_null.fetch_add(1);
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_GT(non_null.load(), 0);
}

// ---------------------------------------------------------------------------
// TOPOLOGY_AWARE — NVLink topology-aware scheduling
// ---------------------------------------------------------------------------

// Helper: build a topology with an NVLink link from device src to device dst.
static GPUClusterTopology makeNVLinkTopology(int num_gpus,
                                              int src, int dst,
                                              float bw_gbps = 300.0f)
{
    GPUClusterTopology topo;
    topo.num_gpus = num_gpus;
    topo.bandwidth_matrix.assign(num_gpus, std::vector<float>(num_gpus, 0.0f));

    TopologyLink lnk;
    lnk.type             = InterconnectType::NVLINK;
    lnk.bandwidth_gbps   = bw_gbps;
    lnk.latency_us       = 1.0f;
    lnk.src_device_index = src;
    lnk.dst_device_index = dst;
    topo.addLink(lnk);
    topo.has_nvlink = true;
    return topo;
}

TEST(GPULoadBalancerTopologyAwareTest, TopologyAware_NoTopology_FallsBackToLeastLoaded) {
    // With no topology set, TOPOLOGY_AWARE should behave like LEAST_LOADED.
    auto devs = makeDevices(2);
    devs[0].free_vram_bytes = 2ULL * 1024 * 1024 * 1024;
    devs[1].free_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    GPULoadBalancer lb(GPULoadBalancer::Strategy::TOPOLOGY_AWARE, devs);
    // No topology set — falls back to least-loaded, should pick device 1 (most VRAM).
    const auto* d = lb.selectDevice();
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->index, 1);
}

TEST(GPULoadBalancerTopologyAwareTest, TopologyAware_NVLinkTopology_PicksBestConnectedDevice) {
    // Three devices.  Device 0 has NVLink to device 1 (300 GB/s) and device 2
    // (300 GB/s), so it has the highest total outgoing bandwidth (600 GB/s).
    // Devices 1 and 2 only have one NVLink connection each (300 GB/s).
    auto devs = makeDevices(3);
    GPUClusterTopology topo;
    topo.num_gpus = 3;
    topo.bandwidth_matrix.assign(3, std::vector<float>(3, 0.0f));
    topo.has_nvlink = true;

    auto addNVLink = [&](int s, int d, float bw) {
        TopologyLink lnk;
        lnk.type             = InterconnectType::NVLINK;
        lnk.bandwidth_gbps   = bw;
        lnk.latency_us       = 1.0f;
        lnk.src_device_index = s;
        lnk.dst_device_index = d;
        topo.addLink(lnk);
    };
    addNVLink(0, 1, 300.0f);
    addNVLink(0, 2, 300.0f);
    addNVLink(1, 0, 300.0f);
    addNVLink(2, 0, 300.0f);

    GPULoadBalancer lb(GPULoadBalancer::Strategy::TOPOLOGY_AWARE, devs);
    lb.setTopology(topo);

    const auto* d = lb.selectDevice();
    ASSERT_NE(d, nullptr);
    // Device 0 has 600 GB/s total outgoing bandwidth — must be selected.
    EXPECT_EQ(d->index, 0);
}

TEST(GPULoadBalancerTopologyAwareTest, TopologyAware_SkipsUnhealthyDevice) {
    auto devs = makeDevices(2);
    GPULoadBalancer lb(GPULoadBalancer::Strategy::TOPOLOGY_AWARE, devs);
    lb.setTopology(makeNVLinkTopology(2, 0, 1));

    lb.markDeviceFailed(0, "oom");
    const auto* d = lb.selectDevice();
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->index, 1);
}

TEST(GPULoadBalancerTopologyAwareTest, TopologyAware_NoDevices_ReturnsNull) {
    GPULoadBalancer lb(GPULoadBalancer::Strategy::TOPOLOGY_AWARE);
    lb.setTopology(makeNVLinkTopology(2, 0, 1));
    EXPECT_EQ(lb.selectDevice(), nullptr);
}

TEST(GPULoadBalancerTopologyAwareTest, TopologyAware_NVLinkTopology_RespectsVRAMRequirement) {
    auto devs = makeDevices(2);
    // Device 0 has more NVLink bandwidth but not enough VRAM.
    devs[0].free_vram_bytes = 512ULL * 1024 * 1024;  // 512 MB
    devs[1].free_vram_bytes = 4ULL  * 1024 * 1024 * 1024;  // 4 GB

    GPULoadBalancer lb(GPULoadBalancer::Strategy::TOPOLOGY_AWARE, devs);
    lb.setTopology(makeNVLinkTopology(2, 0, 1, 300.0f));

    const uint64_t req = 1ULL * 1024 * 1024 * 1024;  // need 1 GB
    const auto* d = lb.selectDevice(req);
    ASSERT_NE(d, nullptr);
    // Device 0 is ineligible (insufficient VRAM) — must pick device 1.
    EXPECT_EQ(d->index, 1);
}

TEST(GPULoadBalancerTopologyAwareTest, TopologyAware_SetTopology_UpdatesScheduling) {
    auto devs = makeDevices(2);
    GPULoadBalancer lb(GPULoadBalancer::Strategy::TOPOLOGY_AWARE, devs);

    // Before setting topology: falls back to least-loaded (both equal VRAM → device 0).
    const auto* d1 = lb.selectDevice();
    ASSERT_NE(d1, nullptr);

    // Now set a topology where device 1 has NVLink to device 0.
    GPUClusterTopology topo;
    topo.num_gpus = 2;
    topo.bandwidth_matrix.assign(2, std::vector<float>(2, 0.0f));
    topo.has_nvlink = true;
    TopologyLink lnk;
    lnk.type             = InterconnectType::NVLINK;
    lnk.bandwidth_gbps   = 300.0f;
    lnk.latency_us       = 1.0f;
    lnk.src_device_index = 1;
    lnk.dst_device_index = 0;
    topo.addLink(lnk);
    lb.setTopology(topo);

    const auto* d2 = lb.selectDevice();
    ASSERT_NE(d2, nullptr);
    // Device 1 now has the highest outgoing NVLink bandwidth.
    EXPECT_EQ(d2->index, 1);
}
