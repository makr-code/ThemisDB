/*
 * test_gpu_query_accelerator_parity.cpp
 *
 * CUDA/CPU parity tests for all 5 GPU-accelerated operations implemented in
 * query_accelerator.cpp.  Each test runs the operation in both forced-CPU
 * mode and the default (GPU-enabled) mode, then asserts that the results are
 * identical (or numerically equivalent within tolerance for floating-point
 * operations).
 *
 * When no CUDA/HIP device is present (the typical CI environment) the GPU
 * path transparently falls back to the CPU implementation, so both modes
 * produce the same results — verifying the fallback is correct.  On a real
 * GPU the tests additionally exercise the device code paths.
 *
 * Operations covered:
 *   1. scan       — parallel row scan with predicate filter
 *   2. sort       — stable sort by numeric key (ASC / DESC)
 *   3. aggregate  — SUM, MIN, MAX, AVG, COUNT
 *   4. hashJoin   — hash / sort-merge join on uint64_t keys
 *   5. dotProduct — FP32, FP16, BF16 precision modes
 *
 * Input sizes:   1 K, 100 K, 10 M rows  (10 M enabled only when
 *                THEMIS_PARITY_LARGE_TESTS is defined to keep CI fast).
 */

#include "themis/gpu/query_accelerator.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>
#include <vector>

using namespace themis::gpu;
using Row     = GPUQueryAccelerator::Row;
using AggFunc = GPUQueryAccelerator::AggFunc;
using SortOrder = GPUQueryAccelerator::SortOrder;
using PrecisionMode = GPUQueryAccelerator::PrecisionMode;

// ============================================================================
// Test-data helpers
// ============================================================================

namespace {

/// Build rows with sequential IDs and a float payload equal to base+i.
static std::vector<Row> makeRows(size_t n, double base = 1.0) {
    std::vector<Row> rows(n);
    for (size_t i = 0; i < n; ++i) {
        rows[i].id = static_cast<uint64_t>(i);
        float v = static_cast<float>(base + static_cast<double>(i));
        rows[i].data.resize(sizeof(float));
        std::memcpy(rows[i].data.data(), &v, sizeof(float));
    }
    return rows;
}

/// Build rows whose ID is drawn from the provided list (for join tests).
static std::vector<Row> makeKeyRows(const std::vector<uint64_t>& ids) {
    std::vector<Row> rows = {};

    rows.reserve(ids.size());
    for (uint64_t id : ids) {
        Row r;
        r.id = id;
        r.data.resize(sizeof(uint64_t));
        std::memcpy(r.data.data(), &id, sizeof(uint64_t));
        rows.push_back(r);
    }
    return rows;
}

/// Extract the float payload of a Row as double.
static double payloadVal(const Row& r) {
    if (r.data.size() < sizeof(float)) {
      return 0.0;
    }
    float v = 0;
    std::memcpy(&v, r.data.data(), sizeof(float));
    return static_cast<double>(v);
}

/// Build a flat float vector with values rng-drawn in [lo, hi].
static std::vector<float> makeFloats(size_t n, float lo = -1.0f, float hi = 1.0f,
                                     uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(lo, hi);
    std::vector<float> v(n);
    for (auto& x : v) {
      x = dist(rng);
    }
    return v;
}

/// Returns a Config that forces the CPU path unconditionally.
static GPUQueryAccelerator::Config cpuConfig() {
    GPUQueryAccelerator::Config c;
    c.force_cpu = true;
    return c;
}

/// Returns a Config that enables the GPU path (falls back to CPU when no
/// device is present, which is the common case in CI).
static GPUQueryAccelerator::Config gpuConfig(size_t threshold = 0) {
    GPUQueryAccelerator::Config c;
    c.force_cpu          = false;
    c.gpu_threshold_rows = threshold;  // 0 → GPU path for any row count
    return c;
}

} // anonymous namespace

// ============================================================================
// Fixture — parameterised over input size
// ============================================================================

class QueryAcceleratorParityTest : public ::testing::TestWithParam<size_t> {};

INSTANTIATE_TEST_SUITE_P(
    Sizes,
    QueryAcceleratorParityTest,
    ::testing::Values(
        1'000UL,
        100'000UL
#ifdef THEMIS_PARITY_LARGE_TESTS
        , 10'000'000UL
#endif
    ),
    [](const ::testing::TestParamInfo<size_t>& info) {
        if (info.param >= 1'000'000) {
          return std::string("10M");
        }
        if (info.param >= 100'000) {
          return std::string("100K");
        }
        return std::string("1K");
    });

