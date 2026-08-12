/**
 * @file benchmark_matrix_test.cc
 * @brief Unit tests for `BenchmarkMatrix` (Phase 4 of Issue #5438).
 *
 * Test categories:
 * - Basic record / lookup operations
 * - Edge-case flag propagation (stale artifact, shard mismatch, etc.)
 * - Scenario and dimension slices
 * - Full-coverage filtering
 * - Cross-scenario comparison
 * - Best-scenario selection
 * - Invalidation (scenario, dimension, full clear)
 * - Error handling (invalid inputs)
 */

#include "benchmark_matrix.h"

#include <gtest/gtest.h>

using namespace themis::evaluation;

// ============================================================================
// Fixture helpers
// ============================================================================

namespace {

/// Build a clean BenchmarkResult with a given value.
BenchmarkResult makeClean(double value, uint32_t samples = 10,
                          double stddev = 0.0) {
    BenchmarkResult r;
    r.value        = value;
    r.stddev       = stddev;
    r.sample_count = samples;
    r.edge_flags   = BenchmarkEdgeCase::NONE;
    return r;
}

/// Build an anomalous BenchmarkResult.
BenchmarkResult makeAnomalous(double value, BenchmarkEdgeCase flags,
                               uint32_t samples = 5) {
    BenchmarkResult r;
    r.value        = value;
    r.sample_count = samples;
    r.edge_flags   = flags;
    return r;
}

} // namespace

// ============================================================================
// Basic record / lookup
// ============================================================================

TEST(BenchmarkMatrixTest, EmptyMatrixLookupReturnsNullopt) {
    BenchmarkMatrix m;
    EXPECT_FALSE(m.lookup(BenchmarkScenario::HNSW_ANN_ONLY,
                          BenchmarkDimension::RECALL_AT_K).has_value());
}

TEST(BenchmarkMatrixTest, RecordAndLookupRoundTrip) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::RECALL_AT_K,
             makeClean(0.95));

    auto r = m.lookup(BenchmarkScenario::HNSW_ANN_ONLY,
                      BenchmarkDimension::RECALL_AT_K);
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(0.95, r->value);
    EXPECT_TRUE(r->isClean());
}

TEST(BenchmarkMatrixTest, RecordOverwritesPreviousValue) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY,
             BenchmarkDimension::QUERY_LATENCY_MS,
             makeClean(5.0));
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY,
             BenchmarkDimension::QUERY_LATENCY_MS,
             makeClean(3.8));

    auto r = m.lookup(BenchmarkScenario::DISKANN_ANN_ONLY,
                      BenchmarkDimension::QUERY_LATENCY_MS);
    ASSERT_TRUE(r.has_value());
    EXPECT_DOUBLE_EQ(3.8, r->value);
}

TEST(BenchmarkMatrixTest, SizeReflectsRecordedCells) {
    BenchmarkMatrix m;
    EXPECT_EQ(0u, m.size());
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::COMPRESSION_RATIO,
             makeClean(4.2));
    EXPECT_EQ(1u, m.size());
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::QUERY_LATENCY_MS,
             makeClean(12.3));
    EXPECT_EQ(2u, m.size());
}

TEST(BenchmarkMatrixTest, ContainsReturnsTrueForRecordedCell) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::LLM_TENSOR_COMPRESSED,
             BenchmarkDimension::FAITHFULNESS_SCORE,
             makeClean(0.88));
    EXPECT_TRUE(m.contains(BenchmarkScenario::LLM_TENSOR_COMPRESSED,
                            BenchmarkDimension::FAITHFULNESS_SCORE));
    EXPECT_FALSE(m.contains(BenchmarkScenario::LLM_FULL_PROMPT,
                             BenchmarkDimension::FAITHFULNESS_SCORE));
}

// ============================================================================
// Error handling — invalid inputs
// ============================================================================

TEST(BenchmarkMatrixTest, RecordThrowsOnZeroSampleCleanResult) {
    BenchmarkMatrix m;
    BenchmarkResult bad;
    bad.value        = 0.9;
    bad.sample_count = 0; // vacuous
    bad.edge_flags   = BenchmarkEdgeCase::NONE;

    EXPECT_THROW(
        m.record(BenchmarkScenario::HNSW_ANN_ONLY,
                 BenchmarkDimension::RECALL_AT_K,
                 bad),
        std::invalid_argument);
}

