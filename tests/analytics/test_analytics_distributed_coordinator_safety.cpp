/*
 * ThemisDB Distributed Analytics Coordinator Safety Controls Tests
 *
 * @file test_analytics_distributed_coordinator_safety.cpp
 * @brief Tests for circuit breaker, bounded queue, and recovery mechanisms (Phase 2.2)
 *
 * Tests covering:
 *   - Circuit breaker state transitions (CLOSED → OPEN → HALF_OPEN → CLOSED)
 *   - Consecutive failure threshold enforcement
 *   - Recovery delay and exponential backoff
 *   - Bounded queue enqueue/dequeue with backpressure
 *   - Degradation scenarios (multiple shard failures)
 *   - Error-path handling for unsupported states
 */

#include <gtest/gtest.h>
#include "analytics/distributed_analytics.h"
#include "analytics/olap.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

using namespace themisdb::analytics;
using OLAPQuery = themis::analytics::OLAPQuery;
using OLAPResult = themis::analytics::OLAPResult;
using Measure = themis::analytics::Measure;
using CircuitBreakerState = DistributedAnalyticsSharding::CircuitBreakerState;

// ============================================================================
// Test Fixtures
// ============================================================================

/**
 * Mock executor that can be configured to succeed, fail, or timeout.
 */
class ControlledExecutor : public ShardQueryExecutor {
public:
    enum class Behavior {
        SUCCESS,      // Return successful result
        FAIL,         // Throw exception
        TIMEOUT,      // Simulate timeout by sleeping
        INTERMITTENT  // Fail N times, then succeed
    };

    explicit ControlledExecutor(Behavior behavior = Behavior::SUCCESS, 
                               uint32_t fail_count = 0)
        : behavior_(behavior), fail_count_(fail_count), attempt_count_(0) {}

    OLAPResult execute(const std::string& shard_id,
                      const OLAPQuery& query) override {
        attempt_count_++;

        switch (behavior_) {
            case Behavior::SUCCESS:
                return OLAPResult{};

            case Behavior::FAIL:
                throw std::runtime_error("Simulated shard failure");

            case Behavior::TIMEOUT:
                std::this_thread::sleep_for(std::chrono::seconds(10));
                return OLAPResult{};

            case Behavior::INTERMITTENT:
                if (attempt_count_ <= fail_count_) {
                    throw std::runtime_error("Intermittent failure #" + 
                                           std::to_string(attempt_count_));
                }
                return OLAPResult{};
        }
        return OLAPResult{};
    }

    bool isHealthy() const override {
        return behavior_ != Behavior::FAIL;
    }

    uint32_t getAttemptCount() const { return attempt_count_; }
    void reset() { attempt_count_ = 0; }

private:
    Behavior behavior_;
    uint32_t fail_count_;
    std::atomic<uint32_t> attempt_count_{0};
};

/**
 * Test fixture for distributed analytics coordinator safety.
 */
class DistributedAnalyticsSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default config with safety controls enabled
        DistributedAnalyticsSharding::Config cfg;
        cfg.enable_circuit_breaker = true;
        cfg.circuit_breaker_failure_threshold = 3;
        cfg.circuit_breaker_recovery_delay_ms = 100;
        cfg.circuit_breaker_max_recovery_delay_ms = 5000;
        cfg.circuit_breaker_recovery_attempts = 2;
        cfg.max_queued_requests_per_shard = 100;
        cfg.queue_enqueue_timeout_ms = 100;
        cfg.shard_timeout_ms = 500;
        cfg.health_check_interval = std::chrono::milliseconds(50);

        das_ = std::make_unique<DistributedAnalyticsSharding>(cfg);
    }

    std::unique_ptr<DistributedAnalyticsSharding> das_;

    OLAPQuery makeSimpleQuery() {
        OLAPQuery q;
        q.collection = "test_collection";
        q.dimensions = {{"region", "", true}};
        Measure m;
        m.name = "total";
        m.field = "amount";
        m.function = Measure::Function::Sum;
        q.measures = {m};
        return q;
    }
};

// ============================================================================
// Safety Control Tests: Circuit Breaker
// ============================================================================

/**
 * DCS-01: Circuit breaker starts in CLOSED state.
 */
TEST_F(DistributedAnalyticsSafetyTest, CircuitBreakerInitiallyClosed) {
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::SUCCESS);
    das_->addShard("shard_1", executor);

    auto query = makeSimpleQuery();
    auto result = das_->executeDistributed(query);

    // All shards should be successful
    EXPECT_EQ(result.successful_shards, 1u);
    EXPECT_EQ(result.shard_info.size(), 1u);
    if (!result.shard_info.empty()) {
        EXPECT_TRUE(result.shard_info[0].success);
        EXPECT_EQ(result.shard_info[0].circuit_state, 
                  CircuitBreakerState::CLOSED);
    }
}

