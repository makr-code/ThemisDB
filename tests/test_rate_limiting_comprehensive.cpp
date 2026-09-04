/**
 * @file test_rate_limiting_comprehensive.cpp
 * @brief Comprehensive tests for the RateLimiter security component
 *
 * Tests cover:
 * - Token bucket algorithm (capacity, refill, burst)
 * - Per-IP rate limiting
 * - Per-user rate limiting
 * - IP whitelisting
 * - IP blacklisting (newly added feature)
 * - Statistics tracking
 * - Configuration update
 * - Reset and cleanup
 * - Concurrency
 * - Edge cases
 */

#include <gtest/gtest.h>
#include "server/rate_limiter.h"
#include <thread>
#include <atomic>
#include <chrono>

using namespace themis::server;

// ============================================================================
// TokenBucket Tests
// ============================================================================

class TokenBucketTest : public ::testing::Test {};

TEST_F(TokenBucketTest, NewBucket_FullCapacity) {
    TokenBucket bucket(10, 1.0);
    EXPECT_DOUBLE_EQ(bucket.getTokens(), 10.0);
}

TEST_F(TokenBucketTest, ConsumeToken_Succeeds_WhenTokensAvailable) {
    TokenBucket bucket(10, 1.0);
    EXPECT_TRUE(bucket.tryConsume(1));
    EXPECT_TRUE(bucket.tryConsume(1));
}

TEST_F(TokenBucketTest, ConsumeAll_ThenFails) {
    TokenBucket bucket(3, 0.1); // Very slow refill
    EXPECT_TRUE(bucket.tryConsume(1));
    EXPECT_TRUE(bucket.tryConsume(1));
    EXPECT_TRUE(bucket.tryConsume(1));
    EXPECT_FALSE(bucket.tryConsume(1)); // Bucket empty
}

TEST_F(TokenBucketTest, ConsumeMoreThanCapacity_Fails) {
    TokenBucket bucket(5, 1.0);
    EXPECT_FALSE(bucket.tryConsume(10)); // More than capacity
}

TEST_F(TokenBucketTest, Reset_RestoresFullCapacity) {
    TokenBucket bucket(5, 0.01); // Very slow refill
    EXPECT_TRUE(bucket.tryConsume(5)); // Drain it
    EXPECT_FALSE(bucket.tryConsume(1)); // Now empty

    bucket.reset();
    EXPECT_TRUE(bucket.tryConsume(1)); // Should work after reset
}

TEST_F(TokenBucketTest, RetryAfter_ZeroWhenTokensAvailable) {
    TokenBucket bucket(10, 1.0);
    EXPECT_EQ(bucket.getRetryAfterMs(), 0u);
}

TEST_F(TokenBucketTest, RetryAfter_NonZeroWhenEmpty) {
    TokenBucket bucket(2, 0.5); // 0.5 tokens/sec
    bucket.tryConsume(2); // Drain
    EXPECT_GT(bucket.getRetryAfterMs(), 0u);
}

// ============================================================================
// RateLimiter Tests
// ============================================================================

class RateLimiterTest : public ::testing::Test {
protected:
    void SetUp() override {
        RateLimitConfig cfg;
        cfg.bucket_capacity = 5;
        cfg.refill_rate = 0.01; // Very slow refill for test isolation
        cfg.per_ip_enabled = true;
        cfg.per_user_enabled = true;
        limiter_ = std::make_unique<RateLimiter>(cfg);
    }

    std::unique_ptr<RateLimiter> limiter_;
};

TEST_F(RateLimiterTest, AllowRequest_WithinLimit) {
    EXPECT_TRUE(limiter_->allowRequest("1.2.3.4"));
    EXPECT_TRUE(limiter_->allowRequest("1.2.3.4"));
    EXPECT_TRUE(limiter_->allowRequest("1.2.3.4"));
}

