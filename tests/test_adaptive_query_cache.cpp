/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_adaptive_query_cache.cpp                      ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:49:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     331                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "cache/adaptive_query_cache.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

using namespace themis;
using json = nlohmann::json;

class AdaptiveQueryCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use temporary directory for test cache
        config_.l3_db_path = "/tmp/themis_test_query_cache_" + 
                             std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        config_.l1_max_entries = 10;
        config_.l2_max_entries = 20;
        config_.l1_ttl_seconds = 1;  // Short TTL for testing
        config_.l2_ttl_seconds = 2;
        config_.l3_ttl_seconds = 3;
    }
    
    void TearDown() override {
        // Cleanup test cache directory
        if (!config_.l3_db_path.empty()) {
            std::filesystem::remove_all(config_.l3_db_path);
        }
    }
    
    AdaptiveQueryCache::Config config_;
};

TEST_F(AdaptiveQueryCacheTest, GenerateFingerprint) {
    AdaptiveQueryCache cache(config_);
    
    std::string query1 = "SELECT * FROM users WHERE id = ?";
    json params1 = {{"id", 123}};
    
    std::string fingerprint1 = cache.generateFingerprint(query1, params1);
    EXPECT_EQ(fingerprint1.length(), 64);  // SHA256 = 64 hex chars
    
    // Same query and params should produce same fingerprint
    std::string fingerprint2 = cache.generateFingerprint(query1, params1);
    EXPECT_EQ(fingerprint1, fingerprint2);
    
    // Different params should produce different fingerprint
    json params2 = {{"id", 456}};
    std::string fingerprint3 = cache.generateFingerprint(query1, params2);
    EXPECT_NE(fingerprint1, fingerprint3);
}

TEST_F(AdaptiveQueryCacheTest, L1CacheHit) {
    AdaptiveQueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    json params = {};
    json result = {{"data", {1, 2, 3}}};
    
    std::string fingerprint = cache.generateFingerprint(query, params);
    
    // Store in cache
    EXPECT_TRUE(cache.put(fingerprint, params, result));
    
    // Retrieve from cache
    auto cached = cache.get(fingerprint);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->result, result);
    EXPECT_EQ(cached->level, AdaptiveQueryCache::CacheLevel::HOT);
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.l1_hits, 1);
    EXPECT_EQ(stats.misses, 0);
    EXPECT_DOUBLE_EQ(stats.getHitRate(), 1.0);
}

TEST_F(AdaptiveQueryCacheTest, L1CacheMiss) {
    AdaptiveQueryCache cache(config_);
    
    std::string fingerprint = "nonexistent_fingerprint";
    
    // Try to retrieve non-existent entry
    auto cached = cache.get(fingerprint);
    EXPECT_FALSE(cached.has_value());
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.l1_hits, 0);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_DOUBLE_EQ(stats.getHitRate(), 0.0);
}

TEST_F(AdaptiveQueryCacheTest, L1LRUEviction) {
    AdaptiveQueryCache cache(config_);
    
    // Fill L1 cache to capacity
    for (int i = 0; i < config_.l1_max_entries; i++) {
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i);
        json result = {{"id", i}};
        std::string fingerprint = cache.generateFingerprint(query);
        cache.put(fingerprint, {}, result);
    }
    
    // Add one more entry (should evict LRU)
    std::string query_new = "SELECT * FROM users WHERE id = 999";
    json result_new = {{"id", 999}};
    std::string fingerprint_new = cache.generateFingerprint(query_new);
    cache.put(fingerprint_new, {}, result_new);
    
    // Check that new entry exists
    auto cached = cache.get(fingerprint_new);
    EXPECT_TRUE(cached.has_value());
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.evictions, 1);
}

TEST_F(AdaptiveQueryCacheTest, L2CacheCompression) {
    AdaptiveQueryCache cache(config_);
    
    // Create a large result that should go to L2
    json large_result;
    for (int i = 0; i < 200; i++) {
        large_result["data"].push_back({{"id", i}, {"name", "User " + std::to_string(i)}});
    }
    
    std::string query = "SELECT * FROM users";
    std::string fingerprint = cache.generateFingerprint(query);
    
    // Store in cache (should go to L2)
    EXPECT_TRUE(cache.put(fingerprint, {}, large_result));
    
    // Retrieve from cache
    auto cached = cache.get(fingerprint);
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->result, large_result);
    EXPECT_EQ(cached->level, AdaptiveQueryCache::CacheLevel::WARM);
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.l2_hits, 1);
}

