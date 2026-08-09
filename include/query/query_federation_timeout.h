/**
 * @file query_federation_timeout.h
 * @brief Timeout and retry infrastructure for federated query execution
 * @version 0.0.1
 * @note Maturity: 🟡 BETA
 * @note Score: 95/100
 * @note Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=0, H=1, M=0, L=0
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

/**
 * @brief Timeout and retry policy for federated query execution
 *
 * Provides:
 * - Per-shard timeout envelopes (5s default, configurable)
 * - Overall query timeout (30s default, configurable)
 * - Exponential backoff retry logic
 * - Timeout statistics and observability
 *
 * Example:
 * ```cpp
 * auto policy = TimeoutPolicy::Builder{}
 *     .withPerShardTimeout(std::chrono::seconds(5))
 *     .withOverallTimeout(std::chrono::seconds(30))
 *     .withMaxRetries(3)
 *     .build();
 *
 * auto executor = FederatedQueryExecutor(router, policy);
 * auto result = executor.execute("SELECT * FROM table");
 * ```
 */
class TimeoutPolicy {
public:
    /**
     * @brief Timeout event for observability
     */
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

    /**
     * @brief Retry statistics for a shard
     */
    struct RetryStats {
        int successful_attempt = -1;  // 0-indexed, -1 if not yet successful
        int total_attempts = 0;
        std::chrono::milliseconds total_elapsed{0};
        std::vector<std::chrono::milliseconds> attempt_latencies;
        std::vector<std::string> failure_reasons;
    };

    /**
     * @brief Builder for TimeoutPolicy
     */
    class Builder {
    public:
        Builder& withPerShardTimeout(std::chrono::milliseconds timeout) {
            per_shard_timeout_ = timeout;
            return *this;
        }

        Builder& withOverallTimeout(std::chrono::milliseconds timeout) {
            overall_timeout_ = timeout;
            return *this;
        }

        Builder& withMaxRetries(int max_retries) {
            max_retries_ = max_retries;
            return *this;
        }

        Builder& withInitialBackoffMs(int backoff_ms) {
            initial_backoff_ms_ = backoff_ms;
            return *this;
        }

        Builder& withMaxBackoffMs(int max_backoff_ms) {
            max_backoff_ms_ = max_backoff_ms;
            return *this;
        }

        Builder& withBackoffMultiplier(double multiplier) {
            backoff_multiplier_ = multiplier;
            return *this;
        }

        Builder& withJitterFraction(double jitter) {
            jitter_fraction_ = jitter;
            return *this;
        }

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
    TimeoutPolicy(TimeoutPolicy&&) = default;
    TimeoutPolicy& operator=(TimeoutPolicy&&) = default;

    /**
     * @brief Get per-shard timeout
     */
    [[nodiscard]] std::chrono::milliseconds getPerShardTimeout() const {
        return per_shard_timeout_;
    }

    /**
     * @brief Get overall query timeout
     */
    [[nodiscard]] std::chrono::milliseconds getOverallTimeout() const {
        return overall_timeout_;
    }

    /**
     * @brief Get maximum retry count
     */
    [[nodiscard]] int getMaxRetries() const {
        return max_retries_;
    }

    /**
     * @brief Calculate backoff delay for given attempt number
     * @param attempt Attempt number (0-indexed)
     * @return Backoff delay in milliseconds
     */
    [[nodiscard]] std::chrono::milliseconds calculateBackoff(int attempt) const;

    /**
     * @brief Check if a shard should be retried based on timeout policy
     * @param elapsed Time elapsed since attempt start
     * @param attempt Current attempt number (0-indexed)
     * @return true if retry is permitted, false if should timeout
     */
    [[nodiscard]] bool shouldRetry(
        std::chrono::milliseconds elapsed,
        int attempt) const;

