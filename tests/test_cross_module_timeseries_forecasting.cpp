/**
 * @file test_cross_module_timeseries_forecasting.cpp
 * @brief Cross-module integration tests: Timeseries (ContinuousAgg / DistributedAgg)
 *        interacting with Analytics (ForecastModel).
 *
 * Group A – ContinuousAggMaterializationEngine → ForecastModel pipeline
 * -----------------------------------------------------------------------
 * These tests exercise the production data-flow:
 *   1. Raw TSStore data points are written.
 *   2. ContinuousAggMaterializationEngine materialises windowed averages.
 *   3. The materialized TSStore::DataPoint results are converted to an
 *      analytics::TimeSeries.
 *   4. A ForecastModel is fitted and predictions are generated.
 *
 * The tests verify cross-module contract invariants that individual
 * module tests cannot catch:
 *   – timestamp field semantics are compatible (both use int64_t ms-since-epoch)
 *   – empty materialized data propagates correctly to an unfit model
 *   – partial refresh watermark is honoured by the forecasting consumer
 *   – two independent metrics produce independent forecast models
 *   – materialized timestamps are monotonically increasing after conversion
 *
 * Group B – DistributedAgg shard-merge correctness
 * -------------------------------------------------
 * These tests call mergeShardResults() directly (no DB) and verify the
 * mathematical properties of the merge:
 *   – sum and count are additive across shards
 *   – avg() equals the weighted average (sum/count)
 *   – min and max are global extrema
 *   – from_ms / to_ms reflect the union of all shard time ranges
 *   – invalid shards are silently skipped
 *   – all-invalid input produces an invalid result
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "timeseries/continuous_agg.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include "analytics/forecasting.h"

#include <chrono>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>

using namespace themis;
namespace fs = std::filesystem;

// ============================================================================
// Shared DB fixture (reused by Group A tests)
// ============================================================================

static std::string makeUniqueTempPath(const char* prefix) {
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            (std::string(prefix) + std::to_string(ns))).string();
}

struct CrossModuleTsFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore>        store;

    // Base timestamp: 2023-11-15 00:00:00 UTC in ms
    static constexpr int64_t kBaseMs   = 1700000000000LL;
    // 1-minute window used throughout
    static constexpr int64_t kWindowMs = 60000LL;

    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CrossModuleTsFixture on Windows due to intermittent timeout/instability in RocksDB-backed forecasting integration tests.";
#endif
        db_path = makeUniqueTempPath("themis_cross_ts_");
        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "RocksDB open failed at " << db_path;
        store = std::make_unique<TSStore>(db->getRawDB());
    }

    void TearDown() override {
        store.reset();
        db.reset();
        fs::remove_all(db_path);
    }

    // Insert n raw data points for (metric, entity) starting at kBaseMs.
    // Each point is placed at the centre of its kWindowMs window so that it
    // falls within exactly one materialization window.
    void insertPoints(const std::string& metric,
                      const std::string& entity,
                      int n,
                      double start_val = 1.0,
                      double step_val  = 1.0) {
        for (int i = 0; i < n; ++i) {
            TSStore::DataPoint p;
            p.metric       = metric;
            p.entity       = entity;
            p.timestamp_ms = kBaseMs + static_cast<int64_t>(i) * kWindowMs + kWindowMs / 2; // centre-of-window: ensures each point falls within exactly one materialization window
            p.value        = start_val + static_cast<double>(i) * step_val;
            ASSERT_TRUE(store->putDataPoint(p).has_value())
                << "putDataPoint failed for " << metric << " i=" << i;
        }
    }

    // Helper: build a ContinuousAggDefinition with 1-minute windows.
    static ContinuousAggDefinition makeAggDef(const std::string& name,
                                              const std::string& metric,
                                              const std::string& entity) {
        ContinuousAggDefinition def;
        def.name               = name;
        def.config.metric      = metric;
        def.config.entity      = entity;
        def.config.window.size = std::chrono::milliseconds(kWindowMs);
        return def;
    }
};

// ============================================================================
// Group A – ContinuousAggMaterializationEngine → ForecastModel
// ============================================================================

// ---------------------------------------------------------------------------
// A-1: Linear trend → linear regression forecast predicts upward continuation
// ---------------------------------------------------------------------------
TEST_F(CrossModuleTsFixture, ContinuousAggToForecast_LinearTrend_ForecastContinuesTrend) {
    constexpr int kWindows = 10;
    insertPoints("cpu", "srv1", kWindows, /*start=*/10.0, /*step=*/10.0);

    ContinuousAggMaterializationEngine engine(store.get());
    ASSERT_TRUE(engine.createAggregate(makeAggDef("cpu_1m", "cpu", "srv1")));

    int64_t end_ms = kBaseMs + static_cast<int64_t>(kWindows) * kWindowMs;
    engine.refreshAggregate("cpu_1m", end_ms);

    auto pts = engine.queryMaterialized("cpu_1m", kBaseMs, end_ms);
    ASSERT_GE(pts.size(), 2u) << "Need at least 2 materialized points for forecasting";

    // Convert TSStore::DataPoint  →  analytics::TimeSeries
    themisdb::analytics::TimeSeries ts;
    for (const auto& p : pts) {
        ts.push(p.timestamp_ms, p.value);
    }

    themisdb::analytics::ForecastModel model(
        themisdb::analytics::ForecastMethod::LINEAR_REGRESSION);
    ASSERT_NO_THROW(model.fit(ts));
    EXPECT_TRUE(model.isFitted());

    auto preds = model.predict(3);
    ASSERT_EQ(preds.size(), 3u);

    // For a strictly increasing linear series, the first prediction must be
    // greater than the mean of the training data.
    double mean_val = 0.0;
    for (const auto& p : pts) {
      mean_val += p.value;
    }
    mean_val /= static_cast<double>(pts.size());
    EXPECT_GT(preds[0].value, mean_val)
        << "Linear forecast should predict beyond the training mean";
    // The three predicted values must also be monotonically increasing.
    EXPECT_LT(preds[0].value, preds[1].value);
    EXPECT_LT(preds[1].value, preds[2].value);
}

