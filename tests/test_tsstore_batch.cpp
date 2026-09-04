/**
 * @file test_tsstore_batch.cpp
 * @brief Focused unit tests for TSStore::putBatch — zero-copy multi-row batch write.
 *
 * Test IDs: TB-01 … TB-14
 */

#include <gtest/gtest.h>
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/error_registry.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace themis {
namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string makeTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_tsbatch_" + tag + "_" + std::to_string(ns))).string();
}

struct TSBatchFixture : ::testing::Test {
    std::string              db_path = {};
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> store;
    const int64_t            BASE_MS = 1700000000000LL;

    void SetUp() override {
        db_path = makeTempPath("batch");
        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "RocksDB open failed: " << db_path;
        TSStore::Config ts_cfg;
        ts_cfg.compression = TSStore::CompressionType::None;
        store = std::make_unique<TSStore>(db->getRawDB(), nullptr, ts_cfg);
    }

    void TearDown() override {
        store.reset();
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    static TSStore::TSRow makeRow(std::string_view metric,
                                   std::string_view entity,
                                   int64_t ts_ms,
                                   double  value = 1.0) {
        return {metric, entity, ts_ms, value};
    }
};

// ---------------------------------------------------------------------------
// TB-01  Empty span → ok, counts both zero
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB01_EmptySpanOk) {
    std::vector<TSStore::TSRow> rows;
    auto res = store->putBatch(rows);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->ok_count,     0u);
    EXPECT_EQ(res->failed_count, 0u);
    EXPECT_TRUE(res->all_ok());
}

// ---------------------------------------------------------------------------
// TB-02  Single row written and query-able
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB02_SingleRow) {
    std::vector<TSStore::TSRow> rows = {
        makeRow("cpu", "host01", BASE_MS, 42.0)
    };
    auto res = store->putBatch(rows);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->all_ok());
    EXPECT_EQ(res->ok_count, 1u);

    TSStore::QueryOptions q;
    q.metric = "cpu";
    q.entity = "host01";
    auto qr = store->query(q);
    ASSERT_TRUE(qr.has_value());
    ASSERT_EQ(qr->size(), 1u);
    EXPECT_DOUBLE_EQ((*qr)[0].value, 42.0);
    EXPECT_EQ((*qr)[0].timestamp_ms, BASE_MS);
}

// ---------------------------------------------------------------------------
// TB-03  Multi-row, single metric/entity
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB03_MultiRowSingleSeries) {
    constexpr size_t N = 50;
    std::vector<TSStore::TSRow> rows;
    rows.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        rows.push_back(makeRow("temp", "sensor1",
                               BASE_MS + static_cast<int64_t>(i) * 1000,
                               static_cast<double>(i)));
    }
    auto res = store->putBatch(rows);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->ok_count,     N);
    EXPECT_EQ(res->failed_count, 0u);

    TSStore::QueryOptions q;
    q.metric = "temp";
    q.entity = "sensor1";
    q.limit  = N + 10;
    auto qr = store->query(q);
    ASSERT_TRUE(qr.has_value());
    EXPECT_EQ(qr->size(), N);
}

// ---------------------------------------------------------------------------
// TB-04  Multi-metric, multi-entity in one batch
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB04_MultiMetricMultiEntity) {
    std::vector<TSStore::TSRow> rows = {
        makeRow("cpu",  "host01", BASE_MS,          10.0),
        makeRow("cpu",  "host02", BASE_MS,          20.0),
        makeRow("mem",  "host01", BASE_MS,          30.0),
        makeRow("disk", "host03", BASE_MS + 1000,   40.0),
    };
    auto res = store->putBatch(rows);
    ASSERT_TRUE(res.has_value());
    EXPECT_TRUE(res->all_ok());
    EXPECT_EQ(res->ok_count, 4u);

    // Spot-check mem:host01
    TSStore::QueryOptions q;
    q.metric = "mem"; q.entity = "host01";
    auto qr = store->query(q);
    ASSERT_TRUE(qr.has_value());
    ASSERT_EQ(qr->size(), 1u);
    EXPECT_DOUBLE_EQ((*qr)[0].value, 30.0);
}

