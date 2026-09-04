#include <gtest/gtest.h>
#include "server/rate_limiter_v2.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

using namespace themis::server;
using Priority = TokenBucketRateLimiter::Priority;
using Backend  = TokenBucketRateLimiter::Backend;

// ============================================================================
// TokenBucketRateLimiter – LOCAL backend
// ============================================================================

class TokenBucketLocalTest : public ::testing::Test {
protected:
    TokenBucketRateLimiter::Config makeCfg(size_t cap = 10, size_t rate = 100) {
        TokenBucketRateLimiter::Config cfg;
        cfg.capacity    = cap;
        cfg.refill_rate = rate;
        cfg.backend     = Backend::LOCAL;
        cfg.enable_priority_lanes = true;
        cfg.high_capacity    = cap * 2;
        cfg.high_refill_rate = rate * 2;
        cfg.low_capacity     = cap / 2;
        cfg.low_refill_rate  = rate / 2;
        return cfg;
    }
};

TEST_F(TokenBucketLocalTest, InitialTokensAvailable) {
    TokenBucketRateLimiter limiter(makeCfg(10));
    EXPECT_EQ(limiter.getAvailableTokens(Priority::NORMAL), 10u);
}

TEST_F(TokenBucketLocalTest, AcquireReducesTokens) {
    TokenBucketRateLimiter limiter(makeCfg(10));
    EXPECT_TRUE(limiter.tryAcquire(3));
    EXPECT_EQ(limiter.getAvailableTokens(Priority::NORMAL), 7u);
}

TEST_F(TokenBucketLocalTest, ExhaustBucketRejects) {
    TokenBucketRateLimiter limiter(makeCfg(5));
    EXPECT_TRUE(limiter.tryAcquire(5));
    EXPECT_FALSE(limiter.tryAcquire(1));
}

TEST_F(TokenBucketLocalTest, ExactBurstAllowed) {
    TokenBucketRateLimiter limiter(makeCfg(10));
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(limiter.tryAcquire(1)) << "Request " << i << " should be allowed";
    }
    EXPECT_FALSE(limiter.tryAcquire(1));
}

TEST_F(TokenBucketLocalTest, RefillRestoresTokens) {
    TokenBucketRateLimiter limiter(makeCfg(10, 1000)); // 1000 tokens/sec
    EXPECT_TRUE(limiter.tryAcquire(10));
    EXPECT_FALSE(limiter.tryAcquire(1));

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    // After 20 ms at 1000 t/s → ~20 tokens (capped at 10)
    EXPECT_TRUE(limiter.tryAcquire(5));
}

TEST_F(TokenBucketLocalTest, PriorityHighBucketLarger) {
    TokenBucketRateLimiter limiter(makeCfg(10));
    // HIGH bucket capacity is 20 (cap * 2)
    EXPECT_EQ(limiter.getAvailableTokens(Priority::HIGH), 20u);
    // LOW bucket capacity is 5 (cap / 2)
    EXPECT_EQ(limiter.getAvailableTokens(Priority::LOW), 5u);
}

TEST_F(TokenBucketLocalTest, PriorityLaneIsolation) {
    TokenBucketRateLimiter limiter(makeCfg(5));
    // Exhaust NORMAL lane
    for (int i = 0; i < 5; ++i) {
      EXPECT_TRUE(limiter.tryAcquire(1, Priority::NORMAL));
    }
    EXPECT_FALSE(limiter.tryAcquire(1, Priority::NORMAL));
    // HIGH lane unaffected
    EXPECT_TRUE(limiter.tryAcquire(1, Priority::HIGH));
}

TEST_F(TokenBucketLocalTest, DisabledPriorityLanesSingleBucket) {
    auto cfg = makeCfg(10);
    cfg.enable_priority_lanes = false;
    TokenBucketRateLimiter limiter(cfg);

    for (int i = 0; i < 10; ++i) {
      EXPECT_TRUE(limiter.tryAcquire(1, Priority::HIGH));
    }
    EXPECT_FALSE(limiter.tryAcquire(1, Priority::HIGH));
}

TEST_F(TokenBucketLocalTest, MetricsCountRequests) {
    TokenBucketRateLimiter limiter(makeCfg(3));
    limiter.tryAcquire(1);
    limiter.tryAcquire(1);
    limiter.tryAcquire(1);
    limiter.tryAcquire(1); // rejected

    EXPECT_EQ(limiter.getTotalRequests(),   4u);
    EXPECT_EQ(limiter.getTotalRejections(), 1u);
}