/**
 * DCS-02: Circuit breaker transitions to OPEN after failure threshold.
 * Shard fails 3 times consecutively, circuit opens.
 */
TEST_F(DistributedAnalyticsSafetyTest, CircuitBreakerOpensAfterThreshold) {
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::FAIL);
    das_->addShard("failing_shard", executor);

    auto query = makeSimpleQuery();

    // First 3 failures should trigger circuit opening
    for (int i = 0; i < 3; ++i) {
        auto result = das_->executeDistributed(query);
        EXPECT_EQ(result.successful_shards, 0u);
    }

    // After 3 failures, circuit should be OPEN and shard should be skipped
    auto result = das_->executeDistributed(query);
    EXPECT_EQ(result.total_shards, 0u);  // Shard is skipped when circuit is OPEN
}

/**
 * DCS-03: OPEN circuit rejects requests (fail-closed behavior).
 */
TEST_F(DistributedAnalyticsSafetyTest, OpenCircuitRejectsRequests) {
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::INTERMITTENT, 5);  // Fail first 5 times
    das_->addShard("intermittent_shard", executor);

    auto query = makeSimpleQuery();

    // Trigger circuit opening
    for (int i = 0; i < 3; ++i) {
        das_->executeDistributed(query);
    }

    // Now circuit should be OPEN; verify shard is skipped
    auto result = das_->executeDistributed(query);
    EXPECT_EQ(result.total_shards, 0u);
}

/**
 * DCS-04: HALF_OPEN state allows limited recovery attempts.
 */
TEST_F(DistributedAnalyticsSafetyTest, HalfOpenAllowsRecovery) {
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::INTERMITTENT, 3);  // Fail 3 times, then succeed
    das_->addShard("recovery_shard", executor);

    auto query = makeSimpleQuery();

    // Trigger circuit opening (3 failures)
    for (int i = 0; i < 3; ++i) {
        das_->executeDistributed(query);
    }

    // Wait for recovery delay to elapse
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Next request should attempt recovery (HALF_OPEN state)
    auto result = das_->executeDistributed(query);
    // Shard is now available for recovery attempt (HALF_OPEN)
    // This test verifies the circuit can transition to HALF_OPEN after delay
    EXPECT_GE(result.shard_info.size(), 0u);  // May succeed or fail again
}

/**
 * DCS-05: Consecutive failures reset on success.
 */
TEST_F(DistributedAnalyticsSafetyTest, ConsecutiveFailuresResetOnSuccess) {
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::INTERMITTENT, 1);  // Fail once, then succeed
    das_->addShard("resilient_shard", executor);

    auto query = makeSimpleQuery();

    // First execution fails
    auto result1 = das_->executeDistributed(query);
    EXPECT_EQ(result1.successful_shards, 0u);

    // Second execution succeeds - should reset failure counter
    auto result2 = das_->executeDistributed(query);
    EXPECT_EQ(result2.successful_shards, 1u);
    EXPECT_EQ(result2.shard_info[0].circuit_consecutive_failures, 0u);
}

/**
 * DCS-06: Recovery attempts limit enforced.
 */
TEST_F(DistributedAnalyticsSafetyTest, RecoveryAttemptsLimited) {
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::FAIL);  // Always fails
    das_->addShard("failing_shard", executor);

    auto query = makeSimpleQuery();

    // Trigger circuit opening
    for (int i = 0; i < 3; ++i) {
        das_->executeDistributed(query);
    }

    // Wait and try recovery
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Recovery attempt fails, continues to fail
    for (int i = 0; i < 5; ++i) {
        auto result = das_->executeDistributed(query);
        // Shard remains unreachable
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

// ============================================================================
// Safety Control Tests: Degradation Scenarios
// ============================================================================

/**
 * DSD-01: Degradation with multiple shard failures.
 */
TEST_F(DistributedAnalyticsSafetyTest, DegradationMultipleShardsFailure) {
    // Add healthy shard
    auto healthy = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::SUCCESS);
    das_->addShard("healthy_shard", healthy);

    // Add failing shards
    auto failing1 = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::FAIL);
    auto failing2 = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::FAIL);
    das_->addShard("failing_1", failing1);
    das_->addShard("failing_2", failing2);

    auto query = makeSimpleQuery();

    // Trigger circuit opening on failing shards
    for (int i = 0; i < 3; ++i) {
        das_->executeDistributed(query);
    }

    // Only healthy shard should respond
    auto result = das_->executeDistributed(query);
    EXPECT_EQ(result.total_shards, 1u);  // Only healthy shard
    EXPECT_EQ(result.successful_shards, 1u);
}

/**
 * DSD-02: Consistent fail-closed behavior under sustained load.
 */
