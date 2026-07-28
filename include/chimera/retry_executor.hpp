/*
 * ThemisDB | File: retry_executor.hpp | Version: 0.1.0 | Last Modified: 2026-06-10
 * Author: Copilot | Maturity: 🟡 BETA
 * 
 * Exponential backoff retry strategy for transient failures.
 * Copyright MIT License.
 */

/**
 * @file retry_executor.hpp
 * @brief Retry executor with configurable back-off for adapter operations.
 *
 * Decorates an IDatabaseAdapter with retry semantics: exponential
 * back-off, jitter, per-operation timeout, and error-classification.
 */

#pragma once

#include "chimera/database_adapter.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <set>

namespace chimera {

/**
 * @struct RetryPolicy
 * @brief Configuration for retry behavior with exponential backoff.
 */
struct RetryPolicy {
    uint32_t max_retries = 3;
    uint32_t initial_backoff_ms = 100;
    uint32_t max_backoff_ms = 30000;
    double backoff_multiplier = 1.5;
    double jitter_factor = 0.1;
    std::set<ErrorCode> retry_on_codes;
    bool retry_on_connection_error = true;
    bool retry_on_resource_exhausted = true;
    bool retry_on_internal_error = true;
};

/**
 * @class RetryExecutor
 * @brief Executes operations with exponential backoff retry logic.
 */
class RetryExecutor {
public:
    RetryExecutor() = default;
    ~RetryExecutor() = default;

    template<typename Func>
    auto execute_with_retry(Func&& fn, const RetryPolicy& policy)
        -> typename std::invoke_result<Func>::type
    {
        using ResultType = typename std::invoke_result<Func>::type;

        for (uint32_t attempt = 0; attempt <= policy.max_retries; ++attempt) {
            ResultType result = fn();
            if (result) {
                return result;
            }
            if (attempt >= policy.max_retries) {
                return result;
            }
            if (!should_retry(result, policy)) {
                return result;
            }
            auto backoff_duration = calculate_backoff(attempt, policy);
            std::this_thread::sleep_for(backoff_duration);
        }
        return ResultType::err(ErrorCode::INTERNAL_ERROR, "Retry failed");
    }

    static std::chrono::milliseconds calculate_backoff(
        uint32_t attempt,
        const RetryPolicy& policy
    ) noexcept;

    static bool should_retry(
        const Result<bool>& result,
        const RetryPolicy& policy
    ) noexcept;

private:
    static double get_jitter(double factor) noexcept;
};

} // namespace chimera

#endif // CHIMERA_RETRY_EXECUTOR_HPP