TEST_F(TokenBucketLocalTest, ResetRestoresCapacity) {
    TokenBucketRateLimiter limiter(makeCfg(5));
    limiter.tryAcquire(5);
    EXPECT_FALSE(limiter.tryAcquire(1));

    limiter.reset();

    EXPECT_TRUE(limiter.tryAcquire(1));
    EXPECT_EQ(limiter.getTotalRequests(),   1u);
    EXPECT_EQ(limiter.getTotalRejections(), 0u);
}

TEST_F(TokenBucketLocalTest, ConcurrentAccessNeverExceedsCapacity) {
    const size_t capacity = 100;
    TokenBucketRateLimiter limiter(makeCfg(capacity, 0)); // refill_rate=0

    std::atomic<int> allowed{0};
    std::atomic<int> rejected{0};

    auto worker = [&]() {
        for (int i = 0; i < 50; ++i) {
            if (limiter.tryAcquire(1)) {
              ++allowed;
            }
            else                       ++rejected;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
      threads.emplace_back(worker);
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(allowed.load(), static_cast<int>(capacity));
    EXPECT_EQ(rejected.load(), 500 - static_cast<int>(capacity));
}

TEST_F(TokenBucketLocalTest, LocalBackendIsNotRedisHealthy) {
    TokenBucketRateLimiter limiter(makeCfg(10));
    EXPECT_FALSE(limiter.isRedisHealthy());
}

// ============================================================================
// TokenBucketRateLimiter – REDIS backend (no real Redis → fallback behaviour)
// ============================================================================

class TokenBucketRedisFallbackTest : public ::testing::Test {
protected:
    TokenBucketRateLimiter::Config makeRedisCfg(size_t cap = 10) {
        TokenBucketRateLimiter::Config cfg;
        cfg.capacity    = cap;
        cfg.refill_rate = 100;
        cfg.backend     = Backend::REDIS;
        cfg.enable_priority_lanes = false;
        // Point at a non-existent Redis to exercise the fallback path
        cfg.redis.host        = "127.0.0.1";
        cfg.redis.port        = 19379; // very likely not running
        cfg.redis.timeout_ms  = 100;
        cfg.redis.max_errors  = 2;
        cfg.redis.key_ttl_seconds = 60;
        cfg.bucket_id = "test_fallback";
        return cfg;
    }
};

TEST_F(TokenBucketRedisFallbackTest, ConstructionSucceedsWithoutRedis) {
    // Should not throw even when Redis is unreachable
    EXPECT_NO_THROW({
        TokenBucketRateLimiter limiter(makeRedisCfg());
    });
}

TEST_F(TokenBucketRedisFallbackTest, RedisUnhealthyAfterFailedConnect) {
    TokenBucketRateLimiter limiter(makeRedisCfg());
    // Redis is not reachable → should not be healthy
    EXPECT_FALSE(limiter.isRedisHealthy());
}

TEST_F(TokenBucketRedisFallbackTest, FallbackLocalBucketEnforced) {
    TokenBucketRateLimiter limiter(makeRedisCfg(5));
    // Local fallback with capacity 5
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(limiter.tryAcquire(1))
            << "Fallback bucket should allow request " << i;
    }
    EXPECT_FALSE(limiter.tryAcquire(1));
}

TEST_F(TokenBucketRedisFallbackTest, FallbackMetricsAccumulate) {
    TokenBucketRateLimiter limiter(makeRedisCfg(3));
    limiter.tryAcquire(1);
    limiter.tryAcquire(1);
    limiter.tryAcquire(1);
    limiter.tryAcquire(1); // rejected via fallback

    EXPECT_EQ(limiter.getTotalRequests(),   4u);
    EXPECT_EQ(limiter.getTotalRejections(), 1u);
}

// ============================================================================
// PerClientRateLimiter – LOCAL backend
// ============================================================================

class PerClientLocalTest : public ::testing::Test {
protected:
    PerClientRateLimiter::Config makeCfg(size_t cap = 5, size_t rate = 100) {
        PerClientRateLimiter::Config cfg;
        cfg.capacity_per_client    = cap;
        cfg.refill_rate_per_client = rate;
        cfg.max_clients            = 100;
        cfg.cleanup_interval       = std::chrono::minutes(10);
        cfg.backend                = Backend::LOCAL;
        return cfg;
    }
};

TEST_F(PerClientLocalTest, AllowsUpToCapacity) {
    PerClientRateLimiter limiter(makeCfg(5));
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(limiter.allowRequest("client_a"))
            << "Request " << i << " should be allowed";
    }
    EXPECT_FALSE(limiter.allowRequest("client_a"));
}

