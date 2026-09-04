#include <gtest/gtest.h>
#include "storage/simd_filter.h"
#include "storage/columnar_format.h"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <vector>
#include <chrono>
#include <cstdint>

using namespace themis::storage;

// ============================================================================
// Helpers
// ============================================================================

/// Build a ColumnSegment containing the given int32 values (NONE codec).
static ColumnSegment makeInt32Segment(const std::vector<int32_t>& vals) {
    auto res = ColumnSegment::create(
        ColumnType::INT32, vals.data(), vals.size(), CompressionCodec::NONE);
    EXPECT_TRUE(res.has_value()) << "Failed to create INT32 segment";
    return std::move(*res);
}

/// Build a ColumnSegment containing the given int64 values.
static ColumnSegment makeInt64Segment(const std::vector<int64_t>& vals) {
    auto res = ColumnSegment::create(
        ColumnType::INT64, vals.data(), vals.size(), CompressionCodec::NONE);
    EXPECT_TRUE(res.has_value()) << "Failed to create INT64 segment";
    return std::move(*res);
}

/// Build a ColumnSegment containing the given float32 values.
static ColumnSegment makeFloat32Segment(const std::vector<float>& vals) {
    auto res = ColumnSegment::create(
        ColumnType::FLOAT32, vals.data(), vals.size(), CompressionCodec::NONE);
    EXPECT_TRUE(res.has_value()) << "Failed to create FLOAT32 segment";
    return std::move(*res);
}

/// Build a ColumnSegment containing the given float64 values.
static ColumnSegment makeFloat64Segment(const std::vector<double>& vals) {
    auto res = ColumnSegment::create(
        ColumnType::FLOAT64, vals.data(), vals.size(), CompressionCodec::NONE);
    EXPECT_TRUE(res.has_value()) << "Failed to create FLOAT64 segment";
    return std::move(*res);
}

/// Scalar reference filter: returns sorted indices of elements satisfying op.
template<typename T>
static std::vector<uint32_t> scalarRef(const std::vector<T>& data,
                                        FilterOp op, T thr) {
    std::vector<uint32_t> out = {};

    for (uint32_t i = 0; i < static_cast<uint32_t>(data.size()); ++i) {
        bool pass = false;
        switch (op) {
            case FilterOp::EQ: pass = data[i] == thr; break;
            case FilterOp::NE: pass = data[i] != thr; break;
            case FilterOp::LT: pass = data[i] <  thr; break;
            case FilterOp::LE: pass = data[i] <= thr; break;
            case FilterOp::GT: pass = data[i] >  thr; break;
            case FilterOp::GE: pass = data[i] >= thr; break;
        }
        if (pass) {
          out.push_back(i);
        }
    }
    return out;
}

// ============================================================================
// SF-1: detectSIMDLevel
// ============================================================================

TEST(SIMDFilterFocusedTests, SF1_DetectSIMDLevelIsValid) {
    SIMDLevel level = detectSIMDLevel();
    // Must be one of the defined enum values
    EXPECT_GE(static_cast<int>(level), static_cast<int>(SIMDLevel::SCALAR));
    EXPECT_LE(static_cast<int>(level), static_cast<int>(SIMDLevel::AVX512));
}

// ============================================================================
// SF-2..7: simd_filter_int32 – all six comparison ops
// ============================================================================

class SimdFilterInt32Test : public ::testing::Test {
protected:
    // 20 values: 0,1,2,...,19
    std::vector<int32_t> data;
    void SetUp() override {
        data.resize(20);
        std::iota(data.begin(), data.end(), 0);
    }
};

TEST_F(SimdFilterInt32Test, SF2_EQ) {
    std::vector<uint32_t> out = {};

    simd_filter_int32(data.data(), data.size(), FilterOp::EQ, 10, out);
    ASSERT_EQ(1u, out.size());
    EXPECT_EQ(10u, out[0]);
}

