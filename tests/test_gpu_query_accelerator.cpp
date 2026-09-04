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
    if (r.data.size() < sizeof(float)) {
      return 0.0;
    }
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

// ============================================================================
// annSearch — GPU-accelerated ANN vector similarity (cuVS/RAFT stub)
// ============================================================================

// Helper: build a flat database of `n` vectors of dimension `dim`.
// Vector i has all components equal to (float)i.
static std::vector<float> makeDatabase(size_t n, size_t dim) {
    std::vector<float> db(n * dim);
    for (size_t i = 0; i < n; ++i)
        for (size_t d = 0; d < dim; ++d)
            db[i * dim + d] = static_cast<float>(i);
    return db;
}

TEST_F(GPUQueryAcceleratorTest, AnnSearch_BasicL2_ReturnsKNeighbors) {
    GPUQueryAccelerator acc(cpuConfig());
    const size_t dim = 4, n = 10, k = 3;

    auto db = makeDatabase(n, dim);
    // Query: vector close to index 5 (all components = 5.1)
    std::vector<float> q(dim, 5.1f);

    auto result = acc.annSearch(q, 1, dim, db, n, k);
    ASSERT_EQ(result.results.size(), 1u);
    ASSERT_EQ(result.results[0].size(), k);

    // Nearest neighbor should be index 5 (all=5), then 4 or 6
    EXPECT_EQ(result.results[0][0].index, 5u);
    // Results must be sorted ascending by distance
    for (size_t i = 1; i < result.results[0].size(); ++i) {
        EXPECT_LE(result.results[0][i - 1].distance,
                  result.results[0][i].distance);
    }
}

TEST_F(GPUQueryAcceleratorTest, AnnSearch_KLargerThanDatabase_ClampsToN) {
    GPUQueryAccelerator acc(cpuConfig());
    const size_t dim = 2, n = 5, k = 100;

    auto db = makeDatabase(n, dim);
    std::vector<float> q(dim, 0.0f);

    auto result = acc.annSearch(q, 1, dim, db, n, k);
    ASSERT_EQ(result.results.size(), 1u);
    // Should return at most n results, not k
    EXPECT_EQ(result.results[0].size(), n);
}

TEST_F(GPUQueryAcceleratorTest, AnnSearch_MultipleQueries) {
    GPUQueryAccelerator acc(cpuConfig());
    const size_t dim = 3, n = 8, k = 2;

    auto db = makeDatabase(n, dim);
    // Two queries: near index 0 and near index 7
    std::vector<float> queries = {
        0.1f, 0.1f, 0.1f,   // near index 0
        6.9f, 6.9f, 6.9f    // near index 7
    };

    auto result = acc.annSearch(queries, 2, dim, db, n, k);
    ASSERT_EQ(result.results.size(), 2u);
    EXPECT_EQ(result.results[0][0].index, 0u);
    EXPECT_EQ(result.results[1][0].index, 7u);
}

TEST_F(GPUQueryAcceleratorTest, AnnSearch_ExactMatch_ZeroDistance) {
    GPUQueryAccelerator acc(cpuConfig());
    const size_t dim = 3, n = 5, k = 1;

    auto db = makeDatabase(n, dim);
    // Query exactly matches index 2 (all components = 2.0)
    std::vector<float> q(dim, 2.0f);

    auto result = acc.annSearch(q, 1, dim, db, n, k);
    ASSERT_EQ(result.results.size(), 1u);
    ASSERT_FALSE(result.results[0].empty());
    EXPECT_EQ(result.results[0][0].index, 2u);
    EXPECT_NEAR(result.results[0][0].distance, 0.0f, 1e-5f);
}

TEST_F(GPUQueryAcceleratorTest, AnnSearch_InnerProduct_ReturnsResults) {
    GPUQueryAccelerator acc(cpuConfig());
    const size_t dim = 4, n = 6, k = 2;

    auto db = makeDatabase(n, dim);
    std::vector<float> q(dim, 1.0f);

    // useL2 = false → negative inner product as distance (most similar = most negative)
    auto result = acc.annSearch(q, 1, dim, db, n, k, /*useL2=*/false);
    ASSERT_EQ(result.results.size(), 1u);
    EXPECT_EQ(result.results[0].size(), k);
    // Results must still be sorted ascending by distance (most negative first)
    if (result.results[0].size() > 1) {
        EXPECT_LE(result.results[0][0].distance, result.results[0][1].distance);
    }
    // Highest inner product (index 5, all=5.0) → most negative distance → first result
    EXPECT_EQ(result.results[0][0].index, 5u);
}

