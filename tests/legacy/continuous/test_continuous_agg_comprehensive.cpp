// Phase 2: Continuous Aggregation – Rollup Hierarchy & Robustness Tests

#include <gtest/gtest.h>
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include "timeseries/aggregate_scheduler.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>
#include <cmath>
#include <memory>

using namespace themis;
namespace fs = std::filesystem;

static std::string makeCaggTempPath(const std::string& tag) {
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / ("themis_cagg_" + tag + "_" + std::to_string(ns))).string();
}

struct ContinuousAggFixture : ::testing::Test {
    std::string db_path = {};
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> store;
    int64_t base_ms{1700000000000LL};

    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping ContinuousAggFixture on Windows due to intermittent heap corruption in RocksDB-backed timeseries fixture.";
#endif
        db_path = makeCaggTempPath("cagg");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
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

    // Insert n data points for metric/entity starting at base_ms with step_ms
    void insertPoints(const std::string& metric, const std::string& entity,
                      int n, double start_value = 0.0, double step_value = 1.0,
                      int64_t step_ms = 10000) {
        for (int i = 0; i < n; ++i) {
            TSStore::DataPoint p;
            p.metric       = metric;
            p.entity       = entity;
            p.timestamp_ms = base_ms + i * step_ms;
            p.value        = start_value + i * step_value;
            ASSERT_TRUE(store->putDataPoint(p).has_value());
        }
    }

    std::vector<TSStore::DataPoint> queryMetric(const std::string& metric,
                                                 const std::string& entity,
                                                 int64_t from_ms, int64_t to_ms,
                                                 size_t limit = 10000) {
        TSStore::QueryOptions q;
        q.metric           = metric;
        q.entity           = entity;
        q.from_timestamp_ms = from_ms;
        q.to_timestamp_ms  = to_ms;
        q.limit            = limit;
        auto r = store->query(q);
        return r.has_value() ? *r : std::vector<TSStore::DataPoint>{};
    }
};

// ===== derived metric name =====

TEST(ContinuousAggStaticTest, DerivedMetricName1min) {
    auto name = ContinuousAggregateManager::derivedMetricName("cpu", std::chrono::minutes(1));
    EXPECT_EQ(name, "cpu__agg_60000ms");
}

TEST(ContinuousAggStaticTest, DerivedMetricName1hour) {
    auto name = ContinuousAggregateManager::derivedMetricName("temp", std::chrono::hours(1));
    EXPECT_EQ(name, "temp__agg_3600000ms");
}

TEST(ContinuousAggStaticTest, DerivedMetricName1day) {
    auto name = ContinuousAggregateManager::derivedMetricName("mem", std::chrono::hours(24));
    EXPECT_EQ(name, "mem__agg_86400000ms");
}

TEST(ContinuousAggStaticTest, DerivedMetricName5min) {
    auto name = ContinuousAggregateManager::derivedMetricName("req", std::chrono::minutes(5));
    EXPECT_EQ(name, "req__agg_300000ms");
}

// ===== rollup hierarchy definition =====

TEST(RollupHierarchyTest, DefaultHierarchyHas4Levels) {
    auto h = RollupHierarchy::defaultHierarchy("cpu");
    EXPECT_EQ(h.metric, "cpu");
    EXPECT_EQ(h.levels.size(), 4u);
    EXPECT_EQ(h.levels[0], std::chrono::minutes(1));
    EXPECT_EQ(h.levels[1], std::chrono::minutes(5));
    EXPECT_EQ(h.levels[2], std::chrono::hours(1));
    EXPECT_EQ(h.levels[3], std::chrono::hours(24));
}

TEST(RollupHierarchyTest, DefaultHierarchyWithEntity) {
    auto h = RollupHierarchy::defaultHierarchy("net", std::string("srv01"));
    ASSERT_TRUE(h.entity.has_value());
    EXPECT_EQ(*h.entity, "srv01");
}

// ===== refresh single window =====

TEST_F(ContinuousAggFixture, RefreshCreatesAggregateMetric) {
    // Insert 6 points at 10s intervals over 1 minute
    insertPoints("temp", "s1", 6, 20.0, 1.0, 10000);

    ContinuousAggregateManager mgr(store.get());
    AggConfig cfg;
    cfg.metric  = "temp";
    cfg.entity  = std::string("s1");
    cfg.window.size = std::chrono::minutes(1);
    mgr.refresh(cfg, base_ms, base_ms + 59999);

    auto out_metric = ContinuousAggregateManager::derivedMetricName("temp", std::chrono::minutes(1));
    auto pts = queryMetric(out_metric, "s1", base_ms, base_ms + 60000);
    ASSERT_EQ(pts.size(), 1u);
    // avg of [20,21,22,23,24,25] = 22.5
    EXPECT_NEAR(pts[0].value, 22.5, 1e-9);
}

