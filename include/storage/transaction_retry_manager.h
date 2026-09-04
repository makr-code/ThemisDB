/**
 * @file transaction_retry_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright (c) 2024 ThemisDB
// Licensed under the MIT License

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

namespace themisdb {
namespace storage {

/**
 * @brief Backoff strategy for retry delays
 */
enum class BackoffStrategy {
    EXPONENTIAL,  // Delay doubles each retry: 100ms, 200ms, 400ms, 800ms...
    LINEAR,       // Delay increases linearly: 100ms, 200ms, 300ms, 400ms...
    FIXED         // Delay stays constant: 100ms, 100ms, 100ms, 100ms...
};

/**
 * @brief Error types for classification
 */
enum class ErrorType {
    // Retryable errors
    WRITE_CONFLICT,      // Optimistic concurrency conflict
    TIMEOUT,             // Operation timed out
    NETWORK_ERROR,       // Transient network issue
    RESOURCE_EXHAUSTED,  // Temporary resource shortage
    SERVICE_UNAVAILABLE, // Service temporarily down
    
    // Non-retryable errors
    CONSTRAINT_VIOLATION, // Data integrity violation
    INVALID_ARGUMENT,     // Bad input data
    NOT_FOUND,           // Resource doesn't exist
    PERMISSION_DENIED,   // Authorization failure
    DATA_CORRUPTION,     // Data integrity compromised
    UNKNOWN              // Unclassified error
};

/**
 * @brief Circuit breaker health state
 */
enum class CircuitState {
    HEALTHY,        // All operations succeed
    DEGRADED,       // Some failures (3-9)
    CIRCUIT_OPEN    // Too many failures (10+), blocking new attempts
};

/**
 * @brief Configuration for transaction retry behavior
 */
struct TransactionRetryConfig {
    size_t max_attempts = 5;                 // Maximum retry attempts
    uint32_t base_delay_ms = 100;            // Initial delay (milliseconds)
    uint32_t max_delay_ms = 30000;           // Maximum delay (30 seconds)
    double backoff_multiplier = 2.0;         // Multiplier for exponential backoff
    BackoffStrategy backoff_strategy = BackoffStrategy::EXPONENTIAL;
    bool enable_jitter = true;               // Add randomization to prevent thundering herd
    double jitter_factor = 0.5;              // Jitter range (±50%)
    uint64_t max_total_timeout_ms = 60000;   // Maximum total time for all retries (60s)
    
    // Circuit breaker settings
    bool enable_circuit_breaker = true;
    size_t failure_threshold = 10;           // Open circuit after this many failures
    uint64_t reset_timeout_ms = 60000;       // Try to close circuit after this time (60s)
};

/**
 * @brief Per-operation retry policy (can override global config)
 */
struct RetryPolicy {
    size_t max_attempts = 0;                 // 0 = use config default
    uint32_t base_delay_ms = 0;              // 0 = use config default
    uint32_t max_delay_ms = 0;               // 0 = use config default
    BackoffStrategy backoff_strategy = BackoffStrategy::EXPONENTIAL;
};

/**
 * @brief Statistics for retry operations
 */
struct RetryStatistics {
    std::atomic<uint64_t> total_operations{0};
    std::atomic<uint64_t> successful_operations{0};
    std::atomic<uint64_t> failed_operations{0};
    std::atomic<uint64_t> retried_operations{0};
    std::atomic<uint64_t> total_retry_attempts{0};
    std::atomic<uint64_t> circuit_breaker_blocks{0};
    std::atomic<uint64_t> total_latency_ms{0};

