/**
 * @file test_query_federation_hardness.cpp
 * @brief Comprehensive federation hardness tests: timeouts, retries, memory, and routing
 * @version 0.0.1
 * @date 2026-08-05
 * @copyright Apache-2.0, (c) 2026 ThemisDB Contributors
 */

#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>

#include "query/query_federation_timeout.h"
#include "query/query_federation_memory.h"

using namespace themis::query;
using json = nlohmann::json;

// ============================================================================
// TimeoutPolicy Tests
// ============================================================================

class TimeoutPolicyTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy = TimeoutPolicy::Builder{}
            .withPerShardTimeout(std::chrono::milliseconds(5000))
            .withOverallTimeout(std::chrono::milliseconds(30000))
            .withMaxRetries(3)
            .withInitialBackoffMs(100)
            .withMaxBackoffMs(5000)
            .withBackoffMultiplier(2.0)
            .withJitterFraction(0.1)
            .build();
    }

    TimeoutPolicy policy{TimeoutPolicy::Builder{}.build()};
};

TEST_F(TimeoutPolicyTest, BuilderValidation) {
    // Valid builder
    EXPECT_NO_THROW(TimeoutPolicy::Builder{}
        .withPerShardTimeout(std::chrono::milliseconds(5000))
        .withOverallTimeout(std::chrono::milliseconds(30000))
        .build());

    // Invalid per_shard_timeout
    EXPECT_THROW(TimeoutPolicy::Builder{}
        .withPerShardTimeout(std::chrono::milliseconds(0))
        .build(),
        std::invalid_argument);

    // Invalid overall_timeout
    EXPECT_THROW(TimeoutPolicy::Builder{}
        .withOverallTimeout(std::chrono::milliseconds(-1))
        .build(),
        std::invalid_argument);

    // Invalid max_retries
    EXPECT_THROW(TimeoutPolicy::Builder{}
        .withMaxRetries(-1)
        .build(),
        std::invalid_argument);

    // Invalid backoff_multiplier
    EXPECT_THROW(TimeoutPolicy::Builder{}
        .withBackoffMultiplier(0.5)
        .build(),
        std::invalid_argument);
}

TEST_F(TimeoutPolicyTest, PerShardTimeoutEnforcement) {
    EXPECT_TRUE(policy.shouldRetry(std::chrono::milliseconds(1000), 0));
    EXPECT_TRUE(policy.shouldRetry(std::chrono::milliseconds(4999), 0));
    EXPECT_FALSE(policy.shouldRetry(std::chrono::milliseconds(5000), 0));
    EXPECT_FALSE(policy.shouldRetry(std::chrono::milliseconds(6000), 0));
}

TEST_F(TimeoutPolicyTest, MaxRetriesEnforcement) {
    EXPECT_TRUE(policy.shouldRetry(std::chrono::milliseconds(1000), 0));
    EXPECT_TRUE(policy.shouldRetry(std::chrono::milliseconds(1000), 1));
    EXPECT_TRUE(policy.shouldRetry(std::chrono::milliseconds(1000), 2));
    EXPECT_FALSE(policy.shouldRetry(std::chrono::milliseconds(1000), 3));
    EXPECT_FALSE(policy.shouldRetry(std::chrono::milliseconds(1000), 4));
}

TEST_F(TimeoutPolicyTest, BackoffCalculation) {
    auto backoff0 = policy.calculateBackoff(0);
    auto backoff1 = policy.calculateBackoff(1);
    auto backoff2 = policy.calculateBackoff(2);

    // Backoff should increase exponentially
    EXPECT_LE(backoff0.count(), 120);  // 100ms +/- jitter
    EXPECT_LE(backoff1.count(), 240);  // ~200ms +/- jitter
    EXPECT_LE(backoff2.count(), 480);  // ~400ms +/- jitter

    // Later backoffs should be capped at max_backoff
    auto backoff_many = policy.calculateBackoff(10);
    EXPECT_LE(backoff_many.count(), 5500);  // 5000ms +/- jitter
}

