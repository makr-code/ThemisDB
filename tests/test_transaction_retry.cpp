#include <gtest/gtest.h>
#include "storage/transaction_retry_manager.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace themis;
using namespace std::chrono_literals;

// Test fixture for TransactionRetryManager
class TransactionRetryManagerTest : public ::testing::Test {
protected:
    TransactionRetryConfig config;
    
    void SetUp() override {
        config.max_attempts = 5;
        config.base_delay_ms = 100;
        config.max_delay_ms = 30000;
        config.backoff_multiplier = 2.0;
        config.backoff_strategy = BackoffStrategy::EXPONENTIAL;
        config.enable_jitter = true;
        config.jitter_factor = 0.5;
        config.max_total_timeout_ms = 60000;
        config.enable_circuit_breaker = true;
        config.failure_threshold = 10;
        config.reset_timeout_ms = 60000;
    }
};

// Test 1: Construction and destruction
TEST_F(TransactionRetryManagerTest, ConstructionDestruction) {
    TransactionRetryManager manager(config);
    EXPECT_TRUE(true); // Should construct and destruct cleanly
}

// Test 2: Configuration retrieval
TEST_F(TransactionRetryManagerTest, GetConfiguration) {
    TransactionRetryManager manager(config);
    const auto& retrieved_config = manager.getConfig();
    EXPECT_EQ(retrieved_config.max_attempts, 5);
    EXPECT_EQ(retrieved_config.base_delay_ms, 100);
    EXPECT_EQ(retrieved_config.backoff_multiplier, 2.0);
}

// Test 3: Successful execution (no retry)
TEST_F(TransactionRetryManagerTest, SuccessfulExecutionNoRetry) {
    TransactionRetryManager manager(config);
    int call_count = 0;
    
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        call_count++;
        return Result<int>::ok(42);
    }, "test_operation");
    
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(call_count, 1); // Should only be called once
}

// Test 4: Single retry on transient error
TEST_F(TransactionRetryManagerTest, SingleRetryOnTransientError) {
    TransactionRetryManager manager(config);
    int call_count = 0;
    
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        call_count++;
        if (call_count == 1) {
            return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
        }
        return Result<int>::ok(42);
    }, "test_operation");
    
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(call_count, 2); // First attempt + 1 retry
}

// Test 5: Multiple retries with exponential backoff
TEST_F(TransactionRetryManagerTest, MultipleRetriesExponentialBackoff) {
    TransactionRetryManager manager(config);
    int call_count = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        call_count++;
        if (call_count < 4) {
            return Result<int>::error(ErrorType::WRITE_CONFLICT, "Conflict");
        }
        return Result<int>::ok(42);
    }, "test_operation");
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(call_count, 4); // 3 retries + 1 success
    // Should have waited at least: 100ms + 200ms + 400ms = 700ms (with jitter, could be less)
    EXPECT_GE(duration_ms, 350); // At least half due to jitter
}

// Test 6: Max attempts enforcement
TEST_F(TransactionRetryManagerTest, MaxAttemptsEnforcement) {
    TransactionRetryManager manager(config);
    int call_count = 0;
    
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        call_count++;
        return Result<int>::error(ErrorType::RESOURCE_EXHAUSTED, "Exhausted");
    }, "test_operation");
    
    ASSERT_FALSE(result.is_ok());
    EXPECT_EQ(call_count, config.max_attempts); // Should try exactly max_attempts times
}

// Test 7: Non-retryable error handling
TEST_F(TransactionRetryManagerTest, NonRetryableErrorHandling) {
    TransactionRetryManager manager(config);
    int call_count = 0;
    
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        call_count++;
        return Result<int>::error(ErrorType::CONSTRAINT_VIOLATION, "Constraint violated");
    }, "test_operation");
    
    ASSERT_FALSE(result.is_ok());
    EXPECT_EQ(call_count, 1); // Should only be called once for non-retryable error
}

// Test 8: Backoff delay verification
TEST_F(TransactionRetryManagerTest, BackoffDelayVerification) {
    config.enable_jitter = false; // Disable jitter for precise timing
    TransactionRetryManager manager(config);
    
    std::vector<int64_t> delays;
    int call_count = 0;
    auto last_time = std::chrono::steady_clock::now();
    
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        auto current_time = std::chrono::steady_clock::now();
        if (call_count > 0) {
            auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time).count();
            delays.push_back(delay);
        }
        last_time = current_time;
        call_count++;
        
        if (call_count < 4) {
            return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
        }
        return Result<int>::ok(42);
    }, "test_operation");
    
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(delays.size(), 3);
    // Exponential: 100ms, 200ms, 400ms (allowing 20% tolerance)
    EXPECT_NEAR(delays[0], 100, 20);
    EXPECT_NEAR(delays[1], 200, 40);
    EXPECT_NEAR(delays[2], 400, 80);
}