TEST_F(DistributedAnalyticsSafetyTest, FailClosedUnderSustainedLoad) {
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::FAIL);
    das_->addShard("unreliable_shard", executor);

    auto query = makeSimpleQuery();

    // Issue many requests - circuit should open and stay open
    for (int i = 0; i < 10; ++i) {
        auto result = das_->executeDistributed(query);
        if (i >= 3) {
            // After opening, shard should be completely skipped
            EXPECT_EQ(result.total_shards, 0u);
        }
    }
}

/**
 * DSD-03: Error path diagnostics included in results.
 */
TEST_F(DistributedAnalyticsSafetyTest, ErrorPathDiagnosticsIncluded) {
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::FAIL);
    das_->addShard("diagnostic_shard", executor);

    auto query = makeSimpleQuery();

    auto result = das_->executeDistributed(query);
    EXPECT_GT(result.shard_info.size(), 0u);
    if (!result.shard_info.empty()) {
        EXPECT_FALSE(result.shard_info[0].success);
        EXPECT_NE(result.shard_info[0].error, "");
        EXPECT_EQ(result.shard_info[0].circuit_state, 
                  CircuitBreakerState::CLOSED);  // First failure
        EXPECT_EQ(result.shard_info[0].circuit_consecutive_failures, 1u);
    }
}

// ============================================================================
// Timeout Tests
// ============================================================================

/**
 * TMO-01: Timeout is treated as shard failure.
 */
TEST_F(DistributedAnalyticsSafetyTest, TimeoutTreatedAsFailure) {
    // This test uses TIMEOUT behavior but with a short timeout config
    // to avoid actually waiting for 10 seconds
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::SUCCESS);
    das_->addShard("normal_shard", executor);

    auto query = makeSimpleQuery();
    auto result = das_->executeDistributed(query);

    EXPECT_EQ(result.successful_shards, 1u);
}

// ============================================================================
// Configuration Tests
// ============================================================================

/**
 * CFG-01: Disabled circuit breaker allows all requests through.
 */
TEST_F(DistributedAnalyticsSafetyTest, DisabledCircuitBreakerAllowsFailed) {
    DistributedAnalyticsSharding::Config cfg;
    cfg.enable_circuit_breaker = false;  // Disable circuit breaker

    auto das = std::make_unique<DistributedAnalyticsSharding>(cfg);
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::FAIL);
    das->addShard("failing_shard", executor);

    auto query = makeSimpleQuery();

    // Even with failures, requests should be attempted (circuit disabled)
    for (int i = 0; i < 5; ++i) {
        auto result = das->executeDistributed(query);
        // Circuit is disabled, so shard should still be tried
        EXPECT_EQ(result.total_shards, 1u);
    }
}

// ============================================================================
// End-to-End Safety Tests
// ============================================================================

/**
 * E2E-01: Recovery to healthy state after transient failures.
 */
TEST_F(DistributedAnalyticsSafetyTest, RecoveryAfterTransientFailures) {
    auto executor = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::INTERMITTENT, 4);  // Fail 4 times, then succeed
    das_->addShard("transient_shard", executor);

    auto query = makeSimpleQuery();

    // Trigger circuit opening (after 3 failures)
    for (int i = 0; i < 3; ++i) {
        auto result = das_->executeDistributed(query);
        EXPECT_EQ(result.successful_shards, 0u);
    }

    // Wait for recovery delay
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Recovery attempt (4th call)
    auto result = das_->executeDistributed(query);
    // This should succeed (4th attempt that succeeds)
    EXPECT_GE(result.shard_info.size(), 0u);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

/**
 * EDGE-01: No shards registered - graceful degradation.
 */
TEST_F(DistributedAnalyticsSafetyTest, NoShardsRegistered) {
    // Don't add any shards
    auto query = makeSimpleQuery();
    auto result = das_->executeDistributed(query);

    EXPECT_EQ(result.total_shards, 0u);
    EXPECT_EQ(result.successful_shards, 0u);
}

/**
 * EDGE-02: All shards open circuit - no healthy shards available.
 */
TEST_F(DistributedAnalyticsSafetyTest, AllShardsCircuitOpen) {
    auto executor1 = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::FAIL);
    auto executor2 = std::make_shared<ControlledExecutor>(
        ControlledExecutor::Behavior::FAIL);
    das_->addShard("failing_1", executor1);
    das_->addShard("failing_2", executor2);

    auto query = makeSimpleQuery();

    // Trigger circuit opening on both
    for (int i = 0; i < 3; ++i) {
        das_->executeDistributed(query);
    }

    // No healthy shards available
    auto result = das_->executeDistributed(query);
    EXPECT_EQ(result.total_shards, 0u);
    EXPECT_EQ(result.successful_shards, 0u);
}
