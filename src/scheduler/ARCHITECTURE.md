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
Canonical references: include/scheduler/task_scheduler.h; include/scheduler/scheduler_api_contract.h; src/scheduler/*.cpp
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler Module Architecture

## Scope and source layout

The scheduler implementation is split across task lifecycle/execution, coordination
and adapter, retention, trigger, audit, result, and anomaly components. The public
declarations are in `include/scheduler/`; implementations are in `src/scheduler/`.
The contracts below describe source-visible behavior, not a distributed-systems
guarantee or production-readiness certification.

| Surface | Canonical implementation | Source-visible responsibility |
|---|---|---|
| `TaskScheduler`, `ScheduledTask` | `task_scheduler.cpp` | Task registration, validation, execution, scheduling, stats, and optional persistence/observability integrations |
| `DistributedTaskCoordinator` | `distributed_task_coordinator.cpp` | Leadership callbacks and activation/deactivation of a local `TaskScheduler` |
| `ExternalSchedulerAdapter` | `external_scheduler_adapter.cpp` | Conversion to/from supported external scheduler representations and status-related operations |
| `HybridRetentionManager` | `hybrid_retention_manager.cpp` | Scheduled time-series retention stages |
| `EventTrigger` | `event_trigger.cpp` | Changefeed filtering, condition checks, debounce, and callback circuit-breaker state |
| `TaskAuditManager`, audit event types | `task_audit_manager.cpp`, `task_audit_event.cpp` | Audit/security event recording, queries, serialization, and related configuration |
| `TaskResultStore` | `task_result_store.cpp`, `task_execution_result.cpp` | Result serialization and per-task result storage/retrieval |
| `TaskAnomalyDetector` | `task_anomaly_detector.cpp` | Per-task statistics, anomaly metrics, and optional callback delivery |

These are separate classes and integration surfaces. `DistributedTaskCoordinator`
attaches a shared execution gate to its `TaskScheduler`; immediate, scheduled, and
DAG dispatch are denied while that gate is closed. Standalone schedulers without
an attached coordinator remain local.

## Error and failure behavior

`include/scheduler/scheduler_api_contract.h` declares `SchedulerError` values
0 and 8400–8407. Its `isSchedulerFailClosed()` helper returns true only for
`kCoordinationError` and `kInternalError`. That enum is not a universal return
type for scheduler operations: for example, `executeTaskNow` reports authorization,
rate-limit, and missing-task failures as JSON, while registration validation can
throw exceptions.

| Path | Verified behavior |
|---|---|
| Task registration | Checks `task:register`, validates applicable task fields, and can throw for invalid input. Conflicting re-registration is rejected. |
| Immediate execution | Checks `task:execute`, applies the execution rate limiter, looks up the task, serializes same-task execution with a per-task lock, and checks the distributed execution gate before dispatch. |
| Result retention | `TaskResultStore::store` returns `kRetentionLimitExceeded` when the configured positive per-task cap is already full. The post-write prune is a safety net, not the normal full-capacity policy. |
| Event conditions | Construction and updates reject malformed predicates, unknown fields, and unsupported operators. Runtime parser/evaluator errors reject the event. |
| Anomaly reporting | The detector returns metrics and can queue callbacks when configured. `TaskAuditManager` also has a separately configured anomaly callback. Neither is represented by a universal `kAnomalyDetected` execution result. |

Distributed dispatch is denied when the shared coordinator gate is closed. The
gate is lifetime-safe and is closed on leadership loss, heartbeat failure, and
coordinator shutdown; standalone scheduler instances have no distributed gate.

## Concurrency and ownership

The headers declare synchronization primitives for individual components, and
the coordinator documents non-owning pointers to its scheduler and distributed
coordinator. This does not establish one global lock hierarchy or prove that all
public methods across all classes are thread-safe. Consult the class declaration
and implementation before adding cross-component locking or making ownership
assumptions.

## Build and test registration

- The main and modular CMake source lists guard the scheduler implementation
  files with `THEMIS_ENABLE_HTTP_SERVER`; `task_execution_result.cpp` is an
  exception, listed unconditionally in `cmake/ModularBuild.cmake`.
- `tests/scheduler/CMakeLists.txt` discovers `test_scheduler_*.cpp` and registers
  focused module tests. Several registered tests are scaffolds: the coordination
  failure fixture skips, and the burst/retention test contains simulated or
  placeholder cases.
- `benchmarks/scheduler/CMakeLists.txt` discovers `bench_*.cpp` when
  `THEMIS_BUILD_BENCHMARKS` is enabled. Some benchmark files measure stubs or
  generic thread-pool behavior rather than the production `TaskScheduler`.

See [the module overview](README.md), [API headers](../../include/scheduler/README.md),
[roadmap](ROADMAP.md), [security notes](SECURITY.md), and
[performance mapping](PERFORMANCE_EXPECTATIONS.md).
