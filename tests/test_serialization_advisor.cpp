/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_serialization_advisor.cpp                     ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-20                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Tests:           SA-01 .. SA-12  (SerializationStrategyAdvisor)     ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Tests for SerializationStrategyAdvisor (OptimizerCostModel::adviseSerializationStrategy),
// serialization-related fields on QueryCostRecord and Plan, new
// AdaptivePlanSelector strategies, and calibration-factor emission.

#include <gtest/gtest.h>
#include "query/optimizer_cost_model.h"
#include "query/query_optimizer.h"
#include "query/adaptive_optimizer.h"
#include "performance/phase3/per_query_cost_model.h"

#include <string>
#include <thread>
#include <chrono>

using namespace themis;
using namespace themis::performance::phase3;
using Format  = OptimizerCostModel::SerializationAdvice::Format;
using WLType  = OptimizerCostModel::SerializationWorkloadType;
using Strategy = AdaptivePlanSelector::PlanChoice::Strategy;

// ============================================================
// SA-01: Small CPU workload → BINARY_BATCH_CPU
// ============================================================
TEST(SerializationAdvisorTests, SA01_SmallCpuWorkload_BinaryBatchCpu) {
    OptimizerCostModel model;
    auto advice = model.adviseSerializationStrategy(WLType::CPU_BATCH, /*rows=*/100);
    EXPECT_EQ(advice.format, Format::BINARY_BATCH_CPU);
    EXPECT_FALSE(advice.gpu_capable);
    EXPECT_FALSE(advice.description.empty());
}

// ============================================================
// SA-02: Large CPU workload (above msgpack threshold) → ARROW_CPU_PARALLEL
// ============================================================
TEST(SerializationAdvisorTests, SA02_LargeCpuWorkload_ArrowCpuParallel) {
    OptimizerCostModel model;
    const size_t rows = model.getConstants().msgpackRowThreshold + 1;
    auto advice = model.adviseSerializationStrategy(WLType::CPU_PARALLEL, rows);
    EXPECT_EQ(advice.format, Format::ARROW_CPU_PARALLEL);
    EXPECT_FALSE(advice.gpu_capable);
}

// ============================================================
// SA-03: GPU workload above gpuRowThresholdLow → ARROW_GPU_VRAM
// ============================================================
TEST(SerializationAdvisorTests, SA03_GpuWorkloadAboveThreshold_ArrowGpuVram) {
    OptimizerCostModel model;
    const size_t rows = model.getConstants().gpuRowThresholdLow + 1;
    auto advice = model.adviseSerializationStrategy(WLType::GPU_VRAM, rows);
    EXPECT_EQ(advice.format, Format::ARROW_GPU_VRAM);
    EXPECT_TRUE(advice.gpu_capable);
}

// ============================================================
// SA-04: GPU workload below gpuRowThresholdLow → BINARY_BATCH_CPU
// ============================================================
TEST(SerializationAdvisorTests, SA04_GpuWorkloadBelowThreshold_BinaryBatchCpu) {
    OptimizerCostModel model;
    const size_t rows = model.getConstants().gpuRowThresholdLow - 1;
    auto advice = model.adviseSerializationStrategy(WLType::GPU_VRAM, rows);
    EXPECT_EQ(advice.format, Format::BINARY_BATCH_CPU);
    EXPECT_FALSE(advice.gpu_capable);
}

// ============================================================
// SA-05: Zero rows → BINARY_BATCH_CPU (never GPU or Arrow parallel)
// ============================================================
TEST(SerializationAdvisorTests, SA05_ZeroRows_BinaryBatchCpu) {
    OptimizerCostModel model;
    auto advice = model.adviseSerializationStrategy(WLType::GPU_VRAM, 0);
    EXPECT_EQ(advice.format, Format::BINARY_BATCH_CPU);
}

// ============================================================
// SA-06: Exactly at gpuRowThresholdLow → ARROW_GPU_VRAM (boundary inclusive)
// ============================================================
TEST(SerializationAdvisorTests, SA06_ExactlyAtGpuThreshold_ArrowGpuVram) {
    OptimizerCostModel model;
    const size_t rows = model.getConstants().gpuRowThresholdLow;
    auto advice = model.adviseSerializationStrategy(WLType::GPU_VRAM, rows);
    EXPECT_EQ(advice.format, Format::ARROW_GPU_VRAM);
}

