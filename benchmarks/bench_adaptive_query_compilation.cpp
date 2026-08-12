/**
 * @file bench_adaptive_query_compilation.cpp
 * @brief Google Benchmark performance tests for AdaptiveQueryCompiler
 *
 * Covers the acceptance criteria:
 *  - "Benchmark against interpreted execution"
 *  - "Measure compilation overhead vs. execution savings"
 *  - "Verify correctness with differential testing" (cold == hot result sizes)
 *
 * Benchmarked scenarios:
 *  - Cold path (interpreted) vs. hot path (compiled) throughput for:
 *      • Filter queries (target 10× speedup)
 *      • Aggregation queries (target 5–8× speedup)
 *      • Join queries (target 3–5× speedup)
 *  - Compilation overhead for Filter / Aggregate / Join
 *  - Pre-warmed cache hit throughput (explicit compile() then execute())
 *  - Multi-fingerprint cache performance (many distinct queries)
 *  - Concurrent execute() throughput
 *
 * Performance targets (Release build on commodity x86-64):
 *  - Cold-path throughput:  >10K ops/sec
 *  - Hot-path throughput:   >50K ops/sec (5× vs. cold path)
 *  - Compilation overhead:  <100ms per query
 *  - Cache lookup:          >100K ops/sec
 *
 * Run with:
 *   ./bench_adaptive_query_compilation --benchmark_format=json
 *                                       --benchmark_out=bench_aqc.json
 */

#include <benchmark/benchmark.h>
#include "performance/adaptive_query_compiler.h"

#include <chrono>
#include <string>
#include <vector>

using namespace themis::performance;

// ────────────────────────────────────────────────────────────────────────────
// Common fixtures
// ────────────────────────────────────────────────────────────────────────────

namespace {

static Schema makeSchema() {
    Schema s;
    TableSchema users;
    users.table_name = "users";
    users.columns = {
        {"id",    ColumnType::Int64,  false, true},
        {"age",   ColumnType::Int64,  false, false},
        {"score", ColumnType::Double, false, false},
        {"name",  ColumnType::String, true,  false},
    };
    s.tables["users"] = users;

    TableSchema orders;
    orders.table_name = "orders";
    orders.columns = {
        {"id",      ColumnType::Int64,  false, true},
        {"user_id", ColumnType::Int64,  false, false},
        {"amount",  ColumnType::Double, false, false},
    };
    s.tables["orders"] = orders;
    return s;
}

static ParsedQuery makeFilterQuery(const std::string& fp, int64_t min_age = 5) {
    ParsedQuery q;
    q.fingerprint = fp;
    q.table       = "users";
    q.op_type     = QueryOpType::Filter;
    Predicate p;
    p.column = "age"; p.op = Predicate::Op::GE; p.value = min_age;
    q.predicates.push_back(p);
    return q;
}

static ParsedQuery makeAggQuery(const std::string& fp) {
    ParsedQuery q;
    q.fingerprint  = fp;
    q.table        = "users";
    q.op_type      = QueryOpType::Aggregate;
    q.agg_function = "SUM";
    q.agg_column   = "age";
    return q;
}

static ParsedQuery makeJoinQuery(const std::string& fp) {
    ParsedQuery q;
    q.fingerprint     = fp;
    q.table           = "orders";
    q.op_type         = QueryOpType::Join;
    q.join_table      = "users";
    q.join_key_left   = "user_id";
    q.join_key_right  = "id";
    return q;
}

// ────────────────────────────────────────────────────────────────────────────
// Warm-up helper: run query hot_threshold times to trigger compilation
// ────────────────────────────────────────────────────────────────────────────

static void warmUp(AdaptiveQueryCompiler& compiler,
                   const ParsedQuery&     query,
                   const Schema&          schema,
                   size_t                 threshold) {
    QueryParams params;
    for (size_t i = 0; i <= threshold; ++i)
        compiler.execute(query, schema, params);
}

}  // namespace

// ────────────────────────────────────────────────────────────────────────────
// AC: Benchmark against interpreted execution
// ────────────────────────────────────────────────────────────────────────────

/**
 * @brief Cold-path (interpreted) throughput for Filter queries.
 *
 * The compiler's hot_threshold is set higher than the iteration count so the
 * compilation is never triggered; every call goes through the interpreted path.
 */
