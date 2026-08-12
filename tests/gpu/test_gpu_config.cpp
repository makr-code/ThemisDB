#include <gtest/gtest.h>
#include "themis/gpu/config.h"
#include "themis/edition.h"

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Default config is valid
// ---------------------------------------------------------------------------

TEST(GPUConfigTest, DefaultConfig_IsValid) {
    GPUConfig cfg;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;  // 4 GB — satisfies reserve
    const auto r = cfg.validate();
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(r.errors.empty());
}

// ---------------------------------------------------------------------------
// OOM threshold
// ---------------------------------------------------------------------------

TEST(GPUConfigTest, OOMThreshold_Zero_Invalid) {
    GPUConfig cfg;
    cfg.oom_warning_threshold = 0.0f;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    EXPECT_FALSE(cfg.validate().ok);
}

TEST(GPUConfigTest, OOMThreshold_GreaterThanOne_Invalid) {
    GPUConfig cfg;
    cfg.oom_warning_threshold = 1.1f;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    EXPECT_FALSE(cfg.validate().ok);
}

TEST(GPUConfigTest, OOMThreshold_ExactlyOne_Valid) {
    GPUConfig cfg;
    cfg.oom_warning_threshold = 1.0f;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    EXPECT_TRUE(cfg.validate().ok);
}

// ---------------------------------------------------------------------------
// Circuit breaker thresholds
// ---------------------------------------------------------------------------

TEST(GPUConfigTest, CircuitFailureThreshold_Zero_Invalid) {
    GPUConfig cfg;
    cfg.circuit_failure_threshold = 0;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    EXPECT_FALSE(cfg.validate().ok);
}

TEST(GPUConfigTest, CircuitSuccessThreshold_Zero_Invalid) {
    GPUConfig cfg;
    cfg.circuit_success_threshold = 0;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    EXPECT_FALSE(cfg.validate().ok);
}

TEST(GPUConfigTest, CircuitResetTimeout_Zero_Invalid) {
    GPUConfig cfg;
    cfg.circuit_reset_timeout_secs = 0;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    EXPECT_FALSE(cfg.validate().ok);
}

// ---------------------------------------------------------------------------
// Pool config
// ---------------------------------------------------------------------------

TEST(GPUConfigTest, Pool_ZeroSlabSize_Invalid) {
    GPUConfig cfg;
    cfg.enable_pool    = true;
    cfg.pool_slab_size = 0;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    EXPECT_FALSE(cfg.validate().ok);
}

TEST(GPUConfigTest, Pool_Disabled_ZeroSlabSizeOK) {
    GPUConfig cfg;
    cfg.enable_pool    = false;
    cfg.pool_slab_size = 0;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    EXPECT_TRUE(cfg.validate().ok);
}

// ---------------------------------------------------------------------------
// min_free_vram_bytes vs max_vram_bytes
// ---------------------------------------------------------------------------

TEST(GPUConfigTest, MinFreeVRAM_EqualToMax_Invalid) {
    GPUConfig cfg;
    cfg.max_vram_bytes      = 1024;
    cfg.min_free_vram_bytes = 1024;
    EXPECT_FALSE(cfg.validate().ok);
}

TEST(GPUConfigTest, MinFreeVRAM_GreaterThanMax_Invalid) {
    GPUConfig cfg;
    cfg.max_vram_bytes      = 512;
    cfg.min_free_vram_bytes = 1024;
    EXPECT_FALSE(cfg.validate().ok);
}

// ---------------------------------------------------------------------------
// Multiple errors reported
// ---------------------------------------------------------------------------

TEST(GPUConfigTest, MultipleErrors_AllReported) {
    GPUConfig cfg;
    cfg.oom_warning_threshold    = 0.0f;
    cfg.circuit_failure_threshold = 0;
    cfg.circuit_reset_timeout_secs = 0;
    cfg.max_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    const auto r = cfg.validate();
    EXPECT_FALSE(r.ok);
    EXPECT_GE(r.errors.size(), 3u);
}