// ---------------------------------------------------------------------------
// TB-05  Row with empty metric → failed, others succeed
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB05_EmptyMetricPartialFailure) {
    std::vector<TSStore::TSRow> rows = {
        makeRow("cpu",  "host01", BASE_MS,        1.0),
        makeRow("",     "host02", BASE_MS + 1000, 2.0),   // invalid
        makeRow("mem",  "host01", BASE_MS + 2000, 3.0),
    };
    auto res = store->putBatch(rows);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->failed_count, 1u);
    EXPECT_EQ(res->ok_count,     2u);
    ASSERT_EQ(res->row_errors.size(), 1u);
    EXPECT_EQ(res->row_errors[0].first, 1u);  // index 1 is the bad row
}

// ---------------------------------------------------------------------------
// TB-06  Row with empty entity → failed
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB06_EmptyEntityPartialFailure) {
    std::vector<TSStore::TSRow> rows = {
        makeRow("cpu", "",       BASE_MS,        5.0),   // invalid
        makeRow("cpu", "host01", BASE_MS + 100,  6.0),
    };
    auto res = store->putBatch(rows);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->failed_count, 1u);
    ASSERT_EQ(res->row_errors.size(), 1u);
    EXPECT_EQ(res->row_errors[0].first, 0u);
}

// ---------------------------------------------------------------------------
// TB-07  all_ok() reflects failure state correctly
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB07_AllOkFlag) {
    std::vector<TSStore::TSRow> good = {
        makeRow("x", "y", BASE_MS, 1.0)
    };
    auto r1 = store->putBatch(good);
    ASSERT_TRUE(r1.has_value());
    EXPECT_TRUE(r1->all_ok());

    std::vector<TSStore::TSRow> bad = {
        makeRow("", "y", BASE_MS, 1.0)
    };
    auto r2 = store->putBatch(bad);
    ASSERT_TRUE(r2.has_value());
    EXPECT_FALSE(r2->all_ok());
}

// ---------------------------------------------------------------------------
// TB-08  Gorilla-compression path: data survives round-trip
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB08_GorillaRoundTrip) {
    // Re-open store with Gorilla enabled.
    TSStore::Config gcfg;
    gcfg.compression = TSStore::CompressionType::Gorilla;
    auto gstore = std::make_unique<TSStore>(db->getRawDB(), nullptr, gcfg);

    std::vector<TSStore::TSRow> rows = {};

    for (int i = 0; i < 10; ++i) {
        rows.push_back(makeRow("load", "box1",
                               BASE_MS + i * 1000,
                               static_cast<double>(i) * 0.1));
    }
    auto res = gstore->putBatch(rows);
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_TRUE(res->all_ok());
    EXPECT_EQ(res->ok_count, 10u);

    TSStore::QueryOptions q;
    q.metric = "load"; q.entity = "box1"; q.limit = 20;
    auto qr = gstore->query(q);
    ASSERT_TRUE(qr.has_value());
    EXPECT_EQ(qr->size(), 10u);
    for (size_t i = 0; i < qr->size(); ++i) {
        EXPECT_NEAR((*qr)[i].value, static_cast<double>(i) * 0.1, 1e-9);
    }
}

// ---------------------------------------------------------------------------
// TB-09  Gorilla path: invalid row reported, valid rows written
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB09_GorillaPartialFailure) {
    TSStore::Config gcfg;
    gcfg.compression = TSStore::CompressionType::Gorilla;
    auto gstore = std::make_unique<TSStore>(db->getRawDB(), nullptr, gcfg);

    std::vector<TSStore::TSRow> rows = {
        makeRow("sig", "s1", BASE_MS,        1.0),
        makeRow("",    "s1", BASE_MS + 1000, 2.0),   // invalid
        makeRow("sig", "s1", BASE_MS + 2000, 3.0),
    };
    auto res = gstore->putBatch(rows);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->failed_count, 1u);
    EXPECT_EQ(res->row_errors[0].first, 1u);
}

