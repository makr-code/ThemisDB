/**
 * @file test_ts_auto_buffer.cpp
 * @brief Unit tests for TSAutoBuffer – time-series auto-batching buffer
 */

#include <gtest/gtest.h>
#include "timeseries/ts_auto_buffer.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include <chrono>
#include <filesystem>
#include <memory>

namespace themis {
namespace {

static std::string makeTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / ("themis_ts_buf_" + tag + "_" + std::to_string(ns))).string();
}

struct TSAutoBufferFixture : ::testing::Test {
    std::string db_path = {};
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> tsstore;

    void SetUp() override {
        db_path = makeTempPath("test");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
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
};

TEST_F(TSAutoBufferFixture, ConstructsWithoutThrowing) {
    TSAutoBufferConfig cfg;
    cfg.async_flush = false;
    EXPECT_NO_THROW({ TSAutoBuffer buf(tsstore.get(), cfg); });
}

TEST_F(TSAutoBufferFixture, NullTSStoreThrows) {
    EXPECT_THROW(TSAutoBuffer(nullptr), std::invalid_argument);
}

TEST_F(TSAutoBufferFixture, AddSinglePointSucceeds) {
    TSAutoBufferConfig cfg;
    cfg.async_flush = false;
    TSAutoBuffer buf(tsstore.get(), cfg);
    auto result = buf.add(makePoint("cpu", "srv01", 42.0));
    EXPECT_TRUE(result.has_value()) << result.error().message();
}

TEST_F(TSAutoBufferFixture, ManualFlushEmptiesBuffer) {
    TSAutoBufferConfig cfg;
    cfg.async_flush = false;
    cfg.max_points_per_buffer = 1000;
    TSAutoBuffer buf(tsstore.get(), cfg);
    for (int i = 0; i < 5; ++i) {
        auto r = buf.add(makePoint("mem", "srv02", static_cast<double>(i),
                                   1700000000000LL + i));
        EXPECT_TRUE(r.has_value());
    }
    size_t flushed = buf.flush();
    EXPECT_GE(flushed, 5u);
    EXPECT_GE(buf.getStats().points_flushed.load(), 5u);
    EXPECT_GE(buf.getStats().flush_count.load(), 1u);
}

TEST_F(TSAutoBufferFixture, SizeTriggeredFlush) {
    TSAutoBufferConfig cfg;
    cfg.async_flush           = false;
    cfg.max_points_per_buffer = 3;
    cfg.flush_batch_size      = 3;
    TSAutoBuffer buf(tsstore.get(), cfg);
    for (int i = 0; i < 4; ++i) {
        buf.add(makePoint("disk", "srv03", static_cast<double>(i),
                          1700000000000LL + i));
    }
    buf.flush();
    EXPECT_GE(buf.getStats().points_buffered.load(), 4u);
}

TEST_F(TSAutoBufferFixture, FlushForSpecificMetricEntity) {
    TSAutoBufferConfig cfg;
    cfg.async_flush = false;
    TSAutoBuffer buf(tsstore.get(), cfg);
    buf.add(makePoint("net", "srv04", 1.0, 1700000000001LL));
    buf.add(makePoint("net", "srv05", 2.0, 1700000000002LL));
    size_t flushed = buf.flushFor("net", "srv04");
    EXPECT_GE(flushed, 1u);
}

TEST_F(TSAutoBufferFixture, IsRunningReflectsStartStop) {
    TSAutoBufferConfig cfg;
    cfg.async_flush    = true;
    cfg.flush_interval = std::chrono::milliseconds(10000);
    TSAutoBuffer buf(tsstore.get(), cfg);
    EXPECT_FALSE(buf.isRunning());
    buf.start();
    EXPECT_TRUE(buf.isRunning());
    buf.stop();
    EXPECT_FALSE(buf.isRunning());
}

TEST_F(TSAutoBufferFixture, GetStatsDefaultsAreZero) {
    TSAutoBufferConfig cfg;
    cfg.async_flush = false;
    TSAutoBuffer buf(tsstore.get(), cfg);
    auto stats = buf.getStats();
    EXPECT_EQ(stats.points_buffered.load(), 0u);
    EXPECT_EQ(stats.points_flushed.load(), 0u);
    EXPECT_EQ(stats.flush_count.load(), 0u);
}

TEST_F(TSAutoBufferFixture, ConfigCanBeUpdated) {
    TSAutoBufferConfig cfg;
    cfg.async_flush           = false;
    cfg.max_points_per_buffer = 500;
    TSAutoBuffer buf(tsstore.get(), cfg);
    TSAutoBufferConfig new_cfg = cfg;
    new_cfg.max_points_per_buffer = 100;
    EXPECT_NO_THROW(buf.setConfig(new_cfg));
    EXPECT_EQ(buf.getConfig().max_points_per_buffer, 100u);
}

}  // namespace
}  // namespace themis
