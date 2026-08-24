/**
 * @file test_acceleration_vllm_resource_manager_focused.cpp
 * @brief Group VM — VLLMResourceManager resource-allocation and LLM co-location tests.
 */

#include <gtest/gtest.h>
#include "acceleration/vllm_resource_manager.h"

#include <memory>
#include <string>

using namespace themis::acceleration;

// ── VM1: Construct with default config does not throw ────────────────────────
TEST(VllmResourceManagerFocused, VM1_DefaultConfig_ConstructDoesNotThrow) {
    VLLMResourceManager::Config cfg;
    EXPECT_NO_THROW({ VLLMResourceManager mgr(cfg); });
}

// ── VM2: getConfig returns the config passed at construction ─────────────────
TEST(VllmResourceManagerFocused, VM2_GetConfig_MatchesConstructorInput) {
    VLLMResourceManager::Config cfg;
    cfg.total_cpu_cores = 8;
    cfg.total_ram_gb    = 32;
    cfg.vllm_cpu_cores  = 2;

    VLLMResourceManager mgr(cfg);
    const auto& got = mgr.getConfig();
    EXPECT_EQ(got.total_cpu_cores, 8u);
    EXPECT_EQ(got.total_ram_gb, 32u);
    EXPECT_EQ(got.vllm_cpu_cores, 2u);
}

// ── VM3: getStats without initialize → valid Stats struct (no crash) ──────────
TEST(VllmResourceManagerFocused, VM3_GetStats_BeforeInit_NoThrow) {
    VLLMResourceManager::Config cfg;
    VLLMResourceManager mgr(cfg);

    VLLMResourceManager::Stats stats;
    EXPECT_NO_THROW({ stats = mgr.getStats(); });
    // Utilization values must be in valid range
    EXPECT_GE(stats.cpu_utilization, 0.0);
    EXPECT_GE(stats.ram_utilization, 0.0);
}

// ── VM4: initialize returns bool without throwing ─────────────────────────────
TEST(VllmResourceManagerFocused, VM4_Initialize_ReturnsBool_NoThrow) {
    VLLMResourceManager::Config cfg;
    VLLMResourceManager mgr(cfg);

    bool result = false;
    EXPECT_NO_THROW({ result = mgr.initialize(); });
    // Pass or fail depending on hardware — either is acceptable in CI
    (void)result;
}

// ── VM5: shutdown after init does not throw ────────────────────────────────────
TEST(VllmResourceManagerFocused, VM5_Shutdown_AfterInit_NoThrow) {
    VLLMResourceManager::Config cfg;
    VLLMResourceManager mgr(cfg);
    mgr.initialize();
    EXPECT_NO_THROW({ mgr.shutdown(); });
}

// ── VM6: canUseGPU() returns false when no GPU is present (CI env) ────────────
TEST(VllmResourceManagerFocused, VM6_CanUseGPU_DoesNotThrow) {
    VLLMResourceManager::Config cfg;
    VLLMResourceManager mgr(cfg);

    bool result = false;
    EXPECT_NO_THROW({ result = mgr.canUseGPU(); });
    // In GPU-less CI this will be false; we only verify no crash
    (void)result;
}

// ── VM7: SimilarityDispatch with null pointers returns failure ────────────────
TEST(VllmResourceManagerFocused, VM7_Dispatch_NullPointers_ReturnsFailed) {
    VLLMResourceManager::Config cfg;
    VLLMResourceManager mgr(cfg);
    mgr.initialize();

    // Pass null data — must not crash and must signal failure
    // Signature: dispatchVectorSimilarity(queries, num_queries, dim, vectors, num_vectors, top_k)
    auto result = mgr.dispatchVectorSimilarity(
        nullptr,   // query vectors
        0,         // num_queries
        64,        // dim
        nullptr,   // db vectors
        0,         // num_vectors
        4          // top_k
    );
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

// ── VM8: SimilarityDispatch zero-dim returns failure ────────────────────────
TEST(VllmResourceManagerFocused, VM8_Dispatch_ZeroDim_ReturnsFailed) {
    VLLMResourceManager::Config cfg;
    VLLMResourceManager mgr(cfg);
    mgr.initialize();

    float q[4] = {1.f, 0.f, 0.f, 0.f};
    float db[4] = {1.f, 0.f, 0.f, 0.f};

    // Signature: (queries, num_queries, dim, vectors, num_vectors, top_k)
    auto result = mgr.dispatchVectorSimilarity(q, 1, /*dim=*/0, db, 1, /*top_k=*/1);
    EXPECT_FALSE(result.success);
}

// ── VM9: double shutdown does not throw ────────────────────────────────────────
TEST(VllmResourceManagerFocused, VM9_DoubleShutdown_NoThrow) {
    VLLMResourceManager::Config cfg;
    VLLMResourceManager mgr(cfg);
    mgr.initialize();
    mgr.shutdown();
    EXPECT_NO_THROW({ mgr.shutdown(); });
}
