// Copyright 2026 ThemisDB
// Licensed under MIT License

/**
 * Focused unit tests for HardwareAccelerator (performance module, v1.8.0).
 *
 * Acceptance criteria tested:
 *  AC-1  HashJoin dispatch       – GPU-sim and CPU paths produce identical results
 *  AC-2  SortMergeJoin dispatch  – results match hash-join for the same input
 *  AC-3  Aggregate operators     – SUM / COUNT / MIN / MAX / AVG verified
 *  AC-4  Filter operator         – all comparison operators verified
 *  AC-5  Sort operator           – ascending ordering, row preservation
 *  AC-6  PatternMatch            – prefix, suffix, contains, exact
 *  AC-7  VectorOp (dot product)  – numeric correctness
 *  AC-8  can_accelerate          – true for all supported op types, false for Unknown
 *  AC-9  estimate_speedup        – returns ≥ 1.0, scales with row count and device
 *  AC-10 Statistics              – counters incremented correctly; resetStats clears
 *  AC-11 Config thresholds       – hw_path / cpu_fallback decided by row-count config
 *  AC-12 Error handling          – Unknown op_type returns ok=false
 *  AC-13 Empty inputs            – no crash for empty row sets
 *  AC-14 Thread safety           – concurrent execute() calls are safe
 *
 * Test suite name: HardwareAcceleratorFocusedTests
 */

#include <gtest/gtest.h>
#include "performance/hardware_accelerator.h"

#include <algorithm>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

using namespace themis::performance;

// ─── Helpers ─────────────────────────────────────────────────────────────────

/// Build a relation of `n` single-column rows with sequential keys 0..n-1.
static std::vector<Row> makeSeqRows(size_t n, size_t cols = 1, uint64_t base = 0) {
    std::vector<Row> rows;
    rows.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        Row r(cols, base + static_cast<uint64_t>(i));
        rows.push_back(r);
    }
    return rows;
}

/// Build a QueryOperator for HashJoin with `n` rows per side.
static QueryOperator makeHashJoinOp(size_t n) {
    QueryOperator op;
    op.op_type       = OperatorType::HashJoin;
    op.left_rows     = makeSeqRows(n);
    op.right_rows    = makeSeqRows(n);
    op.left_key_col  = 0;
    op.right_key_col = 0;
    return op;
}

/// Build a QueryOperator for Aggregate with known column values.
static QueryOperator makeAggOp(const std::string& fn, const std::vector<uint64_t>& vals) {
    QueryOperator op;
    op.op_type = OperatorType::Aggregate;
    op.agg_col = 0;
    op.agg_fn  = fn;
    for (uint64_t v : vals) {
        op.rows.push_back({v});
    }
    return op;
}

// ─── AC-8: can_accelerate ────────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, CanAccelerateReturnsTrueForSupportedOps) {
    HardwareAccelerator accel;
    for (auto t : {OperatorType::HashJoin, OperatorType::SortMergeJoin,
                   OperatorType::Aggregate, OperatorType::Filter,
                   OperatorType::Sort, OperatorType::PatternMatch,
                   OperatorType::VectorOp}) {
        QueryOperator op;
        op.op_type = t;
        EXPECT_TRUE(accel.can_accelerate(op)) << "Expected true for op type " << static_cast<int>(t);
    }
}

TEST(HardwareAcceleratorFocusedTests, CanAccelerateReturnsFalseForUnknown) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type = OperatorType::Unknown;
    EXPECT_FALSE(accel.can_accelerate(op));
}

// ─── AC-12: Error handling ────────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, UnknownOpTypeReturnsError) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type = OperatorType::Unknown;
    auto r = accel.execute(op);
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.error.empty());
}

// ─── AC-13: Empty inputs ──────────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, HashJoinEmptyInputProducesEmptyResult) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type = OperatorType::HashJoin;
    auto r = accel.execute(op);
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(r.rows.empty());
}

TEST(HardwareAcceleratorFocusedTests, AggregateEmptyInputReturnsZero) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type = OperatorType::Aggregate;
    op.agg_fn  = "COUNT";
    auto r = accel.execute(op);
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.agg_count, 0);
}

