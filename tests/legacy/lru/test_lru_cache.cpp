#include <gtest/gtest.h>
#include "config/lru_cache.h"
#include <thread>
#include <chrono>

namespace themis {
namespace config {
namespace test {

// Test fixture for LRU cache
class LRUCacheTest : public ::testing::Test {
protected:
    LRUCacheWithTTL<std::string, std::string> cache_{10, 60}; // 10 entries, 60s TTL
};

// ═══════════════════════════════════════════════════════════
// Basic Operations
// ═══════════════════════════════════════════════════════════

TEST_F(LRUCacheTest, PutAndGet) {
    cache_.put("key1", "value1");
    
    auto result = cache_.get("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "value1");
}

TEST_F(LRUCacheTest, GetNonexistent) {
    auto result = cache_.get("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LRUCacheTest, UpdateExisting) {
    cache_.put("key1", "value1");
    cache_.put("key1", "value2");
    
    auto result = cache_.get("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "value2");
}

TEST_F(LRUCacheTest, Size) {
    EXPECT_EQ(cache_.size(), 0);
    
    cache_.put("key1", "value1");
    EXPECT_EQ(cache_.size(), 1);
    
    cache_.put("key2", "value2");
    EXPECT_EQ(cache_.size(), 2);
}

TEST_F(LRUCacheTest, Clear) {
    cache_.put("key1", "value1");
    cache_.put("key2", "value2");
    EXPECT_EQ(cache_.size(), 2);
    
    cache_.clear();
    EXPECT_EQ(cache_.size(), 0);
    EXPECT_FALSE(cache_.get("key1").has_value());
}

TEST_F(LRUCacheTest, Empty) {
    EXPECT_TRUE(cache_.empty());
    
    cache_.put("key1", "value1");
    EXPECT_FALSE(cache_.empty());
    
    cache_.clear();
    EXPECT_TRUE(cache_.empty());
}

// ═══════════════════════════════════════════════════════════
// LRU Eviction Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LRUCacheTest, LRUEviction) {
    // Fill cache to capacity (10 entries)
    for (int i = 0; i < 10; i++) {
        cache_.put("key" + std::to_string(i), "value" + std::to_string(i));
    }
    EXPECT_EQ(cache_.size(), 10);
    
    // Add one more - should evict the LRU (key0)
    cache_.put("key10", "value10");
    EXPECT_EQ(cache_.size(), 10);
    
    // key0 should be evicted
    EXPECT_FALSE(cache_.get("key0").has_value());
    // key10 should be present
    EXPECT_TRUE(cache_.get("key10").has_value());
}

TEST_F(LRUCacheTest, AccessUpdatesLRU) {
    // Fill cache to capacity
    for (int i = 0; i < 10; i++) {
        cache_.put("key" + std::to_string(i), "value" + std::to_string(i));
    }
    
    // Access key0 to make it most recently used
    cache_.get("key0");
    
    // Add new entry - should evict key1 (now LRU), not key0
    cache_.put("key10", "value10");
    
    EXPECT_TRUE(cache_.get("key0").has_value());
    EXPECT_FALSE(cache_.get("key1").has_value());
}

// ═══════════════════════════════════════════════════════════
// TTL Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LRUCacheTest, TTLExpiration) {
    // Create cache with 1 second TTL for testing
    LRUCacheWithTTL<std::string, std::string> short_ttl_cache(10, 1);
    
    short_ttl_cache.put("key1", "value1");
    
    // Should be available immediately
    EXPECT_TRUE(short_ttl_cache.get("key1").has_value());
    
    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired
    EXPECT_FALSE(short_ttl_cache.get("key1").has_value());
}

TEST_F(LRUCacheTest, CustomTTL) {
    // Create cache with long default TTL
    LRUCacheWithTTL<std::string, std::string> cache(10, 60);
    
    // Add entry with custom short TTL
    cache.put("key1", "value1", 1);
    
    // Should be available immediately
    EXPECT_TRUE(cache.get("key1").has_value());
    
    // Wait for custom TTL to expire
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Should be expired
    EXPECT_FALSE(cache.get("key1").has_value());
}

// ═══════════════════════════════════════════════════════════
// Invalidation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LRUCacheTest, Invalidate) {
    cache_.put("key1", "value1");
    cache_.put("key2", "value2");
    
    EXPECT_TRUE(cache_.invalidate("key1"));
    EXPECT_FALSE(cache_.get("key1").has_value());
    EXPECT_TRUE(cache_.get("key2").has_value());
}

TEST_F(LRUCacheTest, InvalidateNonexistent) {
    EXPECT_FALSE(cache_.invalidate("nonexistent"));
}

// ═══════════════════════════════════════════════════════════
// Statistics Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LRUCacheTest, StatsHitRate) {
    cache_.put("key1", "value1");
    
    // 2 hits
    cache_.get("key1");
    cache_.get("key1");
    
    // 1 miss
    cache_.get("nonexistent");
    
    auto stats = cache_.stats();
    EXPECT_EQ(stats.hits, 2);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_DOUBLE_EQ(stats.hit_rate, 2.0 / 3.0);
}

TEST_F(LRUCacheTest, StatsEvictions) {
    // Fill cache
    for (int i = 0; i < 10; i++) {
        cache_.put("key" + std::to_string(i), "value" + std::to_string(i));
    }
    
    // Cause evictions
    cache_.put("key10", "value10");
    cache_.put("key11", "value11");
    
    auto stats = cache_.stats();
    EXPECT_EQ(stats.evictions, 2);
}

TEST_F(LRUCacheTest, StatsSize) {
    cache_.put("key1", "value1");
    cache_.put("key2", "value2");
    
    auto stats = cache_.stats();
    EXPECT_EQ(stats.size, 2);
    EXPECT_EQ(stats.capacity, 10);
}

// ═══════════════════════════════════════════════════════════
// Thread Safety Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LRUCacheTest, ConcurrentAccess) {
    const int num_threads = 10;
    const int ops_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; i++) {
                std::string key = "key" + std::to_string(t) + "_" + std::to_string(i);
                cache_.put(key, "value");
                cache_.get(key);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Cache should be in valid state
    auto stats = cache_.stats();
    EXPECT_LE(stats.size, 10); // Should not exceed capacity
}

} // namespace test
} // namespace config
} // namespace themis
