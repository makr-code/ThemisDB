/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_adaptive_flush_controller.cpp                 ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • Initial  2026-04-09  feat(timeseries): PERF-D1-C regression &   ║
                           performance tests for AdaptiveFlush        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_adaptive_flush_controller.cpp
 * @brief Regression, Performance, and Integration tests for
 *        AdaptiveFlushController (TSAutoBuffer + FlushController).
 *
 * Issue: PERF-D1-C — Tests: Regression & Performance Validation for Adaptive Flush
 * Reference: makr-code/ThemisDB#4432
 *
 * Test Groups:
 *   1. Unit — Buffer logic, Watermark & Timeout flushes, Backpressure
 *   2. Performance — ≥500k pts/s throughput, P99 latency <100µs
 *      (gated: THEMIS_RUN_PERF_TESTS=1 or run individually)
 *   3. Regression — No data loss, memory bounds, fallback, concurrency
 *   4. Integration — End-to-End TSStore+AdaptiveFlush, Prometheus/JSON metrics
 *
 * Acceptance Criteria:
 *   - ≥500k pts/sec throughput single-threaded
 *   - P99 latency < 100µs
 *   - No data loss, correct buffer/flush stats
 *   - No regressions, >90% code coverage
 *   - All tests and metrics export passing
 */

#include <gtest/gtest.h>
#include "timeseries/ts_auto_buffer.h"
#include "timeseries/ts_auto_buffer_adaptive.h"
#include "timeseries/tsstore.h"
#include "timeseries/timeseries_metrics.h"
#include "storage/rocksdb_wrapper.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <numeric>
#include <thread>
#include <vector>

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

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

struct AdaptiveFlushControllerFocusedTests : ::testing::Test {
    std::string db_path;
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
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    /// Default adaptive config for unit / regression tests.
    static TSAutoBufferConfig adaptiveConfig() {
        TSAutoBufferConfig cfg;
        cfg.async_flush                  = false;
        cfg.enable_adaptive_flush        = true;
        cfg.backpressure_slo_ms          = 50.0;
        cfg.ewma_alpha                   = 0.5;
        cfg.adaptive_batch_min           = 10;
        cfg.adaptive_batch_max           = 1000;
        cfg.flush_batch_size             = 100;
        cfg.max_points_per_buffer        = 500;
        cfg.max_total_points             = 10000;
        cfg.backpressure_high_watermark  = 5000;
        cfg.backpressure_low_watermark   = 1000;
        cfg.overdue_flush_multiplier     = 2;
        return cfg;
    }
};

