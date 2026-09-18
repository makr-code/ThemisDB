/**
 * @file query_federation_timeout.h
 * @brief Timeout and retry infrastructure for federated query execution
 * @version 0.0.1
 * @note Maturity: 🟡 BETA
 * @note Score: 95/100
 * @note Status: Beta - Ready for Phase 3 integration
 * @date 2026-08-05
 * @copyright Apache-2.0, (c) 2026 ThemisDB Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <random>

#include "utils/retry_contract.h"

namespace themis::query {

class TimeoutPolicy {
public:
    struct TimeoutEvent {
        enum class Type {
            SHARD_TIMEOUT,      // Individual shard exceeded timeout
            OVERALL_TIMEOUT,    // Query exceeded overall timeout
            RETRY_BACKOFF,      // Backing off before retry
            RETRY_EXHAUSTED,    // Max retries exceeded
        };

        Type event_type;
        std::string shard_id;
        std::chrono::milliseconds elapsed;
        int attempt_number = 0;
        std::string details;
        themis::utils::RetryExhaustionReason exhaustion_reason =
            themis::utils::RetryExhaustionReason::NONE;
        themis::utils::RetryTimeoutSource timeout_source =
            themis::utils::RetryTimeoutSource::NONE;
        std::string correlation_id;
    };

    struct RetryStats {
        int successful_attempt = -1;  // 0-indexed, -1 if not yet successful
        int total_attempts = 0;
        std::chrono::milliseconds total_elapsed{0};
        std::vector<std::chrono::milliseconds> attempt_latencies;
        std::vector<std::string> failure_reasons;
    };

    class Builder {
    public:
        /**
         * @brief With Per Shard Timeout.
         * @param[in] timeout Input parameter.
         * @return Return value.
         * @details Implements withPerShardTimeout without additional internal calls.
         */
        Builder& withPerShardTimeout(std::chrono::milliseconds timeout) {
            per_shard_timeout_ = timeout;
            return *this;
        }

        /**
         * @brief With Overall Timeout.
         * @param[in] timeout Input parameter.
         * @return Return value.
         * @details Implements withOverallTimeout without additional internal calls.
         */
        Builder& withOverallTimeout(std::chrono::milliseconds timeout) {
            overall_timeout_ = timeout;
            return *this;
        }

        /**
         * @brief With Max Retries.
         * @param[in] max_retries Input parameter.
         * @return Return value.
         * @details Implements withMaxRetries without additional internal calls.
         */
        Builder& withMaxRetries(int max_retries) {
            max_retries_ = max_retries;
            return *this;
        }

        /**
         * @brief With Initial Backoff Ms.
         * @param[in] backoff_ms Input parameter.
         * @return Return value.
         * @details Implements withInitialBackoffMs without additional internal calls.
         */
        Builder& withInitialBackoffMs(int backoff_ms) {
            initial_backoff_ms_ = backoff_ms;
            return *this;
        }

        /**
         * @brief With Max Backoff Ms.
         * @param[in] max_backoff_ms Input parameter.
         * @return Return value.
         * @details Implements withMaxBackoffMs without additional internal calls.
         */
        Builder& withMaxBackoffMs(int max_backoff_ms) {
            max_backoff_ms_ = max_backoff_ms;
            return *this;
        }

        /**
         * @brief With Backoff Multiplier.
         * @param[in] multiplier Input parameter.
         * @return Return value.
         * @details Implements withBackoffMultiplier without additional internal calls.
         */
        Builder& withBackoffMultiplier(double multiplier) {
            backoff_multiplier_ = multiplier;
            return *this;
        }

        /**
         * @brief With Jitter Fraction.
         * @param[in] jitter Input parameter.
         * @return Return value.
         * @details Implements withJitterFraction without additional internal calls.
         */
        Builder& withJitterFraction(double jitter) {
            jitter_fraction_ = jitter;
            return *this;
        }

        /**
         * @brief Build.
         * @return Return value.
         */
        TimeoutPolicy build() const;

    private:
        std::chrono::milliseconds per_shard_timeout_{5000};
        std::chrono::milliseconds overall_timeout_{30000};
        int max_retries_ = 3;
        int initial_backoff_ms_ = 100;
        int max_backoff_ms_ = 5000;
        double backoff_multiplier_ = 2.0;
        double jitter_fraction_ = 0.1;
    };

    // Deleted copy constructor and assignment operator
    TimeoutPolicy(const TimeoutPolicy&) = delete;
    TimeoutPolicy& operator=(const TimeoutPolicy&) = delete;

    // Default move semantics
    TimeoutPolicy(TimeoutPolicy&&) noexcept = default;
    TimeoutPolicy& operator=(TimeoutPolicy&&) noexcept = default;

    [[nodiscard]] std::chrono::milliseconds getPerShardTimeout() const {
        return per_shard_timeout_;
    }

    [[nodiscard]] std::chrono::milliseconds getOverallTimeout() const {
        return overall_timeout_;
    }

    [[nodiscard]] int getMaxRetries() const {
        return max_retries_;
    }

    [[nodiscard]] std::chrono::milliseconds calculateBackoff(int attempt) const;

    [[nodiscard]] bool shouldRetry(
        std::chrono::milliseconds elapsed,
        int attempt) const;

    [[nodiscard]] bool isOverallTimeoutExceeded(
        std::chrono::milliseconds total_elapsed) const;

    /**
     * @brief Record Timeout Event.
     * @param[in] event Input parameter.
     */
    void recordTimeoutEvent(const TimeoutEvent& event);

    /**
     * @brief Record Retry Stats.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] stats Input parameter.
     */
    void recordRetryStats(const std::string& shard_id, const RetryStats& stats);

    [[nodiscard]] std::optional<RetryStats> getRetryStats(
        const std::string& shard_id) const;

    [[nodiscard]] std::vector<TimeoutEvent> getTimeoutEvents() const;

    /**
     * @brief Clear Statistics.
     */
    void clearStatistics();

    [[nodiscard]] std::string getStatisticsSummary() const;

    // Destructor
    ~TimeoutPolicy() = default;

