/**
 * @file test_enterprise_scalability.cpp
 * @brief Tests for Enterprise Scalability features (Rate Limiting, Load Shedding, HTTP Pool)
 */

#include <gtest/gtest.h>
#include "server/rate_limiter_v2.h"
#include "server/load_shedder.h"
#include "utils/http_client_pool.h"
#include <thread>
#include <chrono>

using namespace themis::server;
using namespace themis::utils;

// ============================================================================
// TokenBucketRateLimiter Tests
// ============================================================================

TEST(TokenBucketRateLimiterTest, BasicAcquisition) {
    TokenBucketRateLimiter::Config config;
    config.capacity = 100;
    config.refill_rate = 10; // 10 tokens/sec
    config.enable_priority_lanes = false;
    
    TokenBucketRateLimiter limiter(config);
    
    // Should have full capacity initially
    EXPECT_TRUE(limiter.tryAcquire(50));
    EXPECT_TRUE(limiter.tryAcquire(50));
    
    // Bucket now empty
    EXPECT_FALSE(limiter.tryAcquire(1));
}

TEST(TokenBucketRateLimiterTest, Refill) {
    TokenBucketRateLimiter::Config config;
    config.capacity = 100;
    config.refill_rate = 100; // 100 tokens/sec
    config.enable_priority_lanes = false;
    
    TokenBucketRateLimiter limiter(config);
    
    // Drain bucket
    EXPECT_TRUE(limiter.tryAcquire(100));
    EXPECT_FALSE(limiter.tryAcquire(1));
    
    // Wait for refill (100 tokens/sec = 10 tokens per 100ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Should have ~20 tokens now
    EXPECT_TRUE(limiter.tryAcquire(15));
}

TEST(TokenBucketRateLimiterTest, PriorityLanes) {
    TokenBucketRateLimiter::Config config;
    config.capacity = 100;
    config.refill_rate = 10;
    config.enable_priority_lanes = true;
    
    TokenBucketRateLimiter limiter(config);
    
    // HIGH priority gets 50% of capacity (50 tokens)
    EXPECT_TRUE(limiter.tryAcquire(30, TokenBucketRateLimiter::Priority::HIGH));
    EXPECT_TRUE(limiter.tryAcquire(20, TokenBucketRateLimiter::Priority::HIGH));
    
    // NORMAL priority gets 30% of capacity (30 tokens)
    EXPECT_TRUE(limiter.tryAcquire(25, TokenBucketRateLimiter::Priority::NORMAL));
    
    // LOW priority gets 20% of capacity (20 tokens)
    EXPECT_TRUE(limiter.tryAcquire(15, TokenBucketRateLimiter::Priority::LOW));
}

TEST(TokenBucketRateLimiterTest, BurstHandling) {
    TokenBucketRateLimiter::Config config;
    config.capacity = 1000;  // Large burst allowance
    config.refill_rate = 100; // Sustained rate: 100/sec
    config.enable_priority_lanes = false;
    
    TokenBucketRateLimiter limiter(config);
    
    // Allow burst of 500 requests
    for (int i = 0; i < 500; ++i) {
        EXPECT_TRUE(limiter.tryAcquire(1));
    }
    
    // Still have 500 tokens left
    EXPECT_TRUE(limiter.tryAcquire(100));
}

TEST(TokenBucketRateLimiterTest, Reset) {
    TokenBucketRateLimiter::Config config;
    config.capacity = 10;
    config.refill_rate = 1;
    config.enable_priority_lanes = false;
    
    TokenBucketRateLimiter limiter(config);
    
    // Drain bucket
    EXPECT_TRUE(limiter.tryAcquire(10));
    EXPECT_FALSE(limiter.tryAcquire(1));
    
    // Reset
    limiter.reset();
    
    // Should have full capacity again
    EXPECT_TRUE(limiter.tryAcquire(10));
}

// ============================================================================
// PerClientRateLimiter Tests
// ============================================================================

