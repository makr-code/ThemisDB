// Copyright 2026 ThemisDB
// Licensed under MIT License
//
// Focused tests for RocksDBWrapper: Proper Size Calculation (v1.8.0, Issue #205)
//
// Acceptance criteria covered:
//   AC-1  getApproximateSize() returns a non-zero value after flushing data.
//   AC-2  getStats() JSON contains "total_sst_files_size_bytes" key.
//   AC-3  DiskSpaceMonitor::setRocksDBSize() persists the value in SpaceInfo.
//   AC-4  getApproximateSize() returns 0 when the DB is not open.

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include "storage/disk_space_monitor.h"
#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using themis::RocksDBWrapper;
using themis::storage::DiskSpaceMonitor;
using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Helper utilities
// ---------------------------------------------------------------------------

static std::string uniqueTmpPath(const std::string& tag) {
    return (fs::temp_directory_path() /
            ("themis_size_" + tag + "_" +
             std::to_string(
                 std::chrono::steady_clock::now().time_since_epoch().count())))
               .string();
}

static std::shared_ptr<RocksDBWrapper> openTempDB(const std::string& path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path    = path;
    cfg.enable_wal = true;
    auto db = std::make_shared<RocksDBWrapper>(cfg);
    if (!db->open()) return nullptr;
    return db;
}

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

class RocksDBSizeCalculationTests : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = uniqueTmpPath("sizecalc");
        fs::remove_all(db_path_);
        db_ = openTempDB(db_path_);
        ASSERT_NE(db_, nullptr) << "Failed to open test RocksDB at " << db_path_;
    }

    void TearDown() override {
        db_.reset();
        fs::remove_all(db_path_);
    }

    std::string                     db_path_;
    std::shared_ptr<RocksDBWrapper> db_;
};

// ---------------------------------------------------------------------------
// AC-1: getApproximateSize() returns a meaningful value after a flush
// ---------------------------------------------------------------------------

TEST_F(RocksDBSizeCalculationTests, GetApproximateSizeIsNonZeroAfterFlush) {
    // Write enough data to produce SST files after a flush.
    for (int i = 0; i < 200; ++i) {
        std::string key = "size_key_" + std::to_string(i);
        std::string val(512, 'x');  // 512-byte values to create substantial data
        db_->put(key, val);
    }

    // Force memtable to disk so SST-size property becomes non-zero.
    db_->flush();

    uint64_t sz = db_->getApproximateSize();
    EXPECT_GT(sz, 0u) << "Expected non-zero size after flush with 200 keys";
}

// ---------------------------------------------------------------------------
// AC-1b: getApproximateSize() grows after more data is added
// ---------------------------------------------------------------------------

TEST_F(RocksDBSizeCalculationTests, GetApproximateSizeIncreasesWithData) {
    // Write initial batch and flush.
    for (int i = 0; i < 100; ++i) {
        db_->put("grow_key_" + std::to_string(i), std::string(512, 'a'));
    }
    db_->flush();
    uint64_t size_before = db_->getApproximateSize();

    // Write more data and flush again.
    for (int i = 100; i < 300; ++i) {
        db_->put("grow_key_" + std::to_string(i), std::string(512, 'b'));
    }
    db_->flush();
    uint64_t size_after = db_->getApproximateSize();

    EXPECT_GE(size_after, size_before)
        << "DB size should not decrease after adding more data";
}

// ---------------------------------------------------------------------------
// AC-2: getStats() JSON contains the new total_sst_files_size_bytes key
// ---------------------------------------------------------------------------

TEST_F(RocksDBSizeCalculationTests, GetStatsJsonHasTotalSstFilesSizeKey) {
    for (int i = 0; i < 50; ++i) {
        db_->put("stat_key_" + std::to_string(i), "value");
    }
    db_->flush();

    std::string stats_str = db_->getStats();
    ASSERT_FALSE(stats_str.empty());

    json j = json::parse(stats_str);
    ASSERT_TRUE(j.contains("rocksdb")) << "stats JSON must have 'rocksdb' key";

    const auto& r = j["rocksdb"];
    EXPECT_TRUE(r.contains("total_sst_files_size_bytes"))
        << "stats JSON must include 'total_sst_files_size_bytes'";

    // Value should be a non-negative integer.
    if (r.contains("total_sst_files_size_bytes")) {
        EXPECT_GE(r["total_sst_files_size_bytes"].get<uint64_t>(), 0u);
    }
}

// ---------------------------------------------------------------------------
// AC-3: DiskSpaceMonitor::setRocksDBSize() persists the value in SpaceInfo
// ---------------------------------------------------------------------------

TEST(DiskSpaceMonitorSizeTests, SetRocksDBSizeReflectedInSpaceInfo) {
    // Use the temp directory as the monitored path.
    std::string path = fs::temp_directory_path().string();
    DiskSpaceMonitor dsm(path);

    constexpr uint64_t kExpectedSize = 123456789ULL;
    dsm.setRocksDBSize(kExpectedSize);

    auto info = dsm.getSpaceInfo();
    EXPECT_EQ(info.rocksdb_size_bytes, kExpectedSize);
}

TEST(DiskSpaceMonitorSizeTests, SetRocksDBSizeDefaultsToZero) {
    std::string path = fs::temp_directory_path().string();
    DiskSpaceMonitor dsm(path);

    auto info = dsm.getSpaceInfo();
    EXPECT_EQ(info.rocksdb_size_bytes, 0u)
        << "rocksdb_size_bytes must default to 0 before setRocksDBSize is called";
}

// AC-3b: checkSpace() must preserve rocksdb_size_bytes set via setRocksDBSize()

TEST(DiskSpaceMonitorSizeTests, CheckSpacePreservesRocksDBSize) {
    std::string path = fs::temp_directory_path().string();
    DiskSpaceMonitor dsm(path);

    constexpr uint64_t kSize = 987654321ULL;
    dsm.setRocksDBSize(kSize);

    // checkSpace() performs an OS disk-space query and updates internal state.
    // The rocksdb_size_bytes value must survive the update.
    auto space = dsm.checkSpace();
    EXPECT_EQ(space.rocksdb_size_bytes, kSize)
        << "checkSpace() must preserve rocksdb_size_bytes across OS disk-space queries";

    // Also verify via getSpaceInfo() for completeness.
    auto info = dsm.getSpaceInfo();
    EXPECT_EQ(info.rocksdb_size_bytes, kSize);
}

// ---------------------------------------------------------------------------
// AC-4: getApproximateSize() returns 0 when the DB is not open
// ---------------------------------------------------------------------------

TEST(RocksDBSizeCalculationClosedDBTests, GetApproximateSizeReturnZeroWhenClosed) {
    RocksDBWrapper::Config cfg;
    cfg.db_path = uniqueTmpPath("closed");
    RocksDBWrapper db(cfg);
    // Deliberately not calling db.open()

    EXPECT_EQ(db.getApproximateSize(), 0u)
        << "getApproximateSize() must return 0 when DB is not open";
}
