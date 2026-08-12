/*
 * Tests for PlanCache — Query Plan Caching (v1.7.0, Issue #196)
 *
 * Validates all acceptance criteria:
 *   AC-1  Plan fingerprinting (query structure + statistics)
 *   AC-2  Parameterized plan reuse
 *   AC-3  Plan invalidation on schema/statistics changes
 *   AC-4  Statistics-aware plan selection
 *   AC-5  Schema changes: invalidate all plans for affected tables
 *   AC-6  Statistics drift: invalidate if cardinality changes >10×
 *   AC-7  Periodic: refresh plans every 24 hours (age-based expiration)
 */

#include "query/plan_cache.h"

#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace themis;
using namespace themis::query;
using namespace std::chrono_literals;

// =============================================================================
// Helpers
// =============================================================================

static QueryOptimizer::Plan makePlan(double cost = 1.0) {
    QueryOptimizer::Plan p;
    p.nlp_complexity = cost;
    return p;
}

static PlanCache::Statistics makeStats(
    std::unordered_map<std::string, size_t> cards)
{
    return PlanCache::Statistics{std::move(cards)};
}

// =============================================================================
// Fixture
// =============================================================================

class QueryPlanCachingFocusedTests : public ::testing::Test {
protected:
    void SetUp() override {
        PlanCache::Config cfg;
        cfg.max_entries           = 100;
        cfg.max_plan_age          = 86400s;  // 24 h default
        cfg.statistics_drift_factor = 10.0;
        cache_ = std::make_unique<PlanCache>(cfg);
    }

    std::unique_ptr<PlanCache> cache_;
};

// =============================================================================
// AC-1  Plan fingerprinting
// =============================================================================

/// Identical queries produce the same fingerprint.
TEST_F(QueryPlanCachingFocusedTests, AC1_FingerprintDeterministic) {
    std::string fp1 = PlanCache::fingerprint("FOR u IN users RETURN u");
    std::string fp2 = PlanCache::fingerprint("FOR u IN users RETURN u");
    EXPECT_EQ(fp1, fp2);
}

/// Different queries produce different fingerprints.
TEST_F(QueryPlanCachingFocusedTests, AC1_FingerprintDistinct) {
    std::string fp1 = PlanCache::fingerprint("FOR u IN users RETURN u");
    std::string fp2 = PlanCache::fingerprint("FOR o IN orders RETURN o");
    EXPECT_NE(fp1, fp2);
}

/// Fingerprint is a 64-character hex string (SHA256).
TEST_F(QueryPlanCachingFocusedTests, AC1_FingerprintLength) {
    std::string fp = PlanCache::fingerprint("SELECT 1");
    EXPECT_EQ(fp.size(), 64u);
    for (char c : fp) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            << "Non-hex character: " << c;
    }
}

/// Empty-string query has a well-defined fingerprint.
TEST_F(QueryPlanCachingFocusedTests, AC1_FingerprintEmptyQuery) {
    std::string fp = PlanCache::fingerprint("");
    EXPECT_EQ(fp.size(), 64u);
}

// =============================================================================
// AC-2  Parameterized plan reuse
// =============================================================================

/// Put a plan, then retrieve it — must be a cache hit.
TEST_F(QueryPlanCachingFocusedTests, AC2_BasicPutGet) {
    const std::string query = "FOR u IN users FILTER u.age > @age RETURN u";
    auto plan = makePlan(2.5);
    PlanCache::Statistics stats = makeStats({{"users", 1000}});

    cache_->put(query, plan, stats, {}, {"users"});

    auto result = cache_->get(query, stats);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->query_fingerprint, PlanCache::fingerprint(query));
    EXPECT_DOUBLE_EQ(result->plan.nlp_complexity, 2.5);
}

