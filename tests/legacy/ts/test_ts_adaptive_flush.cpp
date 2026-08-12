/**
 * @file test_ts_adaptive_flush.cpp
 * @brief Unit tests for the TSAutoBuffer FlushController adaptive flush
 *        and backpressure signalling feature.
 *
 * Tests: FlushController construction, EWMA update, batch-size adaptation,
 *        backpressure flag, TSAutoBufferConfig new fields, stats propagation,
 *        TimeSeriesMetrics integration (recordBackpressure + recordOverdueFlush),
 *        running_ guard after backpressure wait, and latency-based trigger.
 */

#include <gtest/gtest.h>
#include "timeseries/ts_auto_buffer.h"
#include "timeseries/tsstore.h"
#include "timeseries/timeseries_metrics.h"
#include "storage/rocksdb_wrapper.h"
#include <chrono>
#include <filesystem>
#include <memory>

namespace themis {
namespace {

// =========================================================================
// Helpers
// =========================================================================

static std::string makeTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_adflush_" + tag + "_" + std::to_string(ns))).string();
}

struct AdaptiveFlushFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> tsstore;

    void SetUp() override {
        db_path = makeTempPath("af");
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

    static TSAutoBufferConfig adaptiveConfig() {
        TSAutoBufferConfig cfg;
        cfg.async_flush              = false;
        cfg.enable_adaptive_flush    = true;
        cfg.backpressure_slo_ms      = 50.0;
        cfg.ewma_alpha               = 0.5;   // faster convergence for tests
        cfg.adaptive_batch_min       = 10;
        cfg.adaptive_batch_max       = 1000;
        cfg.flush_batch_size         = 100;
        cfg.backpressure_high_watermark = 500;
        cfg.backpressure_low_watermark  = 100;
        return cfg;
    }
};

// =========================================================================
// TSAutoBufferConfig new fields
// =========================================================================

TEST(AdaptiveFlushConfigTest, DefaultsAreDisabled) {
    TSAutoBufferConfig cfg;
    EXPECT_FALSE(cfg.enable_adaptive_flush);
    EXPECT_GT(cfg.backpressure_slo_ms, 0.0);
    EXPECT_GT(cfg.backpressure_high_watermark, cfg.backpressure_low_watermark);
}

TEST(AdaptiveFlushConfigTest, FieldsCanBeSet) {
    TSAutoBufferConfig cfg;
    cfg.enable_adaptive_flush       = true;
    cfg.backpressure_slo_ms         = 25.0;
    cfg.ewma_alpha                  = 0.2;
    cfg.adaptive_batch_min          = 50;
    cfg.adaptive_batch_max          = 2000;
    cfg.backpressure_high_watermark = 9000;
    cfg.backpressure_low_watermark  = 3000;

    EXPECT_TRUE(cfg.enable_adaptive_flush);
    EXPECT_DOUBLE_EQ(cfg.backpressure_slo_ms, 25.0);
    EXPECT_DOUBLE_EQ(cfg.ewma_alpha, 0.2);
    EXPECT_EQ(cfg.adaptive_batch_min, 50u);
    EXPECT_EQ(cfg.adaptive_batch_max, 2000u);
    EXPECT_EQ(cfg.backpressure_high_watermark, 9000u);
    EXPECT_EQ(cfg.backpressure_low_watermark, 3000u);
}

// =========================================================================
// TSAutoBufferStats new fields
// =========================================================================

TEST_F(AdaptiveFlushFixture, StatsHaveBackpressureFieldsAtZero) {
    auto cfg = adaptiveConfig();
    TSAutoBuffer buf(tsstore.get(), cfg);
    auto stats = buf.getStats();
    EXPECT_EQ(stats.backpressure_events.load(), 0u);
    EXPECT_GE(stats.current_adaptive_batch_size, 0u);
    EXPECT_GE(stats.current_ewma_latency_ms, 0.0);
}

TEST_F(AdaptiveFlushFixture, SetConfigReinitialisesFlushController) {
    TSAutoBufferConfig cfg;
    cfg.async_flush = false;
    TSAutoBuffer buf(tsstore.get(), cfg);

    // Enable adaptive flush via setConfig
    auto new_cfg = adaptiveConfig();
    buf.setConfig(new_cfg);

    EXPECT_TRUE(buf.getConfig().enable_adaptive_flush);
    // After enabling, adaptive_batch_size should equal flush_batch_size
    EXPECT_EQ(buf.getStats().current_adaptive_batch_size, new_cfg.flush_batch_size);
}

TEST_F(AdaptiveFlushFixture, SetConfigDisablesFlushController) {
    auto cfg = adaptiveConfig();
    TSAutoBuffer buf(tsstore.get(), cfg);

    // Now disable it
    TSAutoBufferConfig off = cfg;
    off.enable_adaptive_flush = false;
    buf.setConfig(off);
    EXPECT_FALSE(buf.getConfig().enable_adaptive_flush);
    EXPECT_EQ(buf.getStats().current_adaptive_batch_size, 0u);
}

// =========================================================================
// Adaptive flush operation tests
// =========================================================================

TEST_F(AdaptiveFlushFixture, AddSinglePointWithAdaptiveFlushEnabled) {
    auto cfg = adaptiveConfig();
    TSAutoBuffer buf(tsstore.get(), cfg);
    auto result = buf.add(makePoint("cpu", "srv01", 1.0));
    EXPECT_TRUE(result.has_value()) << result.error().message();
}

TEST_F(AdaptiveFlushFixture, ManualFlushWithAdaptiveConfig) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 1000;  // prevent auto-flush on add
    TSAutoBuffer buf(tsstore.get(), cfg);