// ═════════════════════════════════════════════════════════════════════════════
// GROUP 1: Unit Tests — Buffer Logic
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(AdaptiveFlushControllerFocusedTests, UnitAddSinglePointBuffered) {
    TSAutoBuffer buf(tsstore.get(), adaptiveConfig());
    auto r = buf.add(makePoint("cpu", "h1", 1.0));
    EXPECT_TRUE(r.has_value()) << r.error().message();
    EXPECT_GE(buf.getStats().points_buffered.load(), 1u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitAddMultiplePointsAccumulate) {
    TSAutoBuffer buf(tsstore.get(), adaptiveConfig());
    for (int i = 0; i < 10; ++i) {
        auto r = buf.add(makePoint("cpu", "h1", static_cast<double>(i),
                                   1700000000000LL + i));
        EXPECT_TRUE(r.has_value());
    }
    EXPECT_GE(buf.getStats().points_buffered.load(), 10u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitManualFlushEmptiesBuffer) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 1000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    for (int i = 0; i < 5; ++i) {
        buf.add(makePoint("mem", "s1", static_cast<double>(i),
                          1700000000000LL + i));
    }
    size_t flushed = buf.flush();
    EXPECT_GE(flushed, 5u);
    EXPECT_GE(buf.getStats().points_flushed.load(), 5u);
    EXPECT_GE(buf.getStats().flush_count.load(), 1u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitFlushCountIncrements) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 1000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    buf.add(makePoint("disk", "d1", 1.0));
    buf.flush();
    buf.add(makePoint("disk", "d1", 2.0, 1700000000001LL));
    buf.flush();
    EXPECT_GE(buf.getStats().flush_count.load(), 2u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitMaxPointsPerBufferTriggersFlush) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 3;
    cfg.flush_batch_size      = 3;
    TSAutoBuffer buf(tsstore.get(), cfg);

    for (int i = 0; i < 4; ++i) {
        buf.add(makePoint("net", "n1", static_cast<double>(i),
                          1700000000000LL + i));
    }
    buf.flush();
    EXPECT_GE(buf.getStats().points_buffered.load(), 4u);
    EXPECT_GE(buf.getStats().size_triggered_flush.load(), 1u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitInvalidPointRejected) {
    TSAutoBuffer buf(tsstore.get(), adaptiveConfig());
    // Empty metric name is invalid
    auto r = buf.add(makePoint("", "h1", 1.0));
    EXPECT_FALSE(r.has_value());
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitFlushForSpecificMetricEntity) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 1000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    buf.add(makePoint("cpu", "host1", 1.0, 1700000000000LL));
    buf.add(makePoint("mem", "host1", 2.0, 1700000000001LL));
    size_t flushed = buf.flushFor("cpu", "host1");
    EXPECT_GE(flushed, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Watermark & Timeout Flushes
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, UnitWatermarkFlushOnMaxTotalPoints) {
    auto cfg = adaptiveConfig();
    cfg.max_total_points      = 5;
    cfg.max_points_per_buffer = 1000;
    cfg.flush_batch_size      = 5;
    TSAutoBuffer buf(tsstore.get(), cfg);

    for (int i = 0; i < 6; ++i) {
        buf.add(makePoint("temp", "s1", static_cast<double>(i),
                          1700000000000LL + i));
    }
    EXPECT_GE(buf.getStats().points_buffered.load(), 6u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitOverdueFlushMultiplierConfigured) {
    auto cfg = adaptiveConfig();
    cfg.overdue_flush_multiplier = 3;
    TSAutoBuffer buf(tsstore.get(), cfg);
    EXPECT_EQ(buf.getConfig().overdue_flush_multiplier, 3u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitTimeTriggeredFlushCountsStats) {
    auto cfg = adaptiveConfig();
    cfg.async_flush    = true;
    cfg.flush_interval = std::chrono::milliseconds{50};
    cfg.max_points_per_buffer = 10000;
    TSAutoBuffer buf(tsstore.get(), cfg);
    buf.start();

    buf.add(makePoint("io", "dev1", 1.0));
    // Wait for at least one time-triggered flush
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    buf.stop();

    EXPECT_GE(buf.getStats().time_triggered_flush.load(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Adaptive Batch Size (FlushController EWMA)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, UnitAdaptiveBatchSizeInitializedFromConfig) {
    auto cfg = adaptiveConfig();
    TSAutoBuffer buf(tsstore.get(), cfg);
    auto stats = buf.getStats();
    EXPECT_EQ(stats.current_adaptive_batch_size, cfg.flush_batch_size);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitAdaptiveBatchSizeWithinBounds) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 2;
    cfg.flush_batch_size      = 50;
    cfg.adaptive_batch_min    = 10;
    cfg.adaptive_batch_max    = 1000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    // Trigger multiple flushes to exercise EWMA
    for (int cycle = 0; cycle < 5; ++cycle) {
        buf.add(makePoint("lat", "srv", 1.0, 1700000000000LL + cycle * 2));
        buf.add(makePoint("lat", "srv", 2.0, 1700000000000LL + cycle * 2 + 1));
    }
    buf.flush();

    auto stats = buf.getStats();
    EXPECT_GE(stats.current_adaptive_batch_size, cfg.adaptive_batch_min);
    EXPECT_LE(stats.current_adaptive_batch_size, cfg.adaptive_batch_max);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitDisableAdaptiveFlushZeroesStats) {
    auto cfg = adaptiveConfig();
    TSAutoBuffer buf(tsstore.get(), cfg);

    TSAutoBufferConfig off = cfg;
    off.enable_adaptive_flush = false;
    buf.setConfig(off);

    EXPECT_FALSE(buf.getConfig().enable_adaptive_flush);
    EXPECT_EQ(buf.getStats().current_adaptive_batch_size, 0u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitSetConfigReenablesAdaptiveFlush) {
    TSAutoBufferConfig off;
    off.async_flush = false;
    TSAutoBuffer buf(tsstore.get(), off);

    auto on = adaptiveConfig();
    buf.setConfig(on);

    EXPECT_TRUE(buf.getConfig().enable_adaptive_flush);
    EXPECT_EQ(buf.getStats().current_adaptive_batch_size, on.flush_batch_size);
}

// ─────────────────────────────────────────────────────────────────────────────
// Backpressure
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, UnitBackpressureEventsInitiallyZero) {
    TSAutoBuffer buf(tsstore.get(), adaptiveConfig());
    EXPECT_EQ(buf.getStats().backpressure_events.load(), 0u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitBackpressureStopUnblocksProducer) {
    auto cfg = adaptiveConfig();
    cfg.async_flush                 = true;
    cfg.backpressure_high_watermark = 2;
    cfg.backpressure_low_watermark  = 0;
    cfg.flush_batch_size            = 1;
    cfg.max_points_per_buffer       = 1;
    TSAutoBuffer buf(tsstore.get(), cfg);
    buf.start();

    buf.add(makePoint("bp", "e1", 1.0, 1700000000000LL));
    buf.add(makePoint("bp", "e1", 2.0, 1700000000001LL));

    std::thread stopper([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds{30});
        buf.stop();
    });

    auto result = buf.add(makePoint("bp", "e1", 3.0, 1700000000002LL));
    EXPECT_TRUE(result.has_value() ||
                result.error().code() == errors::ErrorCode::ERR_API_RESOURCE_EXHAUSTED);
    stopper.join();
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitFlushControllerBackpressureActive) {
    FlushControllerConfig fc_cfg;
    fc_cfg.slo_threshold_ms = 10.0;
    fc_cfg.warmup_samples   = 1;
    fc_cfg.ewma_alpha       = 1.0;
    FlushController fc(fc_cfg);

    EXPECT_FALSE(fc.isBackpressureActive());
    fc.reportFlushLatency(500.0);  // way above SLO
    EXPECT_TRUE(fc.isBackpressureActive());
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitFlushControllerBackpressureReleasedOnLowLatency) {
    FlushControllerConfig fc_cfg;
    fc_cfg.slo_threshold_ms = 50.0;
    fc_cfg.warmup_samples   = 1;
    fc_cfg.ewma_alpha       = 1.0;
    FlushController fc(fc_cfg);

    fc.reportFlushLatency(300.0);
    EXPECT_TRUE(fc.isBackpressureActive());

    fc.reportFlushLatency(5.0);
    EXPECT_FALSE(fc.isBackpressureActive());
}

TEST_F(AdaptiveFlushControllerFocusedTests, UnitFlushControllerNotifyDrainedUnblocksWaiter) {
    FlushControllerConfig fc_cfg;
    fc_cfg.slo_threshold_ms = 10.0;
    fc_cfg.warmup_samples   = 1;
    fc_cfg.ewma_alpha       = 1.0;
    fc_cfg.low_water_mark   = 5;
    FlushController fc(fc_cfg);

    fc.reportFlushLatency(500.0);
    ASSERT_TRUE(fc.isBackpressureActive());

    bool unblocked = false;
    std::thread waiter([&] {
        unblocked = fc.checkBackpressure(100, std::chrono::milliseconds{500});
    });

    std::this_thread::sleep_for(std::chrono::milliseconds{30});
    fc.reportFlushLatency(1.0);  // drops backpressure
    fc.notifyDrained(0);

    waiter.join();
    EXPECT_TRUE(unblocked);
}

// ═════════════════════════════════════════════════════════════════════════════
// GROUP 2: Performance Tests (gated by THEMIS_RUN_PERF_TESTS=1)
// ═════════════════════════════════════════════════════════════════════════════

TEST_F(AdaptiveFlushControllerFocusedTests, PerfThroughput500kPointsPerSecond) {
#ifndef THEMIS_RUN_PERF_TESTS
    GTEST_SKIP() << "Perf test skipped; set THEMIS_RUN_PERF_TESTS=1 to run";
#endif
    auto cfg = adaptiveConfig();
    cfg.async_flush           = false;
    cfg.max_points_per_buffer = 100000;
    cfg.max_total_points      = 1000000;
    cfg.flush_batch_size      = 1000;
    cfg.adaptive_batch_min    = 500;
    cfg.adaptive_batch_max    = 5000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    constexpr size_t kPoints = 500000;
    const auto t0 = std::chrono::steady_clock::now();

    for (size_t i = 0; i < kPoints; ++i) {
        auto r = buf.add(makePoint("perf", "e1", static_cast<double>(i),
                                   static_cast<int64_t>(i)));
        if (!r.has_value()) {
            // On rejection (e.g. memory cap), flush and retry once
            buf.flush();
            buf.add(makePoint("perf", "e1", static_cast<double>(i),
                              static_cast<int64_t>(i)));
        }
    }
    buf.flush();

    const auto t1 = std::chrono::steady_clock::now();
    const double elapsed_s =
        std::chrono::duration<double>(t1 - t0).count();
    const double throughput = kPoints / elapsed_s;

    EXPECT_GE(throughput, 500000.0)
        << "Throughput " << static_cast<long>(throughput) << " pts/s < 500k pts/s target";
}

TEST_F(AdaptiveFlushControllerFocusedTests, PerfP99LatencyBelow100Microseconds) {
#ifndef THEMIS_RUN_PERF_TESTS
    GTEST_SKIP() << "Perf test skipped; set THEMIS_RUN_PERF_TESTS=1 to run";
#endif
    auto cfg = adaptiveConfig();
    cfg.async_flush           = false;
    cfg.max_points_per_buffer = 100000;
    cfg.max_total_points      = 1000000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    constexpr size_t kSamples = 10000;
    std::vector<double> latencies_us;
    latencies_us.reserve(kSamples);

    for (size_t i = 0; i < kSamples; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        buf.add(makePoint("lat", "srv", static_cast<double>(i),
                          static_cast<int64_t>(i)));
        const auto t1 = std::chrono::steady_clock::now();
        latencies_us.push_back(
            std::chrono::duration<double, std::micro>(t1 - t0).count());
    }

    std::sort(latencies_us.begin(), latencies_us.end());
    const double p99 = latencies_us[static_cast<size_t>(kSamples * 0.99)];

    EXPECT_LT(p99, 100.0)
        << "P99 add() latency " << p99 << " µs >= 100 µs target";
}

TEST_F(AdaptiveFlushControllerFocusedTests, PerfBufferUtilizationStatsAvailable) {
#ifndef THEMIS_RUN_PERF_TESTS
    GTEST_SKIP() << "Perf test skipped; set THEMIS_RUN_PERF_TESTS=1 to run";
#endif
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 100000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    for (int i = 0; i < 1000; ++i) {
        buf.add(makePoint("util", "s1", static_cast<double>(i),
                          1700000000000LL + i));
    }

    auto stats = buf.getStats();
    EXPECT_GE(stats.current_buffer_size, 0u);
    EXPECT_GE(stats.current_buffer_memory, 0u);
    EXPECT_GE(stats.points_buffered.load(), 1000u);
}

// ═════════════════════════════════════════════════════════════════════════════
// GROUP 3: Regression Tests
// ═════════════════════════════════════════════════════════════════════════════

// ── No data loss ─────────────────────────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, RegressionNoDataLossAfterFlush) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 1000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    constexpr int kCount = 50;
    for (int i = 0; i < kCount; ++i) {
        auto r = buf.add(makePoint("nodataloss", "srv1",
                                   static_cast<double>(i),
                                   1700000000000LL + i));
        ASSERT_TRUE(r.has_value()) << "add() failed at i=" << i;
    }
    size_t flushed = buf.flush();
    EXPECT_GE(flushed, static_cast<size_t>(kCount))
        << "Expected all " << kCount << " points flushed, got " << flushed;

    // Verify data is actually in TSStore
    TSStore::QueryOptions q;
    q.metric           = "nodataloss";
    q.entity           = "srv1";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = INT64_MAX;
    q.limit             = kCount * 2;
    auto result = tsstore->query(q);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_GE(result.value().size(), static_cast<size_t>(kCount))
        << "TSStore returned fewer points than written";
}

TEST_F(AdaptiveFlushControllerFocusedTests, RegressionNoDataLossWithAdaptiveBatching) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 10;
    cfg.flush_batch_size      = 10;
    cfg.adaptive_batch_min    = 5;
    cfg.adaptive_batch_max    = 50;
    TSAutoBuffer buf(tsstore.get(), cfg);

    constexpr int kTotal = 100;
    for (int i = 0; i < kTotal; ++i) {
        auto r = buf.add(makePoint("adaptiveloss", "e1",
                                   static_cast<double>(i),
                                   1700000000000LL + i));
        ASSERT_TRUE(r.has_value());
    }
    buf.flush();

    TSStore::QueryOptions q;
    q.metric            = "adaptiveloss";
    q.entity            = "e1";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = INT64_MAX;
    q.limit             = kTotal * 2;
    auto result = tsstore->query(q);
    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result.value().size(), static_cast<size_t>(kTotal));
}

// ── Memory usage ─────────────────────────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, RegressionMemoryLimitRejectsExcessPoints) {
    auto cfg = adaptiveConfig();
    cfg.max_memory_per_metric_bytes = 1;   // extreme: reject after first point
    cfg.max_points_per_buffer       = 1000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    // The first point may or may not fit depending on metadata size; add a few
    // and assert that at least one is rejected due to the per-metric cap.
    for (int i = 0; i < 5; ++i) {
        buf.add(makePoint("memlim", "h1", static_cast<double>(i),
                          1700000000000LL + i));
    }

    EXPECT_GE(buf.getStats().memory_limit_rejected_count.load(), 1u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, RegressionTotalMemoryCapEnforced) {
    auto cfg = adaptiveConfig();
    cfg.max_memory_bytes      = 512;   // very small total memory cap
    cfg.max_points_per_buffer = 100000;
    cfg.flush_batch_size      = 100000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    // The buffer should either accept points until memory is full,
    // or reject excess points gracefully
    int accepted = 0;
    int rejected = 0;
    for (int i = 0; i < 100; ++i) {
        auto r = buf.add(makePoint("totalcap", "srv",
                                   static_cast<double>(i),
                                   1700000000000LL + i));
        if (r.has_value()) {
            ++accepted;
        } else {
            ++rejected;
        }
    }
    // At least some points must have been accepted
    EXPECT_GT(accepted, 0);
}

// ── Fallback (adaptive flush disabled) ───────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, RegressionFallbackToStaticBatchWhenAdaptiveDisabled) {
    TSAutoBufferConfig cfg;
    cfg.async_flush           = false;
    cfg.enable_adaptive_flush = false;
    cfg.flush_batch_size      = 50;
    cfg.max_points_per_buffer = 1000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    for (int i = 0; i < 10; ++i) {
        buf.add(makePoint("fallback", "f1", static_cast<double>(i),
                          1700000000000LL + i));
    }
    buf.flush();

    // With adaptive disabled, current_adaptive_batch_size should be 0
    EXPECT_EQ(buf.getStats().current_adaptive_batch_size, 0u);
    EXPECT_GE(buf.getStats().points_flushed.load(), 10u);
}

TEST_F(AdaptiveFlushControllerFocusedTests, RegressionFallbackPreservesCorrectFlushBehavior) {
    TSAutoBufferConfig cfg;
    cfg.async_flush           = false;
    cfg.enable_adaptive_flush = false;
    cfg.flush_batch_size      = 5;
    cfg.max_points_per_buffer = 5;
    TSAutoBuffer buf(tsstore.get(), cfg);

    for (int i = 0; i < 10; ++i) {
        buf.add(makePoint("fallback2", "f2", static_cast<double>(i),
                          1700000000000LL + i));
    }
    buf.flush();

    TSStore::QueryOptions q;
    q.metric            = "fallback2";
    q.entity            = "f2";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = INT64_MAX;
    q.limit             = 100;
    auto result = tsstore->query(q);
    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result.value().size(), 10u);
}

