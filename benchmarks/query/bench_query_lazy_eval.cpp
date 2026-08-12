/**
 * @file bench_query_lazy_eval.cpp
 * @brief Google Benchmark — Query Engine Lazy Evaluation & Predicate Pushdown
 *
 * Implements acceptance criteria for PERF-D7 (Issue: [PERF-D7] Query Engine:
 * 47% Speedup vs ClickHouse via SIMD Column Compression).
 *
 * Benchmarked scenarios:
 *  - BM_LazyEval_FilterOnly         : Single-predicate columnar scan (gt threshold)
 *  - BM_LazyEval_FilterProject      : Filter + late-materialization project
 *  - BM_LazyEval_MultiPredicate     : Two-column AND filter (predicate pushdown)
 *  - BM_LazyEval_FilterAggregate    : Filter then SUM aggregation
 *  - BM_LazyEval_FullPipeline       : Filter → Project → Sort (end-to-end)
 *  - BM_LazyEval_BatchSizes         : Parametric batch size sweep (128/512/1024/4096)
 *
 * Execution path:
 *   JSON rows → ColumnBatch (columnar layout) → SelectionVector filter (lazy)
 *   → optional project/aggregate → JSON materialization
 *
 * The VectorizedExecutionEngine delegates to analytics::ColumnarExecutionEngine
 * which evaluates predicates in tight inner loops amenable to auto-vectorization
 * (SIMD) and defers row materialization until after filtering (late materialization).
 *
 * Performance targets (Release build, commodity x86-64):
 *  - FilterOnly throughput:         ≥ 200 M items/s
 *  - FilterProject throughput:      ≥ 150 M items/s
 *  - MultiPredicate throughput:     ≥ 100 M items/s
 *  - FilterAggregate throughput:    ≥  80 M items/s
 *  - FullPipeline throughput:       ≥  50 M items/s
 *
 * CI acceptance: benchmark must compile and produce items_processed > 0.
 *
 * Run with:
 *   ./bench_query_lazy_eval --benchmark_format=json \
 *                           --benchmark_out=bench_query_lazy_eval.json
 */

#include <benchmark/benchmark.h>
#include <nlohmann/json.hpp>

#include "query/vectorized_execution.h"
#include "analytics/columnar_execution.h"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

using namespace themis::query;
using namespace themisdb::analytics;
using json = nlohmann::json;

// ============================================================================
// Test-data helpers
// ============================================================================

namespace {

/**
 * @brief Generate N JSON rows with fields: id (int), value (int64), score
 *        (double), label (string).  Used by all benchmarks.
 *
 * Distribution:
 *  - id:    sequential [0, N)
 *  - value: uniform [0, 1000)   — threshold 500 selects ~50 %
 *  - score: uniform [0.0, 100.0)
 *  - label: one of {"A","B","C","D"}
 */
static std::vector<json> makeRows(size_t n, uint64_t seed = 42) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int64_t> val_dist(0, 999);
    std::uniform_real_distribution<double>  score_dist(0.0, 100.0);

    static const char* labels[] = {"A", "B", "C", "D"};
    std::uniform_int_distribution<int> label_dist(0, 3);

    std::vector<json> rows;
    rows.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        rows.push_back({
            {"id",    static_cast<int64_t>(i)},
            {"value", val_dist(rng)},
            {"score", score_dist(rng)},
            {"label", labels[label_dist(rng)]}
        });
    }
    return rows;
}

// Number of rows used by the fixed-size benchmarks (fits comfortably in L2).
constexpr size_t kDefaultRows = 65536;

}  // namespace

// ============================================================================
// BM_LazyEval_FilterOnly
// ============================================================================

/**
 * @brief Measure columnar filter throughput with a single int predicate.
 *
 * Exercises the SelectionVector (lazy) code path in FilterOperator.
 * 50 % of rows pass the filter (value > 500), so the selection vector
 * is non-trivial and materialization is deferred.
 */