// ---------------------------------------------------------------------------
// A-2: Empty materialized store → ForecastModel::fit() must throw
// ---------------------------------------------------------------------------
TEST_F(CrossModuleTsFixture, ContinuousAggToForecast_EmptyStore_FitThrows) {
    ContinuousAggMaterializationEngine engine(store.get());
    ASSERT_TRUE(engine.createAggregate(makeAggDef("mem_1m", "mem", "srv1")));
    // Intentionally skip refreshAggregate → no materialized data exists.

    auto pts = engine.queryMaterialized("mem_1m", kBaseMs,
                                        kBaseMs + 10 * kWindowMs);
    EXPECT_TRUE(pts.empty())
        << "No refresh performed; queryMaterialized must return empty";

    themisdb::analytics::TimeSeries ts;
    for (const auto& p : pts) {
      ts.push(p.timestamp_ms, p.value);
    }
    EXPECT_TRUE(ts.empty());

    themisdb::analytics::ForecastModel model;
    EXPECT_THROW(model.fit(ts), std::invalid_argument)
        << "fit() on empty TimeSeries must throw std::invalid_argument";
}

// ---------------------------------------------------------------------------
// A-3: Partial refresh – forecast consumer sees only refreshed windows
// ---------------------------------------------------------------------------
TEST_F(CrossModuleTsFixture, ContinuousAggToForecast_PartialRefresh_ForecastBoundedByWatermark) {
    constexpr int kTotal = 20;
    insertPoints("net", "srv1", kTotal, 1.0, 1.0);

    ContinuousAggMaterializationEngine engine(store.get());
    ASSERT_TRUE(engine.createAggregate(makeAggDef("net_1m", "net", "srv1")));

    // Only refresh the FIRST half of the data.
    constexpr int kRefreshed = 10;
    int64_t partial_end_ms = kBaseMs + static_cast<int64_t>(kRefreshed) * kWindowMs;
    engine.refreshAggregate("net_1m", partial_end_ms);

    // Query the full range: materialized data must not exceed the watermark.
    int64_t full_end_ms = kBaseMs + static_cast<int64_t>(kTotal) * kWindowMs;
    auto pts_full = engine.queryMaterialized("net_1m", kBaseMs, full_end_ms);
    EXPECT_LE(pts_full.size(), static_cast<size_t>(kRefreshed))
        << "queryMaterialized must honour the refresh watermark";

    // The second half must be entirely absent from materialized store.
    auto pts_second = engine.queryMaterialized("net_1m",
                                               partial_end_ms + 1,
                                               full_end_ms);
    EXPECT_TRUE(pts_second.empty())
        << "Unrefreshed windows must not appear in queryMaterialized results";
}