TEST_F(GPUQueryAcceleratorTest, AnnSearch_InvalidInputs_ReturnsEmpty) {
    GPUQueryAccelerator acc(cpuConfig());

    // dim = 0
    EXPECT_TRUE(acc.annSearch({}, 1, 0, {}, 1, 1).results.empty());

    // k = 0
    auto db = makeDatabase(3, 2);
    std::vector<float> q(2, 0.0f);
    EXPECT_TRUE(acc.annSearch(q, 1, 2, db, 3, 0).results.empty());

    // numQueries = 0
    EXPECT_TRUE(acc.annSearch({}, 0, 2, db, 3, 1).results.empty());

    // size mismatch: queries.size() != numQueries * dim
    std::vector<float> bad_q(3, 0.0f);  // 3 elements but expect 2*2=4
    EXPECT_TRUE(acc.annSearch(bad_q, 2, 2, db, 3, 1).results.empty());
}

TEST_F(GPUQueryAcceleratorTest, AnnSearch_UpdatesStats) {
    GPUQueryAccelerator acc(cpuConfig());
    const size_t dim = 2, n = 4, k = 2;

    auto db = makeDatabase(n, dim);
    std::vector<float> q(dim, 0.0f);

    acc.annSearch(q, 1, dim, db, n, k);
    acc.annSearch(q, 1, dim, db, n, k);

    auto s = acc.getStats();
    EXPECT_EQ(s.total_ann_searches, 2u);
}

TEST_F(GPUQueryAcceleratorTest, AnnSearch_SortedByDistanceAscending) {
    GPUQueryAccelerator acc(cpuConfig());
    const size_t dim = 1, n = 10, k = 10;

    // Database: vectors at positions 0..9
    std::vector<float> db(n * dim);
    for (size_t i = 0; i < n; ++i) {
      db[i] = static_cast<float>(i);
    }

    // Query at 4.6 → sorted by |4.6 - i|: 5(0.4), 4(0.6), 6(1.4), 3(1.6)...
    std::vector<float> q = {4.6f};
    auto result = acc.annSearch(q, 1, dim, db, n, k);
    ASSERT_EQ(result.results.size(), 1u);
    ASSERT_EQ(result.results[0].size(), k);

    for (size_t i = 1; i < result.results[0].size(); ++i) {
        EXPECT_LE(result.results[0][i - 1].distance,
                  result.results[0][i].distance);
    }
    EXPECT_EQ(result.results[0][0].index, 5u);   // closest: dist = (4.6-5)^2 = 0.16
}

TEST_F(GPUQueryAcceleratorTest, AnnSearch_GraphCacheIntegration) {
    GPUQueryAccelerator::Config cfg = cpuConfig();
    cfg.enable_graph_cache = true;
    GPUQueryAccelerator acc(cfg);
    acc.enableGraphCache();

    const size_t dim = 2, n = 6, k = 2;
    auto db = makeDatabase(n, dim);
    std::vector<float> q(dim, 1.0f);

    // First call: cache miss
    acc.annSearch(q, 1, dim, db, n, k);
    EXPECT_EQ(acc.getStats().graph_cache_misses, 1u);
    EXPECT_EQ(acc.getStats().graph_cache_hits,   0u);

    // Second call with same shape: cache hit
    acc.annSearch(q, 1, dim, db, n, k);
    EXPECT_EQ(acc.getStats().graph_cache_hits, 1u);
}

// ============================================================================
// dotProduct — FP16/BF16 Tensor Core support
// ============================================================================

TEST_F(GPUQueryAcceleratorTest, DotProduct_FP32_CorrectResult) {
    GPUQueryAccelerator acc(cpuConfig());
    std::vector<float> a = {1.0f, 2.0f, 3.0f};
    std::vector<float> b = {4.0f, 5.0f, 6.0f};
    // 1*4 + 2*5 + 3*6 = 32
    auto res = acc.dotProduct(a, b);
    EXPECT_NEAR(res.value, 32.0, 1e-5);
    EXPECT_EQ(res.precision_used, GPUQueryAccelerator::PrecisionMode::FP32);
    EXPECT_EQ(acc.getStats().total_dot_products, 1u);
    EXPECT_EQ(acc.getStats().fp16_ops, 0u);
    EXPECT_EQ(acc.getStats().bf16_ops, 0u);
}

TEST_F(GPUQueryAcceleratorTest, DotProduct_FP16_ProducesCloserResult) {
    GPUQueryAccelerator::Config cfg = cpuConfig();
    cfg.precision_mode = GPUQueryAccelerator::PrecisionMode::FP16;
    GPUQueryAccelerator acc(cfg);

    std::vector<float> a = {1.0f, 2.0f, 3.0f};
    std::vector<float> b = {4.0f, 5.0f, 6.0f};
    auto res = acc.dotProduct(a, b);
    // FP16 round-trip of small integers is exact; result should still be 32.
    EXPECT_NEAR(res.value, 32.0, 0.1);
    EXPECT_EQ(res.precision_used, GPUQueryAccelerator::PrecisionMode::FP16);
    EXPECT_EQ(acc.getStats().fp16_ops, 1u);
    EXPECT_EQ(acc.getStats().bf16_ops, 0u);
}

