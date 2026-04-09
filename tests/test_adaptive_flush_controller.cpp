/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_adaptive_flush_controller.cpp                 ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_adaptive_flush_controller.cpp
 * @brief Integration tests for AdaptiveFlushController + TSStore write path.
 *
 * Test Groups:
 *   Unit          – AdaptiveFlushController construction, config, getStats, isBackpressured
 *   Integration   – TSStore.putDataPoint / putDataPoints routing (buffered + direct modes)
 *   Metrics       – Stats queryable via TSStore::getAdaptiveFlushStats()
 *   Regression    – Legacy fallback path (no controller), auto_buffer_ priority ordering
 *
 * Issue: PERF-D1-B
 */

#include <gtest/gtest.h>
#include "timeseries/adaptive_flush_controller.h"
#include "timeseries/tsstore.h"
#include "timeseries/timeseries_metrics.h"
#include "storage/rocksdb_wrapper.h"
#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

namespace themis {
namespace {

// =========================================================================
// Fixture
// =========================================================================

static std::string makeTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_afc_" + tag + "_" + std::to_string(ns))).string();
}

struct AFCFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> tsstore;

    void SetUp() override {
        db_path = makeTempPath("afc");
        RocksDBWrapper::Config cfg;
        cfg.db_path      = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "Failed to open RocksDB at " << db_path;
        tsstore = std::make_unique<TSStore>(db->getRawDB());
    }

    void TearDown() override {
        tsstore.reset();
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    static TSStore::DataPoint makePoint(const std::string& metric,
                                        const std::string& entity,
                                        double value,
                                        int64_t ts_ms = 1700000000000LL) {
        TSStore::DataPoint p;
        p.metric       = metric;
        p.entity       = entity;
        p.timestamp_ms = ts_ms;
        p.value        = value;
        return p;
    }

    static AdaptiveFlushControllerConfig defaultConfig() {
        AdaptiveFlushControllerConfig cfg;
        cfg.capacity              = 500;
        cfg.flush_interval        = std::chrono::milliseconds{200};
        cfg.watermark_ratio       = 0.80;
        cfg.overdue_flush_multiplier = 2;
        cfg.enable_adaptive_flush = true;
        cfg.backpressure_slo_ms   = 50.0;
        cfg.ewma_alpha            = 0.5;
        cfg.adaptive_batch_min    = 10;
        cfg.adaptive_batch_max    = 500;
        cfg.flush_batch_size      = 50;
        cfg.async_flush           = false;
        return cfg;
    }
};

// =========================================================================
// Unit: Construction
// =========================================================================

TEST_F(AFCFixture, ConstructionWithDefaultConfig) {
    AdaptiveFlushController ctrl(tsstore.get());
    EXPECT_FALSE(ctrl.isRunning());
    auto stats = ctrl.getStats();
    EXPECT_EQ(stats.points_buffered, 0u);
    EXPECT_EQ(stats.points_flushed, 0u);
    EXPECT_EQ(stats.flush_count, 0u);
    EXPECT_EQ(stats.backpressure_events, 0u);
    EXPECT_EQ(stats.current_buffer_size, 0u);
}

TEST_F(AFCFixture, ConstructionWithCustomConfig) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    EXPECT_EQ(ctrl.getConfig().capacity, cfg.capacity);
    EXPECT_EQ(ctrl.getConfig().flush_batch_size, cfg.flush_batch_size);
    EXPECT_EQ(ctrl.getConfig().watermark_ratio, cfg.watermark_ratio);
}

TEST_F(AFCFixture, NullStoreThrows) {
    EXPECT_THROW(AdaptiveFlushController(nullptr), std::invalid_argument);
}

// =========================================================================
// Unit: isBackpressured
// =========================================================================

TEST_F(AFCFixture, IsBackpressuredFalseWhenEmpty) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    EXPECT_FALSE(ctrl.isBackpressured());
}

TEST_F(AFCFixture, IsBackpressuredFalseAfterFewPoints) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);

    auto r = ctrl.add(makePoint("cpu", "s1", 1.0));
    EXPECT_TRUE(r.has_value());
    EXPECT_FALSE(ctrl.isBackpressured());
}