// Test 9: Jitter application
TEST_F(TransactionRetryManagerTest, JitterApplication) {
    config.enable_jitter = true;
    config.jitter_factor = 0.5;
    TransactionRetryManager manager(config);
    
    std::vector<int64_t> delays;
    for (int i = 0; i < 10; i++) {
        int call_count = 0;
        auto last_time = std::chrono::steady_clock::now();
        
        auto result = manager.executeWithRetry([&]() -> Result<int> {
            auto current_time = std::chrono::steady_clock::now();
            if (call_count > 0) {
                auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time).count();
                delays.push_back(delay);
            }
            last_time = current_time;
            call_count++;
            
            if (call_count < 2) {
                return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
            }
            return Result<int>::ok(42);
        }, "test_operation");
    }
    
    // With jitter, delays should vary
    bool has_variation = false;
    for (size_t i = 1; i < delays.size(); i++) {
        if (std::abs(delays[i] - delays[i-1]) > 5) {
            has_variation = true;
            break;
        }
    }
    EXPECT_TRUE(has_variation);
}

// Test 10: Max delay enforcement
TEST_F(TransactionRetryManagerTest, MaxDelayEnforcement) {
    config.max_delay_ms = 500;
    config.enable_jitter = false;
    TransactionRetryManager manager(config);
    
    std::vector<int64_t> delays;
    int call_count = 0;
    auto last_time = std::chrono::steady_clock::now();
    
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        auto current_time = std::chrono::steady_clock::now();
        if (call_count > 0) {
            auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time).count();
            delays.push_back(delay);
        }
        last_time = current_time;
        call_count++;
        
        if (call_count < 5) {
            return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
        }
        return Result<int>::ok(42);
    }, "test_operation");
    
    ASSERT_TRUE(result.is_ok());
    // All delays should be <= max_delay_ms
    for (auto delay : delays) {
        EXPECT_LE(delay, config.max_delay_ms + 50); // Allow some tolerance
    }
}

// Test 11: Total timeout enforcement
TEST_F(TransactionRetryManagerTest, TotalTimeoutEnforcement) {
    config.max_total_timeout_ms = 500;
    config.base_delay_ms = 200;
    TransactionRetryManager manager(config);
    
    int call_count = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        call_count++;
        return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
    }, "test_operation");
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    ASSERT_FALSE(result.is_ok());
    EXPECT_LE(duration_ms, config.max_total_timeout_ms + 200); // Allow tolerance
}

// Test 12: Statistics tracking
TEST_F(TransactionRetryManagerTest, StatisticsTracking) {
    TransactionRetryManager manager(config);
    
    // Successful operation
    manager.executeWithRetry([&]() -> Result<int> {
        return Result<int>::ok(42);
    }, "test_operation");
    
    // Operation with retries
    int call_count = 0;
    manager.executeWithRetry([&]() -> Result<int> {
        call_count++;
        if (call_count < 3) {
            return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
        }
        return Result<int>::ok(42);
    }, "test_operation");
    
    const auto& stats = manager.getStatistics();
    EXPECT_EQ(stats.total_operations, 2);
    EXPECT_EQ(stats.successful_operations, 2);
    EXPECT_GE(stats.total_attempts, 4); // 1 + 3 attempts
}

// Test 13: Success rate calculation
TEST_F(TransactionRetryManagerTest, SuccessRateCalculation) {
    TransactionRetryManager manager(config);
    
    // 7 successful, 3 failed
    for (int i = 0; i < 7; i++) {
        manager.executeWithRetry([&]() -> Result<int> {
            return Result<int>::ok(42);
        }, "test_operation");
    }
    
    for (int i = 0; i < 3; i++) {
        manager.executeWithRetry([&]() -> Result<int> {
            return Result<int>::error(ErrorType::CONSTRAINT_VIOLATION, "Non-retryable");
        }, "test_operation");
    }
    
    const auto& stats = manager.getStatistics();
    EXPECT_EQ(stats.total_operations, 10);
    EXPECT_EQ(stats.successful_operations, 7);
    EXPECT_DOUBLE_EQ(stats.success_rate, 0.7);
}

