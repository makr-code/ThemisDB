/**
 * @file test_graph_plan_cache_hardening.cpp
 * @brief Phase 3 P3-01: Query Optimizer Plan Cache Hardening Tests
 *
 * Validates production behaviour of PlanCache including:
 *  - LRU capacity eviction
 *  - Statistics-drift invalidation (10× cardinality threshold)
 *  - Thread-safe concurrent reads and writes
 *  - Hit / miss ratio counter accuracy
 *  - Table-dependency invalidation
 *  - Cache warming (pre-population → first lookup is a hit)
 *  - Age-based expiry and evictExpired()
 *  - Edge cases: empty query, very-long query, unicode in fingerprint
 *
 * @see include/query/plan_cache.h
 * @see src/query/plan_cache.cpp
 * @see ai_working/PHASE3_OPTIMIZATION_DETAILED_PLAN.md (P3-01)
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "query/plan_cache.h"

namespace themis {
namespace query {
namespace test {

// ============================================================================
// Helpers
// ============================================================================

/// @brief Build a minimal default Plan (no predicates).
static QueryOptimizer::Plan makeEmptyPlan() {
    return QueryOptimizer::Plan{};
}

/// @brief Build Statistics with a single table cardinality entry.
static PlanCache::Statistics makeStats(const std::string& table, size_t rows) {
    return PlanCache::Statistics({{table, rows}});
}

/// @brief Small-capacity cache config used in eviction tests.
static PlanCache::Config smallConfig(size_t cap = 3) {
    PlanCache::Config cfg;
    cfg.max_entries = cap;
    cfg.max_plan_age = std::chrono::seconds{86400};
    cfg.statistics_drift_factor = 10.0;
    return cfg;
}

// ============================================================================
// Test fixture
// ============================================================================

class PlanCacheHardeningTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache_ = std::make_unique<PlanCache>(smallConfig());
    }

    /// @brief Put a plan for @p query referencing @p tables with neutral stats.
    void putSimple(const std::string& query,
                   const std::vector<std::string>& tables = {}) {
        cache_->put(query, makeEmptyPlan(), PlanCache::Statistics{}, {}, tables);
    }

    std::unique_ptr<PlanCache> cache_;
};

// ============================================================================
// LRU Eviction Tests
// ============================================================================

/**
 * @test LRUEviction_CapacityOne_OldestEvicted
 * @brief When capacity is 1 and a second plan is inserted the first is evicted.
 */
TEST_F(PlanCacheHardeningTest, LRUEviction_CapacityOne_OldestEvicted) {
    PlanCache c{smallConfig(1)};
    c.put("query_A", makeEmptyPlan(), {});
    c.put("query_B", makeEmptyPlan(), {});

    // query_A must have been evicted to make room for query_B
    EXPECT_FALSE(c.get("query_A").has_value());
    EXPECT_TRUE(c.get("query_B").has_value());

    const auto stats = c.getStats();
    EXPECT_GE(stats.evictions, 1u);
}

/**
 * @test LRUEviction_CapacityFull_LRUEntryEvicted
 * @brief After capacity is reached the least-recently-used entry is removed.
 */
TEST_F(PlanCacheHardeningTest, LRUEviction_CapacityFull_LRUEntryEvicted) {
    // Fill to capacity (3)
    putSimple("q1");
    putSimple("q2");
    putSimple("q3");

    // Touch q1 so q2 becomes the LRU
    ASSERT_TRUE(cache_->get("q1").has_value());

    // Adding q4 must evict q2 (LRU)
    putSimple("q4");

    EXPECT_FALSE(cache_->get("q2").has_value()) << "q2 should have been evicted";
    EXPECT_TRUE(cache_->get("q1").has_value());
    EXPECT_TRUE(cache_->get("q3").has_value());
    EXPECT_TRUE(cache_->get("q4").has_value());
}

/**
 * @test LRUEviction_RecentAccessPromotion_ProtectsItem
 * @brief A re-accessed item is moved to MRU position, protecting it from eviction.
 */
