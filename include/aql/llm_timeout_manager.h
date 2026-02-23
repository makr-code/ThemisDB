/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_timeout_manager.h                              ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     318                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7a0f10c19  2026-02-22  feat(llm/aql): cooperative cancellation in LLMTimeoutManager ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <atomic>
#include <chrono>
#include <future>
#include <functional>
#include <memory>
#include <stdexcept>
#include "aql/llm_error_codes.h"

namespace themis {
namespace aql {

/**
 * @brief Timeout manager for LLM operations
 * 
 * Provides timeout enforcement for long-running LLM operations.
 * Supports configurable timeouts per operation type.
 */
class LLMTimeoutManager {
public:
    /**
     * @brief Timeout configuration for different operation types
     */
    struct TimeoutConfig {
        std::chrono::seconds infer_timeout{300};      // 5 minutes default
        std::chrono::seconds rag_timeout{600};        // 10 minutes default (RAG is slower)
        std::chrono::seconds embed_timeout{60};       // 1 minute default
        std::chrono::seconds model_load_timeout{900}; // 15 minutes default
    };
    
    explicit LLMTimeoutManager(const TimeoutConfig& config = TimeoutConfig{})
        : config_(config) {}
    
    /**
     * @brief Execute function with timeout
     * @tparam Func Callable type
     * @tparam Result Return type of function
     * @param func Function to execute
     * @param timeout Timeout duration
     * @param operation_name Name for error reporting
     * @return Result of function execution
     * @throws LLMException with TIMEOUT code if execution exceeds timeout
     * 
     * @note If timeout occurs, the worker thread is detached and may continue
     *       executing. For production use, consider implementing cooperative
     *       cancellation via std::stop_token (C++20) or manual cancellation flags.
     */
    template<typename Func, typename Result = std::invoke_result_t<Func>>
    Result executeWithTimeout(Func&& func, std::chrono::seconds timeout, const std::string& operation_name) {
        // Create a packaged task
        std::packaged_task<Result()> task(std::forward<Func>(func));
        auto future = task.get_future();
        
        // Run task in a separate thread
        std::thread worker(std::move(task));
        
        // Wait for result with timeout
        auto status = future.wait_for(timeout);
        
        if (status == std::future_status::timeout) {
            // Timeout occurred — the worker thread is detached and may complete later.
            // For cooperative cancellation (allowing the function to abort early),
            // use executeWithCancelToken() instead, which passes a cancel flag to func.
            worker.detach();
            throw LLMException(LLMErrorCode::TIMEOUT,
                "Operation '" + operation_name + "' exceeded timeout of " +
                std::to_string(timeout.count()) + " seconds");
        }
        
        // Get result (may throw exception from task)
        worker.join();
        return future.get();
    }
    
    /**
     * @brief Execute inference with configured timeout
     */
    template<typename Func>
    auto executeInferWithTimeout(Func&& func) {
        return executeWithTimeout(std::forward<Func>(func), config_.infer_timeout, "LLM INFER");
    }
    
    /**
     * @brief Execute RAG with configured timeout
     */
    template<typename Func>
    auto executeRAGWithTimeout(Func&& func) {
        return executeWithTimeout(std::forward<Func>(func), config_.rag_timeout, "LLM RAG");
    }
    
    /**
     * @brief Execute embedding with configured timeout
     */
    template<typename Func>
    auto executeEmbedWithTimeout(Func&& func) {
        return executeWithTimeout(std::forward<Func>(func), config_.embed_timeout, "LLM EMBED");
    }
    
    /**
     * @brief Execute model load with configured timeout
     */
    template<typename Func>
    auto executeModelLoadWithTimeout(Func&& func) {
        return executeWithTimeout(std::forward<Func>(func), config_.model_load_timeout, "LLM MODEL LOAD");
    }

    /**
     * @brief Execute function with timeout and cooperative cancellation.
     *
     * Like executeWithTimeout(), but passes a shared cancel token to @p func.
     * When the timeout fires the token is set to @c true before the worker
     * thread is detached, giving the function an opportunity to abort at the
     * next point where it checks the token.
     *
     * @tparam Func Callable of the form @c Result(std::shared_ptr<std::atomic<bool>>).
     * @param func Function to execute; receives the cancel token as its sole argument.
     * @param timeout Timeout duration.
     * @param operation_name Name for error reporting.
     * @return Result of function execution.
     * @throws LLMException with TIMEOUT code if execution exceeds timeout.
     *
     * Usage example:
     * @code
     *   timeout_mgr.executeWithCancelToken(
     *       [](auto cancel_token) {
     *           for (auto& chunk : data) {
     *               if (cancel_token->load(std::memory_order_acquire)) break;
     *               process(chunk);
     *           }
     *       },
     *       std::chrono::seconds(5), "my_op");
     * @endcode
     */
    template<typename Func,
             typename Result = std::invoke_result_t<std::decay_t<Func>, std::shared_ptr<std::atomic<bool>>>>
    Result executeWithCancelToken(Func&& func,
                                  std::chrono::seconds timeout,
                                  const std::string& operation_name) {
        auto cancel_token = std::make_shared<std::atomic<bool>>(false);

        std::packaged_task<Result()> task(
            [f = std::forward<Func>(func), ct = cancel_token]() mutable {
                return f(ct);
            });
        auto future = task.get_future();

        std::thread worker(std::move(task));

        auto status = future.wait_for(timeout);

        if (status == std::future_status::timeout) {
            // Signal cooperative cancellation so the function can exit early.
            cancel_token->store(true, std::memory_order_release);
            worker.detach();
            throw LLMException(LLMErrorCode::TIMEOUT,
                "Operation '" + operation_name + "' exceeded timeout of " +
                std::to_string(timeout.count()) + " seconds");
        }

        worker.join();
        return future.get();
    }

