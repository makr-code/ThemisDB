/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_vllm_resource_stats.cpp                       ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-16 04:32:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     146                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 592b54382  2026-03-15  fix(scheduler,acceleration): remove stale TODOs, add VLLM... ║
    • e627c556b  2026-03-15  feat(acceleration): BackendRegistry thread-safety, VLLMRe... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// =============================================================================
// VLLMResourceManager::getStats() — CPU / RAM monitoring tests
//
// Validates that the OS-specific CPU utilisation and RAM usage metrics return
// plausible values on a live system (Linux / Windows).  Tests are intentionally
// loose (range checks only) since exact values are system-dependent.
// =============================================================================

#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <string>
#include "acceleration/vllm_resource_manager.h"

using namespace themis::acceleration;

namespace {

VLLMResourceManager::Config makeConfig() {
    VLLMResourceManager::Config cfg;
    cfg.total_cpu_cores  = 8;
    cfg.total_ram_gb     = 16;
    cfg.themis_cpu_cores = 6;
    return cfg;
}

} // namespace

// ---------------------------------------------------------------------------
// CPU utilisation sanity checks (Linux / Windows only)
// ---------------------------------------------------------------------------

TEST(VLLMResourceStatsTest, CpuUtilization_InRange) {
#if !defined(__linux__) && !defined(_WIN32)
    GTEST_SKIP() << "CPU monitoring implemented for Linux and Windows only";
#endif

    VLLMResourceManager mgr(makeConfig());
    ASSERT_TRUE(mgr.initialize());

    auto stats = mgr.getStats();
    EXPECT_GE(stats.cpu_utilization, 0.0)  << "CPU utilisation must be >= 0";
    EXPECT_LE(stats.cpu_utilization, 100.0) << "CPU utilisation must be <= 100";
}

// ---------------------------------------------------------------------------
// RAM usage sanity checks (Linux / Windows only)
// ---------------------------------------------------------------------------

TEST(VLLMResourceStatsTest, RamUsedMb_GreaterThanZero) {
#if !defined(__linux__) && !defined(_WIN32)
    GTEST_SKIP() << "RAM monitoring implemented for Linux and Windows only";
#endif

    VLLMResourceManager mgr(makeConfig());
    ASSERT_TRUE(mgr.initialize());

    auto stats = mgr.getStats();
    EXPECT_GT(stats.ram_used_mb, static_cast<size_t>(0))
        << "ram_used_mb should be > 0 on a live system";
}

TEST(VLLMResourceStatsTest, RamUtilization_InRange) {
#if !defined(__linux__) && !defined(_WIN32)
    GTEST_SKIP() << "RAM monitoring implemented for Linux and Windows only";
#endif

    VLLMResourceManager mgr(makeConfig());
    ASSERT_TRUE(mgr.initialize());

    auto stats = mgr.getStats();
    EXPECT_GE(stats.ram_utilization, 0.0)   << "RAM utilisation must be >= 0";
    EXPECT_LE(stats.ram_utilization, 100.0)  << "RAM utilisation must be <= 100";
}

// ---------------------------------------------------------------------------
// canUseGPU() sanity check — should not hang and return a bool
// ---------------------------------------------------------------------------

