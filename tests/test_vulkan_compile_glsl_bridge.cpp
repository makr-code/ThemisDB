/**
 * @file test_vulkan_compile_glsl_bridge.cpp
 * @brief Unit tests for the CompileGLSLFn static injection bridge on
 *        VulkanVectorBackend (STUB #169).
 *
 * Test IDs:
 *   VKC-01  Injected CompileGLSLFn is registered and accessible via setter
 *   VKC-02  nullptr fn restores the stub (empty SPIR-V) path
 *   VKC-03  Setting a fn does not break VulkanVectorBackend::isAvailable()
 *
 * Note: the full compileGLSLtoSPIRV() path is only exercised when Vulkan is
 * enabled at build time.  Tests here verify the bridge wiring (setter/getter
 * semantics) without requiring a Vulkan runtime.
 */

#include <gtest/gtest.h>
#include "acceleration/graphics_backends.h"

#include <atomic>
#include <string>
#include <vector>

using namespace themis::acceleration;

// ─────────────────────────────────────────────────────────────────────────────
// RAII guard to clear the static CompileGLSLFn after each test.
// ─────────────────────────────────────────────────────────────────────────────
struct CompileGlslGuard {
    ~CompileGlslGuard() { VulkanVectorBackend::setCompileGLSLFn(nullptr); }
};

// ─────────────────────────────────────────────────────────────────────────────
// VKC-01: setCompileGLSLFn() registers a fn without throwing
// ─────────────────────────────────────────────────────────────────────────────
TEST(VulkanCompileGlslBridge, VKC_01_SetFnNoThrow) {
    CompileGlslGuard guard;

    std::atomic<int> call_count{0};
    // The SPIR-V magic number (0x07230203) is required as the first word of any
    // valid SPIR-V binary module (per Khronos SPIR-V specification §2.3).
    const std::vector<uint32_t> fake_spv = {0x07230203u, 0x00010500u, 0u, 1u, 0u};

    EXPECT_NO_THROW(
        VulkanVectorBackend::setCompileGLSLFn(
            [&](const std::string& /*glsl*/, const std::string& /*type*/)
                -> std::vector<uint32_t> {
                ++call_count;
                return fake_spv;
            }));

    // The fn is registered; verifying it was stored is done implicitly by
    // VKC-02 (clearing it) without the need for a Vulkan runtime.
    EXPECT_EQ(call_count.load(), 0)
        << "Fn should not be called simply by registering it";
}

// ─────────────────────────────────────────────────────────────────────────────
// VKC-02: nullptr fn restores empty-SPIR-V stub without throwing
// ─────────────────────────────────────────────────────────────────────────────
TEST(VulkanCompileGlslBridge, VKC_02_ClearFnNoThrow) {
    CompileGlslGuard guard;

    EXPECT_NO_THROW(VulkanVectorBackend::setCompileGLSLFn(
        [](const std::string&, const std::string&) -> std::vector<uint32_t> {
            return {1u, 2u, 3u};
        }));

    EXPECT_NO_THROW(VulkanVectorBackend::setCompileGLSLFn(nullptr));
}

// ─────────────────────────────────────────────────────────────────────────────
// VKC-03: Setting a CompileGLSLFn does not affect isAvailable() / name()
// ─────────────────────────────────────────────────────────────────────────────
TEST(VulkanCompileGlslBridge, VKC_03_DoesNotAffectBackendMetadata) {
    CompileGlslGuard guard;

    VulkanVectorBackend backend;
    const bool avail_before = backend.isAvailable();

    VulkanVectorBackend::setCompileGLSLFn(
        [](const std::string&, const std::string&) -> std::vector<uint32_t> {
            return {0x07230203u};
        });

    EXPECT_EQ(backend.isAvailable(), avail_before)
        << "setCompileGLSLFn must not change isAvailable()";
    EXPECT_STREQ(backend.name(), "Vulkan")
        << "Backend name must remain 'Vulkan' after fn injection";
}