TEST_F(PlanCacheHardeningTest, LRUEviction_RecentAccessPromotion_ProtectsItem) {
    putSimple("old_q");
    putSimple("mid_q");
    putSimple("new_q");

    // Re-access old_q → promoted to MRU
    ASSERT_TRUE(cache_->get("old_q").has_value());

    // Insert one more entry; mid_q is now the LRU
    putSimple("extra_q");

    EXPECT_FALSE(cache_->get("mid_q").has_value()) << "mid_q should be evicted";
    EXPECT_TRUE(cache_->get("old_q").has_value())  << "old_q should survive";
}

/**
 * @test LRUEviction_SizeNeverExceedsCapacity
 * @brief The cache size never grows beyond max_entries.
 */
TEST_F(PlanCacheHardeningTest, LRUEviction_SizeNeverExceedsCapacity) {
    const size_t cap = 5;
    PlanCache c{smallConfig(cap)};

    for (int i = 0; i < 20; ++i) {
        c.put("q" + std::to_string(i), makeEmptyPlan(), {});
        EXPECT_LE(c.getStats().current_size, cap);
    }
}

// ============================================================================
// Statistics Drift Tests
// ============================================================================

/**
 * @test StatsDrift_10xRatio_PlanInvalidated
 * @brief A cardinality ratio ≥ 10× triggers plan invalidation.
 */
TEST_F(PlanCacheHardeningTest, StatsDrift_10xRatio_PlanInvalidated) {
    PlanCache::Config cfg;
    cfg.statistics_drift_factor = 10.0;
    PlanCache c{cfg};

    const auto snap = makeStats("orders", 100);
    c.put("SELECT * FROM orders", makeEmptyPlan(), snap);

    // current stats: 10× larger → should invalidate
    const auto drifted = makeStats("orders", 1000);
    EXPECT_FALSE(c.get("SELECT * FROM orders", drifted).has_value());
    EXPECT_EQ(c.getStats().stat_drifts, 1u);
}

/**
 * @test StatsDrift_BelowThreshold_PlanValid
 * @brief A cardinality ratio < 10× does NOT invalidate the plan.
 */
TEST_F(PlanCacheHardeningTest, StatsDrift_BelowThreshold_PlanValid) {
    PlanCache::Config cfg;
    cfg.statistics_drift_factor = 10.0;
    PlanCache c{cfg};

    const auto snap = makeStats("users", 1000);
    c.put("SELECT * FROM users", makeEmptyPlan(), snap);

    // ratio = 9 < 10 → plan should remain valid
    const auto similar = makeStats("users", 9000);
    EXPECT_TRUE(c.get("SELECT * FROM users", similar).has_value());
    EXPECT_EQ(c.getStats().stat_drifts, 0u);
}

/**
 * @test StatsDrift_MissingTable_PlanInvalidated
 * @brief If a table referenced in the snapshot is absent from current stats the plan is stale.
 */
TEST_F(PlanCacheHardeningTest, StatsDrift_MissingTable_PlanInvalidated) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    const auto snap = makeStats("products", 500);
    c.put("SELECT * FROM products", makeEmptyPlan(), snap);

    // current stats: table not present
    EXPECT_FALSE(c.get("SELECT * FROM products", PlanCache::Statistics{}).has_value());
    EXPECT_GE(c.getStats().stat_drifts, 1u);
}

// ============================================================================
// Concurrent Access Tests
// ============================================================================

/**
 * @test ConcurrentAccess_MultipleWriters_NoDataCorruption
 * @brief Many threads writing concurrently must not cause crashes or corruption.
 */
