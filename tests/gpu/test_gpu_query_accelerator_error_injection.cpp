/**
 * @file test_gpu_query_accelerator_error_injection.cpp
 * @brief Error injection and fallback tests for GPU Query Accelerator.
 *
 * Tests that GPU operations gracefully fall back to CPU when:
 * - CUDA memory allocation fails
 * - CUDA memory copy operations fail
 * - Kernel execution times out (exceeds SLA)
 * - Other GPU operations encounter errors
 *
 * Since we cannot directly inject CUDA errors in tests, we verify fallback
 * behavior by:
 * 1. Using force_cpu mode to simulate CPU-only operation
 * 2. Running on systems without CUDA to trigger automatic fallback
 * 3. Verifying stats reflect correct fallback behavior
 * 4. Verifying results are still correct despite fallback
 */

#include "themis/gpu/query_accelerator.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cmath>

using namespace themis::gpu;
using Row = GPUQueryAccelerator::Row;
using AggFunc = GPUQueryAccelerator::AggFunc;
using SortOrder = GPUQueryAccelerator::SortOrder;
using PrecisionMode = GPUQueryAccelerator::PrecisionMode;

// ============================================================================
// Test Fixture
// ============================================================================

class GPUQueryAcceleratorErrorInjectionTest : public ::testing::Test {
protected:
    /// Helper to build rows with sequential IDs and a float payload
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

    /// Helper to extract the float payload as a double
    static double payloadVal(const Row& r) {
        if (r.data.size() < sizeof(float)) {
          return 0.0;
        }
        float v;
        std::memcpy(&v, r.data.data(), sizeof(float));
        return static_cast<double>(v);
    }

    /// Config that forces CPU path
    GPUQueryAccelerator::Config cpuOnlyConfig() {
        GPUQueryAccelerator::Config cfg;
        cfg.force_cpu = true;
        return cfg;
    }

    /// Config that enables GPU but allows fallback
    GPUQueryAccelerator::Config gpuEnabledConfig() {
        GPUQueryAccelerator::Config cfg;
        cfg.force_cpu = false;
        return cfg;
    }
};

// ============================================================================
// Test: CPU Fallback Behavior
// ============================================================================

/// Test that forcing CPU mode produces deterministic results
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_DotProduct_FP32_ProducesResult) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    std::vector<float> a = {1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> b = {5.0f, 6.0f, 7.0f, 8.0f};

    auto result = acc.dotProduct(a, b);

    // Expected: 1*5 + 2*6 + 3*7 + 4*8 = 5 + 12 + 21 + 32 = 70
    EXPECT_NEAR(result.value, 70.0, 1e-5);
    EXPECT_FALSE(result.used_gpu) << "CPU-only mode should never use GPU";
}

/// Test that CPU path correctly handles FP16 quantization
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_DotProduct_FP16_QuantizationConsistent) {
    GPUQueryAccelerator::Config cfg = cpuOnlyConfig();
    cfg.precision_mode = PrecisionMode::FP16;
    GPUQueryAccelerator acc(cfg);

    std::vector<float> a = {1.5f, 2.5f, 3.5f};
    std::vector<float> b = {4.5f, 5.5f, 6.5f};

    auto result = acc.dotProduct(a, b);

    // Even with FP16 quantization, should be reasonably close to the exact result
    // Exact: 1.5*4.5 + 2.5*5.5 + 3.5*6.5 = 6.75 + 13.75 + 22.75 = 43.25
    EXPECT_NEAR(result.value, 43.25, 0.2);
    EXPECT_FALSE(result.used_gpu);
}

/// Test that CPU path correctly handles BF16 quantization
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_DotProduct_BF16_QuantizationConsistent) {
    GPUQueryAccelerator::Config cfg = cpuOnlyConfig();
    cfg.precision_mode = PrecisionMode::BF16;
    GPUQueryAccelerator acc(cfg);

    std::vector<float> a = {2.0f, 4.0f, 6.0f};
    std::vector<float> b = {3.0f, 5.0f, 7.0f};

    auto result = acc.dotProduct(a, b);

    // Expected: 2*3 + 4*5 + 6*7 = 6 + 20 + 42 = 68
    EXPECT_NEAR(result.value, 68.0, 0.5);
    EXPECT_FALSE(result.used_gpu);
}

/// Test that CPU scan with filter produces correct results
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_Scan_FilterCorrect) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    auto rows = makeRows(100);
    auto result = acc.scan(rows, [](const Row& r) { return r.id % 2 == 0; });

    EXPECT_EQ(result.rows_scanned, 100u);
    EXPECT_EQ(result.rows_passed, 50u);
    EXPECT_FALSE(result.used_gpu);

    // Verify all results pass the filter
    for (const auto& r : result.rows) {
        EXPECT_EQ(r.id % 2, 0u);
    }
}

