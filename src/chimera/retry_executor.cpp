/**
 * @file retry_executor.cpp
 * @brief Retry executor implementation.
 *
 * Implements RetryExecutor with concrete error classification and
 * configurable bounded back-off.
 */

#include "chimera/retry_executor.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <thread>

namespace chimera {

std::chrono::milliseconds RetryExecutor::calculate_backoff(
    uint32_t attempt,
    const RetryPolicy& policy
) noexcept {
    if (!is_valid_policy(policy)) {
        return std::chrono::milliseconds(0);
    }

    if (attempt == 0) {
        return std::chrono::milliseconds(0);
    }

    // base_backoff = initial * (multiplier ^ attempt)
    double base_backoff = static_cast<double>(policy.initial_backoff_ms) *
                          std::pow(policy.backoff_multiplier, static_cast<double>(attempt));

    // Apply jitter
    double jittered = base_backoff * (1.0 + get_jitter(policy.jitter_factor));

    // Keep the configured bound effective after jitter is applied.
    jittered = std::clamp(jittered, 0.0, static_cast<double>(policy.max_backoff_ms));

    return std::chrono::milliseconds(static_cast<int64_t>(jittered));
}

bool RetryExecutor::should_retry(
    const Result<bool>& result,
    const RetryPolicy& policy
) noexcept {
    if (result.is_ok() || !policy.operation_is_idempotent) {
        return false;
    }

    return should_retry_error(result.error_code, policy);
}

bool RetryExecutor::is_valid_policy(const RetryPolicy& policy) noexcept {
    return policy.initial_backoff_ms <= policy.max_backoff_ms &&
           std::isfinite(policy.backoff_multiplier) &&
           policy.backoff_multiplier >= 1.0 &&
           std::isfinite(policy.jitter_factor) &&
           policy.jitter_factor >= 0.0 &&
           policy.jitter_factor <= 1.0 &&
           !policy.retry_on_codes.contains(ErrorCode::SUCCESS);
}

bool RetryExecutor::should_retry_error(
    ErrorCode error_code,
    const RetryPolicy& policy
) noexcept {
    if (!is_valid_policy(policy)) {
        return false;
    }

    if (!policy.retry_on_codes.empty()) {
        return policy.retry_on_codes.contains(error_code);
    }

    switch (error_code) {
        case ErrorCode::CONNECTION_ERROR:
            return policy.retry_on_connection_error;
        case ErrorCode::RESOURCE_EXHAUSTED:
            return policy.retry_on_resource_exhausted;
        case ErrorCode::INTERNAL_ERROR:
            return policy.retry_on_internal_error;
        default:
            return false;
    }
}

double RetryExecutor::get_jitter(double factor) noexcept {
    // Thread-safe random number generation.
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-factor, factor);
    return dis(gen);
}

} // namespace chimera
