# PERFORMANCE_EXPECTATIONS - src/maintenance

## Scope

- Module: src/maintenance
- This file defines measurable maintenance module performance expectations for release gating.

## Benchmark Reference

- Relevant benchmark files:
  - benchmarks/bench_task_scheduler.cpp
  - benchmarks/bench_distributed_coordinator.cpp
  - benchmarks/bench_index_rebuild.cpp
  - benchmarks/bench_tpcc.cpp

## Specific Expectations

| Target ID | Expectation | Benchmark case |
|---|---|---|
| MNTP-1 | maintenance scheduling registration, sync execution, listing, and stats paths remain bounded | TaskSchedulerBenchFixture/RegisterUnregister, TaskSchedulerBenchFixture/ExecuteTaskNow, TaskSchedulerBenchFixture/ListTasks, TaskSchedulerBenchFixture/GetStats, TaskSchedulerBenchFixture/ConcurrentRegister |
| MNTP-2 | distributed maintenance coordination scheduling/control paths remain bounded | BM_Coordinator_ScheduleTask, BM_Coordinator_GetPendingTasks, BM_Coordinator_CancelTask, BM_Coordinator_StartElection, BM_Coordinator_BecomeLeader |
| MNTP-3 | maintenance-adjacent index rebuild and full reindex operations remain bounded | RebuildFixture/Rebuild_Regular_Email, RebuildFixture/Rebuild_Composite_CityAge, RebuildFixture/Rebuild_Range_Salary, RebuildFixture/Rebuild_Sparse_Nickname, RebuildFixture/Rebuild_TTL_ExpiresAt, RebuildFixture/Rebuild_Fulltext_Bio, RebuildFixture/ReindexEntireTable |
| MNTP-4 | system-level transactional pressure proxy for maintenance windows remains bounded | TPCCLiteFixture/NewOrderTransaction, TPCCLiteFixture/PaymentTransaction, TPCCLiteFixture/OrderStatusTransaction, TPCCLiteFixture/StockLevelTransaction, TPCCLiteFixture/NewOrderLite, TPCCLiteFixture/MixedWorkload |

## Module Hard Gates (v1.0 docs baseline)

| Gate ID | Expectation | Measurement |
|---|---|---|
| MNTG-1 | Regression <= 10 percent vs release baseline | (current - baseline) / baseline |
| MNTG-2 | maintenance hot-path p99 <= release threshold | p99 from mapped maintenance benchmark cases |
| MNTG-3 | No mapped benchmark case missing in release run | benchmark run manifest completeness |

## Validation

- Expectations are met when mapped benchmarks run reproducibly in release profile and remain inside configured thresholds.
- Mapping should be expanded as additional maintenance-dedicated benchmark scenarios are introduced.

## Sourcecode Verification (Module: maintenance/performance)

- Verified benchmark sources:
  - benchmarks/bench_task_scheduler.cpp
  - benchmarks/bench_distributed_coordinator.cpp
  - benchmarks/bench_index_rebuild.cpp
  - benchmarks/bench_tpcc.cpp
  - benchmarks/maintenance/bench_maintenance_release_gates.cpp
  - benchmarks/maintenance/bench_maintenance_distributed_gates.cpp
- Verified mapping surfaces:
  - scheduler/orchestrator hot paths and distributed maintenance coordination
  - maintenance-adjacent index rebuild operations
  - transactional proxy pressure for maintenance windows
  - concurrent-scheduling-guard (in-flight lookup + insert) — Phase 2
  - persistence/reload round-trip — Phase 3
  - DispatchOutcome ring-buffer write — Phase 4
  - distributed lock acquisition and schedule listing — Phase 5
- Result:
  - Referenced benchmark cases exist in current benchmark sources.
  - Release gates remain tied to reproducible benchmark runs and baseline comparisons.

## Phase 1-6 Hardening Gates (added 2026-08-05)

| Gate ID | Expectation | Measurement | Benchmark File |
|---|---|---|---|
| GATE-MTN-05 | In-flight guard check p99 ≤ 50 µs | latency_p99_us | benchmarks/maintenance/bench_maintenance_release_gates.cpp |
| GATE-MTN-06 | Persist+reload (10 schedules) p99 ≤ 2 ms | latency_p99_ms | benchmarks/maintenance/bench_maintenance_release_gates.cpp |
| GATE-MTN-07 | Ring-buffer write p99 ≤ 200 ns | latency_p99_ns | benchmarks/maintenance/bench_maintenance_release_gates.cpp |
| GATE-MTN-DIST-01 | Leader-gated dispatch p99 ≤ 500 µs | latency_p99_us | benchmarks/maintenance/bench_maintenance_distributed_gates.cpp |
| GATE-MTN-DIST-02 | Schedule listing (1000 entries) p99 ≤ 5 ms | latency_p99_ms | benchmarks/maintenance/bench_maintenance_distributed_gates.cpp |
