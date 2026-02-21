/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_gpu_query_accelerator.cpp                     ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:08:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     329                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "themis/gpu/query_accelerator.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace themis::gpu;
using Row = GPUQueryAccelerator::Row;

// Helper – build a set of rows with sequential IDs and a single float payload.
static std::vector<Row> makeRows(size_t n, double base_value = 1.0) {
    std::vector<Row> rows(n);
    for (size_t i = 0; i < n; ++i) {
        rows[i].id = static_cast<uint64_t>(i);
        float v = static_cast<float>(base_value + static_cast<double>(i));
        rows[i].data.resize(sizeof(float));
        std::memcpy(rows[i].data.data(), &v, sizeof(float));
    }
    return rows;
}

// Helper – extract the float payload as a double.
static double payloadVal(const Row& r) {
    if (r.data.size() < sizeof(float)) return 0.0;
    float v;
    std::memcpy(&v, r.data.data(), sizeof(float));
    return static_cast<double>(v);
}

// ============================================================================
// Fixture
// ============================================================================

class GPUQueryAcceleratorTest : public ::testing::Test {
protected:
    GPUQueryAccelerator::Config cpuConfig() {
        GPUQueryAccelerator::Config cfg;
        cfg.force_cpu = true;
        return cfg;
    }
};

// ============================================================================
// Construction & stats
// ============================================================================

TEST_F(GPUQueryAcceleratorTest, DefaultConstructor) {
    GPUQueryAccelerator acc;
    auto s = acc.getStats();
    EXPECT_EQ(s.total_scans, 0u);
    EXPECT_EQ(s.total_sorts, 0u);
    EXPECT_EQ(s.total_aggregates, 0u);
    EXPECT_EQ(s.total_joins, 0u);
}

TEST_F(GPUQueryAcceleratorTest, ResetStats) {
    GPUQueryAccelerator acc(cpuConfig());
    acc.scan(makeRows(5));
    EXPECT_EQ(acc.getStats().total_scans, 1u);
    acc.resetStats();
    EXPECT_EQ(acc.getStats().total_scans, 0u);
}

// ============================================================================
// Scan
// ============================================================================

TEST_F(GPUQueryAcceleratorTest, ScanNoFilter) {
    GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows(10);
    auto result = acc.scan(rows);
    EXPECT_EQ(result.rows_scanned, 10u);
    EXPECT_EQ(result.rows_passed, 10u);
    EXPECT_EQ(result.rows.size(), 10u);
}

TEST_F(GPUQueryAcceleratorTest, ScanWithFilter) {
    GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows(10);
    // Pass only even IDs
    auto result = acc.scan(rows, [](const Row& r) { return r.id % 2 == 0; });
    EXPECT_EQ(result.rows_scanned, 10u);
    EXPECT_EQ(result.rows_passed, 5u);
    for (const auto& r : result.rows) {
        EXPECT_EQ(r.id % 2, 0u);
    }
}

TEST_F(GPUQueryAcceleratorTest, ScanEmptyInput) {
    GPUQueryAccelerator acc(cpuConfig());
    auto result = acc.scan({});
    EXPECT_EQ(result.rows_scanned, 0u);
    EXPECT_EQ(result.rows_passed, 0u);
    EXPECT_TRUE(result.rows.empty());
}

TEST_F(GPUQueryAcceleratorTest, ScanFilterAllOut) {
    GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows(5);
    auto result = acc.scan(rows, [](const Row&) { return false; });
    EXPECT_EQ(result.rows_passed, 0u);
    EXPECT_TRUE(result.rows.empty());
}

TEST_F(GPUQueryAcceleratorTest, ScanUpdatesStats) {
    GPUQueryAccelerator acc(cpuConfig());
    acc.scan(makeRows(3));
    acc.scan(makeRows(7));
    auto s = acc.getStats();
    EXPECT_EQ(s.total_scans, 2u);
    EXPECT_EQ(s.rows_processed, 10u);
}

// ============================================================================
// Sort
// ============================================================================