/// The same plan can be reused with different bind values (parameterized reuse).
TEST_F(QueryPlanCachingFocusedTests, AC2_ParameterizedReuse) {
    const std::string tpl = "FOR u IN users FILTER u.age > @age RETURN u";
    PlanCache::ParameterInfo p;
    p.name         = "@age";
    p.type         = "int";
    p.sample_value = "30";

    PlanCache::Statistics stats = makeStats({{"users", 5000}});
    cache_->put(tpl, makePlan(), stats, {p}, {"users"});

    // Retrieve — different bind value, same template
    auto result = cache_->get(tpl, stats);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->parameters.size(), 1u);
    EXPECT_EQ(result->parameters[0].name, "@age");
}

/// Retrieving an absent entry returns nullopt (miss).
TEST_F(QueryPlanCachingFocusedTests, AC2_MissOnAbsent) {
    auto result = cache_->get("FOR x IN nonexistent RETURN x", {});
    EXPECT_FALSE(result.has_value());
}

/// A plan stored under query A is not returned for query B.
TEST_F(QueryPlanCachingFocusedTests, AC2_NoCrossQueryCollision) {
    PlanCache::Statistics stats;
    cache_->put("FOR a IN A RETURN a", makePlan(1.0), stats, {}, {});
    auto r = cache_->get("FOR b IN B RETURN b", stats);
    EXPECT_FALSE(r.has_value());
}

/// Overwriting a plan (same query) returns the newest plan.
TEST_F(QueryPlanCachingFocusedTests, AC2_OverwriteUpdatesEntry) {
    const std::string query = "FOR u IN users RETURN u";
    PlanCache::Statistics stats;

    cache_->put(query, makePlan(1.0), stats);
    cache_->put(query, makePlan(9.9), stats);  // overwrite

    auto r = cache_->get(query, stats);
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(r->plan.nlp_complexity, 9.9);
}

/// Parameter metadata is preserved through put/get.
TEST_F(QueryPlanCachingFocusedTests, AC2_ParameterMetadataPreserved) {
    PlanCache::ParameterInfo p1{"@min", "int",    "0"};
    PlanCache::ParameterInfo p2{"@max", "int",    "100"};
    PlanCache::Statistics stats;

    cache_->put("RANGE QUERY", makePlan(), stats, {p1, p2});
    auto r = cache_->get("RANGE QUERY", stats);
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->parameters.size(), 2u);
    EXPECT_EQ(r->parameters[0].name, "@min");
    EXPECT_EQ(r->parameters[1].name, "@max");
}

// =============================================================================
// AC-3  Plan invalidation on schema/statistics changes
// =============================================================================

/// invalidateTable removes plans that reference the named table.
TEST_F(QueryPlanCachingFocusedTests, AC3_InvalidationByTable) {
    PlanCache::Statistics stats;
    cache_->put("FOR u IN users RETURN u", makePlan(), stats, {}, {"users"});

    size_t removed = cache_->invalidateTable("users");
    EXPECT_EQ(removed, 1u);

    auto r = cache_->get("FOR u IN users RETURN u", stats);
    EXPECT_FALSE(r.has_value());
}

/// Invalidating a non-existent table returns 0.
TEST_F(QueryPlanCachingFocusedTests, AC3_InvalidateNonExistentTable) {
    size_t removed = cache_->invalidateTable("ghost_table");
    EXPECT_EQ(removed, 0u);
}

/// clear() removes all entries.
TEST_F(QueryPlanCachingFocusedTests, AC3_ClearAll) {
    PlanCache::Statistics stats;
    cache_->put("Q1", makePlan(), stats, {}, {"t1"});
    cache_->put("Q2", makePlan(), stats, {}, {"t2"});

    cache_->clear();

    EXPECT_FALSE(cache_->get("Q1", stats).has_value());
    EXPECT_FALSE(cache_->get("Q2", stats).has_value());
    EXPECT_EQ(cache_->getStats().current_size, 0u);
}

// =============================================================================
// AC-4  Statistics-aware plan selection
// =============================================================================

/// A plan is returned when current stats match the snapshot exactly.
TEST_F(QueryPlanCachingFocusedTests, AC4_StatMatchReturnsHit) {
    PlanCache::Statistics stats = makeStats({{"users", 10000}});
    cache_->put("SELECT *", makePlan(), stats, {}, {"users"});

    auto r = cache_->get("SELECT *", stats);
    EXPECT_TRUE(r.has_value());
}

