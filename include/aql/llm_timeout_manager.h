/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_timeout_manager.h                              ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:36:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     225                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • cdac7ec21  2026-02-19  Add production hardening for AQL/LLM subsystem: validatio... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <chrono>
#include <future>
#include <functional>
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
            // Timeout occurred - we can't safely cancel the thread, but we report the error
            // Note: The thread is detached and may complete later (resource consideration)
            // TODO: Implement cooperative cancellation for cleaner shutdown
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
