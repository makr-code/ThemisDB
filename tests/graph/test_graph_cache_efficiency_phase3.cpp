#include <gtest/gtest.h>
#include "graph/graph_cache_manager.h"
#include "graph/graph_plan_cache.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace themis::graph;

// ============================================================================
// Fixture: GraphMultiTierCache<string, int>
// ============================================================================
class MultiTierCacheTest : public ::testing::Test {
protected:
    GraphMultiTierCache<std::string, int>::Config small_cfg{2, 4, 8};
};

// ============================================================================
// P3-02 Group 1: Basic operations
// ============================================================================

TEST_F(MultiTierCacheTest, InitiallyEmpty) {
    GraphMultiTierCache<std::string, int> cache;
    EXPECT_EQ(0u, cache.size());
}

TEST_F(MultiTierCacheTest, PutAndGetHit) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("k1", 42);
    auto v = cache.get("k1");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(42, *v);
}

TEST_F(MultiTierCacheTest, MissReturnsNullopt) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    EXPECT_FALSE(cache.get("nonexistent").has_value());
}

TEST_F(MultiTierCacheTest, SizeIncrementsOnInsert) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("a", 1);
    cache.put("b", 2);
    EXPECT_EQ(2u, cache.size());
}

TEST_F(MultiTierCacheTest, DuplicatePutUpdatesValue) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("k", 1);
    cache.put("k", 2);
    auto v = cache.get("k");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(2, *v);
    EXPECT_EQ(1u, cache.size());
}

TEST_F(MultiTierCacheTest, RemoveKnownKey) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("r", 99);
    EXPECT_TRUE(cache.remove("r"));
    EXPECT_FALSE(cache.get("r").has_value());
}

TEST_F(MultiTierCacheTest, RemoveUnknownKeyReturnsFalse) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    EXPECT_FALSE(cache.remove("does_not_exist"));
}

TEST_F(MultiTierCacheTest, ClearEmptiesAllTiers) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    for (int i = 0; i < 5; ++i)
        cache.put("k" + std::to_string(i), i);
    cache.clear();
    EXPECT_EQ(0u, cache.size());
    EXPECT_EQ(0u, cache.hotSize());
    EXPECT_EQ(0u, cache.warmSize());
    EXPECT_EQ(0u, cache.coldSize());
}

// ============================================================================
// P3-02 Group 2: Tier placement and promotion
// ============================================================================

TEST_F(MultiTierCacheTest, NewEntryEntersColdTier) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("c1", 10);
    EXPECT_EQ(1u, cache.coldSize());
    EXPECT_EQ(0u, cache.warmSize());
    EXPECT_EQ(0u, cache.hotSize());
}

TEST_F(MultiTierCacheTest, ColdHitPromotesToWarm) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("c1", 10);
    cache.get("c1"); // promotes cold → warm
    EXPECT_EQ(0u, cache.coldSize());
    EXPECT_EQ(1u, cache.warmSize());
}

TEST_F(MultiTierCacheTest, WarmHitPromotesToHot) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("c1", 10);
    cache.get("c1"); // cold → warm
    cache.get("c1"); // warm → hot
    EXPECT_EQ(0u, cache.warmSize());
    EXPECT_EQ(1u, cache.hotSize());
}

TEST_F(MultiTierCacheTest, HotHitStaysHot) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("c1", 10);
    cache.get("c1"); // → warm
    cache.get("c1"); // → hot
    cache.get("c1"); // stays hot
    EXPECT_EQ(1u, cache.hotSize());
}

// ============================================================================
// P3-02 Group 3: Eviction behaviour
// ============================================================================

TEST_F(MultiTierCacheTest, ColdEvictsWhenFull) {
    // cold_capacity = 8, insert 9 entries
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    for (int i = 0; i < 9; ++i)
        cache.put("c" + std::to_string(i), i);
    // Cold tier should not exceed capacity; overflow demoted to... eviction
    EXPECT_LE(cache.coldSize(), 8u);
}

TEST_F(MultiTierCacheTest, HotEvictsToWarmWhenFull) {
    // hot_capacity = 2, warm_capacity = 4
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    // Insert 4 entries, promote them all to Hot
    for (int i = 0; i < 4; ++i) {
        auto key = "k" + std::to_string(i);
        cache.put(key, i);
        cache.get(key); // cold → warm
        cache.get(key); // warm → hot — may evict from hot to warm
    }
    // Hot must not exceed its capacity of 2
    EXPECT_LE(cache.hotSize(), 2u);
}

// ============================================================================
// P3-02 Group 4: Hit ratio
// ============================================================================

TEST_F(MultiTierCacheTest, HitRatioZeroWhenNoLookups) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    EXPECT_EQ(0.0, cache.metrics().hitRatio());
}

TEST_F(MultiTierCacheTest, HitRatioAfterPureHits) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("x", 1);
    cache.get("x");
    cache.get("x");
    const double ratio = cache.metrics().hitRatio();
    EXPECT_GT(ratio, 0.0);
}

TEST_F(MultiTierCacheTest, HitRatioAbove85PercentOnRepeatedAccess) {
    // Populate 10 items, access them 10 times each → 100 hits, 10 initial misses
    GraphMultiTierCache<std::string, int> cache({8, 32, 128});
    for (int i = 0; i < 10; ++i)
        cache.put("k" + std::to_string(i), i);
    for (int round = 0; round < 10; ++round)
        for (int i = 0; i < 10; ++i)
            cache.get("k" + std::to_string(i));
    EXPECT_GE(cache.metrics().hitRatio(), 0.85);
}

