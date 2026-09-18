/**
 * @file circuit_breaker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <memory>

namespace themis::sharding {

/**
 * Circuit Breaker Pattern Implementation
 * 
 * Prevents cascade failures by isolating failing shards automatically.
 * Implements a state machine: CLOSED → OPEN → HALF_OPEN
 * 
 * States:
 * - CLOSED: Normal operation, requests allowed
 * - OPEN: Circuit tripped, requests blocked
 * - HALF_OPEN: Testing recovery, limited requests allowed
 * 
 * Best Practices:
 * - Thread-safe for concurrent access
 * - Configurable thresholds and timeouts
 * - Automatic recovery testing
 * - Per-shard circuit breaker instances
 */
class CircuitBreaker {
public:
    /**
     * Circuit breaker states
     */
    enum class State {
        CLOSED,      // Normal operation
        OPEN,        // Circuit tripped (failures exceeded threshold)
        HALF_OPEN    // Testing recovery
    };
    
    /**
     * Configuration for circuit breaker
     */
    struct Config {
        // Failure threshold before opening circuit
        size_t failure_threshold = 5;
        
        // Timeout before attempting recovery (OPEN → HALF_OPEN)
        std::chrono::milliseconds timeout = std::chrono::seconds(30);
        
        // Success threshold in HALF_OPEN before closing circuit
        size_t success_threshold = 2;
        
        // Window for counting failures (rolling window)
        std::chrono::seconds failure_window = std::chrono::seconds(60);
    };
    
    /**
     * Construct circuit breaker with configuration
     * @brief TBD: Describe CircuitBreaker.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit CircuitBreaker(const Config& config);
    
    /**
     * Check if request should be allowed
     * @return true if request is allowed, false if circuit is OPEN
     * @brief TBD: Describe allowRequest.
     */
    bool allowRequest();
    
    /**
     * Record successful request
     * @brief TBD: Describe recordSuccess.
     */
    void recordSuccess();
    
    /**
     * Record failed request
     * @brief TBD: Describe recordFailure.
     */
    void recordFailure();
    
    /**
     * Get current state
     * @brief TBD: Describe getState.
     * @return Return value.
     */
    State getState() const;
    
    /**
     * Get failure count in current window
     * @brief TBD: Describe getFailureCount.
     * @return Return value.
     */
    size_t getFailureCount() const;
    
    /**
     * Get success count in HALF_OPEN state
     * @brief TBD: Describe getSuccessCount.
     * @return Return value.
     */
    size_t getSuccessCount() const;
    
    /**
     * Reset circuit breaker to CLOSED state
     * @brief TBD: Describe reset.
     */
    void reset();
    
    /**
     * Force circuit breaker to OPEN state
     * @brief TBD: Describe forceOpen.
     */
    void forceOpen();
    
    /**
     * Get human-readable state name
     * @brief TBD: Describe stateToString.
     * @param[in] state Input parameter.
     * @return Return value.
     */
    static std::string stateToString(State state);

private:
    Config config_;
    std::atomic<State> state_{State::CLOSED};
    
    // Failure tracking
    mutable std::mutex mutex_;
    std::vector<std::chrono::steady_clock::time_point> failure_timestamps_;
    size_t consecutive_failures_{0};
    
    // HALF_OPEN state tracking
    size_t half_open_successes_{0};
    
    // Timestamp when circuit was opened
    std::chrono::steady_clock::time_point open_timestamp_;
    
    /**
     * Transition to new state (thread-safe)
     * @brief TBD: Describe transitionTo.
     * @param[in] new_state Input parameter.
     */
    void transitionTo(State new_state);
    
    /**
     * Check if timeout has elapsed (OPEN → HALF_OPEN)
     * @brief TBD: Describe isTimeoutElapsed.
     * @return True on success.
     */
    bool isTimeoutElapsed() const;
    
    /**
     * Remove old failures outside the rolling window
     * @brief TBD: Describe cleanupOldFailures.
     */
    void cleanupOldFailures();
    
    /**
     * Get current failure count in rolling window
     * @brief TBD: Describe getCurrentFailureCount.
     * @return Return value.
     */
    size_t getCurrentFailureCount() const;
};

/**
 * Circuit Breaker Manager
 * 
 * Manages multiple circuit breakers (one per shard)
 * Provides factory and lookup methods
 */
class CircuitBreakerManager {
public:
    /**
     * Get or create circuit breaker for a shard
     * @param shard_id Shard identifier
     * @param config Configuration (used only if creating new circuit breaker)
     * @return Reference to circuit breaker for this shard
     */
    CircuitBreaker& getCircuitBreaker(
        const std::string& shard_id,
        const CircuitBreaker::Config& config = CircuitBreaker::Config{}
    );
    
    /**
     * Check if circuit breaker exists for shard
     * @brief TBD: Describe hasCircuitBreaker.
     * @param[in] shard_id Input parameter.
     * @return True on success.
     */
    bool hasCircuitBreaker(const std::string& shard_id) const;
    
    /**
     * Remove circuit breaker for shard
     * @brief TBD: Describe removeCircuitBreaker.
     * @param[in] shard_id Input parameter.
     */
    void removeCircuitBreaker(const std::string& shard_id);
    
    /**
     * Reset all circuit breakers
     * @brief TBD: Describe resetAll.
     */
    void resetAll();
    
    /**
     * Get all shard IDs with circuit breakers
     * @brief TBD: Describe getAllShardIds.
     * @return Return value.
     */
    std::vector<std::string> getAllShardIds() const;
    
    /**
     * Get count of circuit breakers in each state
     */
    struct StateCount {
        size_t closed = 0;
        size_t open = 0;
        size_t half_open = 0;
    };
    /**
     * @brief TBD: Describe getStateCount.
     * @return Return value.
     */
    StateCount getStateCount() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<CircuitBreaker>> circuit_breakers_;
};

} // namespace themis::sharding