// ============================================================================
// 1. Scan — parity
// ============================================================================

TEST_P(QueryAcceleratorParityTest, ScanPassAll_Parity) {
    const size_t n = GetParam();
    auto rows = makeRows(n);

    GPUQueryAccelerator cpu_acc(cpuConfig());
    GPUQueryAccelerator gpu_acc(gpuConfig());

    auto cpu_res = cpu_acc.scan(rows);
    auto gpu_res = gpu_acc.scan(rows);

    ASSERT_EQ(cpu_res.rows.size(), gpu_res.rows.size());
    EXPECT_EQ(cpu_res.rows_scanned, gpu_res.rows_scanned);
    EXPECT_EQ(cpu_res.rows_passed,  gpu_res.rows_passed);

    for (size_t i = 0; i < cpu_res.rows.size(); ++i) {
        EXPECT_EQ(cpu_res.rows[i].id, gpu_res.rows[i].id);
    }
}

TEST_P(QueryAcceleratorParityTest, ScanWithFilter_Parity) {
    const size_t n = GetParam();
    auto rows = makeRows(n);

    GPUQueryAccelerator cpu_acc(cpuConfig());
    GPUQueryAccelerator gpu_acc(gpuConfig());

    // Filter: keep only even IDs (host predicate — GPU path falls through to CPU)
    auto filter = [](const Row& r) { return r.id % 2 == 0; };

    auto cpu_res = cpu_acc.scan(rows, filter);
    auto gpu_res = gpu_acc.scan(rows, filter);

    ASSERT_EQ(cpu_res.rows.size(), gpu_res.rows.size());
    for (size_t i = 0; i < cpu_res.rows.size(); ++i) {
        EXPECT_EQ(cpu_res.rows[i].id, gpu_res.rows[i].id);
    }
}

// ============================================================================
// 2. Sort — parity
// ============================================================================

TEST_P(QueryAcceleratorParityTest, SortAscending_Parity) {
    const size_t n = GetParam();

    // Shuffle deterministically so the sort actually does work.
    auto rows = makeRows(n);
    {
        std::mt19937 rng(1234);
        std::shuffle(rows.begin(), rows.end(), rng);
    }

    // Make separate copies for each accelerator (sort takes by-value).
    auto rows_cpu = rows;
    auto rows_gpu = rows;

    GPUQueryAccelerator cpu_acc(cpuConfig());
    GPUQueryAccelerator gpu_acc(gpuConfig());

    auto cpu_res = cpu_acc.sort(std::move(rows_cpu), payloadVal, SortOrder::ASC);
    auto gpu_res = gpu_acc.sort(std::move(rows_gpu), payloadVal, SortOrder::ASC);

    ASSERT_EQ(cpu_res.rows.size(), gpu_res.rows.size());

    // Both results must be sorted ascending.
    for (size_t i = 1; i < cpu_res.rows.size(); ++i) {
        EXPECT_LE(payloadVal(cpu_res.rows[i - 1]), payloadVal(cpu_res.rows[i]));
        EXPECT_LE(payloadVal(gpu_res.rows[i - 1]), payloadVal(gpu_res.rows[i]));
    }

    // Parity: same keys in same order.
    for (size_t i = 0; i < cpu_res.rows.size(); ++i) {
        EXPECT_DOUBLE_EQ(payloadVal(cpu_res.rows[i]),
                         payloadVal(gpu_res.rows[i]));
    }
}

TEST_P(QueryAcceleratorParityTest, SortDescending_Parity) {
    const size_t n = GetParam();
    auto rows = makeRows(n);
    {
        std::mt19937 rng(5678);
        std::shuffle(rows.begin(), rows.end(), rng);
    }
    auto rows_cpu = rows;
    auto rows_gpu = rows;

    GPUQueryAccelerator cpu_acc(cpuConfig());
    GPUQueryAccelerator gpu_acc(gpuConfig());

    auto cpu_res = cpu_acc.sort(std::move(rows_cpu), payloadVal, SortOrder::DESC);
    auto gpu_res = gpu_acc.sort(std::move(rows_gpu), payloadVal, SortOrder::DESC);

    ASSERT_EQ(cpu_res.rows.size(), gpu_res.rows.size());

    for (size_t i = 1; i < cpu_res.rows.size(); ++i) {
        EXPECT_GE(payloadVal(cpu_res.rows[i - 1]), payloadVal(cpu_res.rows[i]));
        EXPECT_GE(payloadVal(gpu_res.rows[i - 1]), payloadVal(gpu_res.rows[i]));
    }
    for (size_t i = 0; i < cpu_res.rows.size(); ++i) {
        EXPECT_DOUBLE_EQ(payloadVal(cpu_res.rows[i]),
                         payloadVal(gpu_res.rows[i]));
    }
}

