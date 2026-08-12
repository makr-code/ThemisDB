/*
 * Tests for VulkanVectorIndexBackend callback bridge (STUB #54)
 *
 * Covers: VVI-BRIDGE-01..VVI-BRIDGE-04
 *   VVI-BRIDGE-01 — initialize() delegates to injected InitializeFn
 *   VVI-BRIDGE-02 — uploadVectors() delegates to injected UploadFn
 *   VVI-BRIDGE-03 — search() delegates to injected SearchFn
 *   VVI-BRIDGE-04 — searchBatch() delegates to injected SearchBatchFn; exception fail-closes to {}
 */

#include <gtest/gtest.h>

// Pull in the .cpp directly so we always exercise the #else (non-Vulkan) stub path.
// We do this by forcing THEMIS_HAS_VULKAN_IMPL=0 before the include.
#undef THEMIS_HAS_VULKAN_IMPL
#define THEMIS_HAS_VULKAN_IMPL 0

#include "index/gpu_vector_index.h"
#include <functional>
#include <stdexcept>
#include <vector>

// ─── helpers ─────────────────────────────────────────────────────────────────

// Re-include the class declaration only (not the full .cpp); the class is
// forward-declared in the .cpp itself.  We reach the bridge setters through
// the header-level declaration that was added to gpu_vector_index_vulkan.cpp.
// Because the class is defined entirely inside that TU, we do a forward-decl
// trick: link against the TU and use the static setters via extern linkage.

// Actually, since VulkanVectorIndexBackend is only declared inside the .cpp,
// we need to access it via the GPUVectorIndex public API or include the TU.
// For this test we compile together with the TU and access the static setters.

// The simplest approach: compile this test together with the source TU so that
// the class definition is visible.  We forward-declare the parts we need.

namespace themis {
namespace index {

// Forward-declare just enough of the class to call the static setters.
// (The full definition lives in gpu_vector_index_vulkan.cpp.)
class VulkanVectorIndexBackend;

} // namespace index
} // namespace themis

// We include the source file to get the full class definition in scope.
// This is standard practice for white-box tests on classes without public headers.
#include "../src/index/gpu_vector_index_vulkan.cpp"  // NOLINT

using namespace themis::index;
using SR = themis::index::GPUVectorIndex::SearchResult;

// ─── fixture ─────────────────────────────────────────────────────────────────

class VulkanVectorIndexBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear all bridge callbacks before each test.
        VulkanVectorIndexBackend::setInitializeFn(nullptr);
        VulkanVectorIndexBackend::setUploadFn(nullptr);
        VulkanVectorIndexBackend::setSearchFn(nullptr);
        VulkanVectorIndexBackend::setSearchBatchFn(nullptr);
    }
    void TearDown() override {
        VulkanVectorIndexBackend::setInitializeFn(nullptr);
        VulkanVectorIndexBackend::setUploadFn(nullptr);
        VulkanVectorIndexBackend::setSearchFn(nullptr);
        VulkanVectorIndexBackend::setSearchBatchFn(nullptr);
    }
};

// ─── VVI-BRIDGE-01: InitializeFn ─────────────────────────────────────────────

TEST_F(VulkanVectorIndexBridgeTest, InitializeFnIsCalledAndReturnsTrueWhenSet) {
    bool called = false;
    int received_dim = -1;
    VulkanVectorIndexBackend::setInitializeFn([&](int dim) {
        called = true;
        received_dim = dim;
        return true;
    });

    GPUVectorIndex::Config cfg;
    VulkanVectorIndexBackend backend(cfg);
    EXPECT_FALSE(backend.isInitialized()); // before initialization

    bool ok = backend.initialize(128);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(called);
    EXPECT_EQ(received_dim, 128);
    EXPECT_TRUE(backend.isInitialized());
}

