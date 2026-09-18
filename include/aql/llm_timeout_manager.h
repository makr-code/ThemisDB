/**
 * @file llm_timeout_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class LLMTimeoutManager {
public:
    struct TimeoutConfig {
        std::chrono::seconds infer_timeout{300};      ///< Soft default: 5 minutes
        std::chrono::seconds rag_timeout{600};        ///< Soft default: 10 minutes (RAG is slower)
        std::chrono::seconds embed_timeout{60};       ///< Soft default: 1 minute
        std::chrono::seconds model_load_timeout{900}; ///< Soft default: 15 minutes
        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static TimeoutConfig defaults() { return {}; }
    };
    
    explicit LLMTimeoutManager(const TimeoutConfig& config = TimeoutConfig::defaults())
        : config_(config) {}
    
    template<typename Func, typename Duration, typename Result = std::invoke_result_t<Func>>
    /**
     * @brief Execute With Timeout.
     * @param[in] func Input parameter.
     * @param[in] timeout Input parameter.
     * @param[in] operation_name Name of the operation.
     * @return Return value.
     * @throws LLMException if an error occurs.
     * @details Calls: Result(), task(), get_future(), worker(), std::move(), t(), wait_for(), request_stop().
     */
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
    
    template<typename Func>
    /**
     * @brief Execute Infer With Timeout.
     * @param[in] func Input parameter.
     * @return Return value.
     * @details Calls: executeWithTimeout().
     */
    auto executeInferWithTimeout(Func&& func) {
        return executeWithTimeout(std::forward<Func>(func), config_.infer_timeout, "LLM INFER");
    }
    
    template<typename Func>
    /**
     * @brief Execute RAGWith Timeout.
     * @param[in] func Input parameter.
     * @return Return value.
     * @details Calls: executeWithTimeout().
     */
    auto executeRAGWithTimeout(Func&& func) {
        return executeWithTimeout(std::forward<Func>(func), config_.rag_timeout, "LLM RAG");
    }
    
    template<typename Func>
    /**
     * @brief Execute Embed With Timeout.
     * @param[in] func Input parameter.
     * @return Return value.
     * @details Calls: executeWithTimeout().
     */
    auto executeEmbedWithTimeout(Func&& func) {
        return executeWithTimeout(std::forward<Func>(func), config_.embed_timeout, "LLM EMBED");
    }
    
    template<typename Func>
    /**
     * @brief Execute Model Load With Timeout.
     * @param[in] func Input parameter.
     * @return Return value.
     * @details Calls: executeWithTimeout().
     */
    auto executeModelLoadWithTimeout(Func&& func) {
        return executeWithTimeout(std::forward<Func>(func), config_.model_load_timeout, "LLM MODEL LOAD");
    }

    template<typename Func,
             typename Duration,
             typename Result = std::invoke_result_t<std::decay_t<Func>, std::shared_ptr<std::atomic<bool>>>>
    /**
     * @brief Execute With Cancel Token.
     * @param[in] func Input parameter.
     * @param[in] timeout Input parameter.
     * @param[in] operation_name Name of the operation.
     * @return Return value.
     * @throws LLMException if an error occurs.
     * @details Calls: Result(), task(), f(), get_future(), worker(), std::move(), t(), wait_for().
     */
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

    template<typename Func>
    /**
     * @brief Execute Infer With Cancel Token.
     * @param[in] func Input parameter.
     * @return Return value.
     * @details Calls: executeWithCancelToken().
     */
    auto executeInferWithCancelToken(Func&& func) {
        return executeWithCancelToken(std::forward<Func>(func),
                                      config_.infer_timeout, "LLM INFER");
    }

    template<typename Func>
    /**
     * @brief Execute RAGWith Cancel Token.
     * @param[in] func Input parameter.
     * @return Return value.
     * @details Calls: executeWithCancelToken().
     */
    auto executeRAGWithCancelToken(Func&& func) {
        return executeWithCancelToken(std::forward<Func>(func),
                                      config_.rag_timeout, "LLM RAG");
    }

    template<typename Func>
    /**
     * @brief Execute Embed With Cancel Token.
     * @param[in] func Input parameter.
     * @return Return value.
     * @details Calls: executeWithCancelToken().
     */
    auto executeEmbedWithCancelToken(Func&& func) {
        return executeWithCancelToken(std::forward<Func>(func),
                                      config_.embed_timeout, "LLM EMBED");
    }

    template<typename Func>
    /**
     * @brief Execute Model Load With Cancel Token.
     * @param[in] func Input parameter.
     * @return Return value.
     * @details Calls: executeWithCancelToken().
     */
    auto executeModelLoadWithCancelToken(Func&& func) {
        return executeWithCancelToken(std::forward<Func>(func),
                                      config_.model_load_timeout, "LLM MODEL LOAD");
    }
    
    const TimeoutConfig& getConfig() const { return config_; }
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @details Implements setConfig without additional internal calls.
     */
    void setConfig(const TimeoutConfig& config) { config_ = config; }

private:
    TimeoutConfig config_;
};

class RetryPolicy {
public:
    struct Config {
        size_t max_retries = 3;
        std::chrono::milliseconds initial_delay{100};
        double backoff_multiplier = 2.0;
        std::chrono::milliseconds max_delay{10000};  // 10 seconds max
        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };
    
    explicit RetryPolicy(const Config& config = Config::defaults())
        : config_(config) {}
    
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
     * @brief Is Retryable Error.
     * @param[in] e Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: getErrorCode().
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
