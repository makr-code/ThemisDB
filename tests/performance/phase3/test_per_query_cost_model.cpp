// Unit tests for PerQueryCostModel (Phase 3, Issue #2419)
// Tests cover:
//  - Basic RAII timing (beginQuery / QueryGuard)
//  - Record accumulation and rolling window
//  - Statistics computation (avg, p50, p95, per-type)
//  - Calibration factor derivation
//  - Integration with OptimizerCostModel::calibrateCosts
//  - reset() clears state
//  - Explicit end() vs. destructor-triggered end()

#include "performance/phase3/per_query_cost_model.h"
#include "query/optimizer_cost_model.h"

#include <gtest/gtest.h>
#include <cmath>
#include <thread>
#include <chrono>

using namespace themis::performance::phase3;
using namespace themis;

// ============================================================
// Fixture
// ============================================================

class PerQueryCostModelTest : public ::testing::Test {
protected:
    PerQueryCostModel model;
};

// ============================================================
// Basic recording
// ============================================================

TEST_F(PerQueryCostModelTest, InitialStateEmpty) {
    EXPECT_EQ(model.totalQueries(), 0u);

    auto stats = model.getStats();
    EXPECT_EQ(stats.total_queries, 0u);
    EXPECT_DOUBLE_EQ(stats.avg_execution_time_ms, 0.0);

    auto records = model.getRecentRecords(10);
    EXPECT_TRUE(records.empty());
}

TEST_F(PerQueryCostModelTest, SingleQueryRecorded) {
    {
        auto guard = model.beginQuery("table_scan", 50.0);
        // Simulate a tiny workload
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        guard.end(100, 5);
    }

    EXPECT_EQ(model.totalQueries(), 1u);

    auto records = model.getRecentRecords(10);
    ASSERT_EQ(records.size(), 1u);

    EXPECT_EQ(records[0].query_type, "table_scan");
    EXPECT_GT(records[0].execution_time_ms, 0.0);
    EXPECT_EQ(records[0].rows_processed, 100u);
    EXPECT_EQ(records[0].pages_read, 5u);
    EXPECT_DOUBLE_EQ(records[0].estimated_cost, 50.0);
    EXPECT_GT(records[0].cycles_elapsed, 0u);
}

TEST_F(PerQueryCostModelTest, GuardAutoEndsOnDestruction) {
    // Do NOT call guard.end() explicitly – destructor should record it.
    {
        auto guard = model.beginQuery("index_scan", 20.0);
        (void)guard; // ensure the guard is used
    }

    EXPECT_EQ(model.totalQueries(), 1u);
    auto records = model.getRecentRecords(1);
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].query_type, "index_scan");
}

TEST_F(PerQueryCostModelTest, MultipleQueriesAccumulate) {
    for (int i = 0; i < 5; ++i) {
        auto guard = model.beginQuery("hash_join", 10.0 * (i + 1));
        guard.end(static_cast<size_t>(i + 1) * 10, 1);
    }

    EXPECT_EQ(model.totalQueries(), 5u);

    auto records = model.getRecentRecords(100);
    EXPECT_EQ(records.size(), 5u);
}

// ============================================================
// Rolling window
// ============================================================

TEST_F(PerQueryCostModelTest, RollingWindowLimitsRecords) {
    // Fill past MAX_RECORDS, using distinct estimated_cost to distinguish generations
    const size_t OVER = PerQueryCostModel::MAX_RECORDS + 10;
    for (size_t i = 0; i < OVER; ++i) {
        auto guard = model.beginQuery("scan", static_cast<double>(i));
        guard.end(1, 0);
    }

    EXPECT_EQ(model.totalQueries(), OVER);

    auto records = model.getRecentRecords(20);
    // After rollover, must not exceed MAX_RECORDS in the internal store
    EXPECT_LE(records.size(), 20u);
    EXPECT_EQ(records.size(), 20u);

    // The most recent records should have the largest estimated_cost values
    // (i.e., from the last 20 iterations: indices OVER-20 .. OVER-1)
    double first_expected_cost = static_cast<double>(OVER - 20);
    EXPECT_DOUBLE_EQ(records.front().estimated_cost, first_expected_cost);
    EXPECT_DOUBLE_EQ(records.back().estimated_cost, static_cast<double>(OVER - 1));
}

// ============================================================
// Statistics
// ============================================================

