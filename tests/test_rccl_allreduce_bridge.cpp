#include <gtest/gtest.h>

#include "acceleration/rccl_vector_backend.h"

using namespace themis::acceleration;

TEST(RCCLAllReduceBridgeTest, InjectedAllReduceFunctionIsUsed)
{
    RCCLVectorBackend::setAllReduceFn(nullptr);

    RCCLVectorBackend backend;
    float send[3] = {1.0f, 2.0f, 3.0f};
    float recv[3] = {0.0f, 0.0f, 0.0f};

#ifdef THEMIS_ENABLE_RCCL
    GTEST_SKIP() << "RCCL enabled build uses native allReduce path";
#else
    EXPECT_FALSE(backend.allReduce(send, recv, 3, RCCLVectorBackend::ReductionOp::SUM, nullptr));

    bool called = false;
    RCCLVectorBackend::setAllReduceFn(
        [&](const float* sendBuf,
            float* recvBuf,
            size_t count,
            RCCLVectorBackend::ReductionOp,
            hipStream_t) {
            called = true;
            for (size_t i = 0; i < count; ++i) {
                recvBuf[i] = sendBuf[i];
            }
            return true;
        });

    EXPECT_TRUE(backend.allReduce(send, recv, 3, RCCLVectorBackend::ReductionOp::SUM, nullptr));
    EXPECT_TRUE(called);
    EXPECT_FLOAT_EQ(recv[0], 1.0f);
    EXPECT_FLOAT_EQ(recv[1], 2.0f);
    EXPECT_FLOAT_EQ(recv[2], 3.0f);
#endif

    RCCLVectorBackend::setAllReduceFn(nullptr);
}
