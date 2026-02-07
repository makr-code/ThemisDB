#include <gtest/gtest.h>
#include <core/concerns/strategic_cache_impl.h>
#include <core/concerns/eviction_strategies.h>
#include <string>
#include <thread>
#include <chrono>

using namespace themis::core::concerns;

// ============================================================================
// Strategic Cache with LRU Tests
// ============================================================================

class StrategicCacheWithLRUTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache = std::make_unique<StrategicCacheImpl>(
            5,  // max size
            std::make_unique<LRUEvictionStrategy>()
        );
    }

    std::unique_ptr<StrategicCacheImpl> cache;
};

TEST_F(StrategicCacheWithLRUTest, BasicPutAndGet) {
    CacheEntry entry1("value1", 1, 1000);
    CacheEntry entry2("value2", 1, 2000);

    EXPECT_TRUE(cache->put("key1", entry1));
    EXPECT_TRUE(cache->put("key2", entry2));

    auto result1 = cache->get("key1");
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1->payload, "value1");

    auto result2 = cache->get("key2");
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2->payload, "value2");
}

TEST_F(StrategicCacheWithLRUTest, CacheMiss) {
    auto result = cache->get("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(StrategicCacheWithLRUTest, LRUEviction) {
    // Fill cache to capacity
    for (int i = 1; i <= 5; i++) {
        CacheEntry entry("value" + std::to_string(i), 1, i * 1000);
        cache->put("key" + std::to_string(i), entry);
    }

    EXPECT_EQ(cache->size(), 5);

    // Access key2 and key3 to make them recently used
    cache->get("key2");
    cache->get("key3");

    // Add new entry, should evict LRU (key1)
    CacheEntry entry6("value6", 1, 6000);
    cache->put("key6", entry6);

    EXPECT_EQ(cache->size(), 5);
    EXPECT_FALSE(cache->get("key1").has_value());  // key1 evicted
    EXPECT_TRUE(cache->get("key2").has_value());   // key2 still there
    EXPECT_TRUE(cache->get("key6").has_value());   // key6 added
}

TEST_F(StrategicCacheWithLRUTest, Invalidate) {
    CacheEntry entry("value1", 1, 1000);
    cache->put("key1", entry);

    EXPECT_TRUE(cache->get("key1").has_value());

    cache->invalidate("key1");

    EXPECT_FALSE(cache->get("key1").has_value());
    EXPECT_EQ(cache->size(), 0);
}

TEST_F(StrategicCacheWithLRUTest, Clear) {
    for (int i = 1; i <= 3; i++) {
        CacheEntry entry("value" + std::to_string(i), 1, i * 1000);
        cache->put("key" + std::to_string(i), entry);
    }

    EXPECT_EQ(cache->size(), 3);

    cache->clear();

    EXPECT_EQ(cache->size(), 0);
}

TEST_F(StrategicCacheWithLRUTest, InvalidatePattern) {
    CacheEntry entry1("value1", 1, 1000);
    CacheEntry entry2("value2", 1, 2000);
    CacheEntry entry3("value3", 1, 3000);

    cache->put("user:1", entry1);
    cache->put("user:2", entry2);
    cache->put("product:1", entry3);

    cache->invalidatePattern("user:.*");

    EXPECT_FALSE(cache->get("user:1").has_value());
    EXPECT_FALSE(cache->get("user:2").has_value());
    EXPECT_TRUE(cache->get("product:1").has_value());
}

TEST_F(StrategicCacheWithLRUTest, HitRateTracking) {
    CacheEntry entry("value1", 1, 1000);
    cache->put("key1", entry);

    // Initial state
    EXPECT_EQ(cache->hitCount(), 0);
    EXPECT_EQ(cache->missCount(), 0);

    // Hit
    cache->get("key1");
    EXPECT_EQ(cache->hitCount(), 1);
    EXPECT_EQ(cache->missCount(), 0);
    EXPECT_DOUBLE_EQ(cache->hitRate(), 1.0);

    // Miss
    cache->get("key2");
    EXPECT_EQ(cache->hitCount(), 1);
    EXPECT_EQ(cache->missCount(), 1);
    EXPECT_DOUBLE_EQ(cache->hitRate(), 0.5);
}

TEST_F(StrategicCacheWithLRUTest, TTLExpiration) {
    // Testing strategy: Create an entry with a past timestamp to simulate expiration
    // without waiting. The cache checks if (current_time - timestamp) > TTL.
    // By creating an entry with timestamp 200ms in the past and TTL of 100ms,
    // the entry should be immediately expired when accessed.
    
    auto past_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch() - std::chrono::milliseconds(200)
    ).count();
    
    CacheEntry entry("value1", 1, past_time);

    // Put with 100ms TTL (but entry timestamp is already 200ms old)
    cache->put("key1", entry, 100);

    // Should be expired immediately since entry timestamp + TTL < now
    EXPECT_FALSE(cache->get("key1").has_value()) 
        << "Entry should be expired (timestamp 200ms old, TTL 100ms)";
    
    // Now test with fresh entry to verify non-expired case
    auto now_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    CacheEntry entry2("value2", 1, now_time);
    cache->put("key2", entry2, 50);
    
    // Should be available immediately
    EXPECT_TRUE(cache->get("key2").has_value()) 
        << "Fresh entry should be immediately available";
}

TEST_F(StrategicCacheWithLRUTest, GetEvictionStrategy) {
    auto* strategy = cache->getEvictionStrategy();
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->getName(), "LRU");
}