// Test 14: Circuit breaker state transitions
TEST_F(TransactionRetryManagerTest, CircuitBreakerStateTransitions) {
    TransactionRetryManager manager(config);
    
    // Initial state should be HEALTHY
    EXPECT_EQ(manager.getHealthState(), HealthState::HEALTHY);
    
    // Trigger failures to move to DEGRADED
    for (int i = 0; i < 5; i++) {
        manager.executeWithRetry([&]() -> Result<int> {
            return Result<int>::error(ErrorType::CONSTRAINT_VIOLATION, "Non-retryable");
        }, "test_operation");
    }
    EXPECT_EQ(manager.getHealthState(), HealthState::DEGRADED);
    
    // More failures to open circuit
    for (int i = 0; i < 10; i++) {
        manager.executeWithRetry([&]() -> Result<int> {
            return Result<int>::error(ErrorType::CONSTRAINT_VIOLATION, "Non-retryable");
        }, "test_operation");
    }
    EXPECT_EQ(manager.getHealthState(), HealthState::CIRCUIT_OPEN);
}

// Test 15: Circuit breaker blocks execution when open
TEST_F(TransactionRetryManagerTest, CircuitBreakerBlocksExecution) {
    TransactionRetryManager manager(config);
    
    // Open the circuit
    for (int i = 0; i < 15; i++) {
        manager.executeWithRetry([&]() -> Result<int> {
            return Result<int>::error(ErrorType::CONSTRAINT_VIOLATION, "Non-retryable");
        }, "test_operation");
    }
    
    EXPECT_EQ(manager.getHealthState(), HealthState::CIRCUIT_OPEN);
    
    // Next operation should fail immediately
    int call_count = 0;
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        call_count++;
        return Result<int>::ok(42);
    }, "test_operation");
    
    ASSERT_FALSE(result.is_ok());
    EXPECT_EQ(call_count, 0); // Should not be called when circuit is open
}

// Test 16: Circuit breaker auto-reset after timeout
TEST_F(TransactionRetryManagerTest, CircuitBreakerAutoReset) {
    config.reset_timeout_ms = 100; // Short timeout for testing
    TransactionRetryManager manager(config);
    
    // Open the circuit
    for (int i = 0; i < 15; i++) {
        manager.executeWithRetry([&]() -> Result<int> {
            return Result<int>::error(ErrorType::CONSTRAINT_VIOLATION, "Non-retryable");
        }, "test_operation");
    }
    
    EXPECT_EQ(manager.getHealthState(), HealthState::CIRCUIT_OPEN);
    
    // Wait for reset timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Should allow one attempt (circuit half-open)
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        return Result<int>::ok(42);
    }, "test_operation");
    
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(manager.getHealthState(), HealthState::HEALTHY);
}

// Test 17: Alert callback invocation
TEST_F(TransactionRetryManagerTest, AlertCallbackInvocation) {
    TransactionRetryManager manager(config);
    
    std::atomic<int> alert_count{0};
    manager.setAlertCallback([&](HealthState state, const std::string& message) {
        alert_count++;
    });
    
    // Trigger state transitions
    for (int i = 0; i < 5; i++) {
        manager.executeWithRetry([&]() -> Result<int> {
            return Result<int>::error(ErrorType::CONSTRAINT_VIOLATION, "Non-retryable");
        }, "test_operation");
    }
    
    EXPECT_GT(alert_count.load(), 0); // Should have received alerts
}

// Test 18: Different backoff strategies
TEST_F(TransactionRetryManagerTest, LinearBackoffStrategy) {
    config.backoff_strategy = BackoffStrategy::LINEAR;
    config.enable_jitter = false;
    TransactionRetryManager manager(config);
    
    std::vector<int64_t> delays;
    int call_count = 0;
    auto last_time = std::chrono::steady_clock::now();
    
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        auto current_time = std::chrono::steady_clock::now();
        if (call_count > 0) {
            auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time).count();
            delays.push_back(delay);
        }
        last_time = current_time;
        call_count++;
        
        if (call_count < 4) {
            return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
        }
        return Result<int>::ok(42);
    }, "test_operation");
    
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(delays.size(), 3);
    // Linear: 100ms, 200ms, 300ms (allowing tolerance)
    EXPECT_NEAR(delays[0], 100, 20);
    EXPECT_NEAR(delays[1], 200, 40);
    EXPECT_NEAR(delays[2], 300, 60);
}

