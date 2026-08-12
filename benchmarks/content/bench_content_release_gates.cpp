#include <benchmark/benchmark.h>
#include "content/content_api_contract.h"

#include <vector>

using namespace themis::content;

static constexpr uint64_t kCanonicalSeed = 42;

static void BM_ScanResult_Alloc(benchmark::State& state) {
    for (auto _ : state) {
        ScanResult r;
        r.verdict = ScanVerdict::kAllow;
        benchmark::DoNotOptimize(r.verdict);
    }
}
BENCHMARK(BM_ScanResult_Alloc)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ContentError_Cast(benchmark::State& state) {
    static const ContentError kErrors[] = {
        ContentError::kScanError,
        ContentError::kUnsupportedFormat,
        ContentError::kSizeLimit,
        ContentError::kEncodingInvalid,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 4]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_ContentError_Cast)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ScanVerdict_Switch(benchmark::State& state) {
    static const ScanVerdict kVerdicts[] = {
        ScanVerdict::kAllow, ScanVerdict::kBlock,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        auto v = kVerdicts[idx++ % 2];
        int32_t result = 0;
        switch (v) {
            case ScanVerdict::kAllow: result = 0; break;
            case ScanVerdict::kBlock: result = 1; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ScanVerdict_Switch)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ScanResult_BatchAlloc(benchmark::State& state) {
    const int N = static_cast<int>(state.range(0));
    for (auto _ : state) {
        std::vector<ScanResult> v(static_cast<size_t>(N));
        for (auto& r : v) r.verdict = ScanVerdict::kAllow;
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_ScanResult_BatchAlloc)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