TEST_F(SimdFilterInt32Test, SF3_NE) {
    std::vector<uint32_t> out = {};

    simd_filter_int32(data.data(), data.size(), FilterOp::NE, 10, out);
    EXPECT_EQ(19u, out.size());
}

TEST_F(SimdFilterInt32Test, SF4_LT) {
    std::vector<uint32_t> out = {};

    simd_filter_int32(data.data(), data.size(), FilterOp::LT, 5, out);
    // values 0..4
    ASSERT_EQ(5u, out.size());
    for (uint32_t i = 0; i < 5; ++i) {
      EXPECT_EQ(i, out[i]);
    }
}

TEST_F(SimdFilterInt32Test, SF5_LE) {
    std::vector<uint32_t> out = {};

    simd_filter_int32(data.data(), data.size(), FilterOp::LE, 5, out);
    // values 0..5
    ASSERT_EQ(6u, out.size());
    for (uint32_t i = 0; i <= 5; ++i) {
      EXPECT_EQ(i, out[i]);
    }
}

TEST_F(SimdFilterInt32Test, SF6_GT) {
    std::vector<uint32_t> out = {};

    simd_filter_int32(data.data(), data.size(), FilterOp::GT, 15, out);
    // values 16..19 → indices 16..19
    ASSERT_EQ(4u, out.size());
    for (uint32_t i = 0; i < 4; ++i) {
      EXPECT_EQ(16u + i, out[i]);
    }
}

TEST_F(SimdFilterInt32Test, SF7_GE) {
    std::vector<uint32_t> out = {};

    simd_filter_int32(data.data(), data.size(), FilterOp::GE, 15, out);
    // values 15..19 → 5 rows
    ASSERT_EQ(5u, out.size());
}

// ============================================================================
// SF-8..10: edge cases
// ============================================================================

TEST(SIMDFilterFocusedTests, SF8_EmptyInput) {
    std::vector<uint32_t> out;
    size_t n = simd_filter_int32(nullptr, 0, FilterOp::EQ, 0, out);
    EXPECT_EQ(0u, n);
    EXPECT_TRUE(out.empty());
}

TEST(SIMDFilterFocusedTests, SF9_MatchesNone) {
    std::vector<int32_t> data = {1, 2, 3};
    std::vector<uint32_t> out = {};

    simd_filter_int32(data.data(), data.size(), FilterOp::EQ, 99, out);
    EXPECT_TRUE(out.empty());
}

TEST(SIMDFilterFocusedTests, SF10_MatchesAll) {
    std::vector<int32_t> data = {5, 5, 5, 5, 5};
    std::vector<uint32_t> out = {};

    simd_filter_int32(data.data(), data.size(), FilterOp::EQ, 5, out);
    ASSERT_EQ(5u, out.size());
    for (uint32_t i = 0; i < 5; ++i) {
      EXPECT_EQ(i, out[i]);
    }
}

// ============================================================================
// SF-11..12: simd_filter_int64
// ============================================================================

TEST(SIMDFilterFocusedTests, SF11_Int64EQ_MultiBatch) {
    // 32 elements (> 4 lanes × 8 = fills multiple AVX2 iterations)
    std::vector<int64_t> data(32);
    std::iota(data.begin(), data.end(), int64_t(100));
    // Expect exactly one match: value 115 → index 15
    std::vector<uint32_t> out = {};

    simd_filter_int64(data.data(), data.size(), FilterOp::EQ, int64_t(115), out);
    ASSERT_EQ(1u, out.size());
    EXPECT_EQ(15u, out[0]);
}

TEST(SIMDFilterFocusedTests, SF12_Int64GT) {
    std::vector<int64_t> data = {10, 20, 30, 40, 50};
    std::vector<uint32_t> out = {};

    simd_filter_int64(data.data(), data.size(), FilterOp::GT, int64_t(25), out);
    // values 30,40,50 → indices 2,3,4
    ASSERT_EQ(3u, out.size());
    EXPECT_EQ(2u, out[0]);
    EXPECT_EQ(3u, out[1]);
    EXPECT_EQ(4u, out[2]);
}