TEST(VLLMResourceStatsTest, CanUseGpu_DoesNotHang) {
    VLLMResourceManager mgr(makeConfig());
    ASSERT_TRUE(mgr.initialize());

    // Call should return within the 500 ms timeout regardless of CUDA availability.
    bool result = mgr.canUseGPU();
    // Without CUDA, should return false; with CUDA, result is hw-dependent.
    // Just assert no hang / crash.
    (void)result;
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Uninitialised manager returns zeroed stats (guard against segfault)
// ---------------------------------------------------------------------------

TEST(VLLMResourceStatsTest, GetStats_UninitializedReturnsZero) {
    VLLMResourceManager mgr(makeConfig());
    // Do NOT call initialize()
    auto stats = mgr.getStats();
    EXPECT_EQ(stats.cpu_utilization, 0.0);
    EXPECT_EQ(stats.ram_used_mb,     static_cast<size_t>(0));
    EXPECT_EQ(stats.ram_utilization, 0.0);
}

// ---------------------------------------------------------------------------
// gpu_device_index / gpu_device_indices config (no CUDA runtime needed)
// ---------------------------------------------------------------------------

TEST(VLLMResourceStatsTest, ConfigDefaultDeviceIndex_IsZero) {
    VLLMResourceManager::Config cfg;
    EXPECT_EQ(cfg.gpu_device_index, 0u);
    EXPECT_TRUE(cfg.gpu_device_indices.empty());
}

TEST(VLLMResourceStatsTest, ConfigSingleDeviceIndex_StoresCorrectly) {
    VLLMResourceManager::Config cfg;
    cfg.gpu_device_index = 2;
    EXPECT_EQ(cfg.gpu_device_index, 2u);
    EXPECT_TRUE(cfg.gpu_device_indices.empty());
}

TEST(VLLMResourceStatsTest, ConfigMultiDeviceIndices_StoreCorrectly) {
    VLLMResourceManager::Config cfg;
    cfg.gpu_device_indices = {2, 3};
    ASSERT_EQ(cfg.gpu_device_indices.size(), 2u);
    EXPECT_EQ(cfg.gpu_device_indices[0], 2u);
    EXPECT_EQ(cfg.gpu_device_indices[1], 3u);
}

// initialize() must succeed even when NVML is unavailable (no CUDA in CI).
// The manager gracefully falls back to CPU-only mode.
TEST(VLLMResourceStatsTest, MultiDeviceConfig_InitializeSucceeds_WithoutCUDA) {
    VLLMResourceManager::Config cfg = makeConfig();
    cfg.gpu_device_indices = {0, 1};  // Would monitor two GPUs if NVML were present

    VLLMResourceManager mgr(cfg);
    // initialize() should not crash/throw even without NVML on CI
    ASSERT_TRUE(mgr.initialize());
    mgr.shutdown();
}

TEST(VLLMResourceStatsTest, SingleDeviceIndex_InitializeSucceeds_WithoutCUDA) {
    VLLMResourceManager::Config cfg = makeConfig();
    cfg.gpu_device_index = 1;  // Non-default single device

    VLLMResourceManager mgr(cfg);
    ASSERT_TRUE(mgr.initialize());
    mgr.shutdown();
}

// ---------------------------------------------------------------------------
// CPU snapshot cache: rapid successive calls return valid data
// ---------------------------------------------------------------------------

TEST(VLLMResourceStatsTest, RapidSuccessiveCalls_ReturnValidData) {
#if !defined(__linux__) && !defined(_WIN32)
    GTEST_SKIP() << "CPU monitoring implemented for Linux and Windows only";
#endif

    VLLMResourceManager mgr(makeConfig());
    ASSERT_TRUE(mgr.initialize());

    // First call: warms the cache (takes ~100 ms for the two-snapshot baseline).
    auto stats1 = mgr.getStats();
    EXPECT_GE(stats1.cpu_utilization, 0.0);
    EXPECT_LE(stats1.cpu_utilization, 100.0);

    // Rapid second call: uses the cached snapshot.  Result must still be valid.
    auto stats2 = mgr.getStats();
    EXPECT_GE(stats2.cpu_utilization, 0.0)  << "cache-hit cpu_utilization must be >= 0";
    EXPECT_LE(stats2.cpu_utilization, 100.0) << "cache-hit cpu_utilization must be <= 100";
    EXPECT_GT(stats2.ram_used_mb, static_cast<size_t>(0))
        << "cache-hit ram_used_mb should still be > 0";
}

// ---------------------------------------------------------------------------
// Performance test: cache-hit getStats() must complete in < 2 ms.
// Opt-in via THEMIS_RUN_PERF_TESTS=1 to avoid CI flakiness.
// ---------------------------------------------------------------------------

TEST(VLLMResourceStatsTest, CacheHit_CompletesUnder2ms) {
#if !defined(__linux__) && !defined(_WIN32)
    GTEST_SKIP() << "CPU monitoring implemented for Linux and Windows only";
#endif

    {
        const char* env = std::getenv("THEMIS_RUN_PERF_TESTS");
        if (!env || std::string(env) != "1") {
            GTEST_SKIP() << "Set THEMIS_RUN_PERF_TESTS=1 to run performance tests";
        }
    }

    VLLMResourceManager mgr(makeConfig());
    ASSERT_TRUE(mgr.initialize());

    // Must match kCpuCacheTTL in vllm_resource_manager.cpp.
    constexpr long kCacheTtlMs = 200;

    // Warm the cache: first call always takes ~100 ms for the two-snapshot baseline.
    const auto warm_start = std::chrono::steady_clock::now();
    mgr.getStats();
    const auto warm_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - warm_start).count();

    // If scheduling delays consumed the entire TTL window during warm-up,
    // the next call would be a cache miss (~100 ms), making the 2 ms assertion
    // meaningless.  Skip rather than produce a spurious failure.
    if (warm_elapsed_ms >= kCacheTtlMs) {
        GTEST_SKIP() << "Warm-up took " << warm_elapsed_ms
                     << " ms (>= " << kCacheTtlMs
                     << " ms TTL) — cache miss expected; skipping perf gate";
    }

    // Second call must hit the cache and complete in < 2 ms.
    auto t0 = std::chrono::steady_clock::now();
    auto stats = mgr.getStats();
    auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0).count();

    EXPECT_LT(elapsed_us, 2000)
        << "Cache-hit getStats() must complete in < 2 ms (got "
        << elapsed_us << " µs)";
    EXPECT_GE(stats.cpu_utilization, 0.0);
    EXPECT_LE(stats.cpu_utilization, 100.0);
}
