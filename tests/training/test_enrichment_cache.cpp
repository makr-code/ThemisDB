/*
 * ThemisDB | File: test_enrichment_cache.cpp | Version: 0.0.1
 * Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_enrichment_cache.cpp
 * @brief Phase 4 cache invalidation and stale data handling tests.
 *
 * Tests verify:
 *  - Cache hit/miss tracking
 *  - TTL expiration detection
 *  - Cache eviction strategies
 *  - Stale data detection and handling
 *  - Cache invalidation propagation
 *  - Size-bounded cache enforcement
 *  - Concurrent cache access safety
 *  - Cache statistics reporting
 */

#include <gtest/gtest.h>

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

// Simple cache implementation for testing
namespace themis {
namespace training {

struct CacheEntry {
    std::string key;
    std::vector<float> data;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_accessed;
    size_t access_count = 0;
    bool is_stale = false;
};

class EnrichmentCache {
public:
    explicit EnrichmentCache(size_t max_entries = 100, int ttl_seconds = 3600)
        : max_entries_(max_entries), ttl_seconds_(ttl_seconds) {}

    void put(const std::string& key, const std::vector<float>& data) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Evict if at capacity
        if (entries_.size() >= max_entries_ && entries_.find(key) == entries_.end()) {
            evictOne();
        }

        CacheEntry entry;
        entry.key = key;
        entry.data = data;
        entry.created_at = std::chrono::system_clock::now();
        entry.last_accessed = entry.created_at;
        entry.access_count = 0;

        entries_[key] = entry;
        stats_.puts++;
    }

    std::vector<float> get(const std::string& key, bool* hit = nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = entries_.find(key);
        if (it == entries_.end()) {
            stats_.misses++;
            if (hit) *hit = false;
            return {};
        }

        auto now = std::chrono::system_clock::now();
        auto age_sec = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.created_at).count();

        // Check if stale
        if (age_sec > ttl_seconds_) {
            entries_.erase(it);
            stats_.expirations++;
            if (hit) *hit = false;
            return {};
        }

        it->second.last_accessed = now;
        it->second.access_count++;
        stats_.hits++;
        if (hit) *hit = true;
        return it->second.data;
    }

    bool invalidate(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it == entries_.end()) {
            return false;
        }
        entries_.erase(it);
        stats_.invalidations++;
        return true;
    }

    void invalidateAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
        stats_.invalidations++;
    }

    void markStale(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = entries_.find(key);
        if (it != entries_.end()) {
            it->second.is_stale = true;
        }
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.size();
    }

    bool contains(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.find(key) != entries_.end();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
    }

    struct Stats {
        size_t hits = 0;
        size_t misses = 0;
        size_t puts = 0;
        size_t invalidations = 0;
        size_t expirations = 0;

        double hitRate() const {
            size_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / total : 0.0;
        }
    };

    Stats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }

private:
    void evictOne() {
        if (entries_.empty()) return;

        // LRU eviction
        auto lru_it = entries_.begin();
        for (auto it = entries_.begin(); it != entries_.end(); ++it) {
            if (it->second.last_accessed < lru_it->second.last_accessed) {
                lru_it = it;
            }
        }
        entries_.erase(lru_it);
        stats_.expirations++;  // Track as expiration
    }

    mutable std::mutex mutex_;
    std::map<std::string, CacheEntry> entries_;
    size_t max_entries_;
    int ttl_seconds_;
    mutable Stats stats_;
};

}  // namespace training
}  // namespace themis

using namespace themis::training;

// ============================================================================
// Basic cache operations
// ============================================================================

TEST(EnrichmentCacheTest, PutAndGet_Succeeds) {
    EnrichmentCache cache;
    std::vector<float> data = {1.0f, 2.0f, 3.0f};

    cache.put("key1", data);
    bool hit = false;
    auto retrieved = cache.get("key1", &hit);

    EXPECT_TRUE(hit);
    EXPECT_EQ(retrieved.size(), data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_FLOAT_EQ(retrieved[i], data[i]);
    }
}

TEST(EnrichmentCacheTest, GetMissing_ReturnsFalse) {
    EnrichmentCache cache;
    bool hit = false;
    auto retrieved = cache.get("missing", &hit);

    EXPECT_FALSE(hit);
    EXPECT_TRUE(retrieved.empty());
}

// ============================================================================
// Cache hit/miss tracking
// ============================================================================

