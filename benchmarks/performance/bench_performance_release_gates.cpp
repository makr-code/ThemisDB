/**
 * @file bench_performance_release_gates.cpp
 * @brief Release-gate benchmarks for the performance module.
 * @see include/performance/performance_api_contract.h
 * @see GATE-PFM-01 .. GATE-PFM-06
 */

#include <benchmark/benchmark.h>
#include "performance/performance_api_contract.h"

#include <random>
#include <vector>

using namespace themis::performance;

static constexpr uint64_t kCanonicalSeed = 42;

// ---------------------------------------------------------------------------
// GATE-PFM-01: CacheStats computation latency
// ---------------------------------------------------------------------------
static void BM_CacheStats_Compute(benchmark::State& state) {
    std::mt19937_64 rng(kCanonicalSeed);
    for (auto _ : state) {
        CacheStats cs;
        cs.hits   = rng() % 10000;
        cs.misses = rng() % 10000;
        cs.evictions = rng() % 1000;
        if (cs.hits + cs.misses > 0) {
            cs.hitRatePercent =
                100.0 * static_cast<double>(cs.hits) /
                static_cast<double>(cs.hits + cs.misses);
        }
        benchmark::DoNotOptimize(cs.hitRatePercent);
    }
}
BENCHMARK(BM_CacheStats_Compute)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// GATE-PFM-02: PoolAcquireResult construction throughput
// ---------------------------------------------------------------------------
static void BM_PoolAcquireResult_Construct(benchmark::State& state) {
    for (auto _ : state) {
        PoolAcquireResult r;
        r.acquired = true;
        benchmark::DoNotOptimize(r.acquired);
    }
}
BENCHMARK(BM_PoolAcquireResult_Construct)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// GATE-PFM-03: Error-code cast throughput
// ---------------------------------------------------------------------------
static void BM_PerfError_Cast(benchmark::State& state) {
    static const PerfError kErrors[] = {
        PerfError::kCompileTimeout,
        PerfError::kCacheEvictionFull,
        PerfError::kPoolExhausted,
        PerfError::kCostModelInvalid,
        PerfError::kNoHealthyNode,
        PerfError::kPlanStale,
        PerfError::kStatsUnavailable,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 7]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_PerfError_Cast)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// GATE-PFM-04: CostEstimate arithmetic throughput
// ---------------------------------------------------------------------------
static void BM_CostEstimate_Arithmetic(benchmark::State& state) {
    std::mt19937_64 rng(kCanonicalSeed + 1);
    for (auto _ : state) {
        CostEstimate a = static_cast<double>(rng() % 10000);
        CostEstimate b = static_cast<double>(rng() % 1000 + 1);
        CostEstimate result = a * b + a / b;
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_CostEstimate_Arithmetic)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// GATE-PFM-05: Batch CacheStats vector allocation
// ---------------------------------------------------------------------------
static void BM_CacheStats_BatchAlloc(benchmark::State& state) {
    const int N = static_cast<int>(state.range(0));
    for (auto _ : state) {
        std::vector<CacheStats> v(static_cast<size_t>(N));
        for (auto& cs : v) {
            cs.hits = 100; cs.misses = 50;
            cs.hitRatePercent = 100.0 * 100 / 150;
        }
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_CacheStats_BatchAlloc)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

// ---------------------------------------------------------------------------
// GATE-PFM-06: PoolAcquireResult batch construction
// ---------------------------------------------------------------------------
static void BM_PoolAcquireResult_Batch(benchmark::State& state) {
    const int N = static_cast<int>(state.range(0));
    for (auto _ : state) {
        std::vector<PoolAcquireResult> v(static_cast<size_t>(N));
        for (auto& r : v) {
          r.acquired = true;
        }
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_PoolAcquireResult_Batch)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