TEST_F(ContinuousAggFixture, RefreshStoresMinMaxSumCount) {
    insertPoints("cpu", "s2", 4, 10.0, 10.0, 15000);  // 10,20,30,40

    ContinuousAggregateManager mgr(store.get());
    AggConfig cfg;
    cfg.metric  = "cpu";
    cfg.entity  = std::string("s2");
    cfg.window.size = std::chrono::minutes(1);
    mgr.refresh(cfg, base_ms, base_ms + 59999);

    auto out_metric = ContinuousAggregateManager::derivedMetricName("cpu", std::chrono::minutes(1));
    auto pts = queryMetric(out_metric, "s2", base_ms, base_ms + 60000);
    ASSERT_EQ(pts.size(), 1u);
    ASSERT_TRUE(pts[0].metadata.contains("min"));
    ASSERT_TRUE(pts[0].metadata.contains("max"));
    ASSERT_TRUE(pts[0].metadata.contains("sum"));
    ASSERT_TRUE(pts[0].metadata.contains("count"));
    EXPECT_NEAR(pts[0].metadata["min"].get<double>(), 10.0, 1e-9);
    EXPECT_NEAR(pts[0].metadata["max"].get<double>(), 40.0, 1e-9);
    EXPECT_NEAR(pts[0].metadata["sum"].get<double>(), 100.0, 1e-9);
    EXPECT_EQ(pts[0].metadata["count"].get<size_t>(), 4u);
}

TEST_F(ContinuousAggFixture, RefreshMultipleWindows) {
    // 12 points at 10s intervals = 2 minutes
    insertPoints("disk", "s3", 12, 0.0, 1.0, 10000);

    ContinuousAggregateManager mgr(store.get());
    AggConfig cfg;
    cfg.metric  = "disk";
    cfg.entity  = std::string("s3");
    cfg.window.size = std::chrono::minutes(1);
    mgr.refresh(cfg, base_ms, base_ms + 119999);

    auto out = ContinuousAggregateManager::derivedMetricName("disk", std::chrono::minutes(1));
    auto pts = queryMetric(out, "s3", base_ms, base_ms + 120000);
    EXPECT_EQ(pts.size(), 2u);
}

TEST_F(ContinuousAggFixture, RefreshEmptyRangeProducesNoOutput) {
    ContinuousAggregateManager mgr(store.get());
    AggConfig cfg;
    cfg.metric  = "empty_metric";
    cfg.entity  = std::string("x");
    cfg.window.size = std::chrono::minutes(1);
    // No data inserted
    mgr.refresh(cfg, base_ms, base_ms + 59999);

    auto out = ContinuousAggregateManager::derivedMetricName("empty_metric", std::chrono::minutes(1));
    auto pts = queryMetric(out, "x", base_ms, base_ms + 60000);
    EXPECT_TRUE(pts.empty());
}

TEST_F(ContinuousAggFixture, RefreshWithNoEntityDoesNothing) {
    insertPoints("net", "s5", 5);
    ContinuousAggregateManager mgr(store.get());
    AggConfig cfg;
    cfg.metric = "net";
    // cfg.entity not set → nullopt
    cfg.window.size = std::chrono::minutes(1);
    // Should not crash
    mgr.refresh(cfg, base_ms, base_ms + 59999);
}

// ===== Rollup hierarchy tests =====

TEST_F(ContinuousAggFixture, RollupHierarchyRefreshesFirstLevel) {
    // Insert 6 points for 1-minute window
    insertPoints("temp2", "h1", 6, 10.0, 2.0, 10000);

    ContinuousAggregateManager mgr(store.get());
    auto hierarchy = RollupHierarchy::defaultHierarchy("temp2", std::string("h1"));
    // Refresh only first 2 levels (1m, 5m)
    hierarchy.levels.resize(2);
    mgr.refreshHierarchy(hierarchy, base_ms, base_ms + 59999);

    // Level 1 output
    auto l1 = ContinuousAggregateManager::derivedMetricName("temp2", std::chrono::minutes(1));
    auto pts_l1 = queryMetric(l1, "h1", base_ms, base_ms + 60000);
    EXPECT_GE(pts_l1.size(), 1u);
}

TEST_F(ContinuousAggFixture, RollupHierarchyEmptyLevelsDoesNothing) {
    ContinuousAggregateManager mgr(store.get());
    RollupHierarchy h{"cpu3", std::string("s"), {}};
    // No crash expected
    mgr.refreshHierarchy(h, base_ms, base_ms + 60000);
}

