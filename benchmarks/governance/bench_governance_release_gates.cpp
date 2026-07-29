/*
 * ThemisDB | File: bench_governance_release_gates.cpp | Version: 1.0.0
 * Gate table:
 * | ID          | Metric                           | Gate         |
 * |-------------|----------------------------------|--------------|
 * | GATE-GOV-01 | GovError cast throughput         | ≥ 50M ops/s  |
 * | GATE-GOV-02 | ComplianceCheckResult alloc rate | ≥ 5M ops/s   |
 * | GATE-GOV-03 | Regulation enum switch dispatch  | ≥ 50M ops/s  |
 * | GATE-GOV-04 | GovError switch dispatch         | ≥ 50M ops/s  |
 */

#include <benchmark/benchmark.h>
#include "governance/governance_api_contract.h"

#include <vector>

using namespace themis::governance;

static constexpr uint64_t kCanonicalSeed = 42;

static void BM_GovError_Cast(benchmark::State& state) {
    static const GovError kErrors[] = {
        GovError::kRegulationUnknown,
        GovError::kConsentMissing,
        GovError::kJurisdictionBlocked,
        GovError::kExportLimitExceeded,
        GovError::kAuditWriteFailed,
        GovError::kRuleConflict,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        int32_t v = static_cast<int32_t>(kErrors[idx++ % 6]);
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_GovError_Cast)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_ComplianceCheckResult_Alloc(benchmark::State& state) {
    for (auto _ : state) {
        ComplianceCheckResult r;
        r.allowed = true;
        r.justification = "Consent verified";
        benchmark::DoNotOptimize(r.allowed);
    }
}
BENCHMARK(BM_ComplianceCheckResult_Alloc)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_Regulation_SwitchDispatch(benchmark::State& state) {
    static const Regulation kRegs[] = {
        Regulation::kGDPR, Regulation::kCCPA,
        Regulation::kLGPD, Regulation::kHIPAA,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        auto reg = kRegs[idx++ % 4];
        int32_t result = 0;
        switch (reg) {
            case Regulation::kGDPR:  result = 1; break;
            case Regulation::kCCPA:  result = 2; break;
            case Regulation::kLGPD:  result = 3; break;
            case Regulation::kHIPAA: result = 4; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_Regulation_SwitchDispatch)->Repetitions(5)->ReportAggregatesOnly(true);

static void BM_GovError_SwitchDispatch(benchmark::State& state) {
    static const GovError kErrors[] = {
        GovError::kRegulationUnknown,
        GovError::kConsentMissing,
        GovError::kJurisdictionBlocked,
        GovError::kExportLimitExceeded,
        GovError::kAuditWriteFailed,
        GovError::kRuleConflict,
    };
    uint64_t idx = 0;
    for (auto _ : state) {
        auto err = kErrors[idx++ % 6];
        int32_t result = 0;
        switch (err) {
            case GovError::kRegulationUnknown:   result = 1; break;
            case GovError::kConsentMissing:      result = 2; break;
            case GovError::kJurisdictionBlocked: result = 3; break;
            case GovError::kExportLimitExceeded: result = 4; break;
            case GovError::kAuditWriteFailed:    result = 5; break;
            case GovError::kRuleConflict:        result = 6; break;
        }
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_GovError_SwitchDispatch)->Repetitions(5)->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