// ---------------------------------------------------------------------------
// A-4: Two metrics produce independent ForecastModel instances
// ---------------------------------------------------------------------------
TEST_F(CrossModuleTsFixture, ContinuousAggToForecast_TwoMetrics_ModelsAreIndependent) {
    constexpr int kWindows = 8;
    // Metric 1: strictly ascending (1, 2, … 8)
    insertPoints("m_up",   "h1", kWindows, 1.0,   1.0);
    // Metric 2: strictly descending (80, 70, … 10)
    insertPoints("m_down", "h1", kWindows, 80.0, -10.0);

    ContinuousAggMaterializationEngine engine(store.get());
    ASSERT_TRUE(engine.createAggregate(makeAggDef("up_1m",   "m_up",   "h1")));
    ASSERT_TRUE(engine.createAggregate(makeAggDef("down_1m", "m_down", "h1")));

    int64_t end_ms = kBaseMs + static_cast<int64_t>(kWindows) * kWindowMs;
    engine.refreshAggregate("up_1m",   end_ms);
    engine.refreshAggregate("down_1m", end_ms);

    auto pts1 = engine.queryMaterialized("up_1m",   kBaseMs, end_ms);
    auto pts2 = engine.queryMaterialized("down_1m", kBaseMs, end_ms);
    ASSERT_GE(pts1.size(), 2u);
    ASSERT_GE(pts2.size(), 2u);

    themisdb::analytics::TimeSeries ts1, ts2;
    for (const auto& p : pts1) {
      ts1.push(p.timestamp_ms, p.value);
    }
    for (const auto& p : pts2) {
      ts2.push(p.timestamp_ms, p.value);
    }

    themisdb::analytics::ForecastModel m1(
        themisdb::analytics::ForecastMethod::LINEAR_REGRESSION);
    themisdb::analytics::ForecastModel m2(
        themisdb::analytics::ForecastMethod::LINEAR_REGRESSION);
    m1.fit(ts1);
    m2.fit(ts2);

    auto pred1 = m1.predict(1);
    auto pred2 = m2.predict(1);
    ASSERT_EQ(pred1.size(), 1u);
    ASSERT_EQ(pred2.size(), 1u);

    // Model 1 (ascending): forecast must exceed last training value.
    EXPECT_GT(pred1[0].value, pts1.back().value)
        << "Ascending-trend forecast must exceed last observed value";
    // Model 2 (descending): forecast must be below first training value.
    EXPECT_LT(pred2[0].value, pts2.front().value)
        << "Descending-trend forecast must be below first observed value";
    // The two models must produce genuinely different predictions.
    EXPECT_NE(pred1[0].value, pred2[0].value)
        << "Independent models trained on different series must differ";
}

// ---------------------------------------------------------------------------
// A-5: Materialized timestamps flow into TimeSeries in monotone order
// ---------------------------------------------------------------------------
TEST_F(CrossModuleTsFixture, ContinuousAggToForecast_MaterializedTimestamps_MonotoneInTimeSeries) {
    constexpr int kWindows = 6;
    insertPoints("disk", "srv1", kWindows, 50.0, 0.0); // constant value

    ContinuousAggMaterializationEngine engine(store.get());
    ASSERT_TRUE(engine.createAggregate(makeAggDef("disk_1m", "disk", "srv1")));

    int64_t end_ms = kBaseMs + static_cast<int64_t>(kWindows) * kWindowMs;
    engine.refreshAggregate("disk_1m", end_ms);

    auto pts = engine.queryMaterialized("disk_1m", kBaseMs, end_ms);
    ASSERT_GE(pts.size(), 2u) << "Need at least 2 materialized points";

    // Verify the raw materialized timestamps are ordered.
    for (size_t i = 1; i < pts.size(); ++i) {
        EXPECT_LT(pts[i - 1].timestamp_ms, pts[i].timestamp_ms)
            << "queryMaterialized must return points in timestamp order (index "
            << i - 1 << " vs " << i << ")";
    }

    // Convert to TimeSeries and verify the analytics module preserves order.
    themisdb::analytics::TimeSeries ts;
    for (const auto& p : pts) {
      ts.push(p.timestamp_ms, p.value);
    }

    const auto& ts_pts = ts.points();
    ASSERT_EQ(ts_pts.size(), pts.size());
    for (size_t i = 1; i < ts_pts.size(); ++i) {
        EXPECT_LE(ts_pts[i - 1].timestamp_ms, ts_pts[i].timestamp_ms)
            << "TimeSeries must maintain non-decreasing timestamp order (index "
            << i - 1 << " vs " << i << ")";
    }
}

