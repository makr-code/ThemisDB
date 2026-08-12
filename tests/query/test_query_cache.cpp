#include <gtest/gtest.h>
#include "query/query_cache.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>

using namespace themis;
using namespace themis::query;
using json = nlohmann::json;

class QueryCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.max_entries = 10;
        config_.max_memory_bytes = 1024 * 1024;  // 1MB
        config_.default_ttl = std::chrono::seconds(2);  // Short TTL for testing
        config_.enable_ttl = true;
    }
    
    QueryCache::Config config_;
};

// ============================================================================
// Fingerprint Generation Tests
// ============================================================================

TEST_F(QueryCacheTest, GenerateFingerprint_Basic) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM users WHERE id = ?";
    json params = {{"id", 123}};
    
    std::string fp1 = cache.generateFingerprint(query, params);
    EXPECT_EQ(fp1.length(), 64);  // SHA256 = 64 hex chars
    
    // Same query and params should produce same fingerprint
    std::string fp2 = cache.generateFingerprint(query, params);
    EXPECT_EQ(fp1, fp2);
}

TEST_F(QueryCacheTest, GenerateFingerprint_DifferentParams) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM users WHERE id = ?";
    json params1 = {{"id", 123}};
    json params2 = {{"id", 456}};
    
    std::string fp1 = cache.generateFingerprint(query, params1);
    std::string fp2 = cache.generateFingerprint(query, params2);
    
    EXPECT_NE(fp1, fp2);
}

TEST_F(QueryCacheTest, GenerateFingerprint_NoParams) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    
    std::string fp1 = cache.generateFingerprint(query);
    std::string fp2 = cache.generateFingerprint(query, json::object());
    
    EXPECT_EQ(fp1, fp2);
}

// ============================================================================
// Basic Cache Operations
// ============================================================================

TEST_F(QueryCacheTest, Put_Get_Basic) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    json params = json::object();
    json result = {{"data", {1, 2, 3}}};
    
    // Store in cache
    auto put_result = cache.put(query, params, result);
    ASSERT_TRUE(put_result.has_value());
    
    // Retrieve from cache
    auto get_result = cache.get(query, params);
    ASSERT_TRUE(get_result.has_value());
    ASSERT_TRUE(get_result->found);
    EXPECT_EQ(get_result->result, result);
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.misses, 0);
    EXPECT_EQ(stats.current_entries, 1);
    EXPECT_DOUBLE_EQ(stats.hitRate(), 1.0);
}

TEST_F(QueryCacheTest, Get_CacheMiss) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM nonexistent";
    
    auto result = cache.get(query);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->found);
    
    // Check stats
    auto stats = cache.getStats();
    EXPECT_EQ(stats.hits, 0);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_DOUBLE_EQ(stats.hitRate(), 0.0);
}

TEST_F(QueryCacheTest, Put_EmptyQuery) {
    QueryCache cache(config_);
    
    std::string query = "";
    json result = {{"data", 1}};
    
    auto put_result = cache.put(query, json::object(), result);
    ASSERT_FALSE(put_result.has_value());
    EXPECT_EQ(put_result.error().code(), errors::ErrorCode::ERR_QUERY_INVALID);
}

TEST_F(QueryCacheTest, Put_Update) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    json result1 = {{"data", {1, 2, 3}}};
    json result2 = {{"data", {4, 5, 6}}};
    
    // First put
    auto put1 = cache.put(query, json::object(), result1);
    ASSERT_TRUE(put1.has_value());
    
    // Second put (update)
    auto put2 = cache.put(query, json::object(), result2);
    ASSERT_TRUE(put2.has_value());
    
    // Should have updated result
    auto get_result = cache.get(query);
    ASSERT_TRUE(get_result.has_value());
    ASSERT_TRUE(get_result->found);
    EXPECT_EQ(get_result->result, result2);
    
    // Should still have only 1 entry
    auto stats = cache.getStats();
    EXPECT_EQ(stats.current_entries, 1);
}

// ============================================================================
// LRU Eviction Tests
// ============================================================================