// =========================================================================
// Unit: getStats
// =========================================================================

TEST_F(AFCFixture, StatsPointsBufferedIncrements) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);

    ctrl.add(makePoint("mem", "h1", 42.0, 1700000000000LL));
    ctrl.add(makePoint("mem", "h1", 43.0, 1700000000001LL));

    auto stats = ctrl.getStats();
    EXPECT_GE(stats.points_buffered, 2u);
}

TEST_F(AFCFixture, StatsPointsFlushedAfterManualFlush) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);

    for (int i = 0; i < 3; ++i) {
        ctrl.add(makePoint("net", "r1", static_cast<double>(i), 1700000000000LL + i));
    }
    size_t flushed = ctrl.flush();
    EXPECT_GE(flushed, 3u);
    EXPECT_GE(ctrl.getStats().points_flushed, 3u);
}

// =========================================================================
// Integration: TSStore.setAdaptiveFlushController / getter
// =========================================================================

TEST_F(AFCFixture, TSStoreGetAdaptiveFlushControllerNullByDefault) {
    EXPECT_EQ(tsstore->getAdaptiveFlushController(), nullptr);
}

TEST_F(AFCFixture, TSStoreSetAndGetAdaptiveFlushController) {
    AdaptiveFlushController ctrl(tsstore.get(), defaultConfig());
    tsstore->setAdaptiveFlushController(&ctrl);
    EXPECT_EQ(tsstore->getAdaptiveFlushController(), &ctrl);

    tsstore->setAdaptiveFlushController(nullptr);
    EXPECT_EQ(tsstore->getAdaptiveFlushController(), nullptr);
}

// =========================================================================
// Integration: putDataPoint routes via AdaptiveFlushController
// =========================================================================

TEST_F(AFCFixture, PutDataPointBufferedWhenControllerSet) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    tsstore->setAdaptiveFlushController(&ctrl);

    auto result = tsstore->putDataPoint(makePoint("cpu", "s1", 1.0));
    EXPECT_TRUE(result.has_value()) << result.error().message();

    // Point must be in buffer (not yet flushed)
    auto stats = ctrl.getStats();
    EXPECT_GE(stats.points_buffered, 1u);

    tsstore->setAdaptiveFlushController(nullptr);
}

TEST_F(AFCFixture, PutDataPointFlushesDataToStorage) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    tsstore->setAdaptiveFlushController(&ctrl);

    auto pt = makePoint("cpu", "s1", 99.5, 1700000000000LL);
    ASSERT_TRUE(tsstore->putDataPoint(pt).has_value());

    // Flush and verify the data is queryable
    ctrl.flush();

    TSStore::QueryOptions q;
    q.metric           = "cpu";
    q.entity           = "s1";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms  = INT64_MAX;
    auto results = tsstore->query(q);
    EXPECT_TRUE(results.has_value());
    EXPECT_FALSE(results.value().empty());

    tsstore->setAdaptiveFlushController(nullptr);
}

// =========================================================================
// Integration: putDataPoints routes via AdaptiveFlushController
// =========================================================================

TEST_F(AFCFixture, PutDataPointsBatchBufferedWhenControllerSet) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    tsstore->setAdaptiveFlushController(&ctrl);

    std::vector<TSStore::DataPoint> pts;
    for (int i = 0; i < 5; ++i) {
        pts.push_back(makePoint("disk", "d1", static_cast<double>(i),
                                1700000000000LL + i));
    }
    auto result = tsstore->putDataPoints(pts);
    EXPECT_TRUE(result.has_value()) << result.error().message();

    auto stats = ctrl.getStats();
    EXPECT_GE(stats.points_buffered, 5u);

    tsstore->setAdaptiveFlushController(nullptr);
}

TEST_F(AFCFixture, PutDataPointsBatchFlushesToStorage) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    tsstore->setAdaptiveFlushController(&ctrl);

    std::vector<TSStore::DataPoint> pts;
    for (int i = 0; i < 4; ++i) {
        pts.push_back(makePoint("disk", "d2", static_cast<double>(i),
                                1700000000000LL + i));
    }
    ASSERT_TRUE(tsstore->putDataPoints(pts).has_value());
    ctrl.flush();

    TSStore::QueryOptions q;
    q.metric           = "disk";
    q.entity           = "d2";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms  = INT64_MAX;
    auto results = tsstore->query(q);
    EXPECT_TRUE(results.has_value());
    EXPECT_GE(results.value().size(), 4u);

    tsstore->setAdaptiveFlushController(nullptr);
}