TEST(BenchmarkMatrixTest, RecordAllowsZeroSampleWithEdgeFlag) {
    BenchmarkMatrix m;
    // If edge_flags is non-NONE, zero samples are permitted (measurement was
    // attempted but flagged as insufficient).
    BenchmarkResult r;
    r.value        = 0.0;
    r.sample_count = 0;
    r.edge_flags   = BenchmarkEdgeCase::INSUFFICIENT_METRIC_DATA;

    EXPECT_NO_THROW(
        m.record(BenchmarkScenario::HNSW_ANN_ONLY,
                 BenchmarkDimension::RECALL_AT_K,
                 r));
}

// ============================================================================
// Edge-case flag propagation
// ============================================================================

TEST(BenchmarkMatrixTest, StaleArtifactFlagIsPreserved) {
    BenchmarkMatrix m;
    auto r = makeAnomalous(1.2, BenchmarkEdgeCase::STALE_ARTIFACT);
    m.record(BenchmarkScenario::ANN_TENSOR_SNAPSHOT_REBUILT,
             BenchmarkDimension::REBUILD_LATENCY_MS, r);

    auto got = m.lookup(BenchmarkScenario::ANN_TENSOR_SNAPSHOT_REBUILT,
                        BenchmarkDimension::REBUILD_LATENCY_MS);
    ASSERT_TRUE(got.has_value());
    EXPECT_TRUE(hasEdgeCase(got->edge_flags, BenchmarkEdgeCase::STALE_ARTIFACT));
    EXPECT_FALSE(got->isClean());
}

TEST(BenchmarkMatrixTest, ShardSummaryMismatchFlagIsPreserved) {
    BenchmarkMatrix m;
    auto r = makeAnomalous(300.0,
                           BenchmarkEdgeCase::SHARD_SUMMARY_MISMATCH,
                           /*samples=*/2);
    m.record(BenchmarkScenario::SUMMARY_FIRST_DISTRIBUTED,
             BenchmarkDimension::SHARD_FAN_OUT, r);

    auto got = m.lookup(BenchmarkScenario::SUMMARY_FIRST_DISTRIBUTED,
                        BenchmarkDimension::SHARD_FAN_OUT);
    ASSERT_TRUE(got.has_value());
    EXPECT_TRUE(hasEdgeCase(got->edge_flags, BenchmarkEdgeCase::SHARD_SUMMARY_MISMATCH));
}

TEST(BenchmarkMatrixTest, ResidualPlannerFallbackFlagIsPreserved) {
    BenchmarkMatrix m;
    auto r = makeAnomalous(7.0, BenchmarkEdgeCase::RESIDUAL_PLANNER_FALLBACK);
    m.record(BenchmarkScenario::ANN_TENSOR_GRAPH,
             BenchmarkDimension::QUERY_LATENCY_MS, r);

    auto got = m.lookup(BenchmarkScenario::ANN_TENSOR_GRAPH,
                        BenchmarkDimension::QUERY_LATENCY_MS);
    ASSERT_TRUE(got.has_value());
    EXPECT_TRUE(hasEdgeCase(got->edge_flags, BenchmarkEdgeCase::RESIDUAL_PLANNER_FALLBACK));
}

TEST(BenchmarkMatrixTest, UnmeasuredCombinationFlagMarksNewWorkload) {
    BenchmarkMatrix m;
    auto r = makeAnomalous(0.0, BenchmarkEdgeCase::UNMEASURED_COMBINATION, 0);
    m.record(BenchmarkScenario::LORA_INFERENCE,
             BenchmarkDimension::HALLUCINATION_RATE, r);

    auto got = m.lookup(BenchmarkScenario::LORA_INFERENCE,
                        BenchmarkDimension::HALLUCINATION_RATE);
    ASSERT_TRUE(got.has_value());
    EXPECT_TRUE(hasEdgeCase(got->edge_flags, BenchmarkEdgeCase::UNMEASURED_COMBINATION));
    EXPECT_FALSE(got->hasSufficientData(1));
}

TEST(BenchmarkMatrixTest, CombinedEdgeFlagsAreBitwise) {
    BenchmarkEdgeCase combined =
        BenchmarkEdgeCase::STALE_ARTIFACT | BenchmarkEdgeCase::SHARD_SUMMARY_MISMATCH;
    EXPECT_TRUE(hasEdgeCase(combined, BenchmarkEdgeCase::STALE_ARTIFACT));
    EXPECT_TRUE(hasEdgeCase(combined, BenchmarkEdgeCase::SHARD_SUMMARY_MISMATCH));
    EXPECT_FALSE(hasEdgeCase(combined, BenchmarkEdgeCase::RESIDUAL_PLANNER_FALLBACK));
}

