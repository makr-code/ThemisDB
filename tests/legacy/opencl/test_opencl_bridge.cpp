/**
 * @file test_opencl_bridge.cpp
 * @brief Unit tests for OpenCLVectorBackend/OneAPIVectorBackend injectable
 *        computeDistances bridges (STUB #69, STUB #70).
 *
 * Tests verify the computeDistances injectable callback slot:
 *   OCL-BRIDGE-01  no fn injected → computeDistances returns {} (stub default)
 *   OCL-BRIDGE-02  fn injected → fn is called, its return value is propagated
 *   OCL-BRIDGE-03  fn throws → computeDistances returns {} (fail-closed)
 *   ONEAPI-BRIDGE-01..03  same pattern for the OneAPI stub free-function bridge
 */

#include <gtest/gtest.h>
#include "acceleration/opencl_backend.h"
#include <functional>
#include <memory>
#include <vector>

using themis::acceleration::OpenCLVectorBackend;

// Forward declaration for the OneAPI stub free-function bridge
// (class is local to oneapi_backend.cpp; exposed via this free function in
//  non-ONEAPI builds only).
#ifndef THEMIS_ENABLE_ONEAPI
namespace themis { namespace acceleration {
void setOneAPIComputeDistancesFn(
    std::function<std::vector<float>(
        const float*, size_t, size_t, const float*, size_t, bool)> fn);
std::unique_ptr<IVectorBackend> createOneAPIBackend();
}} // namespace themis::acceleration
#endif

#ifdef THEMIS_ENABLE_ZLUDA
namespace themis { namespace acceleration {
void setZLUDAComputeDistancesFn(
    std::function<std::vector<float>(
        const float*, size_t, size_t, const float*, size_t, bool)> fn);
void setZLUDABatchKnnSearchFn(
    std::function<std::vector<std::vector<std::pair<uint32_t, float>>>(
        const float*, size_t, size_t, const float*, size_t, size_t, bool)> fn);
std::unique_ptr<IVectorBackend> createZLUDABackend();
}} // namespace themis::acceleration
#endif

// ── Fixture ───────────────────────────────────────────────────────────────────

class OpenCLBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        OpenCLVectorBackend::setComputeDistancesFn({});
    }
};

// ── OCL-BRIDGE-01 ─────────────────────────────────────────────────────────────
// With no fn injected the stub returns an empty vector.
TEST_F(OpenCLBridgeTest, NoFnReturnsEmpty) {
    OpenCLVectorBackend backend;
    const float q[4] = {1.f, 0.f, 0.f, 0.f};
    const float v[4] = {0.f, 1.f, 0.f, 0.f};
    auto result = backend.computeDistances(q, 1, 4, v, 1, true);
    EXPECT_TRUE(result.empty());
}

// ── OCL-BRIDGE-02 ─────────────────────────────────────────────────────────────
// With fn injected, fn is called and its return value propagated.
TEST_F(OpenCLBridgeTest, InjectedFnIsCalled) {
    bool fn_called = false;
    const std::vector<float> sentinel = {1.0f};

    OpenCLVectorBackend::setComputeDistancesFn(
        [&](const float* queries, size_t numQ, size_t dim,
            const float* vectors, size_t numV, bool useL2) -> std::vector<float> {
            fn_called = true;
            EXPECT_EQ(numQ, 1u);
            EXPECT_EQ(dim, 4u);
            EXPECT_EQ(numV, 1u);
            EXPECT_TRUE(useL2);
            return sentinel;
        });

    OpenCLVectorBackend backend;
    const float q[4] = {1.f, 0.f, 0.f, 0.f};
    const float v[4] = {0.f, 1.f, 0.f, 0.f};
    auto result = backend.computeDistances(q, 1, 4, v, 1, true);
    EXPECT_TRUE(fn_called);
    EXPECT_EQ(result, sentinel);
}

// ── OCL-BRIDGE-03 ─────────────────────────────────────────────────────────────
// When fn throws, computeDistances returns {} (fail-closed).
TEST_F(OpenCLBridgeTest, ThrowingFnIsFailClosed) {
    OpenCLVectorBackend::setComputeDistancesFn(
        [](const float*, size_t, size_t, const float*, size_t, bool) -> std::vector<float> {
            throw std::runtime_error("simulated opencl error");
        });

    OpenCLVectorBackend backend;
    const float q[4] = {0.f, 0.f, 0.f, 0.f};
    const float v[4] = {0.f, 0.f, 0.f, 0.f};
    EXPECT_NO_THROW({
        auto result = backend.computeDistances(q, 1, 4, v, 1, true);
        EXPECT_TRUE(result.empty());
    });
}

// ============================================================================
// OneAPI stub bridge tests (STUB #70)
// These run only in non-ONEAPI builds.
// ============================================================================
#ifndef THEMIS_ENABLE_ONEAPI
using namespace themis::acceleration;

using OneAPIComputeDistancesFn = std::function<std::vector<float>(
    const float*, size_t, size_t, const float*, size_t, bool)>;

class OneAPIBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        setOneAPIComputeDistancesFn(OneAPIComputeDistancesFn{});
    }
};

// ONEAPI-BRIDGE-01: factory backend returns empty without injected fn
TEST_F(OneAPIBridgeTest, NoFnReturnsEmpty) {
    auto backend = createOneAPIBackend();
    const float q[2] = {1.f, 0.f};
    const float v[2] = {0.f, 1.f};
    auto result = backend->computeDistances(q, 1, 2, v, 1, true);
    EXPECT_TRUE(result.empty());
}

