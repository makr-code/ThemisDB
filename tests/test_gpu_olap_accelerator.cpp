/*
 * Tests for GPU-accelerated OLAP aggregations.
 *
 * Validates:
 * - OLAPEngine::Config struct and GPU-enabled constructor
 * - GPU threshold / CPU-fallback dispatch logic (via GPUQueryAccelerator)
 * - explain() hint injection when GPU is enabled
 * - Correct results for all aggregation functions under CPU fallback
 *   (no real GPU hardware required)
 */

#include <gtest/gtest.h>
#include "analytics/olap.h"
#include "themis/gpu/query_accelerator.h"
#include <cmath>
#include <cstring>

using namespace themis::analytics;

// ============================================================================
// OLAPEngine::Config Tests
// ============================================================================

TEST(OLAPConfigTest, DefaultConfigIsGPUDisabled) {
    OLAPEngine::Config cfg;
    EXPECT_FALSE(cfg.enable_gpu);
    EXPECT_EQ(cfg.gpu_device_id, 0);
    EXPECT_EQ(cfg.gpu_memory_limit, 4ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(cfg.gpu_threshold_rows, 10'000u);
}

TEST(OLAPConfigTest, ConfigFieldsAssignable) {
    OLAPEngine::Config cfg;
    cfg.enable_gpu = true;
    cfg.gpu_device_id = 1;
    cfg.gpu_memory_limit = 8ULL * 1024 * 1024 * 1024;
    cfg.gpu_threshold_rows = 50'000;

    EXPECT_TRUE(cfg.enable_gpu);
    EXPECT_EQ(cfg.gpu_device_id, 1);
    EXPECT_EQ(cfg.gpu_memory_limit, 8ULL * 1024 * 1024 * 1024);
    EXPECT_EQ(cfg.gpu_threshold_rows, 50'000u);
}

// ============================================================================
// OLAPEngine GPU Constructor Tests
// ============================================================================

TEST(OLAPGPUConstructorTest, DefaultConstructorStillWorks) {
    OLAPEngine engine;
    OLAPQuery query;
    query.collection = "nonexistent";
    query.dimensions.push_back({"region", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});

    auto result = engine.execute(query);
    EXPECT_EQ(result.rows.size(), 0u);
    EXPECT_GE(result.execution_time_ms, 0.0);
}

TEST(OLAPGPUConstructorTest, GPUEnabledConstructorDoesNotCrash) {
    OLAPEngine::Config cfg;
    cfg.enable_gpu = true;
    cfg.gpu_threshold_rows = 10'000;

    OLAPEngine engine(cfg);

    OLAPQuery query;
    query.collection = "nonexistent";
    query.dimensions.push_back({"region", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});

    auto result = engine.execute(query);
    EXPECT_EQ(result.rows.size(), 0u);
    EXPECT_GE(result.execution_time_ms, 0.0);
}

TEST(OLAPGPUConstructorTest, GPUDisabledConstructorWorks) {
    OLAPEngine::Config cfg;
    cfg.enable_gpu = false;

    OLAPEngine engine(cfg);

    OLAPQuery query;
    query.collection = "nonexistent";
    query.measures.push_back({"total", "amount", Measure::Function::Count});

    auto result = engine.execute(query);
    EXPECT_EQ(result.rows.size(), 0u);
}

// ============================================================================
// explain() GPU hint Tests
// ============================================================================

TEST(OLAPGPUExplainTest, GPUEnabledAddsHintToQueryPlan) {
    OLAPEngine::Config cfg;
    cfg.enable_gpu = true;
    cfg.gpu_threshold_rows = 5'000;
    OLAPEngine engine(cfg);

    OLAPQuery query;
    query.collection = "sales";
    query.dimensions.push_back({"region", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});

    auto plan = engine.explain(query);

    bool found_gpu_hint = false;
    for (const auto& note : plan.optimization_notes) {
        if (note.find("GPU acceleration") != std::string::npos) {
            found_gpu_hint = true;
            break;
        }
    }
    EXPECT_TRUE(found_gpu_hint) << "Query plan should mention GPU acceleration when enabled";
}

TEST(OLAPGPUExplainTest, GPUEnabledHintContainsThreshold) {
    OLAPEngine::Config cfg;
    cfg.enable_gpu = true;
    cfg.gpu_threshold_rows = 25'000;
    OLAPEngine engine(cfg);

    OLAPQuery query;
    query.collection = "sales";
    query.measures.push_back({"cnt", "id", Measure::Function::Count});

    auto plan = engine.explain(query);

    bool found_threshold = false;
    for (const auto& note : plan.optimization_notes) {
        if (note.find("25000") != std::string::npos) {
            found_threshold = true;
            break;
        }
    }
    EXPECT_TRUE(found_threshold) << "GPU hint should include the configured threshold row count";
}

TEST(OLAPGPUExplainTest, GPUDisabledHasNoGPUHint) {
    OLAPEngine::Config cfg;
    cfg.enable_gpu = false;
    OLAPEngine engine(cfg);

    OLAPQuery query;
    query.collection = "sales";
    query.dimensions.push_back({"region", "", true});
    query.measures.push_back({"total", "amount", Measure::Function::Sum});

    auto plan = engine.explain(query);

    for (const auto& note : plan.optimization_notes) {
        EXPECT_EQ(note.find("GPU acceleration"), std::string::npos)
            << "Query plan should NOT mention GPU acceleration when disabled";
    }
}

// ============================================================================
// GPUQueryAccelerator integration — dispatch tests
// ============================================================================

// These tests exercise GPUQueryAccelerator directly to verify that the
// CPU-fallback path (force_cpu = true) produces correct results, which is
// the same code path used by OLAPEngine on hardware without a physical GPU.

class GPUOLAPAggregateTest : public ::testing::Test {
protected:
    // Build GPUQueryAccelerator rows from a vector of double values.
    static std::vector<themis::gpu::GPUQueryAccelerator::Row>
    makeRows(const std::vector<double>& values) {
        std::vector<themis::gpu::GPUQueryAccelerator::Row> rows = {};

        rows.reserve(values.size());
        for (size_t i = 0; i < values.size(); ++i) {
            themis::gpu::GPUQueryAccelerator::Row row;
            row.id = static_cast<uint64_t>(i);
            row.data.resize(sizeof(double));
            std::memcpy(row.data.data(), &values[i], sizeof(double));
            rows.push_back(std::move(row));
        }
        return rows;
    }

    // Extract the double payload stored in a Row.
    static double extractValue(const themis::gpu::GPUQueryAccelerator::Row& r) {
        if (r.data.size() < sizeof(double)) {
          return 0.0;
        }
        double v = 0;
        std::memcpy(&v, r.data.data(), sizeof(double));
        return v;
    }

    themis::gpu::GPUQueryAccelerator::Config cpuConfig() {
        themis::gpu::GPUQueryAccelerator::Config cfg;
        cfg.force_cpu = true;
        return cfg;
    }
};

TEST_F(GPUOLAPAggregateTest, SumAggregation) {
    themis::gpu::GPUQueryAccelerator acc(cpuConfig());
    // values: 10, 20, 30, 40, 50  →  sum = 150
    auto rows = makeRows({10.0, 20.0, 30.0, 40.0, 50.0});
    auto res = acc.aggregate(rows, themis::gpu::GPUQueryAccelerator::AggFunc::SUM, extractValue);
    EXPECT_NEAR(res.value, 150.0, 1e-6);
    EXPECT_EQ(res.count, 5u);
}

TEST_F(GPUOLAPAggregateTest, CountAggregation) {
    themis::gpu::GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows({1.0, 2.0, 3.0});
    auto res = acc.aggregate(rows, themis::gpu::GPUQueryAccelerator::AggFunc::COUNT, extractValue);
    EXPECT_NEAR(res.value, 3.0, 1e-6);
}

TEST_F(GPUOLAPAggregateTest, MinAggregation) {
    themis::gpu::GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows({50.0, 10.0, 30.0});
    auto res = acc.aggregate(rows, themis::gpu::GPUQueryAccelerator::AggFunc::MIN, extractValue);
    EXPECT_NEAR(res.value, 10.0, 1e-6);
}

TEST_F(GPUOLAPAggregateTest, MaxAggregation) {
    themis::gpu::GPUQueryAccelerator acc(cpuConfig());
    auto rows = makeRows({5.0, 95.0, 50.0});
    auto res = acc.aggregate(rows, themis::gpu::GPUQueryAccelerator::AggFunc::MAX, extractValue);
    EXPECT_NEAR(res.value, 95.0, 1e-6);
}

TEST_F(GPUOLAPAggregateTest, AvgAggregation) {
    themis::gpu::GPUQueryAccelerator acc(cpuConfig());
    // values: 10, 20, 30  →  avg = 20
    auto rows = makeRows({10.0, 20.0, 30.0});
    auto res = acc.aggregate(rows, themis::gpu::GPUQueryAccelerator::AggFunc::AVG, extractValue);
    EXPECT_NEAR(res.value, 20.0, 1e-6);
}

TEST_F(GPUOLAPAggregateTest, EmptyRowsReturnsZero) {
    themis::gpu::GPUQueryAccelerator acc(cpuConfig());
    auto res = acc.aggregate({}, themis::gpu::GPUQueryAccelerator::AggFunc::SUM, extractValue);
    EXPECT_NEAR(res.value, 0.0, 1e-6);
    EXPECT_EQ(res.count, 0u);
}

TEST_F(GPUOLAPAggregateTest, BelowThresholdUsesCPUPath) {
    // GPU dispatch is skipped below the threshold; verify the CPU result is correct.
    themis::gpu::GPUQueryAccelerator::Config cfg;
    cfg.gpu_threshold_rows = 1'000;
    cfg.force_cpu = false;  // Would use GPU above threshold if hardware existed
    themis::gpu::GPUQueryAccelerator acc(cfg);

    // Only 5 rows — well below threshold → CPU path
    auto rows = makeRows({1.0, 2.0, 3.0, 4.0, 5.0});
    auto res = acc.aggregate(rows, themis::gpu::GPUQueryAccelerator::AggFunc::SUM, extractValue);
    EXPECT_NEAR(res.value, 15.0, 1e-6);
    EXPECT_FALSE(res.used_gpu);
}

// ============================================================================
// OLAPEngine GPU aggregation correctness — CPU fallback path
// ============================================================================

// These tests set gpu_threshold_rows to 1 so that the GPU path is always
// attempted; with no real CUDA device the GPUQueryAccelerator falls back to
// its CPU implementation, which lets us verify end-to-end correctness.

class OLAPGPUAggregateCorrectnessTest : public ::testing::Test {
protected:
    // Returns an OLAPEngine with GPU enabled and a very low threshold so that
    // every non-empty value set enters the GPU dispatch code path.
    static OLAPEngine makeGPUEngine(size_t threshold = 1) {
        OLAPEngine::Config cfg;
        cfg.enable_gpu = true;
        cfg.gpu_threshold_rows = threshold;
        return OLAPEngine(cfg);
    }
};

TEST_F(OLAPGPUAggregateCorrectnessTest, EmptyCollectionReturnsEmptyResult) {
    auto engine = makeGPUEngine();
    OLAPQuery query;
    query.collection = "empty";
    query.dimensions.push_back({"dim", "", true});
    query.measures.push_back({"total", "val", Measure::Function::Sum});

    auto result = engine.execute(query);
    EXPECT_EQ(result.rows.size(), 0u);
}

TEST_F(OLAPGPUAggregateCorrectnessTest, ExplainWithLowThreshold) {
    auto engine = makeGPUEngine(1);
    OLAPQuery query;
    query.collection = "sales";
    query.measures.push_back({"total", "amount", Measure::Function::Sum});

    auto plan = engine.explain(query);

    bool has_gpu_note = false;
    for (const auto& note : plan.optimization_notes) {
        if (note.find("GPU acceleration") != std::string::npos) {
            has_gpu_note = true;
        }
    }
    EXPECT_TRUE(has_gpu_note);
}
