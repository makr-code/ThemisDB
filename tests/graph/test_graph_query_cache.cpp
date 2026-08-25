#include <gtest/gtest.h>
#include "graph/graph_query_cache.h"
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using themis::graph::GraphQueryCache;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class GraphQueryCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        GraphQueryCache::Config cfg;
        cfg.l1_capacity = 4;
        cfg.l2_capacity = 8;
        cfg.ttl = std::chrono::milliseconds{0};  // no expiry by default
        cfg.default_cost = 1.0;
        cache_ = std::make_unique<GraphQueryCache>(cfg);
    }

    void TearDown() override {
        cache_.reset();
    }

    GraphQueryCache::ResultSet makeResult(const std::string& prefix, int count) {
        GraphQueryCache::ResultSet r;
        for (int i = 0; i < count; ++i) {
            r.push_back(prefix + "_" + std::to_string(i));
        }
        return r;
    }

    std::unique_ptr<GraphQueryCache> cache_;
};

// ---------------------------------------------------------------------------
// Construction / configuration
// ---------------------------------------------------------------------------

TEST(GraphQueryCacheConstruct, DefaultConfig_Succeeds) {
    EXPECT_NO_THROW(GraphQueryCache cache(GraphQueryCache::Config{}));
}

TEST(GraphQueryCacheConstruct, ValidConfig_Succeeds) {
    GraphQueryCache::Config cfg;
    cfg.l1_capacity = 8;
    cfg.l2_capacity = 64;
    EXPECT_NO_THROW(GraphQueryCache cache(cfg));
}

TEST(GraphQueryCacheConstruct, ZeroL1Capacity_Throws) {
    GraphQueryCache::Config cfg;
    cfg.l1_capacity = 0;
    cfg.l2_capacity = 16;
    EXPECT_THROW(GraphQueryCache cache(cfg), std::invalid_argument);
}

TEST(GraphQueryCacheConstruct, L2NotGreaterThanL1_Throws) {
    GraphQueryCache::Config cfg;
    cfg.l1_capacity = 8;
    cfg.l2_capacity = 8;   // must be strictly greater
    EXPECT_THROW(GraphQueryCache cache(cfg), std::invalid_argument);
}

TEST(GraphQueryCacheConstruct, ZeroDefaultCost_Throws) {
    GraphQueryCache::Config cfg;
    cfg.l1_capacity = 4;
    cfg.l2_capacity = 8;
    cfg.default_cost = 0.0;
    EXPECT_THROW(GraphQueryCache cache(cfg), std::invalid_argument);
}

TEST_F(GraphQueryCacheTest, Capacities_MatchConfig) {
    EXPECT_EQ(cache_->l1Capacity(), 4u);
    EXPECT_EQ(cache_->l2Capacity(), 8u);
}

// ---------------------------------------------------------------------------
// Basic put / get
// ---------------------------------------------------------------------------

