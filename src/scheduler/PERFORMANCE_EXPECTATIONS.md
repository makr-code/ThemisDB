---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: module-behavior
Canonical references: benchmarks/scheduler/*.cpp; benchmarks/scheduler/CMakeLists.txt; include/scheduler/task_scheduler.h; tests/scheduler/CMakeLists.txt
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler performance benchmark map

This page maps existing benchmark source to the code it measures. The existence
of a benchmark is not evidence that a release gate runs in CI or that a
hardware-specific target has been met.

## Benchmark sources

| File / cases | What the source measures | Scope caveat |
|---|---|---|
| `benchmarks/scheduler/bench_task_scheduler.cpp`: `TaskSchedulerBenchFixture/{RegisterUnregister,ExecuteTaskNow,ListTasks,GetStats,ConcurrentRegister}` | Calls the production `TaskScheduler` APIs. `ListTasks` registers 10, 50, or 100 tasks. | The fixture uses a real in-process storage/query setup and disables persistence/audit/anomaly features. This is the only scheduler benchmark used for the CTest smoke. |
| `benchmarks/scheduler/bench_scheduler_dedicated_gates.cpp`: `BM_SC_BM_01_RegisterP95` through `BM_SC_BM_04_StatsQueryP95` | A local `stubs::TaskScheduler` with a mutex and queue. | Does not call production `TaskScheduler`. “p95” appears in case names/labels; no percentile computation is configured in this source. |
| `benchmarks/scheduler/bench_scheduler_release_gates.cpp`: `BM_SCH01`–`BM_SCH04` | Enum cast/switch, a locally declared descriptor allocation, and batch enum casts. | Not measurements of task registration, execution, or end-to-end scheduler behavior. The labels state targets; this source does not enforce them as pass/fail assertions. |
| `benchmarks/scheduler/bench_thread_pool_saturation.cpp` | `ThreadPoolManager` submission, saturation, ordering, producer, stats, and shutdown cases. | Generic thread-pool measurements, not scheduler task lifecycle benchmarks. |

`benchmarks/scheduler/CMakeLists.txt` registers `bench_*.cpp` through
`themis_add_standard_benchmark` when `THEMIS_BUILD_BENCHMARKS` is enabled.

## Expectations and gates

No current scheduler CI threshold, baseline manifest, benchmark owner, or
representative-hardware result was verified in the scoped benchmark/CMake
sources. The production-path benchmark is registered as a CTest smoke that
checks successful execution only; its timing is informational. Numeric targets
embedded in other benchmark comments/labels are not scheduler release criteria.

**Release gate remains unresolved:** an owner must identify stable representative
hardware, establish a baseline, agree on the measured statistic and threshold,
and decide which cases gate a release. Until then, do not claim a verified
p95/p99, regression budget, or release-critical benchmark.