// ---------------------------------------------------------------------------
// Warnings (non-fatal)
// ---------------------------------------------------------------------------

TEST(GPUConfigTest, Pool_NumSlabsZero_MaxVramZero_ProducesWarning) {
    GPUConfig cfg;
    cfg.max_vram_bytes = 0;
    cfg.pool_num_slabs = 0;
    cfg.enable_pool    = true;
    // May or may not be valid depending on other fields, but must warn.
    const auto r = cfg.validate();
    EXPECT_FALSE(r.warnings.empty());
}

// ---------------------------------------------------------------------------
// simulateAllocation — dry-run
// ---------------------------------------------------------------------------

TEST(GPUConfigTest, Simulate_FitsWithinLimit_Accepted) {
    GPUConfig cfg;
    cfg.max_vram_bytes      = 8ULL * 1024 * 1024 * 1024;  // 8 GB
    cfg.min_free_vram_bytes = 512ULL * 1024 * 1024;
    cfg.oom_warning_threshold = 0.85f;

    auto [ok, reason] = cfg.simulateAllocation(
        1ULL * 1024 * 1024 * 1024,  // 1 GB request
        0);                          // nothing allocated yet
    EXPECT_TRUE(ok);
    EXPECT_FALSE(reason.empty());
}

TEST(GPUConfigTest, Simulate_ExceedsLimit_Rejected) {
    GPUConfig cfg;
    cfg.max_vram_bytes      = 4ULL * 1024 * 1024 * 1024;
    cfg.min_free_vram_bytes = 0;

    auto [ok, reason] = cfg.simulateAllocation(
        5ULL * 1024 * 1024 * 1024,  // 5 GB request
        0);
    EXPECT_FALSE(ok);
    EXPECT_NE(reason.find("Rejected"), std::string::npos);
}

TEST(GPUConfigTest, Simulate_InsufficientReserve_Rejected) {
    GPUConfig cfg;
    cfg.max_vram_bytes      = 8ULL * 1024 * 1024 * 1024;
    cfg.min_free_vram_bytes = 1ULL * 1024 * 1024 * 1024;  // 1 GB reserve

    // Request 7.5 GB; leaves only 0.5 GB, below 1 GB reserve.
    auto [ok, reason] = cfg.simulateAllocation(
        7ULL * 1024 * 1024 * 1024 + 512ULL * 1024 * 1024,
        0);
    EXPECT_FALSE(ok);
}

TEST(GPUConfigTest, Simulate_AboveOOMThreshold_AcceptedWithWarning) {
    GPUConfig cfg;
    cfg.max_vram_bytes        = 8ULL * 1024 * 1024 * 1024;
    cfg.min_free_vram_bytes   = 0;
    cfg.oom_warning_threshold = 0.80f;

    // Already 6 GB allocated, request 1 GB more → 87.5% which is > 80%.
    auto [ok, reason] = cfg.simulateAllocation(
        1ULL * 1024 * 1024 * 1024,
        6ULL * 1024 * 1024 * 1024);
    EXPECT_TRUE(ok);
    EXPECT_NE(reason.find("warning"), std::string::npos);
}

TEST(GPUConfigTest, Simulate_EditionLimitApplied_WhenMaxVramZero) {
    GPUConfig cfg;
    cfg.max_vram_bytes      = 0;   // use edition default
    cfg.min_free_vram_bytes = 0;

    const uint64_t edition_limit =
        static_cast<uint64_t>(themis::edition::GPU_MAX_VRAM_GB) *
        1024ULL * 1024ULL * 1024ULL;

    if (edition_limit == 0) {
        // GPU not available in this edition.
        auto [ok, reason] = cfg.simulateAllocation(1, 0);
        EXPECT_FALSE(ok);
    } else {
        auto [ok, reason] = cfg.simulateAllocation(1, 0);
        EXPECT_TRUE(ok);
    }
}
