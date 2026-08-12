#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <atomic>
#include "utils/concurrent_cache.h"

using namespace themis;

class ConcurrentCacheTest : public ::testing::Test {
protected:
    ConcurrentCache<std::string, int> cache;
};

TEST_F(ConcurrentCacheTest, BasicInsertAndGet) {
    cache.insert("key1", 42);
    auto val = cache.get("key1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 42);
}

TEST_F(ConcurrentCacheTest, GetNonExistent) {
    auto val = cache.get("nonexistent");
    EXPECT_FALSE(val.has_value());
}

TEST_F(ConcurrentCacheTest, Erase) {
    cache.insert("key1", 42);
    EXPECT_TRUE(cache.erase("key1"));
    EXPECT_FALSE(cache.get("key1").has_value());
    EXPECT_FALSE(cache.erase("key1"));  // Already erased
}

TEST_F(ConcurrentCacheTest, Contains) {
    cache.insert("key1", 42);
    EXPECT_TRUE(cache.contains("key1"));
    EXPECT_FALSE(cache.contains("key2"));
}

TEST_F(ConcurrentCacheTest, Size) {
    EXPECT_EQ(cache.size(), 0);
    cache.insert("key1", 1);
    EXPECT_EQ(cache.size(), 1);
    cache.insert("key2", 2);
    EXPECT_EQ(cache.size(), 2);
}

TEST_F(ConcurrentCacheTest, Clear) {
    cache.insert("key1", 1);
    cache.insert("key2", 2);
    cache.clear();
    EXPECT_EQ(cache.size(), 0);
}

TEST_F(ConcurrentCacheTest, UpdateValue) {
    cache.insert("key1", 10);
    bool updated = cache.try_update("key1", 20);
    EXPECT_TRUE(updated);
    EXPECT_EQ(*cache.get("key1"), 20);
}

TEST_F(ConcurrentCacheTest, ConcurrentInserts) {
    const int NUM_THREADS = 8;
    const int ITEMS_PER_THREAD = 1000;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < ITEMS_PER_THREAD; ++i) {
                std::string key = "thread_" + std::to_string(t) + "_key_" + std::to_string(i);
                cache.insert(key, i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(cache.size(), NUM_THREADS * ITEMS_PER_THREAD);
}

TEST_F(ConcurrentCacheTest, ConcurrentReadWrite) {
    // Pre-populate
    for (int i = 0; i < 100; ++i) {
        cache.insert("key_" + std::to_string(i), i);
    }
    
    std::atomic<int> read_count{0};
    std::atomic<int> write_count{0};
    const int NUM_THREADS = 8;
    const int ITERATIONS = 500;
    std::vector<std::thread> threads;
    
    // Mix of readers and writers
    for (int t = 0; t < NUM_THREADS; ++t) {
        if (t % 2 == 0) {
            // Writer thread
            threads.emplace_back([this, &write_count, t]() {
                for (int i = 0; i < ITERATIONS; ++i) {
                    cache.insert("key_" + std::to_string(i % 100), i);
                    write_count++;
                }
            });
        } else {
            // Reader thread
            threads.emplace_back([this, &read_count, t]() {
                for (int i = 0; i < ITERATIONS; ++i) {
                    auto val = cache.get("key_" + std::to_string(i % 100));
                    if (val.has_value()) {
                        read_count++;
                    }
                }
            });
        }
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(read_count, 0);
    EXPECT_GT(write_count, 0);
}

TEST_F(ConcurrentCacheTest, ForEach) {
    cache.insert("a", 1);
    cache.insert("b", 2);
    cache.insert("c", 3);
    
    std::vector<int> values;
    cache.for_each([&]([[maybe_unused]] const std::string& k, int v) {
        values.push_back(v);
    });
    
    EXPECT_EQ(values.size(), 3);
    std::sort(values.begin(), values.end());
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 3);
}

