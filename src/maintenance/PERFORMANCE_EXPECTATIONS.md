---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-01
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

| Gate | Registered benchmark case | Target | What the case measures | Status |
|---|---|---|---|---|
| GATE-MTN-01 | `mtn/BM_MTN01_ErrorEnumCast` | p99 ≤ 5 ns | Casts a helper `MaintenanceError` value | UNVERIFIED |
| GATE-MTN-02 | `mtn/BM_MTN02_SwitchDispatch` | p99 ≤ 10 ns | Switches over helper error codes | UNVERIFIED |
| GATE-MTN-03 | `mtn/BM_MTN03_StructAlloc` | p99 ≤ 500 ns | Initializes a `MaintenanceScheduleDescriptor`; not schedule CRUD | UNVERIFIED |
| GATE-MTN-04 | `mtn/BM_MTN04_BatchCast` | p99 ≤ 5 µs/batch | Casts 1,000 helper error values | UNVERIFIED |
| GATE-MTN-05 | `mtn/BM_MTN05_InFlightGuard` | p99 ≤ 50 µs | Local mutex and in-flight-set insert/erase model | UNVERIFIED |
| GATE-MTN-06 | `mtn/BM_MTN06_PersistReloadRoundTrip` | p99 ≤ 2 ms | JSON and temporary-file round trip for 10 schedules; not `MaintenanceScheduleStore` | UNVERIFIED |
| GATE-MTN-07 | `mtn/BM_MTN07_RingBufferWrite` | p99 ≤ 200 ns | Local mutex/deque ring; not the orchestrator's ring buffer | UNVERIFIED |
| GATE-MTN-DIST-01 | `mtn/BM_MTN_DIST01_LeaderGatedDispatch` | p99 ≤ 500 µs | Mock mutex/boolean leadership and counter, not `IDistributedLock` dispatch | UNVERIFIED |
| GATE-MTN-DIST-02 | `mtn/BM_MTN_DIST02_ScheduleListing` | p99 ≤ 5 ms | Copies 1,000 entries from a local map; not `DatabaseMaintenanceOrchestrator::listSchedules()` | UNVERIFIED |

## Interpretation and evidence

- The case names, thresholds, target registrations, and manifest mapping are
  defined in `benchmarks/maintenance/` and
  `benchmarks/maintenance/release_gate_manifest_mtn.json`.
- The benchmark implementations are proxies or helper-type microbenchmarks;
  none measures the production orchestrator or schedule store end to end.
- The p99 values are target thresholds only. The benchmark sources report
  repeated Google Benchmark aggregates; they do not calculate per-operation
  p99, and no external processor wired to these targets was verified.
- The manifest marks all nine gates `UNVERIFIED`; there is no measured run output
  or baseline comparison to support a `PASS`. Do not treat source labels,
  aggregate means, or target registration as p99 evidence.
- Repository benchmarks for scheduler, sharding, index rebuild, or TPCC are not
  maintenance-owned release gates and are intentionally not mapped here.