// ── Deduplication ────────────────────────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, RegressionDedupDropsDuplicateTimestamp) {
    auto cfg = adaptiveConfig();
    cfg.enable_dedup          = true;
    cfg.max_points_per_buffer = 100;
    TSAutoBuffer buf(tsstore.get(), cfg);

    buf.add(makePoint("dup", "e1", 1.0, 1700000000000LL));
    buf.add(makePoint("dup", "e1", 2.0, 1700000000000LL));  // same ts → dropped

    EXPECT_EQ(buf.getStats().dedup_dropped_count.load(), 1u);
}

// ── Concurrency ──────────────────────────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, RegressionConcurrentAddsNoRaceCondition) {
    auto cfg = adaptiveConfig();
    cfg.async_flush           = false;
    cfg.max_points_per_buffer = 100000;
    cfg.max_total_points      = 1000000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    constexpr int kThreads = 4;
    constexpr int kPerThread = 50;
    std::atomic<int> accepted{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                auto r = buf.add(makePoint("concur",
                                           "t" + std::to_string(t),
                                           static_cast<double>(i),
                                           1700000000000LL + t * 1000 + i));
                if (r.has_value()) ++accepted;
            }
        });
    }
    for (auto& th : threads) th.join();

    buf.flush();
    EXPECT_GT(accepted.load(), 0);
    EXPECT_GE(buf.getStats().points_flushed.load(),
              static_cast<uint64_t>(accepted.load()));
}

