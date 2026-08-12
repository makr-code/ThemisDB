/**
 * @file test_adaptive_compaction.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 83/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Focused tests for AdaptiveCompactionScheduler (v1.7.0, Issue #209)
//
// Acceptance criteria covered:
//   AC-1  Monitor read/write patterns (recordRead/recordWrite, EMA rates)
//   AC-2  Predict compaction impact (predictCompactionImpact)
//   AC-3  Schedule compactions during low-load periods (isLowLoadPeriod /
//          shouldTriggerCompaction)
//   AC-4  Adjust compaction triggers dynamically (getAdaptedConfig /
//          applyAdaptedConfig)

#include <gtest/gtest.h>
#include "storage/adaptive_compaction.h"
#include "storage/compaction_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

namespace fs = std::filesystem;
using namespace themis;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: open a temporary RocksDB instance
// ─────────────────────────────────────────────────────────────────────────────

static std::shared_ptr<RocksDBWrapper> openTempDB(const std::string& path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path    = path;
    cfg.enable_wal = true;
    auto db = std::make_shared<RocksDBWrapper>(cfg);
    if (!db->open()) return nullptr;
    return db;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite
// ─────────────────────────────────────────────────────────────────────────────

class AdaptiveCompactionFocusedTests : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (fs::temp_directory_path() /
                    ("themis_ac_test_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
        fs::remove_all(db_path_);
        db_ = openTempDB(db_path_);
        ASSERT_NE(db_, nullptr) << "Failed to open test database";
    }

    void TearDown() override {
        db_.reset();
        fs::remove_all(db_path_);
    }

    std::string                     db_path_;
    std::shared_ptr<RocksDBWrapper> db_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdaptiveCompactionFocusedTests, DefaultConstruction) {
    EXPECT_NO_THROW(AdaptiveCompactionScheduler{});
}

TEST_F(AdaptiveCompactionFocusedTests, CustomConfigConstruction) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.window_samples         = 30;
    cfg.low_load_write_rate    = 50.0;
    cfg.low_load_read_rate     = 500.0;
    cfg.ema_alpha              = 0.3;
    EXPECT_NO_THROW(AdaptiveCompactionScheduler{cfg});
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-1: Monitor read/write patterns
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdaptiveCompactionFocusedTests, AC1_RecordReads_UpdatesTotalCount) {
    AdaptiveCompactionScheduler sched;
    sched.recordRead(100);
    sched.recordRead(50);

    auto s = sched.stats();
    EXPECT_EQ(s.total_reads, 150u);
}

TEST_F(AdaptiveCompactionFocusedTests, AC1_RecordWrites_UpdatesTotalCount) {
    AdaptiveCompactionScheduler sched;
    sched.recordWrite(200);
    sched.recordWrite(75);

    auto s = sched.stats();
    EXPECT_EQ(s.total_writes, 275u);
}

TEST_F(AdaptiveCompactionFocusedTests, AC1_DefaultCounts_AreZero) {
    AdaptiveCompactionScheduler sched;
    auto s = sched.stats();
    EXPECT_EQ(s.total_reads,  0u);
    EXPECT_EQ(s.total_writes, 0u);
}

TEST_F(AdaptiveCompactionFocusedTests, AC1_RecordRead_DefaultCountIsOne) {
    AdaptiveCompactionScheduler sched;
    sched.recordRead();
    EXPECT_EQ(sched.stats().total_reads, 1u);
}

TEST_F(AdaptiveCompactionFocusedTests, AC1_RecordWrite_DefaultCountIsOne) {
    AdaptiveCompactionScheduler sched;
    sched.recordWrite();
    EXPECT_EQ(sched.stats().total_writes, 1u);
}

TEST_F(AdaptiveCompactionFocusedTests, AC1_InitialEmaRates_AreZero) {
    AdaptiveCompactionScheduler sched;
    auto s = sched.stats();
    EXPECT_DOUBLE_EQ(s.ema_read_rate,  0.0);
    EXPECT_DOUBLE_EQ(s.ema_write_rate, 0.0);
}

TEST_F(AdaptiveCompactionFocusedTests, AC1_BackgroundSampling_StartStop) {
    AdaptiveCompactionScheduler sched;
    EXPECT_FALSE(sched.isSamplingRunning());

    sched.startSampling();
    EXPECT_TRUE(sched.isSamplingRunning());

    sched.stopSampling();
    EXPECT_FALSE(sched.isSamplingRunning());
}

TEST_F(AdaptiveCompactionFocusedTests, AC1_BackgroundSampling_DoubleStart_Safe) {
    AdaptiveCompactionScheduler sched;
    sched.startSampling();
    sched.startSampling(); // idempotent
    EXPECT_TRUE(sched.isSamplingRunning());
    sched.stopSampling();
}

TEST_F(AdaptiveCompactionFocusedTests, AC1_BackgroundSampling_StopBeforeStart_Safe) {
    AdaptiveCompactionScheduler sched;
    EXPECT_NO_THROW(sched.stopSampling());
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-2: Predict compaction impact
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdaptiveCompactionFocusedTests, AC2_PredictImpact_ZeroLoad_LowOverhead) {
    AdaptiveCompactionScheduler sched; // EMA rates are 0
    auto pred = sched.predictCompactionImpact(0.0);

    // With zero load and zero write amplification, overhead should be low
    EXPECT_GE(pred.estimated_cpu_overhead, 0.0);
    EXPECT_LE(pred.estimated_cpu_overhead, 0.5);
    EXPECT_GT(pred.estimated_duration_s, 0.0);
}

TEST_F(AdaptiveCompactionFocusedTests, AC2_PredictImpact_HighWriteAmp_IsUrgent) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.urgent_write_amp_threshold = 8.0;
    AdaptiveCompactionScheduler sched(cfg);

    auto pred = sched.predictCompactionImpact(10.0); // above urgent threshold
    EXPECT_TRUE(pred.is_urgent);
}

TEST_F(AdaptiveCompactionFocusedTests, AC2_PredictImpact_LowWriteAmp_NotUrgent) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.urgent_write_amp_threshold = 8.0;
    AdaptiveCompactionScheduler sched(cfg);

    auto pred = sched.predictCompactionImpact(2.0); // below urgent threshold
    EXPECT_FALSE(pred.is_urgent);
}

TEST_F(AdaptiveCompactionFocusedTests, AC2_PredictImpact_ZeroLoad_IsLowImpact) {
    AdaptiveCompactionScheduler sched; // EMA rates are 0, thresholds > 0
    auto pred = sched.predictCompactionImpact(0.0);

    // Zero EMA rates are below any positive threshold → low impact
    EXPECT_TRUE(pred.is_low_impact);
}

TEST_F(AdaptiveCompactionFocusedTests, AC2_PredictImpact_WriteAmplificationReflected) {
    AdaptiveCompactionScheduler sched;
    auto pred = sched.predictCompactionImpact(5.0);
    EXPECT_DOUBLE_EQ(pred.write_amplification, 5.0);
}

TEST_F(AdaptiveCompactionFocusedTests, AC2_PredictImpact_OverheadInRange) {
    AdaptiveCompactionScheduler sched;
    auto pred = sched.predictCompactionImpact(2.0);
    EXPECT_GE(pred.estimated_cpu_overhead, 0.0);
    EXPECT_LE(pred.estimated_cpu_overhead, 1.0);
}

TEST_F(AdaptiveCompactionFocusedTests, AC2_PredictImpact_DurationPositive) {
    AdaptiveCompactionScheduler sched;
    auto pred = sched.predictCompactionImpact(1.0);
    EXPECT_GT(pred.estimated_duration_s, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-3: Schedule compactions during low-load periods
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdaptiveCompactionFocusedTests, AC3_LowLoadPeriod_WithZeroRates_IsTrue) {
    AdaptiveCompactionScheduler sched; // EMA rates start at 0
    EXPECT_TRUE(sched.isLowLoadPeriod());
}

TEST_F(AdaptiveCompactionFocusedTests, AC3_ShouldTrigger_UrgentWriteAmp_Always) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.urgent_write_amp_threshold = 8.0;
    AdaptiveCompactionScheduler sched(cfg);

    // Urgent write amplification should trigger even if "not low load"
    EXPECT_TRUE(sched.shouldTriggerCompaction(9.0));
}

TEST_F(AdaptiveCompactionFocusedTests, AC3_ShouldTrigger_LowWA_LowLoad_DesiredThreshold) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.desired_write_amp_threshold = 3.0;
    cfg.urgent_write_amp_threshold  = 8.0;
    cfg.low_load_write_rate         = 100.0;
    cfg.low_load_read_rate          = 1000.0;
    AdaptiveCompactionScheduler sched(cfg);
    // EMA rates are 0 → low load; WA == desired threshold
    EXPECT_TRUE(sched.shouldTriggerCompaction(3.5));
}

TEST_F(AdaptiveCompactionFocusedTests, AC3_ShouldTrigger_LowWA_NotCounted) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.desired_write_amp_threshold = 5.0;
    cfg.urgent_write_amp_threshold  = 8.0;
    AdaptiveCompactionScheduler sched(cfg);

    // WA below desired threshold → no trigger
    EXPECT_FALSE(sched.shouldTriggerCompaction(1.0));
    EXPECT_EQ(sched.stats().compaction_schedules, 0u);
}

TEST_F(AdaptiveCompactionFocusedTests, AC3_ShouldTrigger_IncreasesCounter) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.urgent_write_amp_threshold = 8.0;
    AdaptiveCompactionScheduler sched(cfg);

    sched.shouldTriggerCompaction(9.0); // trigger #1
    sched.shouldTriggerCompaction(9.0); // trigger #2
    EXPECT_EQ(sched.stats().compaction_schedules, 2u);
}

TEST_F(AdaptiveCompactionFocusedTests, AC3_ShouldNotTrigger_DoesNotIncrementCounter) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.desired_write_amp_threshold = 5.0;
    cfg.urgent_write_amp_threshold  = 8.0;
    AdaptiveCompactionScheduler sched(cfg);

    sched.shouldTriggerCompaction(2.0); // no trigger
    EXPECT_EQ(sched.stats().compaction_schedules, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-4: Adjust compaction triggers dynamically
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdaptiveCompactionFocusedTests, AC4_AdaptedConfig_ZeroLoad_AggressiveThreshold) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.min_tombstone_threshold = 1'000;
    cfg.max_tombstone_threshold = 100'000;
    AdaptiveCompactionScheduler sched(cfg);

    // With zero EMA rates (pressure = 0), tombstone threshold should be at minimum
    auto adapted = sched.getAdaptedConfig();
    EXPECT_EQ(adapted.tombstone_gc_threshold, cfg.min_tombstone_threshold);
}

TEST_F(AdaptiveCompactionFocusedTests, AC4_AdaptedConfig_ZeroLoad_ShortGCInterval) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.min_gc_interval = 60s;
    cfg.max_gc_interval = 1800s;
    AdaptiveCompactionScheduler sched(cfg);

    auto adapted = sched.getAdaptedConfig();
    EXPECT_EQ(adapted.bg_gc_interval, cfg.min_gc_interval);
}

TEST_F(AdaptiveCompactionFocusedTests, AC4_AdaptedConfig_ZeroLoad_FullCompactionEnabled) {
    AdaptiveCompactionScheduler sched;
    auto adapted = sched.getAdaptedConfig();
    // pressure = 0 < 0.2 → full compaction enabled
    EXPECT_TRUE(adapted.enable_full_compaction);
}

TEST_F(AdaptiveCompactionFocusedTests, AC4_AdaptedConfig_ThresholdInRange) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.min_tombstone_threshold = 500;
    cfg.max_tombstone_threshold = 50'000;
    AdaptiveCompactionScheduler sched(cfg);

    auto adapted = sched.getAdaptedConfig();
    EXPECT_GE(adapted.tombstone_gc_threshold, cfg.min_tombstone_threshold);
    EXPECT_LE(adapted.tombstone_gc_threshold, cfg.max_tombstone_threshold);
}

TEST_F(AdaptiveCompactionFocusedTests, AC4_AdaptedConfig_GCIntervalInRange) {
    AdaptiveCompactionScheduler::Config cfg;
    cfg.min_gc_interval = 30s;
    cfg.max_gc_interval = 900s;
    AdaptiveCompactionScheduler sched(cfg);

    auto adapted = sched.getAdaptedConfig();
    EXPECT_GE(adapted.bg_gc_interval.count(), cfg.min_gc_interval.count());
    EXPECT_LE(adapted.bg_gc_interval.count(), cfg.max_gc_interval.count());
}

TEST_F(AdaptiveCompactionFocusedTests, AC4_ApplyAdaptedConfig_UpdatesCompactionManager) {
    AdaptiveCompactionScheduler::Config sched_cfg;
    sched_cfg.min_tombstone_threshold = 100;
    sched_cfg.max_tombstone_threshold = 10'000;
    sched_cfg.min_gc_interval         = 10s;
    sched_cfg.max_gc_interval         = 600s;
    AdaptiveCompactionScheduler sched(sched_cfg);

    CompactionManager::Config mgr_cfg;
    mgr_cfg.tombstone_gc_threshold = 999'999; // deliberately wrong
    mgr_cfg.bg_gc_interval         = 999s;    // deliberately wrong
    CompactionManager mgr(db_, mgr_cfg);

    sched.applyAdaptedConfig(mgr);

    auto updated = mgr.getConfig();
    // After applying, the threshold should be within the adaptive range
    EXPECT_GE(updated.tombstone_gc_threshold, sched_cfg.min_tombstone_threshold);
    EXPECT_LE(updated.tombstone_gc_threshold, sched_cfg.max_tombstone_threshold);
    EXPECT_GE(updated.bg_gc_interval.count(), sched_cfg.min_gc_interval.count());
    EXPECT_LE(updated.bg_gc_interval.count(), sched_cfg.max_gc_interval.count());
}

TEST_F(AdaptiveCompactionFocusedTests, AC4_ApplyAdaptedConfig_IncrementsTriggerAdjustments) {
    AdaptiveCompactionScheduler sched;
    CompactionManager mgr(db_);

    uint64_t before = sched.stats().trigger_adjustments;
    sched.applyAdaptedConfig(mgr);
    EXPECT_GT(sched.stats().trigger_adjustments, before);
}

TEST_F(AdaptiveCompactionFocusedTests, AC4_CompactionManager_SetConfig_UpdatesConfig) {
    CompactionManager::Config cfg;
    cfg.tombstone_gc_threshold = 500;
    cfg.bg_gc_interval         = 120s;
    cfg.enable_full_compaction = true;
    CompactionManager mgr(db_);

    mgr.setConfig(cfg);
    auto retrieved = mgr.getConfig();
    EXPECT_EQ(retrieved.tombstone_gc_threshold, 500u);
    EXPECT_EQ(retrieved.bg_gc_interval, 120s);
    EXPECT_TRUE(retrieved.enable_full_compaction);
}

TEST_F(AdaptiveCompactionFocusedTests, AC4_SetConfig_RestartsBackgroundGC) {
    CompactionManager mgr(db_);
    mgr.startBackgroundGC();
    EXPECT_TRUE(mgr.isBackgroundGCRunning());

    CompactionManager::Config new_cfg;
    new_cfg.bg_gc_interval = 600s;
    mgr.setConfig(new_cfg);

    // Background GC should still be running after setConfig
    EXPECT_TRUE(mgr.isBackgroundGCRunning());
    mgr.stopBackgroundGC();
}

// ─────────────────────────────────────────────────────────────────────────────
// Integration: scheduler + CompactionManager
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(AdaptiveCompactionFocusedTests, Integration_FullWorkflow) {
    AdaptiveCompactionScheduler::Config sched_cfg;
    sched_cfg.desired_write_amp_threshold = 2.0;
    sched_cfg.urgent_write_amp_threshold  = 5.0;
    sched_cfg.min_tombstone_threshold     = 100;
    sched_cfg.max_tombstone_threshold     = 50'000;
    AdaptiveCompactionScheduler sched(sched_cfg);

    CompactionManager::Config mgr_cfg;
    mgr_cfg.enable_full_compaction = false;
    CompactionManager mgr(db_, mgr_cfg);

    // Simulate read/write traffic
    for (int n = 0; n < 50; ++n) sched.recordWrite();
    for (int n = 0; n < 200; ++n) sched.recordRead();

    // At WA = 0 (unknown), low load → should not trigger (WA < desired threshold)
    EXPECT_FALSE(sched.shouldTriggerCompaction(0.0));

    // At urgent WA → should always trigger
    EXPECT_TRUE(sched.shouldTriggerCompaction(6.0));

    // Apply adapted config – must not throw
    EXPECT_NO_THROW(sched.applyAdaptedConfig(mgr));

    // Manager config must now reflect the adapted values
    auto applied = mgr.getConfig();
    EXPECT_GE(applied.tombstone_gc_threshold, sched_cfg.min_tombstone_threshold);
    EXPECT_LE(applied.tombstone_gc_threshold, sched_cfg.max_tombstone_threshold);
}
