#include <gtest/gtest.h>
#include "auth/auth_rate_limiter.h"
#include "auth/rate_limiter_backend.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::auth;

// ============================================================================
// InMemoryRateLimiterBackend unit tests
// ============================================================================

/**
 * @brief Verify that increment() counts within the sliding window.
 */
TEST(InMemoryRateLimiterBackendTest, BasicIncrement) {
    InMemoryRateLimiterBackend backend;

    EXPECT_EQ(backend.increment("key1", 60), 1);
    EXPECT_EQ(backend.increment("key1", 60), 2);
    EXPECT_EQ(backend.increment("key1", 60), 3);

    // Different key has independent counter
    EXPECT_EQ(backend.increment("key2", 60), 1);
}

/**
 * @brief Verify that getCount() is read-only (does not modify counters).
 */
TEST(InMemoryRateLimiterBackendTest, GetCountIsReadOnly) {
    InMemoryRateLimiterBackend backend;

    EXPECT_EQ(backend.getCount("k", 60), 0);
    backend.increment("k", 60);
    EXPECT_EQ(backend.getCount("k", 60), 1);
    EXPECT_EQ(backend.getCount("k", 60), 1); // still 1 — not incremented
    backend.increment("k", 60);
    EXPECT_EQ(backend.getCount("k", 60), 2);
}

/**
 * @brief Verify that reset() clears the counter for the given key.
 */
TEST(InMemoryRateLimiterBackendTest, Reset) {
    InMemoryRateLimiterBackend backend;

    backend.increment("k", 60);
    backend.increment("k", 60);
    EXPECT_EQ(backend.getCount("k", 60), 2);

    backend.reset("k");
    EXPECT_EQ(backend.getCount("k", 60), 0);

    // After reset, fresh increments start from 1 again
    EXPECT_EQ(backend.increment("k", 60), 1);
}

/**
 * @brief Verify that entries older than the window are pruned automatically.
 */
TEST(InMemoryRateLimiterBackendTest, SlidingWindowExpiry) {
    InMemoryRateLimiterBackend backend;

    // Record two requests with a very short window (1 second).
    backend.increment("k", 1);
    backend.increment("k", 1);
    EXPECT_EQ(backend.getCount("k", 1), 2);

    // Wait for the window to elapse.
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Old entries should have expired; count must be 0.
    EXPECT_EQ(backend.getCount("k", 1), 0);

    // A fresh increment after expiry starts the counter from 1.
    EXPECT_EQ(backend.increment("k", 1), 1);
}

/**
 * @brief Verify thread-safety: concurrent increments from multiple threads
 *        produce the correct total count.
 */
TEST(InMemoryRateLimiterBackendTest, ConcurrentIncrements) {
    InMemoryRateLimiterBackend backend;

    constexpr int kThreads    = 8;
    constexpr int kPerThread  = 50;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < kPerThread; ++j) {
                backend.increment("shared", 60);
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(backend.getCount("shared", 60), kThreads * kPerThread);
}

TEST(RedisRateLimiterBackendStubBridgeTest, BridgeCallbacksOverrideLocalFallback) {
    RedisRateLimiterBackend::setIncrementFn([](const std::string& key, uint32_t window_seconds) {
        return key == "bridge" && window_seconds == 30 ? 11 : 0;
    });
    RedisRateLimiterBackend::setGetCountFn([](const std::string& key, uint32_t) {
        return key == "bridge" ? 7 : 0;
    });

    bool reset_called = false;
    RedisRateLimiterBackend::setResetFn([&reset_called](const std::string& key) {
        reset_called = (key == "bridge");
    });
    RedisRateLimiterBackend::setIsConnectedFn([] { return true; });
    RedisRateLimiterBackend::setReconnectFn([] { return true; });

    RedisRateLimiterBackend backend;
    EXPECT_EQ(backend.increment("bridge", 30), 11);
    EXPECT_EQ(backend.getCount("bridge", 30), 7);
    backend.reset("bridge");
    EXPECT_TRUE(reset_called);
    EXPECT_TRUE(backend.isConnected());
    EXPECT_TRUE(backend.reconnect());

    RedisRateLimiterBackend::setIncrementFn(nullptr);
    RedisRateLimiterBackend::setGetCountFn(nullptr);
    RedisRateLimiterBackend::setResetFn(nullptr);
    RedisRateLimiterBackend::setIsConnectedFn(nullptr);
    RedisRateLimiterBackend::setReconnectFn(nullptr);
}