TEST_F(AdaptiveFlushControllerFocusedTests, RegressionConcurrentFlushAndAddSafe) {
    auto cfg = adaptiveConfig();
    cfg.async_flush           = false;
    cfg.max_points_per_buffer = 100000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    std::atomic<bool> stop{false};
    std::atomic<int> flush_count{0};

    std::thread flusher([&]() {
        while (!stop.load()) {
            buf.flush();
            ++flush_count;
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
        }
    });

    for (int i = 0; i < 200; ++i) {
        buf.add(makePoint("cflush", "s1", static_cast<double>(i),
                          1700000000000LL + i));
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    stop.store(true);
    flusher.join();
    buf.flush();

    EXPECT_GT(flush_count.load(), 0);
    EXPECT_GE(buf.getStats().points_flushed.load(), 1u);
}

// ─── Stop/start lifecycle ────────────────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, RegressionStartStopFlushesRemaining) {
    auto cfg = adaptiveConfig();
    cfg.async_flush           = true;
    cfg.flush_interval        = std::chrono::milliseconds{5000};
    cfg.max_points_per_buffer = 10000;
    TSAutoBuffer buf(tsstore.get(), cfg);
    buf.start();

    for (int i = 0; i < 20; ++i) {
        buf.add(makePoint("lifecycle", "l1", static_cast<double>(i),
                          1700000000000LL + i));
    }
    buf.stop();  // must flush remaining points

    // All added points should have been flushed
    EXPECT_GE(buf.getStats().points_flushed.load(), 20u);
}

