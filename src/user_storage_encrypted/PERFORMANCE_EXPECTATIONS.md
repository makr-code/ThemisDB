# PERFORMANCE_EXPECTATIONS - src/user_storage_encrypted

## Scope

- Module: src/user_storage_encrypted
- This file defines measurable user_storage_encrypted performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_user_storage_mount_latency.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| USEP-1 | backend initialization, availability check, and mount-state inspection remain bounded | GocryptfsBenchFixture/Initialize, GocryptfsBenchFixture/CheckAvailability, GocryptfsBenchFixture/IsMounted |
| USEP-2 | fast-fail mount and unmount paths remain bounded under invalid or unavailable backend conditions | GocryptfsBenchFixture/MountContainer_FastFail, GocryptfsBenchFixture/UnmountContainer_FastFail |
| USEP-3 | lifecycle dispatch and repeated mount/unmount cycles remain bounded | GocryptfsBenchFixture/MountUnmountCycle, BM_MountDispatch_NoKDF, GocryptfsBenchFixture/BackendMeta |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| USEG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| USEG-2 | encrypted mount lifecycle p99 <= release threshold | p99 from mapped encrypted storage benchmark cases |
| USEG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional encrypted-storage benchmark scenarios are introduced.

## Sourcecode Verification (Module: user_storage_encrypted/performance)

- Verified benchmark sources:
  - benchmarks/bench_user_storage_mount_latency.cpp
- Verified mapping surfaces:
  - backend initialization and availability behavior
  - mount-state, fast-fail, and mount/unmount cycle behavior
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.