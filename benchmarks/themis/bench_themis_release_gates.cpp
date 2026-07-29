/*
 * ThemisDB | File: bench_themis_release_gates.cpp | Version: 1.0.0
 * Gate table:
 * | ID          | Metric                        | Gate         |
 * |-------------|-------------------------------|--------------|
 * | GATE-THE-01 | ThemisError cast throughput   | ≥ 50M ops/s  |
 * | GATE-THE-02 | Edition enum switch dispatch  | ≥ 50M ops/s  |
 * | GATE-THE-03 | Edition value lookup          | ≥ 50M ops/s  |
 * | GATE-THE-04 | Batch Edition cast            | ≥ 1M ops/s   |
 */

#include <benchmark/benchmark.h>
#include "themis/themis_api_contract.h"

#include <vector>

using namespace themis::engine;

static constexpr uint64_t kCanonicalSeed = 42;

static void BM_ThemisError_Cast(benchmark::State& state) {
    static const ThemisError kErrors[] = {
        ThemisError::kEditionMismatch,
        ThemisError::kFeatureUnknown,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 2]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_ThemisError_Cast)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_Edition_SwitchDispatch(benchmark::State& state) {
    static const Edition kEditions[] = {
        Edition::kMinimal, Edition::kCommunity,
        Edition::kEnterprise, Edition::kHyperscaler, Edition::kMilitary,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        auto ed = kEditions[idx++ % 5];
        int32_t result = 0;
        switch (ed) {
            case Edition::kMinimal:     result = 1; break;
            case Edition::kCommunity:   result = 2; break;
            case Edition::kEnterprise:  result = 3; break;
            case Edition::kHyperscaler: result = 4; break;
            case Edition::kMilitary:    result = 5; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Edition_SwitchDispatch)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_Edition_ValueLookup(benchmark::State& state) {
    static const Edition kEditions[] = {
        Edition::kMinimal, Edition::kCommunity,
        Edition::kEnterprise, Edition::kHyperscaler, Edition::kMilitary,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kEditions[idx++ % 5]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_Edition_ValueLookup)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_Edition_BatchCast(benchmark::State& state) {
    const int N = static_cast<int>(state.range(0));
    static const Edition kEditions[] = {
        Edition::kMinimal, Edition::kCommunity,
        Edition::kEnterprise, Edition::kHyperscaler, Edition::kMilitary,
    };
    for (auto _ : state) {
        std::vector<int32_t> v;
        v.reserve(static_cast<size_t>(N));
        for (int i = 0; i < N; ++i)
            v.push_back(static_cast<int32_t>(kEditions[i % 5]));
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_Edition_BatchCast)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