// ============================================================
// SA-07: Custom CostConstants thresholds are respected
// ============================================================
TEST(SerializationAdvisorTests, SA07_CustomThresholds_Respected) {
    OptimizerCostModel::CostConstants custom;
    custom.gpuRowThresholdLow  = 1'000;
    custom.msgpackRowThreshold = 500;
    OptimizerCostModel model(custom);

    // 600 rows: above msgpack (500), below GPU (1000) → ARROW_CPU_PARALLEL
    auto cpu_advice = model.adviseSerializationStrategy(WLType::CPU_BATCH, 600);
    EXPECT_EQ(cpu_advice.format, Format::ARROW_CPU_PARALLEL);

    // 1000 rows on GPU: exactly at custom GPU threshold → ARROW_GPU_VRAM
    auto gpu_advice = model.adviseSerializationStrategy(WLType::GPU_VRAM, 1'000);
    EXPECT_EQ(gpu_advice.format, Format::ARROW_GPU_VRAM);
}

// ============================================================
// SA-08: Plan struct carries a serialization_advice field
// ============================================================
TEST(SerializationAdvisorTests, SA08_PlanStruct_HasSerializationAdviceField) {
    QueryOptimizer::Plan plan;
    // Default-constructed advice should be BINARY_BATCH_CPU
    EXPECT_EQ(plan.serialization_advice.format, Format::BINARY_BATCH_CPU);
    EXPECT_FALSE(plan.serialization_advice.gpu_capable);

    // Mutate it
    plan.serialization_advice.format      = Format::ARROW_GPU_VRAM;
    plan.serialization_advice.gpu_capable = true;
    EXPECT_EQ(plan.serialization_advice.format, Format::ARROW_GPU_VRAM);
    EXPECT_TRUE(plan.serialization_advice.gpu_capable);
}

// ============================================================
// SA-09: QueryCostRecord carries exec_path_used + serialization_time_ms
// ============================================================
TEST(SerializationAdvisorTests, SA09_QueryCostRecord_HasSerializationFields) {
    QueryCostRecord rec;
    EXPECT_TRUE(rec.exec_path_used.empty());
    EXPECT_DOUBLE_EQ(rec.serialization_time_ms, 0.0);

    rec.exec_path_used        = "ARROW_GPU_VRAM";
    rec.serialization_time_ms = 3.14;
    EXPECT_EQ(rec.exec_path_used, "ARROW_GPU_VRAM");
    EXPECT_DOUBLE_EQ(rec.serialization_time_ms, 3.14);
}

// ============================================================
// SA-10: AdaptivePlanSelector::Strategy includes serialization strategies
// ============================================================
TEST(SerializationAdvisorTests, SA10_AdaptivePlanSelector_HasSerializationStrategies) {
    // Just verify the enum values compile and are distinct
    EXPECT_NE(Strategy::BINARY_BATCH_CPU,   Strategy::ARROW_CPU_PARALLEL);
    EXPECT_NE(Strategy::ARROW_CPU_PARALLEL, Strategy::ARROW_GPU_VRAM);
    EXPECT_NE(Strategy::BINARY_BATCH_CPU,   Strategy::ARROW_GPU_VRAM);

    // They should not collide with the existing query strategies
    EXPECT_NE(Strategy::BINARY_BATCH_CPU,   Strategy::INDEX_SCAN);
    EXPECT_NE(Strategy::ARROW_GPU_VRAM,     Strategy::TABLE_SCAN);
}

// ============================================================
// SA-11: getCalibrationFactors emits gpu_row_threshold_low after recording rows
// ============================================================
TEST(SerializationAdvisorTests, SA11_CalibrationFactors_EmitGpuRowThresholdLow) {
    PerQueryCostModel pcm;

    // Record a query with 100k rows so we get above the min clamp
    {
        auto guard = pcm.beginQuery("table_scan", 1.0);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        guard.end(/*rows=*/100'000, /*pages=*/0);
    }

    auto factors = pcm.getCalibrationFactors();
    ASSERT_NE(factors.find("gpu_row_threshold_low"), factors.end())
        << "Expected gpu_row_threshold_low in calibration factors";
    EXPECT_GT(factors.at("gpu_row_threshold_low"), 0.0);
}

// ============================================================
// SA-12: getCalibrationFactors emits msgpack_row_threshold after recording rows
// ============================================================
TEST(SerializationAdvisorTests, SA12_CalibrationFactors_EmitMsgpackRowThreshold) {
    PerQueryCostModel pcm;

    {
        auto guard = pcm.beginQuery("index_scan", 2.0);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        guard.end(/*rows=*/50'000, /*pages=*/0);
    }

    auto factors = pcm.getCalibrationFactors();
    ASSERT_NE(factors.find("msgpack_row_threshold"), factors.end())
        << "Expected msgpack_row_threshold in calibration factors";
    EXPECT_GT(factors.at("msgpack_row_threshold"), 0.0);

    // msgpack threshold must be <= gpu threshold (Arrow CPU before GPU)
    if (factors.count("gpu_row_threshold_low")) {
        EXPECT_LE(factors.at("msgpack_row_threshold"),
                  factors.at("gpu_row_threshold_low"));
    }
}