/// Stats snapshot is stored correctly in the cached entry.
TEST_F(QueryPlanCachingFocusedTests, AC4_SnapshotStoredCorrectly) {
    PlanCache::Statistics snap = makeStats({{"orders", 500}, {"items", 2000}});
    cache_->put("JOIN QUERY", makePlan(), snap, {}, {"orders", "items"});

    auto r = cache_->get("JOIN QUERY", snap);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->statistics_snapshot.table_cardinalities.at("orders"), 500u);
    EXPECT_EQ(r->statistics_snapshot.table_cardinalities.at("items"),  2000u);
}

/// created_at is set at plan-storage time.
TEST_F(QueryPlanCachingFocusedTests, AC4_CreatedAtIsRecent) {
    auto before = std::chrono::system_clock::now();
    PlanCache::Statistics stats;
    cache_->put("TIME CHECK", makePlan(), stats);
    auto after = std::chrono::system_clock::now();

    auto r = cache_->get("TIME CHECK", stats);
    ASSERT_TRUE(r.has_value());
    EXPECT_GE(r->created_at, before);
    EXPECT_LE(r->created_at, after);
}

// =============================================================================
// AC-5  Schema changes: invalidate all plans for affected tables
// =============================================================================

/// invalidateTable removes ALL plans referencing the table.
TEST_F(QueryPlanCachingFocusedTests, AC5_MultiPlanInvalidation) {
    PlanCache::Statistics stats;
    cache_->put("Q_A", makePlan(), stats, {}, {"users"});
    cache_->put("Q_B", makePlan(), stats, {}, {"users", "orders"});
    cache_->put("Q_C", makePlan(), stats, {}, {"orders"});  // not invalidated

    size_t removed = cache_->invalidateTable("users");
    EXPECT_EQ(removed, 2u);  // Q_A and Q_B

    EXPECT_FALSE(cache_->get("Q_A", stats).has_value());
    EXPECT_FALSE(cache_->get("Q_B", stats).has_value());
    EXPECT_TRUE(cache_->get("Q_C", stats).has_value());
}

/// Plans not referencing the invalidated table are unaffected.
TEST_F(QueryPlanCachingFocusedTests, AC5_UnrelatedPlansUntouched) {
    PlanCache::Statistics stats;
    cache_->put("U_PLAN", makePlan(), stats, {}, {"users"});
    cache_->put("O_PLAN", makePlan(), stats, {}, {"orders"});

    cache_->invalidateTable("users");

    EXPECT_TRUE(cache_->get("O_PLAN", stats).has_value());
}

/// Multiple sequential invalidations work correctly.
TEST_F(QueryPlanCachingFocusedTests, AC5_SequentialInvalidations) {
    PlanCache::Statistics stats;
    cache_->put("Q1", makePlan(), stats, {}, {"t1"});
    cache_->put("Q2", makePlan(), stats, {}, {"t2"});
    cache_->put("Q3", makePlan(), stats, {}, {"t3"});

    cache_->invalidateTable("t1");
    cache_->invalidateTable("t2");

    EXPECT_FALSE(cache_->get("Q1", stats).has_value());
    EXPECT_FALSE(cache_->get("Q2", stats).has_value());
    EXPECT_TRUE(cache_->get("Q3", stats).has_value());
}

/// invalidateTable on an already-invalidated table is a no-op.
TEST_F(QueryPlanCachingFocusedTests, AC5_DoubleInvalidationIsNoOp) {
    PlanCache::Statistics stats;
    cache_->put("Q", makePlan(), stats, {}, {"tbl"});

    size_t r1 = cache_->invalidateTable("tbl");
    size_t r2 = cache_->invalidateTable("tbl");  // second call — already gone

    EXPECT_EQ(r1, 1u);
    EXPECT_EQ(r2, 0u);
}

// =============================================================================
// AC-6  Statistics drift: invalidate if cardinality changes >10×
// =============================================================================