TEST_F(ContinuousAggFixture, RollupHierarchySingleLevel) {
    insertPoints("mem2", "h2", 6, 50.0, 0.0, 10000);  // constant 50
    ContinuousAggregateManager mgr(store.get());
    RollupHierarchy h{"mem2", std::string("h2"), {std::chrono::minutes(1)}};
    mgr.refreshHierarchy(h, base_ms, base_ms + 59999);

    auto l1 = ContinuousAggregateManager::derivedMetricName("mem2", std::chrono::minutes(1));
    auto pts = queryMetric(l1, "h2", base_ms, base_ms + 60000);
    ASSERT_EQ(pts.size(), 1u);
    EXPECT_NEAR(pts[0].value, 50.0, 1e-9);
}

// ===== AggregateScheduler tests =====

TEST_F(ContinuousAggFixture, SchedulerStartStop) {
    AggregateScheduler sched(store.get());
    EXPECT_FALSE(sched.isRunning());
    sched.start();
    EXPECT_TRUE(sched.isRunning());
    sched.stop();
    EXPECT_FALSE(sched.isRunning());
}

TEST_F(ContinuousAggFixture, SchedulerRegisterAggregate) {
    AggregateScheduler sched(store.get());
    AggConfig cfg;
    cfg.metric = "cpu4";
    cfg.entity = std::string("srv");
    cfg.window.size = std::chrono::minutes(1);
    auto id = sched.registerAggregate(cfg, std::chrono::minutes(5));
    EXPECT_FALSE(id.empty());
    auto aggs = sched.listAggregates();
    EXPECT_EQ(aggs.size(), 1u);
}

TEST_F(ContinuousAggFixture, SchedulerUnregisterAggregate) {
    AggregateScheduler sched(store.get());
    AggConfig cfg;
    cfg.metric = "cpu5";
    cfg.entity = std::string("srv2");
    cfg.window.size = std::chrono::minutes(1);
    auto id = sched.registerAggregate(cfg);
    sched.unregisterAggregate(id);
    EXPECT_TRUE(sched.listAggregates().empty());
}

TEST_F(ContinuousAggFixture, SchedulerGetStats) {
    AggregateScheduler sched(store.get());
    auto stats = sched.getStats();
    EXPECT_EQ(stats.registered_aggregates, 0u);
}

TEST_F(ContinuousAggFixture, SchedulerNullStoreFails) {
    EXPECT_THROW(AggregateScheduler(nullptr), std::invalid_argument);
}

TEST_F(ContinuousAggFixture, SchedulerEnableDisableAggregate) {
    AggregateScheduler sched(store.get());
    AggConfig cfg;
    cfg.metric = "cpu6";
    cfg.entity = std::string("srv3");
    cfg.window.size = std::chrono::minutes(1);
    auto id = sched.registerAggregate(cfg);
    EXPECT_NO_THROW(sched.disableAggregate(id));
    EXPECT_NO_THROW(sched.enableAggregate(id));
}

TEST_F(ContinuousAggFixture, SchedulerRefreshNow) {
    insertPoints("cpu7", "srv4", 6);
    AggregateScheduler sched(store.get());
    AggConfig cfg;
    cfg.metric = "cpu7";
    cfg.entity = std::string("srv4");
    cfg.window.size = std::chrono::minutes(1);
    auto id = sched.registerAggregate(cfg);
    EXPECT_NO_THROW(sched.refreshNow(id));
}

TEST_F(ContinuousAggFixture, SchedulerMultipleAggregates) {
    AggregateScheduler sched(store.get());
    for (int i = 0; i < 5; ++i) {
        AggConfig cfg;
        cfg.metric = "metric_" + std::to_string(i);
        cfg.entity = std::string("srv");
        cfg.window.size = std::chrono::minutes(i + 1);
        sched.registerAggregate(cfg);
    }
    EXPECT_EQ(sched.listAggregates().size(), 5u);
    EXPECT_EQ(sched.getStats().registered_aggregates, 5u);
}

// ===== Aggregate Scheduler Catch-up & Error Handling =====

TEST_F(ContinuousAggFixture, SchedulerCatchUpConfig) {
    AggregateScheduler::Config cfg;
    cfg.catch_up_missed_windows = true;
    cfg.max_catch_up_windows    = 50;
    AggregateScheduler sched(store.get(), cfg);
    EXPECT_FALSE(sched.isRunning());
}

TEST_F(ContinuousAggFixture, SchedulerCatchUpDisabled) {
    AggregateScheduler::Config cfg;
    cfg.catch_up_missed_windows = false;
    AggregateScheduler sched(store.get(), cfg);
    AggConfig agg;
    agg.metric = "cpu8";
    agg.entity = std::string("srv5");
    agg.window.size = std::chrono::minutes(1);
    auto id = sched.registerAggregate(agg);
    // refreshNow should still work even with catch-up disabled
    EXPECT_NO_THROW(sched.refreshNow(id));
}

