#include <gtest/gtest.h>

#include "utils/retry_policy.h"

#include <memory>
#include <optional>
#include <stdexcept>

namespace {

themis::utils::RetryConfig immediateRetries(unsigned attempts) {
    themis::utils::RetryConfig config;
    config.max_attempts = attempts;
    config.initial_backoff_ms = 0;
    config.max_backoff_ms = 0;
    config.jitter_fraction = 0.0;
    return config;
}

TEST(UtilsRetryBoundaryTest, ZeroAttemptBudgetNeverInvokesOperation) {
    int calls = 0;
    const auto result = themis::utils::retry_with_backoff(
        [&]() -> std::optional<int> { ++calls; return 42; }, immediateRetries(0));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(calls, 0);
}

TEST(UtilsRetryBoundaryTest, SuccessOnLastAttemptRetainsFalsePayload) {
    int calls = 0;
    const auto result = themis::utils::retry_with_backoff(
        [&]() -> std::optional<bool> {
            return ++calls == 3 ? std::optional<bool>(false) : std::nullopt;
        }, immediateRetries(3));
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(*result);
    EXPECT_EQ(calls, 3);
}

TEST(UtilsRetryBoundaryTest, ReturnsMoveOnlyResultAfterTransientFailure) {
    int calls = 0;
    auto result = themis::utils::retry_with_backoff(
        [&]() -> std::optional<std::unique_ptr<int>> {
            if (++calls == 1) {
                return std::nullopt;
            }
            return std::make_unique<int>(7);
        }, immediateRetries(3));
    ASSERT_TRUE(result.has_value());
    ASSERT_NE(*result, nullptr);
    EXPECT_EQ(**result, 7);
    EXPECT_EQ(calls, 2);
}

TEST(UtilsRetryBoundaryTest, ExceptionAfterTransientFailureStopsFurtherAttempts) {
    int calls = 0;
    EXPECT_THROW(themis::utils::retry_with_backoff(
        [&]() -> std::optional<int> {
            if (++calls == 1) {
                return std::nullopt;
            }
            throw std::logic_error("non-retryable");
        }, immediateRetries(4)), std::logic_error);
    EXPECT_EQ(calls, 2);
}

} // namespace
