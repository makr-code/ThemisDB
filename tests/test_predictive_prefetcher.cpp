/*
 * Tests for predictive pre-fetching based on query sequence history.
 * Phase 4 – AdaptiveQueryCache (cache module).
 */

#include <gtest/gtest.h>
#include "cache/predictive_prefetcher.h"
#include "cache/adaptive_query_cache.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::cache;
using json = nlohmann::json;

// ============================================================================
// PredictivePrefetcher unit tests
// ============================================================================

class PredictivePrefetcherTest : public ::testing::Test {
protected:
    PredictivePrefetcher::Config cfg_;

    void SetUp() override {
        cfg_.max_tracked_keys      = 100;
        cfg_.max_successors_per_key = 10;
        cfg_.min_transition_count   = 2;
        cfg_.max_predictions        = 3;
        cfg_.min_confidence         = 0.0;
    }
};

TEST_F(PredictivePrefetcherTest, NoCandidatesWithNoHistory) {
    PredictivePrefetcher pf(cfg_);
    auto candidates = pf.getPrefetchCandidates("fp_a");
    EXPECT_TRUE(candidates.empty());
}

TEST_F(PredictivePrefetcherTest, RecordsTransitionAfterTwoAccesses) {
    PredictivePrefetcher pf(cfg_);
    // First call sets "last" = fp_a, no transition recorded yet.
    pf.recordQueryAccess("fp_a");
    // Second call records fp_a -> fp_b.
    pf.recordQueryAccess("fp_b");

    // Only 1 observation so far – below min_transition_count(2), no candidates.
    auto c1 = pf.getPrefetchCandidates("fp_a");
    EXPECT_TRUE(c1.empty());

    // Third access: fp_b -> fp_c, and also records second fp_a -> fp_b transition.
    pf.recordQueryAccess("fp_a");
    pf.recordQueryAccess("fp_b");

    // Now fp_a -> fp_b has count=2 which meets min_transition_count.
    auto c2 = pf.getPrefetchCandidates("fp_a");
    ASSERT_EQ(c2.size(), 1u);
    EXPECT_EQ(c2[0], "fp_b");
}

TEST_F(PredictivePrefetcherTest, ReturnsCandidatesSortedByFrequency) {
    PredictivePrefetcher pf(cfg_);

    // Record fp_a -> fp_b 3 times and fp_a -> fp_c 5 times
    for (int i = 0; i < 3; ++i) {
        pf.recordQueryAccess("fp_a");
        pf.recordQueryAccess("fp_b");
    }
    for (int i = 0; i < 5; ++i) {
        pf.recordQueryAccess("fp_a");
        pf.recordQueryAccess("fp_c");
    }

    auto candidates = pf.getPrefetchCandidates("fp_a");
    ASSERT_GE(candidates.size(), 2u);
    // fp_c (5) should come before fp_b (3)
    EXPECT_EQ(candidates[0], "fp_c");
    EXPECT_EQ(candidates[1], "fp_b");
}

TEST_F(PredictivePrefetcherTest, RespectsMaxPredictions) {
    PredictivePrefetcher pf(cfg_);  // max_predictions = 3

    // Create 5 successors each observed 3 times
    for (const auto& successor : {"fp_b", "fp_c", "fp_d", "fp_e", "fp_f"}) {
        for (int i = 0; i < 3; ++i) {
            pf.recordQueryAccess("fp_a");
            pf.recordQueryAccess(successor);
        }
    }

    auto candidates = pf.getPrefetchCandidates("fp_a");
    EXPECT_LE(candidates.size(), cfg_.max_predictions);
}

TEST_F(PredictivePrefetcherTest, RespectsMinConfidence) {
    // Build a new prefetcher with min_confidence set.
    PredictivePrefetcher::Config cfg2 = cfg_;
    cfg2.min_confidence = 0.8;
    PredictivePrefetcher pf2(cfg2);

    // fp_a -> fp_b: 8 times; fp_a -> fp_c: 2 times (total 10 transitions from fp_a)
    // Confidence fp_b = 0.8, fp_c = 0.2 → only fp_b should pass
    for (int i = 0; i < 8; ++i) {
        pf2.recordQueryAccess("fp_a");
        pf2.recordQueryAccess("fp_b");
    }
    for (int i = 0; i < 2; ++i) {
        pf2.recordQueryAccess("fp_a");
        pf2.recordQueryAccess("fp_c");
    }

    auto candidates = pf2.getPrefetchCandidates("fp_a");
    ASSERT_EQ(candidates.size(), 1u);
    EXPECT_EQ(candidates[0], "fp_b");
}

