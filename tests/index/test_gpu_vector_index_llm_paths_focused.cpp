/**
 * @file test_gpu_vector_index_llm_paths_focused.cpp
 * @brief Group GV — GPUVectorIndex Vulkan/LLM path and CPU-fallback tests.
 *
 * The GPUVectorIndex includes Vulkan context headers from llm/lora_framework/.
 * These tests verify the LLM-path activation and graceful fallback behaviour
 * when Vulkan/GPU is not available (normal CI environment).
 */

#include <gtest/gtest.h>
#include "index/gpu_vector_index.h"

#include <vector>
#include <string>

using namespace themis;
using namespace themis::index;

// ── GV1: Construct with CPU-only config does not throw ────────────────────────
TEST(GpuVectorIndexLlmPathsFocused, GV1_ConstructCpuOnly_NoThrow) {
    GPUVectorIndex::Config cfg;
    cfg.backend          = GPUVectorIndex::Backend::CPU;
    cfg.allowCPUFallback = true;

    EXPECT_NO_THROW({ GPUVectorIndex idx(cfg); });
}

// ── GV2: Initialize with CPU backend succeeds ─────────────────────────────────
TEST(GpuVectorIndexLlmPathsFocused, GV2_InitCpu_Succeeds) {
    GPUVectorIndex::Config cfg;
    cfg.backend          = GPUVectorIndex::Backend::CPU;
    cfg.allowCPUFallback = true;

    GPUVectorIndex idx(cfg);
    EXPECT_TRUE(idx.initialize(4));
}

// ── GV3: Initialize with AUTO backend + fallback enabled does not crash ───────
TEST(GpuVectorIndexLlmPathsFocused, GV3_InitAuto_FallbackEnabled_NoThrow) {
    GPUVectorIndex::Config cfg;
    cfg.backend          = GPUVectorIndex::Backend::AUTO;
    cfg.allowCPUFallback = true;

    GPUVectorIndex idx(cfg);
    bool ok = false;
    EXPECT_NO_THROW({ ok = idx.initialize(4); });
    EXPECT_TRUE(ok);
}

// ── GV4: With Vulkan requested but fallback enabled, init succeeds on CI ─────
TEST(GpuVectorIndexLlmPathsFocused, GV4_VulkanRequested_FallbackEnabled_InitOkOnCI) {
    GPUVectorIndex::Config cfg;
    cfg.backend          = GPUVectorIndex::Backend::VULKAN;
    cfg.allowCPUFallback = true;  // CI has no Vulkan → falls back to CPU

    GPUVectorIndex idx(cfg);
    bool ok = false;
    EXPECT_NO_THROW({ ok = idx.initialize(4); });
    EXPECT_TRUE(ok);
}

// ── GV5: Without CPU fallback and no GPU, init returns false (not crash) ──────
TEST(GpuVectorIndexLlmPathsFocused, GV5_VulkanRequested_NoFallback_ReturnsFalseOrTrue) {
    GPUVectorIndex::Config cfg;
    cfg.backend          = GPUVectorIndex::Backend::VULKAN;
    cfg.allowCPUFallback = false;

    GPUVectorIndex idx(cfg);
    bool ok = false;
    // On CI without GPU this is expected to return false; with GPU it may be true
    EXPECT_NO_THROW({ ok = idx.initialize(4); });
    (void)ok;  // Outcome is environment-dependent; we only verify no crash
}

// ── GV6: addVector + search roundtrip on CPU backend returns results ──────────
TEST(GpuVectorIndexLlmPathsFocused, GV6_AddThenSearch_CPU_ReturnsResults) {
    GPUVectorIndex::Config cfg;
    cfg.backend          = GPUVectorIndex::Backend::CPU;
    cfg.allowCPUFallback = true;

    GPUVectorIndex idx(cfg);
    ASSERT_TRUE(idx.initialize(4));

    ASSERT_TRUE(idx.addVector("a", {1.f, 0.f, 0.f, 0.f}));
    ASSERT_TRUE(idx.addVector("b", {0.f, 1.f, 0.f, 0.f}));
    ASSERT_TRUE(idx.addVector("c", {0.f, 0.f, 1.f, 0.f}));

    auto results = idx.search({1.f, 0.f, 0.f, 0.f}, 1);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, "a");
}

// ── GV7: getActiveBackend on CPU-only init returns CPU ────────────────────────
TEST(GpuVectorIndexLlmPathsFocused, GV7_GetActiveBackend_CPU_ReturnsCPU) {
    GPUVectorIndex::Config cfg;
    cfg.backend          = GPUVectorIndex::Backend::CPU;
    cfg.allowCPUFallback = true;

    GPUVectorIndex idx(cfg);
    ASSERT_TRUE(idx.initialize(4));

    EXPECT_EQ(idx.getActiveBackend(), GPUVectorIndex::Backend::CPU);
}

// ── GV8: getAvailableBackends always includes CPU ────────────────────────────
TEST(GpuVectorIndexLlmPathsFocused, GV8_GetAvailableBackends_IncludesCPU) {
    GPUVectorIndex::Config cfg;
    cfg.backend          = GPUVectorIndex::Backend::AUTO;
    cfg.allowCPUFallback = true;

    GPUVectorIndex idx(cfg);
    ASSERT_TRUE(idx.initialize(4));

    auto backends = idx.getAvailableBackends();
    bool has_cpu = false;
    for (auto b : backends) {
        if (b == GPUVectorIndex::Backend::CPU) {
          has_cpu = true;
        }
    }
    EXPECT_TRUE(has_cpu);
}
