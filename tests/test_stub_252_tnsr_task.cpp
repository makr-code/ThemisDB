/*
 * @file tests/test_stub_252_tnsr_task.cpp
 * @brief GTest coverage for the TNSRTask::RerouteSerializeFn injection bridge.
 *
 * TNSR-252-01: No bridge set → hasRerouteSerializeFn() returns false.
 * TNSR-252-02: Bridge set   → hasRerouteSerializeFn() returns true; fn is callable.
 * TNSR-252-03: Bridge clear → hasRerouteSerializeFn() returns false after clear.
 */

#include <gtest/gtest.h>
#include "tensor/tnsr_task.h"

namespace themis {
namespace {

class TNSRTaskBridgeTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Always clean up the global bridge so tests are independent.
        TNSRTask::clearRerouteSerializeFn();
    }
};

/// TNSR-252-01: hasRerouteSerializeFn() returns false when no bridge is set.
TEST_F(TNSRTaskBridgeTest, NoSetReturnsFalse) {
    EXPECT_FALSE(TNSRTask::hasRerouteSerializeFn());
}

/// TNSR-252-02: After setRerouteSerializeFn(), hasRerouteSerializeFn() returns
/// true and the injected function is reachable via getRerouteSerializeFn().
TEST_F(TNSRTaskBridgeTest, SetReturnsTrueAndIsCallable) {
    bool called = false;
    TNSRTask::setRerouteSerializeFn(
        [&called](storage::TensorNetworkStorageEngine& /*engine*/,
                  const storage::TensorFieldKey&       /*key*/,
                  const TensorNetworkGraph&             /*tng*/,
                  const storage::TTTrain&               /*train*/) -> bool {
            called = true;
            return true;
        });

    EXPECT_TRUE(TNSRTask::hasRerouteSerializeFn());

    auto fn = TNSRTask::getRerouteSerializeFn();
    ASSERT_TRUE(static_cast<bool>(fn));

    // Invoke the bridge through the retrieved fn.
    // We only need to verify the callable path works; construct minimal args.
    storage::TensorNetworkStorageEngine engine;
    storage::TensorFieldKey key;
    TensorNetworkGraph tng;
    storage::TTTrain train;
    bool result = fn(engine, key, tng, train);
    EXPECT_TRUE(called);
    EXPECT_TRUE(result);
}

/// TNSR-252-03: clearRerouteSerializeFn() reverts the bridge;
/// hasRerouteSerializeFn() returns false afterwards.
TEST_F(TNSRTaskBridgeTest, ClearRevertsToAdvisory) {
    TNSRTask::setRerouteSerializeFn(
        [](storage::TensorNetworkStorageEngine& /*engine*/,
           const storage::TensorFieldKey&       /*key*/,
           const TensorNetworkGraph&             /*tng*/,
           const storage::TTTrain&               /*train*/) -> bool {
            return true;
        });
    ASSERT_TRUE(TNSRTask::hasRerouteSerializeFn());

    TNSRTask::clearRerouteSerializeFn();
    EXPECT_FALSE(TNSRTask::hasRerouteSerializeFn());
}

} // namespace
} // namespace themis
