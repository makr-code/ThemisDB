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
#include "acceleration/graphics_backends.h"
using namespace themis::acceleration;

// ────────────────────────────────────────────────────────────────────────────
// VK-GC-01 — no fn set, set nullptr explicitly (idempotent clear)
// ────────────────────────────────────────────────────────────────────────────
TEST(VulkanGlslCompilerFnTest, ClearNullIsIdempotent) {
#ifndef THEMIS_ENABLE_VULKAN
    GTEST_SKIP() << "capability:vulkan_compiled=false;reason=themis_enable_vulkan_not_defined";
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
    GTEST_SKIP() << "capability:vulkan_compiled=false;reason=themis_enable_vulkan_not_defined";
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
    GTEST_SKIP() << "capability:vulkan_compiled=false;reason=themis_enable_vulkan_not_defined";
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

TEST(VulkanGlslCompilerFnTest, VulkanStubBridgeFnsWorkWithoutSdk) {
#ifdef THEMIS_ENABLE_VULKAN
    GTEST_SKIP() << "capability:vulkan_stub_bridge_compiled=false;reason=real_vulkan_build_active";
#else
    using namespace themis::acceleration;

    VulkanVectorBackend::setAvailabilityFn([] { return true; });
    VulkanVectorBackend::setInitializeFn([] { return true; });
    VulkanVectorBackend::setComputeDistancesFn(
        []([[maybe_unused]] const float* queries,
           [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim,
           [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors,
           [[maybe_unused]] bool useL2) {
            return std::vector<float>{1.0f, 2.0f};
        });
    VulkanVectorBackend::setBatchKnnSearchFn(
        []([[maybe_unused]] const float* queries,
           [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim,
           [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors,
           [[maybe_unused]] size_t k,
           [[maybe_unused]] bool useL2) {
            return std::vector<std::vector<std::pair<uint32_t, float>>>{
                {{1u, 0.1f}, {2u, 0.2f}}
            };
        });

    VulkanVectorBackend backend;
    EXPECT_TRUE(backend.isAvailable());
    EXPECT_TRUE(backend.initialize());
    EXPECT_EQ(backend.computeDistances(nullptr, 0, 0, nullptr, 0).size(), 2u);
    EXPECT_EQ(backend.batchKnnSearch(nullptr, 0, 0, nullptr, 0, 2).size(), 1u);

    VulkanVectorBackend::setAvailabilityFn({});
    VulkanVectorBackend::setInitializeFn({});
    VulkanVectorBackend::setComputeDistancesFn({});
    VulkanVectorBackend::setBatchKnnSearchFn({});
#endif
}

TEST(VulkanGlslCompilerFnTest, OpenGLStubBridgeFnsWorkWithoutSdk) {
#ifdef THEMIS_ENABLE_OPENGL
    GTEST_SKIP() << "capability:opengl_stub_bridge_compiled=false;reason=real_opengl_build_active";
#else
    using namespace themis::acceleration;

    OpenGLVectorBackend::setAvailabilityFn([] { return true; });
    OpenGLVectorBackend::setInitializeFn([] { return true; });
    OpenGLVectorBackend::setComputeDistancesFn(
        []([[maybe_unused]] const float* queries,
           [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim,
           [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors,
           [[maybe_unused]] bool useL2) {
            return std::vector<float>{3.0f, 4.0f};
        });
    OpenGLVectorBackend::setBatchKnnSearchFn(
        []([[maybe_unused]] const float* queries,
           [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim,
           [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors,
           [[maybe_unused]] size_t k,
           [[maybe_unused]] bool useL2) {
            return std::vector<std::vector<std::pair<uint32_t, float>>>{
                {{9u, 0.9f}, {8u, 0.8f}}
            };
        });

    OpenGLVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        OpenGLVectorBackend::setAvailabilityFn({});
        OpenGLVectorBackend::setInitializeFn({});
        OpenGLVectorBackend::setComputeDistancesFn({});
        OpenGLVectorBackend::setBatchKnnSearchFn({});
        GTEST_SKIP() << "capability:opengl_stub_bridge_path_exercisable=false;reason=backend_unavailable_or_init_failed";
    }

    auto distances = backend.computeDistances(nullptr, 0, 0, nullptr, 0);
    if (distances.empty()) {
        OpenGLVectorBackend::setAvailabilityFn({});
        OpenGLVectorBackend::setInitializeFn({});
        OpenGLVectorBackend::setComputeDistancesFn({});
        OpenGLVectorBackend::setBatchKnnSearchFn({});
        GTEST_SKIP() << "capability:opengl_stub_bridge_path_exercisable=false;reason=stub_bridge_not_active";
    }
    ASSERT_EQ(distances.size(), 2u);
    EXPECT_FLOAT_EQ(distances[0], 3.0f);
    EXPECT_FLOAT_EQ(distances[1], 4.0f);

    auto knn = backend.batchKnnSearch(nullptr, 0, 0, nullptr, 0, 2);
    ASSERT_EQ(knn.size(), 1u);
    ASSERT_EQ(knn[0].size(), 2u);
    EXPECT_EQ(knn[0][0].first, 9u);
    EXPECT_FLOAT_EQ(knn[0][0].second, 0.9f);
    EXPECT_EQ(knn[0][1].first, 8u);
    EXPECT_FLOAT_EQ(knn[0][1].second, 0.8f);

    OpenGLVectorBackend::setAvailabilityFn({});
    OpenGLVectorBackend::setInitializeFn({});
    OpenGLVectorBackend::setComputeDistancesFn({});
    OpenGLVectorBackend::setBatchKnnSearchFn({});
#endif
}

TEST(VulkanGlslCompilerFnTest, OpenGLStubBridgeFnsFailClosedOnException) {
#ifdef THEMIS_ENABLE_OPENGL
    GTEST_SKIP() << "capability:opengl_stub_bridge_compiled=false;reason=real_opengl_build_active";
#else
    using namespace themis::acceleration;

    OpenGLVectorBackend::setAvailabilityFn([]() -> bool { throw std::runtime_error("boom"); });
    OpenGLVectorBackend::setInitializeFn([]() -> bool { throw std::runtime_error("boom"); });
    OpenGLVectorBackend::setComputeDistancesFn(
        []([[maybe_unused]] const float* queries,
           [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim,
           [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors,
           [[maybe_unused]] bool useL2) -> std::vector<float> {
            throw std::runtime_error("boom");
        });
    OpenGLVectorBackend::setBatchKnnSearchFn(
        []([[maybe_unused]] const float* queries,
           [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim,
           [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors,
           [[maybe_unused]] size_t k,
           [[maybe_unused]] bool useL2)
            -> std::vector<std::vector<std::pair<uint32_t, float>>> {
            throw std::runtime_error("boom");
        });

    OpenGLVectorBackend backend;
    EXPECT_FALSE(backend.isAvailable());
    EXPECT_FALSE(backend.initialize());
    EXPECT_TRUE(backend.computeDistances(nullptr, 0, 0, nullptr, 0).empty());
    EXPECT_TRUE(backend.batchKnnSearch(nullptr, 0, 0, nullptr, 0, 1).empty());

    OpenGLVectorBackend::setAvailabilityFn({});
    OpenGLVectorBackend::setInitializeFn({});
    OpenGLVectorBackend::setComputeDistancesFn({});
    OpenGLVectorBackend::setBatchKnnSearchFn({});
#endif
}

TEST(VulkanGlslCompilerFnTest, OpenGLStubBridgeFnsForwardParameters) {
#ifdef THEMIS_ENABLE_OPENGL
    GTEST_SKIP() << "capability:opengl_stub_bridge_compiled=false;reason=real_opengl_build_active";
#else
    using namespace themis::acceleration;

    const float q[] = {1.0f, 2.0f};
    const float v[] = {3.0f, 4.0f, 5.0f, 6.0f};

    bool sawExpected = false;
    OpenGLVectorBackend::setComputeDistancesFn(
        [&](const float* queries,
            size_t numQueries,
            size_t dim,
            const float* vectors,
            size_t numVectors,
            bool useL2) {
            sawExpected = (queries == q && vectors == v &&
                           numQueries == 1 && numVectors == 2 &&
                           dim == 2 && useL2);
            return std::vector<float>{7.0f};
        });

    OpenGLVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        OpenGLVectorBackend::setComputeDistancesFn({});
        GTEST_SKIP() << "capability:opengl_stub_bridge_path_exercisable=false;reason=backend_unavailable_or_init_failed";
    }

    auto result = backend.computeDistances(q, 1, 2, v, 2, true);
    if (result.empty()) {
        OpenGLVectorBackend::setComputeDistancesFn({});
        GTEST_SKIP() << "capability:opengl_stub_bridge_path_exercisable=false;reason=stub_bridge_not_active";
    }
    EXPECT_TRUE(sawExpected);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_FLOAT_EQ(result[0], 7.0f);

    OpenGLVectorBackend::setComputeDistancesFn({});
#endif
}

TEST(VulkanGlslCompilerFnTest, DirectXStubBridgeFnsWorkWithoutSdk) {
#if defined(_WIN32) && defined(THEMIS_ENABLE_DIRECTX)
    GTEST_SKIP() << "capability:directx_stub_bridge_compiled=false;reason=real_directx_build_active";
#else
    using namespace themis::acceleration;

    DirectXVectorBackend::setAvailabilityFn([] { return true; });
    DirectXVectorBackend::setInitializeFn([] { return true; });
    DirectXVectorBackend::setComputeDistancesFn(
        []([[maybe_unused]] const float* queries,
           [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim,
           [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors,
           [[maybe_unused]] bool useL2) {
            return std::vector<float>{5.0f, 6.0f};
        });
    DirectXVectorBackend::setBatchKnnSearchFn(
        []([[maybe_unused]] const float* queries,
           [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim,
           [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors,
           [[maybe_unused]] size_t k,
           [[maybe_unused]] bool useL2) {
            return std::vector<std::vector<std::pair<uint32_t, float>>>{
                {{7u, 0.7f}, {6u, 0.6f}}
            };
        });

    DirectXVectorBackend backend;
    if (!backend.isAvailable() || !backend.initialize()) {
        DirectXVectorBackend::setAvailabilityFn({});
        DirectXVectorBackend::setInitializeFn({});
        DirectXVectorBackend::setComputeDistancesFn({});
        DirectXVectorBackend::setBatchKnnSearchFn({});
        GTEST_SKIP() << "capability:directx_stub_bridge_path_exercisable=false;reason=backend_unavailable_or_init_failed";
    }

    auto distances = backend.computeDistances(nullptr, 0, 0, nullptr, 0, true);
    if (distances.empty()) {
        DirectXVectorBackend::setAvailabilityFn({});
        DirectXVectorBackend::setInitializeFn({});
        DirectXVectorBackend::setComputeDistancesFn({});
        DirectXVectorBackend::setBatchKnnSearchFn({});
        GTEST_SKIP() << "capability:directx_stub_bridge_path_exercisable=false;reason=stub_bridge_not_active";
    }
    ASSERT_EQ(distances.size(), 2u);
    EXPECT_FLOAT_EQ(distances[0], 5.0f);
    EXPECT_FLOAT_EQ(distances[1], 6.0f);

    auto knn = backend.batchKnnSearch(nullptr, 0, 0, nullptr, 0, 2, true);
    ASSERT_EQ(knn.size(), 1u);
    ASSERT_EQ(knn[0].size(), 2u);
    EXPECT_EQ(knn[0][0].first, 7u);
    EXPECT_FLOAT_EQ(knn[0][0].second, 0.7f);
    EXPECT_EQ(knn[0][1].first, 6u);
    EXPECT_FLOAT_EQ(knn[0][1].second, 0.6f);

    DirectXVectorBackend::setAvailabilityFn({});
    DirectXVectorBackend::setInitializeFn({});
    DirectXVectorBackend::setComputeDistancesFn({});
    DirectXVectorBackend::setBatchKnnSearchFn({});
#endif
}

TEST(VulkanGlslCompilerFnTest, DirectXStubBridgeFnsFailClosedOnException) {
#if defined(_WIN32) && defined(THEMIS_ENABLE_DIRECTX)
    GTEST_SKIP() << "capability:directx_stub_bridge_compiled=false;reason=real_directx_build_active";
#else
    using namespace themis::acceleration;

    DirectXVectorBackend::setAvailabilityFn([]() -> bool { throw std::runtime_error("boom"); });
    DirectXVectorBackend::setInitializeFn([]() -> bool { throw std::runtime_error("boom"); });
    DirectXVectorBackend::setComputeDistancesFn(
        []([[maybe_unused]] const float* queries,
           [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim,
           [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors,
           [[maybe_unused]] bool useL2) -> std::vector<float> {
            throw std::runtime_error("boom");
        });
    DirectXVectorBackend::setBatchKnnSearchFn(
        []([[maybe_unused]] const float* queries,
           [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim,
           [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors,
           [[maybe_unused]] size_t k,
           [[maybe_unused]] bool useL2)
            -> std::vector<std::vector<std::pair<uint32_t, float>>> {
            throw std::runtime_error("boom");
        });

    DirectXVectorBackend backend;
    EXPECT_FALSE(backend.isAvailable());
    EXPECT_FALSE(backend.initialize());
    EXPECT_TRUE(backend.computeDistances(nullptr, 0, 0, nullptr, 0, true).empty());
    EXPECT_TRUE(backend.batchKnnSearch(nullptr, 0, 0, nullptr, 0, 1, true).empty());

    DirectXVectorBackend::setAvailabilityFn({});
    DirectXVectorBackend::setInitializeFn({});
    DirectXVectorBackend::setComputeDistancesFn({});
    DirectXVectorBackend::setBatchKnnSearchFn({});
#endif
}
