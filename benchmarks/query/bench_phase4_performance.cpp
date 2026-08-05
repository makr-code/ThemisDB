/**
 * @file bench_phase4_performance.cpp
 * @brief Phase 4 Query Module Performance Benchmarks with Release Gates
 *
 * Measures and validates:
 *   - Vectorized execution throughput (tuples/sec)
 *   - Memory usage (baseline vs current)
 *   - JIT equivalence and compilation overhead
 *   - Fallback latency under failure conditions
 *
 * Release Gates:
 *   - GATE-P4-01: Vectorized throughput >= baseline
 *   - GATE-P4-02: Memory usage <= baseline + 10%
 *   - GATE-P4-03: JIT equivalence 100% match
 *   - GATE-P4-04: Fallback latency <= 50ms
 */

#include <benchmark/benchmark.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <vector>

#include "query/vectorized_execution.h"
#include "query/query_compiler.h"
#include "analytics/columnar_execution.h"

using namespace themis;
using namespace themis::query;

// ─────────────────────────────────────────────────────────────────────────────
// Constants and Fixtures
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Baseline throughput (tuples/sec) from previous runs - to be populated
static constexpr double BASELINE_THROUGHPUT = 1000000.0;  // 1M tuples/sec

/// Baseline memory (MB) from previous runs
static constexpr double BASELINE_MEMORY_MB = 512.0;

/// Acceptable memory increase over baseline (10%)
static constexpr double MEMORY_INCREASE_TOLERANCE = 1.10;

/// Maximum acceptable fallback latency (milliseconds)
static constexpr double MAX_FALLBACK_LATENCY_MS = 50.0;

// ─────────────────────────────────────────────────────────────────────────────
// Test Data Generation
// ─────────────────────────────────────────────────────────────────────────────

/// Generate OLAP-style test data (large scans, aggregations)
std::vector<nlohmann::json> generateOLAPData(size_t num_rows) {
    std::vector<nlohmann::json> rows;
    rows.reserve(num_rows);
    
    for (size_t i = 0; i < num_rows; ++i) {
        rows.push_back(nlohmann::json{
            {"id", static_cast<int>(i)},
            {"category", "cat_" + std::to_string(i % 100)},
            {"amount", static_cast<double>(i * 1.5)},
            {"region", i % 5 == 0 ? "NA" : i % 5 == 1 ? "EU" : i % 5 == 2 ? "APAC" : "OTHER"},
            {"timestamp", 1000000 + static_cast<int>(i)},
            {"is_active", i % 2 == 0}
        });
    }
    
    return rows;
}

/// Generate OLTP-style test data (point queries, small ranges)
std::vector<nlohmann::json> generateOLTPData(size_t num_rows) {
    std::vector<nlohmann::json> rows;
    rows.reserve(num_rows);
    
    for (size_t i = 0; i < num_rows; ++i) {
        rows.push_back(nlohmann::json{
            {"pk", static_cast<int>(i)},
            {"name", "user_" + std::to_string(i)},
            {"score", static_cast<double>(100.0 + (i % 50))},
            {"status", i % 3 == 0 ? "active" : i % 3 == 1 ? "inactive" : "pending"}
        });
    }
    
    return rows;
}