TEST(PerClientRateLimiterTest, IndependentClients) {
    PerClientRateLimiter::Config config;
    config.capacity_per_client = 10;
    config.refill_rate_per_client = 1;
    
    PerClientRateLimiter limiter(config);
    
    // Client A can use its full quota
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(limiter.allowRequest("client_a"));
    }
    EXPECT_FALSE(limiter.allowRequest("client_a"));
    
    // Client B has independent quota
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(limiter.allowRequest("client_b"));
    }
    EXPECT_FALSE(limiter.allowRequest("client_b"));
}

TEST(PerClientRateLimiterTest, BasicFunctionality) {
    PerClientRateLimiter::Config config;
    config.capacity_per_client = 100;
    
    PerClientRateLimiter limiter(config);
    
    // Test that different clients work independently
    EXPECT_TRUE(limiter.allowRequest("client_1"));
    EXPECT_TRUE(limiter.allowRequest("client_2"));
    EXPECT_TRUE(limiter.allowRequest("client_3"));
}

TEST(PerClientRateLimiterTest, ExceedQuota) {
    PerClientRateLimiter::Config config;
    config.capacity_per_client = 5;
    
    PerClientRateLimiter limiter(config);
    
    // Exhaust quota
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(limiter.allowRequest("client_1"));
    }
    
    // Should be rate limited
    EXPECT_FALSE(limiter.allowRequest("client_1"));
}

// ============================================================================
// LoadShedder Tests
// ============================================================================

TEST(LoadShedderTest, NormalLoad) {
    LoadShedder::Config config;
    config.cpu_threshold = 0.95;
    config.memory_threshold = 0.90;
    config.enable_shedding = true;
    
    LoadShedder shedder(config);
    
    // Normal load (50% CPU, 50% memory)
    shedder.updateLoad(0.5, 0.5, 100);
    
    // All priorities should be accepted
    EXPECT_FALSE(shedder.shouldReject(LoadShedder::Priority::HIGH));
    EXPECT_FALSE(shedder.shouldReject(LoadShedder::Priority::NORMAL));
    EXPECT_FALSE(shedder.shouldReject(LoadShedder::Priority::LOW));
}

TEST(LoadShedderTest, HighLoadRejectsLow) {
    LoadShedder::Config config;
    config.cpu_threshold = 0.95;
    config.enable_shedding = true;
    
    LoadShedder shedder(config);
    
    // High load (90% CPU, 90% memory, high queue depth)
    // Formula: (0.90 * 0.5) + (0.90 * 0.3) + (800/1000 * 0.2) = 0.45 + 0.27 + 0.16 = 0.88
    shedder.updateLoad(0.90, 0.90, 800);
    
    // LOW priority should be rejected (threshold 0.80)
    EXPECT_FALSE(shedder.shouldReject(LoadShedder::Priority::HIGH));
    EXPECT_FALSE(shedder.shouldReject(LoadShedder::Priority::NORMAL));
    EXPECT_TRUE(shedder.shouldReject(LoadShedder::Priority::LOW));
}

TEST(LoadShedderTest, CriticalLoadRejectsNormal) {
    LoadShedder::Config config;
    config.cpu_threshold = 0.95;
    config.enable_shedding = true;
    
    LoadShedder shedder(config);
    
    // Critical load (98% CPU, 95% memory)
    shedder.updateLoad(0.98, 0.95, 1000);
    
    // NORMAL and LOW should be rejected
    EXPECT_FALSE(shedder.shouldReject(LoadShedder::Priority::HIGH));
    EXPECT_TRUE(shedder.shouldReject(LoadShedder::Priority::NORMAL));
    EXPECT_TRUE(shedder.shouldReject(LoadShedder::Priority::LOW));
}

TEST(LoadShedderTest, DisabledShedding) {
    LoadShedder::Config config;
    config.enable_shedding = false;
    
    LoadShedder shedder(config);
    
    // Even at 100% load, nothing should be rejected
    shedder.updateLoad(1.0, 1.0, 10000);
    
    EXPECT_FALSE(shedder.shouldReject(LoadShedder::Priority::HIGH));
    EXPECT_FALSE(shedder.shouldReject(LoadShedder::Priority::NORMAL));
    EXPECT_FALSE(shedder.shouldReject(LoadShedder::Priority::LOW));
}

