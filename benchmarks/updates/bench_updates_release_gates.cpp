/*
 * ThemisDB | File: bench_updates_release_gates.cpp | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gate table:
 * | ID          | Metric                          | Gate         |
 * |-------------|---------------------------------|--------------|
 * | GATE-UPD-01 | VerificationResult alloc rate   | ≥ 5M ops/s   |
 * | GATE-UPD-02 | DeploymentSlot switch dispatch  | ≥ 50M ops/s  |
 * | GATE-UPD-03 | Error-code cast throughput      | ≥ 50M ops/s  |
 * | GATE-UPD-04 | VerificationResult copy rate    | ≥ 5M ops/s   |
 * | GATE-UPD-05 | VerificationResult batch alloc  | ≥ 1M ops/s   |
 * | GATE-UPD-06 | Error switch dispatch           | ≥ 50M ops/s  |
 */

#include <benchmark/benchmark.h>
#include "updates/updates_api_contract.h"

#include <random>
#include <vector>

using namespace themis::updates;

static constexpr uint64_t kCanonicalSeed = 42;

static void BM_VerificationResult_Alloc(benchmark::State& state) {
    for (auto _ : state) {
        VerificationResult r;
        r.valid = true;
        r.signatureToken = "tok";
        benchmark::DoNotOptimize(r.valid);
    }
}
BENCHMARK(BM_VerificationResult_Alloc)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_DeploymentSlot_Switch(benchmark::State& state) {
    static const DeploymentSlot kSlots[] = {
        DeploymentSlot::kBlue, DeploymentSlot::kGreen,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kSlots[idx++ % 2]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_DeploymentSlot_Switch)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_UpdatesError_Cast(benchmark::State& state) {
    static const UpdatesError kErrors[] = {
        UpdatesError::kNoRollbackTarget,
        UpdatesError::kChecksumMismatch,
        UpdatesError::kArtifactMissing,
        UpdatesError::kPatchIncompatible,
        UpdatesError::kSwitchInProgress,
        UpdatesError::kCanaryAborted,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 6]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_UpdatesError_Cast)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_VerificationResult_Copy(benchmark::State& state) {
    VerificationResult orig;
    orig.valid = true;
    orig.signatureToken = "sha256:aabbcc";
    for (auto _ : state) {
        VerificationResult copy = orig;
        benchmark::DoNotOptimize(copy.valid);
    }
}
BENCHMARK(BM_VerificationResult_Copy)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_VerificationResult_BatchAlloc(benchmark::State& state) {
    const int N = static_cast<int>(state.range(0));
    for (auto _ : state) {
        std::vector<VerificationResult> v(static_cast<size_t>(N));
        for (auto& r : v) r.valid = true;
        benchmark::DoNotOptimize(v.data());
    }
    state.SetItemsProcessed(state.iterations() * N);
}
BENCHMARK(BM_VerificationResult_BatchAlloc)->Arg(1000)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_UpdatesError_SwitchDispatch(benchmark::State& state) {
    static const UpdatesError kErrors[] = {
        UpdatesError::kNoRollbackTarget,
        UpdatesError::kChecksumMismatch,
        UpdatesError::kArtifactMissing,
        UpdatesError::kPatchIncompatible,
        UpdatesError::kSwitchInProgress,
        UpdatesError::kCanaryAborted,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        auto err = kErrors[idx++ % 6];
        int32_t result = 0;
        switch (err) {
            case UpdatesError::kNoRollbackTarget:  result = 1; break;
            case UpdatesError::kChecksumMismatch:  result = 2; break;
            case UpdatesError::kArtifactMissing:   result = 3; break;
            case UpdatesError::kPatchIncompatible: result = 4; break;
            case UpdatesError::kSwitchInProgress:  result = 5; break;
            case UpdatesError::kCanaryAborted:     result = 6; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_UpdatesError_SwitchDispatch)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