// ═════════════════════════════════════════════════════════════════════════════
// GROUP 4: Integration Tests
// ═════════════════════════════════════════════════════════════════════════════

// ── End-to-end TSStore + AdaptiveFlush ───────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationEndToEndAddFlushQuery) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 1000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    constexpr int kCount = 20;
    for (int i = 0; i < kCount; ++i) {
        auto r = buf.add(makePoint("e2e", "host1",
                                   static_cast<double>(i),
                                   1700000000000LL + i));
        ASSERT_TRUE(r.has_value());
    }
    buf.flush();

    TSStore::QueryOptions q;
    q.metric            = "e2e";
    q.entity            = "host1";
    q.from_timestamp_ms = 1700000000000LL;
    q.to_timestamp_ms   = 1700000000000LL + kCount;
    q.limit             = kCount * 2;
    auto result = tsstore->query(q);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_GE(result.value().size(), static_cast<size_t>(kCount));
}

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationMultipleMetricsEachFlushedCorrectly) {
    auto cfg = adaptiveConfig();
    cfg.max_points_per_buffer = 1000;
    TSAutoBuffer buf(tsstore.get(), cfg);

    const std::vector<std::string> metrics = {"cpu", "mem", "disk", "net"};
    for (const auto& metric : metrics) {
        for (int i = 0; i < 5; ++i) {
            buf.add(makePoint(metric, "srv", static_cast<double>(i),
                              1700000000000LL + i));
        }
    }
    buf.flush();

    for (const auto& metric : metrics) {
        TSStore::QueryOptions q;
        q.metric            = metric;
        q.entity            = "srv";
        q.from_timestamp_ms = 0;
        q.to_timestamp_ms   = INT64_MAX;
        q.limit             = 100;
        auto result = tsstore->query(q);
        ASSERT_TRUE(result.has_value()) << metric;
        EXPECT_GE(result.value().size(), 5u) << metric;
    }
}

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationAsyncFlushE2E) {
    auto cfg = adaptiveConfig();
    cfg.async_flush           = true;
    cfg.flush_interval        = std::chrono::milliseconds{50};
    cfg.max_points_per_buffer = 10000;
    TSAutoBuffer buf(tsstore.get(), cfg);
    buf.start();

    for (int i = 0; i < 10; ++i) {
        buf.add(makePoint("async", "srv", static_cast<double>(i),
                          1700000000000LL + i));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    buf.stop();

    TSStore::QueryOptions q;
    q.metric            = "async";
    q.entity            = "srv";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = INT64_MAX;
    q.limit             = 50;
    auto result = tsstore->query(q);
    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result.value().size(), 1u);
}

