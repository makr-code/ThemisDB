/**
 * @file test_transaction_retry.cpp
 * @brief Tests for TransactionRetryManager
 */

#include <gtest/gtest.h>
#include "storage/transaction_retry_manager.h"
#include <atomic>
#include <stdexcept>

using namespace themisdb::storage;

// ── Helpers ──────────────────────────────────────────────────────────────────

/** Return a config with negligible delays so tests run fast. */
static TransactionRetryConfig fastConfig(size_t max_attempts = 3) {
    TransactionRetryConfig cfg;
    cfg.max_attempts           = max_attempts;
    cfg.base_delay_ms          = 0;
    cfg.max_delay_ms           = 0;
    cfg.enable_jitter          = false;
    cfg.max_total_timeout_ms   = 60000;
    cfg.enable_circuit_breaker = false;
    return cfg;
}

// ── Basic success ─────────────────────────────────────────────────────────────

TEST(TransactionRetryManager, SuccessOnFirstAttempt) {
    TransactionRetryManager mgr(fastConfig());

    int value = mgr.executeWithRetry([]() { return 42; }, "op");

    EXPECT_EQ(value, 42);
    EXPECT_EQ(mgr.getStatistics().successful_operations.load(), 1u);
    EXPECT_EQ(mgr.getStatistics().failed_operations.load(), 0u);
}

// ── Retry on retryable error ──────────────────────────────────────────────────

TEST(TransactionRetryManager, RetriesOnWriteConflict) {
    TransactionRetryManager mgr(fastConfig(5));

    std::atomic<int> calls{0};
    int result = mgr.executeWithRetry([&]() -> int {
        int c = ++calls;
        if (c < 3) throw std::runtime_error("write conflict detected");
        return c;
    }, "conflict_op");

    EXPECT_EQ(calls.load(), 3);
    EXPECT_EQ(result, 3);
    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.successful_operations.load(), 1u);
    EXPECT_GT(stats.total_retry_attempts.load(), 0u);
}

// ── No retry on non-retryable error ──────────────────────────────────────────

TEST(TransactionRetryManager, NoRetryOnPermissionDenied) {
    TransactionRetryManager mgr(fastConfig(5));

    std::atomic<int> calls{0};
    EXPECT_THROW({
        mgr.executeWithRetry([&]() -> int {
            ++calls;
            throw std::runtime_error("permission denied");
        }, "perm_op");
    }, std::runtime_error);

    // Must not retry non-retryable errors
    EXPECT_EQ(calls.load(), 1);
}

// ── Max attempts exhausted ────────────────────────────────────────────────────

TEST(TransactionRetryManager, ThrowsAfterMaxAttempts) {
    TransactionRetryManager mgr(fastConfig(3));

    std::atomic<int> calls{0};
    EXPECT_THROW({
        mgr.executeWithRetry([&]() -> int {
            ++calls;
            throw std::runtime_error("timeout occurred");
        }, "timeout_op");
    }, std::runtime_error);

    EXPECT_EQ(calls.load(), 3);
    EXPECT_EQ(mgr.getStatistics().failed_operations.load(), 1u);
}

TEST(TransactionRetryManager, RetryAttemptsNotOvercountedWhenExhausted) {
    // max_attempts = 2 → one initial try + one retry
    TransactionRetryManager mgr(fastConfig(2));

    std::atomic<int> calls{0};
    EXPECT_THROW({
        mgr.executeWithRetry([&]() -> int {
            ++calls;
            throw std::runtime_error("timeout occurred");
        }, "timeout_op");
    }, std::runtime_error);

    EXPECT_EQ(calls.load(), 2);
    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.total_retry_attempts.load(), 1u);
}

// ── Error classification ──────────────────────────────────────────────────────

TEST(TransactionRetryManager, ClassifyRetryableErrors) {
    EXPECT_EQ(TransactionRetryManager::classifyError("write conflict"), ErrorType::WRITE_CONFLICT);
    EXPECT_EQ(TransactionRetryManager::classifyError("operation timed out"), ErrorType::TIMEOUT);
    EXPECT_EQ(TransactionRetryManager::classifyError("network connection lost"), ErrorType::NETWORK_ERROR);
    EXPECT_EQ(TransactionRetryManager::classifyError("resource exhausted"), ErrorType::RESOURCE_EXHAUSTED);
    EXPECT_EQ(TransactionRetryManager::classifyError("service unavailable"), ErrorType::SERVICE_UNAVAILABLE);
}

