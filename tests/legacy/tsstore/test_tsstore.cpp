/**
 * @file test_tsstore.cpp
 * @brief Unit tests for TSStore – time-series RocksDB storage engine
 */

#include <gtest/gtest.h>
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/error_registry.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <limits>
#include <memory>

namespace themis {
namespace {

static std::string makeTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / ("themis_ts_core_" + tag + "_" + std::to_string(ns))).string();
}

struct TSStoreFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> store;
    int64_t base = 1700000000000LL;

    void SetUp() override {
        db_path = makeTempPath("test");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "Failed to open RocksDB at " << db_path;
        TSStore::Config ts_cfg;
        ts_cfg.compression = TSStore::CompressionType::None; // raw for predictable queries
        store = std::make_unique<TSStore>(db->getRawDB(), nullptr, ts_cfg);
    }

    void TearDown() override {
        store.reset();
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    static TSStore::DataPoint makePoint(const std::string& metric,
                                        const std::string& entity,
                                        int64_t ts_ms,
                                        double value = 1.0,
                                        nlohmann::json tags = nlohmann::json::object()) {
        TSStore::DataPoint p;
        p.metric       = metric;
        p.entity       = entity;
        p.timestamp_ms = ts_ms;
        p.value        = value;
        p.tags         = std::move(tags);
        return p;
    }
};

// ─── Validation ──────────────────────────────────────────────────────────────

TEST_F(TSStoreFixture, RejectsEmptyMetric) {
    auto r = store->putDataPoint(makePoint("", "e", base));
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_API_INVALID_REQUEST);
}

TEST_F(TSStoreFixture, RejectsEmptyEntity) {
    auto r = store->putDataPoint(makePoint("m", "", base));
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_API_INVALID_REQUEST);
}

TEST_F(TSStoreFixture, QueryRequiresMetric) {
    TSStore::QueryOptions q;
    // metric left empty
    auto r = store->query(q);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_API_INVALID_REQUEST);
}

// ─── Basic write + read ───────────────────────────────────────────────────────

TEST_F(TSStoreFixture, PutAndQuerySinglePoint) {
    ASSERT_TRUE(store->putDataPoint(makePoint("cpu", "host1", base, 42.5)).has_value());

    TSStore::QueryOptions q;
    q.metric            = "cpu";
    q.from_timestamp_ms = base;
    q.to_timestamp_ms   = base + 1;
    auto r = store->query(q);
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->size(), 1u);
    EXPECT_NEAR((*r)[0].value, 42.5, 1e-9);
    EXPECT_EQ((*r)[0].metric, "cpu");
    EXPECT_EQ((*r)[0].entity, "host1");
    EXPECT_EQ((*r)[0].timestamp_ms, base);
}

TEST_F(TSStoreFixture, MultiplePointsReturnedInTimestampOrder) {
    // Insert in reverse order
    for (int i = 4; i >= 0; --i) {
        ASSERT_TRUE(store->putDataPoint(makePoint("m", "e", base + i * 1000, static_cast<double>(i))).has_value());
    }

    TSStore::QueryOptions q;
    q.metric            = "m";
    q.from_timestamp_ms = base;
    q.to_timestamp_ms   = base + 5000;
    auto r = store->query(q);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->size(), 5u);
    for (size_t i = 1; i < r->size(); ++i) {
        EXPECT_LT((*r)[i-1].timestamp_ms, (*r)[i].timestamp_ms);
    }
}

// ─── Range queries ────────────────────────────────────────────────────────────

TEST_F(TSStoreFixture, RangeQueryReturnsOnlyPointsInRange) {
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(store->putDataPoint(makePoint("m", "e", base + i * 1000, static_cast<double>(i))).has_value());
    }

    TSStore::QueryOptions q;
    q.metric            = "m";
    q.from_timestamp_ms = base + 3000;
    q.to_timestamp_ms   = base + 6000;
    auto r = store->query(q);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->size(), 4u);  // timestamps: base+3000, +4000, +5000, +6000
    for (const auto& pt : *r) {
        EXPECT_GE(pt.timestamp_ms, base + 3000);
        EXPECT_LE(pt.timestamp_ms, base + 6000);
    }
}

TEST_F(TSStoreFixture, EmptyRangeReturnsNothing) {
    ASSERT_TRUE(store->putDataPoint(makePoint("m", "e", base, 1.0)).has_value());

    TSStore::QueryOptions q;
    q.metric            = "m";
    q.from_timestamp_ms = base + 10000;
    q.to_timestamp_ms   = base + 20000;
    auto r = store->query(q);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->size(), 0u);
}

