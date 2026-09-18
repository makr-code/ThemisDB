/**
 * @file federation_resilience.h
 * @brief Resilience patterns and degraded-mode execution for federated queries
 * @version 0.0.1
 * @note Maturity: 🟡 BETA
 * @note Score: 93/100
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
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace themis::query {

class CircuitBreaker {
public:
    enum class State {
        CLOSED,      // Normal operation
        OPEN,        // Failing, reject requests
        HALF_OPEN,   // Testing recovery
    };

    struct Config {
        // Failure threshold: open if N failures in window
        int failure_threshold = 5;
        
        // Time window for failure counting
        std::chrono::milliseconds failure_window{60000};  // 60s
        
        // Time to wait before trying half-open
        std::chrono::milliseconds timeout{30000};  // 30s
        
        // Max requests allowed in HALF_OPEN state
        int half_open_max_requests = 3;
    };

    /**
     * @brief Circuit Breaker.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit CircuitBreaker(const std::string& shard_id, const Config& config);

    /**
     * @brief Record Success.
     */
    void recordSuccess();

    /**
     * @brief Record Failure.
     * @param[in] failure_reason Input parameter.
     */
    void recordFailure(const std::string& failure_reason);

    [[nodiscard]] bool allowRequest() const;

    [[nodiscard]] State getState() const;

    [[nodiscard]] const std::string& getShardId() const;

    [[nodiscard]] std::optional<std::string> getFailureReason() const;

    [[nodiscard]] std::string getStatistics() const;

    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

private:
    std::string shard_id_;
    Config config_;
    State state_ = State::CLOSED;
    int failure_count_ = 0;
    std::chrono::steady_clock::time_point last_failure_time_;
    std::chrono::steady_clock::time_point open_time_;
    int half_open_requests_ = 0;
    std::optional<std::string> last_failure_reason_;

    /**
     * @brief Update State.
     */
    void updateState();
};

class DegradedModeExecutor {
public:
    enum class Strategy {
        FAIL_FAST,        // Fail if any shard unavailable
        PARTIAL_RESULTS,  // Return partial results from available shards
        FALLBACK_REPLICA, // Redirect to replica/backup
        BEST_EFFORT,      // Return whatever results are available
    };

    explicit DegradedModeExecutor(Strategy strategy = Strategy::PARTIAL_RESULTS);

    /**
     * @brief Set Strategy.
     * @param[in] strategy Input parameter.
     */
    void setStrategy(Strategy strategy);

    [[nodiscard]] Strategy getStrategy() const;

    [[nodiscard]] bool shouldProceedDegraded(
        size_t available_shards,
        size_t total_shards) const;

    [[nodiscard]] double calculateConfidence(
        size_t available_shards,
        size_t total_shards) const;

    [[nodiscard]] double getMinimumCoverage() const;

    /**
     * @brief Set Minimum Coverage.
     * @param[in] coverage_pct Input parameter.
     */
    void setMinimumCoverage(double coverage_pct);

    [[nodiscard]] std::string getStatistics() const;

private:
    Strategy strategy_;
    double minimum_coverage_pct_ = 50.0;  // Default: require 50% coverage
    uint64_t degraded_executions_ = 0;
    uint64_t partial_result_queries_ = 0;
};

class RecoveryTimeTracker {
public:
    RecoveryTimeTracker(const std::string& shard_id, uint64_t recovery_sla_ms);

    /**
     * @brief Mark Degraded.
     */
    void markDegraded();

    /**
     * @brief Mark Recovered.
     */
    void markRecovered();

    [[nodiscard]] bool isSLAMet() const;

    [[nodiscard]] uint64_t getTimeSinceDegradation() const;

    [[nodiscard]] uint64_t getRecoverySLAMs() const;

    [[nodiscard]] const std::string& getShardId() const;

    [[nodiscard]] std::string getStatistics() const;

private:
    std::string shard_id_;
    uint64_t recovery_sla_ms_;
    bool is_degraded_ = false;
    std::chrono::steady_clock::time_point degradation_start_;
    uint64_t total_degradation_time_ms_ = 0;
    uint64_t recovery_event_count_ = 0;
};

class FederationResilienceCoordinator {
public:
    explicit FederationResilienceCoordinator(
        DegradedModeExecutor::Strategy default_strategy =
            DegradedModeExecutor::Strategy::PARTIAL_RESULTS);

    CircuitBreaker& getOrCreateCircuitBreaker(
        const std::string& shard_id,
        const CircuitBreaker::Config& config = CircuitBreaker::Config());
    [[nodiscard]] bool isShardAvailable(const std::string& shard_id) const;

    [[nodiscard]] size_t getAvailableShardCount() const;

    [[nodiscard]] size_t getDegradedShardCount() const;

    /**
     * @brief Get Degraded Mode Executor.
     * @return Return value.
     */
    DegradedModeExecutor& getDegradedModeExecutor();

    void registerShardForRecoveryTracking(
        const std::string& shard_id,
        uint64_t recovery_sla_ms = 30000);

    [[nodiscard]] std::optional<std::reference_wrapper<RecoveryTimeTracker>>
    getRecoveryTracker(const std::string& shard_id);

    [[nodiscard]] std::string getResilienceStatistics() const;

    [[nodiscard]] std::unordered_map<std::string, std::string>
    getShardStatesSummary() const;

private:
    std::unordered_map<std::string, CircuitBreaker> circuit_breakers_;
    DegradedModeExecutor degraded_executor_;
    std::unordered_map<std::string, RecoveryTimeTracker> recovery_trackers_;
};

} // namespace themis::query