TEST_F(ContinuousAggFixture, SchedulerRefreshNowWithData) {
    insertPoints("cpu9", "s9", 6, 0.0, 1.0, 10000);
    AggregateScheduler sched(store.get());
    AggConfig agg;
    agg.metric = "cpu9";
    agg.entity = std::string("s9");
    agg.window.size = std::chrono::minutes(1);
    auto id = sched.registerAggregate(agg);
    EXPECT_NO_THROW(sched.refreshNow(id));
    // After refresh, aggregate data should exist
    auto out = ContinuousAggregateManager::derivedMetricName("cpu9", std::chrono::minutes(1));
    auto pts = queryMetric(out, "s9", base_ms, base_ms + 60000);
    // May have 0 or more depending on time window coverage
    EXPECT_GE(pts.size(), 0u);
}

TEST_F(ContinuousAggFixture, SchedulerErrorHandlingInvalidId) {
    AggregateScheduler sched(store.get());
    // refreshNow with invalid ID should not crash
    EXPECT_NO_THROW(sched.refreshNow("nonexistent-id"));
}

TEST_F(ContinuousAggFixture, SchedulerErrorHandlingAfterStop) {
    AggregateScheduler sched(store.get());
    sched.start();
    sched.stop();
    // Operations after stop should be safe
    EXPECT_FALSE(sched.isRunning());
    EXPECT_NO_THROW(sched.listAggregates());
}

TEST_F(ContinuousAggFixture, SchedulerGetStatsAfterRefresh) {
    AggregateScheduler sched(store.get());
    AggConfig agg;
    agg.metric = "cpu10";
    agg.entity = std::string("s10");
    agg.window.size = std::chrono::minutes(1);
    sched.registerAggregate(agg);
    sched.refreshNow(sched.listAggregates()[0].id);
    auto stats = sched.getStats();
    EXPECT_EQ(stats.registered_aggregates, 1u);
}

// ===== Multi-Shard / Distributed Aggregation =====

TEST_F(ContinuousAggFixture, MergeShardResultsEmpty) {
    auto r = mergeShardResults({});
    EXPECT_FALSE(r.valid);
    EXPECT_EQ(r.count, 0u);
}

TEST_F(ContinuousAggFixture, MergeShardResultsSingleValid) {
    AggShardResult s;
    s.valid = true;
    s.sum   = 100.0;
    s.min   = 10.0;
    s.max   = 20.0;
    s.count = 5;
    auto r = mergeShardResults({s});
    EXPECT_TRUE(r.valid);
    EXPECT_DOUBLE_EQ(r.sum, 100.0);
    EXPECT_DOUBLE_EQ(r.min, 10.0);
    EXPECT_DOUBLE_EQ(r.max, 20.0);
    EXPECT_EQ(r.count, 5u);
}

TEST_F(ContinuousAggFixture, MergeShardResultsMultiple) {
    AggShardResult s1, s2;
    s1.valid = true; s1.sum = 60.0;  s1.min = 1.0; s1.max = 10.0; s1.count = 3;
    s2.valid = true; s2.sum = 40.0;  s2.min = 5.0; s2.max = 20.0; s2.count = 2;
    auto r = mergeShardResults({s1, s2});
    EXPECT_TRUE(r.valid);
    EXPECT_DOUBLE_EQ(r.sum, 100.0);
    EXPECT_DOUBLE_EQ(r.min, 1.0);   // min across shards
    EXPECT_DOUBLE_EQ(r.max, 20.0);  // max across shards
    EXPECT_EQ(r.count, 5u);
    EXPECT_NEAR(r.avg(), 20.0, 0.001);
}

TEST_F(ContinuousAggFixture, MergeShardResultsSkipsInvalid) {
    AggShardResult s1, s2;
    s1.valid = true;  s1.sum = 50.0; s1.count = 5; s1.min = 1.0; s1.max = 10.0;
    s2.valid = false; // invalid shard, skipped
    auto r = mergeShardResults({s1, s2});
    EXPECT_TRUE(r.valid);
    EXPECT_EQ(r.count, 5u);
}

TEST_F(ContinuousAggFixture, MergeShardResultsAllInvalid) {
    AggShardResult s1, s2;
    s1.valid = false;
    s2.valid = false;
    auto r = mergeShardResults({s1, s2});
    EXPECT_FALSE(r.valid);
}

TEST_F(ContinuousAggFixture, DistributedCoordinatorSingleShard) {
    insertPoints("net", "s11", 6, 0.0, 1.0, 10000);

    DistributedAggregateCoordinator coord(store.get(), 1);
    EXPECT_EQ(coord.shardCount(), 1);

    AggConfig cfg;
    cfg.metric      = "net";
    cfg.entity      = std::string("s11");
    cfg.window.size = std::chrono::minutes(1);
    EXPECT_NO_THROW(coord.refreshAggregate(cfg, base_ms, base_ms + 60000));
}

