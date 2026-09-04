/**
 * @file test_rag_evaluation_cache.cpp
 * @brief Unit tests for EvaluationCache (LRU + TTL)
 *
 * Tests cover:
 *  - Default construction and empty state
 *  - Put and get round-trip
 *  - LRU eviction when capacity is exceeded
 *  - TTL expiry removes stale entries
 *  - contains() reflects live entries only
 *  - clear() empties the cache and fires invalidation callback
 *  - invalidate(TTL_EXPIRED) sweeps expired entries
 *  - invalidate(MODEL_UPDATE) purges all entries
 *  - Statistics: hit rate, misses, evictions
 *  - resetStatistics() zeroes counters without clearing entries
 *  - Config accessors (setConfig / getConfig)
 *  - Invalidation callback is invoked on purge
 *  - Thread-safety stress test
 */

#include "rag/evaluation_cache.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::rag::judge;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static EvaluationResult makeResult(double score = 0.9) {
    EvaluationResult r{};
    r.faithfulness_score       = score;
    r.relevance_score          = score;
    r.completeness_score       = score;
    r.coherence_score          = score;
    r.ethical_compliance_score = score;
    r.overall_score            = score;
    r.passed_quality_threshold = score >= 0.7;
    r.confidence               = 0.95;
    r.respects_human_autonomy  = true;
    r.shows_moral_diversity    = false;
    r.has_ethical_citations    = false;
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvaluationCacheTest, DefaultConstruction) {
    EvaluationCache cache;
    EXPECT_EQ(cache.get("query", "answer"), nullptr);
    EXPECT_FALSE(cache.contains("query", "answer"));
}

TEST(EvaluationCacheTest, CustomConfig) {
    CacheConfig cfg;
    cfg.max_entries = 5;
    cfg.ttl         = std::chrono::seconds(60);
    EvaluationCache cache(cfg);

    auto got = cache.getConfig();
    EXPECT_EQ(got.max_entries, 5u);
    EXPECT_EQ(got.ttl, std::chrono::seconds(60));
}

// ─────────────────────────────────────────────────────────────────────────────
// Basic put / get
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvaluationCacheTest, PutAndGet) {
    EvaluationCache cache;
    auto result = makeResult(0.85);
    cache.put("q1", "a1", result);

    const EvaluationResult* got = cache.get("q1", "a1");
    ASSERT_NE(got, nullptr);
    EXPECT_DOUBLE_EQ(got->overall_score, 0.85);
}

TEST(EvaluationCacheTest, MissOnUnknownKey) {
    EvaluationCache cache;
    cache.put("q1", "a1", makeResult(0.9));
    EXPECT_EQ(cache.get("q2", "a1"), nullptr);  // different query
    EXPECT_EQ(cache.get("q1", "a2"), nullptr);  // different answer
}

TEST(EvaluationCacheTest, OverwriteExistingEntry) {
    EvaluationCache cache;
    cache.put("q", "a", makeResult(0.5));
    cache.put("q", "a", makeResult(0.9));  // overwrite

    const EvaluationResult* got = cache.get("q", "a");
    ASSERT_NE(got, nullptr);
    EXPECT_DOUBLE_EQ(got->overall_score, 0.9);
}

// ─────────────────────────────────────────────────────────────────────────────
// contains()
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvaluationCacheTest, ContainsAfterPut) {
    EvaluationCache cache;
    EXPECT_FALSE(cache.contains("q", "a"));
    cache.put("q", "a", makeResult());
    EXPECT_TRUE(cache.contains("q", "a"));
}

// ─────────────────────────────────────────────────────────────────────────────
// LRU eviction
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvaluationCacheTest, LRUEvictsLeastRecentlyUsed) {
    CacheConfig cfg;
    cfg.max_entries = 3;
    cfg.ttl         = std::chrono::seconds(3600);
    EvaluationCache cache(cfg);

    // Fill cache: doc0, doc1, doc2
    for (int i = 0; i < 3; ++i) {
        cache.put("q" + std::to_string(i), "a", makeResult(0.9));
    }

    // Access q0 to make q1 the least-recently used
    EXPECT_NE(cache.get("q0", "a"), nullptr);

    // Insert q3 → should evict q1 (LRU)
    cache.put("q3", "a", makeResult(0.9));

    EXPECT_NE(cache.get("q0", "a"), nullptr);  // recently used → kept
    EXPECT_EQ(cache.get("q1", "a"), nullptr);  // LRU → evicted
    EXPECT_NE(cache.get("q2", "a"), nullptr);
    EXPECT_NE(cache.get("q3", "a"), nullptr);
}