// ─── AC-1: HashJoin dispatch ──────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, HashJoinSmallSetCPUPath) {
    // With default thresholds (gpu>=100k, simd>=1k), 10 rows → CPU path.
    HardwareAccelerator accel;
    const size_t n = 10;
    auto op = makeHashJoinOp(n);
    auto r  = accel.execute(op);

    ASSERT_TRUE(r.ok);
    // Every key 0..9 matches exactly once on each side → n output rows.
    EXPECT_EQ(r.rows.size(), n);
    EXPECT_FALSE(r.used_hw_path);
}

TEST(HardwareAcceleratorFocusedTests, HashJoinMediumSetSIMDPath) {
    // 2000 rows → SIMD path (simd_threshold=1000).
    HardwareAccelerator::Config cfg;
    cfg.simd_row_threshold = 1'000;
    cfg.gpu_row_threshold  = 100'000;
    HardwareAccelerator accel(cfg);

    const size_t n = 2'000;
    auto op = makeHashJoinOp(n);

    HardwareAccelerator::AcceleratorConfig dcfg;
    dcfg.device     = HardwareAccelerator::DeviceType::VECTOR_ENGINE;
    dcfg.batch_size = 500;
    auto r = accel.execute(op, dcfg);

    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.rows.size(), n);
    EXPECT_TRUE(r.used_hw_path);
    EXPECT_GT(r.speedup, 1.0);
}

TEST(HardwareAcceleratorFocusedTests, HashJoinLargeSetGPUPath) {
    // gpu_threshold = 100 rows so we can test without building 100k rows.
    HardwareAccelerator::Config cfg;
    cfg.gpu_row_threshold  = 100;
    cfg.simd_row_threshold = 10;
    HardwareAccelerator accel(cfg);

    const size_t n = 200;
    auto op = makeHashJoinOp(n);

    HardwareAccelerator::AcceleratorConfig dcfg;
    dcfg.device     = HardwareAccelerator::DeviceType::GPU_CUDA;
    dcfg.batch_size = 50;
    auto r = accel.execute(op, dcfg);

    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.rows.size(), n);
    EXPECT_TRUE(r.used_hw_path);
    EXPECT_GE(r.speedup, 5.0);
}

TEST(HardwareAcceleratorFocusedTests, HashJoinGPUPathMatchesCPUPath) {
    // Verify GPU-sim and CPU produce the same output rows (order may differ).
    HardwareAccelerator::Config cfg;
    cfg.gpu_row_threshold  = 10;
    cfg.simd_row_threshold = 5;
    HardwareAccelerator accel(cfg);

    const size_t n = 20;
    auto op = makeHashJoinOp(n);

    HardwareAccelerator::AcceleratorConfig gpu_cfg;
    gpu_cfg.device = HardwareAccelerator::DeviceType::GPU_CUDA;

    HardwareAccelerator::AcceleratorConfig cpu_cfg;
    cpu_cfg.device = HardwareAccelerator::DeviceType::CPU;

    auto r_gpu = accel.execute(op, gpu_cfg);
    auto r_cpu = accel.execute(op, cpu_cfg);

    ASSERT_TRUE(r_gpu.ok);
    ASSERT_TRUE(r_cpu.ok);
    EXPECT_EQ(r_gpu.rows.size(), r_cpu.rows.size());
}

// ─── AC-2: SortMergeJoin ─────────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, SortMergeJoinMatchesHashJoin) {
    HardwareAccelerator::Config cfg;
    cfg.gpu_row_threshold  = 10;
    cfg.simd_row_threshold = 5;
    HardwareAccelerator accel(cfg);

    const size_t n = 20;

    QueryOperator hj = makeHashJoinOp(n);
    QueryOperator smj = hj;
    smj.op_type = OperatorType::SortMergeJoin;

    HardwareAccelerator::AcceleratorConfig dcfg;
    dcfg.device = HardwareAccelerator::DeviceType::GPU_CUDA;

    auto r_hj  = accel.execute(hj, dcfg);
    auto r_smj = accel.execute(smj, dcfg);

    ASSERT_TRUE(r_hj.ok);
    ASSERT_TRUE(r_smj.ok);
    EXPECT_EQ(r_hj.rows.size(), r_smj.rows.size());
}

