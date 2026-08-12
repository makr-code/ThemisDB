/**
 * Unit tests for Circuit Breaker Pattern
 * 
 * Tests the three states: CLOSED → OPEN → HALF_OPEN
 * and automatic recovery behavior
 */

#include <gtest/gtest.h>

// Disable legacy circuit breaker tests
#if 0
#include "sharding/circuit_breaker.h"
#include <thread>
#include <chrono>

using namespace themis::sharding;

// ============================================================================
// Circuit Breaker Basic Tests
// ============================================================================

TEST(CircuitBreakerTest, InitialStateClosed) {
    CircuitBreaker cb;
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_TRUE(cb.allowRequest());
}

TEST(CircuitBreakerTest, ClosedToOpenAfterFailures) {
    CircuitBreaker::Config config;
    config.failure_threshold = 3;
    CircuitBreaker cb(config);
    
    // Record 2 failures - should stay CLOSED
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_TRUE(cb.allowRequest());
    
    // Record 3rd failure - should open
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.allowRequest());
}

TEST(CircuitBreakerTest, OpenBlocksRequests) {
    CircuitBreaker::Config config;
    config.failure_threshold = 2;
    CircuitBreaker cb(config);
    
    // Trip circuit
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    
    // Requests should be blocked
    EXPECT_FALSE(cb.allowRequest());
    EXPECT_FALSE(cb.allowRequest());
    EXPECT_FALSE(cb.allowRequest());
}

TEST(CircuitBreakerTest, OpenToHalfOpenAfterTimeout) {
    CircuitBreaker::Config config;
    config.failure_threshold = 2;
    config.timeout = std::chrono::seconds(1); // Short timeout for testing
    CircuitBreaker cb(config);
    
    // Trip circuit
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Next request should transition to HALF_OPEN
    EXPECT_TRUE(cb.allowRequest());
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
}

TEST(CircuitBreakerTest, HalfOpenToClosedAfterSuccesses) {
    CircuitBreaker::Config config;
    config.failure_threshold = 2;
    config.success_threshold = 2;
    config.timeout = std::chrono::seconds(1);
    CircuitBreaker cb(config);
    
    // Trip circuit
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Transition to HALF_OPEN
    EXPECT_TRUE(cb.allowRequest());
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
    
    // Record successes
    cb.recordSuccess();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
    
    cb.recordSuccess();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_TRUE(cb.allowRequest());
}

TEST(CircuitBreakerTest, HalfOpenToOpenOnFailure) {
    CircuitBreaker::Config config;
    config.failure_threshold = 2;
    config.timeout = std::chrono::seconds(1);
    CircuitBreaker cb(config);
    
    // Trip circuit
    cb.recordFailure();
    cb.recordFailure();
    
    // Wait and transition to HALF_OPEN
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_TRUE(cb.allowRequest());
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::HALF_OPEN);
    
    // Any failure in HALF_OPEN should reopen circuit
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.allowRequest());
}

TEST(CircuitBreakerTest, SuccessInClosedResetsFailureCount) {
    CircuitBreaker::Config config;
    config.failure_threshold = 3;
    CircuitBreaker cb(config);
    
    // Record some failures
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getFailureCount(), 2);
    
    // Success should reset consecutive failures
    cb.recordSuccess();
    
    // Should still be CLOSED
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST(CircuitBreakerTest, RollingWindowCleanup) {
    CircuitBreaker::Config config;
    config.failure_threshold = 3;
    config.failure_window = std::chrono::seconds(2);
    CircuitBreaker cb(config);
    
    // Record 2 failures
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getFailureCount(), 2);
    
    // Wait for window to expire
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Old failures should be cleaned up
    EXPECT_EQ(cb.getFailureCount(), 0);
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
}

TEST(CircuitBreakerTest, Reset) {
    CircuitBreaker::Config config;
    config.failure_threshold = 2;
    CircuitBreaker cb(config);
    
    // Trip circuit
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    
    // Reset should close circuit
    cb.reset();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_EQ(cb.getFailureCount(), 0);
    EXPECT_TRUE(cb.allowRequest());
}