/// Test that CPU sort (ascending) produces correct results
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_Sort_AscendingCorrect) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    auto rows = makeRows(50);
    // Intentionally use unsorted order to verify sort works
    std::reverse(rows.begin(), rows.end());

    auto result = acc.sort(rows, [](const Row& r) { return r.id; }, SortOrder::ASC);

    EXPECT_FALSE(result.used_gpu);
    EXPECT_EQ(result.rows.size(), 50u);

    // Verify sorted order
    for (size_t i = 1; i < result.rows.size(); ++i) {
        EXPECT_LE(result.rows[i - 1].id, result.rows[i].id);
    }
}

/// Test that CPU sort (descending) produces correct results
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_Sort_DescendingCorrect) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    auto rows = makeRows(50);
    // Intentionally sort in ascending order first
    std::sort(rows.begin(), rows.end(),
              [](const Row& a, const Row& b) { return a.id < b.id; });

    auto result = acc.sort(rows, [](const Row& r) { return r.id; }, SortOrder::DESC);

    EXPECT_FALSE(result.used_gpu);
    EXPECT_EQ(result.rows.size(), 50u);

    // Verify reverse sorted order
    for (size_t i = 1; i < result.rows.size(); ++i) {
        EXPECT_GE(result.rows[i - 1].id, result.rows[i].id);
    }
}

/// Test that CPU aggregate (SUM) produces correct results
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_Aggregate_SUM_Correct) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    auto rows = makeRows(10, 1.0);
    auto result = acc.aggregate(rows, AggFunc::SUM, [](const Row& r) { return payloadVal(r); });

    EXPECT_FALSE(result.used_gpu);
    
    // Sum of [1, 2, 3, ..., 10] = 55
    double expected = 0.0;
    for (int i = 1; i <= 10; ++i) {
        expected += static_cast<double>(i);
    }
    EXPECT_NEAR(result.value, expected, 1e-5);
}

/// Test that CPU aggregate (COUNT) produces correct results
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_Aggregate_COUNT_Correct) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    auto rows = makeRows(25);
    auto result = acc.aggregate(rows, AggFunc::COUNT, [](const Row& r) { return payloadVal(r); });

    EXPECT_FALSE(result.used_gpu);
    EXPECT_EQ(static_cast<int>(result.value), 25);
}

/// Test that CPU aggregate (MIN) produces correct results
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_Aggregate_MIN_Correct) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    auto rows = makeRows(10, 5.0);
    auto result = acc.aggregate(rows, AggFunc::MIN, [](const Row& r) { return payloadVal(r); });

    EXPECT_FALSE(result.used_gpu);
    EXPECT_NEAR(result.value, 5.0, 1e-5); // First row has value 5.0
}

/// Test that CPU aggregate (MAX) produces correct results
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_Aggregate_MAX_Correct) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    auto rows = makeRows(10, 5.0);
    auto result = acc.aggregate(rows, AggFunc::MAX, [](const Row& r) { return payloadVal(r); });

    EXPECT_FALSE(result.used_gpu);
    EXPECT_NEAR(result.value, 14.0, 1e-5); // Last row has value 5.0 + 9 = 14.0
}

/// Test that CPU hashJoin produces correct results
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_HashJoin_CorrectMatches) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    // Create two sets with overlapping IDs
    std::vector<Row> left = makeRows(10, 1.0);   // IDs: 0-9
    std::vector<Row> right = makeRows(10, 11.0); // IDs: 0-9

    auto result = acc.hashJoin(
        left, right,
        [](const Row& r) { return r.id; },     // left key
        [](const Row& r) { return r.id; }      // right key
    );

    EXPECT_FALSE(result.used_gpu);
    EXPECT_EQ(result.pairs.size(), 10u); // All 10 IDs should match
}

/// Test that CPU hashJoin correctly returns empty result when no matches
TEST_F(GPUQueryAcceleratorErrorInjectionTest, ForceCPU_HashJoin_NoMatches) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    // Create two sets with non-overlapping IDs
    std::vector<Row> left;
    for (size_t i = 0; i < 10; ++i) {
        Row r;
        r.id = i;
        r.data.resize(sizeof(float));
        float v = static_cast<float>(1.0 + i);
        std::memcpy(r.data.data(), &v, sizeof(float));
        left.push_back(r);
    }

    std::vector<Row> right;
    for (size_t i = 10; i < 20; ++i) {
        Row r;
        r.id = i;
        r.data.resize(sizeof(float));
        float v = static_cast<float>(11.0 + (i - 10));
        std::memcpy(r.data.data(), &v, sizeof(float));
        right.push_back(r);
    }

    auto result = acc.hashJoin(
        left, right,
        [](const Row& r) { return r.id; },
        [](const Row& r) { return r.id; }
    );

    EXPECT_FALSE(result.used_gpu);
    EXPECT_EQ(result.pairs.size(), 0u);
}

// ============================================================================
// Test: Parity Between CPU and GPU Paths (when GPU available)
// ============================================================================