static void BM_LazyEval_FilterOnly(benchmark::State& state) {
    const size_t n_rows = static_cast<size_t>(state.range(0));
    auto rows = makeRows(n_rows);

    VectorizedExecutionEngine engine;

    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("value", json(int64_t{500}))});

    for (auto _ : state) {
        auto result = engine.execute(rows, plan);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(n_rows));
    state.SetLabel("filter_only:value>500");
}
BENCHMARK(BM_LazyEval_FilterOnly)->Arg(kDefaultRows)->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_LazyEval_FilterProject
// ============================================================================

/**
 * @brief Filter followed by late-materialization project.
 *
 * Only the two surviving columns {"id", "score"} are materialised from
 * the ColumnBatch after the SelectionVector has been applied.  This
 * exercises the zero-copy projection path (shared column references).
 */
static void BM_LazyEval_FilterProject(benchmark::State& state) {
    const size_t n_rows = static_cast<size_t>(state.range(0));
    auto rows = makeRows(n_rows);

    VectorizedExecutionEngine engine;

    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("value", json(int64_t{500}))})
        .addProject({"id", "score"});

    for (auto _ : state) {
        auto result = engine.execute(rows, plan);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(n_rows));
    state.SetLabel("filter+project:value>500,cols=[id,score]");
}
BENCHMARK(BM_LazyEval_FilterProject)->Arg(kDefaultRows)->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_LazyEval_MultiPredicate
// ============================================================================

/**
 * @brief Two-column AND predicate (predicate pushdown).
 *
 * Both predicates are evaluated in columnar mode before materialization.
 * Expected selectivity ~25 % (value > 500 AND score > 50.0).
 */
static void BM_LazyEval_MultiPredicate(benchmark::State& state) {
    const size_t n_rows = static_cast<size_t>(state.range(0));
    auto rows = makeRows(n_rows);

    VectorizedExecutionEngine engine;

    VectorizedQueryPlan plan;
    plan.addFilter({
        VectorizedPredicate::gt("value", json(int64_t{500})),
        VectorizedPredicate::gt("score", json(50.0))
    });

    for (auto _ : state) {
        auto result = engine.execute(rows, plan);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(n_rows));
    state.SetLabel("multi_pred:value>500 AND score>50");
}
BENCHMARK(BM_LazyEval_MultiPredicate)->Arg(kDefaultRows)->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_LazyEval_FilterAggregate
// ============================================================================

/**
 * @brief Filter predicate followed by SUM aggregation on the surviving rows.
 *
 * Demonstrates that the aggregation operator works on the compact
 * post-filter ColumnBatch (SelectionVector already applied).
 */
static void BM_LazyEval_FilterAggregate(benchmark::State& state) {
    const size_t n_rows = static_cast<size_t>(state.range(0));
    auto rows = makeRows(n_rows);

    VectorizedExecutionEngine engine;

    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("value", json(int64_t{500}))})
        .addAggregate({{
            /* result_field */ "total_score",
            /* input_field  */ "score",
            /* function     */ VectorizedAggregation::Function::Sum,
            /* group_by     */ {}
        }});

    for (auto _ : state) {
        auto result = engine.execute(rows, plan);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(n_rows));
    state.SetLabel("filter+sum:value>500,agg=score");
}
BENCHMARK(BM_LazyEval_FilterAggregate)->Arg(kDefaultRows)->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_LazyEval_FullPipeline
// ============================================================================

/**
 * @brief End-to-end pipeline: Filter → Project → Sort.
 *
 * Exercises the full operator chain with late materialization.  Sort is
 * performed on the compact post-filter, post-project column set.
 */
static void BM_LazyEval_FullPipeline(benchmark::State& state) {
    const size_t n_rows = static_cast<size_t>(state.range(0));
    auto rows = makeRows(n_rows);

    VectorizedExecutionEngine engine;

    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("value", json(int64_t{300}))})
        .addProject({"id", "value", "score"})
        .addSort({{"score", /*ascending=*/false}});

    for (auto _ : state) {
        auto result = engine.execute(rows, plan);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(n_rows));
    state.SetLabel("filter+project+sort");
}
BENCHMARK(BM_LazyEval_FullPipeline)->Arg(kDefaultRows)->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_LazyEval_BatchSizes
// ============================================================================