// ============================================================================
// Group B – mergeShardResults mathematical correctness (no DB required)
// ============================================================================

class ShardMergeTest : public ::testing::Test {};

// Helper: build a valid AggShardResult with all fields set.
static AggShardResult makeValidShard(const std::string& metric,
                                     double sum, size_t count,
                                     double min, double max,
                                     int64_t from_ms, int64_t to_ms) {
    AggShardResult s;
    s.metric   = metric;
    s.entity   = "host1";
    s.sum      = sum;
    s.count    = count;
    s.min      = min;
    s.max      = max;
    s.from_ms  = from_ms;
    s.to_ms    = to_ms;
    s.valid    = true;
    return s;
}

// ---------------------------------------------------------------------------
// B-1: Empty input → invalid result, zero aggregates
// ---------------------------------------------------------------------------
TEST_F(ShardMergeTest, MergeShardResults_EmptyInput_ReturnsInvalid) {
    auto merged = mergeShardResults({});
    EXPECT_FALSE(merged.valid);
    EXPECT_EQ(merged.sum,   0.0);
    EXPECT_EQ(merged.count, 0u);
}

// ---------------------------------------------------------------------------
// B-2: Single valid shard → identity (merged == input)
// ---------------------------------------------------------------------------
TEST_F(ShardMergeTest, MergeShardResults_SingleValidShard_EqualsInput) {
    auto shard = makeValidShard("cpu", 300.0, 3, 90.0, 110.0, 1000, 2000);
    auto merged = mergeShardResults({shard});

    EXPECT_TRUE(merged.valid);
    EXPECT_DOUBLE_EQ(merged.sum,   300.0);
    EXPECT_EQ(merged.count,        3u);
    EXPECT_DOUBLE_EQ(merged.min,   90.0);
    EXPECT_DOUBLE_EQ(merged.max,  110.0);
    EXPECT_DOUBLE_EQ(merged.avg(), 100.0);
    EXPECT_EQ(merged.from_ms, 1000);
    EXPECT_EQ(merged.to_ms,   2000);
}

// ---------------------------------------------------------------------------
// B-3: Two shards – sum and count are additive, avg is weighted
// ---------------------------------------------------------------------------
TEST_F(ShardMergeTest, MergeShardResults_TwoShards_SumCountAreAdditive) {
    // Shard 1: 3 observations summing to 150 (avg = 50)
    auto s1 = makeValidShard("cpu", 150.0, 3, 40.0, 60.0, 1000, 2000);
    // Shard 2: 5 observations summing to 250 (avg = 50)
    auto s2 = makeValidShard("cpu", 250.0, 5, 30.0, 70.0, 1500, 2500);

    auto merged = mergeShardResults({s1, s2});

    EXPECT_TRUE(merged.valid);
    EXPECT_DOUBLE_EQ(merged.sum,  400.0);
    EXPECT_EQ(merged.count,       8u);
    // Weighted average: (150 + 250) / (3 + 5) = 400 / 8 = 50.0
    EXPECT_DOUBLE_EQ(merged.avg(), 50.0);
}