// ============================================================================
// SF-13: simd_filter_float
// ============================================================================

TEST(SIMDFilterFocusedTests, SF13_FloatLT) {
    std::vector<float> data = {1.0f, 2.5f, 3.0f, 0.5f, 4.0f};
    std::vector<uint32_t> out = {};

    simd_filter_float(data.data(), data.size(), FilterOp::LT, 2.5f, out);
    // 1.0f (idx 0) and 0.5f (idx 3)
    ASSERT_EQ(2u, out.size());
    EXPECT_EQ(0u, out[0]);
    EXPECT_EQ(3u, out[1]);
}

// ============================================================================
// SF-14: simd_filter_double
// ============================================================================

TEST(SIMDFilterFocusedTests, SF14_DoubleGE) {
    std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<uint32_t> out = {};

    simd_filter_double(data.data(), data.size(), FilterOp::GE, 3.0, out);
    // indices 2,3,4
    ASSERT_EQ(3u, out.size());
    EXPECT_EQ(2u, out[0]);
    EXPECT_EQ(3u, out[1]);
    EXPECT_EQ(4u, out[2]);
}

// ============================================================================
// SF-15..18: SIMDColumnFilter::scan
// ============================================================================

TEST(SIMDFilterFocusedTests, SF15_ScanInt32EQ) {
    std::vector<int32_t> vals = {3, 7, 3, 9, 3, 1};
    auto seg = makeInt32Segment(vals);

    SIMDColumnFilter filter;
    ColumnPredicate pred{FilterOp::EQ, ColumnType::INT32, {}};
    pred.threshold.i32 = 3;

    auto result = filter.scan(seg, pred);
    // indices 0, 2, 4
    ASSERT_EQ(3u, result.size());
    EXPECT_EQ(0u, result[0]);
    EXPECT_EQ(2u, result[1]);
    EXPECT_EQ(4u, result[2]);
}

TEST(SIMDFilterFocusedTests, SF16_ScanInt64GT_ZoneMapEarlyOut) {
    // All values are <= 100 → GT 200 should be skipped by zone map
    std::vector<int64_t> vals = {10, 20, 30, 40, 50};
    auto seg = makeInt64Segment(vals);

    SIMDColumnFilter filter;
    ColumnPredicate pred{FilterOp::GT, ColumnType::INT64, {}};
    pred.threshold.i64 = 200;  // > max(50) → zone-map skip

    auto result = filter.scan(seg, pred);
    EXPECT_TRUE(result.empty());

    // rows_matched should be 0, rows_processed should be 5 (zone-map counted)
    EXPECT_EQ(5u, filter.lastStats().rows_processed);
    EXPECT_EQ(0u, filter.lastStats().rows_matched);
}

TEST(SIMDFilterFocusedTests, SF17_ScanFloat32LT) {
    std::vector<float> vals = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    auto seg = makeFloat32Segment(vals);

    SIMDColumnFilter filter;
    ColumnPredicate pred{FilterOp::LT, ColumnType::FLOAT32, {}};
    pred.threshold.f32 = 3.5f;

    auto result = filter.scan(seg, pred);
    // values < 3.5f: 1.0f, 2.0f, 3.0f → indices 0,1,2
    ASSERT_EQ(3u, result.size());
    EXPECT_EQ(0u, result[0]);
    EXPECT_EQ(1u, result[1]);
    EXPECT_EQ(2u, result[2]);
}