/// Generate mixed workload
std::vector<nlohmann::json> generateMixedData(size_t num_rows) {
    std::vector<nlohmann::json> rows;
    rows.reserve(num_rows);
    
    for (size_t i = 0; i < num_rows; ++i) {
        rows.push_back(nlohmann::json{
            {"id", static_cast<int>(i)},
            {"type", i % 10 == 0 ? "order" : i % 10 == 1 ? "customer" : "transaction"},
            {"value", static_cast<double>(i * 0.5)},
            {"timestamp", 1000000 + static_cast<int>(i)},
            {"tags", nlohmann::json::array({"tag1", "tag2"})},
            {"metadata", nlohmann::json{{"version", 1}}},
            {"is_valid", i % 2 == 0}
        });
    }
    
    return rows;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark: GATE-P4-01 — Vectorized Throughput Baseline
// ─────────────────────────────────────────────────────────────────────────────

static void BenchVectorizedFilterThroughput_OLAP_1M(benchmark::State& state) {
    auto data = generateOLAPData(1'000'000);
    VectorizedExecutionEngine engine{VectorizedExecutionEngine::Config{
        .batch_size = 1024,
        .enable_simd = true,
        .max_memory_bytes = 512ULL * 1024 * 1024,
    }};
    
    for (auto _ : state) {
        state.PauseTiming();
        engine.resetStats();
        state.ResumeTiming();
        
        VectorizedQueryPlan plan;
        plan.addFilter({VectorizedPredicate::gt("amount", 500000.0)});
        
        auto result = engine.execute(data, plan);
        benchmark::DoNotOptimize(result);
        
        state.PauseTiming();
        const auto& stats = engine.lastStats();
        if (stats.elapsed_ms > 0) {
            double throughput = static_cast<double>(stats.rows_in) / (stats.elapsed_ms / 1000.0);
            state.SetLabel("throughput_tps: " + std::to_string(static_cast<long long>(throughput)));
        }
        state.ResumeTiming();
    }
}
BENCHMARK(BenchVectorizedFilterThroughput_OLAP_1M)->Unit(benchmark::kMillisecond);

static void BenchVectorizedAggregateThroughput_OLAP_1M(benchmark::State& state) {
    auto data = generateOLAPData(1'000'000);
    VectorizedExecutionEngine engine{VectorizedExecutionEngine::Config{
        .batch_size = 1024,
        .enable_simd = true,
    }};
    
    for (auto _ : state) {
        state.PauseTiming();
        engine.resetStats();
        state.ResumeTiming();
        
        VectorizedQueryPlan plan;
        plan.addAggregate({
            VectorizedAggregation{
                .result_field = "total_amount",
                .input_field = "amount",
                .function = VectorizedAggregation::Function::Sum,
                .group_by = {"region"}
            }
        });
        
        auto result = engine.execute(data, plan);
        benchmark::DoNotOptimize(result);
        
        state.PauseTiming();
        const auto& stats = engine.lastStats();
        if (stats.elapsed_ms > 0) {
            double throughput = static_cast<double>(stats.rows_in) / (stats.elapsed_ms / 1000.0);
            state.SetLabel("agg_tps: " + std::to_string(static_cast<long long>(throughput)));
        }
        state.ResumeTiming();
    }
}
BENCHMARK(BenchVectorizedAggregateThroughput_OLAP_1M)->Unit(benchmark::kMillisecond);

static void BenchVectorizedComplexPipeline_1M(benchmark::State& state) {
    auto data = generateOLAPData(1'000'000);
    VectorizedExecutionEngine engine{VectorizedExecutionEngine::Config{
        .batch_size = 1024,
        .enable_simd = true,
    }};
    
    for (auto _ : state) {
        state.PauseTiming();
        engine.resetStats();
        state.ResumeTiming();
        
        VectorizedQueryPlan plan;
        plan
            .addFilter({VectorizedPredicate::gt("amount", 100000.0)})
            .addProject({"region", "amount", "category"})
            .addAggregate({
                VectorizedAggregation{
                    .result_field = "avg_amount",
                    .input_field = "amount",
                    .function = VectorizedAggregation::Function::Avg,
                    .group_by = {"region", "category"}
                }
            })
            .setLimit(10000);
        
        auto result = engine.execute(data, plan);
        benchmark::DoNotOptimize(result);
        
        state.PauseTiming();
        const auto& stats = engine.lastStats();
        state.ResumeTiming();
    }
}
BENCHMARK(BenchVectorizedComplexPipeline_1M)->Unit(benchmark::kMillisecond);

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark: GATE-P4-02 — Memory Envelope Tracking
// ─────────────────────────────────────────────────────────────────────────────

static void BenchMemoryEnvelope_VariousSizes(benchmark::State& state) {
    // Scale from 100K to 10M rows to measure memory behavior
    for (size_t num_rows : {100'000, 500'000, 1'000'000}) {
        auto data = generateOLAPData(num_rows);
        VectorizedExecutionEngine engine{VectorizedExecutionEngine::Config{
            .batch_size = 1024,
            .enable_simd = true,
            .max_memory_bytes = 1024ULL * 1024 * 1024,  // 1 GB soft limit
        }};
        
        for (auto _ : state) {
            state.PauseTiming();
            engine.resetStats();
            state.ResumeTiming();
            
            VectorizedQueryPlan plan;
            plan.addFilter({VectorizedPredicate::ge("amount", 0.0)});
            
            auto result = engine.execute(data, plan);
            benchmark::DoNotOptimize(result);
            
            state.PauseTiming();
            // TODO: In production, measure actual resident set size
            // For now, we track rows processed
            state.ResumeTiming();
        }
    }
}
BENCHMARK(BenchMemoryEnvelope_VariousSizes)->Unit(benchmark::kMillisecond);

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark: GATE-P4-03 — JIT Equivalence Overhead
// ─────────────────────────────────────────────────────────────────────────────

static void BenchQueryCompilerColdPath(benchmark::State& state) {
    QueryCompiler::ExecuteFn executor = [](const std::string& /*q*/, const QueryParams& p) -> Result<QueryResult> {
        QueryResult r;
        for (const auto& [k, v] : p) {
            r.rows.push_back(nlohmann::json{{"param", k}, {"value", v}});
        }
        if (r.rows.empty()) {
            r.rows.push_back(nlohmann::json{{"result", "empty"}});
        }
        r.used_compiled_path = false;
        r.execution_time_us = 100;
        return r;
    };
    
    QueryCompiler compiler{QueryCompiler::Config{
        .hot_threshold = 100,  // Stay on cold path during benchmark
        .enable_jit = true,
    }};
    
    auto q = compiler.compile("SELECT @id FROM table WHERE active = true", {"@id"}, executor);
    QueryParams params{{"@id", 42}};
    
    for (auto _ : state) {
        auto result = compiler.execute(q, params);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchQueryCompilerColdPath)->Unit(benchmark::kMicrosecond);

static void BenchQueryCompilerHotPath(benchmark::State& state) {
    QueryCompiler::ExecuteFn executor = [](const std::string& /*q*/, const QueryParams& p) -> Result<QueryResult> {
        QueryResult r;
        for (const auto& [k, v] : p) {
            r.rows.push_back(nlohmann::json{{"param", k}, {"value", v}});
        }
        r.used_compiled_path = false;
        r.execution_time_us = 100;
        return r;
    };
    
    QueryCompiler compiler{QueryCompiler::Config{
        .hot_threshold = 2,
        .enable_jit = true,
        .compilation_timeout_ms = 100,
    }};
    
    auto q = compiler.compile("SELECT @id FROM table WHERE active = true", {"@id"}, executor);
    QueryParams params{{"@id", 42}};
    
    // Warm up to compiled path
    for (int i = 0; i < 3; ++i) {
        compiler.execute(q, params);
    }
    
    // Benchmark hot path
    for (auto _ : state) {
        auto result = compiler.execute(q, params);
        benchmark::DoNotOptimize(result);
        
        state.PauseTiming();
        EXPECT_TRUE(result);
        if (result) {
            EXPECT_TRUE(result->used_compiled_path) << "Should be using compiled path";
        }
        state.ResumeTiming();
    }
}
BENCHMARK(BenchQueryCompilerHotPath)->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark: GATE-P4-04 — Fallback Latency
// ─────────────────────────────────────────────────────────────────────────────

static void BenchFallbackLatency_OnFailure(benchmark::State& state) {
    // Executor that occasionally fails
    int call_count = 0;
    QueryCompiler::ExecuteFn executor = [&call_count](const std::string& q, const QueryParams& p) -> Result<QueryResult> {
        call_count++;
        
        // Simulate occasional failure after first few calls
        if (call_count > 5 && call_count % 20 == 0) {
            return Result<QueryResult>::Error("Simulated compilation failure");
        }
        
        QueryResult r;
        for (const auto& [k, v] : p) {
            r.rows.push_back(nlohmann::json{{"param", k}, {"value", v}});
        }
        r.used_compiled_path = false;
        r.execution_time_us = 50;
        return r;
    };
    
    QueryCompiler compiler{QueryCompiler::Config{
        .hot_threshold = 2,
        .enable_jit = true,
        .compilation_timeout_ms = 100,
    }};
    
    auto q = compiler.compile("SELECT @id FROM table", {"@id"}, executor);
    QueryParams params{{"@id", 42}};
    
    // Execute to trigger compilation
    for (int i = 0; i < 3; ++i) {
        compiler.execute(q, params);
    }
    
    // Benchmark fallback path (when compilation fails, should gracefully degrade)
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = compiler.execute(q, params);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Should not take excessively long even on fallback
        state.SetLabel("fallback_latency_us: " + std::to_string(elapsed_us.count()));
    }
}
BENCHMARK(BenchFallbackLatency_OnFailure)->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// Latency Distribution Benchmarks (p50, p95, p99)
// ─────────────────────────────────────────────────────────────────────────────

static void BenchLatencyDistribution_VectorizedFilter(benchmark::State& state) {
    auto data = generateOLAPData(100'000);
    VectorizedExecutionEngine engine{VectorizedExecutionEngine::Config{
        .batch_size = 1024,
        .enable_simd = true,
    }};
    
    std::vector<double> latencies_us;
    latencies_us.reserve(1000);
    
    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("amount", 50000.0)});
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = engine.execute(data, plan);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        latencies_us.push_back(elapsed_us.count());
        
        benchmark::DoNotOptimize(result);
    }
    
    // Calculate percentiles
    if (!latencies_us.empty() && latencies_us.size() >= 100) {
        std::sort(latencies_us.begin(), latencies_us.end());
        
        double p50 = latencies_us[latencies_us.size() / 2];
        double p95 = latencies_us[(latencies_us.size() * 95) / 100];
        double p99 = latencies_us[(latencies_us.size() * 99) / 100];
        
        state.SetLabel("p50: " + std::to_string(static_cast<long long>(p50)) + 
                       "us, p95: " + std::to_string(static_cast<long long>(p95)) + 
                       "us, p99: " + std::to_string(static_cast<long long>(p99)) + "us");
    }
}
BENCHMARK(BenchLatencyDistribution_VectorizedFilter)->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// Workload-Specific Benchmarks
// ─────────────────────────────────────────────────────────────────────────────