    RetryStatistics() = default;
    RetryStatistics(const RetryStatistics& o)
        : total_operations{o.total_operations.load()},
          successful_operations{o.successful_operations.load()},
          failed_operations{o.failed_operations.load()},
          retried_operations{o.retried_operations.load()},
          total_retry_attempts{o.total_retry_attempts.load()},
          circuit_breaker_blocks{o.circuit_breaker_blocks.load()},
          total_latency_ms{o.total_latency_ms.load()},
          errors_by_type{o.errors_by_type} {}
    RetryStatistics& operator=(const RetryStatistics& o) {
        if (this == &o) {
          return *this;
        }
        total_operations.store(o.total_operations.load());
        successful_operations.store(o.successful_operations.load());
        failed_operations.store(o.failed_operations.load());
        retried_operations.store(o.retried_operations.load());
        total_retry_attempts.store(o.total_retry_attempts.load());
        circuit_breaker_blocks.store(o.circuit_breaker_blocks.load());
        total_latency_ms.store(o.total_latency_ms.load());
        errors_by_type = o.errors_by_type;
        return *this;
    }
    
    // Per-error-type counters
    std::unordered_map<ErrorType, uint64_t> errors_by_type;
    
    double getSuccessRate() const {
        uint64_t total = total_operations.load();
        if (total == 0) {
          return 1.0;
        }
        return static_cast<double>(successful_operations.load()) / total;
    }
    
    double getAverageLatencyMs() const {
        uint64_t total = total_operations.load();
        if (total == 0) {
          return 0.0;
        }
        return static_cast<double>(total_latency_ms.load()) / total;
    }
    
    double getRetryRate() const {
        uint64_t total = total_operations.load();
        if (total == 0) {
          return 0.0;
        }
        return static_cast<double>(retried_operations.load()) / total;
    }
};

/**
 * @brief Automatic transaction retry manager with exponential backoff
 * 
 * Implements automatic retry logic for database transactions with:
 * - Exponential backoff with jitter to prevent thundering herd
 * - Circuit breaker to prevent wasted retries
 * - Error classification (retryable vs non-retryable)
 * - Comprehensive metrics tracking
 * 
 * Thread-safe for concurrent use.
 * 
 * Example:
 * @code
 * TransactionRetryManager manager(config);
 * 
 * auto result = manager.executeWithRetry([&]() {
 *     return database.executeTransaction(tx);
 * }, "user_update");
 * @endcode
 */
class TransactionRetryManager {
public:
    using AlertCallback = std::function<void(CircuitState, const std::string&)>;
    
    /**
     * @brief Construct retry manager with configuration
     * @param config Retry configuration
     */
    explicit TransactionRetryManager(const TransactionRetryConfig& config);
    
    /**
     * @brief Destructor
     */
    ~TransactionRetryManager();
    
    /**
     * @brief Execute operation with automatic retry
     * @tparam Func Function type that returns a result
     * @param operation Function to execute
     * @param operation_name Name for logging/metrics
     * @param policy Optional custom retry policy
     * @return Result of the operation
     * @throws std::runtime_error if max retries exceeded or circuit is open
     */
    template<typename Func>
    auto executeWithRetry(Func&& operation, const std::string& operation_name,
                         const RetryPolicy* policy = nullptr) -> decltype(operation()) {
        auto start_time = std::chrono::steady_clock::now();
        
        // Check circuit breaker
        if (config_.enable_circuit_breaker && isCircuitOpen()) {
            stats_.circuit_breaker_blocks.fetch_add(1);
            stats_.failed_operations.fetch_add(1);
            stats_.total_operations.fetch_add(1);
            throw std::runtime_error("Circuit breaker is open for: " + operation_name);
        }
        
        size_t max_attempts = policy && policy->max_attempts > 0 
                             ? policy->max_attempts 
                             : config_.max_attempts;
        
        size_t attempt = 0;
        bool did_retry = false;
        
        while (attempt < max_attempts) {
            try {
                // Execute operation
                auto result = operation();
                
                // Success!
                recordSuccess();
                
                // Update stats
                auto end_time = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                    end_time - start_time).count();
                
                stats_.total_latency_ms.fetch_add(duration);
                stats_.successful_operations.fetch_add(1);
                stats_.total_operations.fetch_add(1);
                if (did_retry) {
                    stats_.retried_operations.fetch_add(1);
                }
                
                return result;
                
            } catch (const std::exception& e) {
                attempt++;
                
                // Classify error
                ErrorType error_type = classifyError(e.what());
                
                // Update error stats
                {
                    std::lock_guard<std::shared_mutex> lock(stats_mutex_);
                    stats_.errors_by_type[error_type]++;
                }
                
                // Check if error is retryable
                if (!isRetryable(error_type)) {
                    // Non-retryable error - fail immediately
                    stats_.failed_operations.fetch_add(1);
                    stats_.total_operations.fetch_add(1);
                    recordFailure();
                    throw;
                }
                
                // Check if we've exceeded max total timeout
                auto now = std::chrono::steady_clock::now();
                auto elapsed = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - start_time).count());
                if (elapsed >= config_.max_total_timeout_ms) {
                    stats_.failed_operations.fetch_add(1);
                    stats_.total_operations.fetch_add(1);
                    recordFailure();
                    throw std::runtime_error("Max total timeout exceeded for: " + operation_name);
                }
                