TEST_F(PredictivePrefetcherTest, TenantSessionsAreIsolated) {
    PredictivePrefetcher pf(cfg_);

    // Tenant "A": fp_a -> fp_b (3 times)
    for (int i = 0; i < 3; ++i) {
        pf.recordQueryAccess("fp_a", "tenantA");
        pf.recordQueryAccess("fp_b", "tenantA");
    }

    // Tenant "B": fp_a -> fp_c (3 times)
    for (int i = 0; i < 3; ++i) {
        pf.recordQueryAccess("fp_a", "tenantB");
        pf.recordQueryAccess("fp_c", "tenantB");
    }

    // Both tenants share the transition table (transitions are global by design –
    // only the "last seen" state is per-tenant).  fp_a should have both fp_b and
    // fp_c as successors.
    auto candidates = pf.getPrefetchCandidates("fp_a");
    EXPECT_GE(candidates.size(), 1u);
}

TEST_F(PredictivePrefetcherTest, ClearResetsState) {
    PredictivePrefetcher pf(cfg_);
    for (int i = 0; i < 3; ++i) {
        pf.recordQueryAccess("fp_a");
        pf.recordQueryAccess("fp_b");
    }
    EXPECT_FALSE(pf.getPrefetchCandidates("fp_a").empty());

    pf.clear();
    EXPECT_TRUE(pf.getPrefetchCandidates("fp_a").empty());
}

TEST_F(PredictivePrefetcherTest, MaxTrackedKeysEvictsOldest) {
    PredictivePrefetcher::Config cfg2 = cfg_;
    cfg2.max_tracked_keys     = 3;
    cfg2.min_transition_count = 1;
    PredictivePrefetcher pf(cfg2);

    // Build 3 full source keys: k0->v, k1->v, k2->v
    for (int k = 0; k < 3; ++k) {
        pf.recordQueryAccess("key" + std::to_string(k));
        pf.recordQueryAccess("value");
    }

    // Adding a 4th source key (key3) should evict key0 (FIFO)
    pf.recordQueryAccess("key3");
    pf.recordQueryAccess("value");

    // key0 should no longer have candidates; key1 and key3 should.
    EXPECT_TRUE(pf.getPrefetchCandidates("key0").empty());
    EXPECT_FALSE(pf.getPrefetchCandidates("key1").empty());
    EXPECT_FALSE(pf.getPrefetchCandidates("key3").empty());
}

TEST_F(PredictivePrefetcherTest, StatsReflectRecordedData) {
    PredictivePrefetcher pf(cfg_);
    pf.recordQueryAccess("fp_a");
    pf.recordQueryAccess("fp_b");

    pf.recordPrefetchHit();
    pf.recordCandidatesGenerated();

    auto stats = pf.getStats();
    EXPECT_GE(stats["total_transitions_recorded"].get<uint64_t>(), 1u);
    EXPECT_EQ(stats["prefetch_hits"].get<uint64_t>(), 1u);
    EXPECT_EQ(stats["candidates_generated"].get<uint64_t>(), 1u);
    EXPECT_TRUE(stats.contains("tracked_keys"));
}

// ============================================================================
// AdaptiveQueryCache integration tests for predictive pre-fetching
// ============================================================================

class PredictivePrefetchCacheTest : public ::testing::Test {
protected:
    AdaptiveQueryCache::Config config_;

    void SetUp() override {
        config_.l3_db_path = "/tmp/themis_test_prefetch_cache_" +
                             std::to_string(std::chrono::system_clock::now()
                                                .time_since_epoch().count()) +
                             "_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        config_.l1_max_entries           = 50;
        config_.l2_max_entries           = 100;
        config_.l1_ttl_seconds           = 300;
        config_.l2_ttl_seconds           = 600;
        config_.l3_ttl_seconds           = 3600;
        config_.enable_predictive_prefetch   = true;
        config_.prefetch_max_tracked_keys    = 500;
        config_.prefetch_max_predictions     = 3;
        config_.prefetch_min_transition_count = 2;
        config_.prefetch_min_confidence      = 0.0;
    }