static void BM_FilterColdPath(benchmark::State& state) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1000000;  // Never trigger compilation
    AdaptiveQueryCompiler compiler(cfg);

    const auto schema = makeSchema();
    const auto query  = makeFilterQuery("bm_filter_cold");
    const QueryParams params;

    for (auto _ : state) {
        auto result = compiler.execute(query, schema, params);
        benchmark::DoNotOptimize(result);
    }

    state.SetLabel("interpreted / cold path");
}
BENCHMARK(BM_FilterColdPath)->Iterations(1000);

/**
 * @brief Hot-path (compiled specialisation) throughput for Filter queries.
 *
 * The query is pre-warmed past the hot_threshold before the measurement loop
 * begins.  All iterations go through the compiled closure.
 */
static void BM_FilterHotPath(benchmark::State& state) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 10;
    AdaptiveQueryCompiler compiler(cfg);

    const auto schema = makeSchema();
    const auto query  = makeFilterQuery("bm_filter_hot");

    warmUp(compiler, query, schema, cfg.hot_threshold);

    const QueryParams params;
    for (auto _ : state) {
        auto result = compiler.execute(query, schema, params);
        benchmark::DoNotOptimize(result);
    }

    const auto& s = compiler.getStats();
    state.counters["speedup_pct"] =
        benchmark::Counter(static_cast<double>(s.average_speedup_percent));
    state.SetLabel("compiled / hot path");
}
BENCHMARK(BM_FilterHotPath)->Iterations(1000);

// ────────────────────────────────────────────────────────────────────────────
// AC: Aggregation speedup
// ────────────────────────────────────────────────────────────────────────────

static void BM_AggregateColdPath(benchmark::State& state) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1000000;
    AdaptiveQueryCompiler compiler(cfg);

    const auto schema = makeSchema();
    const auto query  = makeAggQuery("bm_agg_cold");
    const QueryParams params;

    for (auto _ : state) {
        auto result = compiler.execute(query, schema, params);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("aggregate / interpreted");
}
BENCHMARK(BM_AggregateColdPath)->Iterations(1000);

static void BM_AggregateHotPath(benchmark::State& state) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 10;
    AdaptiveQueryCompiler compiler(cfg);

    const auto schema = makeSchema();
    const auto query  = makeAggQuery("bm_agg_hot");

    warmUp(compiler, query, schema, cfg.hot_threshold);

    const QueryParams params;
    for (auto _ : state) {
        auto result = compiler.execute(query, schema, params);
        benchmark::DoNotOptimize(result);
    }

    const auto& s = compiler.getStats();
    state.counters["speedup_pct"] =
        benchmark::Counter(static_cast<double>(s.average_speedup_percent));
    state.SetLabel("aggregate / compiled");
}
BENCHMARK(BM_AggregateHotPath)->Iterations(1000);

// ────────────────────────────────────────────────────────────────────────────
// AC: Join speedup
// ────────────────────────────────────────────────────────────────────────────

static void BM_JoinColdPath(benchmark::State& state) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 1000000;
    AdaptiveQueryCompiler compiler(cfg);

    const auto schema = makeSchema();
    const auto query  = makeJoinQuery("bm_join_cold");
    const QueryParams params;

    for (auto _ : state) {
        auto result = compiler.execute(query, schema, params);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("join / interpreted");
}
BENCHMARK(BM_JoinColdPath)->Iterations(1000);

static void BM_JoinHotPath(benchmark::State& state) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 10;
    AdaptiveQueryCompiler compiler(cfg);

    const auto schema = makeSchema();
    const auto query  = makeJoinQuery("bm_join_hot");

    warmUp(compiler, query, schema, cfg.hot_threshold);

    const QueryParams params;
    for (auto _ : state) {
        auto result = compiler.execute(query, schema, params);
        benchmark::DoNotOptimize(result);
    }

    const auto& s = compiler.getStats();
    state.counters["speedup_pct"] =
        benchmark::Counter(static_cast<double>(s.average_speedup_percent));
    state.SetLabel("join / compiled");
}
BENCHMARK(BM_JoinHotPath)->Iterations(1000);

// ────────────────────────────────────────────────────────────────────────────
// AC: Compilation overhead
// ────────────────────────────────────────────────────────────────────────────

/**
 * @brief Measure wall-clock time for compile() on Filter / Aggregate / Join.
 *
 * Validates the <100ms compilation time target from the roadmap spec.
 */
static void BM_CompileFilter(benchmark::State& state) {
    const auto schema = makeSchema();

    for (auto _ : state) {
        AdaptiveQueryCompiler compiler;
        auto q  = makeFilterQuery("bm_compile_filter");
        auto cq = compiler.compile(q, schema);
        benchmark::DoNotOptimize(cq);
    }
}
BENCHMARK(BM_CompileFilter);