// Test 19: Fixed backoff strategy
TEST_F(TransactionRetryManagerTest, FixedBackoffStrategy) {
    config.backoff_strategy = BackoffStrategy::FIXED;
    config.enable_jitter = false;
    TransactionRetryManager manager(config);
    
    std::vector<int64_t> delays;
    int call_count = 0;
    auto last_time = std::chrono::steady_clock::now();
    
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        auto current_time = std::chrono::steady_clock::now();
        if (call_count > 0) {
            auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time).count();
            delays.push_back(delay);
        }
        last_time = current_time;
        call_count++;
        
        if (call_count < 4) {
            return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
        }
        return Result<int>::ok(42);
    }, "test_operation");
    
    ASSERT_TRUE(result.is_ok());
    ASSERT_EQ(delays.size(), 3);
    // Fixed: 100ms, 100ms, 100ms (allowing tolerance)
    for (auto delay : delays) {
        EXPECT_NEAR(delay, 100, 20);
    }
}

// Test 20: Custom retry policies
TEST_F(TransactionRetryManagerTest, CustomRetryPolicies) {
    TransactionRetryManager manager(config);
    
    RetryPolicy custom_policy;
    custom_policy.max_attempts = 10;
    custom_policy.base_delay_ms = 50;
    
    int call_count = 0;
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        call_count++;
        if (call_count < 8) {
            return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
        }
        return Result<int>::ok(42);
    }, "test_operation", custom_policy);
    
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(call_count, 8);
}

// Test 21: Concurrent retry scenarios (thread-safe)
TEST_F(TransactionRetryManagerTest, ConcurrentRetryScenarios) {
    TransactionRetryManager manager(config);
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&, i]() {
            auto result = manager.executeWithRetry([&]() -> Result<int> {
                if (i % 2 == 0) {
                    return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
                }
                return Result<int>::ok(i);
            }, "test_operation");
            
            if (result.is_ok()) {
                success_count++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Half should succeed (odd indices)
    EXPECT_EQ(success_count.load(), num_threads / 2);
}

// Test 22: Error classification accuracy
TEST_F(TransactionRetryManagerTest, ErrorClassificationAccuracy) {
    TransactionRetryManager manager(config);
    
    // Retryable errors
    std::vector<ErrorType> retryable = {
        ErrorType::WRITE_CONFLICT,
        ErrorType::TIMEOUT,
        ErrorType::NETWORK_ERROR,
        ErrorType::RESOURCE_EXHAUSTED,
        ErrorType::SERVICE_UNAVAILABLE
    };
    
    for (auto error_type : retryable) {
        int call_count = 0;
        auto result = manager.executeWithRetry([&]() -> Result<int> {
            call_count++;
            if (call_count < 2) {
                return Result<int>::error(error_type, "Error");
            }
            return Result<int>::ok(42);
        }, "test_operation");
        
        EXPECT_TRUE(result.is_ok()) << "Should retry error type: " << static_cast<int>(error_type);
    }
}

// Test 23: Retry with different error types
TEST_F(TransactionRetryManagerTest, RetryWithDifferentErrorTypes) {
    TransactionRetryManager manager(config);
    
    int call_count = 0;
    auto result = manager.executeWithRetry([&]() -> Result<int> {
        call_count++;
        if (call_count == 1) {
            return Result<int>::error(ErrorType::TIMEOUT, "Timeout");
        } else if (call_count == 2) {
            return Result<int>::error(ErrorType::WRITE_CONFLICT, "Conflict");
        } else if (call_count == 3) {
            return Result<int>::error(ErrorType::NETWORK_ERROR, "Network");
        }
        return Result<int>::ok(42);
    }, "test_operation");
    
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(call_count, 4); // 3 different errors + 1 success
}

// Test 24: Statistics reset
TEST_F(TransactionRetryManagerTest, StatisticsReset) {
    TransactionRetryManager manager(config);
    
    // Generate some statistics
    for (int i = 0; i < 5; i++) {
        manager.executeWithRetry([&]() -> Result<int> {
            return Result<int>::ok(42);
        }, "test_operation");
    }
    
    const auto& stats_before = manager.getStatistics();
    EXPECT_GT(stats_before.total_operations, 0);
    
    manager.resetStatistics();
    
    const auto& stats_after = manager.getStatistics();
    EXPECT_EQ(stats_after.total_operations, 0);
    EXPECT_EQ(stats_after.successful_operations, 0);
}

// Test 25: Alert callback exception handling
TEST_F(TransactionRetryManagerTest, AlertCallbackExceptionHandling) {
    TransactionRetryManager manager(config);
    
    manager.setAlertCallback([](HealthState state, const std::string& message) {
        throw std::runtime_error("Alert callback exception");
    });
    
    // Should not crash even if callback throws
    EXPECT_NO_THROW({
        for (int i = 0; i < 15; i++) {
            manager.executeWithRetry([&]() -> Result<int> {
                return Result<int>::error(ErrorType::CONSTRAINT_VIOLATION, "Non-retryable");
            }, "test_operation");
        }
    });
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
