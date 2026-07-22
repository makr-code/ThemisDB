/**
 * @file llm_timeout_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: llm_timeout_manager.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 363
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4201 feat(base): async retry bac... (2026-03-15) | #1262 Add production hardening fo... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <atomic>
#include <chrono>
#include <future>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
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
     * @brief Timeout configuration for different operation types.
     *
     * All four fields are **soft defaults** — they are used when a timeout is not
     * specified explicitly.  Override any value at construction time or at runtime
     * via @ref LLMTimeoutManager::setConfig():
     *
     * @code
     *   LLMTimeoutManager::TimeoutConfig cfg;
     *   cfg.infer_timeout        = std::chrono::seconds(60);  // tighter inference budget
     *   cfg.rag_timeout          = std::chrono::seconds(120); // more time for RAG
     *   cfg.embed_timeout        = std::chrono::seconds(30);
     *   cfg.model_load_timeout   = std::chrono::seconds(300);
     *   timeout_mgr.setConfig(cfg);
     * @endcode
     *
     * Default values:
     *   - @c infer_timeout        300 s  (5 min)   — LLM inference / chat completion
     *   - @c rag_timeout          600 s  (10 min)  — retrieval-augmented generation (slower)
     *   - @c embed_timeout         60 s  (1 min)   — embedding generation
     *   - @c model_load_timeout   900 s  (15 min)  — cold model load from disk
     */
    struct TimeoutConfig {
        std::chrono::seconds infer_timeout{300};      ///< Soft default: 5 minutes
        std::chrono::seconds rag_timeout{600};        ///< Soft default: 10 minutes (RAG is slower)
        std::chrono::seconds embed_timeout{60};       ///< Soft default: 1 minute
        std::chrono::seconds model_load_timeout{900}; ///< Soft default: 15 minutes
        static TimeoutConfig defaults() { return {}; }
    };
    
    explicit LLMTimeoutManager(const TimeoutConfig& config = TimeoutConfig::defaults())
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
     * @note Uses `std::jthread` internally.  When a timeout occurs,
     *       `request_stop()` is signalled on the worker and ownership of the
     *       thread is transferred to a thin background cleanup thread that joins
     *       it once it finishes — **the worker thread is never detached**.  This
     *       eliminates the thread-leak that occurred when the previous
     *       implementation called `worker.detach()` on timeout.  (The cleanup
     *       wrapper thread itself is detached, but it holds no captured references
     *       other than the jthread handle and exits as soon as the join completes.)
     *       For cooperative early exit (so the function can abort at its next
     *       check-point) use @ref executeWithCancelToken() instead.
     */
    template<typename Func, typename Duration, typename Result = std::invoke_result_t<Func>>
    Result executeWithTimeout(Func&& func, Duration timeout, const std::string& operation_name) {
        // Wrap the user callable in a packaged_task so we can retrieve the result
        // (or propagated exception) via a future.
        std::packaged_task<Result()> task(std::forward<Func>(func));
        auto future = task.get_future();

        // std::jthread: destructor calls request_stop() + join() automatically,
        // so no explicit join is needed on the success path.
        std::jthread worker([t = std::move(task)](std::stop_token) mutable { t(); });

        auto status = future.wait_for(timeout);

        if (status == std::future_status::timeout) {
            // Signal the stop token so a cooperative worker can exit early, then
            // transfer ownership to a background cleanup thread that will join the
            // worker once it finishes.  This avoids both blocking the calling
            // thread and leaking the worker thread handle.
            // NOTE: the empty lambda body is intentional — when the lambda's local
            // variable `w` (a std::jthread) is destroyed at the end of the cleanup
            // thread's invocation, its destructor calls request_stop() + join(),
            // blocking the cleanup thread until the worker finishes.  This ensures
            // the worker is properly joined with no thread leak.
            worker.request_stop();
            std::thread([w = std::move(worker)]() mutable {
                // jthread destructor: request_stop() + join() — blocks here until
                // the worker finishes, then both this cleanup thread and the worker
                // thread exit cleanly.
            }).detach();
            using seconds_t = std::chrono::seconds;
            auto secs = std::chrono::duration_cast<seconds_t>(timeout);
            throw LLMException(LLMErrorCode::TIMEOUT,
                "Operation '" + operation_name + "' exceeded timeout of " +
                std::to_string(secs.count()) + " seconds");
        }

        // Task is already complete; future.get() returns/throws immediately.
        // worker.~jthread() will join (returns at once since the task is done).
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
     * thread is handed off for cleanup, giving the function an opportunity to
     * abort at the next point where it checks the token.  The worker thread is
     * never detached — it is joined by a background cleanup thread once it
     * finishes.
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
             typename Duration,
             typename Result = std::invoke_result_t<std::decay_t<Func>, std::shared_ptr<std::atomic<bool>>>>
    Result executeWithCancelToken(Func&& func,
                                  Duration timeout,
                                  const std::string& operation_name) {
        auto cancel_token = std::make_shared<std::atomic<bool>>(false);

        std::packaged_task<Result()> task(
            [f = std::forward<Func>(func), ct = cancel_token]() mutable {
                return f(ct);
            });
        auto future = task.get_future();

        // std::jthread: destructor calls request_stop() + join() automatically.
        std::jthread worker([t = std::move(task)](std::stop_token) mutable { t(); });

        auto status = future.wait_for(timeout);

        if (status == std::future_status::timeout) {
            // Signal cooperative cancellation so the function can exit early.
            cancel_token->store(true, std::memory_order_release);
            // Also signal the jthread stop token and hand off to cleanup thread.
            // NOTE: the empty lambda body is intentional — when the lambda's local
            // variable `w` (a std::jthread) is destroyed at the end of the cleanup
            // thread's invocation, its destructor calls request_stop() + join(),
            // blocking the cleanup thread until the worker finishes.
            worker.request_stop();
            std::thread([w = std::move(worker)]() mutable {
                // jthread destructor: request_stop() + join() — blocks here until
                // the worker finishes, then both this cleanup thread and the worker
                // thread exit cleanly.
            }).detach();
            using seconds_t = std::chrono::seconds;
            auto secs = std::chrono::duration_cast<seconds_t>(timeout);
            throw LLMException(LLMErrorCode::TIMEOUT,
                "Operation '" + operation_name + "' exceeded timeout of " +
                std::to_string(secs.count()) + " seconds");
        }

        // Task is already complete; future.get() returns/throws immediately.
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
        static Config defaults() { return {}; }
    };
    
    explicit RetryPolicy(const Config& config = Config::defaults())
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
                
                // Check if we've exhausted retries.
                // max_retries is the number of retries after the initial call,
                // so we exhaust only once attempt exceeds it.
                // Check if we've exhausted retries (max_retries counts retry attempts,
                // not the initial call)
                if (attempt > config_.max_retries) {
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