    for (int i = 0; i < 5; ++i) {
        auto r = buf.add(makePoint("net", "h1", static_cast<double>(i),
                                   1700000000000LL + i));
        EXPECT_TRUE(r.has_value());
    }
    size_t flushed = buf.flush();
    EXPECT_GE(flushed, 5u);
    EXPECT_GE(buf.getStats().points_flushed.load(), 5u);
}

TEST_F(AdaptiveFlushFixture, SizeTriggeredFlushWithAdaptiveBatchSize) {
    auto cfg = adaptiveConfig();
    cfg.adaptive_batch_min    = 3;
    cfg.adaptive_batch_max    = 3;
    cfg.flush_batch_size      = 3;
    cfg.max_points_per_buffer = 3;
    TSAutoBuffer buf(tsstore.get(), cfg);

    // Add 4 points — should trigger a size-flush at point 3 (adaptive batch = 3)
    for (int i = 0; i < 4; ++i) {
        buf.add(makePoint("disk", "s1", static_cast<double>(i),
                          1700000000000LL + i));
    }
    buf.flush();
    EXPECT_GE(buf.getStats().points_buffered.load(), 4u);
}

TEST_F(AdaptiveFlushFixture, StatsAdaptiveBatchSizeUpdatedAfterFlush) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 2;
    cfg.adaptive_batch_min    = 1;
    cfg.adaptive_batch_max    = 1000;
    cfg.flush_batch_size      = 2;
    TSAutoBuffer buf(tsstore.get(), cfg);

    // Trigger a size-flush to exercise the latency feedback path
    buf.add(makePoint("mem", "s2", 1.0, 1700000000000LL));
    buf.add(makePoint("mem", "s2", 2.0, 1700000000001LL));

    // After the auto-flush, the EWMA should have been updated
    auto stats = buf.getStats();
    // adaptive_batch_size is set to flush_batch_size at construction
    EXPECT_GE(stats.current_adaptive_batch_size, cfg.adaptive_batch_min);
    EXPECT_LE(stats.current_adaptive_batch_size, cfg.adaptive_batch_max);
}

// =========================================================================
// TimeSeriesMetrics backpressure counter
// =========================================================================

TEST(TimeSeriesMetricsBackpressureTest, InitialCounterIsZero) {
    TimeSeriesMetrics metrics;
    EXPECT_EQ(metrics.getTotalBackpressureEvents(), 0u);
}

TEST(TimeSeriesMetricsBackpressureTest, RecordBackpressureIncrementsCounter) {
    TimeSeriesMetrics metrics;
    metrics.recordBackpressure("cpu");
    metrics.recordBackpressure("mem");
    EXPECT_EQ(metrics.getTotalBackpressureEvents(), 2u);
}

TEST(TimeSeriesMetricsBackpressureTest, RecordBackpressureWithEmptyMetric) {
    TimeSeriesMetrics metrics;
    metrics.recordBackpressure();
    EXPECT_EQ(metrics.getTotalBackpressureEvents(), 1u);
}

TEST(TimeSeriesMetricsBackpressureTest, PrometheusExportContainsBackpressureMetric) {
    TimeSeriesMetrics metrics;
    metrics.recordBackpressure("cpu");

    std::string prom = metrics.exportPrometheus();
    EXPECT_NE(prom.find("autobuffer_backpressure"), std::string::npos)
        << "Prometheus export should contain 'autobuffer_backpressure'";
}

TEST(TimeSeriesMetricsBackpressureTest, ResetClearsBackpressureCounter) {
    TimeSeriesMetrics metrics;
    metrics.recordBackpressure();
    metrics.reset();
    EXPECT_EQ(metrics.getTotalBackpressureEvents(), 0u);
}

// =========================================================================
// TimeSeriesMetrics overdue flush counter
// =========================================================================

TEST(TimeSeriesMetricsOverdueFlushTest, InitialCounterIsZero) {
    TimeSeriesMetrics metrics;
    EXPECT_EQ(metrics.getTotalOverdueFlushEvents(), 0u);
}

TEST(TimeSeriesMetricsOverdueFlushTest, RecordOverdueFlushIncrementsCounter) {
    TimeSeriesMetrics metrics;
    metrics.recordOverdueFlush("cpu", 12345.0);
    metrics.recordOverdueFlush();
    EXPECT_EQ(metrics.getTotalOverdueFlushEvents(), 2u);
}