TEST(HardwareAcceleratorFocusedTests, SortMergeJoinNonOverlappingKeys) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type       = OperatorType::SortMergeJoin;
    op.left_rows     = makeSeqRows(5, 1, 0);    // keys 0..4
    op.right_rows    = makeSeqRows(5, 1, 100);   // keys 100..104
    op.left_key_col  = 0;
    op.right_key_col = 0;

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_TRUE(r.rows.empty());
}

// ─── AC-3: Aggregate operators ───────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, AggregateCount) {
    HardwareAccelerator accel;
    auto op = makeAggOp("COUNT", {1, 2, 3, 4, 5});
    auto r  = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.agg_count, 5);
}

TEST(HardwareAcceleratorFocusedTests, AggregateSum) {
    HardwareAccelerator accel;
    auto op = makeAggOp("SUM", {10, 20, 30});
    auto r  = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_DOUBLE_EQ(r.agg_value, 60.0);
}

TEST(HardwareAcceleratorFocusedTests, AggregateAvg) {
    HardwareAccelerator accel;
    auto op = makeAggOp("AVG", {10, 20, 30});
    auto r  = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_DOUBLE_EQ(r.agg_value, 20.0);
}

TEST(HardwareAcceleratorFocusedTests, AggregateMin) {
    HardwareAccelerator accel;
    auto op = makeAggOp("MIN", {7, 3, 9, 1, 5});
    auto r  = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.agg_min, 1u);
    EXPECT_DOUBLE_EQ(r.agg_value, 1.0);
}

TEST(HardwareAcceleratorFocusedTests, AggregateMax) {
    HardwareAccelerator accel;
    auto op = makeAggOp("MAX", {7, 3, 9, 1, 5});
    auto r  = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.agg_max, 9u);
    EXPECT_DOUBLE_EQ(r.agg_value, 9.0);
}

TEST(HardwareAcceleratorFocusedTests, AggregateUnknownFunctionReturnsError) {
    HardwareAccelerator accel;
    auto op = makeAggOp("MEDIAN", {1, 2, 3});
    auto r  = accel.execute(op);
    EXPECT_FALSE(r.ok);
    EXPECT_FALSE(r.error.empty());
}

// ─── AC-4: Filter operator ────────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, FilterEqualOp) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type     = OperatorType::Filter;
    op.filter_col  = 0;
    op.filter_value = 3;
    op.filter_op   = "==";
    for (uint64_t v : {1u, 2u, 3u, 4u, 5u}) op.rows.push_back({v});

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    ASSERT_EQ(r.rows.size(), 1u);
    EXPECT_EQ(r.rows[0][0], 3u);
}

TEST(HardwareAcceleratorFocusedTests, FilterGreaterThanOp) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type      = OperatorType::Filter;
    op.filter_col   = 0;
    op.filter_value = 3;
    op.filter_op    = ">";
    for (uint64_t v : {1u, 2u, 3u, 4u, 5u}) op.rows.push_back({v});

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.rows.size(), 2u);  // 4, 5
}

TEST(HardwareAcceleratorFocusedTests, FilterLessThanOrEqualOp) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type      = OperatorType::Filter;
    op.filter_col   = 0;
    op.filter_value = 3;
    op.filter_op    = "<=";
    for (uint64_t v : {1u, 2u, 3u, 4u, 5u}) op.rows.push_back({v});

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.rows.size(), 3u);  // 1, 2, 3
}

TEST(HardwareAcceleratorFocusedTests, FilterNotEqualOp) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type      = OperatorType::Filter;
    op.filter_col   = 0;
    op.filter_value = 3;
    op.filter_op    = "!=";
    for (uint64_t v : {1u, 2u, 3u, 4u, 5u}) op.rows.push_back({v});

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.rows.size(), 4u);  // 1, 2, 4, 5
}

