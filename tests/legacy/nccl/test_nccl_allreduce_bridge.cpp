/**
 * @file test_nccl_allreduce_bridge.cpp
 * @brief Unit tests for NCCLVectorBackend injectable allReduce bridge (STUB #67).
 *
 * Tests verify the allReduce injectable callback slot:
 *   NCCL-BRIDGE-01  no fn injected → allReduce returns false (stub default)
 *   NCCL-BRIDGE-02  fn injected → fn is called, its return value is propagated
 *   NCCL-BRIDGE-03  fn throws → allReduce returns false (fail-closed)
 */

#include <gtest/gtest.h>
#include "acceleration/nccl_vector_backend.h"

using themis::acceleration::NCCLVectorBackend;
using ReductionOp = NCCLVectorBackend::ReductionOp;

// ── Fixture ───────────────────────────────────────────────────────────────────

class NCCLBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        NCCLVectorBackend::setAllReduceFn({});
    }
};

// ── NCCL-BRIDGE-01 ────────────────────────────────────────────────────────────
// With no fn injected the stub returns false.
TEST_F(NCCLBridgeTest, NoFnReturnsFalse) {
    NCCLVectorBackend backend;
    const float send[4] = {1.f, 2.f, 3.f, 4.f};
    float recv[4]       = {};
    EXPECT_FALSE(backend.allReduce(send, recv, 4, ReductionOp::SUM, nullptr));
}

// ── NCCL-BRIDGE-02 ────────────────────────────────────────────────────────────
// With fn injected, fn is called and its return value propagated.
TEST_F(NCCLBridgeTest, InjectedFnIsCalled) {
    bool fn_called = false;
    NCCLVectorBackend::setAllReduceFn(
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

    NCCLVectorBackend backend;
    const float send[4] = {1.f, 2.f, 3.f, 4.f};
    float recv[4]       = {};
    EXPECT_TRUE(backend.allReduce(send, recv, 4, ReductionOp::SUM, nullptr));
    EXPECT_TRUE(fn_called);
    EXPECT_FLOAT_EQ(recv[0], 2.f);
    EXPECT_FLOAT_EQ(recv[3], 8.f);
}

// ── NCCL-BRIDGE-03 ────────────────────────────────────────────────────────────
// When fn throws, allReduce returns false (fail-closed).
TEST_F(NCCLBridgeTest, ThrowingFnIsFailClosed) {
    NCCLVectorBackend::setAllReduceFn(
        [](const float*, float*, size_t, ReductionOp, void*) -> bool {
            throw std::runtime_error("simulated nccl error");
        });

    NCCLVectorBackend backend;
    const float send[4] = {1.f, 2.f, 3.f, 4.f};
    float recv[4]       = {};
    EXPECT_NO_THROW({
        EXPECT_FALSE(backend.allReduce(send, recv, 4, ReductionOp::SUM, nullptr));
    });
}