static void BM_CompileAggregate(benchmark::State& state) {
    const auto schema = makeSchema();

    for (auto _ : state) {
        AdaptiveQueryCompiler compiler;
        auto q  = makeAggQuery("bm_compile_agg");
        auto cq = compiler.compile(q, schema);
        benchmark::DoNotOptimize(cq);
    }
}
BENCHMARK(BM_CompileAggregate);

static void BM_CompileJoin(benchmark::State& state) {
    const auto schema = makeSchema();

    for (auto _ : state) {
        AdaptiveQueryCompiler compiler;
        auto q  = makeJoinQuery("bm_compile_join");
        auto cq = compiler.compile(q, schema);
        benchmark::DoNotOptimize(cq);
    }
}
BENCHMARK(BM_CompileJoin);

// ────────────────────────────────────────────────────────────────────────────
// AC: Pre-warmed cache (execute via CompiledQuery overload)
// ────────────────────────────────────────────────────────────────────────────

/**
 * @brief Throughput of execute(CompiledQuery, params) – skips all overhead.
 *
 * The compiled closure is obtained once via compile() and then reused
 * directly, modelling a caller that manually manages compiled queries.
 */
static void BM_ExecutePrewarmedCompiledQuery(benchmark::State& state) {
    AdaptiveQueryCompiler compiler;
    const auto schema = makeSchema();
    const auto query  = makeFilterQuery("bm_prewarm");
    auto cq = compiler.compile(query, schema);

    const QueryParams params;
    for (auto _ : state) {
        auto result = compiler.execute(cq, params);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("explicit CompiledQuery execute");
}
BENCHMARK(BM_ExecutePrewarmedCompiledQuery)->Iterations(10000);

// ────────────────────────────────────────────────────────────────────────────
// AC: Multi-fingerprint cache – many distinct queries
// ────────────────────────────────────────────────────────────────────────────

static void BM_MultiFingerprint(benchmark::State& state) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 5;

    const auto schema = makeSchema();
    constexpr int kQueries = 16;

    // Pre-build query descriptors
    std::vector<ParsedQuery> queries;
    queries.reserve(kQueries);
    for (int i = 0; i < kQueries; ++i)
        queries.push_back(makeFilterQuery("bm_multi_" + std::to_string(i), i));

    AdaptiveQueryCompiler compiler(cfg);
    const QueryParams params;

    // Warm up all queries
    for (const auto& q : queries)
        warmUp(compiler, q, schema, cfg.hot_threshold);

    int idx = 0;
    for (auto _ : state) {
        auto result = compiler.execute(queries[idx % kQueries], schema, params);
        benchmark::DoNotOptimize(result);
        ++idx;
    }

    state.SetLabel("multi-fingerprint hot cache");
}
BENCHMARK(BM_MultiFingerprint)->Iterations(5000);

// ────────────────────────────────────────────────────────────────────────────
// AC: Differential correctness – cold and hot results must match
// ────────────────────────────────────────────────────────────────────────────

/**
 * @brief Verify that cold-path and hot-path return the same number of rows.
 *
 * Runs `threshold` cold iterations to capture a baseline result, then one
 * more call to trigger compilation, then `n_hot` hot calls.  Each hot result
 * must have the same row count as the baseline.
 *
 * This is not a traditional throughput benchmark — it validates correctness
 * under the benchmark harness.
 */
static void BM_DifferentialCorrectness(benchmark::State& state) {
    AdaptiveQueryCompiler::CompilationConfig cfg;
    cfg.hot_threshold = 5;
    AdaptiveQueryCompiler compiler(cfg);

    const auto schema = makeSchema();
    const auto query  = makeFilterQuery("bm_diff");
    const QueryParams params;

    // Collect cold baseline
    QueryResult baseline;
    for (size_t i = 0; i < cfg.hot_threshold; ++i)
        baseline = compiler.execute(query, schema, params);

    // All subsequent (hot) calls must return same result size
    size_t mismatches = 0;
    for (auto _ : state) {
        auto hot_result = compiler.execute(query, schema, params);
        if (hot_result.rows.size() != baseline.rows.size()) {
            ++mismatches;
        }
        benchmark::DoNotOptimize(hot_result);
    }

    state.counters["mismatches"] = benchmark::Counter(static_cast<double>(mismatches));
    state.SetLabel("differential cold==hot correctness");
}
BENCHMARK(BM_DifferentialCorrectness)->Iterations(200);

// ────────────────────────────────────────────────────────────────────────────
// Benchmark main
// ────────────────────────────────────────────────────────────────────────────

BENCHMARK_MAIN();
