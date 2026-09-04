/**
 * @file test_hybrid_retention_manager.cpp
 * @brief Tests for HybridRetentionManager Gorilla compression and statistics
 */

#include <gtest/gtest.h>
#include "scheduler/hybrid_retention_manager.h"
#include "scheduler/task_scheduler.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "query/query_engine.h"
#include "timeseries/gorilla.h"
#include <filesystem>
#include <chrono>

using namespace themis;
using namespace std::chrono_literals;

// ===== GorillaEncoder compression ratio test =====

TEST(GorillaCompressionTest, CompressionRatioIsPositive) {
    GorillaEncoder encoder;
    int64_t ts = 1700000000000LL;
    double val = 100.0;
    for (int i = 0; i < 64; ++i) {
        encoder.add(ts, val);
        ts  += 10000;
        val += (i % 3 == 0) ? 0.5 : 0.0;
    }
    auto compressed = encoder.finish();
    EXPECT_GT(compressed.size(), 0u);

    double raw_bytes = 64.0 * 16.0;  // 16 bytes per point (timestamp + double)
    double ratio = raw_bytes / static_cast<double>(compressed.size());
    EXPECT_GT(ratio, 1.0);  // Compressed is always smaller than raw
}

TEST(GorillaCompressionTest, CompressionRatioExceedsTwo) {
    // Regular time-series data (constant step + minor drift) should compress well
    GorillaEncoder encoder;
    int64_t ts = 1700000000000LL;
    double val = 42.0;
    for (int i = 0; i < 128; ++i) {
        encoder.add(ts, val);
        ts  += 10000;
        val += (i % 5 == 0) ? 0.1 : 0.0;
    }
    auto compressed = encoder.finish();
    double raw_bytes = 128.0 * 16.0;
    double ratio = raw_bytes / static_cast<double>(compressed.size());
    // Gorilla should achieve at least 2x compression on this data
    EXPECT_GT(ratio, 2.0);
}

TEST(GorillaCompressionTest, DecodeRoundtrip) {
    GorillaEncoder encoder;
    std::vector<std::pair<int64_t, double>> original;
    int64_t ts = 1700000000000LL;
    double val = 100.0;
    for (int i = 0; i < 20; ++i) {
        encoder.add(ts, val);
        original.push_back({ts, val});
        ts  += 10000;
        val += 0.5;
    }
    auto compressed = encoder.finish();

    GorillaDecoder decoder(compressed);
    size_t idx = 0;
    while (true) {
        auto point = decoder.next();
        if (!point) {
          break;
        }
        ASSERT_LT(idx, original.size());
        EXPECT_EQ(point->first, original[idx].first);
        EXPECT_DOUBLE_EQ(point->second, original[idx].second);
        ++idx;
    }
    EXPECT_EQ(idx, original.size());
}

// ===== HybridRetentionManager tests =====

class HybridRetentionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        db_path_ = std::filesystem::temp_directory_path() /
                   std::filesystem::path("themis_retention_test_" + std::to_string(now));
        std::filesystem::create_directories(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_.string() + "/db";
        cfg.enable_blobdb = false;
        storage_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(storage_->open());

        idx_     = std::make_unique<SecondaryIndexManager>(*storage_);
        engine_  = std::make_unique<QueryEngine>(*storage_, *idx_);

        TaskScheduler::Config sched_cfg;
        sched_cfg.max_concurrent_tasks = 2;
        sched_cfg.check_interval = 50ms;
        sched_cfg.persist_tasks = false;
        sched_cfg.enable_audit_logging = false;
        sched_cfg.enable_anomaly_detection = false;
        scheduler_ = std::make_unique<TaskScheduler>(engine_.get(), sched_cfg);

        HybridRetentionConfig ret_cfg;
        ret_cfg.stage1.enabled = true;
        ret_cfg.stage2.enabled = false;
        ret_cfg.stage3.enabled = false;
        manager_ = std::make_unique<HybridRetentionManager>(
            engine_.get(), nullptr, scheduler_.get(), ret_cfg);
    }

    void TearDown() override {
        manager_.reset();
        scheduler_->stop();
        scheduler_.reset();
        engine_.reset();
        idx_.reset();
        storage_->close();
        storage_.reset();
        std::filesystem::remove_all(db_path_);
    }

    std::filesystem::path db_path_;
    std::unique_ptr<RocksDBWrapper> storage_;
    std::unique_ptr<SecondaryIndexManager> idx_;
    std::unique_ptr<QueryEngine> engine_;
    std::unique_ptr<TaskScheduler> scheduler_;
    std::unique_ptr<HybridRetentionManager> manager_;
};

TEST_F(HybridRetentionManagerTest, InitialStatsAreZero) {
    auto stats = manager_->getStats();
    EXPECT_EQ(stats.stage1.compressions_total, 0u);
    EXPECT_EQ(stats.stage1.compressions_failed, 0u);
}

TEST_F(HybridRetentionManagerTest, ResetStatsWorks) {
    manager_->resetStats();  // Should not throw
    auto stats = manager_->getStats();
    EXPECT_EQ(stats.stage1.compressions_total, 0u);
}

TEST_F(HybridRetentionManagerTest, CompressionRatioIsNotHardcoded) {
    // start() registers tasks in the scheduler (required before executeStage1)
    manager_->start();
    // Execute stage 1; result should contain a compression_ratio that
    // is computed dynamically (not the old hard-coded 10.5).
    // We verify it is a reasonable positive number.
    manager_->executeStage1();
    auto stats = manager_->getStats();
    // At least one attempt was made
    EXPECT_GE(stats.stage1.compressions_total, 1u);
    // avg_compression_ratio should be > 1.0 (Gorilla always compresses)
    // Allow for 0 if no data was compressed (empty DB) but it should never be exactly 10.5
    if (stats.stage1.avg_compression_ratio > 0.0) {
        EXPECT_NE(stats.stage1.avg_compression_ratio, 10.5);
        EXPECT_GT(stats.stage1.avg_compression_ratio, 1.0);
    }
    manager_->stop();
}

