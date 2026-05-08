/**
 * @file test_opencl_bridge.cpp
 * @brief Unit tests for OpenCLVectorBackend injectable computeDistances bridge (STUB #69).
 *
 * Tests verify the computeDistances injectable callback slot:
 *   OCL-BRIDGE-01  no fn injected → computeDistances returns {} (stub default)
 *   OCL-BRIDGE-02  fn injected → fn is called, its return value is propagated
 *   OCL-BRIDGE-03  fn throws → computeDistances returns {} (fail-closed)
 */

#include <gtest/gtest.h>
#include "acceleration/opencl_backend.h"

using themis::acceleration::OpenCLVectorBackend;

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