TEST(CircuitBreakerTest, ForceOpen) {
    CircuitBreaker cb;
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::CLOSED);
    
    // Force open
    cb.forceOpen();
    EXPECT_EQ(cb.getState(), CircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.allowRequest());
}

// ============================================================================
// Circuit Breaker Manager Tests
// ============================================================================

TEST(CircuitBreakerManagerTest, GetCircuitBreaker) {
    CircuitBreakerManager manager;
    
    auto& cb1 = manager.getCircuitBreaker("shard_1");
    auto& cb2 = manager.getCircuitBreaker("shard_2");
    auto& cb1_again = manager.getCircuitBreaker("shard_1");
    
    // Should return same instance for same shard
    EXPECT_EQ(&cb1, &cb1_again);
    EXPECT_NE(&cb1, &cb2);
}

TEST(CircuitBreakerManagerTest, HasCircuitBreaker) {
    CircuitBreakerManager manager;
    
    EXPECT_FALSE(manager.hasCircuitBreaker("shard_1"));
    
    manager.getCircuitBreaker("shard_1");
    EXPECT_TRUE(manager.hasCircuitBreaker("shard_1"));
}

TEST(CircuitBreakerManagerTest, RemoveCircuitBreaker) {
    CircuitBreakerManager manager;
    
    manager.getCircuitBreaker("shard_1");
    EXPECT_TRUE(manager.hasCircuitBreaker("shard_1"));
    
    manager.removeCircuitBreaker("shard_1");
    EXPECT_FALSE(manager.hasCircuitBreaker("shard_1"));
}

TEST(CircuitBreakerManagerTest, ResetAll) {
    CircuitBreakerManager manager;
    CircuitBreaker::Config config;
    config.failure_threshold = 2;
    
    auto& cb1 = manager.getCircuitBreaker("shard_1", config);
    auto& cb2 = manager.getCircuitBreaker("shard_2", config);
    
    // Trip both circuits
    cb1.recordFailure();
    cb1.recordFailure();
    cb2.recordFailure();
    cb2.recordFailure();
    
    EXPECT_EQ(cb1.getState(), CircuitBreaker::State::OPEN);
    EXPECT_EQ(cb2.getState(), CircuitBreaker::State::OPEN);
    
    // Reset all
    manager.resetAll();
    
    EXPECT_EQ(cb1.getState(), CircuitBreaker::State::CLOSED);
    EXPECT_EQ(cb2.getState(), CircuitBreaker::State::CLOSED);
}

TEST(CircuitBreakerManagerTest, GetAllShardIds) {
    CircuitBreakerManager manager;
    
    manager.getCircuitBreaker("shard_1");
    manager.getCircuitBreaker("shard_2");
    manager.getCircuitBreaker("shard_3");
    
    auto shard_ids = manager.getAllShardIds();
    EXPECT_EQ(shard_ids.size(), 3);
}

TEST(CircuitBreakerManagerTest, GetStateCount) {
    CircuitBreakerManager manager;
    CircuitBreaker::Config config;
    config.failure_threshold = 2;
    
    auto& cb1 = manager.getCircuitBreaker("shard_1", config);
    auto& cb2 = manager.getCircuitBreaker("shard_2", config);
    auto& cb3 = manager.getCircuitBreaker("shard_3", config);
    
    // cb1: CLOSED
    // cb2: OPEN
    cb2.recordFailure();
    cb2.recordFailure();
    // cb3: OPEN
    cb3.recordFailure();
    cb3.recordFailure();
    
    auto count = manager.getStateCount();
    EXPECT_EQ(count.closed, 1);
    EXPECT_EQ(count.open, 2);
    EXPECT_EQ(count.half_open, 0);
}

// ============================================================================
// Concurrent Access Tests
// ============================================================================

