# PERFORMANCE_EXPECTATIONS - src/failover

## Scope

- Module: src/failover
- This file defines measurable failover module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_replication_throughput.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| FOP-1 | recovery stack initialization proxy path remains within release baseline budget | BM_ReplicationManager_Initialize |
| FOP-2 | failover queue and DR-step paths must remain within regression budgets compared to release baseline | proxy via failover integration and soak runs |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| FOG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| FOG-2 | mapped failover proxy hot-path p99 <= release threshold | p99 from mapped proxy benchmark cases |
| FOG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Dedicated failover benchmarks are currently missing and must be added to reduce proxy reliance.

## Sourcecode Verification (Module: failover/performance)

- Verified benchmark sources:
  - benchmarks/bench_replication_throughput.cpp
- Verified mapping surfaces:
  - recovery stack initialization proxy benchmark case
- Result:
  - Referenced benchmark case exists in current benchmark sources.
  - Current release gate uses narrow proxy mapping until dedicated failover benchmarks are introduced.