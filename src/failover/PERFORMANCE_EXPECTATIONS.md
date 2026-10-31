---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Failover Benchmark Workloads and Proposed Budgets

## Scope

- Module: src/failover
- The tables below reproduce workload names and target thresholds declared in benchmark sources. They are targets, not evidence that a release run passed.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/failover/bench_failover_release_gates.cpp
  - benchmarks/bench_replication_throughput.cpp (legacy proxy, supplemental)

## Specific Expectations

| Target ID | Benchmark-source workload | Declared target |
|---|---|---|
| FRG-01 | Mock heartbeat message work (no network send) | p99 ≤ 500 µs |
| FRG-02 | Deterministic in-memory mock election (no network consensus) | p99 ≤ 5 ms |
| FRG-03 | Fixed-size mock state-sync message construction | p99 ≤ 200 µs |
| FRG-04 | Synthetic health predicate evaluation | p99 ≤ 100 µs |
| FRG-05 | Mock in-flight buffer occupancy check | p99 ≤ 50 µs |
| FRG-06 | Atomic epoch increment (mock persist; no storage I/O) | p99 ≤ 1 ms |

## Additional declared thresholds

| Gate ID | Expectation | Measurement |
|---|---|---|
| GATE-FRG-01 | heartbeat p99 ≤ 500 µs | FRG-01 |
| GATE-FRG-02 | election p99 ≤ 5 ms | FRG-02 |
| GATE-FRG-03 | state sync p99 ≤ 200 µs | FRG-03 |
| GATE-FRG-04 | health check p99 ≤ 100 µs | FRG-04 |
| GATE-FRG-05 | buffer check p99 ≤ 50 µs | FRG-05 |
| GATE-FRG-06 | epoch persist p99 ≤ 1 ms | FRG-06 |

The Phase 2/3 (FP23) and Wave B (FWB) benchmark files declare additional thresholds. Those targets do not themselves implement CI pass/fail comparisons.

## Validation status

- `benchmarks/failover/CMakeLists.txt` registers three Google Benchmark executables when `THEMIS_BUILD_BENCHMARKS` is enabled.
- The FRG and FP23 cases use synthetic or inline-mocked workloads; FWB uses simulations for several cases and excludes real log file I/O in its stated append case. They are not end-to-end failover/recovery measurements.
- The source labels declare p99 thresholds but do not configure a percentile statistic or a threshold assertion in these benchmark registrations. FWB-02 also has inconsistent source targets: the overview says p99 ≤ 1 ms (with ≤100 µs for the in-memory path), while its gate table says ≤100 µs.
- No benchmark execution or measured p95/p99 results are asserted by this document. Run release-profile benchmarks on the target hardware before making latency or release-gate claims.

## Sourcecode Verification (Module: failover/performance)

- Audited benchmark sources:
  - benchmarks/failover/bench_failover_release_gates.cpp (FRG-01..FRG-06)
  - benchmarks/bench_replication_throughput.cpp (supplemental proxy)
- Result: the FRG IDs map to registrations in `bench_failover_release_gates.cpp`; this confirms source presence, not a passed performance gate.