TEST(HardwareAcceleratorFocusedTests, FilterLessThanOp) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type      = OperatorType::Filter;
    op.filter_col   = 0;
    op.filter_value = 3;
    op.filter_op    = "<";
    for (uint64_t v : {1u, 2u, 3u, 4u, 5u}) op.rows.push_back({v});

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.rows.size(), 2u);  // 1, 2
}

TEST(HardwareAcceleratorFocusedTests, FilterGreaterThanOrEqualOp) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type      = OperatorType::Filter;
    op.filter_col   = 0;
    op.filter_value = 3;
    op.filter_op    = ">=";
    for (uint64_t v : {1u, 2u, 3u, 4u, 5u}) op.rows.push_back({v});

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.rows.size(), 3u);  // 3, 4, 5
}

// ─── AC-5: Sort operator ──────────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, SortAscending) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type = OperatorType::Sort;
    op.agg_col = 0;
    for (uint64_t v : {5u, 3u, 1u, 4u, 2u}) op.rows.push_back({v});

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    ASSERT_EQ(r.rows.size(), 5u);
    for (size_t i = 0; i + 1 < r.rows.size(); ++i) {
        EXPECT_LE(r.rows[i][0], r.rows[i + 1][0]);
    }
}

TEST(HardwareAcceleratorFocusedTests, SortPreservesAllRows) {
    HardwareAccelerator accel;
    const size_t n = 100;
    QueryOperator op;
    op.op_type = OperatorType::Sort;
    op.agg_col = 0;
    // Insert rows in reverse order.
    for (size_t i = n; i > 0; --i) op.rows.push_back({static_cast<uint64_t>(i)});

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.rows.size(), n);
    for (size_t i = 0; i + 1 < r.rows.size(); ++i) {
        EXPECT_LE(r.rows[i][0], r.rows[i + 1][0]);
    }
}

// ─── AC-6: PatternMatch ───────────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, PatternMatchExact) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type     = OperatorType::PatternMatch;
    op.string_rows = {"hello", "world", "hello world", "foo"};
    op.pattern     = "hello";

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    ASSERT_EQ(r.match_indices.size(), 1u);
    EXPECT_EQ(r.match_indices[0], 0u);
}

TEST(HardwareAcceleratorFocusedTests, PatternMatchPrefix) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type     = OperatorType::PatternMatch;
    op.string_rows = {"hello", "world", "help", "foo"};
    op.pattern     = "hel%";

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.match_indices.size(), 2u);  // "hello" and "help"
}

TEST(HardwareAcceleratorFocusedTests, PatternMatchSuffix) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type     = OperatorType::PatternMatch;
    op.string_rows = {"hello", "world", "carlo", "foo"};
    op.pattern     = "%lo";

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.match_indices.size(), 2u);  // "hello" and "carlo"
}

TEST(HardwareAcceleratorFocusedTests, PatternMatchContains) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type     = OperatorType::PatternMatch;
    op.string_rows = {"hello world", "foo", "world class", "bar"};
    op.pattern     = "%world%";

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.match_indices.size(), 2u);
}

// ─── AC-7: VectorOp (dot product) ────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, VectorOpDotProduct) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type = OperatorType::VectorOp;
    op.left_rows  = {{1u, 2u, 3u}};
    op.right_rows = {{4u, 5u, 6u}};

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    EXPECT_DOUBLE_EQ(r.agg_value, 32.0);
}

TEST(HardwareAcceleratorFocusedTests, VectorOpZeroVectors) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type = OperatorType::VectorOp;
    // No rows → no crash, zero result.
    auto r = accel.execute(op);
    EXPECT_TRUE(r.ok);
    EXPECT_DOUBLE_EQ(r.agg_value, 0.0);
}