TEST_F(PerQueryCostModelTest, StatsAfterSomeQueries) {
    // Record 3 queries with distinct types and slight delays
    {
        auto g = model.beginQuery("table_scan", 10.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        g.end(50, 2);
    }
    {
        auto g = model.beginQuery("index_scan", 5.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        g.end(10, 1);
    }
    {
        auto g = model.beginQuery("table_scan", 15.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        g.end(80, 3);
    }

    auto stats = model.getStats();
    EXPECT_EQ(stats.total_queries, 3u);
    EXPECT_GT(stats.avg_execution_time_ms, 0.0);
    EXPECT_GE(stats.p50_execution_time_ms, 0.0);
    EXPECT_GE(stats.p95_execution_time_ms, stats.p50_execution_time_ms);
    EXPECT_GT(stats.per_type_count.count("table_scan"), 0u);
    EXPECT_EQ(stats.per_type_count.at("table_scan"), 2u);
    EXPECT_EQ(stats.per_type_count.at("index_scan"), 1u);
}

// ============================================================
// Calibration
// ============================================================

TEST_F(PerQueryCostModelTest, CalibrationFactorsEmptyModel) {
    // No records – should return empty map (no-op calibration)
    auto factors = model.getCalibrationFactors();
    EXPECT_TRUE(factors.empty());
}

TEST_F(PerQueryCostModelTest, CalibrationFactorsAfterRecords) {
    // Record queries with known row / page counts so factors can be derived
    for (int i = 0; i < 10; ++i) {
        auto g = model.beginQuery("table_scan", 1.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        g.end(1000, 50);
    }

    auto factors = model.getCalibrationFactors();
    // cpuCostPerRow should be present (rows_processed > 0 for every record)
    EXPECT_GT(factors.count("cpuCostPerRow"), 0u);
    // pageReadCost should be present (pages_read > 0 for every record)
    EXPECT_GT(factors.count("pageReadCost"), 0u);

    // Values should be within sane bounds
    EXPECT_GT(factors.at("cpuCostPerRow"), 0.0);
    EXPECT_LE(factors.at("cpuCostPerRow"), 1.0);
    EXPECT_GT(factors.at("pageReadCost"), 0.0);
}

TEST_F(PerQueryCostModelTest, CalibrateUpdatesOptimizerCostModel) {
    // Record some rows-heavy queries so that cpuCostPerRow can be derived
    for (int i = 0; i < 5; ++i) {
        auto g = model.beginQuery("table_scan", 2.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        g.end(500, 20);
    }

    OptimizerCostModel ocm;
    model.calibrate(ocm);

    // calibrate() must produce at least one calibration factor
    auto factors = model.getCalibrationFactors();
    EXPECT_FALSE(factors.empty());

    // Validate via public API: a table-scan estimate should produce
    // positive finite CPU and total cost after calibration.
    OptimizerCostModel::TableStatistics ts;
    ts.tableName = "t";
    ts.rowCount = 100;
    ts.pageCount = 10;
    ts.avgRowSize = 64.0;

    auto scan = ocm.estimateTableScan(ts);
    EXPECT_GT(scan.cpuCost, 0.0);
    EXPECT_GT(scan.totalCost, 0.0);
    EXPECT_TRUE(std::isfinite(scan.cpuCost));
    EXPECT_TRUE(std::isfinite(scan.totalCost));
}

// ============================================================
// Reset
// ============================================================

TEST_F(PerQueryCostModelTest, ResetClearsAllState) {
    for (int i = 0; i < 20; ++i) {
        auto g = model.beginQuery("scan", 1.0);
        g.end(10, 1);
    }
    EXPECT_EQ(model.totalQueries(), 20u);

    model.reset();

    EXPECT_EQ(model.totalQueries(), 0u);
    EXPECT_TRUE(model.getRecentRecords(100).empty());
    EXPECT_TRUE(model.getCalibrationFactors().empty());
}

// ============================================================
// Move semantics of QueryGuard
// ============================================================

TEST_F(PerQueryCostModelTest, MovedGuardDoesNotDoubleRecord) {
    auto g1 = model.beginQuery("sort_merge_join", 8.0);
    auto g2 = std::move(g1);   // move ownership

    // g1 is now in moved-from state; only g2's destructor should record
    g2.end(30, 2);

    EXPECT_EQ(model.totalQueries(), 1u);
}

// ============================================================
// Cost ratio
// ============================================================

TEST_F(PerQueryCostModelTest, CostRatioComputedCorrectly) {
    // estimated_cost = 0.0 should still produce a sane record
    {
        auto g = model.beginQuery("table_scan", 0.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        g.end(100, 5);
    }
    auto records = model.getRecentRecords(1);
    ASSERT_FALSE(records.empty());
    // When estimated_cost = 0, cost_ratio defaults to 1.0
    EXPECT_DOUBLE_EQ(records[0].cost_ratio, 1.0);
}

TEST_F(PerQueryCostModelTest, CostRatioNonZeroEstimate) {
    {
        auto g = model.beginQuery("table_scan", 10.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        g.end(100, 5);
    }
    auto records = model.getRecentRecords(1);
    ASSERT_FALSE(records.empty());
    EXPECT_GT(records[0].cost_ratio, 0.0);
}