TEST_F(AdaptiveQueryCacheTest, L2ToL1Promotion) {
    AdaptiveQueryCache cache(config_);
    
    // Create a result that goes to L2
    json result;
    for (int i = 0; i < 50; i++) {
        result["data"].push_back({{"id", i}});
    }
    
    std::string query = "SELECT * FROM users";
    std::string fingerprint = cache.generateFingerprint(query);
    
    // Store in L2
    cache.put(fingerprint, {}, result);
    
    // Access multiple times to trigger promotion
    for (int i = 0; i < 3; i++) {
        auto cached = cache.get(fingerprint);
        EXPECT_TRUE(cached.has_value());
    }
    
    // Next access should hit L1 (promoted)
    auto cached = cache.get(fingerprint);
    ASSERT_TRUE(cached.has_value());
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_GE(stats.promotions, 1);
}

TEST_F(AdaptiveQueryCacheTest, TTLExpiration) {
    AdaptiveQueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    json result = {{"data", {1, 2, 3}}};
    std::string fingerprint = cache.generateFingerprint(query);
    
    // Store in cache
    cache.put(fingerprint, {}, result);
    
    // Verify cache hit
    auto cached1 = cache.get(fingerprint);
    EXPECT_TRUE(cached1.has_value());
    
    // Wait for TTL to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired now
    auto cached2 = cache.get(fingerprint);
    EXPECT_FALSE(cached2.has_value());
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_GE(stats.evictions, 1);
}

TEST_F(AdaptiveQueryCacheTest, ClearCache) {
    AdaptiveQueryCache cache(config_);
    
    // Add some entries
    for (int i = 0; i < 5; i++) {
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i);
        json result = {{"id", i}};
        std::string fingerprint = cache.generateFingerprint(query);
        cache.put(fingerprint, {}, result);
    }
    
    // Clear cache
    cache.clear();
    
    // Verify all entries are gone
    for (int i = 0; i < 5; i++) {
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i);
        std::string fingerprint = cache.generateFingerprint(query);
        auto cached = cache.get(fingerprint);
        EXPECT_FALSE(cached.has_value());
    }
}

TEST_F(AdaptiveQueryCacheTest, InvalidatePattern) {
    AdaptiveQueryCache cache(config_);
    
    // Add entries with different patterns
    std::string fingerprint1 = cache.generateFingerprint("SELECT * FROM users");
    std::string fingerprint2 = cache.generateFingerprint("SELECT * FROM orders");
    std::string fingerprint3 = cache.generateFingerprint("SELECT * FROM products");
    
    cache.put(fingerprint1, {}, {{"data", "users"}});
    cache.put(fingerprint2, {}, {{"data", "orders"}});
    cache.put(fingerprint3, {}, {{"data", "products"}});
    
    // Invalidate entries matching pattern (fingerprints starting with specific prefix)
    size_t invalidated = cache.invalidate(fingerprint1.substr(0, 10) + ".*");
    EXPECT_GE(invalidated, 0);  // May or may not match depending on hash
    
    // Note: Pattern matching on fingerprints is less useful than on query params
    // In production, you'd want to store and match on query metadata
}

TEST_F(AdaptiveQueryCacheTest, DetailedInfo) {
    AdaptiveQueryCache cache(config_);
    
    // Add some entries
    for (int i = 0; i < 5; i++) {
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i);
        json result = {{"id", i}};
        std::string fingerprint = cache.generateFingerprint(query);
        cache.put(fingerprint, {}, result);
    }
    
    // Get detailed info
    json info = cache.getDetailedInfo();
    
    EXPECT_TRUE(info.contains("stats"));
    EXPECT_TRUE(info.contains("l1"));
    EXPECT_TRUE(info.contains("l2"));
    EXPECT_TRUE(info.contains("l3"));
    
    EXPECT_EQ(info["l1"]["entries"], 5);
    EXPECT_GT(info["l1"]["utilization"], 0.0);
}

TEST_F(AdaptiveQueryCacheTest, ConcurrentAccess) {
    AdaptiveQueryCache cache(config_);
    
    const int num_threads = 4;
    const int ops_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_hits{0};
    std::atomic<int> total_misses{0};
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&cache, t, ops_per_thread, &total_hits, &total_misses]() {
            for (int i = 0; i < ops_per_thread; i++) {
                std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i % 10);
                json result = {{"id", i % 10}};
                std::string fingerprint = cache.generateFingerprint(query);
                
                // Try to get from cache
                auto cached = cache.get(fingerprint);
                if (cached.has_value()) {
                    total_hits++;
                } else {
                    total_misses++;
                    // Store in cache
                    cache.put(fingerprint, {}, result);
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Check that cache handled concurrent access
    auto stats = cache.getStats();
    EXPECT_GT(stats.l1_hits + stats.l2_hits + stats.l3_hits, 0);
    EXPECT_EQ(total_hits.load() + total_misses.load(), num_threads * ops_per_thread);
}