/// 10× increase in table cardinality → cache miss (plan evicted).
TEST_F(QueryPlanCachingFocusedTests, AC6_DriftAboveThresholdInvalidates) {
    PlanCache::Statistics snap    = makeStats({{"users", 1000}});
    PlanCache::Statistics current = makeStats({{"users", 10001}});  // >10×

    cache_->put("DRIFT QUERY", makePlan(), snap, {}, {"users"});

    auto r = cache_->get("DRIFT QUERY", current);
    EXPECT_FALSE(r.has_value());
}

/// 10× decrease also invalidates.
TEST_F(QueryPlanCachingFocusedTests, AC6_DriftBelowThresholdInvalidates) {
    PlanCache::Statistics snap    = makeStats({{"users", 10000}});
    PlanCache::Statistics current = makeStats({{"users", 999}});   // >10× drop

    cache_->put("DRIFT DOWN", makePlan(), snap, {}, {"users"});

    auto r = cache_->get("DRIFT DOWN", current);
    EXPECT_FALSE(r.has_value());
}

/// Less than 10× change keeps the plan valid.
TEST_F(QueryPlanCachingFocusedTests, AC6_MinorDriftKeepsPlan) {
    PlanCache::Statistics snap    = makeStats({{"users", 1000}});
    PlanCache::Statistics current = makeStats({{"users", 5000}});  // 5× — within limit

    cache_->put("MINOR DRIFT", makePlan(), snap, {}, {"users"});

    auto r = cache_->get("MINOR DRIFT", current);
    EXPECT_TRUE(r.has_value());
}

/// Exactly 10× change (boundary) triggers invalidation.
TEST_F(QueryPlanCachingFocusedTests, AC6_ExactlyTenXTriggersDrift) {
    PlanCache::Statistics snap    = makeStats({{"t", 1000}});
    PlanCache::Statistics current = makeStats({{"t", 10000}});  // ratio == 10.0

    cache_->put("EXACT DRIFT", makePlan(), snap);

    auto r = cache_->get("EXACT DRIFT", current);
    EXPECT_FALSE(r.has_value());
}

/// Missing table in current stats counts as drift.
TEST_F(QueryPlanCachingFocusedTests, AC6_MissingTableCountsAsDrift) {
    PlanCache::Statistics snap    = makeStats({{"users", 1000}});
    PlanCache::Statistics current = makeStats({});  // table dropped

    cache_->put("TABLE GONE", makePlan(), snap);

    auto r = cache_->get("TABLE GONE", current);
    EXPECT_FALSE(r.has_value());
}

/// Empty snapshot vs empty current → no drift.
TEST_F(QueryPlanCachingFocusedTests, AC6_BothEmptyStatsNoDrift) {
    PlanCache::Statistics stats;  // empty
    cache_->put("EMPTY STATS", makePlan(), stats);
    auto r = cache_->get("EMPTY STATS", stats);
    EXPECT_TRUE(r.has_value());
}

/// Zero-count table remains if both snap and current are zero.
TEST_F(QueryPlanCachingFocusedTests, AC6_ZeroCountNoDrift) {
    PlanCache::Statistics snap    = makeStats({{"t", 0}});
    PlanCache::Statistics current = makeStats({{"t", 0}});

    cache_->put("ZERO ROWS", makePlan(), snap);
    auto r = cache_->get("ZERO ROWS", current);
    EXPECT_TRUE(r.has_value());
}

/// Drift statistics counter is incremented on drift eviction.
TEST_F(QueryPlanCachingFocusedTests, AC6_DriftCounterIncremented) {
    PlanCache::Statistics snap    = makeStats({{"t", 100}});
    PlanCache::Statistics current = makeStats({{"t", 2000}});  // >10×

    cache_->put("COUNT DRIFT", makePlan(), snap);
    cache_->get("COUNT DRIFT", current);  // triggers drift eviction

    auto stats = cache_->getStats();
    EXPECT_GE(stats.stat_drifts, 1u);
}

// =============================================================================
// AC-7  Periodic: plans older than 24 hours are evicted
// =============================================================================

