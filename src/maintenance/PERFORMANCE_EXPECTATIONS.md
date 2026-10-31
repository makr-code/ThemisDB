---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# Maintenance Module Performance Expectations

## Scope

- Module: src/maintenance
- This file maps only maintenance-owned benchmark targets. Thresholds are
  definitions, not evidence that a benchmark ran or passed.

## Registered module-owned release gates

`benchmarks/maintenance/CMakeLists.txt` registers
`bench_maintenance_release_gates` and `bench_maintenance_distributed_gates`
when `THEMIS_BUILD_BENCHMARKS` is enabled. The benchmark CMake integration also
registers both targets with CTest.

| Gate | Registered benchmark case | Threshold | What the case measures |
|---|---|---|
| GATE-MTN-01 | `mtn/BM_MTN01_ErrorEnumCast` | p99 ≤ 5 ns | Casts a helper `MaintenanceError` value |
| GATE-MTN-02 | `mtn/BM_MTN02_SwitchDispatch` | p99 ≤ 10 ns | Switches over helper error codes |
| GATE-MTN-03 | `mtn/BM_MTN03_StructAlloc` | p99 ≤ 500 ns | Initializes a `MaintenanceScheduleDescriptor`; not schedule CRUD |
| GATE-MTN-04 | `mtn/BM_MTN04_BatchCast` | p99 ≤ 5 µs/batch | Casts 1,000 helper error values |
| GATE-MTN-05 | `mtn/BM_MTN05_InFlightGuard` | p99 ≤ 50 µs | Local mutex and in-flight-set insert/erase model |
| GATE-MTN-06 | `mtn/BM_MTN06_PersistReloadRoundTrip` | p99 ≤ 2 ms | JSON and temporary-file round trip for 10 schedules; not `MaintenanceScheduleStore` |
| GATE-MTN-07 | `mtn/BM_MTN07_RingBufferWrite` | p99 ≤ 200 ns | Local mutex/deque ring; not the orchestrator's ring buffer |
| GATE-MTN-DIST-01 | `mtn/BM_MTN_DIST01_LeaderGatedDispatch` | p99 ≤ 500 µs | Mock mutex/boolean leadership and counter, not `IDistributedLock` dispatch |
| GATE-MTN-DIST-02 | `mtn/BM_MTN_DIST02_ScheduleListing` | p99 ≤ 5 ms | Copies 1,000 entries from a local map; not `DatabaseMaintenanceOrchestrator::listSchedules()` |

## Interpretation and evidence

- The case names, thresholds, target registrations, and manifest mapping are
  defined in `benchmarks/maintenance/` and
  `benchmarks/maintenance/release_gate_manifest_mtn.json`.
- The benchmark implementations are proxies or helper-type microbenchmarks;
  none measures the production orchestrator or schedule store end to end.
- The source and manifest label these limits as p99, but the benchmark sources
  use repeated Google Benchmark runs with aggregate reporting; they do not
  calculate a p99 percentile themselves. **TODO/Assumption:** verify an
  external result-processing step supplies the manifest's p99 metric before
  treating these as blocking percentile gates.
- The manifest's `PASS` labels have no accompanying run output here and must not
  be treated as measured results. Record release-profile output and baseline
  comparisons before claiming a performance gate passed.
- Repository benchmarks for scheduler, sharding, index rebuild, or TPCC are not
  maintenance-owned release gates and are intentionally not mapped here.
