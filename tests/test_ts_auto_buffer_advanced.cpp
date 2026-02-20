// Phase 5: Auto-Buffer – Deduplication, Memory Limits & Advanced Tests

#include <gtest/gtest.h>
#include "timeseries/ts_auto_buffer.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include <chrono>
#include <filesystem>
#include <memory>

namespace themis {
namespace {

static std::string makeAutoBuffAdvTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / ("themis_abadv_" + tag + "_" + std::to_string(ns))).string();
}

struct TSAutoBufferAdvFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> tsstore;

    void SetUp() override {
        db_path = makeAutoBuffAdvTempPath("adv");
        RocksDBWrapper::Config cfg;
        cfg.db_path    = db_path;
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

// ===== Deduplication =====

TEST_F(TSAutoBufferAdvFixture, DedupDropsDuplicateTimestamp) {
    TSAutoBufferConfig cfg;
    cfg.async_flush   = false;
    cfg.enable_dedup  = true;
    TSAutoBuffer buf(tsstore.get(), cfg);

    auto r1 = buf.add(makePoint("cpu", "s1", 1.0, 1700000000000LL));
    EXPECT_TRUE(r1.has_value());
    auto r2 = buf.add(makePoint("cpu", "s1", 2.0, 1700000000000LL));  // same timestamp
    EXPECT_TRUE(r2.has_value());  // no error, but point dropped

    EXPECT_EQ(buf.getStats().dedup_dropped_count.load(), 1u);
    EXPECT_EQ(buf.getStats().points_buffered.load(), 1u);
}

TEST_F(TSAutoBufferAdvFixture, DedupAllowsDifferentTimestamps) {
    TSAutoBufferConfig cfg;
    cfg.async_flush  = false;
    cfg.enable_dedup = true;
    TSAutoBuffer buf(tsstore.get(), cfg);

    buf.add(makePoint("cpu", "s2", 1.0, 1700000000000LL));
    buf.add(makePoint("cpu", "s2", 2.0, 1700000001000LL));
    buf.add(makePoint("cpu", "s2", 3.0, 1700000002000LL));

    EXPECT_EQ(buf.getStats().dedup_dropped_count.load(), 0u);
    EXPECT_EQ(buf.getStats().points_buffered.load(), 3u);
}

TEST_F(TSAutoBufferAdvFixture, DedupDisabledAllowsDuplicates) {
    TSAutoBufferConfig cfg;
    cfg.async_flush  = false;
    cfg.enable_dedup = false;  // default
    TSAutoBuffer buf(tsstore.get(), cfg);

    buf.add(makePoint("cpu", "s3", 1.0, 1700000000000LL));
    buf.add(makePoint("cpu", "s3", 2.0, 1700000000000LL));  // same timestamp, allowed

    EXPECT_EQ(buf.getStats().dedup_dropped_count.load(), 0u);
    EXPECT_EQ(buf.getStats().points_buffered.load(), 2u);
}

TEST_F(TSAutoBufferAdvFixture, DedupIndependentPerMetricEntity) {
    TSAutoBufferConfig cfg;
    cfg.async_flush  = false;
    cfg.enable_dedup = true;
    TSAutoBuffer buf(tsstore.get(), cfg);

    // Same timestamp, different metric:entity → both accepted
    buf.add(makePoint("cpu", "s4", 1.0, 1700000000000LL));
    buf.add(makePoint("mem", "s4", 2.0, 1700000000000LL));

    EXPECT_EQ(buf.getStats().dedup_dropped_count.load(), 0u);
    EXPECT_EQ(buf.getStats().points_buffered.load(), 2u);
}

// ===== Per-metric memory limit =====

TEST_F(TSAutoBufferAdvFixture, PerMetricMemoryLimitRejectsOverflow) {
    TSAutoBufferConfig cfg;
    cfg.async_flush               = false;
    cfg.max_points_per_buffer     = 10000;   // high flush threshold
    cfg.max_memory_bytes          = 512 * 1024 * 1024;  // 512MB global
    cfg.max_memory_per_metric_bytes = 1;  // 1 byte → immediately reject 2nd point
    TSAutoBuffer buf(tsstore.get(), cfg);

    auto r1 = buf.add(makePoint("cpu", "s5", 1.0, 1700000000000LL));
    EXPECT_TRUE(r1.has_value());
    auto r2 = buf.add(makePoint("cpu", "s5", 2.0, 1700000001000LL));
    // Second point rejected (buffer memory > 1 byte after first)
    EXPECT_FALSE(r2.has_value());
    EXPECT_GE(buf.getStats().memory_limit_rejected_count.load(), 1u);
}

TEST_F(TSAutoBufferAdvFixture, PerMetricMemoryLimitDoesNotAffectOtherMetrics) {
    TSAutoBufferConfig cfg;
    cfg.async_flush               = false;
    cfg.max_points_per_buffer     = 10000;
    cfg.max_memory_bytes          = 512 * 1024 * 1024;
    cfg.max_memory_per_metric_bytes = 1;
    TSAutoBuffer buf(tsstore.get(), cfg);

    buf.add(makePoint("cpu", "s6", 1.0, 1700000000000LL));  // fills cpu:s6 limit
    // Different metric:entity → not affected
    auto r2 = buf.add(makePoint("mem", "s6", 2.0, 1700000000000LL));
    EXPECT_TRUE(r2.has_value());
}

TEST_F(TSAutoBufferAdvFixture, PerMetricMemoryLimitZeroMeansUnlimited) {
    TSAutoBufferConfig cfg;
    cfg.async_flush               = false;
    cfg.max_memory_per_metric_bytes = 0;  // unlimited
    TSAutoBuffer buf(tsstore.get(), cfg);

    for (int i = 0; i < 20; ++i) {
        auto r = buf.add(makePoint("cpu", "s7", static_cast<double>(i),
                                   1700000000000LL + i * 1000));
        EXPECT_TRUE(r.has_value());
    }
    EXPECT_EQ(buf.getStats().memory_limit_rejected_count.load(), 0u);
}

// ===== Config update =====

TEST_F(TSAutoBufferAdvFixture, DedupEnabledViaSetConfig) {
    TSAutoBufferConfig cfg;
    cfg.async_flush  = false;
    cfg.enable_dedup = false;
    TSAutoBuffer buf(tsstore.get(), cfg);

    // Enable dedup after construction
    TSAutoBufferConfig new_cfg = cfg;
    new_cfg.enable_dedup = true;
    buf.setConfig(new_cfg);
    EXPECT_TRUE(buf.getConfig().enable_dedup);

    buf.add(makePoint("cpu", "s8", 1.0, 1700000000000LL));
    buf.add(makePoint("cpu", "s8", 2.0, 1700000000000LL));
    EXPECT_EQ(buf.getStats().dedup_dropped_count.load(), 1u);
}

// ===== Stats: dedup defaults =====

TEST_F(TSAutoBufferAdvFixture, DedupStatsDefaultZero) {
    TSAutoBufferConfig cfg;
    cfg.async_flush = false;
    TSAutoBuffer buf(tsstore.get(), cfg);
    EXPECT_EQ(buf.getStats().dedup_dropped_count.load(), 0u);
    EXPECT_EQ(buf.getStats().memory_limit_rejected_count.load(), 0u);
}

}  // namespace
}  // namespace themis