// =========================================================================
// Integration: Direct write (legacy fallback – no controller set)
// =========================================================================

TEST_F(AFCFixture, PutDataPointDirectWhenNoControllerSet) {
    // No controller attached → direct RocksDB write
    EXPECT_EQ(tsstore->getAdaptiveFlushController(), nullptr);

    auto result = tsstore->putDataPoint(makePoint("temp", "sensor1", 22.5));
    EXPECT_TRUE(result.has_value()) << result.error().message();

    TSStore::QueryOptions q;
    q.metric           = "temp";
    q.entity           = "sensor1";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms  = INT64_MAX;
    auto results = tsstore->query(q);
    EXPECT_TRUE(results.has_value());
    EXPECT_FALSE(results.value().empty());
}

TEST_F(AFCFixture, PutDataPointsDirectWhenNoControllerSet) {
    EXPECT_EQ(tsstore->getAdaptiveFlushController(), nullptr);

    std::vector<TSStore::DataPoint> pts;
    for (int i = 0; i < 3; ++i) {
        pts.push_back(makePoint("pressure", "p1", static_cast<double>(i),
                                1700000000000LL + i));
    }
    auto result = tsstore->putDataPoints(pts);
    EXPECT_TRUE(result.has_value()) << result.error().message();

    TSStore::QueryOptions q;
    q.metric           = "pressure";
    q.entity           = "p1";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms  = INT64_MAX;
    auto results = tsstore->query(q);
    EXPECT_TRUE(results.has_value());
    EXPECT_GE(results.value().size(), 3u);
}

// =========================================================================
// Integration: Remove controller → reverts to legacy path
// =========================================================================

TEST_F(AFCFixture, RemoveControllerRevertsToDirectWrite) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    tsstore->setAdaptiveFlushController(&ctrl);

    ASSERT_TRUE(tsstore->putDataPoint(makePoint("cpu", "x1", 1.0)).has_value());
    auto stats_before = ctrl.getStats();
    EXPECT_GE(stats_before.points_buffered, 1u);

    // Remove controller
    tsstore->setAdaptiveFlushController(nullptr);

    // This write should go directly to RocksDB, not through the controller
    ASSERT_TRUE(tsstore->putDataPoint(makePoint("cpu", "x1", 2.0, 1700000000001LL)).has_value());
    auto stats_after = ctrl.getStats();
    // Controller should not have seen the second write
    EXPECT_EQ(stats_after.points_buffered, stats_before.points_buffered);
}

// =========================================================================
// Metrics: getAdaptiveFlushStats via TSStore
// =========================================================================

TEST_F(AFCFixture, TSStoreGetAdaptiveFlushStatsZeroWhenNoController) {
    auto stats = tsstore->getAdaptiveFlushStats();
    EXPECT_EQ(stats.points_buffered, 0u);
    EXPECT_EQ(stats.points_flushed, 0u);
    EXPECT_EQ(stats.flush_count, 0u);
    EXPECT_EQ(stats.backpressure_events, 0u);
    EXPECT_EQ(stats.current_buffer_size, 0u);
}

TEST_F(AFCFixture, TSStoreGetAdaptiveFlushStatsReflectsController) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    tsstore->setAdaptiveFlushController(&ctrl);

    tsstore->putDataPoint(makePoint("cpu", "s2", 1.0));
    tsstore->putDataPoint(makePoint("cpu", "s2", 2.0, 1700000000001LL));

    auto stats = tsstore->getAdaptiveFlushStats();
    EXPECT_GE(stats.points_buffered, 2u);

    tsstore->setAdaptiveFlushController(nullptr);
}