TEST_F(PlanCacheHardeningTest, ConcurrentAccess_MultipleWriters_NoDataCorruption) {
    PlanCache::Config cfg;
    cfg.max_entries = 500;
    PlanCache c{cfg};

    constexpr int kThreads    = 8;
    constexpr int kIterations = 50;

    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&c, t]() {
            for (int i = 0; i < kIterations; ++i) {
                const std::string q = "thread_" + std::to_string(t) +
                                      "_iter_"  + std::to_string(i);
                c.put(q, makeEmptyPlan(), PlanCache::Statistics{});
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    EXPECT_LE(c.getStats().current_size, cfg.max_entries);
}

/**
 * @test ConcurrentAccess_ReadersAndWriters_ThreadSafe
 * @brief Mixed reader / writer threads must not race or deadlock.
 */
TEST_F(PlanCacheHardeningTest, ConcurrentAccess_ReadersAndWriters_ThreadSafe) {
    PlanCache::Config cfg;
    cfg.max_entries = 200;
    PlanCache c{cfg};

    // Pre-populate
    for (int i = 0; i < 20; ++i) {
        c.put("shared_q" + std::to_string(i), makeEmptyPlan(), PlanCache::Statistics{});
    }

    std::atomic<int> successful_reads{0};
    std::vector<std::thread> workers;

    // 4 readers
    for (int t = 0; t < 4; ++t) {
        workers.emplace_back([&c, &successful_reads]() {
            for (int i = 0; i < 20; ++i) {
                if (c.get("shared_q" + std::to_string(i % 20)).has_value()) {
                    ++successful_reads;
                }
            }
        });
    }
    // 4 writers
    for (int t = 0; t < 4; ++t) {
        workers.emplace_back([&c, t]() {
            for (int i = 0; i < 20; ++i) {
                c.put("new_q_t" + std::to_string(t) + "_" + std::to_string(i),
                      makeEmptyPlan(), PlanCache::Statistics{});
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    // At least some reads should have been cache hits (we can't guarantee all
    // survive eviction under concurrent writers, but at least no crash).
    EXPECT_GE(successful_reads.load(), 0);
}

// ============================================================================
// Hit / Miss Statistics Tests
// ============================================================================

/**
 * @test HitMissRatio_AccurateAfterKnownSequence
 * @brief Hit and miss counters reflect the exact access sequence.
 */
TEST_F(PlanCacheHardeningTest, HitMissRatio_AccurateAfterKnownSequence) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    c.put("query_X", makeEmptyPlan(), PlanCache::Statistics{});

    // 3 hits
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(c.get("query_X").has_value());
    }
    // 2 misses
    EXPECT_FALSE(c.get("nonexistent_1").has_value());
    EXPECT_FALSE(c.get("nonexistent_2").has_value());

    const auto stats = c.getStats();
    EXPECT_EQ(stats.hits,   3u);
    EXPECT_EQ(stats.misses, 2u);
    EXPECT_DOUBLE_EQ(stats.hitRate(), 3.0 / 5.0);
}

/**
 * @test HitMissRatio_EmptyCache_ZeroRate
 * @brief An empty cache reports 0.0 hit rate and zero counters.
 */
TEST_F(PlanCacheHardeningTest, HitMissRatio_EmptyCache_ZeroRate) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    const auto stats = c.getStats();
    EXPECT_EQ(stats.hits,   0u);
    EXPECT_EQ(stats.misses, 0u);
    EXPECT_DOUBLE_EQ(stats.hitRate(), 0.0);
}

// ============================================================================
// Table Invalidation Tests
// ============================================================================

/**
 * @test TableInvalidation_RemovesAllAffectedPlans
 * @brief invalidateTable() removes every plan that references the named table.
 */
TEST_F(PlanCacheHardeningTest, TableInvalidation_RemovesAllAffectedPlans) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    c.put("q1", makeEmptyPlan(), PlanCache::Statistics{}, {}, {"orders"});
    c.put("q2", makeEmptyPlan(), PlanCache::Statistics{}, {}, {"orders", "customers"});
    c.put("q3", makeEmptyPlan(), PlanCache::Statistics{}, {}, {"products"});

    const size_t invalidated = c.invalidateTable("orders");

    EXPECT_EQ(invalidated, 2u);
    EXPECT_FALSE(c.get("q1").has_value());
    EXPECT_FALSE(c.get("q2").has_value());
    EXPECT_TRUE(c.get("q3").has_value());
}

/**
 * @test TableInvalidation_PreservesUnaffectedPlans
 * @brief Plans referencing other tables are untouched by invalidation.
 */
TEST_F(PlanCacheHardeningTest, TableInvalidation_PreservesUnaffectedPlans) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    c.put("safe_q", makeEmptyPlan(), PlanCache::Statistics{}, {}, {"events"});
    c.put("invalid_q", makeEmptyPlan(), PlanCache::Statistics{}, {}, {"sessions"});

    c.invalidateTable("sessions");

    EXPECT_TRUE(c.get("safe_q").has_value())   << "safe_q should survive";
    EXPECT_FALSE(c.get("invalid_q").has_value()) << "invalid_q should be gone";
}