TEST_F(QueryCacheTest, LRU_Eviction) {
    config_.eviction_policy = QueryCache::EvictionPolicy::LRU;
    QueryCache cache(config_);
    
    // Fill cache to capacity
    for (int i = 0; i < config_.max_entries; i++) {
        std::string query = "SELECT * FROM users WHERE id = " + std::to_string(i);
        json result = {{"id", i}};
        auto put_result = cache.put(query, json::object(), result);
        ASSERT_TRUE(put_result.has_value());
    }
    
    auto stats = cache.getStats();
    EXPECT_EQ(stats.current_entries, config_.max_entries);
    
    // Add one more entry - should evict LRU (first entry)
    std::string new_query = "SELECT * FROM users WHERE id = 999";
    json new_result = {{"id", 999}};
    auto put_result = cache.put(new_query, json::object(), new_result);
    ASSERT_TRUE(put_result.has_value());
    
    stats = cache.getStats();
    EXPECT_EQ(stats.current_entries, config_.max_entries);
    EXPECT_EQ(stats.evictions, 1);
    
    // First entry should be evicted
    std::string first_query = "SELECT * FROM users WHERE id = 0";
    auto get_result = cache.get(first_query);
    ASSERT_TRUE(get_result.has_value());
    EXPECT_FALSE(get_result->found);
    
    // New entry should be in cache
    get_result = cache.get(new_query);
    ASSERT_TRUE(get_result.has_value());
    EXPECT_TRUE(get_result->found);
}

TEST_F(QueryCacheTest, LRU_AccessUpdatesOrder) {
    config_.eviction_policy = QueryCache::EvictionPolicy::LRU;
    config_.max_entries = 3;
    QueryCache cache(config_);
    
    // Add 3 entries
    static_cast<void>(cache.put("Q1", json::object(), json({{"data", 1}})));
    static_cast<void>(cache.put("Q2", json::object(), json({{"data", 2}})));
    static_cast<void>(cache.put("Q3", json::object(), json({{"data", 3}})));
    
    // Access Q1 (moves it to front)
    static_cast<void>(cache.get("Q1"));
    
    // Add Q4 - should evict Q2 (now LRU)
    static_cast<void>(cache.put("Q4", json::object(), json({{"data", 4}})));
    
    // Q2 should be evicted
    auto result = cache.get("Q2");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->found);
    
    // Q1 should still be in cache
    result = cache.get("Q1");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->found);
}

// ============================================================================
// LFU Eviction Tests
// ============================================================================

TEST_F(QueryCacheTest, LFU_Eviction) {
    config_.eviction_policy = QueryCache::EvictionPolicy::LFU;
    config_.max_entries = 3;
    QueryCache cache(config_);
    
    // Add 3 entries
    static_cast<void>(cache.put("Q1", json::object(), json({{"data", 1}})));
    static_cast<void>(cache.put("Q2", json::object(), json({{"data", 2}})));
    static_cast<void>(cache.put("Q3", json::object(), json({{"data", 3}})));
    
    // Access Q2 and Q3 multiple times
    static_cast<void>(cache.get("Q2"));
    static_cast<void>(cache.get("Q2"));
    static_cast<void>(cache.get("Q3"));
    static_cast<void>(cache.get("Q3"));
    // Q1 has access_count=1, Q2=3, Q3=3
    
    // Add Q4 - should evict Q1 (lowest frequency)
    static_cast<void>(cache.put("Q4", json::object(), json({{"data", 4}})));
    
    // Q1 should be evicted
    auto result = cache.get("Q1");
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->found);
    
    // Q2 and Q3 should still be in cache
    result = cache.get("Q2");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->found);
    
    result = cache.get("Q3");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->found);
}

// ============================================================================
// TTL Expiration Tests
// ============================================================================

TEST_F(QueryCacheTest, TTL_Expiration) {
    config_.default_ttl = std::chrono::seconds(1);
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    json result = {{"data", 1}};
    
    // Store in cache
    static_cast<void>(cache.put(query, json::object(), result));
    
    // Immediate get should succeed
    auto get_result = cache.get(query);
    ASSERT_TRUE(get_result.has_value());
    EXPECT_TRUE(get_result->found);
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired
    get_result = cache.get(query);
    ASSERT_TRUE(get_result.has_value());
    EXPECT_FALSE(get_result->found);
    
    auto stats = cache.getStats();
    EXPECT_EQ(stats.expirations, 1);
    EXPECT_EQ(stats.current_entries, 0);
}

TEST_F(QueryCacheTest, TTL_CustomTTL) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    json result = {{"data", 1}};
    
    // Store with custom TTL
    auto custom_ttl = std::chrono::seconds(1);
    static_cast<void>(cache.put(query, json::object(), result, {}, custom_ttl));
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired
    auto get_result = cache.get(query);
    ASSERT_TRUE(get_result.has_value());
    EXPECT_FALSE(get_result->found);
}

