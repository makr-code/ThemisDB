/**
 * @file test_llm_resilience.cpp
 * @brief Unit tests for LLM timeout, retry, and circuit breaker functionality
 */

#include <gtest/gtest.h>
#include "aql/llm_aql_handler.h"
#include "aql/llm_error_codes.h"
#include "aql/llm_timeout_manager.h"
#include <thread>
#include <chrono>

using namespace themis::aql;

class LLMResilienceTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<LLMAQLHandler>();
    }
    
    void TearDown() override {
        handler.reset();
    }
    
    std::unique_ptr<LLMAQLHandler> handler;
};

// ============================================================================
// Timeout Manager Tests
// ============================================================================

TEST_F(LLMResilienceTest, TimeoutManager_FastOperation) {
    LLMTimeoutManager timeout_mgr;
    
    // Fast operation should complete normally
    auto result = timeout_mgr.executeWithTimeout(
        []() { return 42; },
        std::chrono::seconds(5),
        "test_operation"
    );
    
    EXPECT_EQ(result, 42);
}

TEST_F(LLMResilienceTest, TimeoutManager_SlowOperation_Timeout) {
    LLMTimeoutManager timeout_mgr;
    
    // Slow operation should timeout
    EXPECT_THROW({
        try {
            timeout_mgr.executeWithTimeout(
                []() {
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    return 42;
                },
                std::chrono::seconds(1),
                "slow_operation"
            );
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::TIMEOUT);
            EXPECT_NE(std::string(e.what()).find("exceeded timeout"), std::string::npos);
            throw;
        }
    }, LLMException);
}