// ============================================================================
// Distributed integration tests: two AuthRateLimiter instances sharing a
// single InMemoryRateLimiterBackend observe each other's request counts.
// ============================================================================

/**
 * @brief Two AuthRateLimiter instances sharing a backend see the combined
 *        IP request count, causing rate-limiting when the joint total exceeds
 *        the per-IP limit.
 *
 * This mirrors the behaviour expected from two nodes sharing a
 * RedisRateLimiterBackend: an attacker cannot bypass the per-IP limit by
 * spreading requests across nodes.
 */
TEST(AuthRateLimiterDistributedTest, SharedBackendCombinedIPCount) {
    AuthRateLimitConfig config;
    config.max_attempts_per_ip_per_minute = 3;
    config.enable_user_rate_limiting      = false;
    config.enable_account_lockout         = false;

    auto backend = std::make_shared<InMemoryRateLimiterBackend>();

    AuthRateLimiter limiter1(config);
    AuthRateLimiter limiter2(config);
    limiter1.setBackend(backend);
    limiter2.setBackend(backend);

    const std::string ip = "192.168.1.100";

    // limiter1 consumes 2 of the 3 allowed slots.
    EXPECT_TRUE(limiter1.allowAuthAttempt(ip)); // count = 1
    EXPECT_TRUE(limiter1.allowAuthAttempt(ip)); // count = 2

    // limiter2 sees the shared state: only 1 slot remains.
    EXPECT_TRUE(limiter2.allowAuthAttempt(ip));  // count = 3 (at limit)
    EXPECT_FALSE(limiter2.allowAuthAttempt(ip)); // count = 4 (exceeds limit)

    // limiter1 is also blocked now (combined state is over the limit).
    EXPECT_FALSE(limiter1.allowAuthAttempt(ip)); // count = 5
}

/**
 * @brief Two AuthRateLimiter instances sharing a backend see the combined
 *        per-user request count.
 */
TEST(AuthRateLimiterDistributedTest, SharedBackendCombinedUserCount) {
    AuthRateLimitConfig config;
    config.max_attempts_per_user_per_minute = 2;
    config.enable_ip_rate_limiting          = false;
    config.enable_account_lockout           = false;

    auto backend = std::make_shared<InMemoryRateLimiterBackend>();

    AuthRateLimiter limiter1(config);
    AuthRateLimiter limiter2(config);
    limiter1.setBackend(backend);
    limiter2.setBackend(backend);

    const std::string ip   = "10.0.0.1";
    const std::string user = "alice";

    // limiter1 uses 1 of the 2 allowed user slots.
    EXPECT_TRUE(limiter1.allowAuthAttempt(ip, user)); // count = 1

    // limiter2 sees 1 already used; 1 slot remains.
    EXPECT_TRUE(limiter2.allowAuthAttempt(ip, user));  // count = 2 (at limit)
    EXPECT_FALSE(limiter2.allowAuthAttempt(ip, user)); // count = 3 (exceeds limit)

    // Different user is unaffected.
    EXPECT_TRUE(limiter1.allowAuthAttempt(ip, "bob")); // independent key
}

/**
 * @brief Verify that two limiters sharing a backend independently enforce
 *        limits for different IPs.
 */
