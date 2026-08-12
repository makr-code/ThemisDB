// Integration tests for per-query cost model + query optimizer (Phase 3, Issue #2419)
// Tests verify:
//  - attachPerQueryCostModel / perQueryCostModel() API
//  - AdaptiveQueryStats (recordQueryExecution / getAdaptiveAdjustment)
//  - Cost model calibration round-trip via PerQueryCostModel + OptimizerCostModel

#include <gtest/gtest.h>
#include "query/query_optimizer.h"
#include "query/adaptive_optimizer.h"
#include "query/optimizer_cost_model.h"
#include "performance/phase3/per_query_cost_model.h"
#include "index/secondary_index.h"

#include <memory>
#include <thread>
#include <chrono>

using namespace themis;
using namespace themis::query;
using namespace themis::performance::phase3;

// ============================================================
// Helper: minimal SecondaryIndexManager for tests that need it
// ============================================================

namespace {
    class OptimizerTestFixture : public ::testing::Test {
    protected:
        // We need a SecondaryIndexManager to construct QueryOptimizer.
        // Use a real RocksDB :memory: instance.
        void SetUp() override {
            RocksDBWrapper::Config cfg;
            cfg.db_path = ":memory:";
            db_ = std::make_unique<RocksDBWrapper>(cfg);
            sec_idx_ = std::make_unique<SecondaryIndexManager>(*db_);
            optimizer_ = std::make_unique<QueryOptimizer>(*sec_idx_);
        }

        std::unique_ptr<RocksDBWrapper> db_;
        std::unique_ptr<SecondaryIndexManager> sec_idx_;
        std::unique_ptr<QueryOptimizer> optimizer_;
    };
} // anonymous namespace

// ============================================================
// PerQueryCostModel attachment API
// ============================================================

TEST_F(OptimizerTestFixture, AttachNullptrDetachesModel) {
    // Initially no model is attached
    EXPECT_EQ(optimizer_->perQueryCostModel(), nullptr);

    // Attach a real model
    auto model = std::make_shared<PerQueryCostModel>();
    optimizer_->attachPerQueryCostModel(model);
    EXPECT_EQ(optimizer_->perQueryCostModel(), model);

    // Detach by passing nullptr
    optimizer_->attachPerQueryCostModel(nullptr);
    EXPECT_EQ(optimizer_->perQueryCostModel(), nullptr);
}

TEST_F(OptimizerTestFixture, AttachPerQueryCostModelRetainsPointer) {
    auto model = std::make_shared<PerQueryCostModel>();
    optimizer_->attachPerQueryCostModel(model);

    auto retrieved = optimizer_->perQueryCostModel();
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved.get(), model.get());
}

// ============================================================
// Adaptive query stats (AdaptiveQueryStats integration)
// ============================================================

TEST_F(OptimizerTestFixture, AdaptiveOptimizationRecordsExecution) {
    optimizer_->enableAdaptiveOptimization(true);
    EXPECT_TRUE(optimizer_->isAdaptiveOptimizationEnabled());

    // Record a query execution (no database needed for this API)
    optimizer_->recordQueryExecution("q_hash_1", 1000, 500, 2.5);

    // Adjustment factor should be available (initially close to smoothed value).
    // AdaptiveQueryStats uses exponential smoothing: factor = 0.7 * selectivity + 0.3.
    // With selectivity = 0.5, expected factor ≈ 0.65, which is within [0, 2.0].
    static constexpr double kMaxAdjustmentFactor = 2.0;
    double factor = optimizer_->getAdaptiveAdjustment("q_hash_1");
    EXPECT_GT(factor, 0.0);
    EXPECT_LE(factor, kMaxAdjustmentFactor);
}

TEST_F(OptimizerTestFixture, AdaptiveOptimizationDefaultFactor) {
    optimizer_->enableAdaptiveOptimization(true);

    // Unknown query hash should return default factor 1.0
    double factor = optimizer_->getAdaptiveAdjustment("unknown_hash");
    EXPECT_DOUBLE_EQ(factor, 1.0);
}

TEST_F(OptimizerTestFixture, AdaptiveOptimizationDisabledReturnsDefault) {
    // Adaptive optimization not enabled
    EXPECT_FALSE(optimizer_->isAdaptiveOptimizationEnabled());

    // recordQueryExecution is a no-op; getAdaptiveAdjustment returns 1.0
    optimizer_->recordQueryExecution("q1", 1000, 2000, 5.0);
    EXPECT_DOUBLE_EQ(optimizer_->getAdaptiveAdjustment("q1"), 1.0);
}

TEST_F(OptimizerTestFixture, AdaptiveFactorConvergesWithHistory) {
    optimizer_->enableAdaptiveOptimization(true);

    // Record many executions: actual is consistently 2x estimated.
    // 10 iterations are sufficient to converge the exponential smoothing average
    // to the true selectivity (actual/estimated = 2.0).
    static constexpr int kHistoryDepth = 10;
    for (int i = 0; i < kHistoryDepth; ++i) {
        optimizer_->recordQueryExecution("under_estimate", 500, 1000, 1.0);
    }

    double factor = optimizer_->getAdaptiveAdjustment("under_estimate");
    // Factor should be > 1.0 (model should adjust upward)
    EXPECT_GT(factor, 1.0);
}

// ============================================================
// PerQueryCostModel calibration round-trip
// ============================================================

TEST(PerQueryCostModelCalibrationIntegration, CalibratesOptimizerAfterExecution) {
    PerQueryCostModel pcm;
    OptimizerCostModel ocm;

    // Record several table-scan executions
    for (int i = 0; i < 5; ++i) {
        auto guard = pcm.beginQuery("table_scan", 5.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        guard.end(1000, 20);
    }

    // Calibrate the cost model from recorded history
    pcm.calibrate(ocm);

    // After calibration, a table scan estimate should produce finite costs
    OptimizerCostModel::TableStatistics ts;
    ts.tableName = "users";
    ts.rowCount = 10000;
    ts.pageCount = 100;
    ts.avgRowSize = 64.0;

    auto scan = ocm.estimateTableScan(ts);
    EXPECT_GT(scan.cpuCost, 0.0);
    EXPECT_GT(scan.totalCost, 0.0);
    EXPECT_TRUE(std::isfinite(scan.cpuCost));
    EXPECT_TRUE(std::isfinite(scan.totalCost));
}

TEST(PerQueryCostModelCalibrationIntegration, CalibrationFactorsMatchStats) {
    PerQueryCostModel pcm;

    // Record queries with consistent page reads
    for (int i = 0; i < 10; ++i) {
        auto guard = pcm.beginQuery("index_scan", 2.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        guard.end(100, 10);
    }

    auto factors = pcm.getCalibrationFactors();
    EXPECT_FALSE(factors.empty());
    EXPECT_GT(factors.count("pageReadCost"), 0u);
    EXPECT_GT(factors.count("cpuCostPerRow"), 0u);
}