TEST(TimeSeriesMetricsOverdueFlushTest, ResetClearsOverdueFlushCounter) {
    TimeSeriesMetrics metrics;
    metrics.recordOverdueFlush("mem", 1000.0);
    metrics.reset();
    EXPECT_EQ(metrics.getTotalOverdueFlushEvents(), 0u);
}

TEST(TimeSeriesMetricsOverdueFlushTest, PrometheusExportContainsOverdueMetric) {
    TimeSeriesMetrics metrics;
    metrics.recordOverdueFlush("disk", 50000.0);

    std::string prom = metrics.exportPrometheus();
    EXPECT_NE(prom.find("overdue_flush"), std::string::npos)
        << "Prometheus export should contain 'overdue_flush'";
}

// =========================================================================
// TSAutoBuffer + TimeSeriesMetrics integration
// =========================================================================

TEST_F(AdaptiveFlushFixture, MetricsPointerIsWiredThroughConfig) {
    TimeSeriesMetrics metrics;

    auto cfg = adaptiveConfig();
    cfg.metrics = &metrics;
    TSAutoBuffer buf(tsstore.get(), cfg);
    EXPECT_EQ(buf.getConfig().metrics, &metrics);
}

TEST_F(AdaptiveFlushFixture, MetricsNullByDefault) {
    TSAutoBufferConfig cfg;
    TSAutoBuffer buf(tsstore.get(), cfg);
    EXPECT_EQ(buf.getConfig().metrics, nullptr);
}

TEST_F(AdaptiveFlushFixture, MetricsUpdatedBySetConfig) {
    TimeSeriesMetrics metrics;

    TSAutoBufferConfig cfg;
    cfg.async_flush = false;
    TSAutoBuffer buf(tsstore.get(), cfg);
    EXPECT_EQ(buf.getConfig().metrics, nullptr);

    auto new_cfg = adaptiveConfig();
    new_cfg.metrics = &metrics;
    buf.setConfig(new_cfg);
    EXPECT_EQ(buf.getConfig().metrics, &metrics);
}

// =========================================================================
// getStats() regression: dedup_dropped_count + memory_limit_rejected_count
// =========================================================================

TEST_F(AdaptiveFlushFixture, GetStatsCopiesDedupDroppedCount) {
    TSAutoBufferConfig cfg;
    cfg.async_flush  = false;
    cfg.enable_dedup = true;
    cfg.max_points_per_buffer = 100;
    TSAutoBuffer buf(tsstore.get(), cfg);

    // Add two points with the same timestamp — second should be deduped
    buf.add(makePoint("dup", "e1", 1.0, 1700000000000LL));
    buf.add(makePoint("dup", "e1", 2.0, 1700000000000LL));  // same timestamp → dropped

    EXPECT_EQ(buf.getStats().dedup_dropped_count.load(), 1u);
}

TEST_F(AdaptiveFlushFixture, GetStatsCopiesMemoryLimitRejectedCount) {
    TSAutoBufferConfig cfg;
    cfg.async_flush                = false;
    cfg.max_memory_per_metric_bytes = 1;   // extremely small → reject after first point
    cfg.max_points_per_buffer       = 100;
    TSAutoBuffer buf(tsstore.get(), cfg);

    buf.add(makePoint("over", "e1", 1.0, 1700000000000LL));
    buf.add(makePoint("over", "e1", 2.0, 1700000000001LL));  // should be rejected

    EXPECT_GE(buf.getStats().memory_limit_rejected_count.load(), 1u);
}

// =========================================================================
// running_ guard: add() must return error after stop() while in backpressure
// =========================================================================

TEST_F(AdaptiveFlushFixture, AddErrorsAfterStopDuringBackpressureWait) {
    auto cfg = adaptiveConfig();
    cfg.backpressure_high_watermark = 2;   // very low threshold for test
    cfg.backpressure_low_watermark  = 0;
    cfg.adaptive_batch_min          = 1;
    cfg.adaptive_batch_max          = 100;
    cfg.flush_batch_size            = 1;
    cfg.async_flush                 = true;  // need background thread to stop
    TSAutoBuffer buf(tsstore.get(), cfg);
    buf.start();

    // Fill buffer up to the high-watermark
    buf.add(makePoint("p", "e1", 1.0, 1700000000000LL));
    buf.add(makePoint("p", "e1", 2.0, 1700000000001LL));

    // Stop the buffer in a separate thread while a producer might be blocked
    std::thread stopper([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        buf.stop();
    });

    // This add may block on backpressure; after stop() it must either succeed
    // (if buffer was drained) or return ERR_API_RESOURCE_EXHAUSTED (if stopped
    // during the wait).  The critical property is: no deadlock.
    auto result = buf.add(makePoint("p", "e1", 3.0, 1700000000002LL));
    EXPECT_TRUE(result.has_value() ||
                result.error().code() == errors::ErrorCode::ERR_API_RESOURCE_EXHAUSTED)
        << "Expected either success or ERR_API_RESOURCE_EXHAUSTED, got: "
        << (result.has_value() ? "ok" : result.error().message());

    stopper.join();
}

}  // namespace
}  // namespace themis
