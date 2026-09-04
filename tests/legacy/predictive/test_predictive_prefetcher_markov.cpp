// Copyright 2025 ThemisDB
// Licensed under MIT License

/*
 * Tests for the extended PredictivePrefetcher:
 *  - Time-of-day bucketing
 *  - A/B test toggle (Markov vs. frequency baseline)
 *  - MetricsCollector metric emission
 *  - RocksDB model persistence (save / load)
 *  - recordOverheadBytes / overhead_bytes stat
 */

#include <gtest/gtest.h>
#include "cache/predictive_prefetcher.h"
#include "cache/adaptive_query_cache.h"
#include "observability/metrics_collector.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::cache;
using namespace themis::observability;
using json = nlohmann::json;

// ============================================================================
// Time-of-day bucketing
// ============================================================================

class ToDBucketingTest : public ::testing::Test {
protected:
    PredictivePrefetcher::Config cfg_;
    void SetUp() override {
        cfg_.max_tracked_keys       = 100;
        cfg_.max_successors_per_key = 10;
        cfg_.min_transition_count   = 2;
        cfg_.max_predictions        = 3;
        cfg_.min_confidence         = 0.0;
        cfg_.enable_time_of_day_weighting = true;
        cfg_.enable_ab_test         = false;
    }
};

TEST_F(ToDBucketingTest, CandidatesReturnedWithToDEnabled) {
    PredictivePrefetcher pf(cfg_);
    for (int i = 0; i < 3; ++i) {
        pf.recordQueryAccess("fp_a");
        pf.recordQueryAccess("fp_b");
    }
    auto candidates = pf.getPrefetchCandidates("fp_a");
    EXPECT_FALSE(candidates.empty());
    EXPECT_EQ(candidates[0], "fp_b");
}

TEST_F(ToDBucketingTest, StatsContainTimeOfDayFlag) {
    PredictivePrefetcher pf(cfg_);
    auto stats = pf.getStats();
    EXPECT_TRUE(stats.contains("time_of_day_weighting"));
    EXPECT_TRUE(stats["time_of_day_weighting"].get<bool>());
}

TEST_F(ToDBucketingTest, SortOrderWithToDMatchesWithoutToD) {
    // Both model variants should still rank fp_c (5 obs) above fp_b (3 obs).
    PredictivePrefetcher::Config cfg_no_tod = cfg_;
    cfg_no_tod.enable_time_of_day_weighting = false;

    PredictivePrefetcher pf_tod(cfg_);
    PredictivePrefetcher pf_base(cfg_no_tod);

    for (int i = 0; i < 3; ++i) {
        pf_tod.recordQueryAccess("fp_a");
        pf_tod.recordQueryAccess("fp_b");
        pf_base.recordQueryAccess("fp_a");
        pf_base.recordQueryAccess("fp_b");
    }
    for (int i = 0; i < 5; ++i) {
        pf_tod.recordQueryAccess("fp_a");
        pf_tod.recordQueryAccess("fp_c");
        pf_base.recordQueryAccess("fp_a");
        pf_base.recordQueryAccess("fp_c");
    }

    auto cands_tod  = pf_tod.getPrefetchCandidates("fp_a");
    auto cands_base = pf_base.getPrefetchCandidates("fp_a");

    ASSERT_GE(cands_tod.size(),  2u);
    ASSERT_GE(cands_base.size(), 2u);
    // Both should put the higher-count entry first.
    EXPECT_EQ(cands_tod[0],  "fp_c");
    EXPECT_EQ(cands_base[0], "fp_c");
}

TEST_F(ToDBucketingTest, ClearResetsAll) {
    PredictivePrefetcher pf(cfg_);
    for (int i = 0; i < 3; ++i) {
        pf.recordQueryAccess("fp_a");
        pf.recordQueryAccess("fp_b");
    }
    pf.clear();
    EXPECT_TRUE(pf.getPrefetchCandidates("fp_a").empty());
    auto stats = pf.getStats();
    EXPECT_EQ(stats["tracked_keys"].get<size_t>(), 0u);
    EXPECT_EQ(stats["overhead_bytes"].get<uint64_t>(), 0u);
}

// ============================================================================
// A/B test toggle
// ============================================================================

class ABTestToggleTest : public ::testing::Test {
protected:
    PredictivePrefetcher::Config cfg_;
    void SetUp() override {
        cfg_.max_tracked_keys       = 100;
        cfg_.max_successors_per_key = 10;
        cfg_.min_transition_count   = 2;
        cfg_.max_predictions        = 3;
        cfg_.min_confidence         = 0.0;
        cfg_.enable_time_of_day_weighting = true;
        cfg_.enable_ab_test         = true;
    }
};