// ============================================================================
// BenchmarkResult::hasSufficientData
// ============================================================================

TEST(BenchmarkResultTest, CleanResultWithEnoughSamplesIsSufficient) {
    auto r = makeClean(0.93, 10);
    EXPECT_TRUE(r.hasSufficientData(3));
    EXPECT_TRUE(r.hasSufficientData(10));
    EXPECT_FALSE(r.hasSufficientData(11));
}

TEST(BenchmarkResultTest, InsufficientDataFlagOverridesSampleCount) {
    BenchmarkResult r;
    r.value        = 0.5;
    r.sample_count = 100; // high count, but flagged
    r.edge_flags   = BenchmarkEdgeCase::INSUFFICIENT_METRIC_DATA;
    EXPECT_FALSE(r.hasSufficientData(1));
}

// ============================================================================
// Scenario and dimension slices
// ============================================================================

TEST(BenchmarkMatrixTest, ScenarioSliceReturnsAllDimensionsForScenario) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(8.5));
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::COMPRESSION_RATIO, makeClean(3.1));
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(2.4));

    auto slice = m.scenarioSlice(BenchmarkScenario::ANN_TENSOR);
    EXPECT_EQ(2u, slice.size());
}

TEST(BenchmarkMatrixTest, ScenarioSliceEmptyForMissingScenario) {
    BenchmarkMatrix m;
    auto slice = m.scenarioSlice(BenchmarkScenario::DISKANN_ANN_ONLY);
    EXPECT_TRUE(slice.empty());
}

TEST(BenchmarkMatrixTest, DimensionSliceReturnsAllScenariosForDimension) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::QPS, makeClean(1500.0));
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY,
             BenchmarkDimension::QPS, makeClean(900.0));
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::QPS, makeClean(700.0));

    auto slice = m.dimensionSlice(BenchmarkDimension::QPS);
    EXPECT_EQ(3u, slice.size());
}

// ============================================================================
// Full-coverage filtering
// ============================================================================

TEST(BenchmarkMatrixTest, ScenariosWithFullCoverageFiltersIncomplete) {
    BenchmarkMatrix m;
    // HNSW: recall + latency  → fully covered
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::RECALL_AT_K,   makeClean(0.95));
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(2.0));

    // DiskANN: only recall → partial
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY,
             BenchmarkDimension::RECALL_AT_K, makeClean(0.92));

    std::vector<BenchmarkDimension> required = {
        BenchmarkDimension::RECALL_AT_K,
        BenchmarkDimension::QUERY_LATENCY_MS,
    };
    auto covered = m.scenariosWithFullCoverage(required);
    ASSERT_EQ(1u, covered.size());
    EXPECT_EQ(BenchmarkScenario::HNSW_ANN_ONLY, covered.front());
}

TEST(BenchmarkMatrixTest, ScenariosWithFullCoverageExcludesInsufficientData) {
    BenchmarkMatrix m;
    // ANN_TENSOR has recall with INSUFFICIENT_METRIC_DATA flag → not sufficient
    BenchmarkResult bad;
    bad.value        = 0.8;
    bad.sample_count = 1;
    bad.edge_flags   = BenchmarkEdgeCase::INSUFFICIENT_METRIC_DATA;
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::RECALL_AT_K, bad);
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(11.0));

    std::vector<BenchmarkDimension> required = {
        BenchmarkDimension::RECALL_AT_K,
        BenchmarkDimension::QUERY_LATENCY_MS,
    };
    auto covered = m.scenariosWithFullCoverage(required);
    EXPECT_TRUE(covered.empty());
}

// ============================================================================
// Cross-scenario comparison
// ============================================================================

TEST(BenchmarkMatrixTest, CompareScenariosReturnsRatio) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(2.0));
    m.record(BenchmarkScenario::ANN_TENSOR_GRAPH,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(6.0));

    // ANN_TENSOR_GRAPH / HNSW latency ratio = 3.0 (HNSW is 3× faster)
    auto ratio = m.compareScenarios(BenchmarkScenario::ANN_TENSOR_GRAPH,
                                    BenchmarkScenario::HNSW_ANN_ONLY,
                                    BenchmarkDimension::QUERY_LATENCY_MS);
    ASSERT_TRUE(ratio.has_value());
    EXPECT_DOUBLE_EQ(3.0, *ratio);
}