    /**
     * @brief Execute inference with configured timeout and cooperative cancellation.
     */
    template<typename Func>
    auto executeInferWithCancelToken(Func&& func) {
        return executeWithCancelToken(std::forward<Func>(func),
                                      config_.infer_timeout, "LLM INFER");
    }

    /**
     * @brief Execute RAG with configured timeout and cooperative cancellation.
     */
    template<typename Func>
    auto executeRAGWithCancelToken(Func&& func) {
        return executeWithCancelToken(std::forward<Func>(func),
                                      config_.rag_timeout, "LLM RAG");
    }

    /**
     * @brief Execute embedding with configured timeout and cooperative cancellation.
     */
    template<typename Func>
    auto executeEmbedWithCancelToken(Func&& func) {
        return executeWithCancelToken(std::forward<Func>(func),
                                      config_.embed_timeout, "LLM EMBED");
    }

    /**
     * @brief Execute model load with configured timeout and cooperative cancellation.
     */
    template<typename Func>
    auto executeModelLoadWithCancelToken(Func&& func) {
        return executeWithCancelToken(std::forward<Func>(func),
                                      config_.model_load_timeout, "LLM MODEL LOAD");
    }
    
    /**
     * @brief Get current timeout configuration
     */
    const TimeoutConfig& getConfig() const { return config_; }
    
    /**
     * @brief Update timeout configuration
     */
    void setConfig(const TimeoutConfig& config) { config_ = config; }

private:
    TimeoutConfig config_;
};

/**
 * @brief Retry policy with exponential backoff
 */
class RetryPolicy {
public:
    struct Config {
        size_t max_retries = 3;
        std::chrono::milliseconds initial_delay{100};
        double backoff_multiplier = 2.0;
        std::chrono::milliseconds max_delay{10000};  // 10 seconds max
    };
    
    explicit RetryPolicy(const Config& config = Config{})
        : config_(config) {}
    
    /**
     * @brief Execute function with retry and exponential backoff
     * @tparam Func Callable type
     * @param func Function to execute
     * @param should_retry Predicate to determine if error is retryable
     * @return Result of successful execution
     * @throws Last exception if all retries exhausted
     */
    template<typename Func>
    auto executeWithRetry(Func&& func, 
                         std::function<bool(const std::exception&)> should_retry = nullptr) {
        size_t attempt = 0;
        std::chrono::milliseconds delay = config_.initial_delay;
        
        while (true) {
            try {
                return func();
            } catch (const std::exception& e) {
                attempt++;
                
                // Check if we should retry this error
                if (should_retry && !should_retry(e)) {
                    throw;  // Non-retryable error
                }
                
                // Check if we've exhausted retries
                if (attempt >= config_.max_retries) {
                    throw;  // Give up after max retries
                }
                
                // Wait before retry with exponential backoff
                std::this_thread::sleep_for(delay);
                
                // Calculate next delay
                delay = std::chrono::milliseconds(
                    static_cast<long long>(delay.count() * config_.backoff_multiplier)
                );
                delay = std::min(delay, config_.max_delay);
            }
        }
    }
    
    /**
     * @brief Check if an LLM error is retryable
     */
    static bool isRetryableError(const std::exception& e) {
        // Try to cast to LLMException
        try {
            const auto& llm_ex = dynamic_cast<const LLMException&>(e);
            auto code = llm_ex.getErrorCode();
            
            // Retryable errors: timeouts, transient failures
            return code == LLMErrorCode::TIMEOUT ||
                   code == LLMErrorCode::OUT_OF_MEMORY ||
                   code == LLMErrorCode::MODEL_NOT_LOADED;
        } catch (const std::bad_cast&) {
            // Not an LLMException, assume it might be retryable
            return true;
        }
    }

private:
    Config config_;
};

} // namespace aql
} // namespace themis