TEST_F(GPUQueryAcceleratorTest, SortAscending) {
    GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows(5, 5.0);  // values 5, 6, 7, 8, 9
    // Reverse them
    std::reverse(rows.begin(), rows.end());

    auto result = acc.sort(rows, payloadVal, GPUQueryAccelerator::SortOrder::ASC);
    ASSERT_EQ(result.rows.size(), 5u);
    for (size_t i = 1; i < result.rows.size(); ++i) {
        EXPECT_LE(payloadVal(result.rows[i - 1]), payloadVal(result.rows[i]));
    }
}

TEST_F(GPUQueryAcceleratorTest, SortDescending) {
    GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows(6);
    auto result = acc.sort(rows, payloadVal, GPUQueryAccelerator::SortOrder::DESC);
    ASSERT_EQ(result.rows.size(), 6u);
    for (size_t i = 1; i < result.rows.size(); ++i) {
        EXPECT_GE(payloadVal(result.rows[i - 1]), payloadVal(result.rows[i]));
    }
}

TEST_F(GPUQueryAcceleratorTest, SortSingleRow) {
    GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows(1);
    auto result = acc.sort(rows, payloadVal);
    EXPECT_EQ(result.rows.size(), 1u);
}

TEST_F(GPUQueryAcceleratorTest, SortUpdatesStats) {
    GPUQueryAccelerator acc(cpuConfig());
    acc.sort(makeRows(4), payloadVal);
    EXPECT_EQ(acc.getStats().total_sorts, 1u);
}

// ============================================================================
// Aggregate
// ============================================================================

TEST_F(GPUQueryAcceleratorTest, AggregateSUM) {
    GPUQueryAccelerator acc(cpuConfig());
    // values: 1, 2, 3, 4, 5  → sum = 15
    auto rows = makeRows(5, 1.0);
    auto res  = acc.aggregate(rows, GPUQueryAccelerator::AggFunc::SUM, payloadVal);
    EXPECT_NEAR(res.value, 15.0, 0.01);
    EXPECT_EQ(res.count, 5u);
}

TEST_F(GPUQueryAcceleratorTest, AggregateCOUNT) {
    GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows(7);
    auto res  = acc.aggregate(rows, GPUQueryAccelerator::AggFunc::COUNT, payloadVal);
    EXPECT_NEAR(res.value, 7.0, 0.01);
}

TEST_F(GPUQueryAcceleratorTest, AggregateMIN) {
    GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows(5, 3.0);  // values 3..7
    auto res  = acc.aggregate(rows, GPUQueryAccelerator::AggFunc::MIN, payloadVal);
    EXPECT_NEAR(res.value, 3.0, 0.01);
}

TEST_F(GPUQueryAcceleratorTest, AggregateMAX) {
    GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows(5, 3.0);
    auto res  = acc.aggregate(rows, GPUQueryAccelerator::AggFunc::MAX, payloadVal);
    EXPECT_NEAR(res.value, 7.0, 0.01);
}

TEST_F(GPUQueryAcceleratorTest, AggregateAVG) {
    GPUQueryAccelerator acc(cpuConfig());
    // values 1,2,3,4,5 → avg = 3
    auto rows = makeRows(5, 1.0);
    auto res  = acc.aggregate(rows, GPUQueryAccelerator::AggFunc::AVG, payloadVal);
    EXPECT_NEAR(res.value, 3.0, 0.01);
}

TEST_F(GPUQueryAcceleratorTest, AggregateEmptyRows) {
    GPUQueryAccelerator acc(cpuConfig());
    auto res = acc.aggregate({}, GPUQueryAccelerator::AggFunc::SUM, payloadVal);
    EXPECT_EQ(res.count, 0u);
    EXPECT_NEAR(res.value, 0.0, 0.01);
}

TEST_F(GPUQueryAcceleratorTest, AggregateUpdatesStats) {
    GPUQueryAccelerator acc(cpuConfig());
    acc.aggregate(makeRows(3), GPUQueryAccelerator::AggFunc::SUM, payloadVal);
    acc.aggregate(makeRows(3), GPUQueryAccelerator::AggFunc::AVG, payloadVal);
    EXPECT_EQ(acc.getStats().total_aggregates, 2u);
}

