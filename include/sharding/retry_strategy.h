// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_RETRY_STRATEGY_H
#define THEMISDB_SHARDING_RETRY_STRATEGY_H

#include "sharding/error_handling.h"
#include <chrono>
#include <functional>
#include <random>

namespace themisdb {
namespace sharding {

/**
 * @brief Retry strategy types
 */
enum class RetryStrategy {
    NO_RETRY,
    IMMEDIATE,
    EXPONENTIAL_BACKOFF,
    LINEAR_BACKOFF,
    ADAPTIVE
};

/**
 * @brief Configuration for retry behavior
 */
struct RetryConfig {
    RetryStrategy strategy{RetryStrategy::EXPONENTIAL_BACKOFF};
    size_t max_retries{3};
    std::chrono::milliseconds initial_delay{100};
    std::chrono::milliseconds max_delay{10000};
    double backoff_multiplier{2.0};
    bool jitter{true};  // Add random jitter to prevent thundering herd
};

/**
 * @brief Execute an operation with retry logic
 * 
 * @tparam Func Callable that returns Result<T>
 * @param operation The operation to execute
 * @param config Retry configuration
 * @return Result of the operation after retries
 * 
 * Example:
 *   auto result = executeWithRetry(
 *     []() -> Result<int> { return performRiskyOperation(); },
 *     RetryConfig{.max_retries = 5}
 *   );
 */
template<typename Func>
auto executeWithRetry(Func operation, const RetryConfig& config)
    -> decltype(operation());

/**
 * @brief Calculate delay for next retry attempt
 */
std::chrono::milliseconds calculateRetryDelay(
    const RetryConfig& config,
    size_t attempt_number
);

/**
 * @brief Add jitter to a delay to prevent thundering herd
 */
std::chrono::milliseconds addJitter(
    std::chrono::milliseconds delay,
    double jitter_factor = 0.1
);

} // namespace sharding
} // namespace themisdb

// Template implementation must be in header
namespace themisdb {
namespace sharding {

template<typename Func>
auto executeWithRetry(Func operation, const RetryConfig& config)
    -> decltype(operation()) {
    
    using ResultType = decltype(operation());
    
    if (config.strategy == RetryStrategy::NO_RETRY) {
        return operation();
    }
    
    size_t attempt = 0;
    while (true) {
        auto result = operation();
        
        // Success - return immediately
        if (result) {
            return result;
        }
        
        // Check if we should retry
        attempt++;
        if (attempt >= config.max_retries) {
            return result;  // Return last error
        }
        
        // Check if error is retriable
        if (!isRetriableError(result.error)) {
            return result;  // Don't retry non-retriable errors
        }
        
        // Calculate and apply delay
        auto delay = calculateRetryDelay(config, attempt);
        std::this_thread::sleep_for(delay);
    }
}

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_RETRY_STRATEGY_H
