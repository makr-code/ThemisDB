/*
 * Tests for out-of-order write support with configurable late-arrival window.
 *
 * The late-arrival window (TSStore::Config::late_arrival_window_ms) controls
 * whether out-of-order data points are accepted or rejected:
 *   - 0 (default): no restriction, all timestamps are accepted.
 *   - N > 0: points whose timestamp < (watermark - N) are rejected with
 *            ERR_TIMESERIES_LATE_ARRIVAL.  In-window out-of-order points
 *            are accepted and counted.
 */

#include <gtest/gtest.h>
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/error_registry.h"
#include <filesystem>
#include <chrono>
#include <memory>

namespace themis {
namespace {

static std::string makeTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / ("themis_ts_ooo_" + tag + "_" + std::to_string(ns))).string();
}

struct TSStoreOOOFixture : ::testing::Test {
    std::string db_path = {};
    std::unique_ptr<RocksDBWrapper> db;

    void SetUp() override {
        db_path = makeTempPath("test");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "Failed to open RocksDB at " << db_path;
    }

    void TearDown() override {
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    static TSStore::DataPoint makePoint(const std::string& metric,
                                        const std::string& entity,
                                        int64_t ts_ms,
                                        double value = 1.0) {
        TSStore::DataPoint p;
        p.metric       = metric;
        p.entity       = entity;
        p.timestamp_ms = ts_ms;
        p.value        = value;
        return p;
    }

    std::unique_ptr<TSStore> makeStore(int64_t window_ms,
                                       TSStore::CompressionType compression = TSStore::CompressionType::None) {
        TSStore::Config cfg;
        cfg.late_arrival_window_ms = window_ms;
        cfg.compression = compression;
        return std::make_unique<TSStore>(db->getRawDB(), nullptr, cfg);
    }
};

// ─── Default behaviour (window == 0) ────────────────────────────────────────

TEST_F(TSStoreOOOFixture, DefaultWindowAcceptsAllTimestamps) {
    auto store = makeStore(0);

    // Write points in reverse order – should all succeed with default config
    int64_t base = 1700000000000LL;
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 2000)).has_value());
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 1000)).has_value());
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base)).has_value());

    auto ooo = store->getOutOfOrderStats();
    EXPECT_EQ(ooo.out_of_order_accepted, 0u);
    EXPECT_EQ(ooo.late_arrival_rejected, 0u);
}

// ─── In-order writes with window enabled ────────────────────────────────────

TEST_F(TSStoreOOOFixture, InOrderWritesAlwaysAccepted) {
    auto store = makeStore(60000); // 60 s window

    int64_t base = 1700000000000LL;
    EXPECT_TRUE(store->putDataPoint(makePoint("cpu", "s1", base)).has_value());
    EXPECT_TRUE(store->putDataPoint(makePoint("cpu", "s1", base + 1000)).has_value());
    EXPECT_TRUE(store->putDataPoint(makePoint("cpu", "s1", base + 2000)).has_value());

    auto ooo = store->getOutOfOrderStats();
    EXPECT_EQ(ooo.out_of_order_accepted, 0u);
    EXPECT_EQ(ooo.late_arrival_rejected, 0u);
}

// ─── Out-of-order within window: accepted ───────────────────────────────────

TEST_F(TSStoreOOOFixture, OutOfOrderWithinWindowAccepted) {
    const int64_t window_ms = 60000; // 60 s
    auto store = makeStore(window_ms);

    int64_t base = 1700000000000LL;
    // Advance watermark to base + 30 s
    EXPECT_TRUE(store->putDataPoint(makePoint("temp", "sensor1", base + 30000)).has_value());

    // Write a point 10 s before the watermark – within window, should succeed
    auto result = store->putDataPoint(makePoint("temp", "sensor1", base + 20000));
    ASSERT_TRUE(result.has_value()) << "Expected in-window out-of-order point to be accepted";

    auto ooo = store->getOutOfOrderStats();
    EXPECT_EQ(ooo.out_of_order_accepted, 1u);
    EXPECT_EQ(ooo.late_arrival_rejected, 0u);
}

// ─── Out-of-order outside window: rejected ──────────────────────────────────