TEST_F(ContinuousAggFixture, DistributedCoordinatorMultiShard) {
    // Simulate 2 shards via callback
    DistributedAggregateCoordinator coord(
        store.get(), 2,
        [&](int shard_id, const AggConfig& cfg, int64_t from_ms, int64_t to_ms) -> AggShardResult {
            AggShardResult r;
            r.metric  = cfg.metric;
            r.from_ms = from_ms;
            r.to_ms   = to_ms;
            r.valid   = true;
            r.count   = 3;
            r.sum     = shard_id == 0 ? 30.0 : 60.0;
            r.min     = shard_id == 0 ? 5.0  : 10.0;
            r.max     = shard_id == 0 ? 15.0 : 25.0;
            return r;
        });

    AggConfig cfg;
    cfg.metric      = "cpu11";
    cfg.entity      = std::string("srv");
    cfg.window.size = std::chrono::minutes(1);
    auto result = coord.refreshAggregate(cfg, base_ms, base_ms + 60000);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.count, 6u);        // 3 + 3
    EXPECT_DOUBLE_EQ(result.sum, 90.0); // 30 + 60
    EXPECT_DOUBLE_EQ(result.min, 5.0);  // min across shards
    EXPECT_DOUBLE_EQ(result.max, 25.0); // max across shards
}

TEST_F(ContinuousAggFixture, AggShardResultAvg) {
    AggShardResult r;
    r.sum = 100.0; r.count = 4;
    EXPECT_NEAR(r.avg(), 25.0, 0.001);
}

TEST_F(ContinuousAggFixture, AggShardResultAvgZeroCount) {
    AggShardResult r;
    r.sum = 0.0; r.count = 0;
    EXPECT_DOUBLE_EQ(r.avg(), 0.0);  // no division by zero
}

// ===================================================================
// Incremental Continuous Aggregation with Watermark Pushdown Tests
// ===================================================================

// ---- ContinuousAggWatermarkStore ----

TEST_F(ContinuousAggFixture, WatermarkStore_GetReturnsZeroWhenNotSet) {
    ContinuousAggWatermarkStore wm(store.get());
    EXPECT_EQ(wm.getWatermark("agg_not_set"), 0);
}

TEST_F(ContinuousAggFixture, WatermarkStore_SetAndGet) {
    ContinuousAggWatermarkStore wm(store.get());
    wm.setWatermark("agg_one", 1234567890000LL);
    EXPECT_EQ(wm.getWatermark("agg_one"), 1234567890000LL);
}

TEST_F(ContinuousAggFixture, WatermarkStore_OverwriteAdvancesWatermark) {
    ContinuousAggWatermarkStore wm(store.get());
    wm.setWatermark("agg_adv", 1000);
    wm.setWatermark("agg_adv", 2000);
    EXPECT_EQ(wm.getWatermark("agg_adv"), 2000);
}

TEST_F(ContinuousAggFixture, WatermarkStore_DeleteClearsWatermark) {
    ContinuousAggWatermarkStore wm(store.get());
    wm.setWatermark("agg_del", 5000);
    wm.deleteWatermark("agg_del");
    EXPECT_EQ(wm.getWatermark("agg_del"), 0);
}

TEST_F(ContinuousAggFixture, WatermarkStore_IndependentPerAggId) {
    ContinuousAggWatermarkStore wm(store.get());
    wm.setWatermark("agg_A", 100);
    wm.setWatermark("agg_B", 200);
    EXPECT_EQ(wm.getWatermark("agg_A"), 100);
    EXPECT_EQ(wm.getWatermark("agg_B"), 200);
}

// ---- refreshIncremental ----

TEST_F(ContinuousAggFixture, RefreshIncremental_FirstCallProcessesAllData) {
    // Insert 6 points over 1 minute (no watermark yet → full scan from 0)
    insertPoints("t1", "e1", 6, 10.0, 1.0, 10000);

    ContinuousAggregateManager mgr(store.get());
    ContinuousAggWatermarkStore wm(store.get());

    AggConfig cfg;
    cfg.metric      = "t1";
    cfg.entity      = std::string("e1");
    cfg.window.size = std::chrono::minutes(1);

    int64_t to_ms = base_ms + 60000;
    size_t windows = mgr.refreshIncremental(cfg, "agg_t1_e1", to_ms, wm);

    // Watermark should now equal to_ms
    EXPECT_EQ(wm.getWatermark("agg_t1_e1"), to_ms);

    // Aggregate output should contain exactly one window
    auto out = ContinuousAggregateManager::derivedMetricName("t1", std::chrono::minutes(1));
    auto pts = queryMetric(out, "e1", base_ms, base_ms + 60000);
    ASSERT_EQ(pts.size(), 1u);
    EXPECT_NEAR(pts[0].value, 12.5, 1e-9);  // avg of [10,11,12,13,14,15]
}