TEST_F(StrategicCacheWithLRUTest, GetMetrics) {
    const auto* metrics = cache->getMetrics();
    ASSERT_NE(metrics, nullptr);
    
    EXPECT_EQ(metrics->hit_count, 0);
    EXPECT_EQ(metrics->miss_count, 0);
    EXPECT_EQ(metrics->max_size, 5);
    
    CacheEntry entry("value1", 1, 1000);
    cache->put("key1", entry);
    cache->get("key1");
    
    metrics = cache->getMetrics();
    EXPECT_EQ(metrics->hit_count, 1);
    EXPECT_EQ(metrics->insertion_count, 1);
}

// ============================================================================
// Strategic Cache with LFU Tests
// ============================================================================

class StrategicCacheWithLFUTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache = std::make_unique<StrategicCacheImpl>(
            5,
            std::make_unique<LFUEvictionStrategy>()
        );
    }

    std::unique_ptr<StrategicCacheImpl> cache;
};

TEST_F(StrategicCacheWithLFUTest, LFUEviction) {
    // Fill cache
    for (int i = 1; i <= 5; i++) {
        CacheEntry entry("value" + std::to_string(i), 1, i * 1000);
        cache->put("key" + std::to_string(i), entry);
    }

    // Access some entries multiple times
    cache->get("key1");
    cache->get("key1");
    cache->get("key2");
    cache->get("key3");
    cache->get("key3");
    cache->get("key3");

    // Add new entry - should evict least frequently used (key4 or key5)
    CacheEntry entry6("value6", 1, 6000);
    cache->put("key6", entry6);

    EXPECT_EQ(cache->size(), 5);
    
    // Most frequently accessed should still be there
    EXPECT_TRUE(cache->get("key1").has_value());
    EXPECT_TRUE(cache->get("key3").has_value());
}

TEST_F(StrategicCacheWithLFUTest, GetEvictionStrategy) {
    auto* strategy = cache->getEvictionStrategy();
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->getName(), "LFU");
}

// ============================================================================
// Strategic Cache with TTL Tests
// ============================================================================

class StrategicCacheWithTTLTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache = std::make_unique<StrategicCacheImpl>(
            5,
            std::make_unique<TTLEvictionStrategy>(100)  // 100ms default TTL
        );
    }

    std::unique_ptr<StrategicCacheImpl> cache;
};

TEST_F(StrategicCacheWithTTLTest, GetEvictionStrategy) {
    auto* strategy = cache->getEvictionStrategy();
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->getName(), "TTL");
}

// ============================================================================
// Strategic Cache with TwoTier Tests
// ============================================================================

class StrategicCacheWithTwoTierTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto l1 = std::make_unique<LRUEvictionStrategy>();
        auto l2 = std::make_unique<LFUEvictionStrategy>();
        
        cache = std::make_unique<StrategicCacheImpl>(
            10,
            std::make_unique<TwoTierEvictionStrategy>(
                std::move(l1),
                std::move(l2),
                3  // L1 capacity = 3
            )
        );
    }

    std::unique_ptr<StrategicCacheImpl> cache;
};

TEST_F(StrategicCacheWithTwoTierTest, BasicOperation) {
    for (int i = 1; i <= 5; i++) {
        CacheEntry entry("value" + std::to_string(i), 1, i * 1000);
        cache->put("key" + std::to_string(i), entry);
    }

    EXPECT_EQ(cache->size(), 5);
    EXPECT_TRUE(cache->get("key1").has_value());
}

TEST_F(StrategicCacheWithTwoTierTest, GetEvictionStrategy) {
    auto* strategy = cache->getEvictionStrategy();
    ASSERT_NE(strategy, nullptr);
    EXPECT_EQ(strategy->getName(), "TwoTier");
}

// ============================================================================
// Strategy Swapping Tests
// ============================================================================

class StrategicCacheStrategySwapTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache = std::make_unique<StrategicCacheImpl>(
            5,
            std::make_unique<LRUEvictionStrategy>()
        );
    }

    std::unique_ptr<StrategicCacheImpl> cache;
};

TEST_F(StrategicCacheStrategySwapTest, SwapStrategy) {
    // Start with LRU
    EXPECT_EQ(cache->getEvictionStrategy()->getName(), "LRU");

    // Add some data
    for (int i = 1; i <= 3; i++) {
        CacheEntry entry("value" + std::to_string(i), 1, i * 1000);
        cache->put("key" + std::to_string(i), entry);
    }

    // Swap to LFU
    cache->setEvictionStrategy(std::make_unique<LFUEvictionStrategy>());
    EXPECT_EQ(cache->getEvictionStrategy()->getName(), "LFU");

    // Data should still be accessible
    EXPECT_TRUE(cache->get("key1").has_value());
    EXPECT_TRUE(cache->get("key2").has_value());
    EXPECT_TRUE(cache->get("key3").has_value());
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

class StrategicCacheThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache = std::make_unique<StrategicCacheImpl>(
            100,
            std::make_unique<LRUEvictionStrategy>()
        );
    }

    std::unique_ptr<StrategicCacheImpl> cache;
};

TEST_F(StrategicCacheThreadSafetyTest, ConcurrentAccess) {
    const int num_threads = 4;
    const int ops_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, t, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; i++) {
                std::string key = "key" + std::to_string(t * ops_per_thread + i);
                CacheEntry entry("value" + std::to_string(i), 1, i * 1000);
                
                cache->put(key, entry);
                cache->get(key);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Should not crash and maintain size constraint
    EXPECT_LE(cache->size(), 100);
}
