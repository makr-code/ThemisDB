// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for TsStreamCursor:
//   SC-01  open() on empty store returns a cursor with valid()==false
//   SC-02  open() fetches first page eagerly; valid()==true with data
//   SC-03  advance() iterates through all DataPoints
//   SC-04  rowsConsumed() matches total written rows
//   SC-05  pagesFetched() >= 1 after a non-empty scan
//   SC-06  close() makes valid()==false
//   SC-07  Scan across page boundaries returns all rows
//   SC-08  current() reflects correct DataPoint at each step

#include <gtest/gtest.h>
#include "timeseries/ts_stream_cursor.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;
using namespace themis::timeseries;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class TsStreamCursorTest : public ::testing::Test {
protected:
    const int64_t BASE_MS = 1700000000000LL; // ~2023-11

    void SetUp() override {
        auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        db_path_ = (fs::temp_directory_path() /
                    ("themis_cursor_" + std::to_string(ns))).string();

        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path_;
        cfg.enable_blobdb = false;
        db_    = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        TSStore::Config ts_cfg;
        ts_cfg.compression = TSStore::CompressionType::None;
        store_ = std::make_unique<TSStore>(db_->getRawDB(), nullptr, ts_cfg);
    }

    void TearDown() override {
        store_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    void writePoints(const std::string& metric, int count, int64_t start_ms = 0) {
        if (start_ms == 0) start_ms = BASE_MS;
        for (int i = 0; i < count; ++i) {
            TSStore::DataPoint dp;
            dp.metric       = metric;
            dp.entity       = "entity0";
            dp.timestamp_ms = start_ms + i * 1000;
            dp.value        = static_cast<double>(i);
            ASSERT_TRUE(store_->putDataPoint(dp));
        }
    }

    TSStore::QueryOptions allOf(const std::string& metric) {
        TSStore::QueryOptions q;
        q.metric           = metric;
        q.from_timestamp_ms = 0;
        q.to_timestamp_ms  = INT64_MAX;
        q.limit            = 1'000'000;
        return q;
    }

    std::string                      db_path_;
    std::shared_ptr<RocksDBWrapper>  db_;
    std::unique_ptr<TSStore>         store_;
};

// ─────────────────────────────────────────────────────────────────────────────
// SC-01: Open on empty store — valid() is false immediately
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TsStreamCursorTest, SC01_EmptyStoreValidIsFalse) {
    auto res = TsStreamCursor::open(*store_, allOf("no_such_metric"));
    ASSERT_TRUE(res) << res.error().message();
    EXPECT_FALSE((*res)->valid());
}

// ─────────────────────────────────────────────────────────────────────────────
// SC-02: Open with data — valid()==true right after open
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TsStreamCursorTest, SC02_OpenWithDataIsValid) {
    writePoints("cpu", 5);
    auto res = TsStreamCursor::open(*store_, allOf("cpu"));
    ASSERT_TRUE(res);
    EXPECT_TRUE((*res)->valid());
}

// ─────────────────────────────────────────────────────────────────────────────
// SC-03: advance() iterates through all DataPoints
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TsStreamCursorTest, SC03_AdvanceIteratesAll) {
    const int N = 10;
    writePoints("load", N);

    auto res = TsStreamCursor::open(*store_, allOf("load"));
    ASSERT_TRUE(res);
    auto& cursor = **res;

    int count = 0;
    while (cursor.valid()) {
        ++count;
        ASSERT_TRUE(cursor.advance());
    }
    EXPECT_EQ(count, N);
}

// ─────────────────────────────────────────────────────────────────────────────
// SC-04: rowsConsumed() matches total written rows
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TsStreamCursorTest, SC04_RowsConsumedMatchesWritten) {
    const int N = 7;
    writePoints("mem", N);

    auto res = TsStreamCursor::open(*store_, allOf("mem"));
    ASSERT_TRUE(res);
    auto& cursor = **res;

    while (cursor.valid()) {
        ASSERT_TRUE(cursor.advance());
    }
    EXPECT_EQ(cursor.rowsConsumed(), static_cast<uint64_t>(N));
}

// ─────────────────────────────────────────────────────────────────────────────
// SC-05: pagesFetched() >= 1 after non-empty scan
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TsStreamCursorTest, SC05_PagesFetchedAtLeastOne) {
    writePoints("disk", 3);

    auto res = TsStreamCursor::open(*store_, allOf("disk"));
    ASSERT_TRUE(res);
    auto& cursor = **res;

    while (cursor.valid()) {
        ASSERT_TRUE(cursor.advance());
    }
    EXPECT_GE(cursor.pagesFetched(), uint64_t{1});
}

// ─────────────────────────────────────────────────────────────────────────────
// SC-06: close() makes valid()==false
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TsStreamCursorTest, SC06_CloseInvalidatesCursor) {
    writePoints("net", 5);

    auto res = TsStreamCursor::open(*store_, allOf("net"));
    ASSERT_TRUE(res);
    auto& cursor = **res;

    ASSERT_TRUE(cursor.valid());
    cursor.close();
    EXPECT_FALSE(cursor.valid());
}

// ─────────────────────────────────────────────────────────────────────────────
// SC-07: Scan across page boundaries — all rows returned
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TsStreamCursorTest, SC07_CrossPageBoundaryReturnsAll) {
    const int N = 20; // > default small page in test config
    writePoints("iops", N);

    TsStreamCursor::Config cfg;
    cfg.page_size = 5; // force multiple page fetches

    auto opts = allOf("iops");
    opts.limit = N;

    auto res = TsStreamCursor::open(*store_, opts, cfg);
    ASSERT_TRUE(res);
    auto& cursor = **res;

    int count = 0;
    while (cursor.valid()) {
        ++count;
        ASSERT_TRUE(cursor.advance());
    }
    EXPECT_EQ(count, N);
    EXPECT_GE(cursor.pagesFetched(), uint64_t{2});
}

// ─────────────────────────────────────────────────────────────────────────────
// SC-08: current() reflects correct DataPoint at each step
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TsStreamCursorTest, SC08_CurrentReflectsCorrectPoint) {
    const int N = 5;
    writePoints("lat", N);

    auto opts  = allOf("lat");
    opts.limit = N;

    auto res = TsStreamCursor::open(*store_, opts);
    ASSERT_TRUE(res);
    auto& cursor = **res;

    int idx = 0;
    while (cursor.valid()) {
        const auto& dp = cursor.current();
        EXPECT_EQ(dp.metric, "lat");
        EXPECT_EQ(dp.entity, "entity0");
        EXPECT_DOUBLE_EQ(dp.value, static_cast<double>(idx));
        ASSERT_TRUE(cursor.advance());
        ++idx;
    }
    EXPECT_EQ(idx, N);
}