TEST(LoadShedderTest, GetCurrentLoad) {
    LoadShedder::Config config;
    
    LoadShedder shedder(config);
    
    // Test load calculation
    shedder.updateLoad(0.6, 0.4, 500); // 500 queue depth
    
    // Expected: (0.6 * 0.5) + (0.4 * 0.3) + (0.5 * 0.2) = 0.52
    double load = shedder.getCurrentLoad();
    EXPECT_NEAR(load, 0.52, 0.05); // Allow small deviation
}

// ============================================================================
// HTTPClientPool Tests
// ============================================================================

TEST(HTTPClientPoolTest, BasicPooling) {
    HTTPClientPool::Config config;
    config.max_connections = 10;
    
    HTTPClientPool pool(config);
    
    auto stats = pool.getStats();
    EXPECT_EQ(stats.total_connections, 0);
    EXPECT_EQ(stats.available_connections, 0);
    EXPECT_EQ(stats.in_use_connections, 0);
}

TEST(HTTPClientPoolTest, AsyncPost) {
    HTTPClientPool::Config config;
    config.max_connections = 5;
    config.connect_timeout = std::chrono::seconds(3);
    config.request_timeout = std::chrono::seconds(5);
    
    HTTPClientPool pool(config);
    
    // Test with httpbin.org echo service
    json request_body = {
        {"test", "data"},
        {"number", 42}
    };
    
    try {
        auto future = pool.post("https://httpbin.org/post", request_body);
        
        // Wait for response (max 10 seconds)
        auto status = future.wait_for(std::chrono::seconds(10));
        
        if (status == std::future_status::ready) {
            auto response = future.get();
            
            EXPECT_TRUE(response.isSuccess());
            EXPECT_EQ(response.status_code, 200);
            
            // httpbin.org returns JSON with our data
            auto response_json = json::parse(response.body);
            EXPECT_TRUE(response_json.contains("json"));
            EXPECT_EQ(response_json["json"]["test"], "data");
            EXPECT_EQ(response_json["json"]["number"], 42);
        } else {
            // Timeout - skip test (network issue)
            GTEST_SKIP() << "HTTP request timed out - network may be unavailable";
        }
    } catch (const std::exception& e) {
        // Network error - skip test
        GTEST_SKIP() << "HTTP request failed: " << e.what();
    }
}

TEST(HTTPClientPoolTest, AsyncGet) {
    HTTPClientPool::Config config;
    config.max_connections = 5;
    
    HTTPClientPool pool(config);
    
    try {
        auto future = pool.get("https://httpbin.org/get");
        
        auto status = future.wait_for(std::chrono::seconds(10));
        
        if (status == std::future_status::ready) {
            auto response = future.get();
            
            EXPECT_TRUE(response.isSuccess());
            EXPECT_EQ(response.status_code, 200);
            
            // httpbin.org returns JSON
            auto response_json = json::parse(response.body);
            EXPECT_TRUE(response_json.contains("url"));
        } else {
            GTEST_SKIP() << "HTTP request timed out - network may be unavailable";
        }
    } catch (const std::exception& e) {
        GTEST_SKIP() << "HTTP request failed: " << e.what();
    }
}

TEST(HTTPClientPoolTest, HTTPSSupport) {
    HTTPClientPool::Config config;
    config.max_connections = 5;
    
    HTTPClientPool pool(config);
    
    try {
        // Test HTTPS with SSL/TLS
        auto future = pool.get("https://www.google.com");
        
        auto status = future.wait_for(std::chrono::seconds(10));
        
        if (status == std::future_status::ready) {
            auto response = future.get();
            
            // Google may return 200 or redirect (301/302)
            EXPECT_TRUE(response.status_code == 200 || 
                       response.status_code == 301 || 
                       response.status_code == 302);
        } else {
            GTEST_SKIP() << "HTTPS request timed out";
        }
    } catch (const std::exception& e) {
        GTEST_SKIP() << "HTTPS request failed: " << e.what();
    }
}

