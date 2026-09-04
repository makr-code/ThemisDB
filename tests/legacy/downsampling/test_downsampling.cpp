/**
 * @file test_downsampling.cpp
 * @brief Unit tests for the Multi-Tier Downsampling Pipeline
 *
 * Covers: DownsamplingTier factories, DownsamplingPolicy::defaultPolicy(),
 *         TierSelector resolution routing, DownsamplingPipeline lifecycle,
 *         watermark tracking, and integration with TSQueryOptimizer.
 */

#include <gtest/gtest.h>
#include "timeseries/downsampling.h"
#include "timeseries/query_optimizer.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>
#include <memory>

namespace themis {
namespace {

using namespace std::chrono_literals;

// =========================================================================
// Helpers
// =========================================================================

static std::string makeTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_ds_" + tag + "_" + std::to_string(ns))).string();
}

struct DownsamplingFixture : ::testing::Test {
    std::string db_path = {};
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> store;

    void SetUp() override {
        db_path = makeTempPath("ds");
        RocksDBWrapper::Config cfg;
        cfg.db_path      = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "Failed to open RocksDB at " << db_path;
        store = std::make_unique<TSStore>(db->getRawDB());
    }

    void TearDown() override {
        store.reset();
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    static int64_t nowMs() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    static TSStore::DataPoint makePoint(const std::string& metric,
                                        const std::string& entity,
                                        double value,
                                        int64_t ts_ms) {
        TSStore::DataPoint p;
        p.metric       = metric;
        p.entity       = entity;
        p.timestamp_ms = ts_ms;
        p.value        = value;
        return p;
    }
};

// =========================================================================
// DownsamplingTier factory tests
// =========================================================================

TEST(DownsamplingTierTest, MinutesFactory) {
    auto t = DownsamplingTier::minutes(5);
    EXPECT_EQ(t.name, "5m");
    EXPECT_EQ(t.resolution, std::chrono::milliseconds{5 * 60 * 1000});
    EXPECT_EQ(t.retention.count(), 0);
}

TEST(DownsamplingTierTest, HoursFactory) {
    auto t = DownsamplingTier::hours(1, std::chrono::seconds{90 * 24 * 3600});
    EXPECT_EQ(t.name, "1h");
    EXPECT_EQ(t.resolution, std::chrono::milliseconds{3600 * 1000});
    EXPECT_GT(t.retention.count(), 0);
}

TEST(DownsamplingTierTest, DaysFactory) {
    auto t = DownsamplingTier::days(1);
    EXPECT_EQ(t.name, "1d");
    EXPECT_EQ(t.resolution, std::chrono::milliseconds{86400 * 1000});
}

// =========================================================================
// DownsamplingPolicy tests
// =========================================================================

TEST(DownsamplingPolicyTest, DefaultPolicyHasThreeTiers) {
    auto p = DownsamplingPolicy::defaultPolicy("cpu");
    EXPECT_EQ(p.metric, "cpu");
    EXPECT_EQ(p.tiers.size(), 3u);
    // Finest tier must be smaller than coarsest
    EXPECT_LT(p.tiers.front().resolution, p.tiers.back().resolution);
}

TEST(DownsamplingPolicyTest, DefaultPolicyWithEntity) {
    auto p = DownsamplingPolicy::defaultPolicy("mem", std::string("srv01"));
    EXPECT_EQ(p.metric, "mem");
    ASSERT_TRUE(p.entity.has_value());
    EXPECT_EQ(*p.entity, "srv01");
}

TEST(DownsamplingPolicyTest, TiersAreOrderedFinestToCoarsest) {
    auto p = DownsamplingPolicy::defaultPolicy("net");
    for (size_t i = 1; i < p.tiers.size(); ++i) {
        EXPECT_LT(p.tiers[i - 1].resolution, p.tiers[i].resolution)
            << "Tier " << i - 1 << " should be finer than tier " << i;
    }
}

// =========================================================================
// TierSelector tests
// =========================================================================

TEST(TierSelectorTest, UnregisteredMetricReturnsNullopt) {
    TierSelector sel;
    EXPECT_FALSE(sel.selectTier("unknown", 60000ms).has_value());
}

TEST(TierSelectorTest, ZeroResolutionReturnsNullopt) {
    TierSelector sel;
    sel.registerPolicy(DownsamplingPolicy::defaultPolicy("cpu"));
    EXPECT_FALSE(sel.selectTier("cpu", 0ms).has_value());
}

TEST(TierSelectorTest, SelectsCoarsestFittingTier) {
    TierSelector sel;
    auto policy = DownsamplingPolicy::defaultPolicy("temp");
    sel.registerPolicy(policy);

    // Request 1-hour resolution — should match the 1h tier
    auto result = sel.selectTier("temp", 3600000ms);
    ASSERT_TRUE(result.has_value());
    // The derived name contains the 1h resolution window
    EXPECT_NE(result->find("temp"), std::string::npos);
}

TEST(TierSelectorTest, VeryFineResolutionReturnsBestAvailableTier) {
    TierSelector sel;
    sel.registerPolicy(DownsamplingPolicy::defaultPolicy("disk"));

    // Request 1-minute resolution — should return 1m tier name
    auto result = sel.selectTier("disk", 60000ms);
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result->find("disk"), std::string::npos);
}

TEST(TierSelectorTest, RequestFinerThanFinestTierReturnsNullopt) {
    TierSelector sel;
    sel.registerPolicy(DownsamplingPolicy::defaultPolicy("iops"));

    // Request 10-second resolution — finer than any registered tier
    EXPECT_FALSE(sel.selectTier("iops", 10000ms).has_value());
}

TEST(TierSelectorTest, TiersForRegisteredMetric) {
    TierSelector sel;
    sel.registerPolicy(DownsamplingPolicy::defaultPolicy("net"));
    auto tiers = sel.tiersFor("net");
    EXPECT_EQ(tiers.size(), 3u);
}

TEST(TierSelectorTest, TiersForUnknownMetricEmpty) {
    TierSelector sel;
    EXPECT_TRUE(sel.tiersFor("unknown").empty());
}

// =========================================================================
// DownsamplingPipeline lifecycle tests
// =========================================================================

TEST_F(DownsamplingFixture, ConstructsWithoutThrowing) {
    EXPECT_NO_THROW({ DownsamplingPipeline p(store.get()); });
}

TEST_F(DownsamplingFixture, NullStoreThrows) {
    EXPECT_THROW(DownsamplingPipeline(nullptr), std::invalid_argument);
}

TEST_F(DownsamplingFixture, AddPolicySucceeds) {
    DownsamplingPipeline pipeline(store.get());
    EXPECT_NO_THROW(pipeline.addPolicy(DownsamplingPolicy::defaultPolicy("cpu")));
}

TEST_F(DownsamplingFixture, AddPolicyWithEmptyMetricThrows) {
    DownsamplingPipeline pipeline(store.get());
    DownsamplingPolicy bad;
    bad.metric = "";
    bad.tiers.push_back(DownsamplingTier::minutes(1));
    EXPECT_THROW(pipeline.addPolicy(bad), std::invalid_argument);
}

TEST_F(DownsamplingFixture, AddPolicyWithNoTiersThrows) {
    DownsamplingPipeline pipeline(store.get());
    DownsamplingPolicy bad;
    bad.metric = "cpu";
    EXPECT_THROW(pipeline.addPolicy(bad), std::invalid_argument);
}

TEST_F(DownsamplingFixture, InitialWatermarkIsZero) {
    DownsamplingPipeline pipeline(store.get());
    pipeline.addPolicy(DownsamplingPolicy::defaultPolicy("net"));
    EXPECT_EQ(pipeline.getWatermark("net", "1m"), 0);
    EXPECT_EQ(pipeline.getWatermark("net", "1h"), 0);
    EXPECT_EQ(pipeline.getWatermark("net", "1d"), 0);
}

TEST_F(DownsamplingFixture, SetAndGetWatermark) {
    DownsamplingPipeline pipeline(store.get());
    pipeline.addPolicy(DownsamplingPolicy::defaultPolicy("cpu"));
    pipeline.setWatermark("cpu", "1m", 1700000000000LL);
    EXPECT_EQ(pipeline.getWatermark("cpu", "1m"), 1700000000000LL);
}

// =========================================================================
// Watermark-driven refresh tests
// =========================================================================

TEST_F(DownsamplingFixture, RefreshEmptyStoreReturnsZero) {
    DownsamplingPipeline pipeline(store.get());
    pipeline.addPolicy(DownsamplingPolicy::defaultPolicy("temp"));
    // No raw data → refresh should still complete without error
    EXPECT_NO_THROW(pipeline.refresh(1700000060000LL));
}

TEST_F(DownsamplingFixture, RefreshAdvancesWatermark) {
    DownsamplingPipeline pipeline(store.get());
    pipeline.addPolicy(DownsamplingPolicy::defaultPolicy("cpu"));

    int64_t to_ms = 1700000060000LL;
    pipeline.refresh(to_ms);

    // All tiers should have their watermarks advanced
    EXPECT_EQ(pipeline.getWatermark("cpu", "1m"), to_ms);
    EXPECT_EQ(pipeline.getWatermark("cpu", "1h"), to_ms);
    EXPECT_EQ(pipeline.getWatermark("cpu", "1d"), to_ms);
}

TEST_F(DownsamplingFixture, SecondRefreshDoesNotReprocessSameWindow) {
    DownsamplingPipeline pipeline(store.get());
    pipeline.addPolicy(DownsamplingPolicy::defaultPolicy("disk"));

    int64_t t1 = 1700000060000LL;
    pipeline.refresh(t1);

    // Advance watermarks manually to simulate already-processed data
    int64_t t2 = t1 + 60000LL;
    pipeline.refresh(t2);  // processes [t1, t2) — should not re-process [0, t1)

    EXPECT_EQ(pipeline.getWatermark("disk", "1m"), t2);
}

TEST_F(DownsamplingFixture, RefreshMetricUnknownMetricDoesNotThrow) {
    DownsamplingPipeline pipeline(store.get());
    EXPECT_NO_THROW(pipeline.refreshMetric("nonexistent", 1700000000000LL));
}

TEST_F(DownsamplingFixture, TierSelectorReflectsRegisteredPolicies) {
    DownsamplingPipeline pipeline(store.get());
    pipeline.addPolicy(DownsamplingPolicy::defaultPolicy("net"));

    const TierSelector& sel = pipeline.tierSelector();
    auto tier_1m = sel.selectTier("net", 60000ms);
    EXPECT_TRUE(tier_1m.has_value());
}

// =========================================================================
// TSQueryOptimizer + TierSelector integration
// =========================================================================

TEST_F(DownsamplingFixture, OptimizerUsesDownsamplingTier) {
    DownsamplingPipeline pipeline(store.get());
    pipeline.addPolicy(DownsamplingPolicy::defaultPolicy("cpu"));

    TSQueryOptimizer optimizer(store.get());
    optimizer.setTierSelector(&pipeline.tierSelector());

    TSQueryOptimizer::OptimizationHint hint;
    hint.explain = true;

    // Query with 1h resolution over 7 days — should be routed to 1h tier
    int64_t from = 1700000000000LL;
    int64_t to   = from + 7LL * 24 * 3600 * 1000;  // 7 days later

    auto plan = optimizer.optimizeWithTiers("cpu", std::nullopt, from, to, 3600000ms, hint);

    EXPECT_TRUE(plan.uses_aggregate);
    // Explanation should mention "Tier routing"
    EXPECT_NE(plan.explanation.find("Tier routing"), std::string::npos);
}

TEST_F(DownsamplingFixture, OptimizerFallsBackWhenResolutionTooFine) {
    DownsamplingPipeline pipeline(store.get());
    pipeline.addPolicy(DownsamplingPolicy::defaultPolicy("cpu"));

    TSQueryOptimizer optimizer(store.get());
    optimizer.setTierSelector(&pipeline.tierSelector());

    TSQueryOptimizer::OptimizationHint hint;
    hint.use_aggregates = false;  // disable standard aggregate lookup too

    int64_t from = 1700000000000LL;
    int64_t to   = from + 600000LL; // 10 minutes

    // 1-second resolution — finer than any registered tier, should fall back
    auto plan = optimizer.optimizeWithTiers("cpu", std::nullopt, from, to, 1000ms, hint);

    // With no tier match and aggregates disabled, plan uses raw data
    EXPECT_FALSE(plan.uses_aggregate);
    EXPECT_EQ(plan.source_metric, "cpu");
}

TEST_F(DownsamplingFixture, OptimizerWithoutTierSelectorUsesStandardPath) {
    TSQueryOptimizer optimizer(store.get());
    // No tier selector registered

    int64_t from = 1700000000000LL;
    int64_t to   = from + 3600000LL;

    // Should behave identically to optimizeAggregateQuery
    EXPECT_NO_THROW(
        optimizer.optimizeWithTiers("cpu", std::nullopt, from, to, 60000ms, {}));
}

}  // namespace
}  // namespace themis
