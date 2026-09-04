/**
 * @file test_adaptive_flush_controller.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <gtest/gtest.h>

#include "timeseries/adaptive_flush_controller.h"
#include "timeseries/timeseries_metrics.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

namespace themis {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string makeTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_afc_" + tag + "_" + std::to_string(ns))).string();
}

struct AFCFixture : ::testing::Test {
    std::string db_path = {};
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> tsstore;

    void SetUp() override {
        db_path = makeTempPath("afc");
        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "Failed to open RocksDB at " << db_path;
        tsstore = std::make_unique<TSStore>(db->getRawDB());
    }

    void TearDown() override {
        tsstore.reset();
        if (db) {
            db->close();
        }
        db.reset();
        std::error_code ec = {};
        std::filesystem::remove_all(db_path, ec);
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

    /// Build a controller with a large buffer so tests don't accidentally
    /// trigger the watermark unless they specifically set it up.
    static AdaptiveFlushControllerConfig quietConfig() {
        AdaptiveFlushControllerConfig cfg;
        cfg.buffer_capacity  = 10'000;
        cfg.flush_interval   = std::chrono::milliseconds{5000}; // long — won't fire during tests
        cfg.watermark_ratio  = 0.80;
        cfg.flush_batch_size = 500;
        cfg.async_flush      = false;
        return cfg;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// 1. Default configuration
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdaptiveFlushControllerConfig, DefaultCapacityIs10k) {
    AdaptiveFlushControllerConfig cfg;
    EXPECT_EQ(cfg.buffer_capacity, 10'000u);
}

TEST(AdaptiveFlushControllerConfig, DefaultFlushIntervalIs100ms) {
    AdaptiveFlushControllerConfig cfg;
    EXPECT_EQ(cfg.flush_interval.count(), 100);
}

TEST(AdaptiveFlushControllerConfig, DefaultWatermarkRatioIs80Percent) {
    AdaptiveFlushControllerConfig cfg;
    EXPECT_DOUBLE_EQ(cfg.watermark_ratio, 0.80);
}

TEST(AdaptiveFlushControllerConfig, DefaultOverdueMultiplierIs2) {
    AdaptiveFlushControllerConfig cfg;
    EXPECT_EQ(cfg.overdue_flush_multiplier, 2u);
}

TEST(AdaptiveFlushControllerConfig, DefaultAsyncFlushEnabled) {
    AdaptiveFlushControllerConfig cfg;
    EXPECT_TRUE(cfg.async_flush);
}

TEST(AdaptiveFlushControllerConfig, DefaultMetricsIsNull) {
    AdaptiveFlushControllerConfig cfg;
    EXPECT_EQ(cfg.metrics, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Constructor guards
// ─────────────────────────────────────────────────────────────────────────────

TEST(AdaptiveFlushControllerConstruct, NullTSStoreThrows) {
    EXPECT_THROW(
        AdaptiveFlushController(nullptr),
        std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, ZeroCapacityThrows) {
    AdaptiveFlushControllerConfig cfg;
    cfg.buffer_capacity = 0;
    EXPECT_THROW(AdaptiveFlushController(tsstore.get(), cfg), std::invalid_argument);
}

TEST_F(AFCFixture, InvalidWatermarkRatioThrows) {
    AdaptiveFlushControllerConfig cfg;
    cfg.watermark_ratio = 0.0; // invalid: must be in (0, 1]
    EXPECT_THROW(AdaptiveFlushController(tsstore.get(), cfg), std::invalid_argument);
}

TEST_F(AFCFixture, NotRunningBeforeStart) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());
    EXPECT_FALSE(afc.isRunning());
}

TEST_F(AFCFixture, RunningAfterStart) {
    auto cfg = quietConfig();
    cfg.async_flush = true;
    AdaptiveFlushController afc(tsstore.get(), cfg);
    afc.start();
    EXPECT_TRUE(afc.isRunning());
    afc.stop();
}

TEST_F(AFCFixture, NotRunningAfterStop) {
    auto cfg = quietConfig();
    cfg.async_flush = true;
    AdaptiveFlushController afc(tsstore.get(), cfg);
    afc.start();
    afc.stop();
    EXPECT_FALSE(afc.isRunning());
}

TEST_F(AFCFixture, DoubleStartIsNoop) {
    auto cfg = quietConfig();
    cfg.async_flush = true;
    AdaptiveFlushController afc(tsstore.get(), cfg);
    afc.start();
    afc.start(); // must not crash
    afc.stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. add() — single-point buffering
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, AddSinglePointSucceeds) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());
    auto r = afc.add(makePoint("cpu", "srv01", 1.0));
    EXPECT_TRUE(r.has_value()) << r.error().message();
}

TEST_F(AFCFixture, AddEmptyMetricFails) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());
    TSStore::DataPoint p;
    p.metric       = "";
    p.entity       = "srv01";
    p.timestamp_ms = 1700000000000LL;
    p.value        = 1.0;
    auto r = afc.add(p);
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_API_INVALID_REQUEST);
}

TEST_F(AFCFixture, AddEmptyEntityFails) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());
    TSStore::DataPoint p;
    p.metric       = "cpu";
    p.entity       = "";
    p.timestamp_ms = 1700000000000LL;
    p.value        = 1.0;
    auto r = afc.add(p);
    EXPECT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_API_INVALID_REQUEST);
}

TEST_F(AFCFixture, AddUpdatesBufferStats) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());
    (void)afc.add(makePoint("cpu", "s1", 1.0, 1700000000000LL));
    (void)afc.add(makePoint("cpu", "s1", 2.0, 1700000000001LL));

    auto s = afc.getStats();
    EXPECT_EQ(s.points_buffered, 2u);
    EXPECT_EQ(s.current_buffer_size, 2u);
    EXPECT_GT(s.buffer_utilization, 0.0);
    EXPECT_LT(s.buffer_utilization, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. addBatch() — batch buffering
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, AddBatchEmptyIsOk) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());
    auto r = afc.addBatch({});
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(*r, 0u);
}

TEST_F(AFCFixture, AddBatchRejectsInvalidPoint) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());
    TSStore::DataPoint bad;
    bad.entity       = "s1";
    bad.timestamp_ms = 1700000000000LL;
    bad.value        = 1.0;
    // metric is empty → invalid
    auto r = afc.addBatch({bad});
    EXPECT_FALSE(r.has_value());
}

TEST_F(AFCFixture, AddBatchAcceptsAllValidPoints) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());

    std::vector<TSStore::DataPoint> batch = {};

    for (int i = 0; i < 10; ++i) {
        batch.push_back(makePoint("cpu", "s1", static_cast<double>(i),
                                  1700000000000LL + i));
    }
    auto r = afc.addBatch(batch);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(*r, 10u);

    EXPECT_EQ(afc.getStats().points_buffered, 10u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. Manual flush()
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, ManualFlushWritesPointsToStore) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());

    for (int i = 0; i < 5; ++i) {
        (void)afc.add(makePoint("mem", "h1", static_cast<double>(i),
                                1700000000000LL + i));
    }
    size_t flushed = afc.flush();
    EXPECT_EQ(flushed, 5u);

    auto s = afc.getStats();
    EXPECT_EQ(s.points_flushed, 5u);
    EXPECT_EQ(s.flush_count, 1u);
    EXPECT_EQ(s.current_buffer_size, 0u);
}

TEST_F(AFCFixture, ManualFlushOnEmptyBufferReturnsZero) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());
    EXPECT_EQ(afc.flush(), 0u);
}

TEST_F(AFCFixture, MultipleManualFlushesAccumulate) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());

    (void)afc.add(makePoint("disk", "s1", 1.0, 1700000000000LL));
    (void)afc.flush();
    (void)afc.add(makePoint("disk", "s1", 2.0, 1700000000001LL));
    (void)afc.flush();

    auto s = afc.getStats();
    EXPECT_EQ(s.flush_count, 2u);
    EXPECT_EQ(s.points_flushed, 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. Watermark-triggered flush
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, WatermarkAtExactly80Percent) {
    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.buffer_capacity  = 10;
    cfg.watermark_ratio  = 0.80; // threshold = 8
    cfg.async_flush      = true;
    cfg.flush_interval   = std::chrono::milliseconds{5000}; // no timeout-flush
    cfg.flush_batch_size = 100;

    AdaptiveFlushController afc(tsstore.get(), cfg);
    afc.start();

    // Add 8 points (= 80 %) — should trigger watermark
    for (int i = 0; i < 8; ++i) {
        (void)afc.add(makePoint("wm", "s1", static_cast<double>(i),
                                1700000000000LL + i));
    }

    // Give the background thread time to flush
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    afc.stop();

    EXPECT_GT(afc.getStats().points_flushed, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. Timeout-triggered flush
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, TimeoutTriggerFlushesPoints) {
    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.flush_interval   = std::chrono::milliseconds{50}; // very short
    cfg.watermark_ratio  = 1.0;   // never reach watermark
    cfg.buffer_capacity  = 10'000;
    cfg.async_flush      = true;
    cfg.flush_batch_size = 500;

    AdaptiveFlushController afc(tsstore.get(), cfg);
    afc.start();

    (void)afc.add(makePoint("net", "h1", 1.0, 1700000000000LL));
    (void)afc.add(makePoint("net", "h1", 2.0, 1700000000001LL));

    // Wait longer than the flush interval
    std::this_thread::sleep_for(std::chrono::milliseconds{300});
    afc.stop();

    EXPECT_GT(afc.getStats().points_flushed, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. isBackpressured()
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, NotBackpressuredOnEmptyBuffer) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());
    EXPECT_FALSE(afc.isBackpressured());
}

TEST_F(AFCFixture, BackpressuredWhenWatermarkReached) {
    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.buffer_capacity = 10;
    cfg.watermark_ratio = 0.80; // threshold = 8

    AdaptiveFlushController afc(tsstore.get(), cfg);

    // Add 8 points (= watermark)
    for (int i = 0; i < 8; ++i) {
        // Use async=false to avoid background thread draining during add
        (void)afc.add(makePoint("bp", "s1", static_cast<double>(i),
                                1700000000000LL + i));
    }
    EXPECT_TRUE(afc.isBackpressured());
}

TEST_F(AFCFixture, BackpressureClearedAfterFlush) {
    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.buffer_capacity = 10;
    cfg.watermark_ratio = 0.80; // threshold = 8

    AdaptiveFlushController afc(tsstore.get(), cfg);

    for (int i = 0; i < 8; ++i) {
        (void)afc.add(makePoint("bp", "s1", static_cast<double>(i),
                                1700000000000LL + i));
    }
    ASSERT_TRUE(afc.isBackpressured());

    (void)afc.flush(); // drain buffer
    EXPECT_FALSE(afc.isBackpressured());
}

// ─────────────────────────────────────────────────────────────────────────────
// 10. getStats() — correctness
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, StatsInitiallyZero) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());
    auto s = afc.getStats();
    EXPECT_EQ(s.points_buffered, 0u);
    EXPECT_EQ(s.points_flushed, 0u);
    EXPECT_EQ(s.flush_count, 0u);
    EXPECT_EQ(s.backpressure_events, 0u);
    EXPECT_EQ(s.overdue_flush_events, 0u);
    EXPECT_EQ(s.current_buffer_size, 0u);
    EXPECT_DOUBLE_EQ(s.buffer_utilization, 0.0);
}

TEST_F(AFCFixture, BufferUtilisationRatio) {
    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.buffer_capacity = 1000;
    AdaptiveFlushController afc(tsstore.get(), cfg);

    for (int i = 0; i < 100; ++i) {
        (void)afc.add(makePoint("util", "s1", static_cast<double>(i),
                                1700000000000LL + i));
    }
    auto s = afc.getStats();
    EXPECT_NEAR(s.buffer_utilization, 0.10, 0.01);
}

TEST_F(AFCFixture, StatsFlushCountAndPointsFlushedAfterFlush) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());

    for (int i = 0; i < 7; ++i) {
        (void)afc.add(makePoint("q", "e1", static_cast<double>(i),
                                1700000000000LL + i));
    }
    (void)afc.flush();

    auto s = afc.getStats();
    EXPECT_EQ(s.points_buffered, 7u);
    EXPECT_EQ(s.points_flushed, 7u);
    EXPECT_EQ(s.flush_count, 1u);
    EXPECT_EQ(s.current_buffer_size, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 11. Buffer fully drained after stop()
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, StopFlushesRemainingPoints) {
    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.async_flush = true;

    AdaptiveFlushController afc(tsstore.get(), cfg);
    afc.start();

    for (int i = 0; i < 20; ++i) {
        (void)afc.add(makePoint("stop", "s1", static_cast<double>(i),
                                1700000000000LL + i));
    }
    afc.stop();

    EXPECT_EQ(afc.getStats().current_buffer_size, 0u);
    EXPECT_EQ(afc.getStats().points_flushed, 20u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 12. Stop during backpressure wait — no deadlock
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, StopDuringBackpressureNoDeadlock) {
    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.buffer_capacity  = 4;
    cfg.watermark_ratio  = 0.50; // threshold = 2
    cfg.async_flush      = true;
    cfg.flush_batch_size = 100;
    cfg.flush_interval   = std::chrono::milliseconds{5000}; // no auto-flush

    AdaptiveFlushController afc(tsstore.get(), cfg);
    afc.start();

    // Fill to watermark
    (void)afc.add(makePoint("p", "e1", 1.0, 1700000000000LL));
    (void)afc.add(makePoint("p", "e1", 2.0, 1700000000001LL));

    // Stop from another thread while a producer might be blocked
    std::thread stopper([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds{30});
        afc.stop();
    });

    // This add may hit backpressure; after stop() it must unblock
    auto r = afc.add(makePoint("p", "e1", 3.0, 1700000000002LL));
    EXPECT_TRUE(r.has_value() ||
                r.error().code() == errors::ErrorCode::ERR_API_RESOURCE_EXHAUSTED);

    stopper.join();
    // Must not deadlock; test completion is the success criterion
}

// ─────────────────────────────────────────────────────────────────────────────
// 13. Overdue flush alerting
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, OverdueFlushEventAlerted) {
    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.flush_interval          = std::chrono::milliseconds{30};
    cfg.overdue_flush_multiplier = 1;   // overdue if held > 30 ms (deterministic before first timeout flush)
    cfg.watermark_ratio          = 1.0; // no watermark-flush
    cfg.async_flush              = true;
    cfg.flush_batch_size         = 500;

    AdaptiveFlushController afc(tsstore.get(), cfg);
    afc.start();

    (void)afc.add(makePoint("od", "s1", 1.0, 1700000000000LL));

    // Wait longer than the overdue threshold (2 × 30 ms = 60 ms)
    std::this_thread::sleep_for(std::chrono::milliseconds{250});

    afc.stop();

    // The flush thread should have detected the overdue condition at least once
    EXPECT_GT(afc.getStats().overdue_flush_events, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 14. TimeSeriesMetrics integration
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, MetricsPointerWiredThroughConfig) {
    TimeSeriesMetrics metrics;
    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.metrics = &metrics;

    AdaptiveFlushController afc(tsstore.get(), cfg);
    EXPECT_EQ(afc.getConfig().metrics, &metrics);
}

TEST_F(AFCFixture, MetricsBackpressureCounterUpdated) {
    TimeSeriesMetrics metrics;

    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.buffer_capacity  = 4;
    cfg.watermark_ratio  = 0.50; // threshold = 2
    cfg.async_flush      = true;
    cfg.flush_interval   = std::chrono::milliseconds{5000};
    cfg.flush_batch_size = 100;
    cfg.metrics          = &metrics;

    AdaptiveFlushController afc(tsstore.get(), cfg);
    afc.start();

    // Fill to watermark
    (void)afc.add(makePoint("bp_m", "s1", 1.0, 1700000000000LL));
    (void)afc.add(makePoint("bp_m", "s1", 2.0, 1700000000001LL));

    // This add should trigger backpressure
    std::thread flusher([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds{20});
        (void)afc.flush();
    });

    (void)afc.add(makePoint("bp_m", "s1", 3.0, 1700000000002LL));
    flusher.join();
    afc.stop();

    EXPECT_GE(metrics.getTotalBackpressureEvents(), 1u);
}

TEST_F(AFCFixture, MetricsOverdueFlushCounterUpdated) {
    TimeSeriesMetrics metrics;

    AdaptiveFlushControllerConfig cfg = quietConfig();
    cfg.flush_interval           = std::chrono::milliseconds{20};
    cfg.overdue_flush_multiplier = 1;
    cfg.watermark_ratio          = 1.0;
    cfg.async_flush              = true;
    cfg.flush_batch_size         = 500;
    cfg.metrics                  = &metrics;

    AdaptiveFlushController afc(tsstore.get(), cfg);
    afc.start();

    (void)afc.add(makePoint("od_m", "s1", 1.0, 1700000000000LL));

    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    afc.stop();

    EXPECT_GT(metrics.getTotalOverdueFlushEvents(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 15. addBatch() stats check
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AFCFixture, AddBatchAndFlushUpdatesAllStats) {
    AdaptiveFlushController afc(tsstore.get(), quietConfig());

    std::vector<TSStore::DataPoint> batch = {};

    for (int i = 0; i < 15; ++i) {
        batch.push_back(makePoint("batch", "s1", static_cast<double>(i),
                                  1700000000000LL + i));
    }

    auto r = afc.addBatch(batch);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 15u);

    size_t flushed = afc.flush();
    EXPECT_EQ(flushed, 15u);

    auto s = afc.getStats();
    EXPECT_EQ(s.points_buffered, 15u);
    EXPECT_EQ(s.points_flushed, 15u);
    EXPECT_EQ(s.flush_count, 1u);
    EXPECT_EQ(s.current_buffer_size, 0u);
    EXPECT_DOUBLE_EQ(s.buffer_utilization, 0.0);
}

}  // anonymous namespace
}  // namespace themis