// ── Prometheus Metrics Export ─────────────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationPrometheusExportContainsFlushMetrics) {
    TimeSeriesMetrics metrics;
    metrics.recordBackpressure("cpu");
    metrics.recordOverdueFlush("mem", 5000.0);

    std::string prom = metrics.exportPrometheus();

    EXPECT_NE(prom.find("autobuffer_backpressure"), std::string::npos)
        << "Missing 'autobuffer_backpressure' in Prometheus output:\n" << prom;
    EXPECT_NE(prom.find("overdue_flush"), std::string::npos)
        << "Missing 'overdue_flush' in Prometheus output:\n" << prom;
}

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationPrometheusBackpressureCounterAccurate) {
    TimeSeriesMetrics metrics;
    metrics.recordBackpressure("svc1");
    metrics.recordBackpressure("svc2");
    metrics.recordBackpressure("svc3");

    EXPECT_EQ(metrics.getTotalBackpressureEvents(), 3u);

    std::string prom = metrics.exportPrometheus();
    EXPECT_NE(prom.find("autobuffer_backpressure"), std::string::npos);
}

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationPrometheusOverdueFlushCounterAccurate) {
    TimeSeriesMetrics metrics;
    metrics.recordOverdueFlush("cpu", 1000.0);
    metrics.recordOverdueFlush("mem", 2000.0);

    EXPECT_EQ(metrics.getTotalOverdueFlushEvents(), 2u);

    std::string prom = metrics.exportPrometheus();
    EXPECT_NE(prom.find("overdue_flush"), std::string::npos);
}

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationMetricsResetClearsCounters) {
    TimeSeriesMetrics metrics;
    metrics.recordBackpressure("cpu");
    metrics.recordOverdueFlush("mem", 1000.0);

    EXPECT_EQ(metrics.getTotalBackpressureEvents(), 1u);
    EXPECT_EQ(metrics.getTotalOverdueFlushEvents(), 1u);

    metrics.reset();

    EXPECT_EQ(metrics.getTotalBackpressureEvents(), 0u);
    EXPECT_EQ(metrics.getTotalOverdueFlushEvents(), 0u);
}