TEST(BenchmarkMatrixTest, CompareScenariosReturnsNulloptIfMissing) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::QPS, makeClean(2000.0));

    auto ratio = m.compareScenarios(BenchmarkScenario::HNSW_ANN_ONLY,
                                    BenchmarkScenario::DISKANN_ANN_ONLY,
                                    BenchmarkDimension::QPS);
    EXPECT_FALSE(ratio.has_value());
}

TEST(BenchmarkMatrixTest, CompareScenariosReturnsNulloptOnDivisionByZero) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::QPS, makeClean(1500.0));
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY,
             BenchmarkDimension::QPS, makeClean(0.0));

    auto ratio = m.compareScenarios(BenchmarkScenario::HNSW_ANN_ONLY,
                                    BenchmarkScenario::DISKANN_ANN_ONLY,
                                    BenchmarkDimension::QPS);
    EXPECT_FALSE(ratio.has_value());
}

// ============================================================================
// Best-scenario selection
// ============================================================================

TEST(BenchmarkMatrixTest, BestScenarioHigherIsBetter) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::RECALL_AT_K, makeClean(0.91));
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY,
             BenchmarkDimension::RECALL_AT_K, makeClean(0.93));
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::RECALL_AT_K, makeClean(0.96));

    auto best = m.bestScenario(BenchmarkDimension::RECALL_AT_K, /*higher=*/true);
    ASSERT_TRUE(best.has_value());
    EXPECT_EQ(BenchmarkScenario::ANN_TENSOR, *best);
}

TEST(BenchmarkMatrixTest, BestScenarioLowerIsBetter) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(2.0));
    m.record(BenchmarkScenario::ANN_TENSOR_GRAPH,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(9.0));
    m.record(BenchmarkScenario::SUMMARY_FIRST_DISTRIBUTED,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(15.0));

    auto best = m.bestScenario(BenchmarkDimension::QUERY_LATENCY_MS, /*higher=*/false);
    ASSERT_TRUE(best.has_value());
    EXPECT_EQ(BenchmarkScenario::HNSW_ANN_ONLY, *best);
}

TEST(BenchmarkMatrixTest, BestScenarioIgnoresInsufficientData) {
    BenchmarkMatrix m;
    // Perfect recall but flagged as insufficient
    BenchmarkResult bad;
    bad.value        = 1.0;
    bad.sample_count = 1;
    bad.edge_flags   = BenchmarkEdgeCase::INSUFFICIENT_METRIC_DATA;
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY,
             BenchmarkDimension::RECALL_AT_K, bad);
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::RECALL_AT_K, makeClean(0.94));

    auto best = m.bestScenario(BenchmarkDimension::RECALL_AT_K, true);
    ASSERT_TRUE(best.has_value());
    EXPECT_EQ(BenchmarkScenario::HNSW_ANN_ONLY, *best);
}

TEST(BenchmarkMatrixTest, BestScenarioEmptyDimensionReturnsNullopt) {
    BenchmarkMatrix m;
    EXPECT_FALSE(m.bestScenario(BenchmarkDimension::GPU_SPEEDUP_FACTOR).has_value());
}

// ============================================================================
// Invalidation
// ============================================================================

TEST(BenchmarkMatrixTest, InvalidateScenarioRemovesAllItsEntries) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(10.0));
    m.record(BenchmarkScenario::ANN_TENSOR,
             BenchmarkDimension::COMPRESSION_RATIO, makeClean(4.0));
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::QUERY_LATENCY_MS, makeClean(2.0));

    m.invalidateScenario(BenchmarkScenario::ANN_TENSOR);
    EXPECT_FALSE(m.contains(BenchmarkScenario::ANN_TENSOR,
                             BenchmarkDimension::QUERY_LATENCY_MS));
    EXPECT_FALSE(m.contains(BenchmarkScenario::ANN_TENSOR,
                             BenchmarkDimension::COMPRESSION_RATIO));
    // Unrelated scenario must survive.
    EXPECT_TRUE(m.contains(BenchmarkScenario::HNSW_ANN_ONLY,
                            BenchmarkDimension::QUERY_LATENCY_MS));
    EXPECT_EQ(1u, m.size());
}