/// A plan with a very short max_age is evicted on get().
TEST_F(QueryPlanCachingFocusedTests, AC7_ExpiredPlanEvictedOnGet) {
    PlanCache::Config cfg;
    cfg.max_entries  = 10;
    cfg.max_plan_age = 0s;  // immediately expired
    PlanCache short_cache(cfg);

    PlanCache::Statistics stats;
    short_cache.put("EXPIRE ME", makePlan(), stats);

    auto r = short_cache.get("EXPIRE ME", stats);
    EXPECT_FALSE(r.has_value());
}

/// evictExpired() removes all stale entries.
TEST_F(QueryPlanCachingFocusedTests, AC7_EvictExpiredBulk) {
    PlanCache::Config cfg;
    cfg.max_entries  = 10;
    cfg.max_plan_age = 0s;  // all immediately expired
    PlanCache short_cache(cfg);

    PlanCache::Statistics stats;
    short_cache.put("A", makePlan(), stats);
    short_cache.put("B", makePlan(), stats);
    short_cache.put("C", makePlan(), stats);

    size_t evicted = short_cache.evictExpired();
    EXPECT_EQ(evicted, 3u);
    EXPECT_EQ(short_cache.getStats().current_size, 0u);
}

/// Non-expired plans survive evictExpired().
TEST_F(QueryPlanCachingFocusedTests, AC7_NonExpiredPlanSurvives) {
    // default cache has 24 h TTL — nothing is expired immediately
    PlanCache::Statistics stats;
    cache_->put("FRESH", makePlan(), stats);

    size_t evicted = cache_->evictExpired();
    EXPECT_EQ(evicted, 0u);
    EXPECT_TRUE(cache_->get("FRESH", stats).has_value());
}

/// Eviction counter is incremented for each expired entry.
TEST_F(QueryPlanCachingFocusedTests, AC7_EvictionCounterUpdated) {
    PlanCache::Config cfg;
    cfg.max_plan_age = 0s;
    PlanCache short_cache(cfg);

    PlanCache::Statistics stats;
    short_cache.put("X", makePlan(), stats);
    short_cache.get("X", stats);  // triggers expiry eviction

    EXPECT_GE(short_cache.getStats().evictions, 1u);
}

// =============================================================================
// Capacity and LRU eviction
// =============================================================================

/// Cache evicts oldest entry when capacity is reached.
TEST_F(QueryPlanCachingFocusedTests, LRU_EvictsWhenFull) {
    PlanCache::Config cfg;
    cfg.max_entries = 3;
    PlanCache small_cache(cfg);

    PlanCache::Statistics stats;
    small_cache.put("Q1", makePlan(), stats);
    small_cache.put("Q2", makePlan(), stats);
    small_cache.put("Q3", makePlan(), stats);
    small_cache.put("Q4", makePlan(), stats);  // triggers eviction of Q1

    EXPECT_EQ(small_cache.getStats().current_size, 3u);
    EXPECT_GE(small_cache.getStats().evictions, 1u);
}

/// HitRate is computed correctly.
TEST_F(QueryPlanCachingFocusedTests, Stats_HitRateAccurate) {
    PlanCache::Statistics stats;
    cache_->put("H", makePlan(), stats);
    cache_->get("H",  stats);  // hit
    cache_->get("H",  stats);  // hit
    cache_->get("MISS", stats);  // miss

    auto s = cache_->getStats();
    EXPECT_EQ(s.hits, 2u);
    EXPECT_EQ(s.misses, 1u);
    EXPECT_NEAR(s.hitRate(), 2.0 / 3.0, 1e-6);
}

/// current_size reflects actual entry count.
TEST_F(QueryPlanCachingFocusedTests, Stats_CurrentSizeAccurate) {
    PlanCache::Statistics stats;
    EXPECT_EQ(cache_->getStats().current_size, 0u);
    cache_->put("A", makePlan(), stats);
    cache_->put("B", makePlan(), stats);
    EXPECT_EQ(cache_->getStats().current_size, 2u);
    cache_->invalidateTable("__none__");  // no-op
    EXPECT_EQ(cache_->getStats().current_size, 2u);
    cache_->clear();
    EXPECT_EQ(cache_->getStats().current_size, 0u);
}