TEST_F(QueryCacheTest, ClearExpired) {
    config_.default_ttl = std::chrono::seconds(1);
    QueryCache cache(config_);
    
    // Add multiple entries
    for (int i = 0; i < 5; i++) {
        std::string query = "SELECT " + std::to_string(i);
        static_cast<void>(cache.put(query, json::object(), json({{"data", i}})));
    }
    
    EXPECT_EQ(cache.getStats().current_entries, 5);
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Clear expired
    auto result = cache.clearExpired();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 5);
    
    EXPECT_EQ(cache.getStats().current_entries, 0);
}

// ============================================================================
// Dependency Tracking and Invalidation Tests
// ============================================================================

TEST_F(QueryCacheTest, Dependency_Tracking) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    json result = {{"data", 1}};
    std::vector<std::string> deps = {"users"};
    
    static_cast<void>(cache.put(query, json::object(), result, deps));
    
    // Should be in cache
    auto get_result = cache.get(query);
    ASSERT_TRUE(get_result.has_value());
    EXPECT_TRUE(get_result->found);
}

TEST_F(QueryCacheTest, InvalidateByDependency) {
    QueryCache cache(config_);
    
    // Add queries with dependencies
    static_cast<void>(cache.put("SELECT * FROM users", json::object(), json({{"data", 1}}), {"users"}));
    static_cast<void>(cache.put("SELECT * FROM orders", json::object(), json({{"data", 2}}), {"orders"}));
    static_cast<void>(cache.put("SELECT * FROM users JOIN orders", json::object(), json({{"data", 3}}), {"users", "orders"}));
    
    EXPECT_EQ(cache.getStats().current_entries, 3);
    
    // Invalidate by dependency
    auto result = cache.invalidateByDependency("users");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 2);  // 2 queries depend on "users"
    
    EXPECT_EQ(cache.getStats().current_entries, 1);
    EXPECT_EQ(cache.getStats().invalidations, 2);
    
    // Orders query should still be in cache
    auto get_result = cache.get("SELECT * FROM orders");
    ASSERT_TRUE(get_result.has_value());
    EXPECT_TRUE(get_result->found);
}

TEST_F(QueryCacheTest, InvalidateSpecific) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM users";
    static_cast<void>(cache.put(query, json::object(), json({{"data", 1}})));
    
    // Invalidate specific query
    auto result = cache.invalidate(query);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(*result);
    
    EXPECT_EQ(cache.getStats().current_entries, 0);
    EXPECT_EQ(cache.getStats().invalidations, 1);
    
    // Should not be in cache
    auto get_result = cache.get(query);
    ASSERT_TRUE(get_result.has_value());
    EXPECT_FALSE(get_result->found);
}

// ============================================================================
// Cache Management Tests
// ============================================================================

TEST_F(QueryCacheTest, Clear) {
    QueryCache cache(config_);
    
    // Add multiple entries
    for (int i = 0; i < 5; i++) {
        std::string query = "SELECT " + std::to_string(i);
        static_cast<void>(cache.put(query, json::object(), json({{"data", i}})));
    }
    
    EXPECT_EQ(cache.getStats().current_entries, 5);
    
    // Clear cache
    auto result = cache.clear();
    ASSERT_TRUE(result.has_value());
    
    EXPECT_EQ(cache.getStats().current_entries, 0);
}

TEST_F(QueryCacheTest, GetDetailedInfo) {
    QueryCache cache(config_);
    
    static_cast<void>(cache.put("SELECT 1", json::object(), json({{"data", 1}})));
    static_cast<void>(cache.get("SELECT 1"));
    static_cast<void>(cache.get("SELECT 2"));  // Miss
    
    auto info = cache.getDetailedInfo();
    
    EXPECT_TRUE(info.contains("statistics"));
    EXPECT_TRUE(info.contains("memory"));
    EXPECT_TRUE(info.contains("configuration"));
    
    EXPECT_EQ(info["statistics"]["hits"], 1);
    EXPECT_EQ(info["statistics"]["misses"], 1);
    EXPECT_EQ(info["memory"]["current_entries"], 1);
    EXPECT_EQ(info["configuration"]["eviction_policy"], "LRU");
}