    /**
     * @brief Check if overall query has exceeded timeout
     * @param total_elapsed Total time since query start
     * @return true if query should timeout
     */
    [[nodiscard]] bool isOverallTimeoutExceeded(
        std::chrono::milliseconds total_elapsed) const;

    /**
     * @brief Register a timeout event for observability
     * @param event The timeout event to record
     */
    void recordTimeoutEvent(const TimeoutEvent& event);

    /**
     * @brief Record retry statistics for a shard
     * @param shard_id Shard identifier
     * @param stats Retry statistics
     */
    void recordRetryStats(const std::string& shard_id, const RetryStats& stats);

    /**
     * @brief Get retry statistics for a shard
     * @param shard_id Shard identifier
     * @return RetryStats if available, nullopt otherwise
     */
    [[nodiscard]] std::optional<RetryStats> getRetryStats(
        const std::string& shard_id) const;

    /**
     * @brief Get all timeout events recorded
     * @return Vector of timeout events
     */
    [[nodiscard]] std::vector<TimeoutEvent> getTimeoutEvents() const;

    /**
     * @brief Clear all recorded statistics
     */
    void clearStatistics();

    /**
     * @brief Get summary statistics
     * @return JSON with timeout/retry metrics
     */
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

/**
 * @brief Context for a single federated query execution
 *
 * Tracks timing and timeout behavior for the entire query lifecycle.
 */
class QueryTimeoutContext {
public:
    /**
     * @brief Constructor
     * @param policy Timeout policy to use
     */
    explicit QueryTimeoutContext(const TimeoutPolicy& policy);

    /**
     * @brief Start timing for a shard query attempt
     * @param shard_id Shard identifier
     * @param attempt Attempt number (0-indexed)
     */
    void startShardAttempt(const std::string& shard_id, int attempt);

    /**
     * @brief End timing for a shard query attempt
     * @param shard_id Shard identifier
     * @param success Whether the attempt succeeded
     * @param failure_reason Optional failure reason if !success
     */
    void endShardAttempt(
        const std::string& shard_id,
        bool success,
        const std::string& failure_reason = "");

    /**
     * @brief Check if a shard query should be retried
     * @param shard_id Shard identifier
     * @return true if retry is permitted, false otherwise
     */
    [[nodiscard]] bool shouldRetry(const std::string& shard_id) const;

    /**
     * @brief Check if overall query has exceeded timeout
     * @return true if query should timeout
     */
    [[nodiscard]] bool isOverallTimeoutExceeded() const;

    /**
     * @brief Get remaining time for this query
     * @return Remaining milliseconds, 0 if already expired
     */
    [[nodiscard]] std::chrono::milliseconds getRemainingTime() const;

    /**
     * @brief Get remaining time for a shard
     * @param shard_id Shard identifier
     * @return Remaining milliseconds, 0 if already expired
     */
    [[nodiscard]] std::chrono::milliseconds getRemainingShardTime(
        const std::string& shard_id) const;

    /**
     * @brief Get total elapsed time since query start
     * @return Elapsed time in milliseconds
     */
    [[nodiscard]] std::chrono::milliseconds getTotalElapsed() const;

    /**
     * @brief Get statistics for a shard
     * @param shard_id Shard identifier
     * @return RetryStats if available
     */
    [[nodiscard]] std::optional<TimeoutPolicy::RetryStats> getShardStats(
        const std::string& shard_id) const;

    /**
     * @brief Build canonical retry metadata for a shard attempt stream.
     * @param shard_id Shard identifier
     * @return Canonical retry metadata envelope for diagnostics/automation
     */
    [[nodiscard]] themis::utils::RetryMetadata getRetryMetadata(
        const std::string& shard_id) const;

private:
    const TimeoutPolicy& policy_;
    std::chrono::steady_clock::time_point query_start_;
    std::chrono::steady_clock::time_point shard_attempt_start_;
    std::unordered_map<std::string, TimeoutPolicy::RetryStats> shard_stats_;
};

} // namespace themis::query
