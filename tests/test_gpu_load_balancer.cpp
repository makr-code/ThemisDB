/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_gpu_load_balancer.cpp                         ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     262                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "themis/gpu/load_balancer.h"
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