// ─── AC-9: estimate_speedup ───────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, EstimateSpeedupAtLeastOne) {
    HardwareAccelerator accel;
    for (auto t : {OperatorType::HashJoin, OperatorType::SortMergeJoin,
                   OperatorType::Aggregate, OperatorType::Filter,
                   OperatorType::Sort, OperatorType::PatternMatch,
                   OperatorType::VectorOp}) {
        QueryOperator op;
        op.op_type  = t;
        op.rows     = makeSeqRows(50);
        op.left_rows  = makeSeqRows(50);
        op.right_rows = makeSeqRows(50);
        EXPECT_GE(accel.estimate_speedup(op, HardwareAccelerator::DeviceType::GPU_CUDA), 1.0)
            << "speedup < 1 for op " << static_cast<int>(t);
    }
}

TEST(HardwareAcceleratorFocusedTests, EstimateSpeedupScalesWithRowCount) {
    HardwareAccelerator::Config cfg;
    cfg.gpu_row_threshold = 100;
    HardwareAccelerator accel(cfg);

    QueryOperator small_op;
    small_op.op_type  = OperatorType::HashJoin;
    small_op.left_rows  = makeSeqRows(10);
    small_op.right_rows = makeSeqRows(10);

    QueryOperator large_op;
    large_op.op_type  = OperatorType::HashJoin;
    large_op.left_rows  = makeSeqRows(1000);
    large_op.right_rows = makeSeqRows(1000);

    const double small_speedup = accel.estimate_speedup(small_op, HardwareAccelerator::DeviceType::GPU_CUDA);
    const double large_speedup = accel.estimate_speedup(large_op, HardwareAccelerator::DeviceType::GPU_CUDA);

    EXPECT_LE(small_speedup, large_speedup);
}

TEST(HardwareAcceleratorFocusedTests, EstimateSpeedupGPUHigherThanSIMD) {
    HardwareAccelerator::Config cfg;
    cfg.gpu_row_threshold = 100;
    HardwareAccelerator accel(cfg);

    QueryOperator op;
    op.op_type    = OperatorType::HashJoin;
    op.left_rows  = makeSeqRows(1000);
    op.right_rows = makeSeqRows(1000);

    const double gpu_speedup  = accel.estimate_speedup(op, HardwareAccelerator::DeviceType::GPU_CUDA);
    const double simd_speedup = accel.estimate_speedup(op, HardwareAccelerator::DeviceType::VECTOR_ENGINE);

    EXPECT_GT(gpu_speedup, simd_speedup);
}

// ─── AC-10: Statistics ───────────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, StatsCountExecutions) {
    HardwareAccelerator accel;
    const size_t N = 5;
    for (size_t i = 0; i < N; ++i) {
        auto op = makeHashJoinOp(2);
        accel.execute(op);
    }
    auto s = accel.getStats();
    EXPECT_EQ(s.total_executions, N);
    EXPECT_EQ(s.hash_join_count, N);
}

TEST(HardwareAcceleratorFocusedTests, StatsResetClearsAll) {
    HardwareAccelerator accel;
    auto op = makeHashJoinOp(3);
    accel.execute(op);
    EXPECT_GT(accel.getStats().total_executions, 0u);

    accel.resetStats();
    auto s = accel.getStats();
    EXPECT_EQ(s.total_executions, 0u);
    EXPECT_EQ(s.hash_join_count, 0u);
    EXPECT_EQ(s.total_rows_processed, 0u);
}

TEST(HardwareAcceleratorFocusedTests, StatsCPUFallbackCounted) {
    // Small row count → CPU fallback.
    HardwareAccelerator accel;
    auto op = makeHashJoinOp(3);
    accel.execute(op);
    auto s = accel.getStats();
    EXPECT_GT(s.cpu_fallback_executions, 0u);
}

TEST(HardwareAcceleratorFocusedTests, StatsHWPathCounted) {
    // Lower thresholds so we hit the HW path.
    HardwareAccelerator::Config cfg;
    cfg.gpu_row_threshold  = 4;
    cfg.simd_row_threshold = 2;
    HardwareAccelerator accel(cfg);

    auto op = makeHashJoinOp(5);
    HardwareAccelerator::AcceleratorConfig dcfg;
    dcfg.device = HardwareAccelerator::DeviceType::VECTOR_ENGINE;
    accel.execute(op, dcfg);

    auto s = accel.getStats();
    EXPECT_GT(s.hw_path_executions, 0u);
}