// ─── Entity filter ────────────────────────────────────────────────────────────

TEST_F(TSStoreFixture, EntityFilterIsolatesCorrectEntity) {
    ASSERT_TRUE(store->putDataPoint(makePoint("cpu", "host1", base,     10.0)).has_value());
    ASSERT_TRUE(store->putDataPoint(makePoint("cpu", "host2", base + 1, 20.0)).has_value());
    ASSERT_TRUE(store->putDataPoint(makePoint("cpu", "host1", base + 2, 30.0)).has_value());

    TSStore::QueryOptions q;
    q.metric            = "cpu";
    q.entity            = "host1";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
    auto r = store->query(q);
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->size(), 2u);
    for (const auto& pt : *r) EXPECT_EQ(pt.entity, "host1");
}

// ─── Tag filter ───────────────────────────────────────────────────────────────

TEST_F(TSStoreFixture, TagFilterMatchesCorrectPoints) {
    nlohmann::json t1 = {{"env", "prod"}};
    nlohmann::json t2 = {{"env", "dev"}};
    ASSERT_TRUE(store->putDataPoint(makePoint("cpu", "h1", base,     1.0, t1)).has_value());
    ASSERT_TRUE(store->putDataPoint(makePoint("cpu", "h2", base + 1, 2.0, t2)).has_value());
    ASSERT_TRUE(store->putDataPoint(makePoint("cpu", "h3", base + 2, 3.0, t1)).has_value());

    TSStore::QueryOptions q;
    q.metric            = "cpu";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
    q.tag_filter        = {{"env", "prod"}};
    auto r = store->query(q);
    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(r->size(), 2u);
    for (const auto& pt : *r) EXPECT_EQ(pt.tags["env"], "prod");
}

// ─── Limit ────────────────────────────────────────────────────────────────────

TEST_F(TSStoreFixture, LimitCapsReturnedPoints) {
    for (int i = 0; i < 20; ++i) {
        ASSERT_TRUE(store->putDataPoint(makePoint("m", "e", base + i * 1000, static_cast<double>(i))).has_value());
    }
    TSStore::QueryOptions q;
    q.metric            = "m";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
    q.limit             = 5;
    auto r = store->query(q);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->size(), 5u);
}

// ─── Aggregation ─────────────────────────────────────────────────────────────

TEST_F(TSStoreFixture, AggregateBasicStats) {
    // Insert 5 points: 1, 2, 3, 4, 5
    for (int i = 1; i <= 5; ++i) {
        ASSERT_TRUE(store->putDataPoint(makePoint("agg_m", "h", base + i * 1000, static_cast<double>(i))).has_value());
    }
    TSStore::QueryOptions q;
    q.metric            = "agg_m";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
    auto r = store->aggregate(q);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->count, 5u);
    EXPECT_NEAR(r->min, 1.0, 1e-9);
    EXPECT_NEAR(r->max, 5.0, 1e-9);
    EXPECT_NEAR(r->sum, 15.0, 1e-9);
    EXPECT_NEAR(r->avg, 3.0, 1e-9);
}

TEST_F(TSStoreFixture, AggregateEmptyReturnsZeroCount) {
    TSStore::QueryOptions q;
    q.metric            = "nonexistent";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
    auto r = store->aggregate(q);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->count, 0u);
}

// ─── deleteOldData (global) ───────────────────────────────────────────────────

TEST_F(TSStoreFixture, DeleteOldDataRemovesOnlyOldPoints) {
    ASSERT_TRUE(store->putDataPoint(makePoint("m", "e", base,         1.0)).has_value());  // old
    ASSERT_TRUE(store->putDataPoint(makePoint("m", "e", base + 1000,  2.0)).has_value());  // new
    ASSERT_TRUE(store->putDataPoint(makePoint("m", "e", base + 2000,  3.0)).has_value());  // new

    size_t deleted = store->deleteOldData(base + 500);
    EXPECT_EQ(deleted, 1u);  // only the base point

    TSStore::QueryOptions q;
    q.metric            = "m";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
    auto r = store->query(q);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->size(), 2u);
}

// ─── deleteOldDataForMetric ───────────────────────────────────────────────────