TEST_F(RateLimiterTest, AllowRequest_ExceedsLimit_Rejected) {
    // Exhaust the bucket
    for (int i = 0; i < 5; ++i) {
        limiter_->allowRequest("5.5.5.5");
    }
    // Should be rejected now
    EXPECT_FALSE(limiter_->allowRequest("5.5.5.5"));
}

TEST_F(RateLimiterTest, DifferentIPs_IndependentBuckets) {
    // Exhaust bucket for IP1
    for (int i = 0; i < 5; ++i) {
        limiter_->allowRequest("10.0.0.1");
    }
    EXPECT_FALSE(limiter_->allowRequest("10.0.0.1")); // IP1 exhausted

    // IP2 should still work
    EXPECT_TRUE(limiter_->allowRequest("10.0.0.2"));
}

TEST_F(RateLimiterTest, PerUserRateLimit_Works) {
    RateLimitConfig cfg;
    cfg.bucket_capacity = 3;
    cfg.refill_rate = 0.01;
    cfg.per_ip_enabled = false;
    cfg.per_user_enabled = true;
    RateLimiter limiter(cfg);

    EXPECT_TRUE(limiter.allowRequest("", "user1"));
    EXPECT_TRUE(limiter.allowRequest("", "user1"));
    EXPECT_TRUE(limiter.allowRequest("", "user1"));
    EXPECT_FALSE(limiter.allowRequest("", "user1")); // user1 exhausted
    EXPECT_TRUE(limiter.allowRequest("", "user2"));  // user2 ok
}

TEST_F(RateLimiterTest, WhitelistedIP_AlwaysAllowed) {
    RateLimitConfig cfg;
    cfg.bucket_capacity = 1; // Very restrictive
    cfg.refill_rate = 0.001;
    cfg.per_ip_enabled = true;
    cfg.whitelist_ips = {"trusted-host"};
    RateLimiter limiter(cfg);

    // Whitelisted IP should bypass rate limiting
    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(limiter.allowRequest("trusted-host"));
    }
}

TEST_F(RateLimiterTest, IsWhitelisted_ReturnsCorrect) {
    RateLimitConfig cfg;
    cfg.whitelist_ips = {"10.0.0.1", "192.168.1.100"};
    RateLimiter limiter(cfg);

    EXPECT_TRUE(limiter.isWhitelisted("10.0.0.1"));
    EXPECT_TRUE(limiter.isWhitelisted("192.168.1.100"));
    EXPECT_FALSE(limiter.isWhitelisted("8.8.8.8"));
    EXPECT_FALSE(limiter.isWhitelisted(""));
}

// ============================================================================
// IP Blacklisting Tests
// ============================================================================

class IPBlacklistTest : public ::testing::Test {
protected:
    void SetUp() override {
        RateLimitConfig cfg;
        cfg.bucket_capacity = 100;
        cfg.refill_rate = 100.0;
        cfg.per_ip_enabled = true;
        limiter_ = std::make_unique<RateLimiter>(cfg);
    }

    std::unique_ptr<RateLimiter> limiter_;
};

TEST_F(IPBlacklistTest, BlacklistedIP_ImmediatelyBlocked) {
    // IP initially allowed
    EXPECT_TRUE(limiter_->allowRequest("attacker.ip"));

    // Blacklist the IP
    limiter_->blacklistIP("attacker.ip");

    // Now blocked
    EXPECT_FALSE(limiter_->allowRequest("attacker.ip"));
}

TEST_F(IPBlacklistTest, NonBlacklistedIP_StillAllowed) {
    limiter_->blacklistIP("bad.ip");

    // Other IPs should be unaffected
    EXPECT_TRUE(limiter_->allowRequest("good.ip"));
    EXPECT_TRUE(limiter_->allowRequest("another.ip"));
}

TEST_F(IPBlacklistTest, IsBlacklisted_ReturnsCorrect) {
    EXPECT_FALSE(limiter_->isBlacklisted("some.ip"));

    limiter_->blacklistIP("some.ip");
    EXPECT_TRUE(limiter_->isBlacklisted("some.ip"));

    limiter_->unblacklistIP("some.ip");
    EXPECT_FALSE(limiter_->isBlacklisted("some.ip"));
}

