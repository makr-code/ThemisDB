// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/retry_strategy.h"
#include <gtest/gtest.h>
#include <atomic>

using namespace themisdb::sharding;

class RetryStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        attempt_count_ = 0;
    }
    
    void TearDown() override {
        // Test cleanup
    }
    
    std::atomic<int> attempt_count_{0};
};

// Test NO_RETRY strategy
TEST_F(RetryStrategyTest, NoRetry) {
    RetryConfig config;
    config.strategy = RetryStrategy::NO_RETRY;
    config.max_retries = 3;
    
    auto result = executeWithRetry([this]() -> Result<int> {
        attempt_count_++;
        return Err<int>(DistributedSystemError::PARTICIPANT_TIMEOUT, "Timeout");
    }, config);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(attempt_count_.load(), 1);  // Only tried once
}

// Test IMMEDIATE retry strategy
TEST_F(RetryStrategyTest, ImmediateRetry) {
    RetryConfig config;
    config.strategy = RetryStrategy::IMMEDIATE;
    config.max_retries = 3;
    config.jitter = false;
    
    auto result = executeWithRetry([this]() -> Result<int> {
        attempt_count_++;
        if (attempt_count_.load() < 3) {
            return Err<int>(DistributedSystemError::PARTICIPANT_TIMEOUT, "Timeout");
        }
        return Ok(42);
    }, config);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(*result, 42);
    EXPECT_EQ(attempt_count_.load(), 3);
}

// Test EXPONENTIAL_BACKOFF strategy
TEST_F(RetryStrategyTest, ExponentialBackoff) {
    RetryConfig config;
    config.strategy = RetryStrategy::EXPONENTIAL_BACKOFF;
    config.max_retries = 3;
    config.initial_delay = std::chrono::milliseconds(10);
    config.backoff_multiplier = 2.0;
    config.jitter = false;
    
    auto start = std::chrono::steady_clock::now();
    
    auto result = executeWithRetry([this]() -> Result<void> {
        attempt_count_++;
        return Err(DistributedSystemError::PARTICIPANT_TIMEOUT, "Timeout");
    }, config);
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(attempt_count_.load(), 3);
    
    // Should have waited approximately: 10ms + 20ms + 40ms = 70ms
    // Allow some tolerance for timing variations
    EXPECT_GE(elapsed, std::chrono::milliseconds(50));
}

// Test retry with eventual success
TEST_F(RetryStrategyTest, EventualSuccess) {
    RetryConfig config;
    config.strategy = RetryStrategy::IMMEDIATE;
    config.max_retries = 5;
    config.jitter = false;
    
    auto result = executeWithRetry([this]() -> Result<std::string> {
        attempt_count_++;
        if (attempt_count_.load() < 3) {
            return Err<std::string>(DistributedSystemError::NETWORK_UNSTABLE, "Network unstable");
        }
        return Ok(std::string("success"));
    }, config);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(*result, "success");
    EXPECT_EQ(attempt_count_.load(), 3);
}

// Test non-retriable error stops retry
TEST_F(RetryStrategyTest, NonRetriableError) {
    RetryConfig config;
    config.strategy = RetryStrategy::IMMEDIATE;
    config.max_retries = 5;
    
    auto result = executeWithRetry([this]() -> Result<int> {
        attempt_count_++;
        return Err<int>(DistributedSystemError::INVALID_ARGUMENT, "Invalid argument");
    }, config);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(attempt_count_.load(), 1);  // Should not retry non-retriable errors
}

// Test max retries limit
TEST_F(RetryStrategyTest, MaxRetriesLimit) {
    RetryConfig config;
    config.strategy = RetryStrategy::IMMEDIATE;
    config.max_retries = 3;
    config.jitter = false;
    
    auto result = executeWithRetry([this]() -> Result<void> {
        attempt_count_++;
        return Err(DistributedSystemError::PARTICIPANT_TIMEOUT, "Timeout");
    }, config);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(attempt_count_.load(), 3);
}