TEST_F(TSStoreFixture, DeleteOldDataForMetricOnlyTouchesNamedMetric) {
    ASSERT_TRUE(store->putDataPoint(makePoint("cpu",  "h", base, 1.0)).has_value());  // old – should be deleted
    ASSERT_TRUE(store->putDataPoint(makePoint("disk", "h", base, 2.0)).has_value());  // old – kept (different metric)

    size_t deleted = store->deleteOldDataForMetric("cpu", base + 1);
    EXPECT_EQ(deleted, 1u);

    TSStore::QueryOptions q_cpu;
    q_cpu.metric            = "cpu";
    q_cpu.from_timestamp_ms = 0;
    q_cpu.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
    auto r_cpu = store->query(q_cpu);
    ASSERT_TRUE(r_cpu.has_value());
    EXPECT_EQ(r_cpu->size(), 0u);

    TSStore::QueryOptions q_disk;
    q_disk.metric            = "disk";
    q_disk.from_timestamp_ms = 0;
    q_disk.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
    auto r_disk = store->query(q_disk);
    ASSERT_TRUE(r_disk.has_value());
    EXPECT_EQ(r_disk->size(), 1u);
}

// ─── deleteMetric ─────────────────────────────────────────────────────────────

TEST_F(TSStoreFixture, DeleteMetricRemovesAllPointsForMetric) {
    ASSERT_TRUE(store->putDataPoint(makePoint("to_del", "h", base,         1.0)).has_value());
    ASSERT_TRUE(store->putDataPoint(makePoint("to_del", "h", base + 1000,  2.0)).has_value());
    ASSERT_TRUE(store->putDataPoint(makePoint("keep",   "h", base,         3.0)).has_value());

    auto r = store->deleteMetric("to_del");
    ASSERT_TRUE(r.has_value());

    TSStore::QueryOptions q;
    q.metric            = "to_del";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
    EXPECT_EQ(store->query(q)->size(), 0u);

    q.metric = "keep";
    EXPECT_EQ(store->query(q)->size(), 1u);
}

TEST_F(TSStoreFixture, DeleteMetricEmptyNameReturnsError) {
    auto r = store->deleteMetric("");
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_API_INVALID_REQUEST);
}

// ─── clear ───────────────────────────────────────────────────────────────────

TEST_F(TSStoreFixture, ClearDeletesAllData) {
    ASSERT_TRUE(store->putDataPoint(makePoint("a", "h", base,         1.0)).has_value());
    ASSERT_TRUE(store->putDataPoint(makePoint("b", "h", base + 1000,  2.0)).has_value());
    store->clear();

    auto stats = store->getStats();
    EXPECT_EQ(stats.total_data_points, 0u);
}

// ─── getStats ─────────────────────────────────────────────────────────────────

TEST_F(TSStoreFixture, GetStatsReflectsInsertedPoints) {
    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(store->putDataPoint(makePoint("stat_m", "e", base + i * 1000, static_cast<double>(i))).has_value());
    }
    auto stats = store->getStats();
    EXPECT_EQ(stats.total_data_points, 3u);
    EXPECT_EQ(stats.total_metrics, 1u);
    EXPECT_LE(stats.oldest_timestamp_ms, base);
    EXPECT_GE(stats.newest_timestamp_ms, base + 2000);
}

// ─── putDataPoints batch (no compression) ────────────────────────────────────

TEST_F(TSStoreFixture, PutDataPointsBatchAllWritten) {
    std::vector<TSStore::DataPoint> pts;
    for (int i = 0; i < 5; ++i) {
        pts.push_back(makePoint("batch", "h", base + i * 1000, static_cast<double>(i)));
    }
    ASSERT_TRUE(store->putDataPoints(pts).has_value());

    TSStore::QueryOptions q;
    q.metric            = "batch";
    q.from_timestamp_ms = 0;
    q.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
    auto r = store->query(q);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->size(), 5u);
}

TEST_F(TSStoreFixture, PutDataPointsEmptyBatchSucceeds) {
    EXPECT_TRUE(store->putDataPoints({}).has_value());
}

// ─── setConfig round-trip ─────────────────────────────────────────────────────

TEST_F(TSStoreFixture, SetConfigChangesConfig) {
    TSStore::Config new_cfg;
    new_cfg.compression         = TSStore::CompressionType::Gorilla;
    new_cfg.chunk_size_hours    = 12;
    new_cfg.late_arrival_window_ms = 30000;
    store->setConfig(new_cfg);

    const auto& c = store->getConfig();
    EXPECT_EQ(c.compression, TSStore::CompressionType::Gorilla);
    EXPECT_EQ(c.chunk_size_hours, 12);
    EXPECT_EQ(c.late_arrival_window_ms, 30000);
}

} // namespace
} // namespace themis