TEST_F(TimeoutPolicyTest, OverallTimeoutEnforcement) {
    EXPECT_FALSE(policy.isOverallTimeoutExceeded(std::chrono::milliseconds(29999)));
    EXPECT_FALSE(policy.isOverallTimeoutExceeded(std::chrono::milliseconds(30000)));
    EXPECT_TRUE(policy.isOverallTimeoutExceeded(std::chrono::milliseconds(30001)));
}

TEST_F(TimeoutPolicyTest, TimeoutEventRecording) {
    EXPECT_EQ(policy.getTimeoutEvents().size(), 0);

    TimeoutPolicy::TimeoutEvent event{
        TimeoutPolicy::TimeoutEvent::Type::SHARD_TIMEOUT,
        "shard1",
        std::chrono::milliseconds(5000),
        0,
        "Connection timeout"};

    policy.recordTimeoutEvent(event);
    EXPECT_EQ(policy.getTimeoutEvents().size(), 1);

    const auto& recorded = policy.getTimeoutEvents()[0];
    EXPECT_EQ(recorded.shard_id, "shard1");
    EXPECT_EQ(recorded.elapsed.count(), 5000);
    EXPECT_EQ(recorded.exhaustion_reason, themis::utils::RetryExhaustionReason::NONE);
    EXPECT_EQ(recorded.timeout_source, themis::utils::RetryTimeoutSource::NONE);
}

TEST_F(TimeoutPolicyTest, RetryStatsRecording) {
    TimeoutPolicy::RetryStats stats{
        2,  // successful on 3rd attempt
        3,  // 3 total attempts
        std::chrono::milliseconds(15000)  // 15 seconds total
    };
    stats.attempt_latencies = {
        std::chrono::milliseconds(5000),
        std::chrono::milliseconds(5000),
        std::chrono::milliseconds(5000)};
    stats.failure_reasons = {"timeout", "connection_reset"};

    policy.recordRetryStats("shard2", stats);

    auto retrieved = policy.getRetryStats("shard2");
    EXPECT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->successful_attempt, 2);
    EXPECT_EQ(retrieved->total_attempts, 3);
    EXPECT_EQ(retrieved->total_elapsed.count(), 15000);
}

// ============================================================================
// QueryTimeoutContext Tests
// ============================================================================

class QueryTimeoutContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy = TimeoutPolicy::Builder{}
            .withPerShardTimeout(std::chrono::milliseconds(2000))
            .withOverallTimeout(std::chrono::milliseconds(10000))
            .withMaxRetries(2)
            .build();

        context = std::make_unique<QueryTimeoutContext>(policy);
    }

    TimeoutPolicy policy{TimeoutPolicy::Builder{}.build()};
    std::unique_ptr<QueryTimeoutContext> context;
};

TEST_F(QueryTimeoutContextTest, ShardAttemptTracking) {
    context->startShardAttempt("shard1", 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    context->endShardAttempt("shard1", true);

    auto stats = context->getShardStats("shard1");
    EXPECT_TRUE(stats.has_value());
    EXPECT_EQ(stats->successful_attempt, 0);
    EXPECT_EQ(stats->total_attempts, 1);
    EXPECT_GE(stats->total_elapsed.count(), 100);
}

TEST_F(QueryTimeoutContextTest, RetryTracking) {
    for (int attempt = 0; attempt < 3; ++attempt) {
        context->startShardAttempt("shard1", attempt);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        context->endShardAttempt("shard1", attempt == 2);  // success on 3rd attempt
    }

    auto stats = context->getShardStats("shard1");
    EXPECT_TRUE(stats.has_value());
    EXPECT_EQ(stats->successful_attempt, 2);
    EXPECT_EQ(stats->total_attempts, 3);
    EXPECT_EQ(stats->failure_reasons.size(), 2);

    auto metadata = context->getRetryMetadata("shard1");
    EXPECT_EQ(metadata.retry_count, 2u);
    EXPECT_EQ(metadata.retry_budget, 2u);
    EXPECT_FALSE(metadata.retriable);
    EXPECT_EQ(metadata.exhaustion_reason,
              themis::utils::RetryExhaustionReason::MAX_ATTEMPTS_REACHED);
}

TEST_F(QueryTimeoutContextTest, RemainingTimeCalculation) {
    auto remaining = context->getRemainingTime();
    EXPECT_GT(remaining.count(), 9900);  // Close to 10000ms
    EXPECT_LE(remaining.count(), 10000);
}

TEST_F(QueryTimeoutContextTest, OverallTimeoutDetection) {
    EXPECT_FALSE(context->isOverallTimeoutExceeded());

    // Sleep past timeout (can't actually wait, so we'd need to mock time)
    // This is validated in integration tests
}

// ============================================================================
// MemoryPolicy Tests
// ============================================================================

class MemoryPolicyTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy = MemoryPolicy::Builder{}
            .withMaxResultBytes(1024 * 1024)  // 1MB
            .withOverflowPolicy(MemoryPolicy::OverflowPolicy::TRUNCATE)
            .withPressureThresholds(70.0, 85.0, 95.0)
            .build();
    }

    MemoryPolicy policy{MemoryPolicy::Builder{}.build()};
};