TEST_F(AFCFixture, TSStoreGetAdaptiveFlushStatsFlushCount) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    tsstore->setAdaptiveFlushController(&ctrl);

    tsstore->putDataPoint(makePoint("io", "d1", 10.0));
    ctrl.flush();

    auto stats = tsstore->getAdaptiveFlushStats();
    EXPECT_GE(stats.flush_count, 1u);
    EXPECT_GE(stats.points_flushed, 1u);

    tsstore->setAdaptiveFlushController(nullptr);
}

// =========================================================================
// Integration: Metrics integration (TimeSeriesMetrics)
// =========================================================================

TEST_F(AFCFixture, MetricsIntegrationWiredThroughConfig) {
    TimeSeriesMetrics metrics;
    auto cfg = defaultConfig();
    cfg.metrics = &metrics;
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    EXPECT_EQ(ctrl.getConfig().metrics, &metrics);
}

TEST_F(AFCFixture, MetricsNullByDefault) {
    AdaptiveFlushController ctrl(tsstore.get());
    EXPECT_EQ(ctrl.getConfig().metrics, nullptr);
}

// =========================================================================
// Integration: addBatch returns error on first invalid point
// =========================================================================

TEST_F(AFCFixture, AddBatchWithEmptyMetricReturnsError) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);

    std::vector<TSStore::DataPoint> pts = {
        makePoint("cpu", "s1", 1.0),
        makePoint("",    "s1", 2.0),  // invalid: empty metric
        makePoint("cpu", "s1", 3.0, 1700000000002LL),
    };
    auto result = ctrl.addBatch(pts);
    EXPECT_FALSE(result.has_value());
}

TEST_F(AFCFixture, AddWithEmptyEntityReturnsError) {
    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);

    auto result = ctrl.add(makePoint("cpu", "", 1.0));
    EXPECT_FALSE(result.has_value());
}

// =========================================================================
// Regression: AdaptiveFlushController takes priority over auto_buffer_
// =========================================================================

TEST_F(AFCFixture, AdaptiveControllerTakesPriorityOverAutoBuffer) {
    // Set up a TSAutoBuffer (legacy) and also an AdaptiveFlushController.
    // Only the controller should see the write.
    TSAutoBufferConfig buf_cfg;
    buf_cfg.async_flush = false;
    TSAutoBuffer legacy_buf(tsstore.get(), buf_cfg);
    tsstore->setAutoBuffer(&legacy_buf);

    auto cfg = defaultConfig();
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    tsstore->setAdaptiveFlushController(&ctrl);

    tsstore->putDataPoint(makePoint("cpu", "s3", 5.0));

    auto ctrl_stats   = ctrl.getStats();
    auto legacy_stats = legacy_buf.getStats();

    EXPECT_GE(ctrl_stats.points_buffered, 1u);
    // The legacy buffer should NOT have received this write
    EXPECT_EQ(legacy_stats.points_buffered.load(), 0u);

    tsstore->setAdaptiveFlushController(nullptr);
    tsstore->setAutoBuffer(nullptr);
}

// =========================================================================
// Integration: start/stop lifecycle
// =========================================================================

TEST_F(AFCFixture, StartStopLifecycle) {
    auto cfg = defaultConfig();
    cfg.async_flush = true;
    AdaptiveFlushController ctrl(tsstore.get(), cfg);

    EXPECT_FALSE(ctrl.isRunning());
    ctrl.start();
    EXPECT_TRUE(ctrl.isRunning());
    ctrl.stop();
    EXPECT_FALSE(ctrl.isRunning());
}

TEST_F(AFCFixture, StopFlushesRemainingPoints) {
    auto cfg = defaultConfig();
    cfg.async_flush = true;
    AdaptiveFlushController ctrl(tsstore.get(), cfg);
    ctrl.start();

    tsstore->setAdaptiveFlushController(&ctrl);
    for (int i = 0; i < 3; ++i) {
        tsstore->putDataPoint(makePoint("cpu", "stop_test", static_cast<double>(i),
                                        1700000000000LL + i));
    }
    ctrl.stop();

    auto stats = ctrl.getStats();
    EXPECT_GE(stats.points_flushed, 3u);

    tsstore->setAdaptiveFlushController(nullptr);
}

}  // namespace
}  // namespace themis