// ============================================================================
// Hash join
// ============================================================================

// Build rows with ID as join key
static std::vector<Row> makeJoinRows(
    std::initializer_list<uint64_t> ids) {
    std::vector<Row> rows;
    for (uint64_t id : ids) {
        Row r;
        r.id = id;
        r.data.resize(sizeof(uint64_t));
        std::memcpy(r.data.data(), &id, sizeof(uint64_t));
        rows.push_back(r);
    }
    return rows;
}

TEST_F(GPUQueryAcceleratorTest, HashJoinBasic) {
    GPUQueryAccelerator acc(cpuConfig());
    auto left  = makeJoinRows({1, 2, 3});
    auto right = makeJoinRows({2, 3, 4});

    auto result = acc.hashJoin(left, right,
                               [](const Row& r) { return r.id; },
                               [](const Row& r) { return r.id; });
    EXPECT_EQ(result.pairs.size(), 2u);  // ids 2 and 3
}

TEST_F(GPUQueryAcceleratorTest, HashJoinNoMatch) {
    GPUQueryAccelerator acc(cpuConfig());
    auto left  = makeJoinRows({1, 2});
    auto right = makeJoinRows({3, 4});
    auto result = acc.hashJoin(left, right,
                               [](const Row& r) { return r.id; },
                               [](const Row& r) { return r.id; });
    EXPECT_TRUE(result.pairs.empty());
}

TEST_F(GPUQueryAcceleratorTest, HashJoinLeftEmpty) {
    GPUQueryAccelerator acc(cpuConfig());
    auto right = makeJoinRows({1, 2});
    auto result = acc.hashJoin({}, right,
                               [](const Row& r) { return r.id; },
                               [](const Row& r) { return r.id; });
    EXPECT_TRUE(result.pairs.empty());
}

TEST_F(GPUQueryAcceleratorTest, HashJoinAllMatch) {
    GPUQueryAccelerator acc(cpuConfig());
    auto left  = makeJoinRows({1, 2, 3});
    auto right = makeJoinRows({1, 2, 3});
    auto result = acc.hashJoin(left, right,
                               [](const Row& r) { return r.id; },
                               [](const Row& r) { return r.id; });
    EXPECT_EQ(result.pairs.size(), 3u);
}

TEST_F(GPUQueryAcceleratorTest, HashJoinUpdatesStats) {
    GPUQueryAccelerator acc(cpuConfig());
    acc.hashJoin(makeJoinRows({1}), makeJoinRows({1}),
                 [](const Row& r) { return r.id; },
                 [](const Row& r) { return r.id; });
    EXPECT_EQ(acc.getStats().total_joins, 1u);
}

// ============================================================================
// GPU threshold logic
// ============================================================================

TEST_F(GPUQueryAcceleratorTest, BelowThresholdUsesCPU) {
    GPUQueryAccelerator::Config cfg;
    cfg.gpu_threshold_rows = 1000;
    cfg.force_cpu          = false;
    GPUQueryAccelerator acc(cfg);

    auto rows = makeRows(5);   // well below threshold
    auto result = acc.scan(rows);
    // With no real GPU present the logic falls through to CPU regardless,
    // but the used_gpu flag tracks the dispatch decision.
    EXPECT_FALSE(result.used_gpu);
}

TEST_F(GPUQueryAcceleratorTest, ForceCPUOverridesThreshold) {
    GPUQueryAccelerator::Config cfg;
    cfg.gpu_threshold_rows = 1;
    cfg.force_cpu          = true;
    GPUQueryAccelerator acc(cfg);

    auto rows = makeRows(50'000);
    auto result = acc.scan(rows);
    EXPECT_FALSE(result.used_gpu);
    EXPECT_EQ(acc.getStats().cpu_fallback_ops, 1u);
    EXPECT_EQ(acc.getStats().gpu_ops, 0u);
}