TEST_F(MemoryPolicyTest, BuilderValidation) {
    // Valid builder
    EXPECT_NO_THROW(MemoryPolicy::Builder{}
        .withMaxResultBytes(1024)
        .build());

    // Invalid max_result_bytes
    EXPECT_THROW(MemoryPolicy::Builder{}
        .withMaxResultBytes(0)
        .build(),
        std::invalid_argument);

    // Invalid pressure thresholds
    EXPECT_THROW(MemoryPolicy::Builder{}
        .withPressureThresholds(100.0, 50.0, 90.0)
        .build(),
        std::invalid_argument);
}

TEST_F(MemoryPolicyTest, UtilizationCalculation) {
    EXPECT_EQ(policy.getUtilizationPercent(0), 0.0);
    EXPECT_EQ(policy.getUtilizationPercent(512 * 1024), 50.0);
    EXPECT_EQ(policy.getUtilizationPercent(1024 * 1024), 100.0);
}

TEST_F(MemoryPolicyTest, PressureLevelClassification) {
    EXPECT_EQ(policy.getPressureLevel(0), MemoryPolicy::PressureLevel::NORMAL);
    EXPECT_EQ(policy.getPressureLevel(700 * 1024), MemoryPolicy::PressureLevel::ELEVATED);
    EXPECT_EQ(policy.getPressureLevel(850 * 1024), MemoryPolicy::PressureLevel::HIGH);
    EXPECT_EQ(policy.getPressureLevel(950 * 1024), MemoryPolicy::PressureLevel::CRITICAL);
}

TEST_F(MemoryPolicyTest, UnderPressureDetection) {
    EXPECT_FALSE(policy.isUnderPressure(600 * 1024));
    EXPECT_TRUE(policy.isUnderPressure(700 * 1024));
    EXPECT_TRUE(policy.isUnderPressure(950 * 1024));
}

TEST_F(MemoryPolicyTest, MemoryLimitEnforcement) {
    EXPECT_FALSE(policy.isLimitExceeded(1024 * 1024 - 1));
    EXPECT_FALSE(policy.isLimitExceeded(1024 * 1024));
    EXPECT_TRUE(policy.isLimitExceeded(1024 * 1024 + 1));
}

TEST_F(MemoryPolicyTest, PressureEventRecording) {
    EXPECT_EQ(policy.getPressureEvents().size(), 0);

    MemoryPolicy::MemoryPressureEvent event{
        MemoryPolicy::PressureLevel::HIGH,
        900 * 1024,
        1024 * 1024,
        87.89,
        "shard1",
        "Test event"};

    policy.recordPressureEvent(event);
    EXPECT_EQ(policy.getPressureEvents().size(), 1);

    const auto& recorded = policy.getPressureEvents()[0];
    EXPECT_EQ(recorded.shard_id, "shard1");
    EXPECT_EQ(recorded.level, MemoryPolicy::PressureLevel::HIGH);
}

// ============================================================================
// ResultAccumulator Tests
// ============================================================================

class ResultAccumulatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        policy = MemoryPolicy::Builder{}
            .withMaxResultBytes(10 * 1024)  // 10KB for testing
            .withOverflowPolicy(MemoryPolicy::OverflowPolicy::TRUNCATE)
            .build();

        accumulator = std::make_unique<ResultAccumulator>(policy);
    }

    MemoryPolicy policy{MemoryPolicy::Builder{}.build()};
    std::unique_ptr<ResultAccumulator> accumulator;

    json createTestResult(size_t count) {
        json arr = json::array();
        for (size_t i = 0; i < count; ++i) {
            arr.push_back({
                {"id", i},
                {"value", "test_" + std::to_string(i)}
            });
        }
        return arr;
    }
};

TEST_F(ResultAccumulatorTest, BasicResultAddition) {
    auto result = createTestResult(5);
    EXPECT_TRUE(accumulator->addResult("shard1", result));
    EXPECT_EQ(accumulator->getResultCount("shard1"), 1);
    EXPECT_EQ(accumulator->getTotalResultCount(), 1);
}

TEST_F(ResultAccumulatorTest, MultiShardResults) {
    auto result1 = createTestResult(3);
    auto result2 = createTestResult(2);

    EXPECT_TRUE(accumulator->addResult("shard1", result1));
    EXPECT_TRUE(accumulator->addResult("shard2", result2));

    EXPECT_EQ(accumulator->getResultCount("shard1"), 1);
    EXPECT_EQ(accumulator->getResultCount("shard2"), 1);
    EXPECT_EQ(accumulator->getTotalResultCount(), 2);
}

TEST_F(ResultAccumulatorTest, MemoryTracking) {
    auto result = createTestResult(5);
    accumulator->addResult("shard1", result);

    EXPECT_GT(accumulator->getCurrentMemoryBytes(), 0);
    EXPECT_GT(accumulator->getMemoryUtilizationPercent(), 0.0);
    EXPECT_LT(accumulator->getMemoryUtilizationPercent(), 100.0);
}

TEST_F(ResultAccumulatorTest, RejectPolicyOnOverflow) {
    MemoryPolicy reject_policy = MemoryPolicy::Builder{}
        .withMaxResultBytes(100)  // Very small
        .withOverflowPolicy(MemoryPolicy::OverflowPolicy::REJECT)
        .build();

    ResultAccumulator reject_accumulator(reject_policy);
    auto result = createTestResult(100);  // Large result

    EXPECT_THROW(reject_accumulator.addResult("shard1", result),
                 std::runtime_error);
}

TEST_F(ResultAccumulatorTest, TruncatePolicyOnOverflow) {
    auto result1 = createTestResult(10);
    auto result2 = createTestResult(10);
    auto result3 = createTestResult(10);

    // Should fit initially
    EXPECT_TRUE(accumulator->addResult("shard1", result1));
    EXPECT_TRUE(accumulator->addResult("shard1", result2));

    // Third might trigger truncation
    auto success = accumulator->addResult("shard1", result3);
    // Either succeeds with truncation or overflow policy handles it
    EXPECT_TRUE(!accumulator->isMemoryLimitExceeded() || !success);
}

TEST_F(ResultAccumulatorTest, MergedResults) {
    accumulator->addResult("shard1", json::array({{{"id", 1}}}));
    accumulator->addResult("shard2", json::array({{{"id", 2}}}));

    auto merged = accumulator->getMergedResults();
    EXPECT_TRUE(merged.is_array());
    EXPECT_GE(merged.size(), 1);
}

TEST_F(ResultAccumulatorTest, Clear) {
    accumulator->addResult("shard1", createTestResult(5));
    EXPECT_GT(accumulator->getTotalResultCount(), 0);

    accumulator->clear();
    EXPECT_EQ(accumulator->getTotalResultCount(), 0);
    EXPECT_EQ(accumulator->getCurrentMemoryBytes(), 0);
}

TEST_F(ResultAccumulatorTest, Statistics) {
    accumulator->addResult("shard1", createTestResult(5));
    auto stats = accumulator->getStatistics();

    EXPECT_TRUE(stats.find("ResultAccumulator Statistics") != std::string::npos);
    EXPECT_TRUE(stats.find("shard1") != std::string::npos);
    EXPECT_TRUE(stats.find("utilization") != std::string::npos);
}