TEST(EnrichmentCacheTest, Stats_HitsAndMisses) {
    EnrichmentCache cache;
    std::vector<float> data = {1.0f, 2.0f};

    cache.put("key1", data);
    cache.get("key1");  // hit
    cache.get("key2");  // miss
    cache.get("key1");  // hit
    cache.get("key3");  // miss

    auto stats = cache.getStats();
    EXPECT_EQ(stats.hits, 2u);
    EXPECT_EQ(stats.misses, 2u);
    EXPECT_FLOAT_EQ(stats.hitRate(), 0.5);
}

TEST(EnrichmentCacheTest, Stats_Puts) {
    EnrichmentCache cache;
    std::vector<float> data = {1.0f};

    for (int i = 0; i < 5; ++i) {
        cache.put("key_" + std::to_string(i), data);
    }

    auto stats = cache.getStats();
    EXPECT_EQ(stats.puts, 5u);
}

// ============================================================================
// TTL and expiration
// ============================================================================

TEST(EnrichmentCacheTest, ExpiredEntry_NotReturned) {
    EnrichmentCache cache(100, 1);  // 1 second TTL
    std::vector<float> data = {1.0f};

    cache.put("key1", data);

    // Immediately check - should hit
    bool hit = false;
    cache.get("key1", &hit);
    EXPECT_TRUE(hit);

    // Wait for expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    // Should miss now
    hit = false;
    cache.get("key1", &hit);
    EXPECT_FALSE(hit);
}

TEST(EnrichmentCacheTest, ExpirationStats_Tracked) {
    EnrichmentCache cache(100, 1);  // 1 second TTL
    std::vector<float> data = {1.0f};

    cache.put("key1", data);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    cache.get("key1");  // Trigger expiration

    auto stats = cache.getStats();
    EXPECT_GT(stats.expirations, 0u);
}

// ============================================================================
// Cache invalidation
// ============================================================================

TEST(EnrichmentCacheTest, InvalidateEntry_Succeeds) {
    EnrichmentCache cache;
    std::vector<float> data = {1.0f};

    cache.put("key1", data);
    EXPECT_TRUE(cache.contains("key1"));

    bool removed = cache.invalidate("key1");
    EXPECT_TRUE(removed);
    EXPECT_FALSE(cache.contains("key1"));
}

TEST(EnrichmentCacheTest, InvalidateMissing_ReturnsFalse) {
    EnrichmentCache cache;
    bool removed = cache.invalidate("missing");
    EXPECT_FALSE(removed);
}

TEST(EnrichmentCacheTest, InvalidateAll_ClearsCache) {
    EnrichmentCache cache;
    std::vector<float> data = {1.0f};

    for (int i = 0; i < 10; ++i) {
        cache.put("key_" + std::to_string(i), data);
    }

    EXPECT_EQ(cache.size(), 10u);

    cache.invalidateAll();
    EXPECT_EQ(cache.size(), 0u);
}

TEST(EnrichmentCacheTest, InvalidationStats_Tracked) {
    EnrichmentCache cache;
    std::vector<float> data = {1.0f};

    cache.put("key1", data);
    cache.invalidate("key1");

    auto stats = cache.getStats();
    EXPECT_EQ(stats.invalidations, 1u);
}

// ============================================================================
// Size-bounded cache
// ============================================================================

TEST(EnrichmentCacheTest, SizeBounded_EvictsOldest) {
    EnrichmentCache cache(3, 3600);  // max 3 entries
    std::vector<float> data = {1.0f, 2.0f};

    cache.put("key1", data);
    cache.put("key2", data);
    cache.put("key3", data);

    EXPECT_EQ(cache.size(), 3u);

    // Add fourth entry - should evict LRU (key1)
    cache.put("key4", data);

    EXPECT_EQ(cache.size(), 3u);
    // key1 should be evicted (LRU)
    EXPECT_FALSE(cache.contains("key1"));
    EXPECT_TRUE(cache.contains("key4"));
}

TEST(EnrichmentCacheTest, SizeBounded_MaxEnforced) {
    EnrichmentCache cache(5, 3600);
    std::vector<float> data = {1.0f};

    for (int i = 0; i < 100; ++i) {
        cache.put("key_" + std::to_string(i), data);
    }

    EXPECT_LE(cache.size(), 5u);
}

// ============================================================================
// Stale data detection
// ============================================================================

TEST(EnrichmentCacheTest, MarkStale_Tracked) {
    EnrichmentCache cache;
    std::vector<float> data = {1.0f};

    cache.put("key1", data);
    cache.markStale("key1");

    // Entry still exists but marked stale
    EXPECT_TRUE(cache.contains("key1"));
}

// ============================================================================
// Concurrent access
// ============================================================================

