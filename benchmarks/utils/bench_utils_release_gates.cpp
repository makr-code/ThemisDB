/*
 * ThemisDB | File: bench_utils_release_gates.cpp | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gate table:
 * | ID          | Metric                        | Gate         |
 * |-------------|-------------------------------|--------------|
 * | GATE-UTL-01 | BloomFilterConfig alloc rate  | ≥ 5M ops/s   |
 * | GATE-UTL-02 | RetryPolicy copy rate         | ≥ 5M ops/s   |
 * | GATE-UTL-03 | Error-code cast throughput    | ≥ 50M ops/s  |
 * | GATE-UTL-04 | RetryPolicy batch alloc       | ≥ 1M ops/s   |
 * | GATE-UTL-05 | BloomFilterConfig batch alloc | ≥ 1M ops/s   |
 * | GATE-UTL-06 | UtilsError switch dispatch    | ≥ 50M ops/s  |
 */

/**
 * @file bench_utils_release_gates.cpp
 * @brief Release-gate benchmarks for the utils module.
 * @see include/utils/utils_api_contract.h
 */

#include <benchmark/benchmark.h>
#include "utils/utils_api_contract.h"

#include <chrono>
#include <random>
#include <vector>

using namespace themis::utils;

static constexpr uint64_t kCanonicalSeed = 42;

static void BM_BloomFilterConfig_Alloc(benchmark::State& state) {
    for (auto _ : state) {
        BloomFilterConfig cfg;
        cfg.expectedItems = 1'000'000;
        cfg.targetFalsePositiveRate = 0.01;
        benchmark::DoNotOptimize(cfg.expectedItems);
    }
}
BENCHMARK(BM_BloomFilterConfig_Alloc)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_RetryPolicy_Copy(benchmark::State& state) {
    RetryPolicy orig;
    orig.maxAttempts = 5;
    orig.backoffMultiplier = 2.0;
    for (auto _ : state) {
        RetryPolicy copy = orig;
        benchmark::DoNotOptimize(copy.maxAttempts);
    }
}
BENCHMARK(BM_RetryPolicy_Copy)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_UtilsError_Cast(benchmark::State& state) {
    static const UtilsError kErrors[] = {
        UtilsError::kAuditOverflow,
        UtilsError::kBatchRollback,
        UtilsError::kBatchSizeExceeded,
        UtilsError::kRetryExhausted,
        UtilsError::kDeserInvalid,
        UtilsError::kPoolExhausted,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 6]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_UtilsError_Cast)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_RetryPolicy_BatchAlloc(benchmark::State& state) {
    const int N = static_cast<int>(state.range(0));
    for (auto _ : state) {
        std::vector<RetryPolicy> v(static_cast<size_t>(N));
        for (auto& p : v) p.maxAttempts = 3;
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_RetryPolicy_BatchAlloc)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_BloomFilterConfig_Batch(benchmark::State& state) {
    const int N = static_cast<int>(state.range(0));
    for (auto _ : state) {
        std::vector<BloomFilterConfig> v(static_cast<size_t>(N));
        for (auto& c : v) { c.expectedItems = 100; c.targetFalsePositiveRate = 0.01; }
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_BloomFilterConfig_Batch)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_UtilsError_SwitchDispatch(benchmark::State& state) {
    static const UtilsError kErrors[] = {
        UtilsError::kAuditOverflow,
        UtilsError::kBatchRollback,
        UtilsError::kBatchSizeExceeded,
        UtilsError::kRetryExhausted,
        UtilsError::kDeserInvalid,
        UtilsError::kPoolExhausted,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        auto err = kErrors[idx++ % 6];
        int32_t result = 0;
        switch (err) {
            case UtilsError::kAuditOverflow:     result = 1; break;
            case UtilsError::kBatchRollback:     result = 2; break;
            case UtilsError::kBatchSizeExceeded: result = 3; break;
            case UtilsError::kRetryExhausted:    result = 4; break;
            case UtilsError::kDeserInvalid:      result = 5; break;
            case UtilsError::kPoolExhausted:     result = 6; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_UtilsError_SwitchDispatch)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