TEST(CircuitBreakerTest, ConcurrentAccess) {
    CircuitBreaker::Config config;
    config.failure_threshold = 100;
    CircuitBreaker cb(config);
    
    const int num_threads = 10;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&cb, operations_per_thread]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                cb.allowRequest();
                if (j % 2 == 0) {
                    cb.recordSuccess();
                } else {
                    cb.recordFailure();
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should not crash and state should be consistent
    EXPECT_TRUE(cb.getState() == CircuitBreaker::State::CLOSED ||
                cb.getState() == CircuitBreaker::State::OPEN ||
                cb.getState() == CircuitBreaker::State::HALF_OPEN);
}

TEST(CircuitBreakerTest, StateToString) {
    EXPECT_EQ(CircuitBreaker::stateToString(CircuitBreaker::State::CLOSED), "CLOSED");
    EXPECT_EQ(CircuitBreaker::stateToString(CircuitBreaker::State::OPEN), "OPEN");
    EXPECT_EQ(CircuitBreaker::stateToString(CircuitBreaker::State::HALF_OPEN), "HALF_OPEN");
}

#endif // 0

// ============================================================================
// Core ICircuitBreaker / DefaultCircuitBreaker Tests (Issue #1415)
// ============================================================================

#include "core/concerns/i_circuit_breaker.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/concerns_context.h"
#include <thread>
#include <chrono>

using namespace themis::core::concerns;

// ---------------------------------------------------------------------------
// DefaultCircuitBreaker — state machine

TEST(CoreCircuitBreakerTest, InitialStateClosed) {
    DefaultCircuitBreaker cb;
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);
    EXPECT_TRUE(cb.allowRequest());
}

TEST(CoreCircuitBreakerTest, ClosedToOpenAfterFailureThreshold) {
    ICircuitBreaker::Config cfg;
    cfg.failure_threshold = 3;
    DefaultCircuitBreaker cb(cfg);

    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);

    cb.recordFailure();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.allowRequest());
}

TEST(CoreCircuitBreakerTest, OpenBlocksRequests) {
    ICircuitBreaker::Config cfg;
    cfg.failure_threshold = 2;
    DefaultCircuitBreaker cb(cfg);

    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.allowRequest());
    EXPECT_FALSE(cb.allowRequest());
}

TEST(CoreCircuitBreakerTest, OpenToHalfOpenAfterTimeout) {
    ICircuitBreaker::Config cfg;
    cfg.failure_threshold = 2;
    cfg.timeout = std::chrono::seconds(1);
    DefaultCircuitBreaker cb(cfg);

    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);

    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_TRUE(cb.allowRequest());
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::HALF_OPEN);
}

TEST(CoreCircuitBreakerTest, HalfOpenToClosedAfterSuccesses) {
    ICircuitBreaker::Config cfg;
    cfg.failure_threshold  = 2;
    cfg.success_threshold  = 2;
    cfg.timeout            = std::chrono::seconds(1);
    DefaultCircuitBreaker cb(cfg);

    cb.recordFailure();
    cb.recordFailure();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_TRUE(cb.allowRequest());
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::HALF_OPEN);

    cb.recordSuccess();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::HALF_OPEN);
    cb.recordSuccess();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);
}

TEST(CoreCircuitBreakerTest, HalfOpenToOpenOnFailure) {
    ICircuitBreaker::Config cfg;
    cfg.failure_threshold = 2;
    cfg.timeout = std::chrono::seconds(1);
    DefaultCircuitBreaker cb(cfg);

    cb.recordFailure();
    cb.recordFailure();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_TRUE(cb.allowRequest());
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::HALF_OPEN);

    cb.recordFailure();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);
}

TEST(CoreCircuitBreakerTest, Reset) {
    ICircuitBreaker::Config cfg;
    cfg.failure_threshold = 2;
    DefaultCircuitBreaker cb(cfg);

    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);

    cb.reset();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);
    EXPECT_EQ(cb.getFailureCount(), 0u);
    EXPECT_TRUE(cb.allowRequest());
}

TEST(CoreCircuitBreakerTest, ForceOpen) {
    DefaultCircuitBreaker cb;
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);

    cb.forceOpen();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.allowRequest());
}

