# PERFORMANCE_EXPECTATIONS - src/chaos

## Scope

- Module: src/chaos
- This file defines measurable chaos module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark file:
  - benchmarks/bench_chaos_stress.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| CHS-1 | callback dispatch path remains bounded across configured callback fan-out levels | BM_CallbackDispatch |
| CHS-2 | concurrent stress path remains bounded under benchmark thread range | BM_ConcurrentStress |
| CHS-3 | scheduler enqueue path remains bounded in release profile | BM_ChaosScheduler_Schedule |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| CHG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| CHG-2 | concurrent/scheduler path p99 <= release threshold | p99 from mapped chaos-stress benchmark cases |
| CHG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- For proxy-only targets, keep follow-up benchmark hardening explicitly tracked.

## Sourcecode Verification (Module: chaos/performance)

- Verified benchmark source:
  - benchmarks/bench_chaos_stress.cpp
- Verified mapping surfaces:
  - callback dispatch, concurrent stress, and scheduler schedule paths
- Result:
  - Referenced benchmark cases exist in current benchmark source.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.