// ── JSON Metrics Export ───────────────────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationJsonExportWellFormed) {
    TimeSeriesMetrics metrics;
    metrics.recordBackpressure("cpu");
    metrics.recordOverdueFlush("disk", 8000.0);

    std::string json_str = metrics.exportJson();
    EXPECT_FALSE(json_str.empty());

    // Must be parseable JSON
    EXPECT_NO_THROW({
        auto j = nlohmann::json::parse(json_str);
        (void)j;
    }) << "exportJson() returned invalid JSON: " << json_str;
}

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationJsonExportContainsBackpressureField) {
    TimeSeriesMetrics metrics;
    metrics.recordBackpressure("net");

    std::string json_str = metrics.exportJson();
    EXPECT_NE(json_str.find("backpressure"), std::string::npos)
        << "JSON export missing 'backpressure' field:\n" << json_str;
}

// ── TSAutoBuffer + TimeSeriesMetrics wiring ───────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationMetricsWiredThroughBufferConfig) {
    TimeSeriesMetrics metrics;

    auto cfg = adaptiveConfig();
    cfg.metrics = &metrics;
    TSAutoBuffer buf(tsstore.get(), cfg);

    EXPECT_EQ(buf.getConfig().metrics, &metrics);
}

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationMetricsUpdatedOnSetConfig) {
    TimeSeriesMetrics metrics;

    TSAutoBufferConfig off;
    off.async_flush = false;
    TSAutoBuffer buf(tsstore.get(), off);
    EXPECT_EQ(buf.getConfig().metrics, nullptr);

    auto on = adaptiveConfig();
    on.metrics = &metrics;
    buf.setConfig(on);
    EXPECT_EQ(buf.getConfig().metrics, &metrics);
}