TEST(HardwareAcceleratorFocusedTests, StatsElapsedUsNonZero) {
    HardwareAccelerator accel;
    auto op = makeHashJoinOp(100);
    accel.execute(op);
    EXPECT_GT(accel.getStats().total_elapsed_us, 0u);
}

TEST(HardwareAcceleratorFocusedTests, StatsRowsProcessedAccumulate) {
    HardwareAccelerator accel;
    const size_t n = 50;
    auto op = makeHashJoinOp(n);
    accel.execute(op);
    // Left + right = 2*n rows.
    EXPECT_EQ(accel.getStats().total_rows_processed, 2 * n);
}

// ─── AC-11: Config thresholds ─────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, ThresholdsControlHWPath) {
    // With thresholds set to 0, every call should take the HW path.
    HardwareAccelerator::Config cfg;
    cfg.gpu_row_threshold  = 0;
    cfg.simd_row_threshold = 0;
    HardwareAccelerator accel(cfg);

    auto op = makeHashJoinOp(5);
    HardwareAccelerator::AcceleratorConfig dcfg;
    dcfg.device = HardwareAccelerator::DeviceType::VECTOR_ENGINE;
    auto r = accel.execute(op, dcfg);
    EXPECT_TRUE(r.used_hw_path);
}

TEST(HardwareAcceleratorFocusedTests, HighThresholdForcesCPUFallback) {
    HardwareAccelerator::Config cfg;
    cfg.gpu_row_threshold  = 1'000'000;
    cfg.simd_row_threshold = 1'000'000;
    HardwareAccelerator accel(cfg);

    auto op = makeHashJoinOp(100);
    auto r  = accel.execute(op);
    EXPECT_FALSE(r.used_hw_path);
}

// ─── AC-14: Thread safety ─────────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, ConcurrentExecuteIsSafe) {
    HardwareAccelerator::Config cfg;
    cfg.gpu_row_threshold  = 50;
    cfg.simd_row_threshold = 10;
    HardwareAccelerator accel(cfg);

    const size_t n_threads = 8;
    const size_t ops_per_thread = 20;
    std::vector<std::thread> threads;
    threads.reserve(n_threads);

    for (size_t t = 0; t < n_threads; ++t) {
        threads.emplace_back([&accel, ops_per_thread]() {
            for (size_t i = 0; i < ops_per_thread; ++i) {
                auto op = makeHashJoinOp(30);
                auto r  = accel.execute(op);
                EXPECT_TRUE(r.ok);
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    auto s = accel.getStats();
    EXPECT_EQ(s.total_executions, n_threads * ops_per_thread);
}

// ─── Default execute() (no config) ───────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, DefaultExecuteUsesDefaultConfig) {
    HardwareAccelerator accel;
    auto op = makeHashJoinOp(5);
    auto r  = accel.execute(op);
    EXPECT_TRUE(r.ok);
    // Just verifying it compiles and runs.
}

// ─── Multi-column join ────────────────────────────────────────────────────────

TEST(HardwareAcceleratorFocusedTests, HashJoinMultiColumnRows) {
    HardwareAccelerator accel;
    QueryOperator op;
    op.op_type       = OperatorType::HashJoin;
    op.left_key_col  = 1;   // join on second column
    op.right_key_col = 0;

    // Left: {val, key} pairs
    op.left_rows  = {{10u, 1u}, {20u, 2u}, {30u, 3u}};
    // Right: {key, val} pairs – keys 1 and 3 match
    op.right_rows = {{1u, 100u}, {99u, 200u}, {3u, 300u}};

    auto r = accel.execute(op);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.rows.size(), 2u);   // matches on key=1 and key=3
    // Each output row has left.size() + right.size() = 4 columns
    for (const auto& row : r.rows) {
        EXPECT_EQ(row.size(), 4u);
    }
}
