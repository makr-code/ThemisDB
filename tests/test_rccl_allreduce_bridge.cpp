/**
 * @file test_rccl_allreduce_bridge.cpp
 * @brief Unit tests for RCCLVectorBackend injectable allReduce bridge (STUB #68).
 *
 * Tests verify the allReduce injectable callback slot:
 *   RCCL-AR-01  no fn injected → allReduce returns false (stub default)
 *   RCCL-AR-02  fn injected → fn is called, its return value is propagated
 *   RCCL-AR-03  fn throws → allReduce returns false (fail-closed)
 */

#include <gtest/gtest.h>
#include "acceleration/rccl_vector_backend.h"

#include <stdexcept>

using themis::acceleration::RCCLVectorBackend;
using ReductionOp = RCCLVectorBackend::ReductionOp;

#ifndef THEMIS_ENABLE_RCCL

// ── Fixture ───────────────────────────────────────────────────────────────────

class RCCLBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        RCCLVectorBackend::setAllReduceFn({});
    }
};

// ── RCCL-AR-01 ────────────────────────────────────────────────────────────────
// With no fn injected the stub returns false.
TEST_F(RCCLBridgeTest, NoFnReturnsFalse) {
    RCCLVectorBackend backend;
    const float send[4] = {1.f, 2.f, 3.f, 4.f};
    float recv[4]       = {};
    EXPECT_FALSE(backend.allReduce(send, recv, 4, ReductionOp::SUM, nullptr));
}

// ── RCCL-AR-02 ────────────────────────────────────────────────────────────────
// With fn injected, fn is called and its return value propagated.
TEST_F(RCCLBridgeTest, InjectedFnIsCalled) {
    bool fn_called = false;
    RCCLVectorBackend::setAllReduceFn(
        [&](const float* send, float* recv, size_t count,
            ReductionOp op, void* stream) -> bool {
            fn_called = true;
            EXPECT_EQ(count, 4u);
            EXPECT_EQ(op, ReductionOp::SUM);
            EXPECT_EQ(stream, nullptr);
            for (size_t i = 0; i < count; ++i) {
                recv[i] = send[i] * 2.f;
            }
            return true;
        });

    RCCLVectorBackend backend;
    const float send[4] = {1.f, 2.f, 3.f, 4.f};
    float recv[4]       = {};
    EXPECT_TRUE(backend.allReduce(send, recv, 4, ReductionOp::SUM, nullptr));
    EXPECT_TRUE(fn_called);
    EXPECT_FLOAT_EQ(recv[0], 2.f);
    EXPECT_FLOAT_EQ(recv[3], 8.f);
}

// ── RCCL-AR-03 ────────────────────────────────────────────────────────────────
// When fn throws, allReduce returns false (fail-closed).
TEST_F(RCCLBridgeTest, ThrowingFnIsFailClosed) {
    RCCLVectorBackend::setAllReduceFn(
        [](const float*, float*, size_t, ReductionOp, void*) -> bool {
            throw std::runtime_error("simulated rccl error");
        });

    RCCLVectorBackend backend;
    const float send[4] = {1.f, 2.f, 3.f, 4.f};
    float recv[4]       = {};
    EXPECT_NO_THROW({
        EXPECT_FALSE(backend.allReduce(send, recv, 4, ReductionOp::SUM, nullptr));
    });
}

#else

TEST(RCCLBridgeTest, RcclEnabledBuildSkipsStubBridgeChecks) {
    GTEST_SKIP() << "THEMIS_ENABLE_RCCL is enabled; stub bridge API is unavailable";
}

#endif