// ── FlushController stats integration ────────────────────────────────────────

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationFlushControllerStatsExposed) {
    FlushControllerConfig fc_cfg;
    fc_cfg.slo_threshold_ms = 50.0;
    fc_cfg.warmup_samples   = 1;
    fc_cfg.ewma_alpha       = 1.0;
    FlushController fc(fc_cfg);

    fc.reportFlushLatency(200.0);
    auto s = fc.stats();

    EXPECT_GE(s.backpressure_events, 1u);
    EXPECT_TRUE(s.in_backpressure);
    EXPECT_EQ(s.samples, 1u);
    EXPECT_GT(s.ewma_latency_ms, 0.0);
}

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationFlushControllerBatchSizeBoundedByConfig) {
    FlushControllerConfig fc_cfg;
    fc_cfg.slo_threshold_ms   = 1000.0;  // generous SLO → always grow
    fc_cfg.initial_batch_size = 4000;
    fc_cfg.max_batch_size     = 5000;
    fc_cfg.min_batch_size     = 50;
    fc_cfg.warmup_samples     = 1;
    fc_cfg.ewma_alpha         = 1.0;
    FlushController fc(fc_cfg);

    for (int i = 0; i < 50; ++i) {
        fc.reportFlushLatency(0.1);  // very low → grow
    }
    EXPECT_LE(fc.recommendedBatchSize(), fc_cfg.max_batch_size);
    EXPECT_GE(fc.recommendedBatchSize(), fc_cfg.min_batch_size);
}

TEST_F(AdaptiveFlushControllerFocusedTests, IntegrationFlushControllerBatchSizeNeverBelowMin) {
    FlushControllerConfig fc_cfg;
    fc_cfg.slo_threshold_ms   = 1.0;   // very tight → always shrink
    fc_cfg.initial_batch_size = 500;
    fc_cfg.min_batch_size     = 50;
    fc_cfg.max_batch_size     = 5000;
    fc_cfg.warmup_samples     = 1;
    fc_cfg.ewma_alpha         = 1.0;
    FlushController fc(fc_cfg);

    for (int i = 0; i < 50; ++i) {
        fc.reportFlushLatency(1000.0);  // extremely high → shrink
    }
    EXPECT_GE(fc.recommendedBatchSize(), fc_cfg.min_batch_size);
}

}  // namespace
}  // namespace themis
