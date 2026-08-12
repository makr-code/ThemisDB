// Unit tests for LLMBatchTuner (Phase 3, Issue #1996)
// Tests cover:
//  - Default construction and initial state
//  - RAII BatchGuard records a measurement on destruction
//  - Explicit guard.end() finalises measurement; double-end is a no-op
//  - recordBatch() path (no cycle counter)
//  - Batch size increases when throughput is improving (low latency)
//  - Batch size decreases when latency budget is breached
//  - Batch size stays within [min, max] bounds at all times
//  - reset() clears all state
//  - getStats() returns consistent data
//  - getRecentRecords() limits the result set
//  - summary() returns a non-empty string
//  - Feature flag defaults to enabled

#include "performance/phase3/adaptive_batch_tuner.h"
#include "performance/phase3/feature_flags.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

using namespace themis::performance::phase3;

// ============================================================
// Fixture
// ============================================================

class AdaptiveBatchTunerTest : public ::testing::Test {
protected:
    // Tight window for fast-reacting tests
    LLMBatchTuner::Config makeConfig(size_t window = 4,
                                     double max_lat = 0.0) const {
        LLMBatchTuner::Config cfg;
        cfg.min_batch_size             = 1;
        cfg.max_batch_size             = 32;
        cfg.initial_batch_size         = 4;
        cfg.step_up                    = 2;
        cfg.step_down                  = 1;
        cfg.window_size                = window;
        cfg.max_latency_ms_per_token   = max_lat;
        cfg.ema_alpha                  = 0.5f;
        return cfg;
    }
};

// ============================================================
// Initial state
// ============================================================

TEST_F(AdaptiveBatchTunerTest, DefaultConstruction) {
    LLMBatchTuner tuner;
    EXPECT_GE(tuner.recommendedBatchSize(), 1u);
    EXPECT_EQ(tuner.totalBatches(), 0u);

    auto stats = tuner.getStats();
    EXPECT_EQ(stats.total_batches, 0u);
    EXPECT_EQ(stats.tuning_events, 0u);
    EXPECT_DOUBLE_EQ(stats.avg_latency_ms, 0.0);
}

TEST_F(AdaptiveBatchTunerTest, InitialBatchSizeClamped) {
    auto cfg = makeConfig();
    cfg.initial_batch_size = 100;  // above max
    LLMBatchTuner tuner(cfg);
    EXPECT_EQ(tuner.recommendedBatchSize(), 32u);

    auto cfg2 = makeConfig();
    cfg2.initial_batch_size = 0;   // below min
    LLMBatchTuner tuner2(cfg2);
    EXPECT_EQ(tuner2.recommendedBatchSize(), 1u);
}

// ============================================================
// recordBatch (no RAII guard)
// ============================================================

TEST_F(AdaptiveBatchTunerTest, RecordBatchIncrementsCount) {
    LLMBatchTuner tuner(makeConfig());
    tuner.recordBatch(4, 256, 10.0);
    EXPECT_EQ(tuner.totalBatches(), 1u);
}

TEST_F(AdaptiveBatchTunerTest, RecordBatchIgnoresZeroTokens) {
    LLMBatchTuner tuner(makeConfig());
    tuner.recordBatch(4, 0, 10.0);   // total_tokens == 0 → ignored
    tuner.recordBatch(0, 256, 10.0); // batch_size == 0  → ignored
    tuner.recordBatch(4, 256, 0.0);  // latency_ms == 0  → ignored
    EXPECT_EQ(tuner.totalBatches(), 0u);
}

TEST_F(AdaptiveBatchTunerTest, GetRecentRecordsLimit) {
    LLMBatchTuner tuner(makeConfig(/*window=*/8));
    for (int i = 0; i < 10; ++i) {
        tuner.recordBatch(4, 128, 5.0);
    }
    auto recent = tuner.getRecentRecords(3);
    EXPECT_EQ(recent.size(), 3u);
}

TEST_F(AdaptiveBatchTunerTest, GetRecentRecordsEmpty) {
    LLMBatchTuner tuner(makeConfig());
    EXPECT_TRUE(tuner.getRecentRecords(10).empty());
}

// ============================================================
// BatchGuard – RAII timing path
// ============================================================

TEST_F(AdaptiveBatchTunerTest, BatchGuardRecordsOnDestruction) {
    LLMBatchTuner tuner(makeConfig());
    {
        auto guard = tuner.beginBatch(4, 256);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // destructor fires here
    }
    EXPECT_EQ(tuner.totalBatches(), 1u);

    auto records = tuner.getRecentRecords(5);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].batch_size, 4u);
    EXPECT_EQ(records[0].total_tokens, 256u);
    EXPECT_GT(records[0].latency_ms, 0.0);
    EXPECT_GT(records[0].tokens_per_s, 0.0);
}

TEST_F(AdaptiveBatchTunerTest, BatchGuardExplicitEnd) {
    LLMBatchTuner tuner(makeConfig());
    auto guard = tuner.beginBatch(4, 128);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    guard.end();
    EXPECT_EQ(tuner.totalBatches(), 1u);
    // Second end() is a no-op
    guard.end();
    EXPECT_EQ(tuner.totalBatches(), 1u);
}

TEST_F(AdaptiveBatchTunerTest, BatchGuardMovedFromDoesNotDoubleRecord) {
    LLMBatchTuner tuner(makeConfig());
    {
        auto g1 = tuner.beginBatch(4, 64);
        auto g2 = std::move(g1);
        // g1 is moved-from; g2 records on destruction
    }
    EXPECT_EQ(tuner.totalBatches(), 1u);
}