// ---------------------------------------------------------------------------
// TB-10  Late-arrival window rejects out-of-window rows
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB10_LateArrivalRejection) {
    TSStore::Config la_cfg;
    la_cfg.compression          = TSStore::CompressionType::None;
    la_cfg.late_arrival_window_ms = 5000; // 5 s window
    auto la_store = std::make_unique<TSStore>(db->getRawDB(), nullptr, la_cfg);

    // Seed a watermark for "cpu":"host99"
    std::vector<TSStore::TSRow> seed = {
        makeRow("cpu", "host99", BASE_MS + 100000, 1.0)
    };
    ASSERT_TRUE(la_store->putBatch(seed).has_value());

    // Now send a row that is older than (watermark - 5000)
    std::vector<TSStore::TSRow> late = {
        makeRow("cpu", "host99", BASE_MS, 2.0)  // way too old
    };
    auto res = la_store->putBatch(late);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->failed_count, 1u);
    EXPECT_FALSE(res->row_errors.empty());
}

// ---------------------------------------------------------------------------
// TB-11  BatchWriteResult::row_errors contains the correct row index
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB11_RowErrorIndex) {
    std::vector<TSStore::TSRow> rows = {
        makeRow("a", "e", BASE_MS,       1.0),
        makeRow("b", "e", BASE_MS + 100, 2.0),
        makeRow("",  "e", BASE_MS + 200, 3.0),  // index 2 is bad
        makeRow("c", "e", BASE_MS + 300, 4.0),
        makeRow("",  "e", BASE_MS + 400, 5.0),  // index 4 is bad
    };
    auto res = store->putBatch(rows);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->failed_count, 2u);
    EXPECT_EQ(res->ok_count,     3u);
    ASSERT_EQ(res->row_errors.size(), 2u);
    EXPECT_EQ(res->row_errors[0].first, 2u);
    EXPECT_EQ(res->row_errors[1].first, 4u);
}

// ---------------------------------------------------------------------------
// TB-12  Large batch: 1000 rows, all valid
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB12_LargeBatch1000Rows) {
    constexpr size_t N = 1000;
    std::vector<TSStore::TSRow> rows;
    rows.reserve(N);
    for (size_t i = 0; i < N; ++i) {
        rows.push_back(makeRow("metric_x", "entity_y",
                               BASE_MS + static_cast<int64_t>(i),
                               static_cast<double>(i)));
    }
    auto res = store->putBatch(rows);
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_EQ(res->ok_count,     N);
    EXPECT_EQ(res->failed_count, 0u);
}

// ---------------------------------------------------------------------------
// TB-13  putBatch is additive alongside putDataPoints
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB13_CoexistsWithPutDataPoints) {
    TSStore::DataPoint dp;
    dp.metric       = "voltage";
    dp.entity       = "psu1";
    dp.timestamp_ms = BASE_MS;
    dp.value        = 12.0;
    ASSERT_TRUE(store->putDataPoint(dp).has_value());

    std::vector<TSStore::TSRow> rows = {
        makeRow("voltage", "psu1", BASE_MS + 1000, 12.1),
        makeRow("voltage", "psu1", BASE_MS + 2000, 12.2),
    };
    auto res = store->putBatch(rows);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->ok_count, 2u);

    TSStore::QueryOptions q;
    q.metric = "voltage"; q.entity = "psu1"; q.limit = 10;
    auto qr = store->query(q);
    ASSERT_TRUE(qr.has_value());
    EXPECT_EQ(qr->size(), 3u);  // dp + 2 batch rows
}

// ---------------------------------------------------------------------------
// TB-14  std::span overload: view into a std::array (zero-allocation call site)
// ---------------------------------------------------------------------------
TEST_F(TSBatchFixture, TB14_SpanFromArray) {
    // Caller passes a std::array directly — no vector allocation at all.
    const std::array<TSStore::TSRow, 3> arr = {{
        makeRow("io", "disk0", BASE_MS,        100.0),
        makeRow("io", "disk0", BASE_MS + 1000, 200.0),
        makeRow("io", "disk0", BASE_MS + 2000, 300.0),
    }};
    auto res = store->putBatch(std::span<const TSStore::TSRow>(arr));
    ASSERT_TRUE(res.has_value()) << res.error().message();
    EXPECT_EQ(res->ok_count, 3u);
}

} // namespace
} // namespace themis