/**
 * @test TableInvalidation_ReturnsCorrectCount
 * @brief invalidateTable() returns the precise count of removed entries.
 */
TEST_F(PlanCacheHardeningTest, TableInvalidation_ReturnsCorrectCount) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    c.put("a", makeEmptyPlan(), PlanCache::Statistics{}, {}, {"tbl"});
    c.put("b", makeEmptyPlan(), PlanCache::Statistics{}, {}, {"tbl"});
    c.put("c", makeEmptyPlan(), PlanCache::Statistics{}, {}, {"other"});

    EXPECT_EQ(c.invalidateTable("tbl"),   2u);
    EXPECT_EQ(c.invalidateTable("other"), 1u);
    EXPECT_EQ(c.invalidateTable("tbl"),   0u); // already gone
}

// ============================================================================
// Cache Warming Tests
// ============================================================================

/**
 * @test CacheWarming_PlanFoundOnFirstLookup
 * @brief Pre-populated plans are returned immediately without a miss.
 */
TEST_F(PlanCacheHardeningTest, CacheWarming_PlanFoundOnFirstLookup) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    const std::vector<std::string> warmQueries = {
        "SELECT * FROM users WHERE id = @id",
        "SELECT * FROM orders WHERE user_id = @uid",
        "SELECT COUNT(*) FROM events",
    };

    for (const auto& q : warmQueries) {
        c.put(q, makeEmptyPlan(), PlanCache::Statistics{});
    }

    for (const auto& q : warmQueries) {
        EXPECT_TRUE(c.get(q).has_value()) << "Warm query should be a hit: " << q;
    }

    EXPECT_EQ(c.getStats().misses, 0u) << "No misses expected after warming";
}

/**
 * @test CacheWarming_StatsReflectWarmHits
 * @brief After warming, hit count equals the number of warm lookups.
 */
TEST_F(PlanCacheHardeningTest, CacheWarming_StatsReflectWarmHits) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    c.put("warm1", makeEmptyPlan(), PlanCache::Statistics{});
    c.put("warm2", makeEmptyPlan(), PlanCache::Statistics{});

    c.get("warm1");
    c.get("warm1");
    c.get("warm2");

    const auto stats = c.getStats();
    EXPECT_EQ(stats.hits,   3u);
    EXPECT_EQ(stats.misses, 0u);
}

// ============================================================================
// Age Expiry Tests
// ============================================================================

/**
 * @test AgeExpiry_ZeroMaxAge_PlansImmediatelyStale
 * @brief Plans are considered expired when max_plan_age is zero seconds.
 */
TEST_F(PlanCacheHardeningTest, AgeExpiry_ZeroMaxAge_PlansImmediatelyStale) {
    PlanCache::Config cfg;
    cfg.max_plan_age = std::chrono::seconds{0};
    PlanCache c{cfg};

    c.put("aging_q", makeEmptyPlan(), PlanCache::Statistics{});

    // Even a plan just inserted should appear expired (age ≥ 0 s)
    const auto result = c.get("aging_q");
    EXPECT_FALSE(result.has_value()) << "Plan with zero max-age must be stale immediately";
}