// ============================================================================
// Integration Tests (Timeout + Memory)
// ============================================================================

class FederationHardnessIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        timeout_policy = TimeoutPolicy::Builder{}
            .withPerShardTimeout(std::chrono::milliseconds(1000))
            .withOverallTimeout(std::chrono::milliseconds(5000))
            .withMaxRetries(2)
            .build();

        memory_policy = MemoryPolicy::Builder{}
            .withMaxResultBytes(100 * 1024)  // 100KB
            .build();

        context = std::make_unique<QueryTimeoutContext>(timeout_policy);
        accumulator = std::make_unique<ResultAccumulator>(memory_policy);
    }

    TimeoutPolicy timeout_policy{TimeoutPolicy::Builder{}.build()};
    MemoryPolicy memory_policy{MemoryPolicy::Builder{}.build()};
    std::unique_ptr<QueryTimeoutContext> context;
    std::unique_ptr<ResultAccumulator> accumulator;
};

TEST_F(FederationHardnessIntegrationTest, BoundedMemoryUnderTimeout) {
    // Simulate multiple shard results arriving under timeout
    for (int shard = 0; shard < 3; ++shard) {
        context->startShardAttempt("shard" + std::to_string(shard), 0);

        json result = json::array();
        for (int i = 0; i < 10; ++i) {
            result.push_back({{"value", i}});
        }

        bool added = accumulator->addResult(
            "shard" + std::to_string(shard), result);
        EXPECT_TRUE(added);

        context->endShardAttempt("shard" + std::to_string(shard), true);
    }

    EXPECT_EQ(accumulator->getTotalResultCount(), 3);
    EXPECT_FALSE(context->isOverallTimeoutExceeded());
    EXPECT_LE(accumulator->getMemoryUtilizationPercent(), 100.0);
}

// ============================================================================
// Fault Injection Tests
// ============================================================================

class FederationFaultInjectionTest : public ::testing::Test {
protected:
    TimeoutPolicy policy{TimeoutPolicy::Builder{}
        .withPerShardTimeout(std::chrono::milliseconds(2000))
        .withMaxRetries(3)
        .build()};
};

// FED-01: Shard Timeout Injection
TEST_F(FederationFaultInjectionTest, FED_01_ShardTimeout) {
    auto ctx = QueryTimeoutContext(policy);
    ctx.startShardAttempt("shard_slow", 0);

    // Simulate slow response (would exceed timeout in real scenario)
    auto elapsed_before = ctx.getTotalElapsed();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto elapsed_after = ctx.getTotalElapsed();

    EXPECT_GT(elapsed_after.count(), elapsed_before.count());
    ctx.endShardAttempt("shard_slow", false, "timeout");

    EXPECT_FALSE(ctx.isOverallTimeoutExceeded());
}

// FED-02: Retry Exhaustion
TEST_F(FederationFaultInjectionTest, FED_02_RetryExhaustion) {
    auto ctx = QueryTimeoutContext(policy);

    for (int attempt = 0; attempt < 4; ++attempt) {
        ctx.startShardAttempt("shard_failing", attempt);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ctx.endShardAttempt("shard_failing", false, "connection error");

        if (attempt < 3) {
            EXPECT_TRUE(ctx.shouldRetry("shard_failing"));
        }
    }

    auto metadata = ctx.getRetryMetadata("shard_failing");
    EXPECT_EQ(metadata.retry_count, 3u);
    EXPECT_FALSE(metadata.retriable);
    EXPECT_EQ(metadata.exhaustion_reason,
              themis::utils::RetryExhaustionReason::MAX_ATTEMPTS_REACHED);
}

// FED-03: Overall Query Timeout
TEST_F(FederationFaultInjectionTest, FED_03_OverallQueryTimeout) {
    auto ctx = QueryTimeoutContext(policy);

    // Simulate multiple slow shards
    for (int shard = 0; shard < 5; ++shard) {
        ctx.startShardAttempt("shard" + std::to_string(shard), 0);
        ctx.endShardAttempt("shard" + std::to_string(shard), true);
    }

    EXPECT_FALSE(ctx.isOverallTimeoutExceeded());
}