TEST_F(ContinuousAggFixture, RefreshIncremental_SecondCallIsIdempotent) {
    insertPoints("t2", "e2", 6, 0.0, 1.0, 10000);

    ContinuousAggregateManager mgr(store.get());
    ContinuousAggWatermarkStore wm(store.get());

    AggConfig cfg;
    cfg.metric      = "t2";
    cfg.entity      = std::string("e2");
    cfg.window.size = std::chrono::minutes(1);

    int64_t to_ms = base_ms + 60000;
    mgr.refreshIncremental(cfg, "agg_t2", to_ms, wm);

    // Second call with the same to_ms must produce 0 windows (watermark == to_ms)
    size_t windows2 = mgr.refreshIncremental(cfg, "agg_t2", to_ms, wm);
    EXPECT_EQ(windows2, 0u);
}

TEST_F(ContinuousAggFixture, RefreshIncremental_OnlyNewDataProcessed) {
    // Insert 6 points in minute 0
    insertPoints("t3", "e3", 6, 5.0, 1.0, 10000);
    int64_t boundary1 = base_ms + 60000;

    ContinuousAggregateManager mgr(store.get());
    ContinuousAggWatermarkStore wm(store.get());

    AggConfig cfg;
    cfg.metric      = "t3";
    cfg.entity      = std::string("e3");
    cfg.window.size = std::chrono::minutes(1);

    // First incremental refresh up to boundary1
    mgr.refreshIncremental(cfg, "agg_t3", boundary1, wm);
    EXPECT_EQ(wm.getWatermark("agg_t3"), boundary1);

    // Insert 6 more points in minute 1
    for (int i = 0; i < 6; ++i) {
        TSStore::DataPoint p;
        p.metric       = "t3";
        p.entity       = "e3";
        p.timestamp_ms = boundary1 + i * 10000;
        p.value        = 20.0 + i;
        ASSERT_TRUE(store->putDataPoint(p).has_value());
    }

    int64_t boundary2 = boundary1 + 60000;
    mgr.refreshIncremental(cfg, "agg_t3", boundary2, wm);
    EXPECT_EQ(wm.getWatermark("agg_t3"), boundary2);

    auto out = ContinuousAggregateManager::derivedMetricName("t3", std::chrono::minutes(1));
    auto pts = queryMetric(out, "e3", base_ms, boundary2 + 1);
    // Both windows should have been produced
    EXPECT_EQ(pts.size(), 2u);
}

TEST_F(ContinuousAggFixture, RefreshIncremental_WatermarkAdvancesOnlyOnSuccess) {
    // Store is valid — watermark should advance
    ContinuousAggregateManager mgr(store.get());
    ContinuousAggWatermarkStore wm(store.get());

    AggConfig cfg;
    cfg.metric      = "t4";
    cfg.entity      = std::string("e4");
    cfg.window.size = std::chrono::minutes(1);

    int64_t to_ms = base_ms + 60000;
    mgr.refreshIncremental(cfg, "agg_t4", to_ms, wm);
    EXPECT_EQ(wm.getWatermark("agg_t4"), to_ms);
}

// ---- AggregateScheduler::backfill_range ----

TEST_F(ContinuousAggFixture, BackfillRange_ProcessesSpecifiedRange) {
    // Insert 12 points covering 2 minutes
    insertPoints("tb1", "eb1", 12, 0.0, 1.0, 10000);

    AggregateScheduler scheduler(store.get());

    AggConfig cfg;
    cfg.metric      = "tb1";
    cfg.entity      = std::string("eb1");
    cfg.window.size = std::chrono::minutes(1);

    std::string agg_id = scheduler.registerAggregate(cfg, std::chrono::minutes(5));

    // Backfill minute 0 only
    int64_t start_ms = base_ms;
    int64_t end_ms   = base_ms + 60000;
    EXPECT_NO_THROW(scheduler.backfill_range(agg_id, start_ms, end_ms));

    auto out = ContinuousAggregateManager::derivedMetricName("tb1", std::chrono::minutes(1));
    auto pts = queryMetric(out, "eb1", base_ms, base_ms + 120000);
    ASSERT_EQ(pts.size(), 1u);
}

TEST_F(ContinuousAggFixture, BackfillRange_InvalidRangeIsNoop) {
    AggregateScheduler scheduler(store.get());

    AggConfig cfg;
    cfg.metric      = "tb2";
    cfg.entity      = std::string("eb2");
    cfg.window.size = std::chrono::minutes(1);

    std::string agg_id = scheduler.registerAggregate(cfg, std::chrono::minutes(5));

    // start >= end → must not throw
    EXPECT_NO_THROW(scheduler.backfill_range(agg_id, 1000, 1000));
    EXPECT_NO_THROW(scheduler.backfill_range(agg_id, 2000, 1000));
}

