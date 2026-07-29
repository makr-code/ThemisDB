/*
 * ThemisDB | File: bench_process_release_gates.cpp | Version: 1.0.0
 * Gate table:
 * | ID          | Metric                        | Gate         |
 * |-------------|-------------------------------|--------------|
 * | GATE-PRC-01 | ProcError cast throughput     | ≥ 50M ops/s  |
 * | GATE-PRC-02 | ProcError switch dispatch     | ≥ 50M ops/s  |
 * | GATE-PRC-03 | Error range check             | ≥ 50M ops/s  |
 * | GATE-PRC-04 | Batch error cast              | ≥ 1M ops/s   |
 */

#include <benchmark/benchmark.h>
#include "process/process_api_contract.h"

#include <vector>

using namespace themis::process;

static constexpr uint64_t kCanonicalSeed = 42;

static void BM_ProcError_Cast(benchmark::State& state) {
    static const ProcError kErrors[] = {
        ProcError::kUnsupportedElement,
        ProcError::kInvalidTransition,
        ProcError::kSerialiserFailed,
        ProcError::kDeserialiserFailed,
        ProcError::kExecutionTimeout,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 5]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_ProcError_Cast)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ProcError_SwitchDispatch(benchmark::State& state) {
    static const ProcError kErrors[] = {
        ProcError::kUnsupportedElement,
        ProcError::kInvalidTransition,
        ProcError::kSerialiserFailed,
        ProcError::kDeserialiserFailed,
        ProcError::kExecutionTimeout,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        auto err = kErrors[idx++ % 5];
        int32_t result = 0;
        switch (err) {
            case ProcError::kUnsupportedElement:  result = 1; break;
            case ProcError::kInvalidTransition:   result = 2; break;
            case ProcError::kSerialiserFailed:    result = 3; break;
            case ProcError::kDeserialiserFailed:  result = 4; break;
            case ProcError::kExecutionTimeout:    result = 5; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ProcError_SwitchDispatch)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ProcError_RangeCheck(benchmark::State& state) {
    static const ProcError kErrors[] = {
        ProcError::kUnsupportedElement,
        ProcError::kInvalidTransition,
        ProcError::kSerialiserFailed,
        ProcError::kDeserialiserFailed,
        ProcError::kExecutionTimeout,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 5]);
        bool inRange = (v >= 7600 && v <= 7699);
        benchmark::DoNotOptimize(inRange);
    }
}
BENCHMARK(BM_ProcError_RangeCheck)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ProcError_BatchCast(benchmark::State& state) {
    const int N = static_cast<int>(state.range(0));
    static const ProcError kErrors[] = {
        ProcError::kUnsupportedElement,
        ProcError::kInvalidTransition,
        ProcError::kSerialiserFailed,
        ProcError::kDeserialiserFailed,
        ProcError::kExecutionTimeout,
    };
    for (auto _ : state) {
        std::vector<int32_t> v;
        v.reserve(static_cast<size_t>(N));
        for (int i = 0; i < N; ++i)
            v.push_back(static_cast<int32_t>(kErrors[i % 5]));
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_ProcError_BatchCast)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