TEST_F(MultiTierCacheTest, MetricsTotalLookupsCorrect) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("a", 1);
    cache.get("a"); // hit
    cache.get("b"); // miss
    EXPECT_EQ(2u, cache.metrics().totalLookups());
}

TEST_F(MultiTierCacheTest, MetricsColdHitsCountedSeparately) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("k", 1);
    cache.get("k"); // cold hit → warm
    EXPECT_EQ(1u, cache.metrics().cold_hits);
    EXPECT_EQ(0u, cache.metrics().warm_hits);
    EXPECT_EQ(0u, cache.metrics().hot_hits);
}

TEST_F(MultiTierCacheTest, MetricsWarmHitsCountedSeparately) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("k", 1);
    cache.get("k"); // cold → warm
    cache.get("k"); // warm hit → hot
    EXPECT_EQ(1u, cache.metrics().warm_hits);
}

TEST_F(MultiTierCacheTest, MetricsHotHitsCountedSeparately) {
    GraphMultiTierCache<std::string, int> cache(small_cfg);
    cache.put("k", 1);
    cache.get("k"); // cold → warm
    cache.get("k"); // warm → hot
    cache.get("k"); // hot hit
    EXPECT_EQ(1u, cache.metrics().hot_hits);
}

// ============================================================================
// P3-02 Group 5: Reconfigure
// ============================================================================

TEST_F(MultiTierCacheTest, ReconfigureShrinksHotTier) {
    GraphMultiTierCache<std::string, int> cache({10, 20, 40});
    for (int i = 0; i < 5; ++i) {
        auto k = "k" + std::to_string(i);
        cache.put(k, i);
        cache.get(k);
        cache.get(k); // promote to hot
    }
    // Shrink hot to 2
    cache.reconfigure({2, 20, 40});
    EXPECT_LE(cache.hotSize(), 2u);
}

// ============================================================================
// P3-02 Group 6: GraphTraversalResultCache
// ============================================================================

TEST(TraversalResultCacheTest, BFSKeyIsReproducible) {
    auto k1 = GraphTraversalResultCache::bfsKey("A", 3, "knows");
    auto k2 = GraphTraversalResultCache::bfsKey("A", 3, "knows");
    EXPECT_EQ(k1, k2);
}

TEST(TraversalResultCacheTest, BFSKeyVariesByParams) {
    EXPECT_NE(GraphTraversalResultCache::bfsKey("A", 3),
              GraphTraversalResultCache::bfsKey("A", 4));
    EXPECT_NE(GraphTraversalResultCache::bfsKey("A", 3),
              GraphTraversalResultCache::bfsKey("B", 3));
}

TEST(TraversalResultCacheTest, ShortestPathKeyIsReproducible) {
    EXPECT_EQ(GraphTraversalResultCache::shortestPathKey("X", "Y"),
              GraphTraversalResultCache::shortestPathKey("X", "Y"));
}

TEST(TraversalResultCacheTest, PutAndGetResult) {
    GraphTraversalResultCache tc;
    const auto key = GraphTraversalResultCache::bfsKey("A", 2);
    tc.put(key, {"A", "B", "C"});
    auto res = tc.get(key);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(3u, res->size());
}

TEST(TraversalResultCacheTest, MissReturnsNullopt) {
    GraphTraversalResultCache tc;
    EXPECT_FALSE(tc.get("ghost_key").has_value());
}

TEST(TraversalResultCacheTest, InvalidateRemovesEntry) {
    GraphTraversalResultCache tc;
    const auto key = GraphTraversalResultCache::bfsKey("A", 2);
    tc.put(key, {"A", "B"});
    EXPECT_TRUE(tc.invalidate(key));
    EXPECT_FALSE(tc.get(key).has_value());
}

TEST(TraversalResultCacheTest, HitRatioImprovesWith85PercentThreshold) {
    GraphTraversalResultCache tc(8, 32, 128);
    // Fill cache
    for (int i = 0; i < 10; ++i)
        tc.put("q" + std::to_string(i), {"v1", "v2"});
    // Repeated access
    for (int round = 0; round < 10; ++round)
        for (int i = 0; i < 10; ++i)
            tc.get("q" + std::to_string(i));
    EXPECT_GE(tc.metrics().hitRatio(), 0.85);
}

// ============================================================================
// P3-02 Group 7: GraphCostHistogram
// ============================================================================

TEST(GraphCostHistogramTest, TotalStartsAtZero) {
    GraphCostHistogram h;
    EXPECT_EQ(0u, h.total());
}

TEST(GraphCostHistogramTest, RecordIncreasesTotal) {
    GraphCostHistogram h;
    h.record(10);
    h.record(50);
    EXPECT_EQ(2u, h.total());
}

TEST(GraphCostHistogramTest, P99BelowBoundAfterFastQueries) {
    GraphCostHistogram h;
    for (int i = 0; i < 100; ++i) h.record(5); // all ≤ 5ms
    EXPECT_LE(h.percentileMs(0.99), 5.0 + 1e-6);
}

TEST(GraphCostHistogramTest, P99AboveP50) {
    GraphCostHistogram h;
    for (int i = 0; i < 90; ++i) h.record(1);
    for (int i = 0; i < 10; ++i) h.record(500);
    EXPECT_LE(h.percentileMs(0.50), h.percentileMs(0.99) + 1e-6);
}
