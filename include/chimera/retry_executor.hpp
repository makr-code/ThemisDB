/**
 * @file retry_executor.hpp
 * @brief Retry executor with configurable back-off for adapter operations.
 *
 * Provides retry semantics with bounded exponential back-off, jitter,
 * idempotency gating, and error classification.
 */

#pragma once

#include "chimera/database_adapter.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <set>
#include <thread>

namespace chimera {

/**
 * @brief Controls bounded retries for a single adapter operation.
 * @note Retries are disabled unless operation_is_idempotent is true. When
 *       retry_on_codes is non-empty it is the complete error-code allowlist;
 *       otherwise the transient-error category flags are used. All category
 *       flags default to false and must be explicitly enabled.
 * @note The executor validates the backoff fields and bounds retry count and
 *       delay, but cannot apply a deadline to the operation callable.
 */
struct RetryPolicy {
    uint32_t max_retries = 3;
    uint32_t initial_backoff_ms = 100;
    uint32_t max_backoff_ms = 30000;
    double backoff_multiplier = 1.5;
    double jitter_factor = 0.1;
    std::set<ErrorCode> retry_on_codes;
    bool retry_on_connection_error = false;
    bool retry_on_resource_exhausted = false;
    bool retry_on_internal_error = false;
    /** The caller asserts that repeating the operation cannot duplicate effects. */
    bool operation_is_idempotent = false;
};

class RetryExecutor {
public:
    RetryExecutor() = default;
    ~RetryExecutor() = default;

    template<typename Func>
    /**
     * @brief Executes an operation with bounded retries for explicitly safe failures.
     * @param[in] fn Operation returning a CHIMERA Result; it must be idempotent when
     *            retries are enabled.
     * @param[in] policy Retry count, backoff, error allowlist, and idempotency policy.
     * @return The first successful result, the first non-retryable error, or the
     *         final error after at most max_retries retries.
     * @note Invalid backoff settings return INVALID_ARGUMENT without invoking fn.
     * @note This helper bounds retry attempts and backoff delay, but cannot impose
     *       a deadline on fn itself. Callers remain responsible for operation timeouts.
     */
    auto execute_with_retry(Func&& fn, const RetryPolicy& policy)
        -> typename std::invoke_result<Func>::type
    {
        using ResultType = typename std::invoke_result<Func>::type;

        if (!is_valid_policy(policy)) {
            return ResultType::err(ErrorCode::INVALID_ARGUMENT, "Invalid retry policy");
        }

        uint32_t attempt = 0;
        while (true) {
            ResultType result = fn();
            if (result.is_ok()) {
                return result;
            }
            if (attempt >= policy.max_retries || !should_retry(result, policy)) {
                return result;
            }
            auto backoff_duration = calculate_backoff(attempt, policy);
            std::this_thread::sleep_for(backoff_duration);
            ++attempt;
        }
    }

    /**
     * @brief Calculate backoff.
     * @param[in] attempt Input parameter.
     * @param[in] policy Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static std::chrono::milliseconds calculate_backoff(
        uint32_t attempt,
        const RetryPolicy& policy
    ) noexcept;

    /**
     * @brief Determines whether a failed operation may be retried.
     * @param[in] result Failed result whose concrete error code is classified.
     * @param[in] policy Retry policy; retries are denied unless the operation is
     *            explicitly declared idempotent.
     * @return True only for an idempotent operation and an error selected by the
     *         explicit code allowlist or enabled transient-error category.
     * @note Exception safety: noexcept.
     */
    static bool should_retry(
        const Result<bool>& result,
        const RetryPolicy& policy
    ) noexcept;

    /**
     * @brief Determines whether a failed result of any value type may be retried.
     * @tparam T Success value stored in the CHIMERA result.
     * @param[in] result Failed result whose concrete error code is classified.
     * @param[in] policy Retry policy; retries are denied unless the operation is
     *            explicitly declared idempotent.
     * @return True only when the error is allowed by the policy.
     */
    template<typename T>
    static bool should_retry(
        const Result<T>& result,
        const RetryPolicy& policy
    ) noexcept {
        if (result.is_ok() || !policy.operation_is_idempotent) {
            return false;
        }
        return should_retry_error(result.error_code, policy);
    }

private:
    static bool is_valid_policy(const RetryPolicy& policy) noexcept;
    static bool should_retry_error(
        ErrorCode error_code,
        const RetryPolicy& policy
    ) noexcept;

    /**
     * @brief Get jitter.
     * @param[in] factor Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static double get_jitter(double factor) noexcept;
};

} // namespace chimera