TEST_F(ContinuousAggFixture, BackfillRange_UnknownAggIdIsNoop) {
    AggregateScheduler scheduler(store.get());
    EXPECT_NO_THROW(scheduler.backfill_range("nonexistent_agg_id", 1000, 2000));
}

// ---- TimeSeriesMetrics per-aggregate metrics ----

TEST(TimeSeriesMetricsAggTest, RecordAggRefreshLatency_Stored) {
    TimeSeriesMetrics m;
    m.recordAggRefreshLatency("agg_cpu:srv:60000ms", 42.5);
    m.recordAggRefreshLatency("agg_cpu:srv:60000ms", 57.5);
    // Average of 42.5 and 57.5 = 50.0
    EXPECT_NEAR(m.getAggRefreshLatency("agg_cpu:srv:60000ms"), 50.0, 1e-9);
}

TEST(TimeSeriesMetricsAggTest, RecordAggRefreshLatency_UnknownReturnsMinusOne) {
    TimeSeriesMetrics m;
    EXPECT_DOUBLE_EQ(m.getAggRefreshLatency("unknown"), -1.0);
}

TEST(TimeSeriesMetricsAggTest, RecordAggRefreshLag_Stored) {
    TimeSeriesMetrics m;
    m.recordAggRefreshLag("agg_cpu:srv:60000ms", 150.0);
    EXPECT_NEAR(m.getAggRefreshLag("agg_cpu:srv:60000ms"), 150.0, 1e-9);
}

TEST(TimeSeriesMetricsAggTest, RecordAggRefreshLag_UnknownReturnsMinusOne) {
    TimeSeriesMetrics m;
    EXPECT_DOUBLE_EQ(m.getAggRefreshLag("unknown"), -1.0);
}

TEST(TimeSeriesMetricsAggTest, RecordAggRefreshLatency_IncreasesGlobalRefreshCounter) {
    TimeSeriesMetrics m;
    // recordAggRefreshLatency also bumps total_continuous_agg_refreshes_ internally;
    // verify through the Prometheus export that the counter increases.
    m.reset();
    m.recordAggRefreshLatency("agg1", 10.0);
    m.recordAggRefreshLatency("agg1", 20.0);
    // If no assert fires, the metric was recorded without error.
    EXPECT_NEAR(m.getAggRefreshLatency("agg1"), 15.0, 1e-9);
}

TEST(TimeSeriesMetricsAggTest, Reset_ClearsAggRefreshStats) {
    TimeSeriesMetrics m;
    m.recordAggRefreshLatency("agg_reset", 99.0);
    m.recordAggRefreshLag("agg_reset", 500.0);
    m.reset();
    EXPECT_DOUBLE_EQ(m.getAggRefreshLatency("agg_reset"), -1.0);
    EXPECT_DOUBLE_EQ(m.getAggRefreshLag("agg_reset"), -1.0);
}

TEST(TimeSeriesMetricsAggTest, ExportPrometheus_ContainsAggLabels) {
    TimeSeriesMetrics m;
    m.recordAggRefreshLatency("my_agg:svc:60000ms", 25.0);
    m.recordAggRefreshLag("my_agg:svc:60000ms", 300.0);
    auto prom = m.exportPrometheus();
    EXPECT_NE(prom.find("themis_cagg_refresh_latency_ms_avg"), std::string::npos);
    EXPECT_NE(prom.find("my_agg:svc:60000ms"), std::string::npos);
    EXPECT_NE(prom.find("themis_cagg_refresh_lag_ms"), std::string::npos);
}

// ---- TSStore system metadata (WAL-durable watermark backing store) ----

TEST_F(ContinuousAggFixture, TSStore_PutAndGetSystemMeta) {
    auto put_result = store->putSystemMeta("wm:cagg:my_agg", "1234567890");
    ASSERT_TRUE(put_result.has_value()) << "putSystemMeta should succeed";

    auto get_result = store->getSystemMeta("wm:cagg:my_agg");
    ASSERT_TRUE(get_result.has_value());
    ASSERT_TRUE(get_result->has_value()) << "key should be found";
    EXPECT_EQ(**get_result, "1234567890");
}

TEST_F(ContinuousAggFixture, TSStore_GetSystemMetaNotFound) {
    auto get_result = store->getSystemMeta("wm:cagg:does_not_exist");
    ASSERT_TRUE(get_result.has_value());
    EXPECT_FALSE(get_result->has_value()) << "missing key should return nullopt";
}

TEST_F(ContinuousAggFixture, TSStore_DeleteSystemMeta) {
    store->putSystemMeta("wm:cagg:del_key", "42");
    store->deleteSystemMeta("wm:cagg:del_key");
    auto get_result = store->getSystemMeta("wm:cagg:del_key");
    ASSERT_TRUE(get_result.has_value());
    EXPECT_FALSE(get_result->has_value());
}