TEST_F(IPBlacklistTest, UnblacklistedIP_AllowedAgain) {
    limiter_->blacklistIP("temp.banned.ip");
    EXPECT_FALSE(limiter_->allowRequest("temp.banned.ip"));

    limiter_->unblacklistIP("temp.banned.ip");
    EXPECT_TRUE(limiter_->allowRequest("temp.banned.ip"));
}

TEST_F(IPBlacklistTest, BlacklistTakesPrecedenceOverWhitelist) {
    RateLimitConfig cfg;
    cfg.bucket_capacity = 100;
    cfg.refill_rate = 100.0;
    cfg.whitelist_ips = {"special.ip"};
    RateLimiter limiter(cfg);

    // Initially whitelisted, allowed
    EXPECT_TRUE(limiter.allowRequest("special.ip"));

    // Blacklist overrides whitelist
    limiter.blacklistIP("special.ip");
    EXPECT_FALSE(limiter.allowRequest("special.ip"));
}

TEST_F(IPBlacklistTest, MultipleIPsBlacklisted) {
    limiter_->blacklistIP("ip1");
    limiter_->blacklistIP("ip2");
    limiter_->blacklistIP("ip3");

    EXPECT_FALSE(limiter_->allowRequest("ip1"));
    EXPECT_FALSE(limiter_->allowRequest("ip2"));
    EXPECT_FALSE(limiter_->allowRequest("ip3"));
    EXPECT_TRUE(limiter_->allowRequest("ip4"));
}

TEST_F(IPBlacklistTest, Reset_ClearsBlacklist) {
    limiter_->blacklistIP("bad.ip");
    ASSERT_FALSE(limiter_->allowRequest("bad.ip"));

    limiter_->reset();
    EXPECT_TRUE(limiter_->allowRequest("bad.ip"));
}

TEST_F(IPBlacklistTest, BlacklistedIP_StatsTrackAsRejected) {
    limiter_->blacklistIP("stats.test.ip");
    limiter_->allowRequest("stats.test.ip");
    limiter_->allowRequest("stats.test.ip");

    auto stats = limiter_->getStatistics();
    EXPECT_GE(stats.rejected_requests, 2u);
}

// ============================================================================
// Statistics Tests
// ============================================================================

class RateLimiterStatsTest : public ::testing::Test {
protected:
    void SetUp() override {
        RateLimitConfig cfg;
        cfg.bucket_capacity = 3;
        cfg.refill_rate = 0.01;
        cfg.per_ip_enabled = true;
        cfg.per_user_enabled = false;
        limiter_ = std::make_unique<RateLimiter>(cfg);
    }

    std::unique_ptr<RateLimiter> limiter_;
};

TEST_F(RateLimiterStatsTest, TotalRequests_Tracked) {
    limiter_->allowRequest("ip1");
    limiter_->allowRequest("ip2");
    limiter_->allowRequest("ip1");

    auto stats = limiter_->getStatistics();
    EXPECT_EQ(stats.total_requests, 3u);
}

TEST_F(RateLimiterStatsTest, AllowedRequests_Tracked) {
    limiter_->allowRequest("ip1"); // allowed
    limiter_->allowRequest("ip1"); // allowed
    limiter_->allowRequest("ip1"); // allowed

    auto stats = limiter_->getStatistics();
    EXPECT_EQ(stats.allowed_requests, 3u);
}

TEST_F(RateLimiterStatsTest, RejectedRequests_Tracked) {
    // Exhaust bucket
    for (int i = 0; i < 3; ++i) {
        limiter_->allowRequest("limit.ip");
    }
    limiter_->allowRequest("limit.ip"); // rejected
    limiter_->allowRequest("limit.ip"); // rejected

    auto stats = limiter_->getStatistics();
    EXPECT_GE(stats.rejected_requests, 2u);
}

