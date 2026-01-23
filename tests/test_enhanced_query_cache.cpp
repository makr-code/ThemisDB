#include <gtest/gtest.h>
#include "cache/enhanced_query_cache.h"
#include <string>
#include <thread>

using namespace themis::cache;

class EnhancedQueryCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        EnhancedQueryCache<std::string, std::string>::Config config;
        config.max_entries = 100;
        config.default_ttl = std::chrono::seconds(5);
        config.enable_metrics = true;
        cache = std::make_unique<EnhancedQueryCache<std::string, std::string>>(config);
    }
    
    std::unique_ptr<EnhancedQueryCache<std::string, std::string>> cache;
};

TEST_F(EnhancedQueryCacheTest, PutAndGet) {
    cache->put("key1", "value1");
    
    auto result = cache->get("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "value1");
}

TEST_F(EnhancedQueryCacheTest, CacheMiss) {
    auto result = cache->get("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(EnhancedQueryCacheTest, Contains) {
    cache->put("key1", "value1");
    
    EXPECT_TRUE(cache->contains("key1"));
    EXPECT_FALSE(cache->contains("key2"));
}

TEST_F(EnhancedQueryCacheTest, Remove) {
    cache->put("key1", "value1");
    EXPECT_TRUE(cache->contains("key1"));
    
    cache->remove("key1");
    EXPECT_FALSE(cache->contains("key1"));
}

TEST_F(EnhancedQueryCacheTest, Clear) {
    cache->put("key1", "value1");
    cache->put("key2", "value2");
    
    cache->clear();
    
    EXPECT_FALSE(cache->contains("key1"));
    EXPECT_FALSE(cache->contains("key2"));
    
    auto stats = cache->getStats();
    EXPECT_EQ(stats.entries, 0);
}

TEST_F(EnhancedQueryCacheTest, TTLExpiration) {
    cache->put("key1", "value1", std::chrono::seconds(1));
    
    EXPECT_TRUE(cache->contains("key1"));
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    auto result = cache->get("key1");
    EXPECT_FALSE(result.has_value());
}

TEST_F(EnhancedQueryCacheTest, HitMissStatistics) {
    cache->put("key1", "value1");
    
    cache->get("key1"); // Hit
    cache->get("key2"); // Miss
    cache->get("key1"); // Hit
    
    auto stats = cache->getStats();
    EXPECT_EQ(stats.hits, 2);
    EXPECT_EQ(stats.misses, 1);
    EXPECT_DOUBLE_EQ(stats.hit_rate, 2.0 / 3.0);
}

TEST_F(EnhancedQueryCacheTest, PruneExpired) {
    cache->put("key1", "value1", std::chrono::seconds(1));
    cache->put("key2", "value2", std::chrono::seconds(10));
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    size_t pruned = cache->pruneExpired();
    
    EXPECT_EQ(pruned, 1);
    EXPECT_FALSE(cache->contains("key1"));
    EXPECT_TRUE(cache->contains("key2"));
}

TEST_F(EnhancedQueryCacheTest, WarmCache) {
    EnhancedQueryCache<std::string, std::string>::Config config;
    config.enable_warming = true;
    
    EnhancedQueryCache<std::string, std::string> warm_cache(config);
    
    std::vector<std::pair<std::string, std::string>> entries = {
        {"key1", "value1"},
        {"key2", "value2"},
        {"key3", "value3"}
    };
    
    warm_cache.warm(entries);
    
    EXPECT_TRUE(warm_cache.contains("key1"));
    EXPECT_TRUE(warm_cache.contains("key2"));
    EXPECT_TRUE(warm_cache.contains("key3"));
    
    // Access warmed entry
    warm_cache.get("key1");
    
    auto stats = warm_cache.getStats();
    EXPECT_EQ(stats.warm_hits, 1);
}

TEST_F(EnhancedQueryCacheTest, GetHotKeys) {
    cache->put("key1", "value1");
    cache->put("key2", "value2");
    cache->put("key3", "value3");
    
    // Access key1 multiple times
    for (int i = 0; i < 10; ++i) {
        cache->get("key1");
    }
    
    // Access key2 a few times
    for (int i = 0; i < 5; ++i) {
        cache->get("key2");
    }
    
    auto hot_keys = cache->getHotKeys(2);
    
    EXPECT_EQ(hot_keys.size(), 2);
    EXPECT_EQ(hot_keys[0], "key1"); // Most accessed
    EXPECT_EQ(hot_keys[1], "key2"); // Second most accessed
}

TEST_F(EnhancedQueryCacheTest, ConcurrentAccess) {
    std::vector<std::thread> threads;
    
    // Writers
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 20; ++j) {
                std::string key = "key_" + std::to_string(i * 20 + j);
                cache->put(key, "value");
            }
        });
    }
    
    // Readers
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 20; ++j) {
                std::string key = "key_" + std::to_string(i * 20 + j);
                cache->get(key);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto stats = cache->getStats();
    EXPECT_EQ(stats.entries, 100);
}

TEST_F(EnhancedQueryCacheTest, MemoryManagement) {
    // Fill cache beyond capacity
    for (int i = 0; i < 150; ++i) {
        cache->put("key_" + std::to_string(i), "value");
    }
    
    auto stats = cache->getStats();
    
    // Should have evicted some entries
    EXPECT_LE(stats.entries, 100);
    EXPECT_GT(stats.evictions, 0);
}

TEST_F(EnhancedQueryCacheTest, ResetStats) {
    cache->put("key1", "value1");
    cache->get("key1");
    cache->get("key2");
    
    auto stats = cache->getStats();
    EXPECT_GT(stats.hits + stats.misses, 0);
    
    cache->resetStats();
    
    stats = cache->getStats();
    EXPECT_EQ(stats.hits, 0);
    EXPECT_EQ(stats.misses, 0);
}

TEST_F(EnhancedQueryCacheTest, UpdateExistingEntry) {
    cache->put("key1", "value1");
    cache->put("key1", "value2"); // Update
    
    auto result = cache->get("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "value2");
}

TEST_F(EnhancedQueryCacheTest, AccessCount) {
    cache->put("key1", "value1");
    
    for (int i = 0; i < 15; ++i) {
        cache->get("key1");
    }
    
    auto stats = cache->getStats();
    EXPECT_GT(stats.hot_entries, 0); // key1 should be hot (>10 accesses)
}