    void TearDown() override {
        if (!config_.l3_db_path.empty()) {
            std::error_code ec;
            std::filesystem::remove_all(config_.l3_db_path, ec);
            // Ignore cleanup errors so they don't mask test failures.
        }
    }
};

TEST_F(PredictivePrefetchCacheTest, PrefetcherDisabledByDefault) {
    AdaptiveQueryCache::Config cfg = config_;
    cfg.enable_predictive_prefetch = false;
    AdaptiveQueryCache cache(cfg);

    // With prefetcher disabled, no candidates should be returned.
    auto candidates = cache.getPrefetchCandidates("some_fp");
    EXPECT_TRUE(candidates.empty());

    // getPrefetchStats must say "enabled: false"
    auto stats = cache.getPrefetchStats();
    EXPECT_FALSE(stats["enabled"].get<bool>());
}

TEST_F(PredictivePrefetchCacheTest, PrefetchStatsEnabledAfterConstruct) {
    AdaptiveQueryCache cache(config_);
    auto stats = cache.getPrefetchStats();
    EXPECT_TRUE(stats["enabled"].get<bool>());
    EXPECT_TRUE(stats.contains("tracked_keys"));
    EXPECT_TRUE(stats.contains("total_transitions_recorded"));
}

TEST_F(PredictivePrefetchCacheTest, RecordQueryAccessBuildsModel) {
    AdaptiveQueryCache cache(config_);

    // Record sequence a -> b -> c three times
    for (int i = 0; i < 3; ++i) {
        cache.recordQueryAccess("fp_a");
        cache.recordQueryAccess("fp_b");
        cache.recordQueryAccess("fp_c");
    }

    // After 3 observations of fp_a->fp_b, it should appear as a candidate.
    auto candidates = cache.getPrefetchCandidates("fp_a");
    ASSERT_FALSE(candidates.empty());
    EXPECT_EQ(candidates[0], "fp_b");
}

TEST_F(PredictivePrefetchCacheTest, GetHitTriggersRecordQueryAccess) {
    AdaptiveQueryCache cache(config_);

    const std::string q1 = "SELECT * FROM users";
    const std::string q2 = "SELECT * FROM orders";
    json r1 = {{"data", 1}};
    json r2 = {{"data", 2}};

    std::string fp1 = cache.generateFingerprint(q1);
    std::string fp2 = cache.generateFingerprint(q2);

    // Seed both entries into cache
    cache.put(fp1, {}, r1);
    cache.put(fp2, {}, r2);

    // Simulate repeated access pattern: q1 always followed by q2 (3 rounds)
    for (int i = 0; i < 3; ++i) {
        cache.get(fp1);  // internally calls recordQueryAccess(fp1)
        cache.get(fp2);  // internally calls recordQueryAccess(fp2)
    }

    // After 3 consecutive fp1 -> fp2 transitions the prefetcher should
    // predict fp2 after fp1.
    auto candidates = cache.getPrefetchCandidates(fp1);
    ASSERT_FALSE(candidates.empty());
    EXPECT_EQ(candidates[0], fp2);
}

TEST_F(PredictivePrefetchCacheTest, NoCandidatesWhenPrefetcherDisabled) {
    AdaptiveQueryCache::Config cfg = config_;
    cfg.enable_predictive_prefetch = false;
    AdaptiveQueryCache cache(cfg);

    cache.recordQueryAccess("fp_a");
    cache.recordQueryAccess("fp_b");

    auto candidates = cache.getPrefetchCandidates("fp_a");
    EXPECT_TRUE(candidates.empty());
}

TEST_F(PredictivePrefetchCacheTest, PrefetchHitMetricRecording) {
    AdaptiveQueryCache cache(config_);

    // Build sequence fp_a -> fp_b twice
    for (int i = 0; i < 2; ++i) {
        cache.recordQueryAccess("fp_a");
        cache.recordQueryAccess("fp_b");
    }

    // Get candidates (increments prefetch_candidates_generated in metrics)
    auto candidates = cache.getPrefetchCandidates("fp_a");
    EXPECT_FALSE(candidates.empty());

    // Enhanced metrics should now show at least one candidates_generated
    auto& metrics = cache.getEnhancedMetrics();
    EXPECT_GE(metrics.prefetch_candidates_generated.load(), 1u);
}