TEST_F(RateLimiterStatsTest, ActiveIPBuckets_Tracked) {
    limiter_->allowRequest("ip-a");
    limiter_->allowRequest("ip-b");
    limiter_->allowRequest("ip-c");

    auto stats = limiter_->getStatistics();
    EXPECT_GE(stats.active_ip_buckets, 3u);
}

TEST_F(RateLimiterStatsTest, Reset_ClearsStats) {
    limiter_->allowRequest("ip1");
    limiter_->allowRequest("ip2");

    limiter_->reset();

    auto stats = limiter_->getStatistics();
    EXPECT_EQ(stats.total_requests, 0u);
    EXPECT_EQ(stats.allowed_requests, 0u);
    EXPECT_EQ(stats.rejected_requests, 0u);
    EXPECT_EQ(stats.active_ip_buckets, 0u);
}

// ============================================================================
// Configuration Update Tests
// ============================================================================

TEST(RateLimiterConfigTest, UpdateConfig_ChangesLimits) {
    RateLimitConfig cfg;
    cfg.bucket_capacity = 2;
    cfg.refill_rate = 0.01;
    RateLimiter limiter(cfg);

    // With capacity 2
    EXPECT_TRUE(limiter.allowRequest("ip1"));
    EXPECT_TRUE(limiter.allowRequest("ip1"));
    EXPECT_FALSE(limiter.allowRequest("ip1"));

    // Update to higher capacity
    RateLimitConfig new_cfg;
    new_cfg.bucket_capacity = 100;
    new_cfg.refill_rate = 100.0;
    limiter.updateConfig(new_cfg);

    // New IP should get new capacity
    EXPECT_TRUE(limiter.allowRequest("ip2"));
    EXPECT_TRUE(limiter.allowRequest("ip2"));
    EXPECT_TRUE(limiter.allowRequest("ip2"));
}

TEST(RateLimiterConfigTest, RetryAfter_ReturnsNonZeroWhenLimited) {
    RateLimitConfig cfg;
    cfg.bucket_capacity = 1;
    cfg.refill_rate = 0.001; // Very slow
    RateLimiter limiter(cfg);

    limiter.allowRequest("slow.ip"); // Drain
    EXPECT_FALSE(limiter.allowRequest("slow.ip")); // Rejected

    uint32_t retry = limiter.getRetryAfter("slow.ip");
    EXPECT_GT(retry, 0u);
}

TEST(RateLimiterConfigTest, RetryAfter_ZeroWhenNotLimited) {
    RateLimitConfig cfg;
    cfg.bucket_capacity = 100;
    cfg.refill_rate = 100.0;
    RateLimiter limiter(cfg);

    uint32_t retry = limiter.getRetryAfter("fresh.ip");
    EXPECT_EQ(retry, 0u);
}

// ============================================================================
// Concurrency Tests
// ============================================================================

TEST(RateLimiterConcurrencyTest, ConcurrentRequests_ThreadSafe) {
    RateLimitConfig cfg;
    cfg.bucket_capacity = 1000;
    cfg.refill_rate = 1000.0;
    RateLimiter limiter(cfg);

    constexpr int THREADS = 8;
    constexpr int REQUESTS = 50;
    std::atomic<int> total_allowed{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back([&limiter, &total_allowed, i]() {
            std::string ip = "thread-ip-" + std::to_string(i);
            for (int j = 0; j < REQUESTS; ++j) {
                if (limiter.allowRequest(ip)) {
                    total_allowed++;
                }
            }
        });
    }

    for (auto& t : threads) {
      t.join();
    }

    auto stats = limiter.getStatistics();
    EXPECT_EQ(stats.total_requests, static_cast<size_t>(THREADS * REQUESTS));
    EXPECT_EQ(stats.total_requests, stats.allowed_requests + stats.rejected_requests);
}