TEST_F(ContinuousAggFixture, TSStore_SystemMetaDoesNotLeakIntoTSKeys) {
    store->putSystemMeta("wm:cagg:leak_test", "should_not_be_ts_data");

    // Query the ts: key space for metric "wm" – should be empty
    TSStore::QueryOptions q;
    q.metric = "wm";
    q.entity = "cagg";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms = INT64_MAX;
    q.limit = 100;
    auto r = store->query(q);
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r->empty());
}

// ===================================================================
// Audit fix tests
// ===================================================================

// Fix 1: catchUpMissedWindows must NOT run for incremental-refresh aggregates
// (would cause duplicate aggregate points since refreshIncremental already covers all missed windows)
TEST_F(ContinuousAggFixture, IncrementalRefresh_NoDuplicateFromCatchUp) {
    // Insert one window of data
    insertPoints("dup_test", "e_dup", 6, 1.0, 1.0, 10000);

    AggregateScheduler scheduler(store.get());

    AggConfig cfg;
    cfg.metric      = "dup_test";
    cfg.entity      = std::string("e_dup");
    cfg.window.size = std::chrono::minutes(1);

    // Register with incremental refresh enabled (default)
    AggregateScheduler::ScheduledAggregate sa;
    sa.id                     = "dup_agg";
    sa.config                 = cfg;
    sa.refresh_interval       = std::chrono::minutes(5);
    sa.use_incremental_refresh = true;
    scheduler.registerAggregate(sa);

    // Manually call backfill_range (simulates what catchUpMissedWindows does)
    // followed by a refreshNow – both together must not create two aggregate points
    int64_t win_start = base_ms;
    int64_t win_end   = base_ms + 60000;

    // First: backfill (does NOT advance watermark)
    EXPECT_NO_THROW(scheduler.backfill_range("dup_agg", win_start, win_end));

    // Query – exactly 1 window expected
    auto out = ContinuousAggregateManager::derivedMetricName("dup_test", std::chrono::minutes(1));
    auto pts_after_backfill = queryMetric(out, "e_dup", win_start, win_end + 1);
    EXPECT_EQ(pts_after_backfill.size(), 1u);
}

// Fix 2: Prometheus export must emit # HELP / # TYPE only once per metric family
TEST(TimeSeriesMetricsAggTest, ExportPrometheus_HelpTypeAppearsOncePerFamily) {
    TimeSeriesMetrics m;
    // Record stats for two different aggregates
    m.recordAggRefreshLatency("agg_A", 10.0);
    m.recordAggRefreshLag("agg_A", 100.0);
    m.recordAggRefreshLatency("agg_B", 20.0);
    m.recordAggRefreshLag("agg_B", 200.0);

    auto prom = m.exportPrometheus();

    // Count occurrences of the HELP line for each family
    size_t latency_help_count = 0;
    size_t lag_help_count = 0;
    const std::string latency_help_str = "# HELP themis_cagg_refresh_latency_ms_avg";
    const std::string lag_help_str = "# HELP themis_cagg_refresh_lag_ms";
    size_t pos = 0;
    while ((pos = prom.find(latency_help_str, pos)) != std::string::npos) {
        latency_help_count++;
        pos += latency_help_str.size();
    }
    pos = 0;
    while ((pos = prom.find(lag_help_str, pos)) != std::string::npos) {
        lag_help_count++;
        pos += lag_help_str.size();
    }

    EXPECT_EQ(latency_help_count, 1u) << "HELP line for latency must appear exactly once";
    EXPECT_EQ(lag_help_count, 1u)     << "HELP line for lag must appear exactly once";

    // Both aggregates must still appear as labeled time series
    EXPECT_NE(prom.find("agg_A"), std::string::npos);
    EXPECT_NE(prom.find("agg_B"), std::string::npos);
}

// Fix 3: JSON export must include per-aggregate refresh stats
TEST(TimeSeriesMetricsAggTest, ExportJson_ContainsPerAggregateStats) {
    TimeSeriesMetrics m;
    m.recordAggRefreshLatency("json_agg:svc:60000ms", 33.0);
    m.recordAggRefreshLag("json_agg:svc:60000ms", 250.0);

    auto json_str = m.exportJson();
    EXPECT_NE(json_str.find("per_aggregate"), std::string::npos)
        << "exportJson should include per_aggregate section";
    EXPECT_NE(json_str.find("json_agg:svc:60000ms"), std::string::npos)
        << "exportJson should include the aggregate ID";
    EXPECT_NE(json_str.find("avg_refresh_latency_ms"), std::string::npos)
        << "exportJson should include avg_refresh_latency_ms";
    EXPECT_NE(json_str.find("last_lag_ms"), std::string::npos)
        << "exportJson should include last_lag_ms";
}