TEST(TransactionRetryManager, ClassifyNonRetryableErrors) {
    EXPECT_EQ(TransactionRetryManager::classifyError("unique constraint violation"), ErrorType::CONSTRAINT_VIOLATION);
    EXPECT_EQ(TransactionRetryManager::classifyError("invalid argument"), ErrorType::INVALID_ARGUMENT);
    EXPECT_EQ(TransactionRetryManager::classifyError("not found"), ErrorType::NOT_FOUND);
    EXPECT_EQ(TransactionRetryManager::classifyError("permission denied"), ErrorType::PERMISSION_DENIED);
    EXPECT_EQ(TransactionRetryManager::classifyError("data corrupt"), ErrorType::DATA_CORRUPTION);
}

TEST(TransactionRetryManager, IsRetryable) {
    EXPECT_TRUE(TransactionRetryManager::isRetryable(ErrorType::WRITE_CONFLICT));
    EXPECT_TRUE(TransactionRetryManager::isRetryable(ErrorType::TIMEOUT));
    EXPECT_TRUE(TransactionRetryManager::isRetryable(ErrorType::NETWORK_ERROR));
    EXPECT_FALSE(TransactionRetryManager::isRetryable(ErrorType::PERMISSION_DENIED));
    EXPECT_FALSE(TransactionRetryManager::isRetryable(ErrorType::CONSTRAINT_VIOLATION));
    EXPECT_FALSE(TransactionRetryManager::isRetryable(ErrorType::UNKNOWN));
}

// ── Statistics ────────────────────────────────────────────────────────────────

TEST(TransactionRetryManager, StatisticsTracking) {
    TransactionRetryManager mgr(fastConfig(5));

    // One successful call
    mgr.executeWithRetry([]() { return 0; }, "op1");

    // One call that retries then succeeds
    std::atomic<int> calls{0};
    mgr.executeWithRetry([&]() -> int {
        if (++calls < 2) throw std::runtime_error("timeout");
        return 0;
    }, "op2");

    auto stats = mgr.getStatistics();
    EXPECT_EQ(stats.total_operations.load(), 2u);
    EXPECT_EQ(stats.successful_operations.load(), 2u);
    EXPECT_EQ(stats.retried_operations.load(), 1u);

    mgr.resetStatistics();
    auto reset = mgr.getStatistics();
    EXPECT_EQ(reset.total_operations.load(), 0u);
}

// ── Circuit breaker ───────────────────────────────────────────────────────────

TEST(TransactionRetryManager, CircuitBreakerOpensAfterThreshold) {
    TransactionRetryConfig cfg = fastConfig(1);
    cfg.enable_circuit_breaker = true;
    cfg.failure_threshold      = 3;
    cfg.reset_timeout_ms       = 60000;

    TransactionRetryManager mgr(cfg);

    // Trigger enough failures to open the circuit
    for (int i = 0; i < 3; ++i) {
        EXPECT_THROW({
            mgr.executeWithRetry([]() -> int {
                throw std::runtime_error("timeout");
            }, "failing_op");
        }, std::runtime_error);
    }

    EXPECT_EQ(mgr.getCircuitState(), CircuitState::CIRCUIT_OPEN);

    // New requests should be blocked immediately
    EXPECT_THROW({
        mgr.executeWithRetry([]() { return 0; }, "blocked_op");
    }, std::runtime_error);
}

TEST(TransactionRetryManager, AlertCallbackFiredOnCircuitChange) {
    TransactionRetryConfig cfg = fastConfig(1);
    cfg.enable_circuit_breaker = true;
    cfg.failure_threshold      = 3;

    TransactionRetryManager mgr(cfg);

    std::atomic<int> alert_count{0};
    mgr.setAlertCallback([&](CircuitState /*state*/, const std::string& /*msg*/) {
        ++alert_count;
    });

    for (int i = 0; i < 3; ++i) {
        EXPECT_THROW({
            mgr.executeWithRetry([]() -> int {
                throw std::runtime_error("timeout");
            }, "op");
        }, std::runtime_error);
    }

    EXPECT_GT(alert_count.load(), 0);
}

// ── Per-operation RetryPolicy override ───────────────────────────────────────

TEST(TransactionRetryManager, PerOperationPolicyOverridesMaxAttempts) {
    TransactionRetryManager mgr(fastConfig(10));  // global: 10 attempts

    RetryPolicy policy;
    policy.max_attempts = 2;  // override to 2

    std::atomic<int> calls{0};
    EXPECT_THROW({
        mgr.executeWithRetry([&]() -> int {
            ++calls;
            throw std::runtime_error("timeout");
        }, "policy_op", &policy);
    }, std::runtime_error);

    EXPECT_EQ(calls.load(), 2);
}