TEST(CoreCircuitBreakerTest, StateToString) {
    EXPECT_EQ("CLOSED",    ICircuitBreaker::stateToString(ICircuitBreaker::State::CLOSED));
    EXPECT_EQ("OPEN",      ICircuitBreaker::stateToString(ICircuitBreaker::State::OPEN));
    EXPECT_EQ("HALF_OPEN", ICircuitBreaker::stateToString(ICircuitBreaker::State::HALF_OPEN));
}

// ---------------------------------------------------------------------------
// NoOpCircuitBreaker — always allows requests

TEST(CoreCircuitBreakerTest, NoOpAlwaysAllows) {
    NoOpCircuitBreaker cb;
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);
    EXPECT_TRUE(cb.allowRequest());
    cb.recordFailure();
    cb.recordFailure();
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::CLOSED);
    EXPECT_TRUE(cb.allowRequest());
}

TEST(CoreCircuitBreakerTest, NoOpIsHealthy) {
    NoOpCircuitBreaker cb;
    auto result = cb.isHealthy();
    EXPECT_TRUE(result.ok);
}

// ---------------------------------------------------------------------------
// ICircuitBreaker::isHealthy default implementation

TEST(CoreCircuitBreakerTest, DefaultIsHealthyOpenUnhealthy) {
    ICircuitBreaker::Config cfg;
    cfg.failure_threshold = 1;
    DefaultCircuitBreaker cb(cfg);

    EXPECT_TRUE(cb.isHealthy().ok);
    cb.recordFailure();
    EXPECT_EQ(cb.getState(), ICircuitBreaker::State::OPEN);
    EXPECT_FALSE(cb.isHealthy().ok);
}

// ---------------------------------------------------------------------------
// ConcernsContext integration

TEST(CoreCircuitBreakerTest, ConcernsContextExposesCircuitBreaker) {
    auto ctx = ConcernsContext::createNoOp();
    // Accessor must be available and return CLOSED noop state
    EXPECT_EQ(ctx->circuitBreaker().getState(), ICircuitBreaker::State::CLOSED);
    EXPECT_TRUE(ctx->circuitBreaker().allowRequest());
}

TEST(CoreCircuitBreakerTest, ConcernsContextHealthCheckIncludesCircuitBreaker) {
    auto ctx = ConcernsContext::createNoOp();
    auto status = ctx->healthCheck();
    EXPECT_TRUE(status.circuit_breaker.ok);
    EXPECT_TRUE(status.isHealthy());
}

TEST(CoreCircuitBreakerTest, ConcernsContextCustomWithExplicitCircuitBreaker) {
    ICircuitBreaker::Config cfg;
    cfg.failure_threshold = 2;
    auto cb = std::make_unique<DefaultCircuitBreaker>(cfg);
    cb->recordFailure();
    cb->recordFailure();
    EXPECT_EQ(cb->getState(), ICircuitBreaker::State::OPEN);

    auto ctx = ConcernsContext::createCustom(
        std::make_unique<NoOpLogger>(),
        std::make_unique<NoOpTracer>(),
        std::make_unique<NoOpMetrics>(),
        std::make_unique<NoOpCache>(),
        std::move(cb)
    );

    auto status = ctx->healthCheck();
    EXPECT_FALSE(status.circuit_breaker.ok);
    EXPECT_FALSE(status.isHealthy());
}

TEST(CoreCircuitBreakerTest, ConcernsContextCircuitBreakerAdapter_Noop) {
    ConcernsContext::Config cfg;
    cfg.circuitBreakerAdapter = "noop";
    cfg.metricsAdapter        = "noop";
    cfg.tracerAdapter         = "noop";
    // create() validates and instantiates — should not throw
    EXPECT_NO_THROW({
        auto ctx = ConcernsContext::create(cfg);
        EXPECT_EQ(ctx->circuitBreaker().getState(), ICircuitBreaker::State::CLOSED);
        EXPECT_TRUE(ctx->circuitBreaker().allowRequest());
    });
}

TEST(CircuitBreakerDisabledTest, DISABLED_AllTestsSkipped) {
    GTEST_SKIP() << "Legacy sharding circuit breaker tests are currently disabled";
}
