/*
 * ThemisDB | File: bench_metadata_release_gates.cpp | Version: 1.0.0
 * Gate table:
 * | ID          | Metric                        | Gate         |
 * |-------------|-------------------------------|--------------|
 * | GATE-MET-01 | MetaError cast throughput     | ≥ 50M ops/s  |
 * | GATE-MET-02 | MetaError switch dispatch     | ≥ 50M ops/s  |
 * | GATE-MET-03 | Error range check             | ≥ 50M ops/s  |
 * | GATE-MET-04 | Batch error cast              | ≥ 1M ops/s   |
 */

#include <benchmark/benchmark.h>
#include "metadata/metadata_api_contract.h"

#include <vector>

using namespace themis::metadata;

static constexpr uint64_t kCanonicalSeed = 42;

static void BM_MetaError_Cast(benchmark::State& state) {
    static const MetaError kErrors[] = {
        MetaError::kCollectionNotFound,
        MetaError::kFieldNotFound,
        MetaError::kSchemaMismatch,
        MetaError::kExportFailed,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 4]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_MetaError_Cast)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_MetaError_SwitchDispatch(benchmark::State& state) {
    static const MetaError kErrors[] = {
        MetaError::kCollectionNotFound,
        MetaError::kFieldNotFound,
        MetaError::kSchemaMismatch,
        MetaError::kExportFailed,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        auto err = kErrors[idx++ % 4];
        int32_t result = 0;
        switch (err) {
            case MetaError::kCollectionNotFound: result = 1; break;
            case MetaError::kFieldNotFound:      result = 2; break;
            case MetaError::kSchemaMismatch:     result = 3; break;
            case MetaError::kExportFailed:       result = 4; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_MetaError_SwitchDispatch)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_MetaError_RangeCheck(benchmark::State& state) {
    static const MetaError kErrors[] = {
        MetaError::kCollectionNotFound,
        MetaError::kFieldNotFound,
        MetaError::kSchemaMismatch,
        MetaError::kExportFailed,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 4]);
        bool inRange = (v >= 7900 && v <= 7999);
        benchmark::DoNotOptimize(inRange);
    }
}
BENCHMARK(BM_MetaError_RangeCheck)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_MetaError_BatchCast(benchmark::State& state) {
    const int N = static_cast<int>(state.range(0));
    static const MetaError kErrors[] = {
        MetaError::kCollectionNotFound,
        MetaError::kFieldNotFound,
        MetaError::kSchemaMismatch,
        MetaError::kExportFailed,
    };
    for (auto _ : state) {
        std::vector<int32_t> v;
        v.reserve(static_cast<size_t>(N));
        for (int i = 0; i < N; ++i)
            v.push_back(static_cast<int32_t>(kErrors[i % 4]));
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_MetaError_BatchCast)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