// ---------------------------------------------------------------------------
// B-4: Weighted average is correct when shards have different loads
// ---------------------------------------------------------------------------
TEST_F(ShardMergeTest, MergeShardResults_WeightedAverage_CorrectAcrossUnequalShards) {
    // Shard A: 3 values with avg=10  (sum=30)
    auto sA = makeValidShard("q", 30.0, 3, 5.0, 15.0, 0, 1000);
    // Shard B: 7 values with avg=20  (sum=140)
    auto sB = makeValidShard("q", 140.0, 7, 12.0, 28.0, 0, 1000);

    auto merged = mergeShardResults({sA, sB});

    EXPECT_TRUE(merged.valid);
    EXPECT_DOUBLE_EQ(merged.sum,   170.0);
    EXPECT_EQ(merged.count,        10u);
    // Expected weighted avg: (30 + 140) / (3 + 7) = 170 / 10 = 17.0
    EXPECT_DOUBLE_EQ(merged.avg(), 17.0);
}

// ---------------------------------------------------------------------------
// B-5: min and max are global extrema across shards
// ---------------------------------------------------------------------------
TEST_F(ShardMergeTest, MergeShardResults_MinMax_AreGlobalExtrema) {
    auto s1 = makeValidShard("x", 130.0, 2, 50.0,  80.0, 0, 1000);
    auto s2 = makeValidShard("x", 210.0, 2, 10.0, 200.0, 0, 1000); // global min/max
    auto s3 = makeValidShard("x", 115.0, 2, 25.0,  90.0, 0, 1000);

    auto merged = mergeShardResults({s1, s2, s3});

    EXPECT_TRUE(merged.valid);
    EXPECT_DOUBLE_EQ(merged.min,  10.0)  << "Global min must come from shard 2";
    EXPECT_DOUBLE_EQ(merged.max, 200.0)  << "Global max must come from shard 2";
}

// ---------------------------------------------------------------------------
// B-6: from_ms / to_ms form the union of all shard time ranges
// ---------------------------------------------------------------------------
TEST_F(ShardMergeTest, MergeShardResults_TimeRange_IsUnionOfShards) {
    auto s1 = makeValidShard("t",  10.0, 1, 10.0, 10.0, 1000, 3000);
    auto s2 = makeValidShard("t",  20.0, 1, 20.0, 20.0, 2000, 5000);
    auto s3 = makeValidShard("t",  30.0, 1, 30.0, 30.0,  500, 1500); // earliest start

    auto merged = mergeShardResults({s1, s2, s3});

    EXPECT_TRUE(merged.valid);
    EXPECT_EQ(merged.from_ms,  500) << "from_ms must be the minimum across shards";
    EXPECT_EQ(merged.to_ms,   5000) << "to_ms must be the maximum across shards";
}

// ---------------------------------------------------------------------------
// B-7: Invalid shard is silently skipped; valid shard contributes fully
// ---------------------------------------------------------------------------
TEST_F(ShardMergeTest, MergeShardResults_InvalidShardSkipped) {
    auto valid   = makeValidShard("cpu", 200.0, 4, 40.0, 60.0, 1000, 2000);
    AggShardResult invalid;
    invalid.sum   = 999.0;
    invalid.count = 99;
    invalid.valid = false; // must be ignored

    auto merged = mergeShardResults({valid, invalid});

    EXPECT_TRUE(merged.valid);
    EXPECT_DOUBLE_EQ(merged.sum, 200.0) << "Invalid shard must not contribute to sum";
    EXPECT_EQ(merged.count,      4u)    << "Invalid shard must not contribute to count";
    EXPECT_DOUBLE_EQ(merged.avg(), 50.0);
}

// ---------------------------------------------------------------------------
// B-8: All shards invalid → merged result is invalid with zeroed aggregates
// ---------------------------------------------------------------------------
TEST_F(ShardMergeTest, MergeShardResults_AllInvalid_ResultIsInvalid) {
    AggShardResult s1; s1.valid = false; s1.sum = 100.0; s1.count = 2;
    AggShardResult s2; s2.valid = false; s2.sum = 200.0; s2.count = 4;

    auto merged = mergeShardResults({s1, s2});

    EXPECT_FALSE(merged.valid);
    EXPECT_EQ(merged.count, 0u);
    EXPECT_DOUBLE_EQ(merged.sum, 0.0);
    EXPECT_DOUBLE_EQ(merged.min, 0.0);
    EXPECT_DOUBLE_EQ(merged.max, 0.0);
}