TEST_F(TSStoreOOOFixture, OutOfOrderOutsideWindowRejected) {
    const int64_t window_ms = 60000; // 60 s
    auto store = makeStore(window_ms);

    int64_t base = 1700000000000LL;
    // Advance watermark to base + 2 min
    EXPECT_TRUE(store->putDataPoint(makePoint("temp", "sensor2", base + 120000)).has_value());

    // Write a point 90 s before the watermark – outside 60 s window, must be rejected
    auto result = store->putDataPoint(makePoint("temp", "sensor2", base + 30000));
    ASSERT_FALSE(result.has_value()) << "Expected late point to be rejected";
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_TIMESERIES_LATE_ARRIVAL);

    auto ooo = store->getOutOfOrderStats();
    EXPECT_EQ(ooo.out_of_order_accepted, 0u);
    EXPECT_EQ(ooo.late_arrival_rejected, 1u);
}

// ─── Exactly on window boundary: accepted ───────────────────────────────────

TEST_F(TSStoreOOOFixture, PointExactlyAtWindowBoundaryAccepted) {
    const int64_t window_ms = 60000;
    auto store = makeStore(window_ms);

    int64_t base = 1700000000000LL;
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 60000)).has_value());

    // Point exactly at watermark - window = base: accepted
    auto result = store->putDataPoint(makePoint("m", "e", base));
    EXPECT_TRUE(result.has_value()) << "Point at boundary should be accepted";
}

// ─── Point one millisecond outside window: rejected ─────────────────────────

TEST_F(TSStoreOOOFixture, PointOneMillisecondOutsideWindowRejected) {
    const int64_t window_ms = 60000;
    auto store = makeStore(window_ms);

    int64_t base = 1700000000000LL;
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 60000)).has_value());

    // base - 1 ms is one millisecond past the window cutoff
    auto result = store->putDataPoint(makePoint("m", "e", base - 1));
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_TIMESERIES_LATE_ARRIVAL);
}

// ─── First write always accepted (no prior watermark) ───────────────────────

TEST_F(TSStoreOOOFixture, FirstWriteAlwaysAccepted) {
    auto store = makeStore(1000); // 1 s window

    // Regardless of timestamp, the first write should succeed (no watermark yet)
    int64_t ancient = 0LL; // epoch
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", ancient)).has_value());
}

// ─── Different series are independent ───────────────────────────────────────

TEST_F(TSStoreOOOFixture, WatermarksArePerSeries) {
    const int64_t window_ms = 60000;
    auto store = makeStore(window_ms);

    int64_t base = 1700000000000LL;
    // Advance watermark for entity "e1"
    EXPECT_TRUE(store->putDataPoint(makePoint("cpu", "e1", base + 120000)).has_value());

    // A late point for "e1" should be rejected
    auto r1 = store->putDataPoint(makePoint("cpu", "e1", base));
    EXPECT_FALSE(r1.has_value());

    // The same timestamp for "e2" (fresh watermark) should be accepted
    auto r2 = store->putDataPoint(makePoint("cpu", "e2", base));
    EXPECT_TRUE(r2.has_value());
}

// ─── Different metrics are independent ──────────────────────────────────────

TEST_F(TSStoreOOOFixture, WatermarksArePerMetric) {
    const int64_t window_ms = 60000;
    auto store = makeStore(window_ms);

    int64_t base = 1700000000000LL;
    // Advance "cpu" watermark
    EXPECT_TRUE(store->putDataPoint(makePoint("cpu",  "e1", base + 120000)).has_value());

    // Same timestamp but different metric -> fresh watermark -> accepted
    EXPECT_TRUE(store->putDataPoint(makePoint("mem",  "e1", base)).has_value());
}

// ─── putDataPoints batch – in-order batch accepted ──────────────────────────

TEST_F(TSStoreOOOFixture, BatchInOrderAccepted) {
    auto store = makeStore(60000);

    int64_t base = 1700000000000LL;
    std::vector<TSStore::DataPoint> pts = {
        makePoint("m", "e", base),
        makePoint("m", "e", base + 1000),
        makePoint("m", "e", base + 2000),
    };
    EXPECT_TRUE(store->putDataPoints(pts).has_value());
}

