/**
 * @file test_evaluation_benchmark_matrix_focused.cpp
 * @brief Group BM — Unit tests for BenchmarkMatrix record, query, and comparison.
 */

#include <gtest/gtest.h>
#include "benchmark_matrix.h"
#include <stdexcept>

using namespace themis::evaluation;

// ── helpers ───────────────────────────────────────────────────────────────────

static BenchmarkResult make_result(double value, uint32_t samples = 1) {
    BenchmarkResult r;
    r.value        = value;
    r.sample_count = samples;
    r.edge_flags   = BenchmarkEdgeCase::NONE;
    return r;
}

// ── Group BM — Benchmark Matrix ───────────────────────────────────────────────

// BM1: Record and lookup return the same result
TEST(EvaluationBenchmarkMatrixFocusedTests, BM1_RecordAndLookup_Roundtrip) {
    BenchmarkMatrix m;
    auto r = make_result(10.0, 0.95);
    m.record(BenchmarkScenario::HNSW_ANN_ONLY, BenchmarkDimension::QUERY_LATENCY_MS, r);
    auto found = m.lookup(BenchmarkScenario::HNSW_ANN_ONLY, BenchmarkDimension::QUERY_LATENCY_MS);
    ASSERT_TRUE(found.has_value());
    EXPECT_DOUBLE_EQ(found->value, 10.0);
    EXPECT_DOUBLE_EQ(found->recall_at_10, 0.95);
}

// BM2: lookup returns nullopt for a missing entry
TEST(EvaluationBenchmarkMatrixFocusedTests, BM2_Lookup_Missing_ReturnsNullopt) {
    BenchmarkMatrix m;
    auto found = m.lookup(BenchmarkScenario::DISKANN_ANN_ONLY, BenchmarkDimension::QUERY_LATENCY_MS);
    EXPECT_FALSE(found.has_value());
}

// BM3: Record with sample_count == 0 and no edge flags is rejected
TEST(EvaluationBenchmarkMatrixFocusedTests, BM3_RecordZeroSamples_ThrowsInvalidArgument) {
    BenchmarkMatrix m;
    BenchmarkResult r;
    r.sample_count = 0;
    r.edge_flags   = BenchmarkEdgeCase::NONE;
    EXPECT_THROW(
        m.record(BenchmarkScenario::HNSW_ANN_ONLY, BenchmarkDimension::QUERY_LATENCY_MS, r),
        std::invalid_argument
    );
}

// BM4: Overwriting an entry with record() replaces the previous value
TEST(EvaluationBenchmarkMatrixFocusedTests, BM4_Overwrite_ReplacesOldResult) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::ANN_TENSOR, BenchmarkDimension::RECALL_AT_K,
             make_result(5.0, 0.80));
    m.record(BenchmarkScenario::ANN_TENSOR, BenchmarkDimension::RECALL_AT_K,
             make_result(6.0, 0.90));
    auto found = m.lookup(BenchmarkScenario::ANN_TENSOR, BenchmarkDimension::RECALL_AT_K);
    ASSERT_TRUE(found.has_value());
    EXPECT_DOUBLE_EQ(found->value, 0.90);
}

// BM5: invalidateScenario removes all entries for that scenario
TEST(EvaluationBenchmarkMatrixFocusedTests, BM5_InvalidateScenario_RemovesEntries) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY, BenchmarkDimension::QUERY_LATENCY_MS,
             make_result(10.0));
    m.record(BenchmarkScenario::HNSW_ANN_ONLY, BenchmarkDimension::RECALL_AT_K,
             make_result(10.0));
    // Record a different scenario that must survive invalidation
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY, BenchmarkDimension::QUERY_LATENCY_MS,
             make_result(8.0));

    m.invalidateScenario(BenchmarkScenario::HNSW_ANN_ONLY);

    EXPECT_FALSE(m.lookup(BenchmarkScenario::HNSW_ANN_ONLY,
                          BenchmarkDimension::QUERY_LATENCY_MS).has_value());
    EXPECT_TRUE(m.lookup(BenchmarkScenario::DISKANN_ANN_ONLY,
                         BenchmarkDimension::QUERY_LATENCY_MS).has_value());
}

// BM6: invalidateDimension removes all entries for that dimension
TEST(EvaluationBenchmarkMatrixFocusedTests, BM6_InvalidateDimension_RemovesEntries) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY, BenchmarkDimension::QUERY_LATENCY_MS,
             make_result(10.0));
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY, BenchmarkDimension::QUERY_LATENCY_MS,
             make_result(8.0));
    m.record(BenchmarkScenario::HNSW_ANN_ONLY, BenchmarkDimension::RECALL_AT_K,
             make_result(10.0));

    m.invalidateDimension(BenchmarkDimension::QUERY_LATENCY_MS);

    EXPECT_FALSE(m.lookup(BenchmarkScenario::HNSW_ANN_ONLY,
                          BenchmarkDimension::QUERY_LATENCY_MS).has_value());
    EXPECT_FALSE(m.lookup(BenchmarkScenario::DISKANN_ANN_ONLY,
                          BenchmarkDimension::QUERY_LATENCY_MS).has_value());
    EXPECT_TRUE(m.lookup(BenchmarkScenario::HNSW_ANN_ONLY,
                         BenchmarkDimension::RECALL_AT_K).has_value());
}

// BM7: Multiple distinct scenarios and dimensions co-exist
TEST(EvaluationBenchmarkMatrixFocusedTests, BM7_MultipleEntries_Coexist) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,   BenchmarkDimension::QUERY_LATENCY_MS,  make_result(10.0));
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY, BenchmarkDimension::QUERY_LATENCY_MS,  make_result(8.0,  0.85));
    m.record(BenchmarkScenario::ANN_TENSOR,       BenchmarkDimension::RECALL_AT_K, make_result(12.0));

    EXPECT_TRUE(m.lookup(BenchmarkScenario::HNSW_ANN_ONLY,
                         BenchmarkDimension::QUERY_LATENCY_MS).has_value());
    EXPECT_TRUE(m.lookup(BenchmarkScenario::DISKANN_ANN_ONLY,
                         BenchmarkDimension::QUERY_LATENCY_MS).has_value());
    EXPECT_TRUE(m.lookup(BenchmarkScenario::ANN_TENSOR,
                         BenchmarkDimension::RECALL_AT_K).has_value());
}

// BM8: size() reflects number of recorded entries
TEST(EvaluationBenchmarkMatrixFocusedTests, BM8_Size_ReflectsRecordedEntries) {
    BenchmarkMatrix m;
    EXPECT_EQ(m.size(), 0u);
    m.record(BenchmarkScenario::HNSW_ANN_ONLY,   BenchmarkDimension::QUERY_LATENCY_MS,  make_result(10.0, 5));
    m.record(BenchmarkScenario::DISKANN_ANN_ONLY, BenchmarkDimension::RECALL_AT_K, make_result(8.0, 3));
    EXPECT_EQ(m.size(), 2u);
}

// BM9: clear() empties the matrix
TEST(EvaluationBenchmarkMatrixFocusedTests, BM9_Clear_EmptiesMatrix) {
    BenchmarkMatrix m;
    m.record(BenchmarkScenario::HNSW_ANN_ONLY, BenchmarkDimension::QUERY_LATENCY_MS,
             make_result(10.0, 5));
    EXPECT_GT(m.size(), 0u);
    m.clear();
    EXPECT_EQ(m.size(), 0u);
    EXPECT_FALSE(m.lookup(BenchmarkScenario::HNSW_ANN_ONLY,
                          BenchmarkDimension::QUERY_LATENCY_MS).has_value());
}