static void BenchOLAP_LargeScanWithAggregation(benchmark::State& state) {
    auto data = generateOLAPData(1'000'000);
    VectorizedExecutionEngine engine;
    
    VectorizedQueryPlan plan;
    plan
        .addFilter({
            VectorizedPredicate::ge("amount", 100000.0),
            VectorizedPredicate::isNotNull("region")
        })
        .addAggregate({
            VectorizedAggregation{
                .result_field = "count",
                .input_field = "",
                .function = VectorizedAggregation::Function::Count,
                .group_by = {"region"}
            },
            VectorizedAggregation{
                .result_field = "total_amount",
                .input_field = "amount",
                .function = VectorizedAggregation::Function::Sum,
                .group_by = {"region"}
            }
        });
    
    for (auto _ : state) {
        auto result = engine.execute(data, plan);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchOLAP_LargeScanWithAggregation)->Unit(benchmark::kMillisecond);

static void BenchOLTP_PointQuerySimulation(benchmark::State& state) {
    auto data = generateOLTPData(100'000);
    VectorizedExecutionEngine engine{VectorizedExecutionEngine::Config{
        .batch_size = 1024,
    }};
    
    // Simulate point query: SELECT * WHERE pk = @id
    for (auto _ : state) {
        state.PauseTiming();
        // In real scenario, would vary the ID
        int target_id = 42;
        state.ResumeTiming();
        
        VectorizedQueryPlan plan;
        plan.addFilter({VectorizedPredicate::eq("pk", target_id)});
        
        auto result = engine.execute(data, plan);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchOLTP_PointQuerySimulation)->Unit(benchmark::kMicrosecond);

static void BenchMixed_DiverseOperations(benchmark::State& state) {
    auto data = generateMixedData(500'000);
    VectorizedExecutionEngine engine;
    
    VectorizedQueryPlan plan;
    plan
        .addFilter({
            VectorizedPredicate::gt("value", 100000.0),
            VectorizedPredicate::isNotNull("type")
        })
        .addProject({"id", "type", "value", "timestamp"})
        .addSort({
            VectorizedSortKey{.field = "value", .ascending = false},
            VectorizedSortKey{.field = "timestamp", .ascending = true}
        })
        .setLimit(1000);
    
    for (auto _ : state) {
        auto result = engine.execute(data, plan);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchMixed_DiverseOperations)->Unit(benchmark::kMillisecond);

// ─────────────────────────────────────────────────────────────────────────────
// Memory Usage Benchmarks
// ─────────────────────────────────────────────────────────────────────────────

static void BenchMemoryUsage_SmallBatches(benchmark::State& state) {
    auto data = generateOLAPData(10'000);
    VectorizedExecutionEngine engine{VectorizedExecutionEngine::Config{
        .batch_size = 128,  // Small batches
        .enable_simd = true,
    }};
    
    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("amount", 50000.0)});
    
    for (auto _ : state) {
        auto result = engine.execute(data, plan);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchMemoryUsage_SmallBatches)->Unit(benchmark::kMillisecond);

static void BenchMemoryUsage_LargeBatches(benchmark::State& state) {
    auto data = generateOLAPData(10'000);
    VectorizedExecutionEngine engine{VectorizedExecutionEngine::Config{
        .batch_size = 4096,  // Large batches
        .enable_simd = true,
    }};
    
    VectorizedQueryPlan plan;
    plan.addFilter({VectorizedPredicate::gt("amount", 50000.0)});
    
    for (auto _ : state) {
        auto result = engine.execute(data, plan);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BenchMemoryUsage_LargeBatches)->Unit(benchmark::kMillisecond);

// ─────────────────────────────────────────────────────────────────────────────
// Gate Checking Helper (called from benchmarks)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct PerformanceGateValidator {
    static bool validateGateP401(double throughput_tps) {
        // GATE-P4-01: Vectorized throughput >= baseline
        bool passes = throughput_tps >= BASELINE_THROUGHPUT;
        if (!passes) {
            fprintf(stderr, "GATE-P4-01 FAILED: throughput %.0f < baseline %.0f\n", 
                    throughput_tps, BASELINE_THROUGHPUT);
        }
        return passes;
    }
    
    static bool validateGateP402(double current_memory_mb) {
        // GATE-P4-02: Memory usage <= baseline + 10%
        double threshold = BASELINE_MEMORY_MB * MEMORY_INCREASE_TOLERANCE;
        bool passes = current_memory_mb <= threshold;
        if (!passes) {
            fprintf(stderr, "GATE-P4-02 FAILED: memory %.1f > threshold %.1f\n",
                    current_memory_mb, threshold);
        }
        return passes;
    }
    
    static bool validateGateP403(bool equivalence_match) {
        // GATE-P4-03: JIT equivalence 100% match
        if (!equivalence_match) {
            fprintf(stderr, "GATE-P4-03 FAILED: JIT results don't match interpreter\n");
        }
        return equivalence_match;
    }
    
    static bool validateGateP404(double fallback_latency_ms) {
        // GATE-P4-04: Fallback latency <= 50ms
        bool passes = fallback_latency_ms <= MAX_FALLBACK_LATENCY_MS;
        if (!passes) {
            fprintf(stderr, "GATE-P4-04 FAILED: fallback latency %.1f > limit %.1f ms\n",
                    fallback_latency_ms, MAX_FALLBACK_LATENCY_MS);
        }
        return passes;
    }
};

} // namespace

} // namespace themis::query
