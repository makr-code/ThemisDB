#include <gtest/gtest.h>

#include "chimera/retry_executor.hpp"

#include <limits>
#include <set>

using namespace chimera;

namespace {

RetryPolicy no_delay_policy() {
    RetryPolicy policy;
    policy.initial_backoff_ms = 0;
    policy.max_backoff_ms = 0;
    policy.jitter_factor = 0.0;
    return policy;
}

} // namespace

TEST(ChimeraRetryExecutorTest, RequiresIdempotencyOptIn) {
    auto policy = no_delay_policy();
    policy.retry_on_connection_error = true;
    const auto failure = Result<bool>::err(ErrorCode::CONNECTION_ERROR, "offline");

    EXPECT_FALSE(RetryExecutor::should_retry(failure, policy));

    policy.operation_is_idempotent = true;
    EXPECT_TRUE(RetryExecutor::should_retry(failure, policy));
}

TEST(ChimeraRetryExecutorTest, ClassifiesConcreteErrorCodes) {
    auto policy = no_delay_policy();
    policy.operation_is_idempotent = true;

    EXPECT_FALSE(RetryExecutor::should_retry(
        Result<bool>::err(ErrorCode::INVALID_ARGUMENT, "bad input"), policy));
    EXPECT_FALSE(RetryExecutor::should_retry(
        Result<bool>::err(ErrorCode::TIMEOUT, "timed out"), policy));
    EXPECT_FALSE(RetryExecutor::should_retry(
        Result<bool>::err(ErrorCode::CONNECTION_ERROR, "offline"), policy));

    policy.retry_on_connection_error = true;
    EXPECT_TRUE(RetryExecutor::should_retry(
        Result<bool>::err(ErrorCode::CONNECTION_ERROR, "offline"), policy));
    policy.retry_on_codes = {ErrorCode::TIMEOUT};
    EXPECT_TRUE(RetryExecutor::should_retry(
        Result<bool>::err(ErrorCode::TIMEOUT, "timed out"), policy));
    EXPECT_FALSE(RetryExecutor::should_retry(
        Result<bool>::err(ErrorCode::CONNECTION_ERROR, "offline"), policy));
}

TEST(ChimeraRetryExecutorTest, StopsAfterConfiguredRetryCount) {
    auto policy = no_delay_policy();
    policy.max_retries = 2;
    policy.operation_is_idempotent = true;
    policy.retry_on_connection_error = true;
    size_t calls = 0;

    const auto result = RetryExecutor{}.execute_with_retry([&]() {
        ++calls;
        return Result<size_t>::err(ErrorCode::CONNECTION_ERROR, "offline");
    }, policy);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::CONNECTION_ERROR);
    EXPECT_EQ(calls, 3u);
}

TEST(ChimeraRetryExecutorTest, DoesNotRetryNonIdempotentOperation) {
    auto policy = no_delay_policy();
    policy.max_retries = 3;
    size_t calls = 0;

    const auto result = RetryExecutor{}.execute_with_retry([&]() {
        ++calls;
        return Result<bool>::err(ErrorCode::CONNECTION_ERROR, "offline");
    }, policy);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(calls, 1u);
}

TEST(ChimeraRetryExecutorTest, RejectsInvalidBackoffPolicyBeforeCallingOperation) {
    auto policy = no_delay_policy();
    policy.backoff_multiplier = std::numeric_limits<double>::quiet_NaN();
    size_t calls = 0;

    const auto result = RetryExecutor{}.execute_with_retry([&]() {
        ++calls;
        return Result<bool>::ok(true);
    }, policy);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(calls, 0u);
}

TEST(ChimeraRetryExecutorTest, ExplicitErrorAllowlistCannotRetrySuccess) {
    auto policy = no_delay_policy();
    policy.operation_is_idempotent = true;
    policy.retry_on_codes = {ErrorCode::SUCCESS};

    EXPECT_FALSE(RetryExecutor::should_retry(
        Result<bool>::err(ErrorCode::CONNECTION_ERROR, "offline"), policy));
    EXPECT_EQ(RetryExecutor::calculate_backoff(1, policy).count(), 0);
}

TEST(ChimeraRetryExecutorTest, InvalidPolicyCannotAuthorizeRetries) {
    auto policy = no_delay_policy();
    policy.operation_is_idempotent = true;
    policy.jitter_factor = 1.1;

    EXPECT_FALSE(RetryExecutor::should_retry(
        Result<bool>::err(ErrorCode::CONNECTION_ERROR, "offline"), policy));
}

TEST(ChimeraRetryExecutorTest, JitterCannotExceedMaximumBackoff) {
    auto policy = no_delay_policy();
    policy.initial_backoff_ms = 80;
    policy.max_backoff_ms = 100;
    policy.backoff_multiplier = 2.0;
    policy.jitter_factor = 1.0;

    for (size_t sample = 0; sample < 32; ++sample) {
        EXPECT_LE(RetryExecutor::calculate_backoff(1, policy).count(), 100);
    }
}
