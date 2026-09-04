#include <gtest/gtest.h>
#include "server/rate_limiting_middleware.h"
#include <thread>
#include <atomic>
#include <chrono>

using namespace themis::server;

// ============================================================================
// Fixture
// ============================================================================

class RateLimitingMiddlewareTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.default_capacity    = 10;
        config_.default_refill_rate = 10.0;  // 10 req/s (fast for tests)
        config_.whitelist_ips.clear();
        config_.endpoint_overrides.clear();
        config_.send_rate_limit_headers = true;
    }

    RateLimitingMiddleware::Config config_;
};

// ============================================================================
// Basic allow / deny
// ============================================================================

TEST_F(RateLimitingMiddlewareTest, AllowsRequestsUpToCapacity) {
    RateLimitingMiddleware mw(config_);

    for (int i = 0; i < 10; ++i) {
        auto result = mw.check("client1", "/api/v1/docs");
        EXPECT_TRUE(result.allowed) << "request " << i << " should be allowed";
    }
}

TEST_F(RateLimitingMiddlewareTest, RejectsRequestBeyondCapacity) {
    RateLimitingMiddleware mw(config_);

    // Exhaust all tokens
    for (int i = 0; i < 10; ++i) {
        mw.check("client1", "/api/v1/docs");
    }
    auto result = mw.check("client1", "/api/v1/docs");
    EXPECT_FALSE(result.allowed);
    EXPECT_GT(result.retry_after_seconds, 0u);
}

TEST_F(RateLimitingMiddlewareTest, IndependentBucketsPerClient) {
    RateLimitingMiddleware mw(config_);

    // Exhaust client1's bucket
    for (int i = 0; i < 10; ++i) {
        mw.check("client1", "/api");
    }
    // client2 should still be allowed
    EXPECT_TRUE(mw.check("client2", "/api").allowed);
}

// ============================================================================
// Whitelist
// ============================================================================

TEST_F(RateLimitingMiddlewareTest, WhitelistedClientIsNeverRateLimited) {
    config_.whitelist_ips = {"127.0.0.1"};
    RateLimitingMiddleware mw(config_);

    for (int i = 0; i < 1000; ++i) {
        auto result = mw.check("127.0.0.1", "/api/v1/docs");
        EXPECT_TRUE(result.allowed) << "whitelisted client blocked at i=" << i;
    }
}

TEST_F(RateLimitingMiddlewareTest, NonWhitelistedClientIsRateLimited) {
    config_.whitelist_ips = {"127.0.0.1"};
    RateLimitingMiddleware mw(config_);

    for (int i = 0; i < 10; ++i) {
        mw.check("10.0.0.1", "/api");
    }
    EXPECT_FALSE(mw.check("10.0.0.1", "/api").allowed);
}

// ============================================================================
// Per-endpoint overrides
// ============================================================================

TEST_F(RateLimitingMiddlewareTest, EndpointOverrideIsApplied) {
    config_.default_capacity = 20;
    config_.endpoint_overrides = {
        RateLimitingMiddleware::EndpointLimit{"/v2/documents", 5, 5.0}
    };
    RateLimitingMiddleware mw(config_);

    // /v2/documents should be limited to 5
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(mw.check("client1", "/v2/documents").allowed)
            << "request " << i << " to /v2/documents should be allowed";
    }
    EXPECT_FALSE(mw.check("client1", "/v2/documents").allowed);

    // /api/v1/docs uses the default limit (20)
    for (int i = 0; i < 20; ++i) {
        EXPECT_TRUE(mw.check("client1", "/api/v1/docs").allowed)
            << "request " << i << " to /api/v1/docs should be allowed";
    }
    EXPECT_FALSE(mw.check("client1", "/api/v1/docs").allowed);
}

TEST_F(RateLimitingMiddlewareTest, LongestPrefixMatchWins) {
    config_.endpoint_overrides = {
        RateLimitingMiddleware::EndpointLimit{"/v2",           10, 10.0},
        RateLimitingMiddleware::EndpointLimit{"/v2/documents",  3,  3.0},
    };
    RateLimitingMiddleware mw(config_);

    // /v2/documents/123 should match the more-specific /v2/documents prefix
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(mw.check("c1", "/v2/documents/123").allowed)
            << "i=" << i;
    }
    EXPECT_FALSE(mw.check("c1", "/v2/documents/123").allowed);
}

TEST_F(RateLimitingMiddlewareTest, PrefixBoundaryRespected) {
    config_.endpoint_overrides = {
        RateLimitingMiddleware::EndpointLimit{"/v2/doc", 2, 2.0}
    };
    RateLimitingMiddleware mw(config_);

    // /v2/documents should NOT match /v2/doc (no boundary after "doc")
    // It falls through to default capacity (10)
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(mw.check("c1", "/v2/documents").allowed)
            << "i=" << i;
    }
}

// ============================================================================
// Response headers
// ============================================================================

TEST_F(RateLimitingMiddlewareTest, HeadersSentOnSuccess) {
    RateLimitingMiddleware mw(config_);
    auto result = mw.check("client1", "/api");

    EXPECT_TRUE(result.allowed);
    EXPECT_NE(result.headers.count("X-RateLimit-Limit"),     0u);
    EXPECT_NE(result.headers.count("X-RateLimit-Remaining"), 0u);
    EXPECT_EQ(result.headers.count("Retry-After"),            0u);
}

