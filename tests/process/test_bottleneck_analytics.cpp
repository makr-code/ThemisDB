/*
 * ThemisDB - Process Modeling Module
 *
 * File:    test_bottleneck_analytics.cpp
 * Module:  tests/process/
 * Purpose: GTest unit tests BOT-01..BOT-08 for ProcessGraphRag bottleneck analytics.
 */

#include <gtest/gtest.h>

#include "index/process_graph.h"
#include "process/process_graph_rag.h"
#include "process/process_linker.h"
#include "process/process_model_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <algorithm>
#include <filesystem>
#include <numeric>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class BottleneckAnalyticsTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/test_bottleneck_" + std::to_string(::getpid());
        fs::remove_all(db_path_);

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path             = db_path_;
        cfg.memtable_size_mb    = 32;
        cfg.block_cache_size_mb = 64;
        cfg.max_background_jobs = 1;

        db_ = std::make_unique<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        themis::registerProcessEdgeTypes();

        mgr_    = std::make_unique<themis::process::ProcessModelManager>(*db_);
        linker_ = std::make_unique<themis::process::ProcessLinker>(*db_);
        engine_ = std::make_unique<themis::ProcessGraphManager>(*db_);

        rag_ = std::make_unique<themis::process::ProcessGraphRag>(
            *db_, *engine_, *mgr_, *linker_);
    }

    void TearDown() override {
        rag_.reset();
        engine_.reset();
        linker_.reset();
        mgr_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    std::string db_path_;
    std::unique_ptr<themis::RocksDBWrapper>               db_;
    std::unique_ptr<themis::process::ProcessModelManager> mgr_;
    std::unique_ptr<themis::process::ProcessLinker>       linker_;
    std::unique_ptr<themis::ProcessGraphManager> engine_;
    std::unique_ptr<themis::process::ProcessGraphRag>     rag_;
};

