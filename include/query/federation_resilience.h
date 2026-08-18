/**
 * @file federation_resilience.h
 * @brief Resilience patterns and degraded-mode execution for federated queries
 * @version 0.0.1
 * @note Maturity: 🟡 BETA
 * @note Score: 93/100
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
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

namespace themis::query {

/**
 * @brief Circuit breaker pattern for flaky shards
 *
 * Prevents cascading failures by detecting and gracefully handling
 * unavailable or degraded shards.
 *
 * States:
 * - CLOSED: Normal operation, requests pass through
 * - OPEN: Shard failing, requests fail immediately
 * - HALF_OPEN: Testing if shard recovered, limited requests
 */
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
     * @brief Constructor
     * @param shard_id Shard identifier
     * @param config Circuit breaker configuration
     */
    explicit CircuitBreaker(const std::string& shard_id, const Config& config);

    /**
     * @brief Record a successful request
     */
    void recordSuccess();

    /**
     * @brief Record a failed request
     * @param failure_reason Reason for failure
     */
    void recordFailure(const std::string& failure_reason);

    /**
     * @brief Check if request should be allowed
     * @return true if request can proceed
     */
    [[nodiscard]] bool allowRequest() const;

    /**
     * @brief Get current state
     * @return Circuit breaker state
     */
    [[nodiscard]] State getState() const;

    /**
     * @brief Get shard identifier
     * @return Shard ID
     */
    [[nodiscard]] const std::string& getShardId() const;

    /**
     * @brief Get failure reason (last failure)
     * @return Reason if circuit open
     */
    [[nodiscard]] std::optional<std::string> getFailureReason() const;

    /**
     * @brief Get statistics
     * @return Human-readable statistics
     */
    [[nodiscard]] std::string getStatistics() const;

    /**
     * @brief Reset circuit breaker to CLOSED state
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

    void updateState();
};

/**
 * @brief Degraded-mode query execution strategy
 *
 * Handles queries when some shards are unavailable:
 * - Partial result aggregation
 * - Partial failure semantics
 * - Query completion guarantees
 */
class DegradedModeExecutor {
public:
    /**
     * @brief Degradation strategy
     */
    enum class Strategy {
        FAIL_FAST,        // Fail if any shard unavailable
        PARTIAL_RESULTS,  // Return partial results from available shards
        FALLBACK_REPLICA, // Redirect to replica/backup
        BEST_EFFORT,      // Return whatever results are available
    };

    /**
     * @brief Constructor
     * @param strategy Degradation strategy to use
     */
    explicit DegradedModeExecutor(Strategy strategy = Strategy::PARTIAL_RESULTS);

    /**
     * @brief Set the degradation strategy
     * @param strategy New strategy
     */
    void setStrategy(Strategy strategy);

    /**
     * @brief Get current strategy
     * @return Current strategy
     */
    [[nodiscard]] Strategy getStrategy() const;

    /**
     * @brief Check if should allow degraded execution
     * @param available_shards Number of shards available
     * @param total_shards Total number of shards
     * @return true if execution should proceed
     */
    [[nodiscard]] bool shouldProceedDegraded(
        size_t available_shards,
        size_t total_shards) const;

    /**
     * @brief Calculate result confidence score (0-100) based on shard coverage
     * @param available_shards Number of shards with results
     * @param total_shards Total number of shards
     * @return Confidence percentage
     */
    [[nodiscard]] double calculateConfidence(
        size_t available_shards,
        size_t total_shards) const;

    /**
     * @brief Get minimum shard coverage required to proceed
     * @return Minimum percentage (0-100)
     */
    [[nodiscard]] double getMinimumCoverage() const;

    /**
     * @brief Set minimum shard coverage required
     * @param coverage_pct Minimum percentage (0-100)
     */
    void setMinimumCoverage(double coverage_pct);

    /**
     * @brief Get statistics
     * @return Human-readable statistics
     */
    [[nodiscard]] std::string getStatistics() const;

private:
    Strategy strategy_;
    double minimum_coverage_pct_ = 50.0;  // Default: require 50% coverage
    uint64_t degraded_executions_ = 0;
    uint64_t partial_result_queries_ = 0;
};

/**
 * @brief Bounded recovery time SLA tracker
 *
 * Tracks and enforces recovery time objectives (RTO) for degraded shards.
 */
