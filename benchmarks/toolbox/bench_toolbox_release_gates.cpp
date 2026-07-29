/*
 * ThemisDB | File: bench_toolbox_release_gates.cpp | Version: 1.0.0
 * Gate table:
 * | ID          | Metric                        | Gate         |
 * |-------------|-------------------------------|--------------|
 * | GATE-TBX-01 | Fingerprint default-init rate | ≥ 5M ops/s   |
 * | GATE-TBX-02 | ToolboxError cast throughput  | ≥ 50M ops/s  |
 * | GATE-TBX-03 | Fingerprint memset rate       | ≥ 5M ops/s   |
 * | GATE-TBX-04 | Error switch dispatch         | ≥ 50M ops/s  |
 */

#include <benchmark/benchmark.h>
#include "toolbox/toolbox_api_contract.h"

#include <cstring>
#include <vector>

using namespace themis::toolbox;

static constexpr uint64_t kCanonicalSeed = 42;

static void BM_Fingerprint_DefaultInit(benchmark::State& state) {
    for (auto _ : state) {
        Fingerprint fp{};
        benchmark::DoNotOptimize(fp.data());
    }
}
BENCHMARK(BM_Fingerprint_DefaultInit)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ToolboxError_Cast(benchmark::State& state) {
    static const ToolboxError kErrors[] = {
        ToolboxError::kEmptyInput,
        ToolboxError::kNoProcessor,
        ToolboxError::kProcessorFailed,
        ToolboxError::kEncodingUnsupported,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 4]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_ToolboxError_Cast)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_Fingerprint_Memset(benchmark::State& state) {
    for (auto _ : state) {
        Fingerprint fp;
        fp.fill(0xAB);
        benchmark::DoNotOptimize(fp.data());
    }
}
BENCHMARK(BM_Fingerprint_Memset)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ToolboxError_SwitchDispatch(benchmark::State& state) {
    static const ToolboxError kErrors[] = {
        ToolboxError::kEmptyInput,
        ToolboxError::kNoProcessor,
        ToolboxError::kProcessorFailed,
        ToolboxError::kEncodingUnsupported,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        auto err = kErrors[idx++ % 4];
        int32_t result = 0;
        switch (err) {
            case ToolboxError::kEmptyInput:          result = 1; break;
            case ToolboxError::kNoProcessor:         result = 2; break;
            case ToolboxError::kProcessorFailed:     result = 3; break;
            case ToolboxError::kEncodingUnsupported: result = 4; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ToolboxError_SwitchDispatch)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