TEST(AuthRateLimiterDistributedTest, DifferentIPsAreIndependent) {
    AuthRateLimitConfig config;
    config.max_attempts_per_ip_per_minute = 2;
    config.enable_user_rate_limiting      = false;
    config.enable_account_lockout         = false;

    auto backend = std::make_shared<InMemoryRateLimiterBackend>();

    AuthRateLimiter limiter1(config);
    AuthRateLimiter limiter2(config);
    limiter1.setBackend(backend);
    limiter2.setBackend(backend);

    const std::string ip1 = "1.1.1.1";
    const std::string ip2 = "2.2.2.2";

    // Exhaust ip1 limit via limiter1.
    EXPECT_TRUE(limiter1.allowAuthAttempt(ip1));
    EXPECT_TRUE(limiter1.allowAuthAttempt(ip1));
    EXPECT_FALSE(limiter1.allowAuthAttempt(ip1)); // blocked

    // ip2 from limiter2 is completely independent.
    EXPECT_TRUE(limiter2.allowAuthAttempt(ip2));
    EXPECT_TRUE(limiter2.allowAuthAttempt(ip2));
    EXPECT_FALSE(limiter2.allowAuthAttempt(ip2)); // blocked at its own limit
}

/**
 * @brief Verify that a limiter with no backend set continues to use the
 *        existing in-process token-bucket behaviour (backward compatibility).
 */
TEST(AuthRateLimiterDistributedTest, NoBackendFallsBackToTokenBucket) {
    AuthRateLimitConfig config;
    config.max_attempts_per_ip_per_minute = 3;
    config.enable_user_rate_limiting      = false;
    config.enable_account_lockout         = false;

    // No backend set — should behave identically to the original tests.
    AuthRateLimiter limiter(config);

    const std::string ip = "192.168.0.1";

    EXPECT_TRUE(limiter.allowAuthAttempt(ip));
    EXPECT_TRUE(limiter.allowAuthAttempt(ip));
    EXPECT_TRUE(limiter.allowAuthAttempt(ip));
    EXPECT_FALSE(limiter.allowAuthAttempt(ip)); // token bucket exhausted
}

/**
 * @brief Verify that setBackend(nullptr) reverts to the default token-bucket.
 */
TEST(AuthRateLimiterDistributedTest, NullBackendRevertsToDefault) {
    AuthRateLimitConfig config;
    config.max_attempts_per_ip_per_minute = 3;
    config.enable_user_rate_limiting      = false;
    config.enable_account_lockout         = false;

    auto backend = std::make_shared<InMemoryRateLimiterBackend>();

    AuthRateLimiter limiter(config);
    limiter.setBackend(backend);

    // Exhaust the shared backend counter.
    const std::string ip = "192.168.0.2";
    limiter.allowAuthAttempt(ip); // 1
    limiter.allowAuthAttempt(ip); // 2
    limiter.allowAuthAttempt(ip); // 3
    EXPECT_FALSE(limiter.allowAuthAttempt(ip)); // blocked via backend

    // Detach backend; token bucket is still full (never used while backend was set).
    limiter.setBackend(nullptr);
    limiter.reset(); // reset token bucket state

    EXPECT_TRUE(limiter.allowAuthAttempt(ip)); // allowed via token bucket
}

#ifndef THEMIS_ENABLE_REDIS
/**
 * @brief Verify RedisRateLimiterBackend falls back to process-local in-memory
 *        counters when hiredis support is not compiled in.
 */
TEST(RedisRateLimiterBackendNoRedisTest, UsesInProcessFallbackCounters) {
    RedisRateLimiterBackend backend;

    EXPECT_EQ(backend.increment("ip:1.2.3.4", 60), 1);
    EXPECT_EQ(backend.increment("ip:1.2.3.4", 60), 2);
    EXPECT_EQ(backend.getCount("ip:1.2.3.4", 60), 2);

    backend.reset("ip:1.2.3.4");
    EXPECT_EQ(backend.getCount("ip:1.2.3.4", 60), 0);

    // Redis connectivity is still unavailable in this build mode.
    EXPECT_FALSE(backend.isConnected());
    EXPECT_FALSE(backend.reconnect());
}
#endif