// ─────────────────────────────────────────────────────────────────────────────
// BOT-01: analyzeBottlenecks with no data → returns empty vector
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(BottleneckAnalyticsTest, BOT01_NoDataReturnsEmpty) {
    auto result = rag_->analyzeBottlenecks("model-nodata");
    EXPECT_TRUE(result.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// BOT-02: recordNodeCompletion once → analyzeBottlenecks returns 1 entry with correct avg
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(BottleneckAnalyticsTest, BOT02_SingleRecordCorrectAvg) {
    rag_->recordNodeCompletion("model-bot02", "node-A", "Task A", 500);
    auto result = rag_->analyzeBottlenecks("model-bot02");
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].node_id, "node-A");
    EXPECT_NEAR(result[0].avg_dwell_ms, 500.0, 0.001);
    EXPECT_EQ(result[0].sample_count, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// BOT-03: recordNodeCompletion 10x same node → avg computed correctly
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(BottleneckAnalyticsTest, BOT03_MultipleRecordsCorrectAvg) {
    // 10 completions: 100, 200, ..., 1000  → avg = 550
    for (int i = 1; i <= 10; ++i) {
        rag_->recordNodeCompletion("model-bot03", "node-B", "Task B",
                                   static_cast<int64_t>(i * 100));
    }
    auto result = rag_->analyzeBottlenecks("model-bot03");
    ASSERT_EQ(result.size(), 1u);
    EXPECT_NEAR(result[0].avg_dwell_ms, 550.0, 0.01);
    EXPECT_EQ(result[0].sample_count, 10u);
}

// ─────────────────────────────────────────────────────────────────────────────
// BOT-04: multiple nodes → analyzeBottlenecks returns sorted by avg descending
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(BottleneckAnalyticsTest, BOT04_MultiplNodesDescendingOrder) {
    rag_->recordNodeCompletion("model-bot04", "node-slow",   "Slow",   1000);
    rag_->recordNodeCompletion("model-bot04", "node-medium", "Medium",  500);
    rag_->recordNodeCompletion("model-bot04", "node-fast",   "Fast",    100);

    auto result = rag_->analyzeBottlenecks("model-bot04", 10);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_GE(result[0].avg_dwell_ms, result[1].avg_dwell_ms);
    EXPECT_GE(result[1].avg_dwell_ms, result[2].avg_dwell_ms);
    EXPECT_EQ(result[0].node_id, "node-slow");
    EXPECT_EQ(result[2].node_id, "node-fast");
}

// ─────────────────────────────────────────────────────────────────────────────
// BOT-05: top_n=2 with 5 nodes → returns exactly 2 entries
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(BottleneckAnalyticsTest, BOT05_TopNLimitsResults) {
    for (int i = 1; i <= 5; ++i) {
        rag_->recordNodeCompletion("model-bot05",
                                   "node-" + std::to_string(i),
                                   "Task " + std::to_string(i),
                                   static_cast<int64_t>(i * 200));
    }
    auto result = rag_->analyzeBottlenecks("model-bot05", 2);
    EXPECT_EQ(result.size(), 2u);
    // Top-2 should be node-5 (1000ms) and node-4 (800ms)
    EXPECT_EQ(result[0].node_id, "node-5");
    EXPECT_EQ(result[1].node_id, "node-4");
}

// ─────────────────────────────────────────────────────────────────────────────
// BOT-06: p95 computed from samples (inject 100 values, p95 ≈ 95th percentile)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(BottleneckAnalyticsTest, BOT06_P95Correct) {
    // Inject values 1..100; sorted p95 index = (100-1)*0.95 = 94 → value 95
    for (int i = 1; i <= 100; ++i) {
        rag_->recordNodeCompletion("model-bot06", "node-p95", "P95 Node",
                                   static_cast<int64_t>(i));
    }
    auto result = rag_->analyzeBottlenecks("model-bot06");
    ASSERT_EQ(result.size(), 1u);
    // p95 = samples sorted[94] = 95
    EXPECT_NEAR(result[0].p95_dwell_ms, 95.0, 0.001);
    EXPECT_EQ(result[0].sample_count, 100u);
}

// ─────────────────────────────────────────────────────────────────────────────
// BOT-07: recordNodeCompletion >200 samples → still works (rolling buffer)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(BottleneckAnalyticsTest, BOT07_RollingBufferOver200) {
    // Insert 250 samples: first 50 are 1ms, last 200 are 100ms
    for (int i = 0; i < 50; ++i) {
        rag_->recordNodeCompletion("model-bot07", "node-roll", "Roll Node", 1);
    }
    for (int i = 0; i < 200; ++i) {
        rag_->recordNodeCompletion("model-bot07", "node-roll", "Roll Node", 100);
    }
    auto result = rag_->analyzeBottlenecks("model-bot07");
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].sample_count, 250u);  // count is cumulative, not capped
    // p95 of rolling 200-sample buffer (all 100ms) should be 100
    EXPECT_NEAR(result[0].p95_dwell_ms, 100.0, 0.001);
}

// ─────────────────────────────────────────────────────────────────────────────
// BOT-08: analyzeBottlenecks for different model_id → no cross-contamination
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(BottleneckAnalyticsTest, BOT08_NoCrossContamination) {
    rag_->recordNodeCompletion("model-alpha", "shared-node", "Shared", 1000);
    rag_->recordNodeCompletion("model-beta",  "shared-node", "Shared", 50);

    auto alpha = rag_->analyzeBottlenecks("model-alpha");
    auto beta  = rag_->analyzeBottlenecks("model-beta");

    ASSERT_EQ(alpha.size(), 1u);
    ASSERT_EQ(beta.size(), 1u);

    EXPECT_NEAR(alpha[0].avg_dwell_ms, 1000.0, 0.001);
    EXPECT_NEAR(beta[0].avg_dwell_ms,    50.0, 0.001);
}