TEST_F(PerClientLocalTest, IndependentBucketsPerClient) {
    PerClientRateLimiter limiter(makeCfg(3));
    limiter.allowRequest("a");
    limiter.allowRequest("a");
    limiter.allowRequest("a");
    EXPECT_FALSE(limiter.allowRequest("a"));

    // Client b is unaffected
    EXPECT_TRUE(limiter.allowRequest("b"));
}

TEST_F(PerClientLocalTest, MaxClientsEnforced) {
    auto cfg = makeCfg(10);
    cfg.max_clients = 2;
    PerClientRateLimiter limiter(cfg);

    EXPECT_TRUE(limiter.allowRequest("x"));
    EXPECT_TRUE(limiter.allowRequest("y"));
    // 3rd distinct client rejected because max_clients=2 reached
    EXPECT_FALSE(limiter.allowRequest("z"));
}

TEST_F(PerClientLocalTest, ActiveClientsCount) {
    PerClientRateLimiter limiter(makeCfg());
    limiter.allowRequest("c1");
    limiter.allowRequest("c2");
    limiter.allowRequest("c2");
    EXPECT_EQ(limiter.getActiveClients(), 2u);
}

TEST_F(PerClientLocalTest, ClientMetricsAccurate) {
    PerClientRateLimiter limiter(makeCfg(3));
    limiter.allowRequest("m");
    limiter.allowRequest("m");
    limiter.allowRequest("m");
    limiter.allowRequest("m"); // rejected

    auto metrics = limiter.getClientMetrics("m");
    EXPECT_EQ(metrics.total_requests,   4u);
    EXPECT_EQ(metrics.total_rejections, 1u);
}

TEST_F(PerClientLocalTest, UnknownClientMetricsEmpty) {
    PerClientRateLimiter limiter(makeCfg());
    auto metrics = limiter.getClientMetrics("nonexistent");
    EXPECT_EQ(metrics.total_requests,   0u);
    EXPECT_EQ(metrics.total_rejections, 0u);
    EXPECT_EQ(metrics.available_tokens, 0u);
}

// ============================================================================
// PerClientRateLimiter – REDIS backend fallback (no real Redis)
// ============================================================================

class PerClientRedisFallbackTest : public ::testing::Test {
protected:
    PerClientRateLimiter::Config makeCfg(size_t cap = 5) {
        PerClientRateLimiter::Config cfg;
        cfg.capacity_per_client    = cap;
        cfg.refill_rate_per_client = 100;
        cfg.max_clients            = 100;
        cfg.cleanup_interval       = std::chrono::minutes(10);
        cfg.backend                = Backend::REDIS;
        cfg.redis.host             = "127.0.0.1";
        cfg.redis.port             = 19379; // no Redis running
        cfg.redis.timeout_ms       = 100;
        cfg.redis.max_errors       = 2;
        return cfg;
    }
};

TEST_F(PerClientRedisFallbackTest, ConstructionNoThrow) {
    EXPECT_NO_THROW({
        PerClientRateLimiter limiter(makeCfg());
    });
}

TEST_F(PerClientRedisFallbackTest, FallbackLocalBucketUsed) {
    PerClientRateLimiter limiter(makeCfg(4));
    for (int i = 0; i < 4; ++i) {
        EXPECT_TRUE(limiter.allowRequest("redis_client"))
            << "Fallback should allow request " << i;
    }
    EXPECT_FALSE(limiter.allowRequest("redis_client"));
}

TEST_F(PerClientRedisFallbackTest, FallbackIndependentClients) {
    PerClientRateLimiter limiter(makeCfg(2));
    EXPECT_TRUE(limiter.allowRequest("alpha"));
    EXPECT_TRUE(limiter.allowRequest("alpha"));
    EXPECT_FALSE(limiter.allowRequest("alpha"));

    EXPECT_TRUE(limiter.allowRequest("beta"));
}

// ============================================================================
// RedisRateLimiterConfig struct defaults
// ============================================================================

TEST(RedisRateLimiterConfigTest, Defaults) {
    RedisRateLimiterConfig cfg;
    EXPECT_EQ(cfg.host,             "127.0.0.1");
    EXPECT_EQ(cfg.port,             6379);
    EXPECT_TRUE(cfg.auth.empty());
    EXPECT_EQ(cfg.key_prefix,       "themis:rl");
    EXPECT_EQ(cfg.timeout_ms,       5000);
    EXPECT_EQ(cfg.max_errors,       3);
    EXPECT_EQ(cfg.key_ttl_seconds,  3600);
}
