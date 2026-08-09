#pragma once

#include <cstdint>

namespace themis::utils {

/**
 * @brief Canonical retry exhaustion reason shared across module contracts.
 */
enum class RetryExhaustionReason : std::uint8_t {
    NONE = 0,
    MAX_ATTEMPTS_REACHED = 1,
    TIME_BUDGET_EXCEEDED = 2,
    CANCELLED = 3,
    NON_RETRYABLE = 4,
    MEMORY_BUDGET_EXCEEDED = 5,
};

/**
 * @brief Canonical timeout source shared across module contracts.
 */
enum class RetryTimeoutSource : std::uint8_t {
    NONE = 0,
    PER_ATTEMPT = 1,
    OVERALL = 2,
    SHARD = 3,
    QUORUM = 4,
    BACKEND = 5,
};

/**
 * @brief Canonical retry metadata envelope for diagnostics and automation.
 *
 * @note `retry_count` is the number of retries already consumed (attempts - 1).
 */
struct RetryMetadata {
    std::uint32_t retry_count = 0;
    std::uint32_t retry_budget = 0;
    RetryExhaustionReason exhaustion_reason = RetryExhaustionReason::NONE;
    RetryTimeoutSource timeout_source = RetryTimeoutSource::NONE;
    bool retriable = true;
};

[[nodiscard]] inline constexpr bool isRetryExhausted(
    RetryExhaustionReason reason) noexcept {
    return reason != RetryExhaustionReason::NONE;
}

[[nodiscard]] inline constexpr bool isTimeoutDriven(
    RetryExhaustionReason reason) noexcept {
    return reason == RetryExhaustionReason::TIME_BUDGET_EXCEEDED;
}

} // namespace themis::utils