// Test LINEAR_BACKOFF strategy
TEST_F(RetryStrategyTest, LinearBackoff) {
    RetryConfig config;
    config.strategy = RetryStrategy::LINEAR_BACKOFF;
    config.max_retries = 3;
    config.initial_delay = std::chrono::milliseconds(10);
    config.jitter = false;
    
    auto start = std::chrono::steady_clock::now();
    
    auto result = executeWithRetry([this]() -> Result<void> {
        attempt_count_++;
        return Err(DistributedSystemError::REPLICA_UNAVAILABLE, "Replica unavailable");
    }, config);
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(attempt_count_.load(), 3);
    
    // Should have waited approximately: 10ms + 20ms + 30ms = 60ms
    EXPECT_GE(elapsed, std::chrono::milliseconds(40));
}

// Test delay calculation
TEST_F(RetryStrategyTest, CalculateRetryDelay) {
    RetryConfig config;
    config.initial_delay = std::chrono::milliseconds(100);
    config.backoff_multiplier = 2.0;
    config.max_delay = std::chrono::milliseconds(1000);
    config.jitter = false;
    
    // Exponential backoff
    config.strategy = RetryStrategy::EXPONENTIAL_BACKOFF;
    auto delay1 = calculateRetryDelay(config, 1);
    auto delay2 = calculateRetryDelay(config, 2);
    auto delay3 = calculateRetryDelay(config, 3);
    
    EXPECT_EQ(delay1.count(), 100);   // 100 * 2^0
    EXPECT_EQ(delay2.count(), 200);   // 100 * 2^1
    EXPECT_EQ(delay3.count(), 400);   // 100 * 2^2
    
    // Linear backoff
    config.strategy = RetryStrategy::LINEAR_BACKOFF;
    delay1 = calculateRetryDelay(config, 1);
    delay2 = calculateRetryDelay(config, 2);
    delay3 = calculateRetryDelay(config, 3);
    
    EXPECT_EQ(delay1.count(), 100);   // 100 * 1
    EXPECT_EQ(delay2.count(), 200);   // 100 * 2
    EXPECT_EQ(delay3.count(), 300);   // 100 * 3
}

// Test max delay cap
TEST_F(RetryStrategyTest, MaxDelayCap) {
    RetryConfig config;
    config.strategy = RetryStrategy::EXPONENTIAL_BACKOFF;
    config.initial_delay = std::chrono::milliseconds(100);
    config.max_delay = std::chrono::milliseconds(500);
    config.backoff_multiplier = 2.0;
    config.jitter = false;
    
    auto delay1 = calculateRetryDelay(config, 1);
    auto delay2 = calculateRetryDelay(config, 2);
    auto delay3 = calculateRetryDelay(config, 3);
    auto delay4 = calculateRetryDelay(config, 4);
    auto delay5 = calculateRetryDelay(config, 5);
    
    EXPECT_EQ(delay1.count(), 100);   // 100 * 2^0 = 100
    EXPECT_EQ(delay2.count(), 200);   // 100 * 2^1 = 200
    EXPECT_EQ(delay3.count(), 400);   // 100 * 2^2 = 400
    EXPECT_EQ(delay4.count(), 500);   // 100 * 2^3 = 800, capped at 500
    EXPECT_EQ(delay5.count(), 500);   // 100 * 2^4 = 1600, capped at 500
}

// Test jitter addition
TEST_F(RetryStrategyTest, JitterAddition) {
    auto delay = std::chrono::milliseconds(1000);
    
    // Run multiple times to ensure jitter is actually random
    bool has_variation = false;
    auto first_jittered = addJitter(delay, 0.1);
    
    for (int i = 0; i < 10; i++) {
        auto jittered = addJitter(delay, 0.1);
        
        // Should be within ±10% of original
        EXPECT_GE(jittered.count(), 900);
        EXPECT_LE(jittered.count(), 1100);
        
        if (jittered != first_jittered) {
            has_variation = true;
        }
    }
    
    // At least some variation should occur (statistically very likely)
    EXPECT_TRUE(has_variation);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
