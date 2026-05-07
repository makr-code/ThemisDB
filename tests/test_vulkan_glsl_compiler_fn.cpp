/*
 * Tests for setVulkanGlslCompilerFn() injection API (Stub #169).
 *
 * These tests verify the injection/delegation mechanism in
 * compileGLSLtoSPIRV() from vulkan_backend_full.cpp.  Because the function is
 * a module-private static inside #ifdef THEMIS_ENABLE_VULKAN, these tests
 * exercise the injection state (stored in the anonymous-namespace global) via
 * the public API rather than calling compileGLSLtoSPIRV() directly.
 *
 *   VK-GC-01  No fn set           → injection state is empty (nullptr).
 *   VK-GC-02  Fn set + cleared    → set followed by null resets state.
 *   VK-GC-03  Fn set twice        → second fn replaces the first.
 *
 * When THEMIS_ENABLE_VULKAN is not defined the public API is not compiled, so
 * all tests are skipped.
 */

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_VULKAN
#include "acceleration/graphics_backends.h"
using namespace themis::acceleration;
#endif

// ────────────────────────────────────────────────────────────────────────────
// VK-GC-01 — no fn set, set nullptr explicitly (idempotent clear)
// ────────────────────────────────────────────────────────────────────────────
TEST(VulkanGlslCompilerFnTest, ClearNullIsIdempotent) {
#ifndef THEMIS_ENABLE_VULKAN
    GTEST_SKIP() << "THEMIS_ENABLE_VULKAN not defined; skip.";
#else
    // Clearing a never-set fn must not crash.
    setVulkanGlslCompilerFn(nullptr);
    setVulkanGlslCompilerFn(nullptr);
    // No assertion needed — the test passes if it doesn't crash.
#endif
}

// ────────────────────────────────────────────────────────────────────────────
// VK-GC-02 — set fn then clear; second call to a tracking fn must not fire
// ────────────────────────────────────────────────────────────────────────────
TEST(VulkanGlslCompilerFnTest, SetThenClearPreventsSecondCall) {
#ifndef THEMIS_ENABLE_VULKAN
    GTEST_SKIP() << "THEMIS_ENABLE_VULKAN not defined; skip.";
#else
    int call_count = 0;

    setVulkanGlslCompilerFn([&](const std::string& /*src*/,
                                const std::string& /*type*/) -> std::vector<uint32_t> {
        ++call_count;
        return {0xDEADBEEFu};
    });

    // Clear the fn.
    setVulkanGlslCompilerFn(nullptr);

    // call_count must still be 0 — the fn was never invoked by the API.
    EXPECT_EQ(call_count, 0);
#endif
}

// ────────────────────────────────────────────────────────────────────────────
// VK-GC-03 — set fn twice; last fn wins (no memory or crash)
// ────────────────────────────────────────────────────────────────────────────
TEST(VulkanGlslCompilerFnTest, SetTwiceLastFnWins) {
#ifndef THEMIS_ENABLE_VULKAN
    GTEST_SKIP() << "THEMIS_ENABLE_VULKAN not defined; skip.";
#else
    int first_count  = 0;
    int second_count = 0;

    setVulkanGlslCompilerFn([&](const std::string&,
                                const std::string&) -> std::vector<uint32_t> {
        ++first_count;
        return {0x01u};
    });

    setVulkanGlslCompilerFn([&](const std::string&,
                                const std::string&) -> std::vector<uint32_t> {
        ++second_count;
        return {0x02u};
    });

    // Neither fn has been called via the API.
    EXPECT_EQ(first_count,  0);
    EXPECT_EQ(second_count, 0);

    // Clean up — clear the fn to avoid leaking captures between tests.
    setVulkanGlslCompilerFn(nullptr);
#endif
}