                // Check if this was the last attempt
                if (attempt >= max_attempts) {
                    stats_.failed_operations.fetch_add(1);
                    stats_.total_operations.fetch_add(1);
                    recordFailure();
                    throw std::runtime_error("Max retry attempts exceeded for: " + operation_name);
                }
                
                // We will perform another attempt, so count this as a retry.
                stats_.total_retry_attempts.fetch_add(1);
                
                // Calculate delay
                uint32_t delay_ms = calculateDelay(attempt, policy);
                
                // Sleep before retry
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                
                did_retry = true;
            }
        }
        
        // Should never reach here
        stats_.failed_operations.fetch_add(1);
        stats_.total_operations.fetch_add(1);
        recordFailure();
        throw std::runtime_error("Unexpected retry exhaustion for: " + operation_name);
    }
    
    /**
     * @brief Get current configuration
     */
    const TransactionRetryConfig& getConfig() const { return config_; }
    
    /**
     * @brief Get current statistics
     */
    RetryStatistics getStatistics() const;
    
    /**
     * @brief Get current circuit breaker state
     */
    CircuitState getCircuitState() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Set alert callback for circuit breaker state changes
     */
    void setAlertCallback(AlertCallback callback);
    
    /**
     * @brief Classify an error by its message
     */
    static ErrorType classifyError(const std::string& error_message);
    
    /**
     * @brief Check if an error type is retryable
     */
    static bool isRetryable(ErrorType error_type);

private:
    /**
     * @brief Calculate delay for next retry attempt
     */
    uint32_t calculateDelay(size_t attempt, const RetryPolicy* policy);
    
    /**
     * @brief Record successful operation (for circuit breaker)
     */
    void recordSuccess();
    
    /**
     * @brief Record failed operation (for circuit breaker)
     */
    void recordFailure();
    
    /**
     * @brief Check if circuit breaker is open
     */
    bool isCircuitOpen() const;
    
    /**
     * @brief Transition circuit breaker state
     */
    bool transitionCircuitState(CircuitState new_state, std::string* alert_message) const;

    /**
     * @brief Invoke alert callback, if configured
     */
    void invokeAlertCallback(CircuitState state, const std::string& message) const;
    
    TransactionRetryConfig config_;
    RetryStatistics stats_;
    
    // Circuit breaker state
    mutable std::mutex circuit_mutex_;
    mutable CircuitState circuit_state_{CircuitState::HEALTHY};
    mutable size_t consecutive_failures_{0};
    mutable std::chrono::steady_clock::time_point circuit_opened_time_;
    
    // Random number generator for jitter
    mutable std::mutex rng_mutex_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> jitter_dist_;
    
    // Stats mutex
    mutable std::shared_mutex stats_mutex_;
    
    // Alert callback
    mutable std::mutex callback_mutex_;
    AlertCallback alert_callback_;
};

}  // namespace storage
}  // namespace themisdb
