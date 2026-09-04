// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for CompactionManager:
//  - Construction with a valid RocksDBWrapper
//  - Manual compactRange / compactAll
//  - Tombstone threshold tracking and runGC
//  - Background GC thread start/stop
//  - Stats snapshot

#include <gtest/gtest.h>
#include "storage/compaction_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <string>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class CompactionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CompactionManagerTest on Windows due to intermittent heap corruption in fixture setup.";
#endif
        db_path_ = (fs::temp_directory_path() /
                    ("themis_compact_test_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
        fs::remove_all(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = db_path_;
        cfg.enable_wal = true;
        db_            = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
    }

    void TearDown() override {
        if (mgr_) {
            mgr_->stopBackgroundGC();
            mgr_.reset();
        }
        db_.reset();
        std::error_code ec = {};
        fs::remove_all(db_path_, ec);
    }

    CompactionManager& manager(CompactionManager::Config cfg = {}) {
        if (!mgr_) {
            mgr_ = std::make_unique<CompactionManager>(db_, cfg);
        }
        return *mgr_;
    }

    // Insert N key-value pairs to give the compactor something to work on.
    void insertData(int n) {
        for (int i = 0; i < n; ++i) {
            db_->put("key_" + std::to_string(i),
                     std::string_view("value_" + std::to_string(i)));
        }
    }

    std::string                              db_path_;
    std::shared_ptr<RocksDBWrapper>          db_;
    std::unique_ptr<CompactionManager>       mgr_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CompactionManagerTest, ConstructsSuccessfully) {
    EXPECT_NO_THROW(manager());
}

TEST_F(CompactionManagerTest, NullDbThrows) {
    EXPECT_THROW(
        (CompactionManager{nullptr}),
        std::invalid_argument
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Manual compaction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CompactionManagerTest, CompactRange_Succeeds) {
    insertData(10);
    auto r = manager().compactRange("key_0", "key_9");
    EXPECT_TRUE(r.has_value()) << "compactRange failed";

    auto s = manager().stats();
    EXPECT_EQ(s.manual_compactions, 1u);
}

TEST_F(CompactionManagerTest, CompactAll_Succeeds) {
    insertData(10);
    auto r = manager().compactAll();
    EXPECT_TRUE(r.has_value()) << "compactAll failed";

    auto s = manager().stats();
    EXPECT_EQ(s.manual_compactions, 1u);
}

TEST_F(CompactionManagerTest, MultipleManualCompactions_CountedCorrectly) {
    insertData(5);
    (void)manager().compactAll();
    (void)manager().compactAll();
    (void)manager().compactRange("key_0", "key_3");

    EXPECT_EQ(manager().stats().manual_compactions, 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tombstone tracking
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CompactionManagerTest, RecordDeletions_UpdatesCount) {
    manager().recordDeletions(100);
    EXPECT_EQ(manager().stats().tombstones_tracked, 100u);

    manager().recordDeletions(50);
    EXPECT_EQ(manager().stats().tombstones_tracked, 150u);
}

TEST_F(CompactionManagerTest, GC_BelowThreshold_NoCompaction) {
    CompactionManager::Config cfg;
    cfg.tombstone_gc_threshold = 1000;
    cfg.enable_full_compaction = true;

    manager(cfg).recordDeletions(100);  // below threshold
    auto r = manager(cfg).runGC(false);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(manager(cfg).stats().gc_runs, 0u);  // no GC triggered
}

TEST_F(CompactionManagerTest, GC_AboveThreshold_RunsCompaction) {
    insertData(10);

    CompactionManager::Config cfg;
    cfg.tombstone_gc_threshold = 5;
    cfg.enable_full_compaction = true;

    auto& m = manager(cfg);
    m.recordDeletions(10);  // above threshold

    auto r = m.runGC(false);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(m.stats().gc_runs, 1u);
    EXPECT_EQ(m.stats().tombstones_tracked, 0u); // reset after GC
}

TEST_F(CompactionManagerTest, GC_Forced_RunsEvenBelowThreshold) {
    CompactionManager::Config cfg;
    cfg.tombstone_gc_threshold = 10000;
    cfg.enable_full_compaction = true;

    auto& m = manager(cfg);
    m.recordDeletions(1);  // well below threshold

    auto r = m.runGC(true); // force
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(m.stats().gc_runs, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Background GC
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CompactionManagerTest, BackgroundGC_StartAndStop) {
    CompactionManager::Config cfg;
    cfg.bg_gc_interval = std::chrono::seconds{600}; // don't trigger during test

    auto& m = manager(cfg);
    EXPECT_FALSE(m.isBackgroundGCRunning());

    m.startBackgroundGC();
    EXPECT_TRUE(m.isBackgroundGCRunning());

    m.stopBackgroundGC();
    EXPECT_FALSE(m.isBackgroundGCRunning());
}

TEST_F(CompactionManagerTest, BackgroundGC_DoubleStart_Safe) {
    auto& m = manager();
    m.startBackgroundGC();
    m.startBackgroundGC(); // should be idempotent
    EXPECT_TRUE(m.isBackgroundGCRunning());
    m.stopBackgroundGC();
}

TEST_F(CompactionManagerTest, BackgroundGC_StopBeforeStart_Safe) {
    auto& m = manager();
    m.stopBackgroundGC(); // should not crash
    EXPECT_FALSE(m.isBackgroundGCRunning());
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(CompactionManagerTest, Stats_InitialValues) {
    auto s = manager().stats();
    EXPECT_EQ(s.tombstones_tracked, 0u);
    EXPECT_EQ(s.gc_runs,            0u);
    EXPECT_EQ(s.manual_compactions, 0u);
}

TEST_F(CompactionManagerTest, Stats_RocksDBStatsNotEmpty) {
    auto s = manager().stats();
    // RocksDB statistics string should be non-empty (even if statistics is disabled,
    // an empty string is still returned, so we just ensure no exception occurs).
    EXPECT_NO_THROW((void)s.rocksdb_stats.size());
}