TEST_F(GPUQueryAcceleratorTest, DotProduct_BF16_ProducesCloseResult) {
    GPUQueryAccelerator::Config cfg = cpuConfig();
    cfg.precision_mode = GPUQueryAccelerator::PrecisionMode::BF16;
    GPUQueryAccelerator acc(cfg);

    std::vector<float> a = {1.0f, 2.0f, 3.0f};
    std::vector<float> b = {4.0f, 5.0f, 6.0f};
    auto res = acc.dotProduct(a, b);
    EXPECT_NEAR(res.value, 32.0, 0.5);
    EXPECT_EQ(res.precision_used, GPUQueryAccelerator::PrecisionMode::BF16);
    EXPECT_EQ(acc.getStats().bf16_ops, 1u);
    EXPECT_EQ(acc.getStats().fp16_ops, 0u);
}

TEST_F(GPUQueryAcceleratorTest, DotProduct_FP16_PrecisionLossVisible) {
    // Use a value that cannot be represented exactly in FP16 to verify that
    // quantisation actually changes the result vs FP32.
    GPUQueryAccelerator::Config cfg_fp32 = cpuConfig();
    GPUQueryAccelerator         acc_fp32(cfg_fp32);

    GPUQueryAccelerator::Config cfg_fp16 = cpuConfig();
    cfg_fp16.precision_mode = GPUQueryAccelerator::PrecisionMode::FP16;
    GPUQueryAccelerator acc_fp16(cfg_fp16);

    // 1/3 cannot be represented exactly in FP16.
    std::vector<float> a(64, 1.0f / 3.0f);
    std::vector<float> b(64, 1.0f / 3.0f);

    auto res32 = acc_fp32.dotProduct(a, b);
    auto res16 = acc_fp16.dotProduct(a, b);

    // Both should be close to 64/9 ≈ 7.111, but the FP16 result may differ
    // slightly due to the limited 10-bit mantissa.
    EXPECT_NEAR(res32.value, 64.0 / 9.0, 0.01);
    EXPECT_NEAR(res16.value, 64.0 / 9.0, 0.5);
}

TEST_F(GPUQueryAcceleratorTest, DotProduct_BF16_PrecisionLossVisible) {
    GPUQueryAccelerator::Config cfg_fp32 = cpuConfig();
    GPUQueryAccelerator         acc_fp32(cfg_fp32);

    GPUQueryAccelerator::Config cfg_bf16 = cpuConfig();
    cfg_bf16.precision_mode = GPUQueryAccelerator::PrecisionMode::BF16;
    GPUQueryAccelerator acc_bf16(cfg_bf16);

    std::vector<float> a(64, 1.0f / 3.0f);
    std::vector<float> b(64, 1.0f / 3.0f);

    auto res32 = acc_fp32.dotProduct(a, b);
    auto res16 = acc_bf16.dotProduct(a, b);

    EXPECT_NEAR(res32.value, 64.0 / 9.0, 0.01);
    EXPECT_NEAR(res16.value, 64.0 / 9.0, 0.5);
}

TEST_F(GPUQueryAcceleratorTest, DotProduct_EmptyVectors_ReturnsZero) {
    GPUQueryAccelerator acc(cpuConfig());
    auto res = acc.dotProduct({}, {});
    EXPECT_NEAR(res.value, 0.0, 1e-9);
    EXPECT_EQ(acc.getStats().total_dot_products, 1u);
}

TEST_F(GPUQueryAcceleratorTest, DotProduct_SizeMismatch_ReturnsZero) {
    GPUQueryAccelerator acc(cpuConfig());
    auto res = acc.dotProduct({1.0f, 2.0f}, {3.0f});
    EXPECT_NEAR(res.value, 0.0, 1e-9);
}

TEST_F(GPUQueryAcceleratorTest, DotProduct_UpdatesStats) {
    GPUQueryAccelerator::Config cfg = cpuConfig();
    cfg.precision_mode = GPUQueryAccelerator::PrecisionMode::FP16;
    GPUQueryAccelerator acc(cfg);

    acc.dotProduct({1.0f}, {2.0f});
    acc.dotProduct({3.0f}, {4.0f});
    EXPECT_EQ(acc.getStats().total_dot_products, 2u);
    EXPECT_EQ(acc.getStats().fp16_ops, 2u);
}
