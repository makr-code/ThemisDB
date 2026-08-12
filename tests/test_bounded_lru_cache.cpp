// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "cache/bounded_lru_cache.h"
#include <thread>
#include <chrono>

using namespace themis::cache;

class BoundedLRUCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        BoundedLRUCache::Config config;
        config.max_entries = 10;
        config.ttl = std::chrono::seconds(10);
        config.enable_statistics = true;
        cache = std::make_unique<BoundedLRUCache>(config);
    }
    
    std::unique_ptr<BoundedLRUCache> cache;
};

TEST_F(BoundedLRUCacheTest, BasicPutAndGet) {
    cache->put("key1", {{"value", 1}});
    cache->put("key2", {{"value", 2}});
    
    auto val1 = cache->get("key1");
    ASSERT_TRUE(val1.has_value());
    EXPECT_EQ((*val1)["value"], 1);
    
    auto val2 = cache->get("key2");
    ASSERT_TRUE(val2.has_value());
    EXPECT_EQ((*val2)["value"], 2);
    
    auto val3 = cache->get("key3");
    EXPECT_FALSE(val3.has_value());
}

TEST_F(BoundedLRUCacheTest, UpdateExistingKey) {
    cache->put("key1", {{"value", 1}});
    cache->put("key1", {{"value", 2}});
    
    auto val = cache->get("key1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ((*val)["value"], 2);
    
    auto stats = cache->getStatistics();
    EXPECT_EQ(stats.current_size, 1);
}

TEST_F(BoundedLRUCacheTest, EvictsLRUOnCapacity) {
    // Fill cache to capacity (10 entries)
    for (int i = 0; i < 10; i++) {
        cache->put("key" + std::to_string(i), {{"value", i}});
    }
    
    auto stats = cache->getStatistics();
    EXPECT_EQ(stats.current_size, 10);
    
    // Add more entries - should evict LRU
    for (int i = 10; i < 20; i++) {
        cache->put("key" + std::to_string(i), {{"value", i}});
    }
    
    // Should still have 10 entries
    stats = cache->getStatistics();
    EXPECT_EQ(stats.current_size, 10);
    
    // First entries (0-9) should be evicted
    EXPECT_FALSE(cache->get("key0").has_value());
    EXPECT_FALSE(cache->get("key5").has_value());
    EXPECT_FALSE(cache->get("key9").has_value());
    
    // Last entries (10-19) should be present
    EXPECT_TRUE(cache->get("key10").has_value());
    EXPECT_TRUE(cache->get("key15").has_value());
    EXPECT_TRUE(cache->get("key19").has_value());
}

TEST_F(BoundedLRUCacheTest, LRUOrderMaintained) {
    // Add 10 entries
    for (int i = 0; i < 10; i++) {
        cache->put("key" + std::to_string(i), {{"value", i}});
    }
    
    // Access key0 to make it most recently used
    cache->get("key0");
    
    // Add new entry - should evict key1 (now LRU), not key0
    cache->put("key10", {{"value", 10}});
    
    // key0 should still be there
    EXPECT_TRUE(cache->get("key0").has_value());
    
    // key1 should be evicted
    EXPECT_FALSE(cache->get("key1").has_value());
}

TEST_F(BoundedLRUCacheTest, ExpiresTTLEntries) {
    BoundedLRUCache::Config config;
    config.max_entries = 100;
    config.ttl = std::chrono::seconds(1);
    config.enable_statistics = true;
    auto ttl_cache = std::make_unique<BoundedLRUCache>(config);
    
    ttl_cache->put("key1", {{"value", 1}});
    
    // Should be present initially
    EXPECT_TRUE(ttl_cache->get("key1").has_value());
    
    // Wait for TTL expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    
    // Should be expired
    EXPECT_FALSE(ttl_cache->get("key1").has_value());
}

TEST_F(BoundedLRUCacheTest, TracksCacheHitRatio) {
    cache->put("key1", {{"value", 1}});
    cache->put("key2", {{"value", 2}});
    
    // Hit
    cache->get("key1");
    
    // Miss
    cache->get("key3");
    
    // Hit
    cache->get("key2");
    
    // Miss
    cache->get("key4");
    
    auto stats = cache->getStatistics();
    EXPECT_EQ(stats.hits, 2);
    EXPECT_EQ(stats.misses, 2);
    EXPECT_DOUBLE_EQ(stats.hit_ratio(), 0.5);
}

TEST_F(BoundedLRUCacheTest, ClearRemovesAllEntries) {
    for (int i = 0; i < 5; i++) {
        cache->put("key" + std::to_string(i), {{"value", i}});
    }
    
    auto stats = cache->getStatistics();
    EXPECT_EQ(stats.current_size, 5);
    
    cache->clear();
    
    stats = cache->getStatistics();
    EXPECT_EQ(stats.current_size, 0);
    EXPECT_EQ(stats.hits, 0);
    EXPECT_EQ(stats.misses, 0);
    
    EXPECT_FALSE(cache->get("key0").has_value());
}

TEST_F(BoundedLRUCacheTest, EvictLRUIfNeeded) {
    // Fill to capacity
    for (int i = 0; i < 10; i++) {
        cache->put("key" + std::to_string(i), {{"value", i}});
    }

    // At capacity -> one LRU entry should be evicted
    EXPECT_TRUE(cache->evictLRUIfNeeded());

    // Refill to capacity, then eviction should happen again
    cache->put("key_new", {{"value", 10}});
    EXPECT_TRUE(cache->evictLRUIfNeeded());
}

TEST_F(BoundedLRUCacheTest, ComplexJsonValues) {
    nlohmann::json complex_value = {
        {"name", "test"},
        {"version", 123},
        {"metadata", {
            {"created_at", "2024-01-01"},
            {"tags", {"tag1", "tag2", "tag3"}}
        }},
        {"values", {1, 2, 3, 4, 5}}
    };
    
    cache->put("complex", complex_value);
    
    auto retrieved = cache->get("complex");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ((*retrieved)["name"], "test");
    EXPECT_EQ((*retrieved)["version"], 123);
    EXPECT_EQ((*retrieved)["metadata"]["tags"][1], "tag2");
    EXPECT_EQ((*retrieved)["values"][2], 3);
}

TEST_F(BoundedLRUCacheTest, StatisticsWithoutEnabling) {
    BoundedLRUCache::Config config;
    config.max_entries = 10;
    config.enable_statistics = false;
    auto no_stats_cache = std::make_unique<BoundedLRUCache>(config);
    
    no_stats_cache->put("key1", {{"value", 1}});
    no_stats_cache->get("key1");
    no_stats_cache->get("key2");
    
    auto stats = no_stats_cache->getStatistics();
    // Statistics should still be accessible but may be 0
    EXPECT_EQ(stats.current_size, 1);
    // Hits and misses won't be tracked when disabled
    EXPECT_EQ(stats.hits, 0);
    EXPECT_EQ(stats.misses, 0);
}

TEST_F(BoundedLRUCacheTest, ThreadSafety) {
    const int num_threads = 10;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; i++) {
                std::string key = "key" + std::to_string(t) + "_" + std::to_string(i);
                cache->put(key, {{"value", i}, {"thread", t}});
                cache->get(key);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto stats = cache->getStatistics();
    EXPECT_LE(stats.current_size, 10);  // Should not exceed capacity
}

TEST_F(BoundedLRUCacheTest, EmptyHitRatio) {
    auto stats = cache->getStatistics();
    EXPECT_DOUBLE_EQ(stats.hit_ratio(), 0.0);
}

TEST_F(BoundedLRUCacheTest, RemoveEntry) {
    cache->put("key1", {{"value", 1}});
    cache->put("key2", {{"value", 2}});
    cache->put("key3", {{"value", 3}});
    
    auto stats = cache->getStatistics();
    EXPECT_EQ(stats.current_size, 3);
    
    // Remove key2
    EXPECT_TRUE(cache->remove("key2"));
    
    stats = cache->getStatistics();
    EXPECT_EQ(stats.current_size, 2);
    
    // Verify removed
    EXPECT_FALSE(cache->get("key2").has_value());
    
    // Verify others still present
    EXPECT_TRUE(cache->get("key1").has_value());
    EXPECT_TRUE(cache->get("key3").has_value());
    
    // Removing non-existent key should return false
    EXPECT_FALSE(cache->remove("nonexistent"));
}