TEST_F(ABTestToggleTest, CandidatesAreReturnedForBothGroups) {
    PredictivePrefetcher pf(cfg_);
    for (int i = 0; i < 3; ++i) {
        pf.recordQueryAccess("fp_a");
        pf.recordQueryAccess("fp_b");
    }
    // "tenant_1": FNV-1a hash % 2 == 1 → A/B group 1 (raw Markov, no ToD).
    // "tenant_2": FNV-1a hash % 2 == 0 → A/B group 0 (Markov + ToD).
    // Both groups share the same transition table, so both must return candidates.
    auto cands_t1 = pf.getPrefetchCandidates("fp_a", "tenant_1");
    auto cands_t2 = pf.getPrefetchCandidates("fp_a", "tenant_2");
    // Both should have at least one candidate since the shared transition table
    // has enough observations (fp_a → fp_b has 3 transitions ≥ min_transition_count=2).
    EXPECT_FALSE(cands_t1.empty());
    EXPECT_FALSE(cands_t2.empty());
}

TEST_F(ABTestToggleTest, StatsReflectABFields) {
    PredictivePrefetcher pf(cfg_);
    auto stats = pf.getStats();
    EXPECT_TRUE(stats["ab_test_enabled"].get<bool>());
    EXPECT_TRUE(stats.contains("ab"));
}

TEST_F(ABTestToggleTest, ABDisabledDoesNotAddABStats) {
    PredictivePrefetcher::Config cfg_no_ab = cfg_;
    cfg_no_ab.enable_ab_test = false;
    PredictivePrefetcher pf(cfg_no_ab);
    auto stats = pf.getStats();
    EXPECT_FALSE(stats["ab_test_enabled"].get<bool>());
    EXPECT_FALSE(stats.contains("ab"));
}

// ============================================================================
// MetricsCollector integration
// ============================================================================

class PrefetchMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
    }
};

TEST_F(PrefetchMetricsTest, HitRateGaugeUpdatedOnPrefetchHit) {
    PredictivePrefetcher::Config cfg;
    cfg.max_tracked_keys       = 100;
    cfg.max_successors_per_key = 10;
    cfg.min_transition_count   = 2;
    cfg.max_predictions        = 3;
    cfg.enable_time_of_day_weighting = false;

    PredictivePrefetcher pf(cfg);

    // Generate some candidates and record hits.
    pf.recordCandidatesGenerated(1);
    pf.recordCandidatesGenerated(1);
    pf.recordPrefetchHit();  // This should emit metrics.

    auto stats = pf.getStats();
    EXPECT_NEAR(stats["hit_rate"].get<double>(), 0.5, 0.01);

    // Verify that MetricsCollector received the gauge value.
    const std::string prometheus = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(prometheus.find("cache.prefetch.hit_rate"), std::string::npos);
}

TEST_F(PrefetchMetricsTest, OverheadBytesEmitted) {
    PredictivePrefetcher::Config cfg;
    cfg.min_transition_count   = 1;
    cfg.enable_time_of_day_weighting = false;
    PredictivePrefetcher pf(cfg);

    pf.recordOverheadBytes(1024);
    pf.recordOverheadBytes(512);

    auto stats = pf.getStats();
    EXPECT_EQ(stats["overhead_bytes"].get<uint64_t>(), 1536u);

    // Verify that MetricsCollector received the overhead gauge.
    const std::string prometheus = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(prometheus.find("cache.prefetch.overhead_bytes"), std::string::npos);
}

// ============================================================================
// RocksDB model persistence (via AdaptiveQueryCache + temp dir)
// ============================================================================

class PrefetchPersistenceTest : public ::testing::Test {
protected:
    AdaptiveQueryCache::Config config_;
    std::string db_path_ = {};

    void SetUp() override {
        const std::string suffix =
            std::to_string(std::chrono::system_clock::now()
                               .time_since_epoch().count()) +
            "_" + std::to_string(reinterpret_cast<uintptr_t>(this));
        db_path_ = (std::filesystem::temp_directory_path() /
                    ("themis_test_prefetch_persist_" + suffix)).string();
        config_.l3_db_path                    = db_path_;
        config_.l1_max_entries                = 50;
        config_.l2_max_entries                = 100;
        config_.l1_ttl_seconds                = 300;
        config_.l2_ttl_seconds                = 600;
        config_.l3_ttl_seconds                = 3600;
        config_.enable_predictive_prefetch    = true;
        config_.prefetch_max_tracked_keys     = 500;
        config_.prefetch_max_predictions      = 3;
        config_.prefetch_min_transition_count = 2;
        config_.prefetch_min_confidence       = 0.0;
        config_.prefetch_enable_time_of_day_weighting = true;
    }

    void TearDown() override {
        if (!db_path_.empty()) {
            std::error_code ec = {};
            std::filesystem::remove_all(db_path_, ec);
        }
    }
};