/**
 * @brief Parametric batch-size sweep to identify optimal SIMD batch size.
 *
 * Sweeps over 128 / 512 / 1024 / 4096 tuples-per-batch to measure
 * cache-pressure impact on columnar filter throughput.
 * Default batch (1024) is expected to yield best cache utilization.
 */
static void BM_LazyEval_BatchSizes(benchmark::State& state) {
    const size_t batch_sz = static_cast<size_t>(state.range(0));
    constexpr size_t n_rows = kDefaultRows;
    auto rows = makeRows(n_rows);

    VectorizedExecutionEngine::Config cfg;
    cfg.batch_size  = batch_sz;
    cfg.enable_simd = true;
    VectorizedExecutionEngine engine(cfg);

    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("value", json(int64_t{500}))})
        .addProject({"id", "score"});

    for (auto _ : state) {
        auto result = engine.execute(rows, plan);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(n_rows));
    state.SetLabel("batch=" + std::to_string(batch_sz));
}
BENCHMARK(BM_LazyEval_BatchSizes)
    ->Arg(128)
    ->Arg(512)
    ->Arg(1024)
    ->Arg(4096)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_LazyEval_SelectivitySweep
// ============================================================================

/**
 * @brief Selectivity sweep (0 % / 10 % / 50 % / 90 % / 100 % pass-rate).
 *
 * Varies the filter threshold to measure how late materialization performs
 * under different selectivity conditions.  High selectivity (few rows pass)
 * should be particularly fast due to the compact SelectionVector.
 *
 * threshold = state.range(0) encodes the percentile cut-off [0, 1000).
 */
static void BM_LazyEval_SelectivitySweep(benchmark::State& state) {
    const int64_t threshold = state.range(0);  // value in [0, 999]
    constexpr size_t n_rows = kDefaultRows;
    auto rows = makeRows(n_rows);

    VectorizedExecutionEngine engine;

    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("value", json(threshold))})
        .addProject({"id", "score"});

    for (auto _ : state) {
        auto result = engine.execute(rows, plan);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(n_rows));

    // Human-readable label showing approximate selectivity
    const double pct = 100.0 * (1000 - threshold) / 1000.0;
    state.SetLabel("threshold=" + std::to_string(threshold) +
                   " (~" + std::to_string(static_cast<int>(pct)) + "% pass)");
}
BENCHMARK(BM_LazyEval_SelectivitySweep)
    ->Arg(999)   // ~0  % pass  (very selective)
    ->Arg(899)   // ~10 % pass
    ->Arg(499)   // ~50 % pass
    ->Arg(99)    // ~90 % pass
    ->Arg(0)     // ~100% pass  (no filtering)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// BM_LazyEval_SIMD_vs_Scalar
// ============================================================================

/**
 * @brief Compare SIMD-enabled vs. SIMD-disabled filter throughput.
 *
 * state.range(0) = 1 → SIMD enabled, 0 → SIMD disabled.
 * Shows the effect of the enable_simd flag on ColumnarExecutionEngine.
 */
static void BM_LazyEval_SIMD_vs_Scalar(benchmark::State& state) {
    const bool enable_simd = (state.range(0) != 0);
    constexpr size_t n_rows = kDefaultRows;
    auto rows = makeRows(n_rows);

    VectorizedExecutionEngine::Config cfg;
    cfg.enable_simd = enable_simd;
    VectorizedExecutionEngine engine(cfg);

    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("value", json(int64_t{500}))});

    for (auto _ : state) {
        auto result = engine.execute(rows, plan);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                            static_cast<int64_t>(n_rows));
    state.SetLabel(enable_simd ? "simd=on" : "simd=off");
}
BENCHMARK(BM_LazyEval_SIMD_vs_Scalar)
    ->Arg(0)   // scalar
    ->Arg(1)   // SIMD
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