TEST_F(VulkanVectorIndexBridgeTest, InitializeFnNotSetReturnsFalse) {
    GPUVectorIndex::Config cfg;
    VulkanVectorIndexBackend backend(cfg);
    EXPECT_FALSE(backend.initialize(64));
    EXPECT_FALSE(backend.isInitialized());
}

// ─── VVI-BRIDGE-02: UploadFn ─────────────────────────────────────────────────

TEST_F(VulkanVectorIndexBridgeTest, UploadVectorsFnIsCalledWithCorrectData) {
    std::vector<std::vector<float>> received;
    VulkanVectorIndexBackend::setUploadFn([&](const std::vector<std::vector<float>>& vecs) {
        received = vecs;
        return true;
    });

    GPUVectorIndex::Config cfg;
    VulkanVectorIndexBackend backend(cfg);
    std::vector<std::vector<float>> data = {{1.0f, 0.0f}, {0.0f, 1.0f}};
    EXPECT_TRUE(backend.uploadVectors(data));
    EXPECT_EQ(received.size(), 2u);
}

TEST_F(VulkanVectorIndexBridgeTest, UploadVectorsFnNotSetReturnsFalse) {
    GPUVectorIndex::Config cfg;
    VulkanVectorIndexBackend backend(cfg);
    EXPECT_FALSE(backend.uploadVectors({{1.0f}}));
}

// ─── VVI-BRIDGE-03: SearchFn ─────────────────────────────────────────────────

TEST_F(VulkanVectorIndexBridgeTest, SearchFnIsCalledAndReturnsResults) {
    SR result;
    result.id       = "42";
    result.distance = 0.99f;

    VulkanVectorIndexBackend::setSearchFn(
        [&](const std::vector<float>&, size_t) -> std::vector<SR> {
            return {result};
        });

    GPUVectorIndex::Config cfg;
    VulkanVectorIndexBackend backend(cfg);
    auto res = backend.search({0.1f, 0.2f}, 5);
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].id, "42");
    EXPECT_FLOAT_EQ(res[0].distance, 0.99f);
}

TEST_F(VulkanVectorIndexBridgeTest, SearchFnNotSetReturnsEmpty) {
    GPUVectorIndex::Config cfg;
    VulkanVectorIndexBackend backend(cfg);
    EXPECT_TRUE(backend.search({0.1f}, 3).empty());
}

// ─── VVI-BRIDGE-04: SearchBatchFn, exception fail-closes ─────────────────────

TEST_F(VulkanVectorIndexBridgeTest, SearchBatchFnIsCalledAndReturnsResults) {
    VulkanVectorIndexBackend::setSearchBatchFn(
        [](const std::vector<std::vector<float>>& qs, size_t k)
            -> std::vector<std::vector<SR>> {
            std::vector<std::vector<SR>> out;
            for (size_t i = 0; i < qs.size(); ++i) {
                SR sr;
                sr.id       = std::to_string(i);
                sr.distance = 1.0f / (1.0f + static_cast<float>(i));
                out.push_back({sr});
            }
            (void)k;
            return out;
        });

    GPUVectorIndex::Config cfg;
    VulkanVectorIndexBackend backend(cfg);
    std::vector<std::vector<float>> queries = {{1.0f}, {0.5f}};
    auto res = backend.searchBatch(queries, 3);
    ASSERT_EQ(res.size(), 2u);
    EXPECT_EQ(res[0][0].id, "0");
    EXPECT_EQ(res[1][0].id, "1");
}

TEST_F(VulkanVectorIndexBridgeTest, SearchBatchFnExceptionFailClosesToEmpty) {
    VulkanVectorIndexBackend::setSearchBatchFn(
        [](const std::vector<std::vector<float>>&, size_t)
            -> std::vector<std::vector<SR>> {
            throw std::runtime_error("simulated GPU error");
        });

    GPUVectorIndex::Config cfg;
    VulkanVectorIndexBackend backend(cfg);
    auto res = backend.searchBatch({{1.0f}}, 3);
    EXPECT_TRUE(res.empty()); // fail-closed
}