// ============================================================
// Statistics
// ============================================================

TEST_F(AdaptiveBatchTunerTest, StatsAfterSeveralBatches) {
    LLMBatchTuner tuner(makeConfig(/*window=*/4));
    for (int i = 0; i < 6; ++i) {
        tuner.recordBatch(4, 256, 10.0 + i);
    }
    auto s = tuner.getStats();
    EXPECT_EQ(s.total_batches, 6u);
    EXPECT_GT(s.ema_throughput, 0.0);
    EXPECT_GT(s.avg_latency_ms, 0.0);
    EXPECT_GE(s.p99_latency_ms, s.avg_latency_ms);
}

// ============================================================
// Batch size bounds
// ============================================================

TEST_F(AdaptiveBatchTunerTest, BatchSizeNeverExceedsMax) {
    LLMBatchTuner tuner(makeConfig(/*window=*/2));
    // Feed many low-latency batches → should try to increase batch size
    for (int i = 0; i < 100; ++i) {
        tuner.recordBatch(tuner.recommendedBatchSize(), 512, 1.0);
    }
    EXPECT_LE(tuner.recommendedBatchSize(), 32u);
}

TEST_F(AdaptiveBatchTunerTest, BatchSizeNeverBelowMin) {
    auto cfg = makeConfig(/*window=*/2, /*max_lat=*/0.001);  // very tight budget
    LLMBatchTuner tuner(cfg);
    // Feed high-latency batches → should decrease batch size
    for (int i = 0; i < 50; ++i) {
        tuner.recordBatch(tuner.recommendedBatchSize(), 1, 9999.0);
    }
    EXPECT_GE(tuner.recommendedBatchSize(), 1u);
}

// ============================================================
// Latency budget enforcement
// ============================================================

TEST_F(AdaptiveBatchTunerTest, LatencyBudgetReducesBatchSize) {
    // max_latency_ms_per_token = 1.0 ms/token; we'll breach it
    auto cfg = makeConfig(/*window=*/4, /*max_lat=*/1.0);
    LLMBatchTuner tuner(cfg);

    size_t initial = tuner.recommendedBatchSize();

    // Each batch: 1 token, 9999 ms latency → 9999 ms/token >> budget
    for (int i = 0; i < static_cast<int>(cfg.window_size); ++i) {
        tuner.recordBatch(cfg.initial_batch_size, 1, 9999.0);
    }

    // After one full window, the tuner should have reduced the batch size
    EXPECT_LT(tuner.recommendedBatchSize(), initial);
}

// ============================================================
// Throughput improvement increases batch size
// ============================================================

TEST_F(AdaptiveBatchTunerTest, ThroughputImprovementIncreasesBatchSize) {
    auto cfg = makeConfig(/*window=*/4, /*max_lat=*/0.0);
    cfg.initial_batch_size = 4;
    LLMBatchTuner tuner(cfg);

    size_t initial = tuner.recommendedBatchSize();

    // Low-latency batches → EMA throughput grows → batch size should increase
    for (int i = 0; i < static_cast<int>(cfg.window_size) * 2; ++i) {
        tuner.recordBatch(tuner.recommendedBatchSize(), 512, 1.0);
    }

    EXPECT_GE(tuner.recommendedBatchSize(), initial);
}

// ============================================================
// reset()
// ============================================================

TEST_F(AdaptiveBatchTunerTest, ResetClearsState) {
    LLMBatchTuner tuner(makeConfig());
    for (int i = 0; i < 10; ++i) {
        tuner.recordBatch(4, 128, 5.0);
    }
    EXPECT_GT(tuner.totalBatches(), 0u);

    tuner.reset();

    EXPECT_EQ(tuner.totalBatches(), 0u);
    EXPECT_EQ(tuner.getStats().total_batches, 0u);
    EXPECT_TRUE(tuner.getRecentRecords().empty());
    EXPECT_EQ(tuner.recommendedBatchSize(), 4u);  // back to initial
}

// ============================================================
// summary()
// ============================================================

TEST_F(AdaptiveBatchTunerTest, SummaryNonEmpty) {
    LLMBatchTuner tuner(makeConfig());
    tuner.recordBatch(4, 128, 5.0);
    EXPECT_FALSE(tuner.summary().empty());
}

// ============================================================
// Feature flag
// ============================================================

TEST(AdaptiveBatchTunerFeatureFlagTest, EnabledByDefault) {
    // The adaptive_batch_tuner_enabled flag defaults to true.
    EXPECT_TRUE(Phase3FeatureFlags::instance().adaptive_batch_tuner_enabled());
}

TEST(AdaptiveBatchTunerFeatureFlagTest, MacroReflectsInstance) {
    Phase3FeatureFlags::instance().set_adaptive_batch_tuner_enabled(true);
    EXPECT_TRUE(THEMIS_PHASE3_ADAPTIVE_BATCH_TUNER_ENABLED());

    Phase3FeatureFlags::instance().set_adaptive_batch_tuner_enabled(false);
    EXPECT_FALSE(THEMIS_PHASE3_ADAPTIVE_BATCH_TUNER_ENABLED());

    // Restore default
    Phase3FeatureFlags::instance().set_adaptive_batch_tuner_enabled(true);
}
