# PERFORMANCE_EXPECTATIONS - src/failover

## Scope

- Module: src/failover
- This file defines measurable failover module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/failover/bench_failover_release_gates.cpp
  - benchmarks/bench_replication_throughput.cpp (legacy proxy, supplemental)

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| FOP-1 | heartbeat send overhead p99 ≤ 500 µs | BM_Failover_HeartbeatSendOverhead (FRG-01) |
| FOP-2 | leader election decision p99 ≤ 5 ms | BM_Failover_LeaderElectionDecision (FRG-02) |
| FOP-3 | state sync message serialize p99 ≤ 200 µs | BM_Failover_StateSyncSerialize (FRG-03) |
| FOP-4 | health check evaluation p99 ≤ 100 µs | BM_Failover_HealthCheckEvaluation (FRG-04) |
| FOP-5 | in-flight request buffer check p99 ≤ 50 µs | BM_Failover_InFlightBufferCheck (FRG-05) |
| FOP-6 | epoch increment + persist (mock) p99 ≤ 1 ms | BM_Failover_EpochIncrementPersist (FRG-06) |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| GATE-FRG-01 | heartbeat p99 ≤ 500 µs | FRG-01 |
| GATE-FRG-02 | election p99 ≤ 5 ms | FRG-02 |
| GATE-FRG-03 | state sync p99 ≤ 200 µs | FRG-03 |
| GATE-FRG-04 | health check p99 ≤ 100 µs | FRG-04 |
| GATE-FRG-05 | buffer check p99 ≤ 50 µs | FRG-05 |
| GATE-FRG-06 | epoch persist p99 ≤ 1 ms | FRG-06 |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Dedicated failover benchmarks (FRG-01..FRG-06) delivered in Q3 2026; proxy reliance reduced.
- p95/p99 re-baseline planned for Q1 2027 (see ROADMAP.md mid-term planned features).

## Sourcecode Verification (Module: failover/performance)

- Verified benchmark sources:
  - benchmarks/failover/bench_failover_release_gates.cpp (FRG-01..FRG-06)
  - benchmarks/bench_replication_throughput.cpp (supplemental proxy)
- Result:
  - Dedicated failover benchmark cases delivered. Proxy reliance is supplemental only.
  - All six GATE-FRG gates are mapped to concrete benchmark registrations.