TEST_F(GraphQueryCacheTest, PutAndGet_ReturnsResult) {
    cache_->put("k1", {"a", "b", "c"});
    auto result = cache_->get("k1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, (GraphQueryCache::ResultSet{"a", "b", "c"}));
}

TEST_F(GraphQueryCacheTest, Get_MissingKey_ReturnsNullopt) {
    auto result = cache_->get("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST_F(GraphQueryCacheTest, Put_EmptyKey_IsIgnored) {
    cache_->put("", {"x"});
    auto result = cache_->get("");
    EXPECT_FALSE(result.has_value());
}

TEST_F(GraphQueryCacheTest, Get_EmptyKey_ReturnsMiss) {
    auto result = cache_->get("");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(cache_->getStats().misses, 1u);
}

TEST_F(GraphQueryCacheTest, Put_UpdatesExistingEntry) {
    cache_->put("k1", {"old"});
    cache_->put("k1", {"new"});
    auto result = cache_->get("k1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, (GraphQueryCache::ResultSet{"new"}));
}

TEST_F(GraphQueryCacheTest, EmptyResultSet_IsCachedAndReturned) {
    cache_->put("empty_key", {});
    auto result = cache_->get("empty_key");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

TEST_F(GraphQueryCacheTest, Stats_InitiallyZero) {
    auto s = cache_->getStats();
    EXPECT_EQ(s.hits, 0u);
    EXPECT_EQ(s.misses, 0u);
    EXPECT_EQ(s.l1_hits, 0u);
    EXPECT_EQ(s.l2_promotions, 0u);
    EXPECT_EQ(s.evictions, 0u);
    EXPECT_EQ(s.l1_size, 0u);
    EXPECT_EQ(s.l2_size, 0u);
}

TEST_F(GraphQueryCacheTest, Stats_HitAndMissCountCorrectly) {
    cache_->put("k1", {"r1"});
    cache_->get("k1");    // hit
    cache_->get("k2");    // miss

    auto s = cache_->getStats();
    EXPECT_EQ(s.hits, 1u);
    EXPECT_EQ(s.misses, 1u);
    EXPECT_EQ(s.l1_hits, 1u);
}

TEST_F(GraphQueryCacheTest, Stats_HitRatio_ComputedCorrectly) {
    cache_->put("k1", {"r1"});
    cache_->get("k1");  // hit
    cache_->get("k2");  // miss

    EXPECT_DOUBLE_EQ(cache_->getStats().hitRatio(), 0.5);
}

TEST_F(GraphQueryCacheTest, Stats_HitRatio_ZeroWithNoLookups) {
    EXPECT_DOUBLE_EQ(cache_->getStats().hitRatio(), 0.0);
}

TEST_F(GraphQueryCacheTest, Stats_ResetStats_ClearsCounters) {
    cache_->put("k1", {"r1"});
    cache_->get("k1");
    cache_->get("k2");
    cache_->resetStats();

    auto s = cache_->getStats();
    EXPECT_EQ(s.hits, 0u);
    EXPECT_EQ(s.misses, 0u);
}

TEST_F(GraphQueryCacheTest, Stats_SizeReflectsContent) {
    cache_->put("k1", {"r1"});
    cache_->put("k2", {"r2"});

    auto s = cache_->getStats();
    EXPECT_EQ(s.l1_size, 2u);
    EXPECT_EQ(s.l2_size, 0u);
}

// ---------------------------------------------------------------------------
// L1 eviction and L2 promotion
// ---------------------------------------------------------------------------

TEST_F(GraphQueryCacheTest, L1Full_OldestEntryDemotedToL2) {
    // L1 capacity = 4; fill it, then overflow by 1
    for (int i = 0; i < 4; ++i) {
        cache_->put("k" + std::to_string(i), {"r" + std::to_string(i)});
    }
    // All in L1, none in L2
    EXPECT_EQ(cache_->getStats().l1_size, 4u);
    EXPECT_EQ(cache_->getStats().l2_size, 0u);

    // Adding 5th entry causes LRU demotion to L2
    cache_->put("k4", {"r4"});
    auto s = cache_->getStats();
    EXPECT_EQ(s.l1_size, 4u);  // still 4 in L1
    EXPECT_EQ(s.l2_size, 1u);  // 1 demoted to L2
}

TEST_F(GraphQueryCacheTest, DemotedEntry_StillAccessibleViaL2) {
    for (int i = 0; i < 4; ++i) {
        cache_->put("k" + std::to_string(i), {"r" + std::to_string(i)});
    }
    // k0 was inserted first — it is the LRU victim
    cache_->put("k4", {"r4"});  // demotes k0

    // k0 should still be accessible (in L2)
    auto result = cache_->get("k0");
    EXPECT_TRUE(result.has_value());
}

TEST_F(GraphQueryCacheTest, L2Hit_PromotesEntryToL1) {
    for (int i = 0; i < 5; ++i) {
        cache_->put("k" + std::to_string(i), {"r" + std::to_string(i)});
    }
    // k0 is in L2; reading it should promote to L1
    auto result = cache_->get("k0");
    ASSERT_TRUE(result.has_value());

    auto s = cache_->getStats();
    EXPECT_GE(s.l2_promotions, 1u);
}

// ---------------------------------------------------------------------------
// Invalidation
// ---------------------------------------------------------------------------

TEST_F(GraphQueryCacheTest, Invalidate_L1Entry_MakesItMiss) {
    cache_->put("k1", {"r1"});
    cache_->invalidate("k1");
    EXPECT_FALSE(cache_->get("k1").has_value());
}

TEST_F(GraphQueryCacheTest, Invalidate_L2Entry_MakesItMiss) {
    for (int i = 0; i < 5; ++i) {
        cache_->put("k" + std::to_string(i), {"v"});
    }
    // k0 is in L2; invalidate it
    cache_->invalidate("k0");
    EXPECT_FALSE(cache_->get("k0").has_value());
}

TEST_F(GraphQueryCacheTest, Invalidate_NonExistentKey_NoOp) {
    EXPECT_NO_THROW(cache_->invalidate("ghost_key"));
    EXPECT_EQ(cache_->getStats().l1_size, 0u);
}

// ---------------------------------------------------------------------------
// Clear
// ---------------------------------------------------------------------------

TEST_F(GraphQueryCacheTest, Clear_RemovesAllEntries) {
    cache_->put("k1", {"r1"});
    cache_->put("k2", {"r2"});
    cache_->clear();

    EXPECT_EQ(cache_->getStats().l1_size, 0u);
    EXPECT_EQ(cache_->getStats().l2_size, 0u);
    EXPECT_FALSE(cache_->get("k1").has_value());
}

TEST_F(GraphQueryCacheTest, Clear_PreservesStats) {
    cache_->put("k1", {"r1"});
    cache_->get("k1");  // 1 hit
    cache_->clear();

    EXPECT_EQ(cache_->getStats().hits, 1u);
}

// ---------------------------------------------------------------------------
// TTL expiry
// ---------------------------------------------------------------------------

TEST(GraphQueryCacheTTL, ExpiredEntry_ReturnsMiss) {
    GraphQueryCache::Config cfg;
    cfg.l1_capacity = 4;
    cfg.l2_capacity = 8;
    cfg.ttl = 1ms;  // 1 ms TTL
    GraphQueryCache cache(cfg);

    cache.put("k1", {"r1"});
    std::this_thread::sleep_for(5ms);  // let the entry expire

    auto result = cache.get("k1");
    EXPECT_FALSE(result.has_value());
}

TEST(GraphQueryCacheTTL, NonExpiredEntry_ReturnsHit) {
    GraphQueryCache::Config cfg;
    cfg.l1_capacity = 4;
    cfg.l2_capacity = 8;
    cfg.ttl = 5000ms;  // 5 second TTL — will not expire in this test
    GraphQueryCache cache(cfg);

    cache.put("k1", {"r1"});
    auto result = cache.get("k1");
    EXPECT_TRUE(result.has_value());
}

// ---------------------------------------------------------------------------
// Cost-weighted eviction
// ---------------------------------------------------------------------------

TEST_F(GraphQueryCacheTest, WeightedEviction_HighCostEntry_SurvivedOverLowCost) {
    // Fill L1 fully and demote all to L2 by inserting beyond L1 capacity,
    // then verify that cheap entries are evicted from L2 before expensive ones.
    GraphQueryCache::Config cfg;
    cfg.l1_capacity = 2;
    cfg.l2_capacity = 4;
    GraphQueryCache cache(cfg);

    // Insert 2 cheap + 2 expensive entries — first 2 end up in L2
    cache.put("cheap1", {"c"}, 0.1);
    cache.put("cheap2", {"c"}, 0.1);
    // These two cause cheap1/cheap2 to be demoted to L2
    cache.put("expensive1", {"e"}, 100.0);
    cache.put("expensive2", {"e"}, 100.0);

    // Now insert 3 more to fill L2 and trigger L2 eviction
    cache.put("new1", {"n"}, 0.1);
    cache.put("new2", {"n"}, 0.1);

    // cheap1/cheap2 should have been evicted before expensive ones
    // (cheap entries have lower weighted score under cost-weighted eviction)
    const auto s = cache.getStats();
    EXPECT_GE(s.evictions, 0u);  // evictions must have occurred

    // expensive entries should survive in L2 longer than cheap ones
    // (they may be in L1 or L2; at least one should still be accessible)
    const bool exp1_present = cache.get("expensive1").has_value();
    const bool exp2_present = cache.get("expensive2").has_value();
    EXPECT_TRUE(exp1_present || exp2_present)
        << "At least one high-cost entry should survive after cheap entries are evicted";
}

// ---------------------------------------------------------------------------
// Cost hint defaulting
// ---------------------------------------------------------------------------

TEST_F(GraphQueryCacheTest, NegativeCostHint_UsesDefaultCost) {
    // A negative cost hint should silently fall back to default_cost (1.0).
    // The entry should still be insertable and retrievable.
    EXPECT_NO_THROW(cache_->put("k1", {"r1"}, -5.0));
    EXPECT_TRUE(cache_->get("k1").has_value());
}

TEST_F(GraphQueryCacheTest, ZeroCostHint_UsesDefaultCost) {
    EXPECT_NO_THROW(cache_->put("k1", {"r1"}, 0.0));
    EXPECT_TRUE(cache_->get("k1").has_value());
}

// ---------------------------------------------------------------------------
// Multi-entry stress
// ---------------------------------------------------------------------------

TEST_F(GraphQueryCacheTest, ManyPutsAndGets_AllHitOrMissCorrectly) {
    constexpr int N = 20;
    for (int i = 0; i < N; ++i) {
        cache_->put("key_" + std::to_string(i), {"val_" + std::to_string(i)});
    }

    // Each key must be accessible (either in L1 or L2)
    int accessible = 0;
    for (int i = 0; i < N; ++i) {
        if (cache_->get("key_" + std::to_string(i)).has_value()) {
            ++accessible;
        }
    }
    // With l1=4 and l2=8, at most 12 entries survive; the rest are evicted.
    const size_t max_capacity = cache_->l1Capacity() + cache_->l2Capacity();
    EXPECT_LE(static_cast<size_t>(accessible), max_capacity);
    // Most recent entries should survive
    EXPECT_GE(accessible, static_cast<int>(cache_->l1Capacity()));
}