// ─── putDataPoints batch – late arrival rejected ─────────────────────────────

TEST_F(TSStoreOOOFixture, BatchLateArrivalRejected) {
    auto store = makeStore(60000);

    int64_t base = 1700000000000LL;
    // Establish watermark
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 120000)).has_value());

    // Batch contains a point outside the window
    std::vector<TSStore::DataPoint> pts = {
        makePoint("m", "e", base + 100000),
        makePoint("m", "e", base),  // 120 s before watermark -> outside 60 s window
    };
    auto result = store->putDataPoints(pts);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_TIMESERIES_LATE_ARRIVAL);
}

// ─── putDataPoints with Gorilla compression ──────────────────────────────────

TEST_F(TSStoreOOOFixture, GorillaBatchInOrderAccepted) {
    auto store = makeStore(60000, TSStore::CompressionType::Gorilla);

    int64_t base = 1700000000000LL;
    std::vector<TSStore::DataPoint> pts = {
        makePoint("m", "e", base + 3000, 3.0),
        makePoint("m", "e", base + 1000, 1.0),  // out-of-order but within window
        makePoint("m", "e", base + 2000, 2.0),
    };
    EXPECT_TRUE(store->putDataPoints(pts).has_value());
    auto ooo = store->getOutOfOrderStats();
    EXPECT_EQ(ooo.late_arrival_rejected, 0u);
}

TEST_F(TSStoreOOOFixture, GorillaBatchLateArrivalRejected) {
    auto store = makeStore(60000, TSStore::CompressionType::Gorilla);

    int64_t base = 1700000000000LL;
    // Establish watermark
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 120000)).has_value());

    std::vector<TSStore::DataPoint> pts = {
        makePoint("m", "e", base + 100000),
        makePoint("m", "e", base),  // outside window
    };
    auto result = store->putDataPoints(pts);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_TIMESERIES_LATE_ARRIVAL);
}

// ─── getOutOfOrderStats accumulates correctly ────────────────────────────────

TEST_F(TSStoreOOOFixture, StatsAccumulateAcrossMultipleWrites) {
    auto store = makeStore(60000);

    int64_t base = 1700000000000LL;
    // Establish watermark
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 120000)).has_value());

    // Two accepted out-of-order points
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 90000)).has_value());
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 80000)).has_value());

    // One rejected
    EXPECT_FALSE(store->putDataPoint(makePoint("m", "e", base)).has_value());

    auto ooo = store->getOutOfOrderStats();
    EXPECT_EQ(ooo.out_of_order_accepted, 2u);
    EXPECT_EQ(ooo.late_arrival_rejected, 1u);
}

// ─── Watermark advances on new max ──────────────────────────────────────────

TEST_F(TSStoreOOOFixture, WatermarkAdvancesWithNewMax) {
    const int64_t window_ms = 60000;
    auto store = makeStore(window_ms);

    int64_t base = 1700000000000LL;
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 60000)).has_value());
    // After first write watermark = base+60000, cutoff = base

    // Advance watermark to base + 120 000
    EXPECT_TRUE(store->putDataPoint(makePoint("m", "e", base + 120000)).has_value());
    // Now cutoff = base + 60000; a point at base should be rejected
    auto result = store->putDataPoint(makePoint("m", "e", base));
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_TIMESERIES_LATE_ARRIVAL);
}

// ─── Written data is still queryable after out-of-order insert ───────────────

TEST_F(TSStoreOOOFixture, OutOfOrderPointIsQueryable) {
    auto store = makeStore(3600000); // 1 h window

    int64_t base = 1700000000000LL;
    EXPECT_TRUE(store->putDataPoint(makePoint("q", "e", base + 1800000)).has_value());  // +30 min
    EXPECT_TRUE(store->putDataPoint(makePoint("q", "e", base + 600000)).has_value());   // +10 min (ooo)

    TSStore::QueryOptions opts;
    opts.metric = "q";
    opts.from_timestamp_ms = base;
    opts.to_timestamp_ms   = base + 2000000;
    auto result = store->query(opts);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2u);
    // Results should be time-ordered
    EXPECT_LT((*result)[0].timestamp_ms, (*result)[1].timestamp_ms);
}

} // namespace
} // namespace themis