class RecoveryTimeTracker {
public:
    /**
     * @brief Constructor
     * @param shard_id Shard identifier
     * @param recovery_sla_ms Recovery time objective in milliseconds
     */
    RecoveryTimeTracker(const std::string& shard_id, uint64_t recovery_sla_ms);

    /**
     * @brief Mark shard as degraded
     */
    void markDegraded();

    /**
     * @brief Mark shard as recovered
     */
    void markRecovered();

    /**
     * @brief Check if recovery SLA is being met
     * @return true if within SLA
     */
    [[nodiscard]] bool isSLAMet() const;

    /**
     * @brief Get time since degradation started
     * @return Milliseconds, or 0 if not degraded
     */
    [[nodiscard]] uint64_t getTimeSinceDegradation() const;

    /**
     * @brief Get recovery SLA milliseconds
     * @return SLA in milliseconds
     */
    [[nodiscard]] uint64_t getRecoverySLAMs() const;

    /**
     * @brief Get shard identifier
     * @return Shard ID
     */
    [[nodiscard]] const std::string& getShardId() const;

    /**
     * @brief Get statistics
     * @return Human-readable statistics
     */
    [[nodiscard]] std::string getStatistics() const;

private:
    std::string shard_id_;
    uint64_t recovery_sla_ms_;
    bool is_degraded_ = false;
    std::chrono::steady_clock::time_point degradation_start_;
    uint64_t total_degradation_time_ms_ = 0;
    uint64_t recovery_event_count_ = 0;
};

/**
 * @brief Resilience coordinator for federated queries
 *
 * Orchestrates all resilience patterns:
 * - Circuit breaker per shard
 * - Degraded mode execution
 * - Recovery time tracking
 * - Resilience metrics
 */
class FederationResilienceCoordinator {
public:
    /**
     * @brief Constructor
     * @param default_strategy Default degradation strategy
     */
    explicit FederationResilienceCoordinator(
        DegradedModeExecutor::Strategy default_strategy =
            DegradedModeExecutor::Strategy::PARTIAL_RESULTS);

    /**
     * @brief Get or create circuit breaker for shard
     * @param shard_id Shard identifier
     * @param config Optional circuit breaker config
     * @return Reference to circuit breaker
     */
    CircuitBreaker& getOrCreateCircuitBreaker(
        const std::string& shard_id,
        const CircuitBreaker::Config& config = CircuitBreaker::Config());
    /**
     * @brief Check if shard is available
     * @param shard_id Shard identifier
     * @return true if available for queries
     */
    [[nodiscard]] bool isShardAvailable(const std::string& shard_id) const;

    /**
     * @brief Get count of available shards
     * @return Number of available shards
     */
    [[nodiscard]] size_t getAvailableShardCount() const;

    /**
     * @brief Get count of degraded shards
     * @return Number of shards in OPEN or HALF_OPEN state
     */
    [[nodiscard]] size_t getDegradedShardCount() const;

    /**
     * @brief Get degraded-mode executor
     * @return Reference to executor
     */
    DegradedModeExecutor& getDegradedModeExecutor();

    /**
     * @brief Register a shard for recovery tracking
     * @param shard_id Shard identifier
     * @param recovery_sla_ms Recovery SLA in milliseconds
     */
    void registerShardForRecoveryTracking(
        const std::string& shard_id,
        uint64_t recovery_sla_ms = 30000);

    /**
     * @brief Get recovery tracker for shard
     * @param shard_id Shard identifier
     * @return Tracker if registered
     */
    [[nodiscard]] std::optional<std::reference_wrapper<RecoveryTimeTracker>>
    getRecoveryTracker(const std::string& shard_id);

    /**
     * @brief Get overall resilience statistics
     * @return JSON string with all metrics
     */
    [[nodiscard]] std::string getResilienceStatistics() const;

    /**
     * @brief Get summary of shard states
     * @return Map of shard_id -> state string
     */
    [[nodiscard]] std::unordered_map<std::string, std::string>
    getShardStatesSummary() const;

private:
    std::unordered_map<std::string, CircuitBreaker> circuit_breakers_;
    DegradedModeExecutor degraded_executor_;
    std::unordered_map<std::string, RecoveryTimeTracker> recovery_trackers_;
};

} // namespace themis::query