TEST(EvaluationCacheTest, EvictionCountTracked) {
    CacheConfig cfg;
    cfg.max_entries = 2;
    cfg.ttl         = std::chrono::seconds(3600);
    EvaluationCache cache(cfg);

    cache.put("q0", "a", makeResult());
    cache.put("q1", "a", makeResult());
    cache.put("q2", "a", makeResult());  // triggers eviction

    auto stats = cache.getStatistics();
    EXPECT_GE(stats.evictions, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// TTL expiry
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvaluationCacheTest, ExpiredEntryReturnsNull) {
    CacheConfig cfg;
    cfg.max_entries = 100;
    cfg.ttl         = std::chrono::seconds(0);  // immediately expired
    EvaluationCache cache(cfg);

    cache.put("q", "a", makeResult(0.9));
    // With TTL=0 every entry is immediately stale
    EXPECT_EQ(cache.get("q", "a"), nullptr);
    EXPECT_FALSE(cache.contains("q", "a"));
}

// ─────────────────────────────────────────────────────────────────────────────
// clear()
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvaluationCacheTest, ClearEmptiesCache) {
    EvaluationCache cache;
    cache.put("q1", "a", makeResult());
    cache.put("q2", "a", makeResult());
    cache.clear();
    EXPECT_EQ(cache.get("q1", "a"), nullptr);
    EXPECT_EQ(cache.get("q2", "a"), nullptr);
}

TEST(EvaluationCacheTest, ClearFiresInvalidationCallback) {
    EvaluationCache cache;
    cache.put("q", "a", makeResult());

    bool callback_called = false;
    cache.registerInvalidationCallback(
        [&](InvalidationTrigger trigger, size_t count) {
            callback_called = true;
            EXPECT_EQ(trigger, InvalidationTrigger::MANUAL);
            EXPECT_EQ(count, 1u);
        });

    cache.clear();
    EXPECT_TRUE(callback_called);
}

// ─────────────────────────────────────────────────────────────────────────────
// invalidate()
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvaluationCacheTest, InvalidateModelUpdatePurgesAll) {
    EvaluationCache cache;
    cache.put("q1", "a", makeResult());
    cache.put("q2", "a", makeResult());
    cache.invalidate(InvalidationTrigger::MODEL_UPDATE);
    EXPECT_EQ(cache.get("q1", "a"), nullptr);
    EXPECT_EQ(cache.get("q2", "a"), nullptr);
}

TEST(EvaluationCacheTest, InvalidateTTLSweepsOnlyExpired) {
    CacheConfig cfg;
    cfg.max_entries = 100;
    cfg.ttl         = std::chrono::seconds(3600);  // long TTL
    EvaluationCache cache(cfg);

    cache.put("live", "a", makeResult(0.9));
    // The live entry should survive a TTL sweep
    cache.invalidate(InvalidationTrigger::TTL_EXPIRED);
    EXPECT_NE(cache.get("live", "a"), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvaluationCacheTest, StatsHitMissCount) {
    EvaluationCache cache;
    cache.put("q", "a", makeResult(0.9));

    cache.get("q", "a");   // hit
    cache.get("q", "b");   // miss

    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.total_requests, 2u);
    EXPECT_EQ(stats.cache_hits,     1u);
    EXPECT_EQ(stats.cache_misses,   1u);
    EXPECT_DOUBLE_EQ(stats.hit_rate, 0.5);
}

TEST(EvaluationCacheTest, ResetStatisticsZeroesCounters) {
    EvaluationCache cache;
    cache.put("q", "a", makeResult());
    cache.get("q", "a");
    cache.resetStatistics();

    auto stats = cache.getStatistics();
    EXPECT_EQ(stats.total_requests, 0u);
    EXPECT_EQ(stats.cache_hits,     0u);
    EXPECT_EQ(stats.cache_misses,   0u);
    EXPECT_EQ(stats.evictions,      0u);
    // Cache entries survive reset
    EXPECT_NE(cache.get("q", "a"), nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Config accessor
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvaluationCacheTest, SetGetConfig) {
    EvaluationCache cache;
    CacheConfig cfg;
    cfg.max_entries = 42;
    cfg.ttl         = std::chrono::seconds(120);
    cache.setConfig(cfg);

    auto got = cache.getConfig();
    EXPECT_EQ(got.max_entries, 42u);
    EXPECT_EQ(got.ttl, std::chrono::seconds(120));
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread-safety stress test
// ─────────────────────────────────────────────────────────────────────────────

TEST(EvaluationCacheTest, ConcurrentPutGet) {
    CacheConfig cfg;
    cfg.max_entries = 50;
    cfg.ttl         = std::chrono::seconds(3600);
    EvaluationCache cache(cfg);

    constexpr int kThreads = 8;
    constexpr int kOps     = 100;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&cache, t]() {
            for (int i = 0; i < kOps; ++i) {
                std::string key = "q" + std::to_string(t * kOps + i);
                cache.put(key, "a", makeResult(0.9));
                cache.get(key, "a");
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // No crash and statistics are consistent
    auto stats = cache.getStatistics();
    EXPECT_GT(stats.total_requests, 0u);
}