/// Test that GPU (when available) and CPU produce same dotProduct result (FP32)
TEST_F(GPUQueryAcceleratorErrorInjectionTest, GPUEnabled_DotProduct_FP32_ParityWithCPU) {
    GPUQueryAccelerator acc_gpu(gpuEnabledConfig());
    GPUQueryAccelerator acc_cpu(cpuOnlyConfig());

    std::vector<float> a = {1.0f, 2.0f, 3.0f, 4.0f};
    std::vector<float> b = {5.0f, 6.0f, 7.0f, 8.0f};

    auto result_gpu = acc_gpu.dotProduct(a, b);
    auto result_cpu = acc_cpu.dotProduct(a, b);

    // Results should match (GPU will fallback to CPU if no device)
    EXPECT_NEAR(result_gpu.value, result_cpu.value, 1e-5);
    EXPECT_EQ(result_gpu.precision_used, result_cpu.precision_used);
}

/// Test that GPU (when available) and CPU produce same dotProduct result (FP16)
TEST_F(GPUQueryAcceleratorErrorInjectionTest, GPUEnabled_DotProduct_FP16_ParityWithCPU) {
    GPUQueryAccelerator::Config cfg_gpu = gpuEnabledConfig();
    cfg_gpu.precision_mode = PrecisionMode::FP16;
    GPUQueryAccelerator acc_gpu(cfg_gpu);

    GPUQueryAccelerator::Config cfg_cpu = cpuOnlyConfig();
    cfg_cpu.precision_mode = PrecisionMode::FP16;
    GPUQueryAccelerator acc_cpu(cfg_cpu);

    std::vector<float> a = {1.0f, 2.0f, 3.0f};
    std::vector<float> b = {4.0f, 5.0f, 6.0f};

    auto result_gpu = acc_gpu.dotProduct(a, b);
    auto result_cpu = acc_cpu.dotProduct(a, b);

    // With FP16, allow slightly larger tolerance
    EXPECT_NEAR(result_gpu.value, result_cpu.value, 0.2);
}

/// Test that GPU (when available) and CPU produce same dotProduct result (BF16)
TEST_F(GPUQueryAcceleratorErrorInjectionTest, GPUEnabled_DotProduct_BF16_ParityWithCPU) {
    GPUQueryAccelerator::Config cfg_gpu = gpuEnabledConfig();
    cfg_gpu.precision_mode = PrecisionMode::BF16;
    GPUQueryAccelerator acc_gpu(cfg_gpu);

    GPUQueryAccelerator::Config cfg_cpu = cpuOnlyConfig();
    cfg_cpu.precision_mode = PrecisionMode::BF16;
    GPUQueryAccelerator acc_cpu(cfg_cpu);

    std::vector<float> a = {1.0f, 2.0f, 3.0f};
    std::vector<float> b = {4.0f, 5.0f, 6.0f};

    auto result_gpu = acc_gpu.dotProduct(a, b);
    auto result_cpu = acc_cpu.dotProduct(a, b);

    // BF16 has even coarser precision
    EXPECT_NEAR(result_gpu.value, result_cpu.value, 0.5);
}

/// Test that GPU (when available) and CPU produce same scan results
TEST_F(GPUQueryAcceleratorErrorInjectionTest, GPUEnabled_Scan_ParityWithCPU) {
    GPUQueryAccelerator acc_gpu(gpuEnabledConfig());
    GPUQueryAccelerator acc_cpu(cpuOnlyConfig());

    auto rows = makeRows(50);
    auto filter = [](const Row& r) { return r.id % 3 == 0; };

    auto result_gpu = acc_gpu.scan(rows, filter);
    auto result_cpu = acc_cpu.scan(rows, filter);

    EXPECT_EQ(result_gpu.rows_scanned, result_cpu.rows_scanned);
    EXPECT_EQ(result_gpu.rows_passed, result_cpu.rows_passed);
    EXPECT_EQ(result_gpu.rows.size(), result_cpu.rows.size());
}

// ============================================================================
// Test: Stats Verification
// ============================================================================

/// Test that stats are correctly updated when CPU path is used
TEST_F(GPUQueryAcceleratorErrorInjectionTest, Stats_CPUFallback_UpdatesCounters) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    acc.dotProduct({1.0f, 2.0f}, {3.0f, 4.0f});
    acc.dotProduct({5.0f}, {6.0f});

    auto stats = acc.getStats();
    EXPECT_EQ(stats.total_dot_products, 2u);
}

/// Test that resetStats clears all counters
TEST_F(GPUQueryAcceleratorErrorInjectionTest, Stats_ResetStats_ClearsAllCounters) {
    GPUQueryAccelerator acc(cpuOnlyConfig());

    acc.dotProduct({1.0f}, {2.0f});
    acc.scan(makeRows(10));
    acc.sort(makeRows(5), [](const Row& r) { return r.id; }, SortOrder::ASC);

    auto stats_before = acc.getStats();
    EXPECT_GT(stats_before.total_dot_products, 0u);
    EXPECT_GT(stats_before.total_scans, 0u);
    EXPECT_GT(stats_before.total_sorts, 0u);

    acc.resetStats();
    auto stats_after = acc.getStats();

    EXPECT_EQ(stats_after.total_dot_products, 0u);
    EXPECT_EQ(stats_after.total_scans, 0u);
    EXPECT_EQ(stats_after.total_sorts, 0u);
}