// ONEAPI-BRIDGE-02: injected fn is called
TEST_F(OneAPIBridgeTest, InjectedFnIsCalled) {
    bool fn_called = false;
    const std::vector<float> sentinel = {3.14f};
    setOneAPIComputeDistancesFn(
        [&](const float*, size_t, size_t, const float*, size_t, bool) -> std::vector<float> {
            fn_called = true;
            return sentinel;
        });
    auto backend = createOneAPIBackend();
    const float q[2] = {1.f, 0.f};
    const float v[2] = {0.f, 1.f};
    auto result = backend->computeDistances(q, 1, 2, v, 1, true);
    EXPECT_TRUE(fn_called);
    EXPECT_EQ(result, sentinel);
}

// ONEAPI-BRIDGE-03: throwing fn → fail-closed (returns {})
TEST_F(OneAPIBridgeTest, ThrowingFnIsFailClosed) {
    setOneAPIComputeDistancesFn(
        [](const float*, size_t, size_t, const float*, size_t, bool) -> std::vector<float> {
            throw std::runtime_error("simulated oneapi error");
        });
    auto backend = createOneAPIBackend();
    const float q[2] = {};
    const float v[2] = {};
    EXPECT_NO_THROW({
        auto result = backend->computeDistances(q, 1, 2, v, 1, true);
        EXPECT_TRUE(result.empty());
    });
}

#endif // !THEMIS_ENABLE_ONEAPI

// ============================================================================
// ZLUDA bridge tests (STUB #168)
// These run only when ZLUDA backend is compiled in.
// ============================================================================
#ifdef THEMIS_ENABLE_ZLUDA
using ZludaComputeDistancesFn = std::function<std::vector<float>(
    const float*, size_t, size_t, const float*, size_t, bool)>;
using ZludaBatchKnnSearchFn = std::function<std::vector<std::vector<std::pair<uint32_t, float>>>(
    const float*, size_t, size_t, const float*, size_t, size_t, bool)>;

class ZLUDAServiceBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        themis::acceleration::setZLUDAComputeDistancesFn(ZludaComputeDistancesFn{});
        themis::acceleration::setZLUDABatchKnnSearchFn(ZludaBatchKnnSearchFn{});
    }
};

TEST_F(ZLUDAServiceBridgeTest, InjectedComputeDistancesFnIsCalled) {
    bool fn_called = false;
    themis::acceleration::setZLUDAComputeDistancesFn(
        [&]([[maybe_unused]] const float* queries, [[maybe_unused]] size_t numQueries,
            [[maybe_unused]] size_t dim, [[maybe_unused]] const float* vectors,
            [[maybe_unused]] size_t numVectors, [[maybe_unused]] bool useL2) -> std::vector<float> {
            fn_called = true;
            return {2.5f};
        });

    auto backend = themis::acceleration::createZLUDABackend();
    ASSERT_TRUE(backend != nullptr);

    const float q[2] = {1.f, 0.f};
    const float v[2] = {0.f, 1.f};
    auto result = backend->computeDistances(q, 1, 2, v, 1, true);
    EXPECT_TRUE(fn_called);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_FLOAT_EQ(result[0], 2.5f);
}

TEST_F(ZLUDAServiceBridgeTest, InjectedBatchKnnSearchFnIsCalled) {
    bool fn_called = false;
    themis::acceleration::setZLUDABatchKnnSearchFn(
        [&]([[maybe_unused]] const float* queries, [[maybe_unused]] size_t numQueries,
            [[maybe_unused]] size_t dim, [[maybe_unused]] const float* vectors,
            [[maybe_unused]] size_t numVectors, [[maybe_unused]] size_t k, [[maybe_unused]] bool useL2)
            -> std::vector<std::vector<std::pair<uint32_t, float>>> {
            fn_called = true;
            return {{{4u, 0.4f}, {7u, 0.7f}}};
        });

    auto backend = themis::acceleration::createZLUDABackend();
    ASSERT_TRUE(backend != nullptr);

    const float q[2] = {1.f, 0.f};
    const float v[4] = {0.f, 1.f, 1.f, 0.f};
    auto result = backend->batchKnnSearch(q, 1, 2, v, 2, 2, true);
    EXPECT_TRUE(fn_called);
    ASSERT_EQ(result.size(), 1u);
    ASSERT_EQ(result[0].size(), 2u);
    EXPECT_EQ(result[0][0].first, 4u);
    EXPECT_FLOAT_EQ(result[0][0].second, 0.4f);
}

TEST_F(ZLUDAServiceBridgeTest, ThrowingBridgeFnsFailClosed) {
    themis::acceleration::setZLUDAComputeDistancesFn(
        []([[maybe_unused]] const float* queries, [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim, [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors, [[maybe_unused]] bool useL2) -> std::vector<float> {
            throw std::runtime_error("simulated zluda compute failure");
        });
    themis::acceleration::setZLUDABatchKnnSearchFn(
        []([[maybe_unused]] const float* queries, [[maybe_unused]] size_t numQueries,
           [[maybe_unused]] size_t dim, [[maybe_unused]] const float* vectors,
           [[maybe_unused]] size_t numVectors, [[maybe_unused]] size_t k, [[maybe_unused]] bool useL2)
            -> std::vector<std::vector<std::pair<uint32_t, float>>> {
            throw std::runtime_error("simulated zluda knn failure");
        });

    auto backend = themis::acceleration::createZLUDABackend();
    ASSERT_TRUE(backend != nullptr);

    const float q[2] = {};
    const float v[2] = {};
    EXPECT_TRUE(backend->computeDistances(q, 1, 2, v, 1, true).empty());
    EXPECT_TRUE(backend->batchKnnSearch(q, 1, 2, v, 1, 1, true).empty());
}
#endif // THEMIS_ENABLE_ZLUDA