TEST(HTTPClientPoolTest, ConnectionReuse) {
    HTTPClientPool::Config config;
    config.max_connections = 2;  // Small pool to force reuse
    
    HTTPClientPool pool(config);
    
    try {
        // Make multiple requests
        std::vector<std::future<HTTPResponse>> futures;
        
        for (int i = 0; i < 5; ++i) {
            futures.push_back(pool.get("https://httpbin.org/get"));
        }
        
        // Wait for all
        int successful = 0;
        for (auto& f : futures) {
            auto status = f.wait_for(std::chrono::seconds(10));
            if (status == std::future_status::ready) {
                auto response = f.get();
                if (response.isSuccess()) {
                    successful++;
                }
            }
        }
        
        // At least some should succeed (network permitting)
        EXPECT_GT(successful, 0);
        
        // Pool should have created connections
        auto stats = pool.getStats();
        EXPECT_GT(stats.total_connections, 0);
        EXPECT_LE(stats.total_connections, config.max_connections);
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Connection pooling test failed: " << e.what();
    }
}

TEST(HTTPClientPoolTest, Clear) {
    HTTPClientPool::Config config;
    config.max_connections = 10;
    
    HTTPClientPool pool(config);
    
    try {
        // Make some requests to populate pool
        auto f1 = pool.get("https://httpbin.org/get");
        auto f2 = pool.get("https://httpbin.org/get");
        
        auto s1 = f1.wait_for(std::chrono::seconds(10));
        auto s2 = f2.wait_for(std::chrono::seconds(10));
        
        if (s1 == std::future_status::ready) f1.get();
        if (s2 == std::future_status::ready) f2.get();
        
        // Clear pool
        pool.clear();
        
        auto stats = pool.getStats();
        EXPECT_EQ(stats.total_connections, 0);
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Clear test failed: " << e.what();
    }
}

// ============================================================================
// Integration Test: Rate Limiter + Load Shedder
// ============================================================================

TEST(EnterpriseScalabilityTest, RateLimiterWithLoadShedder) {
    // Setup rate limiter
    TokenBucketRateLimiter::Config limiter_config;
    limiter_config.capacity = 100;
    limiter_config.refill_rate = 10;
    limiter_config.enable_priority_lanes = true;
    
    TokenBucketRateLimiter limiter(limiter_config);
    
    // Setup load shedder
    LoadShedder::Config shedder_config;
    shedder_config.enable_shedding = true;
    
    LoadShedder shedder(shedder_config);
    
    // Simulate normal load
    shedder.updateLoad(0.5, 0.5, 100);
    
    // Process requests
    int accepted = 0;
    int rejected_rate_limit = 0;
    int rejected_load = 0;
    
    for (int i = 0; i < 200; ++i) {
        auto prio = (i % 3 == 0) ? TokenBucketRateLimiter::Priority::HIGH :
                    (i % 3 == 1) ? TokenBucketRateLimiter::Priority::NORMAL :
                                   TokenBucketRateLimiter::Priority::LOW;
        
        // Check load shedding first
        if (shedder.shouldReject(
            prio == TokenBucketRateLimiter::Priority::HIGH ? LoadShedder::Priority::HIGH :
            prio == TokenBucketRateLimiter::Priority::NORMAL ? LoadShedder::Priority::NORMAL :
            LoadShedder::Priority::LOW
        )) {
            rejected_load++;
            continue;
        }
        
        // Check rate limit
        if (!limiter.tryAcquire(1, prio)) {
            rejected_rate_limit++;
            continue;
        }
        
        accepted++;
    }
    
    // At normal load, no load shedding should occur
    EXPECT_EQ(rejected_load, 0);
    
    // Rate limiting should kick in after ~100 requests
    EXPECT_GT(rejected_rate_limit, 50);
    EXPECT_LT(accepted, 150);
}