TEST(BenchmarkMatrixTest, InvalidateDimensionRemovesAllItsEntries) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::QPS, makeClean(1800.0));
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY,
             BenchmarkDimension::QPS, makeClean(900.0));
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::RECALL_AT_K, makeClean(0.95));

    m.invalidateDimension(BenchmarkDimension::QPS);
    EXPECT_FALSE(m.contains(BenchmarkScenario::HNSW_ANN_ONLY,
                             BenchmarkDimension::QPS));
    EXPECT_FALSE(m.contains(BenchmarkScenario::DISKANN_ANN_ONLY,
                             BenchmarkDimension::QPS));
    EXPECT_TRUE(m.contains(BenchmarkScenario::HNSW_ANN_ONLY,
                            BenchmarkDimension::RECALL_AT_K));
    EXPECT_EQ(1u, m.size());
}

TEST(BenchmarkMatrixTest, ClearEmptiesEntireMatrix) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::ANN_TENSOR_GRAPH,
             BenchmarkDimension::FAITHFULNESS_SCORE, makeClean(0.87));
    m.record(BenchmarkScenario::LORA_INFERENCE,
             BenchmarkDimension::HALLUCINATION_RATE, makeClean(0.04));

    m.clear();
    EXPECT_EQ(0u, m.size());
}

// ============================================================================
// Serialisation via entries()
// ============================================================================

TEST(BenchmarkMatrixTest, EntriesRoundTripsAllCells) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::GPU_ACCELERATED,
             BenchmarkDimension::GPU_SPEEDUP_FACTOR, makeClean(18.5));
    m.record(BenchmarkScenario::CPU_ONLY,
             BenchmarkDimension::GPU_SPEEDUP_FACTOR, makeClean(1.0));

    auto e = m.entries("msmarco-1M", "rtx3090", "v1.0");
    EXPECT_EQ(2u, e.size());
    for (const auto& entry : e) {
        EXPECT_EQ("msmarco-1M", entry.dataset_tag);
        EXPECT_EQ("rtx3090",    entry.hardware_tag);
        EXPECT_EQ("v1.0",       entry.runner_version);
    }
}

// ============================================================================
// scenarioName / dimensionName helpers
// ============================================================================

TEST(BenchmarkMatrixTest, ScenarioNamesAreNonEmpty) {
    for (uint8_t i = 0; i < static_cast<uint8_t>(BenchmarkScenario::_COUNT); ++i) {
        auto name = scenarioName(static_cast<BenchmarkScenario>(i));
        EXPECT_FALSE(name.empty()) << "Scenario " << static_cast<int>(i)
                                   << " has empty name";
        EXPECT_NE("UNKNOWN", name) << "Scenario " << static_cast<int>(i)
                                   << " returned UNKNOWN";
    }
}

TEST(BenchmarkMatrixTest, DimensionNamesAreNonEmpty) {
    for (uint8_t i = 0; i < static_cast<uint8_t>(BenchmarkDimension::_COUNT); ++i) {
        auto name = dimensionName(static_cast<BenchmarkDimension>(i));
        EXPECT_FALSE(name.empty()) << "Dimension " << static_cast<int>(i)
                                   << " has empty name";
        EXPECT_NE("UNKNOWN", name) << "Dimension " << static_cast<int>(i)
                                   << " returned UNKNOWN";
    }
}

// ============================================================================
// Move semantics
// ============================================================================

TEST(BenchmarkMatrixTest, MatrixIsMoveConstructible) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,
             BenchmarkDimension::RECALL_AT_K, makeClean(0.91));

    BenchmarkMatrix m2(std::move(m));
    EXPECT_EQ(1u, m2.size());
    EXPECT_TRUE(m2.contains(BenchmarkScenario::HNSW_ANN_ONLY,
                             BenchmarkDimension::RECALL_AT_K));
}

TEST(BenchmarkMatrixTest, MatrixIsMoveAssignable) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::COMMIT_OVERHEAD,
             BenchmarkDimension::COMMIT_OVERHEAD_MS, makeClean(0.5));

    BenchmarkMatrix m2;
    m2 = std::move(m);
    EXPECT_TRUE(m2.contains(BenchmarkScenario::COMMIT_OVERHEAD,
                             BenchmarkDimension::COMMIT_OVERHEAD_MS));
}