TEST(SIMDFilterFocusedTests, SF18_ScanFloat64GE) {
    std::vector<double> vals = {1.0, 2.0, 3.0, 4.0, 5.0};
    auto seg = makeFloat64Segment(vals);

    SIMDColumnFilter filter;
    ColumnPredicate pred{FilterOp::GE, ColumnType::FLOAT64, {}};
    pred.threshold.f64 = 3.0;

    auto result = filter.scan(seg, pred);
    ASSERT_EQ(3u, result.size());
    EXPECT_EQ(2u, result[0]);
    EXPECT_EQ(3u, result[1]);
    EXPECT_EQ(4u, result[2]);
}

// ============================================================================
// SF-19: SIMDColumnFilter::scanBatch
// ============================================================================

TEST(SIMDFilterFocusedTests, SF19_ScanBatch_MultipleSegments) {
    // Two segments, 5 rows each
    std::vector<int32_t> a = {1, 2, 3, 4, 5};
    std::vector<int32_t> b = {6, 7, 8, 9, 10};

    auto segA = makeInt32Segment(a);
    auto segB = makeInt32Segment(b);

    SIMDColumnFilter filter;
    ColumnPredicate pred{FilterOp::GT, ColumnType::INT32, {}};
    pred.threshold.i32 = 5;  // rows with value > 5: b[0..4] → global indices 5..9

    auto result = filter.scanBatch({segA, segB}, pred);
    ASSERT_EQ(5u, result.size());
    for (uint32_t i = 0; i < 5; ++i) {
        EXPECT_EQ(5u + i, result[i]);
    }
}

// ============================================================================
// SF-20..21: Stats
// ============================================================================

TEST(SIMDFilterFocusedTests, SF20_ResetStats) {
    std::vector<int32_t> vals = {1, 2, 3};
    auto seg = makeInt32Segment(vals);

    SIMDColumnFilter filter;
    ColumnPredicate pred{FilterOp::EQ, ColumnType::INT32, {}};
    pred.threshold.i32 = 2;
    filter.scan(seg, pred);

    filter.resetStats();
    const auto& s = filter.lastStats();
    EXPECT_EQ(0u, s.rows_processed);
    EXPECT_EQ(0u, s.rows_matched);
    EXPECT_NEAR(0.0, s.elapsed_us, 1e-9);
}

TEST(SIMDFilterFocusedTests, SF21_StatsUpdatedAfterScan) {
    std::vector<int32_t> vals = {10, 20, 30, 40, 50};
    auto seg = makeInt32Segment(vals);

    SIMDColumnFilter filter;
    ColumnPredicate pred{FilterOp::GT, ColumnType::INT32, {}};
    pred.threshold.i32 = 25;

    filter.scan(seg, pred);
    const auto& s = filter.lastStats();
    EXPECT_EQ(5u, s.rows_processed);
    EXPECT_EQ(3u, s.rows_matched);   // 30, 40, 50
    EXPECT_GT(s.elapsed_us, 0.0);
}

// ============================================================================
// SF-22: large batch correctness
// ============================================================================

TEST(SIMDFilterFocusedTests, SF22_LargeBatch_8192_Rows_Correctness) {
    constexpr size_t N = 8192;
    std::vector<int32_t> data(N);
    for (size_t i = 0; i < N; ++i) {
        data[i] = static_cast<int32_t>(i % 1000);  // values 0..999 repeating
    }
    const int32_t thr = 500;

    std::vector<uint32_t> simd_out, scalar_out;
    simd_filter_int32(data.data(), N, FilterOp::LT, thr, simd_out);
    scalar_out = scalarRef(data, FilterOp::LT, thr);

    ASSERT_EQ(scalar_out.size(), simd_out.size());
    for (size_t i = 0; i < scalar_out.size(); ++i) {
        EXPECT_EQ(scalar_out[i], simd_out[i]) << " at index " << i;
    }
}

// ============================================================================
// SF-23: scalar parity for all ops on int32
// ============================================================================