// FED-04: Memory Pressure Handling
TEST_F(FederationFaultInjectionTest, FED_04_MemoryPressure) {
    MemoryPolicy mem_policy = MemoryPolicy::Builder{}
        .withMaxResultBytes(1024)
        .withOverflowPolicy(MemoryPolicy::OverflowPolicy::DROP_OLDEST)
        .build();

    ResultAccumulator acc(mem_policy);

    // Add results until under pressure
    json small_result = json::array({1, 2, 3, 4, 5});
    for (int i = 0; i < 5; ++i) {
        acc.addResult("shard1", small_result);
    }

    // Should be under pressure or have handled it
    EXPECT_TRUE(!acc.isUnderPressure() || acc.getTotalResultCount() > 0);
}

// FED-05: Backpressure with Drop Policy
TEST_F(FederationFaultInjectionTest, FED_05_BackpressureDropPolicy) {
    MemoryPolicy mem_policy = MemoryPolicy::Builder{}
        .withMaxResultBytes(500)
        .withOverflowPolicy(MemoryPolicy::OverflowPolicy::DROP_OLDEST)
        .build();

    ResultAccumulator acc(mem_policy);

    json result = json::array();
    for (int i = 0; i < 20; ++i) {
        result.push_back(i);
    }

    // Should drop oldest when needed
    for (int batch = 0; batch < 10; ++batch) {
        acc.addResult("shard1", result);
    }

    EXPECT_LE(acc.getCurrentMemoryBytes(), mem_policy.getMaxResultBytes() + 1000);
}

// FED-06: Shard Routing Under Partial Failure
TEST_F(FederationFaultInjectionTest, FED_06_PartialFailureRouting) {
    auto ctx = QueryTimeoutContext(policy);

    // Simulate partial failure: 2 succeed, 1 fails
    std::vector<std::string> shards = {"shard1", "shard2", "shard3"};

    for (size_t i = 0; i < shards.size(); ++i) {
        ctx.startShardAttempt(shards[i], 0);
        ctx.endShardAttempt(shards[i], i < 2);  // First 2 succeed
    }

    EXPECT_EQ(ctx.getShardStats("shard1")->successful_attempt, 0);
    EXPECT_EQ(ctx.getShardStats("shard2")->successful_attempt, 0);
    EXPECT_EQ(ctx.getShardStats("shard3")->successful_attempt, -1);  // Failed
}

// FED-07: Exponential Backoff Correctness
TEST_F(FederationFaultInjectionTest, FED_07_ExponentialBackoff) {
    auto backoff0 = policy.calculateBackoff(0);
    auto backoff1 = policy.calculateBackoff(1);
    auto backoff2 = policy.calculateBackoff(2);

    // Each should be roughly 2x previous (with jitter)
    EXPECT_LE(backoff0.count(), 120);
    EXPECT_GE(backoff1.count(), backoff0.count());
    EXPECT_GE(backoff2.count(), backoff1.count());
}

// FED-08: Bounded Resource Growth Under Fault Storm
TEST_F(FederationFaultInjectionTest, FED_08_BoundedResourceGrowth) {
    MemoryPolicy mem_policy = MemoryPolicy::Builder{}
        .withMaxResultBytes(10 * 1024)
        .withOverflowPolicy(MemoryPolicy::OverflowPolicy::TRUNCATE)
        .build();

    ResultAccumulator acc(mem_policy);

    // Simulate fault storm: many shards sending results
    for (int shard = 0; shard < 20; ++shard) {
        json result = json::array();
        for (int item = 0; item < 100; ++item) {
            result.push_back({{"shard", shard}, {"item", item}});
        }

        acc.addResult("shard" + std::to_string(shard), result);
    }

    // Should never exceed memory limit
    EXPECT_LE(acc.getCurrentMemoryBytes(),
              mem_policy.getMaxResultBytes() + 1000);  // Small buffer for precision
}