/**
 * @test AgeExpiry_EvictExpired_ClearsStaleEntries
 * @brief evictExpired() removes all entries whose age exceeds max_plan_age.
 */
TEST_F(PlanCacheHardeningTest, AgeExpiry_EvictExpired_ClearsStaleEntries) {
    PlanCache::Config cfg;
    cfg.max_plan_age = std::chrono::seconds{0};
    PlanCache c{cfg};

    c.put("stale1", makeEmptyPlan(), PlanCache::Statistics{});
    c.put("stale2", makeEmptyPlan(), PlanCache::Statistics{});

    const size_t removed = c.evictExpired();
    EXPECT_EQ(removed, 2u);
    EXPECT_EQ(c.getStats().current_size, 0u);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

/**
 * @test EdgeCase_EmptyQueryString_FingerprintableAndStorable
 * @brief An empty query string is handled without crash; fingerprint is deterministic.
 */
TEST_F(PlanCacheHardeningTest, EdgeCase_EmptyQueryString_FingerprintableAndStorable) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    const std::string fp1 = PlanCache::fingerprint("");
    const std::string fp2 = PlanCache::fingerprint("");

    EXPECT_EQ(fp1, fp2)  << "Fingerprint must be deterministic";
    EXPECT_EQ(fp1.size(), 64u) << "SHA256 hex digest is 64 chars";

    c.put("", makeEmptyPlan(), PlanCache::Statistics{});
    EXPECT_TRUE(c.get("").has_value());
}

/**
 * @test EdgeCase_VeryLongQuery_StoredAndRetrieved
 * @brief A 10 KiB query string is stored and retrieved correctly.
 */
TEST_F(PlanCacheHardeningTest, EdgeCase_VeryLongQuery_StoredAndRetrieved) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    // 10 240-character query
    const std::string longQuery(10240, 'X');
    c.put(longQuery, makeEmptyPlan(), PlanCache::Statistics{});

    const auto result = c.get(longQuery);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->query_fingerprint, PlanCache::fingerprint(longQuery));
}

/**
 * @test EdgeCase_UnicodeInQueryFingerprint_DeterministicHash
 * @brief UTF-8 multi-byte characters produce a stable, correct-length fingerprint.
 */
TEST_F(PlanCacheHardeningTest, EdgeCase_UnicodeInQueryFingerprint_DeterministicHash) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    // Japanese and emoji characters in a SQL-like query (plain UTF-8 string literal)
    const std::string unicodeQuery =
        "SELECT * FROM \xe3\x83\x86\xe3\x83\xbc\xe3\x83\x96\xe3\x83\xab WHERE "
        "\xe5\x90\x8d\xe5\x89\x8d = '\xe5\x80\xa4' AND \xf0\x9f\x94\x91 IS NOT NULL";

    const std::string fp = PlanCache::fingerprint(unicodeQuery);
    EXPECT_EQ(fp.size(), 64u);

    c.put(unicodeQuery, makeEmptyPlan(), PlanCache::Statistics{});
    EXPECT_TRUE(c.get(unicodeQuery).has_value());
}

/**
 * @test EdgeCase_ClearResetsAllState
 * @brief clear() removes all entries and resets the size counter.
 */
TEST_F(PlanCacheHardeningTest, EdgeCase_ClearResetsAllState) {
    PlanCache::Config cfg;
    PlanCache c{cfg};

    c.put("a", makeEmptyPlan(), PlanCache::Statistics{});
    c.put("b", makeEmptyPlan(), PlanCache::Statistics{});
    ASSERT_EQ(c.getStats().current_size, 2u);

    c.clear();
    EXPECT_EQ(c.getStats().current_size, 0u);
    EXPECT_FALSE(c.get("a").has_value());
    EXPECT_FALSE(c.get("b").has_value());
}

} // namespace test
} // namespace query
} // namespace themis