TEST(SIMDFilterFocusedTests, SF23_ScalarParity_AllOps_Int32) {
    std::vector<int32_t> data(100);
    std::iota(data.begin(), data.end(), -50);  // -50 .. 49
    const int32_t thr = 10;

    for (auto op : {FilterOp::EQ, FilterOp::NE, FilterOp::LT,
                    FilterOp::LE, FilterOp::GT, FilterOp::GE}) {
        std::vector<uint32_t> simd_out, ref_out;
        simd_filter_int32(data.data(), data.size(), op, thr, simd_out);
        ref_out = scalarRef(data, op, thr);
        ASSERT_EQ(ref_out.size(), simd_out.size())
            << "Size mismatch for op=" << static_cast<int>(op);
        for (size_t i = 0; i < ref_out.size(); ++i) {
            EXPECT_EQ(ref_out[i], simd_out[i])
                << "Index mismatch at pos=" << i << " op=" << static_cast<int>(op);
        }
    }
}

// ============================================================================
// SF-24: non-aligned tail for int64
// ============================================================================

TEST(SIMDFilterFocusedTests, SF24_Int64_NonAlignedTail) {
    // 7 elements — AVX2 processes 4 at a time, leaving 3 as tail
    std::vector<int64_t> data = {1, 2, 3, 4, 5, 6, 7};
    std::vector<uint32_t> out = {};

    simd_filter_int64(data.data(), data.size(), FilterOp::GE, int64_t(5), out);
    // Values 5,6,7 → indices 4,5,6
    auto ref = scalarRef(data, FilterOp::GE, int64_t(5));
    ASSERT_EQ(ref.size(), out.size());
    for (size_t i = 0; i < ref.size(); ++i) {
      EXPECT_EQ(ref[i], out[i]);
    }
}

// ============================================================================
// SF-25: throughput SLO (perf-gated)
// ============================================================================

#ifdef THEMIS_RUN_PERF_TESTS
TEST(SIMDFilterFocusedTests, SF25_Throughput_SLO_8192_Int32) {
    // Perf target: ≥4× scalar baseline throughput with SIMD on AVX2+ host.
    // We measure wall time for SIMD vs scalar on 1M elements.
    constexpr size_t N = 1'000'000;
    std::vector<int32_t> data(N);
    std::iota(data.begin(), data.end(), 0);
    const int32_t thr = 500'000;

    // Warmup
    {
        std::vector<uint32_t> dummy;
        simd_filter_int32(data.data(), N, FilterOp::LT, thr, dummy);
        dummy.clear();
        auto* p = data.data();
        for (size_t i = 0; i < N; ++i) {
            if (p[i] < thr) {
              dummy.push_back(static_cast<uint32_t>(i));
            }
        }
    }

    // Scalar baseline
    auto t0 = std::chrono::steady_clock::now();
    std::vector<uint32_t> scalar_out;
    auto* p = data.data();
    for (size_t i = 0; i < N; ++i) {
        if (p[i] < thr) {
          scalar_out.push_back(static_cast<uint32_t>(i));
        }
    }
    auto t1 = std::chrono::steady_clock::now();
    double scalar_us =
        std::chrono::duration<double, std::micro>(t1 - t0).count();

    // SIMD path
    auto t2 = std::chrono::steady_clock::now();
    std::vector<uint32_t> simd_out;
    simd_filter_int32(data.data(), N, FilterOp::LT, thr, simd_out);
    auto t3 = std::chrono::steady_clock::now();
    double simd_us =
        std::chrono::duration<double, std::micro>(t3 - t2).count();

    EXPECT_EQ(scalar_out.size(), simd_out.size());

    if (detectSIMDLevel() >= SIMDLevel::AVX2) {
        // Require ≥2× speedup over scalar (conservative for CI variability)
        double speedup = scalar_us / std::max(simd_us, 1.0);
        EXPECT_GT(speedup, 2.0)
            << "SIMD speedup " << speedup << "× below 2× target "
            << "(scalar=" << scalar_us << "us, simd=" << simd_us << "us)";
    }
}
#endif // THEMIS_RUN_PERF_TESTS