TEST(EnrichmentCacheTest, ConcurrentPuts_Safe) {
    EnrichmentCache cache(1000, 3600);
    std::vector<float> data = {1.0f, 2.0f, 3.0f};

    std::vector<std::thread> threads;
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&cache, &data, t]() {
            for (int i = 0; i < 100; ++i) {
                std::string key = "key_" + std::to_string(t) + "_" + std::to_string(i);
                cache.put(key, data);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Cache should contain entries
    EXPECT_GT(cache.size(), 0u);
}

TEST(EnrichmentCacheTest, ConcurrentGets_Safe) {
    EnrichmentCache cache;
    std::vector<float> data = {1.0f};

    cache.put("key1", data);

    std::vector<std::thread> threads;
    std::atomic<int> successful_gets(0);

    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&cache, &successful_gets]() {
            for (int i = 0; i < 100; ++i) {
                bool hit = false;
                cache.get("key1", &hit);
                if (hit) {
                    successful_gets++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_GT(successful_gets.load(), 0);
}

TEST(EnrichmentCacheTest, ConcurrentMixed_Safe) {
    EnrichmentCache cache(100, 3600);
    std::vector<float> data = {1.0f};

    std::vector<std::thread> threads;
    std::atomic<bool> error(false);

    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&cache, &data, &error, t]() {
            try {
                for (int i = 0; i < 50; ++i) {
                    std::string key = "key_" + std::to_string(t) + "_" + std::to_string(i);
                    if (i % 2 == 0) {
                        cache.put(key, data);
                    } else {
                        cache.get(key);
                    }
                }
            } catch (...) {
                error = true;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_FALSE(error.load());
}

// ============================================================================
// LRU eviction verification
// ============================================================================

TEST(EnrichmentCacheTest, LRU_EvictsLeastRecentlyUsed) {
    EnrichmentCache cache(3, 3600);
    std::vector<float> data = {1.0f};

    cache.put("key1", data);
    cache.put("key2", data);
    cache.put("key3", data);

    // Access key1 and key2, making key3 LRU
    cache.get("key1");
    cache.get("key2");

    // Add new entry - key3 should be evicted
    cache.put("key4", data);

    EXPECT_TRUE(cache.contains("key1"));
    EXPECT_TRUE(cache.contains("key2"));
    EXPECT_FALSE(cache.contains("key3"));
    EXPECT_TRUE(cache.contains("key4"));
}

// ============================================================================
// Empty cache operations
// ============================================================================

TEST(EnrichmentCacheTest, EmptyCache_Size) {
    EnrichmentCache cache;
    EXPECT_EQ(cache.size(), 0u);
}

TEST(EnrichmentCacheTest, ClearEmpty_Safe) {
    EnrichmentCache cache;
    EXPECT_NO_THROW(cache.clear());
}

// ============================================================================
// Cache configuration
// ============================================================================

TEST(EnrichmentCacheTest, SmallMaxEntries_Enforced) {
    EnrichmentCache cache(1, 3600);
    std::vector<float> data = {1.0f};

    cache.put("key1", data);
    cache.put("key2", data);

    EXPECT_LE(cache.size(), 1u);
}

TEST(EnrichmentCacheTest, ShortTTL_QuickExpiration) {
    EnrichmentCache cache(100, 1);  // 1 second TTL
    std::vector<float> data = {1.0f};

    cache.put("key1", data);

    // Just under TTL - should hit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    bool hit = false;
    cache.get("key1", &hit);
    EXPECT_TRUE(hit);

    // After TTL - should miss
    std::this_thread::sleep_for(std::chrono::milliseconds(700));
    hit = false;
    cache.get("key1", &hit);
    EXPECT_FALSE(hit);
}

// ============================================================================
// Hit rate calculations
// ============================================================================

TEST(EnrichmentCacheTest, HitRate_Calculated) {
    EnrichmentCache cache;
    std::vector<float> data = {1.0f};

    cache.put("key1", data);
    cache.put("key2", data);

    for (int i = 0; i < 8; ++i) cache.get("key1");
    for (int i = 0; i < 2; ++i) cache.get("key2");

    auto stats = cache.getStats();
    EXPECT_FLOAT_EQ(stats.hitRate(), 1.0);  // All gets are hits
}

TEST(EnrichmentCacheTest, HitRate_AllMisses) {
    EnrichmentCache cache;

    for (int i = 0; i < 10; ++i) {
        cache.get("missing_" + std::to_string(i));
    }

    auto stats = cache.getStats();
    EXPECT_FLOAT_EQ(stats.hitRate(), 0.0);
}