TEST_F(RateLimitingMiddlewareTest, HeadersSentOnRejection) {
    RateLimitingMiddleware mw(config_);
    for (int i = 0; i < 10; ++i) {
      mw.check("c1", "/api");
    }

    auto result = mw.check("c1", "/api");
    EXPECT_FALSE(result.allowed);
    EXPECT_NE(result.headers.count("X-RateLimit-Limit"),     0u);
    EXPECT_NE(result.headers.count("X-RateLimit-Remaining"), 0u);
    EXPECT_NE(result.headers.count("Retry-After"),            0u);
}

TEST_F(RateLimitingMiddlewareTest, HeadersNotSentWhenDisabled) {
    config_.send_rate_limit_headers = false;
    RateLimitingMiddleware mw(config_);
    auto result = mw.check("c1", "/api");

    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(result.headers.count("X-RateLimit-Limit"),     0u);
    EXPECT_EQ(result.headers.count("X-RateLimit-Remaining"), 0u);
}

TEST_F(RateLimitingMiddlewareTest, LimitHeaderReflectsCapacity) {
    config_.default_capacity = 42;
    RateLimitingMiddleware mw(config_);
    auto result = mw.check("c1", "/api");

    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(result.headers.at("X-RateLimit-Limit"), "42");
}

// ============================================================================
// Statistics
// ============================================================================

TEST_F(RateLimitingMiddlewareTest, StatsTrackRequests) {
    RateLimitingMiddleware mw(config_);

    for (int i = 0; i < 10; ++i) {
      mw.check("c1", "/api");
    }
    for (int i = 0; i < 5; ++i)  mw.check("c1", "/api");  // these are rejected

    auto stats = mw.getStats();
    EXPECT_EQ(stats.total_requests,    15u);
    EXPECT_EQ(stats.allowed_requests,  10u);
    EXPECT_EQ(stats.rejected_requests,  5u);
}

TEST_F(RateLimitingMiddlewareTest, StatsResetOnReset) {
    RateLimitingMiddleware mw(config_);
    for (int i = 0; i < 5; ++i) {
      mw.check("c1", "/api");
    }

    mw.reset();

    auto stats = mw.getStats();
    EXPECT_EQ(stats.total_requests,   0u);
    EXPECT_EQ(stats.allowed_requests, 0u);
    EXPECT_TRUE(mw.check("c1", "/api").allowed); // should work again
}

// ============================================================================
// updateConfig
// ============================================================================

TEST_F(RateLimitingMiddlewareTest, UpdateConfigAppliesNewLimits) {
    RateLimitingMiddleware mw(config_);

    // Exhaust initial capacity (10)
    for (int i = 0; i < 10; ++i) {
      mw.check("c1", "/api");
    }
    EXPECT_FALSE(mw.check("c1", "/api").allowed);

    // Increase capacity via updateConfig
    config_.default_capacity = 50;
    mw.updateConfig(config_);

    // Buckets are cleared; new client should have 50 tokens
    for (int i = 0; i < 50; ++i) {
        EXPECT_TRUE(mw.check("c2", "/api").allowed) << "i=" << i;
    }
    EXPECT_FALSE(mw.check("c2", "/api").allowed);
}

// ============================================================================
// Token refill
// ============================================================================

TEST_F(RateLimitingMiddlewareTest, TokensRefillOverTime) {
    config_.default_capacity    = 5;
    config_.default_refill_rate = 5.0;   // 5 t/s
    RateLimitingMiddleware mw(config_);

    // Drain the bucket
    for (int i = 0; i < 5; ++i) {
      mw.check("c1", "/api");
    }
    EXPECT_FALSE(mw.check("c1", "/api").allowed);

    // Wait for at least 1 token to refill (~200 ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    EXPECT_TRUE(mw.check("c1", "/api").allowed);
}

// ============================================================================
// Concurrency
// ============================================================================

TEST_F(RateLimitingMiddlewareTest, ConcurrentAccessIsSafe) {
    config_.default_capacity = 50;
    RateLimitingMiddleware mw(config_);

    std::atomic<int> allowed{0};
    std::atomic<int> rejected{0};

    auto worker = [&]() {
        for (int i = 0; i < 20; ++i) {
            if (mw.check("shared_client", "/api").allowed) {
              ++allowed;
            }
            else ++rejected;
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < 5; ++t) {
      threads.emplace_back(worker);
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(allowed.load() + rejected.load(), 100);
    // Exactly 50 should be allowed (token bucket capacity)
    EXPECT_EQ(allowed.load(), 50);
    EXPECT_EQ(rejected.load(), 50);
}

// ============================================================================
// getConfig round-trip
// ============================================================================

TEST_F(RateLimitingMiddlewareTest, GetConfigReturnsCurrentConfig) {
    config_.default_capacity    = 77;
    config_.default_refill_rate = 3.5;
    config_.whitelist_ips       = {"10.0.0.1"};
    RateLimitingMiddleware mw(config_);

    auto got = mw.getConfig();
    EXPECT_EQ(got.default_capacity,    77u);
    EXPECT_DOUBLE_EQ(got.default_refill_rate, 3.5);
    ASSERT_EQ(got.whitelist_ips.size(), 1u);
    EXPECT_EQ(got.whitelist_ips[0], "10.0.0.1");
}