TEST_F(LLMResilienceTest, TimeoutManager_ConfigurableTimeouts) {
    LLMTimeoutManager::TimeoutConfig config;
    config.infer_timeout = std::chrono::seconds(1);
    config.rag_timeout = std::chrono::seconds(2);
    config.embed_timeout = std::chrono::seconds(1);
    
    LLMTimeoutManager timeout_mgr(config);
    
    // Verify configuration
    EXPECT_EQ(timeout_mgr.getConfig().infer_timeout.count(), 1);
    EXPECT_EQ(timeout_mgr.getConfig().rag_timeout.count(), 2);
    EXPECT_EQ(timeout_mgr.getConfig().embed_timeout.count(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Cooperative cancellation tests
// ─────────────────────────────────────────────────────────────────────────────

// The cancel token must be set to true when the timeout fires, so that the
// function (or a streaming callback it drives) can abort at the next checkpoint.
TEST_F(LLMResilienceTest, TimeoutManager_CancelToken_SetOnTimeout) {
    LLMTimeoutManager timeout_mgr;

    std::shared_ptr<std::atomic<bool>> observed_token;

    EXPECT_THROW({
        timeout_mgr.executeWithCancelToken(
            [&observed_token](auto cancel_token) {
                observed_token = cancel_token;
                // Simulate a slow operation that does NOT check the token itself
                std::this_thread::sleep_for(std::chrono::seconds(10));
                return 0;
            },
            std::chrono::seconds(1),
            "cooperative_op"
        );
    }, LLMException);

    // The cancel token must have been set before the exception was thrown
    ASSERT_NE(observed_token, nullptr);
    EXPECT_TRUE(observed_token->load(std::memory_order_acquire));
}

// A cooperative function that polls the cancel token should exit early when
// the timeout fires, spending far less than the total "work" duration.
TEST_F(LLMResilienceTest, TimeoutManager_CancelToken_FunctionExitsEarly) {
    LLMTimeoutManager timeout_mgr;
    std::atomic<int> iterations_completed{0};

    EXPECT_THROW({
        timeout_mgr.executeWithCancelToken(
            [&iterations_completed](auto cancel_token) {
                // Poll the cancel token at each iteration
                for (int i = 0; i < 1000; ++i) {
                    if (cancel_token->load(std::memory_order_acquire)) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    iterations_completed.fetch_add(1, std::memory_order_relaxed);
                }
                return 0;
            },
            std::chrono::seconds(1),
            "cooperative_poll_op"
        );
    }, LLMException);

    // With a 1 s timeout and 10 ms per iteration, the function should have
    // completed significantly fewer than all 1000 iterations.
    int done = iterations_completed.load();
    EXPECT_LT(done, 1000);
}

// executeWithCancelToken completes normally (no timeout) when the func is fast.
TEST_F(LLMResilienceTest, TimeoutManager_CancelToken_FastOperation_Completes) {
    LLMTimeoutManager timeout_mgr;

    auto result = timeout_mgr.executeWithCancelToken(
        [](auto /*cancel_token*/) { return 42; },
        std::chrono::seconds(5),
        "fast_cooperative_op"
    );

    EXPECT_EQ(result, 42);
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread-leak elimination tests (AC: executeWithTimeout uses jthread, no detach)
// ─────────────────────────────────────────────────────────────────────────────

// After executeWithTimeout() throws TIMEOUT the worker thread must terminate
// within timeout + 500 ms.  A latch (atomic counter) decremented by the worker
// on exit is used to detect termination.
TEST_F(LLMResilienceTest, TimeoutManager_WorkerThreadTerminatesAfterTimeout) {
    LLMTimeoutManager timeout_mgr;
    std::atomic<int> exit_counter{0};

    // Worker sleeps for 1.2 s total; timeout fires at 1 s.
    // The worker will finish ~200 ms after the timeout — well within the
    // 500 ms grace window asserted below.
    EXPECT_THROW({
        timeout_mgr.executeWithTimeout(
            [&exit_counter]() -> int {
                std::this_thread::sleep_for(std::chrono::milliseconds(1200));
                exit_counter.fetch_add(1, std::memory_order_release);
                return 0;
            },
            std::chrono::seconds(1),
            "thread_leak_test"
        );
    }, LLMException);

    // Wait up to 500 ms for the worker to finish.
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (exit_counter.load(std::memory_order_acquire) == 0 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(exit_counter.load(std::memory_order_acquire), 1)
        << "Worker thread did not terminate within timeout + 500 ms";
}

// After executeWithCancelToken() throws TIMEOUT, a cooperative worker that
// polls the cancel token must also terminate within timeout + 500 ms.
TEST_F(LLMResilienceTest, TimeoutManager_CancelToken_WorkerThreadTerminatesAfterTimeout) {
    LLMTimeoutManager timeout_mgr;
    std::atomic<int> exit_counter{0};

    EXPECT_THROW({
        timeout_mgr.executeWithCancelToken(
            [&exit_counter](auto cancel_token) -> int {
                // Poll the cancel token every 20 ms; exits quickly after timeout.
                while (!cancel_token->load(std::memory_order_acquire)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
                exit_counter.fetch_add(1, std::memory_order_release);
                return 0;
            },
            std::chrono::seconds(1),
            "cancel_token_thread_leak_test"
        );
    }, LLMException);

    // Worker should exit within ~20 ms of the cancel token being set,
    // comfortably inside the 500 ms grace window.
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (exit_counter.load(std::memory_order_acquire) == 0 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(exit_counter.load(std::memory_order_acquire), 1)
        << "Cooperative worker thread did not terminate within timeout + 500 ms";
}

// Zero leaked threads after N sequential timeout events.
// A zero-second timeout fires immediately (task is still running); each worker
// completes within 50 ms.  The test waits for all workers to finish and then
// asserts the active-worker counter is zero.
TEST_F(LLMResilienceTest, TimeoutManager_NoThreadLeak_Sequential1000) {
    LLMTimeoutManager timeout_mgr;
    std::atomic<int> active_workers{0};

    const int N = 1000;
    for (int i = 0; i < N; ++i) {
        active_workers.fetch_add(1, std::memory_order_relaxed);
        try {
            timeout_mgr.executeWithTimeout(
                [&active_workers]() -> int {
                    // Worker finishes ~50 ms after it starts.
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    active_workers.fetch_sub(1, std::memory_order_release);
                    return 0;
                },
                std::chrono::seconds(0),   // fires immediately
                "no_leak_stress_test"
            );
            // Reached only if the task finished before the zero-second timeout
            // (should not happen with a 50 ms worker, but handle cleanly).
            active_workers.fetch_sub(1, std::memory_order_release);
        } catch (const LLMException& e) {
            EXPECT_EQ(e.getErrorCode(), LLMErrorCode::TIMEOUT);
        }
    }

    // Wait up to 5 s for all background workers to finish.
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (active_workers.load(std::memory_order_acquire) > 0 &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    const int remaining = active_workers.load(std::memory_order_acquire);
    EXPECT_EQ(remaining, 0)
        << "Thread leak detected: " << remaining << " worker(s) still running";
}

// ============================================================================
// Retry Policy Tests
// ============================================================================

TEST_F(LLMResilienceTest, RetryPolicy_SuccessOnFirstAttempt) {
    RetryPolicy retry_policy;
    int call_count = 0;
    
    auto result = retry_policy.executeWithRetry([&call_count]() {
        call_count++;
        return 42;
    });
    
    EXPECT_EQ(result, 42);
    EXPECT_EQ(call_count, 1);
}

TEST_F(LLMResilienceTest, RetryPolicy_SuccessAfterRetries) {
    RetryPolicy retry_policy;
    int call_count = 0;
    
    auto result = retry_policy.executeWithRetry([&call_count]() {
        call_count++;
        if (call_count < 3) {
            throw std::runtime_error("Temporary failure");
        }
        return 42;
    });
    
    EXPECT_EQ(result, 42);
    EXPECT_EQ(call_count, 3);
}

TEST_F(LLMResilienceTest, RetryPolicy_ExhaustRetries) {
    RetryPolicy::Config config;
    config.max_retries = 2;
    config.initial_delay = std::chrono::milliseconds(10);
    
    RetryPolicy retry_policy(config);
    int call_count = 0;
    
    EXPECT_THROW({
        retry_policy.executeWithRetry([&call_count]() {
            call_count++;
            throw std::runtime_error("Persistent failure");
        });
    }, std::runtime_error);
    
    EXPECT_EQ(call_count, 3);  // 1 initial + 2 retries
}

TEST_F(LLMResilienceTest, RetryPolicy_NonRetryableError) {
    RetryPolicy retry_policy;
    int call_count = 0;
    
    auto should_retry = [](const std::exception& e) {
        // Don't retry validation errors
        return std::string(e.what()).find("validation") == std::string::npos;
    };
    
    EXPECT_THROW({
        retry_policy.executeWithRetry([&call_count]() {
            call_count++;
            throw std::runtime_error("validation failed");
        }, should_retry);
    }, std::runtime_error);
    
    // Should not retry
    EXPECT_EQ(call_count, 1);
}

TEST_F(LLMResilienceTest, RetryPolicy_RetryableErrorCheck) {
    // Test timeout errors are retryable
    LLMException timeout_ex(LLMErrorCode::TIMEOUT, "Timeout occurred");
    EXPECT_TRUE(RetryPolicy::isRetryableError(timeout_ex));
    
    // Test OOM errors are retryable
    LLMException oom_ex(LLMErrorCode::OUT_OF_MEMORY, "Out of memory");
    EXPECT_TRUE(RetryPolicy::isRetryableError(oom_ex));
    
    // Test validation errors are not retryable
    LLMException validation_ex(LLMErrorCode::INVALID_PROMPT, "Invalid prompt");
    EXPECT_FALSE(RetryPolicy::isRetryableError(validation_ex));
}

TEST_F(LLMResilienceTest, RetryPolicy_ExponentialBackoff) {
    RetryPolicy::Config config;
    config.max_retries = 3;
    config.initial_delay = std::chrono::milliseconds(100);
    config.backoff_multiplier = 2.0;
    
    RetryPolicy retry_policy(config);
    int call_count = 0;
    auto start = std::chrono::steady_clock::now();
    
    try {
        retry_policy.executeWithRetry([&call_count]() {
            call_count++;
            throw std::runtime_error("Persistent failure");
        });
    } catch (...) {
        // Expected
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should have waited approximately: 100ms + 200ms = 300ms
    // (first call is immediate, retries are delayed)
    EXPECT_GE(duration.count(), 300);
    EXPECT_LT(duration.count(), 500);  // With some margin
}

// ============================================================================
// Integration Tests with Handler
// ============================================================================

TEST_F(LLMResilienceTest, ExecuteInfer_Timeout_Integration) {
    // This test verifies that timeout exceptions are properly caught and wrapped
    // Note: Actual timeout behavior depends on LLM backend availability
    
    // Test with empty model (should fail quickly, not timeout)
    EXPECT_THROW({
        try {
            // Use very long prompt to potentially trigger timeout in real scenario
            std::string long_prompt(10000, 'a');
            handler->executeInfer(long_prompt, "", "", {});
        } catch (const LLMException& e) {
            // Should get some error (likely model not loaded, not timeout in test env)
            EXPECT_TRUE(
                e.getErrorCode() == LLMErrorCode::INFERENCE_FAILED ||
                e.getErrorCode() == LLMErrorCode::MODEL_NOT_LOADED ||
                e.getErrorCode() == LLMErrorCode::TIMEOUT
            );
            throw;
        }
    }, LLMException);
}

TEST_F(LLMResilienceTest, ExecuteRAG_CircuitBreaker_Integration) {
    // Multiple failures should eventually trip circuit breaker
    // Note: This is an integration test that verifies the circuit breaker is present
    
    for (int i = 0; i < 10; i++) {
        try {
            handler->executeRAG("query", "nonexistent_collection", 5, "", {});
        } catch (const LLMException& e) {
            // Expected to fail - either from validation or circuit breaker
            EXPECT_TRUE(
                e.getErrorCode() == LLMErrorCode::RAG_FAILED ||
                e.getErrorCode() == LLMErrorCode::INVALID_COLLECTION ||
                e.getErrorCode() == LLMErrorCode::INFERENCE_FAILED
            );
        }
    }
}

// ============================================================================
// Timeout Configuration Tests
// ============================================================================

TEST_F(LLMResilienceTest, TimeoutConfig_Defaults) {
    LLMTimeoutManager::TimeoutConfig config;
    
    EXPECT_EQ(config.infer_timeout.count(), 300);      // 5 minutes
    EXPECT_EQ(config.rag_timeout.count(), 600);        // 10 minutes
    EXPECT_EQ(config.embed_timeout.count(), 60);       // 1 minute
    EXPECT_EQ(config.model_load_timeout.count(), 900); // 15 minutes
}

TEST_F(LLMResilienceTest, TimeoutConfig_CustomSettings) {
    LLMTimeoutManager::TimeoutConfig config;
    config.infer_timeout = std::chrono::seconds(10);
    config.rag_timeout = std::chrono::seconds(20);
    config.embed_timeout = std::chrono::seconds(5);
    config.model_load_timeout = std::chrono::seconds(30);
    
    LLMTimeoutManager timeout_mgr(config);
    
    // Verify custom settings are applied
    const auto& applied_config = timeout_mgr.getConfig();
    EXPECT_EQ(applied_config.infer_timeout.count(), 10);
    EXPECT_EQ(applied_config.rag_timeout.count(), 20);
    EXPECT_EQ(applied_config.embed_timeout.count(), 5);
    EXPECT_EQ(applied_config.model_load_timeout.count(), 30);
}

TEST_F(LLMResilienceTest, TimeoutConfig_UpdateConfiguration) {
    LLMTimeoutManager timeout_mgr;
    
    // Update configuration
    LLMTimeoutManager::TimeoutConfig new_config;
    new_config.infer_timeout = std::chrono::seconds(100);
    timeout_mgr.setConfig(new_config);
    
    // Verify update
    EXPECT_EQ(timeout_mgr.getConfig().infer_timeout.count(), 100);
}

// ============================================================================
// Retry Configuration Tests
// ============================================================================

TEST_F(LLMResilienceTest, RetryConfig_Defaults) {
    RetryPolicy::Config config;
    
    EXPECT_EQ(config.max_retries, 3);
    EXPECT_EQ(config.initial_delay.count(), 100);
    EXPECT_EQ(config.backoff_multiplier, 2.0);
    EXPECT_EQ(config.max_delay.count(), 10000);  // 10 seconds
}

TEST_F(LLMResilienceTest, RetryConfig_MaxDelayEnforcement) {
    RetryPolicy::Config config;
    config.max_retries = 10;
    config.initial_delay = std::chrono::milliseconds(100);
    config.backoff_multiplier = 2.0;
    config.max_delay = std::chrono::milliseconds(500);
    
    RetryPolicy retry_policy(config);
    int call_count = 0;
    auto start = std::chrono::steady_clock::now();
    
    try {
        retry_policy.executeWithRetry([&call_count]() {
            call_count++;
            throw std::runtime_error("Persistent failure");
        });
    } catch (...) {
        // Expected
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Even with exponential backoff, max delay should cap the wait time
    // Max total wait: 100 + 200 + 400 + 500*7 = 4200ms
    // Allow some margin for thread scheduling overhead
    EXPECT_LT(duration.count(), 4500);
}
