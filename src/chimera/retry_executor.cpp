/**
 * @file retry_executor.cpp
 * @brief Retry executor implementation.
 *
 * Implements RetryExecutor wrapping any IDatabaseAdapter with
 * configurable retry policies and circuit-breaker integration.
 */

#include "chimera/retry_executor.hpp"

#include <cmath>
#include <random>
#include <thread>

namespace chimera {

std::chrono::milliseconds RetryExecutor::calculate_backoff(
    uint32_t attempt,
    const RetryPolicy& policy
) noexcept {
    if (attempt == 0) {
        return std::chrono::milliseconds(0);
    }

    // base_backoff = initial * (multiplier ^ attempt)
    double base_backoff = static_cast<double>(policy.initial_backoff_ms) *
                          std::pow(policy.backoff_multiplier, static_cast<double>(attempt));

    // Cap at max_backoff_ms
    base_backoff = std::min(base_backoff, static_cast<double>(policy.max_backoff_ms));

    // Apply jitter
    double jittered = base_backoff * (1.0 + get_jitter(policy.jitter_factor));

    // Ensure non-negative (in case jitter goes too negative).
    jittered = std::max(0.0, jittered);

    return std::chrono::milliseconds(static_cast<int64_t>(jittered));
}

bool RetryExecutor::should_retry(
    const Result<bool>& result,
    const RetryPolicy& policy
) noexcept {
    if (result) {
        // Success; no retry needed.
        return false;
    }

    // Extract the error code from the Result.
    // Note: Result needs an error_code() method or similar accessor.
    // For now, we assume Result can be queried for its error code.
    // This assumes Result<T> has a method like: ErrorCode error_code() const;
    
    // If specific retry_on_codes is set, use that.
    if (!policy.retry_on_codes.empty()) {
        // Extract error code from result (implementation dependent).
        // For this simplified implementation, we check general categories.
        // A real implementation would query result.error_code().
        return true;  // Default to retry if not empty (specific logic deferred).
    }

    // Check general retry categories based on policy flags.
    // These checks rely on querying the result's error code.
    // For now, we return a conservative true (actual check deferred to impl).
    return policy.retry_on_connection_error ||
           policy.retry_on_resource_exhausted ||
           policy.retry_on_internal_error;
}

double RetryExecutor::get_jitter(double factor) noexcept {
    // Thread-safe random number generation.
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-factor, factor);
    return dis(gen);
}

} // namespace chimera
