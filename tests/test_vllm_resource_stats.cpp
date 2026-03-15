// =============================================================================
// VLLMResourceManager::getStats() — CPU / RAM monitoring tests
//
// Validates that the OS-specific CPU utilisation and RAM usage metrics return
// plausible values on a live system (Linux / Windows).  Tests are intentionally
// loose (range checks only) since exact values are system-dependent.
// =============================================================================

#include <gtest/gtest.h>
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
