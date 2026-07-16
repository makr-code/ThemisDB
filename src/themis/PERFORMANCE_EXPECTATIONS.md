# PERFORMANCE_EXPECTATIONS - src/themis

## Scope

- Module: src/themis
- This file defines measurable themis core module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_themis_core.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| THMP-1 | secure module loading and verification path remains bounded | BM_ModuleLoad_WithHashVerify |
| THMP-2 | build configuration cold/warm access paths remain bounded | BM_GetBuildConfiguration_Cold, BM_GetBuildConfiguration_Warm |
| THMP-3 | license and edition gate hot-path behavior remains bounded | BM_LicenseValidation_Ed25519, BM_EditionManager_IsFeatureEnabled_HotPath |
| THMP-4 | wire server concurrent-session path remains bounded | BM_WireServer_ConcurrentSessions_10k |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| THMG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| THMG-2 | themis core hot-path p99 <= release threshold | p99 from mapped themis benchmark cases |
| THMG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional themis core benchmark scenarios are introduced.

## Sourcecode Verification (Module: themis/performance)

- Verified benchmark sources:
  - benchmarks/bench_themis_core.cpp
- Verified mapping surfaces:
  - module load/verify, build-info, license/edition gating, and wire-session behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.