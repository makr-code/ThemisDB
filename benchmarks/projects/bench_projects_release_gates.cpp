#include <benchmark/benchmark.h>
#include "projects/projects_api_contract.h"

#include <vector>

using namespace themis::projects;

static constexpr uint64_t kCanonicalSeed = 42;

static void BM_ProjError_Cast(benchmark::State& state) {
    static const ProjError kErrors[] = {
        ProjError::kMemberNotFound,
        ProjError::kProjectNotFound,
        ProjError::kQuotaExceeded,
        ProjError::kAuditOverflow,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 4]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_ProjError_Cast)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ProjError_SwitchDispatch(benchmark::State& state) {
    static const ProjError kErrors[] = {
        ProjError::kMemberNotFound,
        ProjError::kProjectNotFound,
        ProjError::kQuotaExceeded,
        ProjError::kAuditOverflow,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        auto err = kErrors[idx++ % 4];
        int32_t result = 0;
        switch (err) {
            case ProjError::kMemberNotFound:  result = 1; break;
            case ProjError::kProjectNotFound: result = 2; break;
            case ProjError::kQuotaExceeded:   result = 3; break;
            case ProjError::kAuditOverflow:   result = 4; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ProjError_SwitchDispatch)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ProjError_RangeCheck(benchmark::State& state) {
    static const ProjError kErrors[] = {
        ProjError::kMemberNotFound,
        ProjError::kProjectNotFound,
        ProjError::kQuotaExceeded,
        ProjError::kAuditOverflow,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 4]);
        bool inRange = (v >= 7700 && v <= 7799);
        benchmark::DoNotOptimize(inRange);
    }
}
BENCHMARK(BM_ProjError_RangeCheck)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ProjError_BatchCast(benchmark::State& state) {
    const int N = static_cast<int>(state.range(0));
    static const ProjError kErrors[] = {
        ProjError::kMemberNotFound,
        ProjError::kProjectNotFound,
        ProjError::kQuotaExceeded,
        ProjError::kAuditOverflow,
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
BENCHMARK(BM_ProjError_BatchCast)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