private:
    friend class Builder;

    TimeoutPolicy(
        std::chrono::milliseconds per_shard_timeout,
        std::chrono::milliseconds overall_timeout,
        int max_retries,
        int initial_backoff_ms,
        int max_backoff_ms,
        double backoff_multiplier,
        double jitter_fraction);

    std::chrono::milliseconds per_shard_timeout_;
    std::chrono::milliseconds overall_timeout_;
    int max_retries_;
    int initial_backoff_ms_;
    int max_backoff_ms_;
    double backoff_multiplier_;
    double jitter_fraction_;

    mutable std::mt19937 rng_{std::random_device{}()};
    mutable std::unordered_map<std::string, RetryStats> shard_stats_;
    mutable std::vector<TimeoutEvent> timeout_events_;
};

class QueryTimeoutContext {
public:
    /**
     * @brief Query Timeout Context.
     * @param[in] policy Input parameter.
     * @return Return value.
     */
    explicit QueryTimeoutContext(const TimeoutPolicy& policy);

    /**
     * @brief Start Shard Attempt.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] attempt Input parameter.
     */
    void startShardAttempt(const std::string& shard_id, int attempt);

    void endShardAttempt(
        const std::string& shard_id,
        bool success,
        const std::string& failure_reason = "");

    [[nodiscard]] bool shouldRetry(const std::string& shard_id) const;

    [[nodiscard]] bool isOverallTimeoutExceeded() const;

    [[nodiscard]] std::chrono::milliseconds getRemainingTime() const;

    [[nodiscard]] std::chrono::milliseconds getRemainingShardTime(
        const std::string& shard_id) const;

    [[nodiscard]] std::chrono::milliseconds getTotalElapsed() const;

    [[nodiscard]] std::optional<TimeoutPolicy::RetryStats> getShardStats(
        const std::string& shard_id) const;

    [[nodiscard]] themis::utils::RetryMetadata getRetryMetadata(
        const std::string& shard_id) const;

private:
    const TimeoutPolicy& policy_;
    std::chrono::steady_clock::time_point query_start_;
    std::chrono::steady_clock::time_point shard_attempt_start_;
    std::unordered_map<std::string, TimeoutPolicy::RetryStats> shard_stats_;
};

} // namespace themis::query