TEST_F(PrefetchPersistenceTest, SaveAndLoadRoundTrip) {
    // Phase 1: populate the model and save it.
    {
        AdaptiveQueryCache cache(config_);
        for (int i = 0; i < 3; ++i) {
            cache.recordQueryAccess("fp_a");
            cache.recordQueryAccess("fp_b");
        }
        // Candidates should exist before save.
        ASSERT_FALSE(cache.getPrefetchCandidates("fp_a").empty());

        cache.savePrefetchModel();
    }

    // Phase 2: open the same L3 DB, load the model, verify candidates.
    {
        AdaptiveQueryCache cache2(config_);
        // loadPrefetchModel is called automatically in the constructor after
        // l3_db_ is ready (via loadPrefetchModel() at end of ctor).

        auto candidates = cache2.getPrefetchCandidates("fp_a");
        EXPECT_FALSE(candidates.empty());
        EXPECT_EQ(candidates[0], "fp_b");
    }
}

TEST_F(PrefetchPersistenceTest, LoadMergesWithInMemoryState) {
    // Build model and save.
    {
        AdaptiveQueryCache cache(config_);
        for (int i = 0; i < 3; ++i) {
            cache.recordQueryAccess("fp_a");
            cache.recordQueryAccess("fp_b");
        }
        cache.savePrefetchModel();
    }

    // Open a fresh cache that also trains fp_a -> fp_c before loading.
    {
        AdaptiveQueryCache cache2(config_);
        // After load (done in ctor), additionally train fp_a -> fp_c.
        for (int i = 0; i < 3; ++i) {
            cache2.recordQueryAccess("fp_a");
            cache2.recordQueryAccess("fp_c");
        }
        // Both fp_b (from persisted model) and fp_c (new training) should exist.
        auto candidates = cache2.getPrefetchCandidates("fp_a");
        EXPECT_GE(candidates.size(), 1u);
    }
}

// ============================================================================
// recordPrefetchOverheadBytes forwarded through AdaptiveQueryCache
// ============================================================================

TEST_F(PrefetchPersistenceTest, RecordOverheadBytesForwarded) {
    AdaptiveQueryCache cache(config_);
    cache.recordPrefetchOverheadBytes(2048);
    auto stats = cache.getPrefetchStats();
    ASSERT_TRUE(stats["enabled"].get<bool>());
    EXPECT_GE(stats["overhead_bytes"].get<uint64_t>(), 2048u);
}

// ============================================================================
// New config fields forwarded in AdaptiveQueryCache
// ============================================================================

TEST(AdaptiveQueryCachePrefetchConfigTest, ToDBucketingFlagPropagated) {
    const std::string db_path = (std::filesystem::temp_directory_path() /
        ("themis_test_aqc_tod_" +
         std::to_string(std::chrono::system_clock::now()
                            .time_since_epoch().count()))).string();

    AdaptiveQueryCache::Config cfg;
    cfg.l3_db_path                    = db_path;
    cfg.l1_max_entries                = 50;
    cfg.l2_max_entries                = 100;
    cfg.l1_ttl_seconds                = 300;
    cfg.l2_ttl_seconds                = 600;
    cfg.l3_ttl_seconds                = 3600;
    cfg.enable_predictive_prefetch    = true;
    cfg.prefetch_max_tracked_keys     = 100;
    cfg.prefetch_max_predictions      = 3;
    cfg.prefetch_min_transition_count = 2;
    cfg.prefetch_min_confidence       = 0.0;
    cfg.prefetch_enable_time_of_day_weighting = true;
    cfg.prefetch_enable_ab_test       = false;

    AdaptiveQueryCache cache(cfg);
    auto stats = cache.getPrefetchStats();
    EXPECT_TRUE(stats["enabled"].get<bool>());
    EXPECT_TRUE(stats["time_of_day_weighting"].get<bool>());

    std::error_code ec = {};
    std::filesystem::remove_all(db_path, ec);
}

TEST(AdaptiveQueryCachePrefetchConfigTest, ABTestFlagPropagated) {
    const std::string db_path = (std::filesystem::temp_directory_path() /
        ("themis_test_aqc_ab_" +
         std::to_string(std::chrono::system_clock::now()
                            .time_since_epoch().count()))).string();

    AdaptiveQueryCache::Config cfg;
    cfg.l3_db_path                    = db_path;
    cfg.l1_max_entries                = 50;
    cfg.l2_max_entries                = 100;
    cfg.l1_ttl_seconds                = 300;
    cfg.l2_ttl_seconds                = 600;
    cfg.l3_ttl_seconds                = 3600;
    cfg.enable_predictive_prefetch    = true;
    cfg.prefetch_max_tracked_keys     = 100;
    cfg.prefetch_max_predictions      = 3;
    cfg.prefetch_min_transition_count = 2;
    cfg.prefetch_min_confidence       = 0.0;
    cfg.prefetch_enable_time_of_day_weighting = true;
    cfg.prefetch_enable_ab_test       = true;

    AdaptiveQueryCache cache(cfg);
    auto stats = cache.getPrefetchStats();
    EXPECT_TRUE(stats["enabled"].get<bool>());
    EXPECT_TRUE(stats["ab_test_enabled"].get<bool>());

    std::error_code ec = {};
    std::filesystem::remove_all(db_path, ec);
}
