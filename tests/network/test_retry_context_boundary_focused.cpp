#include <gtest/gtest.h>

#include "network/wire_retry_policy.h"

#include <chrono>

namespace {

using namespace themis::network;

TEST(WireRetryBoundaryTest, PermanentFailureDoesNotConsumeRemainingRetryBudget) {
    auto policy = WireRetryPolicy::forTesting();
    RetryContext context(policy);
    ASSERT_TRUE(context.nextDelay().has_value());
    EXPECT_FALSE(context.nextDelay(WireErrorClass::kPermanent).has_value());
    EXPECT_EQ(context.attempts(), 1u);
    EXPECT_TRUE(context.canRetry());
    EXPECT_TRUE(context.nextDelay().has_value());
}

TEST(WireRetryBoundaryTest, ZeroBudgetRemainsExhaustedAfterReset) {
    auto policy = WireRetryPolicy::forTesting();
    policy.max_attempts = 0;
    RetryContext context(policy);
    EXPECT_FALSE(context.canRetry());
    EXPECT_FALSE(context.nextDelay().has_value());
    EXPECT_EQ(context.attempts(), 0u);
    context.reset();
    EXPECT_FALSE(context.canRetry());
    EXPECT_FALSE(context.nextDelay(WireErrorClass::kUnknown).has_value());
}

TEST(WireRetryBoundaryTest, ClampsSubunitMultiplierAndOwnsPolicySnapshot) {
    auto policy = WireRetryPolicy::forBindListen();
    policy.base_delay_ms = 10;
    policy.multiplier = 0.5;
    RetryContext context(policy);
    policy.base_delay_ms = 1000;
    policy.max_attempts = 0;
    for (int i = 0; i < 3; ++i) {
        const auto delay = context.nextDelay();
        ASSERT_TRUE(delay.has_value());
        EXPECT_EQ(*delay, std::chrono::milliseconds(10));
    }
    EXPECT_FALSE(context.nextDelay().has_value());
}

} // namespace