TEST_F(QueryCacheTest, ResetStats) {
    QueryCache cache(config_);
    
    static_cast<void>(cache.put("SELECT 1", json::object(), json({{"data", 1}})));
    static_cast<void>(cache.get("SELECT 1"));
    
    auto stats = cache.getStats();
    EXPECT_EQ(stats.hits, 1);
    EXPECT_EQ(stats.current_entries, 1);
    
    cache.resetStats();
    
    stats = cache.getStats();
    EXPECT_EQ(stats.hits, 0);
    EXPECT_EQ(stats.misses, 0);
    EXPECT_EQ(stats.current_entries, 1);  // Don't reset entry count
}

TEST_F(QueryCacheTest, SetConfig) {
    QueryCache cache(config_);
    
    // Fill cache
    for (int i = 0; i < 10; i++) {
        std::string query = "SELECT " + std::to_string(i);
        static_cast<void>(cache.put(query, json::object(), json({{"data", i}})));
    }
    
    EXPECT_EQ(cache.getStats().current_entries, 10);
    
    // Update config with lower limit
    QueryCache::Config new_config = config_;
    new_config.max_entries = 5;
    
    auto result = cache.setConfig(new_config);
    ASSERT_TRUE(result.has_value());
    
    // Should have evicted entries
    auto stats = cache.getStats();
    EXPECT_LE(stats.current_entries, 5);
}

// ============================================================================
// Memory Management Tests
// ============================================================================

TEST_F(QueryCacheTest, MemoryLimit) {
    config_.max_memory_bytes = 1000;  // Very small limit
    config_.max_entry_size = 500;
    QueryCache cache(config_);
    
    // Add entries until memory limit is reached
    int count = 0;
    for (int i = 0; i < 10; i++) {
        std::string query = "SELECT " + std::to_string(i);
        json result = {{"data", std::string(100, 'x')}};  // Large result
        auto put_result = cache.put(query, json::object(), result);
        if (put_result.has_value()) {
            count++;
        }
    }
    
    // Should have evicted some entries to stay under limit
    auto stats = cache.getStats();
    EXPECT_LT(stats.current_entries, 10);
    EXPECT_LE(stats.current_memory_bytes, config_.max_memory_bytes);
}

TEST_F(QueryCacheTest, EntryTooLarge) {
    config_.max_entry_size = 100;
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM large_table";
    json large_result = {{"data", std::string(1000, 'x')}};
    
    auto result = cache.put(query, json::object(), large_result);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_CACHE_ENTRY_TOO_LARGE);
}

// ============================================================================
// Concurrency Tests
// ============================================================================

TEST_F(QueryCacheTest, ConcurrentAccess) {
    QueryCache cache(config_);
    
    // Pre-populate cache
    for (int i = 0; i < 5; i++) {
        std::string query = "SELECT " + std::to_string(i);
        static_cast<void>(cache.put(query, json::object(), json({{"data", i}})));
    }
    
    // Concurrent reads and writes
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; t++) {
        threads.emplace_back([&cache, t]() {
            for (int i = 0; i < 100; i++) {
                std::string query = "SELECT " + std::to_string(i % 10);
                
                // Mix reads and writes
                if (i % 2 == 0) {
                    static_cast<void>(cache.get(query));
                } else {
                    static_cast<void>(cache.put(query, json::object(), json({{"data", i}})));
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should not crash and have valid stats
    auto stats = cache.getStats();
    EXPECT_GT(stats.total_requests, 0);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(QueryCacheTest, EmptyResult) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM empty_table";
    json empty_result = json::array();
    
    auto put_result = cache.put(query, json::object(), empty_result);
    ASSERT_TRUE(put_result.has_value());
    
    auto get_result = cache.get(query);
    ASSERT_TRUE(get_result.has_value());
    EXPECT_TRUE(get_result->found);
    EXPECT_EQ(get_result->result, empty_result);
}

TEST_F(QueryCacheTest, ComplexParams) {
    QueryCache cache(config_);
    
    std::string query = "SELECT * FROM users WHERE id IN (?)";
    json params = {
        {"ids", {1, 2, 3, 4, 5}},
        {"filter", {{"age", {{"$gt", 30}}}, {"name", {{"$regex", "^John"}}}}},
        {"options", {{"limit", 10}, {"sort", {{"created_at", -1}}}}}
    };
    json result = {{"data", {1, 2, 3}}};
    
    static_cast<void>(cache.put(query, params, result));
    
    auto get_result = cache.get(query, params);
    ASSERT_TRUE(get_result.has_value());
    EXPECT_TRUE(get_result->found);
    EXPECT_EQ(get_result->result, result);
}
