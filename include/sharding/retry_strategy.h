/**
 * @file retry_strategy.h
 * @brief Exponential backoff retry strategy for sharding module consensus operations
 * @version 0.0.1
 * @note Provides retry helpers for route failures and stream writes with exponential backoff
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <stdexcept>
#include <thread>
#include <spdlog/spdlog.h>

namespace themisdb {
namespace sharding {

/**
 * @brief ExponentialBackoff helper for retrying operations with exponential delay
 * 
 * Used for:
 * - Route failures in shard_router.cpp
 * - Stream write failures in stream_protocol.cpp
 * 
 * Delay progression: 100ms → 200ms → 400ms → 800ms → 1600ms (with max 10s)
 */
class ExponentialBackoff {
public:
    /**
     * @brief Construct backoff strategy
     * @param max_retries Maximum number of retry attempts (default: 5)
     * @param initial_delay Initial delay in milliseconds (default: 100ms)
     * @param max_delay Maximum delay in milliseconds (default: 10000ms)
     */
    explicit ExponentialBackoff(
        size_t max_retries = 5,
        std::chrono::milliseconds initial_delay = std::chrono::milliseconds(100),
        std::chrono::milliseconds max_delay = std::chrono::milliseconds(10000)
    ) : max_retries_(max_retries),
        initial_delay_(initial_delay),
        max_delay_(max_delay),
        current_retry_(0),
        current_delay_(initial_delay) {
        if (max_retries == 0) {
            throw std::invalid_argument("max_retries must be > 0");
        }
        if (initial_delay.count() <= 0 || max_delay.count() <= 0) {
            throw std::invalid_argument("Delays must be positive");
        }
        if (initial_delay > max_delay) {
            throw std::invalid_argument("initial_delay must be <= max_delay");
        }
    }

    /**
     * @brief Execute operation with exponential backoff retry
     * @tparam Func Callable that returns bool (true = success, false = retry)
     * @param func Operation to retry
     * @return true if operation succeeded, false if all retries exhausted
     * 
     * @example
     * auto result = ExponentialBackoff(5, 100ms, 10s)
     *   .execute([this] { return tryRouteWrite(payload); });
     */
    template<typename Func>
    bool execute(Func func) {
        for (current_retry_ = 0; current_retry_ < max_retries_; ++current_retry_) {
            try {
                if (func()) {
                    return true;  // Success
                }
            } catch (const std::exception& e) {
                spdlog::debug("ExponentialBackoff: Attempt {} failed with exception: {}",
                             current_retry_ + 1, e.what());
            }

            // If not the last retry, wait before retrying
            if (current_retry_ < max_retries_ - 1) {
                std::this_thread::sleep_for(current_delay_);
                // Double delay for next retry, bounded by max
                current_delay_ = std::chrono::milliseconds(
                    std::min(current_delay_.count() * 2, max_delay_.count())
                );
            }
        }

        spdlog::warn("ExponentialBackoff: Exhausted {} retries, giving up", max_retries_);
        return false;  // All retries exhausted
    }

    /**
     * @brief Execute operation with exception suppression
     * @tparam Func Callable that returns bool
     * @param func Operation to retry
     * @param error_handler Optional handler for final errors
     * @return true if succeeded, false otherwise
     */
    template<typename Func, typename ErrorHandler = std::function<void(const std::string&)>>
    bool executeWithHandler(Func func, ErrorHandler error_handler = nullptr) {
        try {
            return execute(func);
        } catch (const std::exception& e) {
            if (error_handler) {
                error_handler(e.what());
            }
            return false;
        }
    }

    /**
     * @brief Get current retry attempt number (0-indexed)
     */
    size_t getCurrentRetry() const { return current_retry_; }

    /**
     * @brief Get current delay being used
     */
    std::chrono::milliseconds getCurrentDelay() const { return current_delay_; }

    /**
     * @brief Reset retry state for reuse
     */
    void reset() {
        current_retry_ = 0;
        current_delay_ = initial_delay_;
    }

private:
    size_t max_retries_;
    std::chrono::milliseconds initial_delay_;
    std::chrono::milliseconds max_delay_;
    size_t current_retry_;
    std::chrono::milliseconds current_delay_;
};

}  // namespace sharding
}  // namespace themisdb