// ============================================================================
// 3. Aggregate — parity
// ============================================================================

struct AggParityParam {
    size_t   n = 0;
    AggFunc  func;
};

class AggregateParityTest
    : public ::testing::TestWithParam<AggParityParam> {};

INSTANTIATE_TEST_SUITE_P(
    SizesAndFuncs,
    AggregateParityTest,
    ::testing::Values(
        AggParityParam{1'000,   AggFunc::SUM},
        AggParityParam{1'000,   AggFunc::MIN},
        AggParityParam{1'000,   AggFunc::MAX},
        AggParityParam{1'000,   AggFunc::AVG},
        AggParityParam{1'000,   AggFunc::COUNT},
        AggParityParam{100'000, AggFunc::SUM},
        AggParityParam{100'000, AggFunc::MIN},
        AggParityParam{100'000, AggFunc::MAX},
        AggParityParam{100'000, AggFunc::AVG},
        AggParityParam{100'000, AggFunc::COUNT}
    ));

TEST_P(AggregateParityTest, Parity) {
    const auto [n, func] = GetParam();
    auto rows = makeRows(n, 1.0);

    GPUQueryAccelerator cpu_acc(cpuConfig());
    GPUQueryAccelerator gpu_acc(gpuConfig());

    auto cpu_res = cpu_acc.aggregate(rows, func, payloadVal);
    auto gpu_res = gpu_acc.aggregate(rows, func, payloadVal);

    EXPECT_EQ(cpu_res.count, gpu_res.count);
    // For floating-point results allow a small relative tolerance to account
    // for different summation order on GPU vs CPU.
    const double tol = std::max(1e-4, std::abs(cpu_res.value) * 1e-5);
    EXPECT_NEAR(cpu_res.value, gpu_res.value, tol);
}

// ============================================================================
// 4. Hash join — parity
// ============================================================================

TEST_P(QueryAcceleratorParityTest, HashJoin_Parity) {
    const size_t n = GetParam();

    // Build two key sets with ~50 % overlap.
    std::vector<uint64_t> left_ids(n), right_ids(n);
    for (size_t i = 0; i < n; ++i) {
      left_ids[i]  = static_cast<uint64_t>(i);
    }
    for (size_t i = 0; i < n; ++i) {
      right_ids[i] = static_cast<uint64_t>(i / 2);
    }

    auto left_rows  = makeKeyRows(left_ids);
    auto right_rows = makeKeyRows(right_ids);

    GPUQueryAccelerator cpu_acc(cpuConfig());
    GPUQueryAccelerator gpu_acc(gpuConfig());

    auto key_fn = [](const Row& r) -> uint64_t { return r.id; };

    auto cpu_res = cpu_acc.hashJoin(left_rows, right_rows, key_fn, key_fn);
    auto gpu_res = gpu_acc.hashJoin(left_rows, right_rows, key_fn, key_fn);

    ASSERT_EQ(cpu_res.pairs.size(), gpu_res.pairs.size());

    // Compare actual pair contents: build (left.id, right.id) multisets from
    // both results and assert they are identical, regardless of emit order.
    using IdPair = std::pair<uint64_t, uint64_t>;
    auto toIdPairs = [](const std::vector<std::pair<Row, Row>>& pairs) {
        std::vector<IdPair> v = {};

        v.reserve(pairs.size());
        for (const auto& p : pairs) {
          v.emplace_back(p.first.id, p.second.id);
        }
        std::sort(v.begin(), v.end());
        return v;
    };

    EXPECT_EQ(toIdPairs(cpu_res.pairs), toIdPairs(gpu_res.pairs));
}

TEST_P(QueryAcceleratorParityTest, HashJoin_NoMatch_Parity) {
    const size_t n = GetParam();

    std::vector<uint64_t> left_ids(n), right_ids(n);
    for (size_t i = 0; i < n; ++i) {
      left_ids[i]  = static_cast<uint64_t>(i);
    }
    for (size_t i = 0; i < n; ++i) {
      right_ids[i] = static_cast<uint64_t>(n + i);
    }

    auto left_rows  = makeKeyRows(left_ids);
    auto right_rows = makeKeyRows(right_ids);

    GPUQueryAccelerator cpu_acc(cpuConfig());
    GPUQueryAccelerator gpu_acc(gpuConfig());
    auto key_fn = [](const Row& r) -> uint64_t { return r.id; };

    auto cpu_res = cpu_acc.hashJoin(left_rows, right_rows, key_fn, key_fn);
    auto gpu_res = gpu_acc.hashJoin(left_rows, right_rows, key_fn, key_fn);

    EXPECT_TRUE(cpu_res.pairs.empty());
    EXPECT_EQ(cpu_res.pairs.size(), gpu_res.pairs.size());
}

// ============================================================================
// 5. dotProduct — parity (FP32, FP16, BF16)
// ============================================================================

struct DotParityParam {
    size_t        n = 0;
    PrecisionMode prec;
};

class DotProductParityTest
    : public ::testing::TestWithParam<DotParityParam> {};

INSTANTIATE_TEST_SUITE_P(
    SizesAndPrecisions,
    DotProductParityTest,
    ::testing::Values(
        DotParityParam{1'000,   PrecisionMode::FP32},
        DotParityParam{1'000,   PrecisionMode::FP16},
        DotParityParam{1'000,   PrecisionMode::BF16},
        DotParityParam{100'000, PrecisionMode::FP32},
        DotParityParam{100'000, PrecisionMode::FP16},
        DotParityParam{100'000, PrecisionMode::BF16}
    ));

TEST_P(DotProductParityTest, Parity) {
    const auto [n, prec] = GetParam();

    auto a = makeFloats(n, -1.0f, 1.0f, 42);
    auto b = makeFloats(n, -1.0f, 1.0f, 99);

    GPUQueryAccelerator::Config cpu_cfg = cpuConfig();
    cpu_cfg.precision_mode = prec;

    GPUQueryAccelerator::Config gpu_cfg = gpuConfig();
    gpu_cfg.precision_mode = prec;

    GPUQueryAccelerator cpu_acc(cpu_cfg);
    GPUQueryAccelerator gpu_acc(gpu_cfg);

    auto cpu_res = cpu_acc.dotProduct(a, b);
    auto gpu_res = gpu_acc.dotProduct(a, b);

    EXPECT_EQ(cpu_res.precision_used, gpu_res.precision_used);

    // FP32 should match closely; FP16/BF16 allow larger tolerance due to
    // potential reordering of operations on device.
    double tol = (prec == PrecisionMode::FP32)
                 ? std::max(1e-3, std::abs(cpu_res.value) * 1e-4)
                 : std::max(1.0,  std::abs(cpu_res.value) * 0.02);

    EXPECT_NEAR(cpu_res.value, gpu_res.value, tol);
}

// ============================================================================
// Stats consistency: GPU-path accelerator updates stats even when falling
// back to CPU due to absent hardware.
// ============================================================================

TEST(QueryAcceleratorStats, StatsUpdatedOnAllPaths) {
    GPUQueryAccelerator acc(gpuConfig(0));

    acc.scan(makeRows(100));
    EXPECT_EQ(acc.getStats().total_scans, 1u);

    auto rows = makeRows(100);
    acc.sort(std::move(rows), payloadVal);
    EXPECT_EQ(acc.getStats().total_sorts, 1u);

    acc.aggregate(makeRows(100), AggFunc::SUM, payloadVal);
    EXPECT_EQ(acc.getStats().total_aggregates, 1u);

    auto lr = makeKeyRows({1, 2, 3});
    auto rr = makeKeyRows({2, 3, 4});
    auto kf = [](const Row& r) -> uint64_t { return r.id; };
    acc.hashJoin(lr, rr, kf, kf);
    EXPECT_EQ(acc.getStats().total_joins, 1u);

    acc.dotProduct({1.0f, 2.0f}, {3.0f, 4.0f});
    EXPECT_EQ(acc.getStats().total_dot_products, 1u);
}
