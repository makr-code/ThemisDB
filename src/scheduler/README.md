---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: module-behavior
Canonical references: include/scheduler/*.h; src/scheduler/*.cpp; tests/scheduler/CMakeLists.txt; benchmarks/scheduler/CMakeLists.txt
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler module

## Purpose and boundaries

The scheduler code provides task registration and execution APIs, plus separate
components for changefeed triggers, distributed leadership coordination,
external-scheduler conversion, time-series retention, audit events, result
storage, and anomaly metrics. The declarations in `include/scheduler/` are the
API reference; this page is an implementation-oriented index, not a claim that
every optional integration is enabled or production-certified.

## Source map

| Component | Header | Implementation |
|---|---|---|
| Task lifecycle, execution, scheduling, stats | `task_scheduler.h` | `task_scheduler.cpp` |
| Distributed leadership / local scheduler activation | `distributed_task_coordinator.h` | `distributed_task_coordinator.cpp` |
| External scheduler representations and status operations | `external_scheduler_adapter.h` | `external_scheduler_adapter.cpp` |
| Hybrid retention stages | `hybrid_retention_manager.h` | `hybrid_retention_manager.cpp` |
| CDC event matching and callbacks | `event_trigger.h` | `event_trigger.cpp` |
| Audit/security event model and manager | `task_audit_event.h`, `task_audit_manager.h` | `task_audit_event.cpp`, `task_audit_manager.cpp` |
| Execution result persistence | `task_result_store.h` | `task_result_store.cpp`, `task_execution_result.cpp` |
| Execution anomaly metrics | `task_anomaly_detector.h` | `task_anomaly_detector.cpp` |

`TaskScheduler` accepts AQL or registered-function tasks and declares interval,
cron, CDC-event, and manual trigger types. Registration validates task-specific
configuration; an enabled CDC task requires a non-null `Changefeed`. The
`WEBHOOK` trigger value is declared in the header but is not documented here as
an implemented trigger path.

## Important runtime behavior

- `registerTask`, `executeTaskNow`, and function registration perform permission
  checks. The permission helpers allow access when no request context is set;
  callers must supply the intended `RequestContext` where authorization matters.
- `executeTaskNow` also applies a rate limit and serializes execution per task.
  Its distributed-coordinator health check is commented out and is not an
  implemented fail-closed check.
- `TaskResultStore` rejects a write with `kRetentionLimitExceeded` if the
  positive configured cap is already full. Do not describe the ordinary
  capacity behavior as eviction.
- Event-trigger condition handling is not uniformly fail-closed: structural
  validation can reject an event, while an unknown operator is logged and
  treated as a match. See [architecture notes](ARCHITECTURE.md).
- Audit logging, anomaly detection, result storage, and sandboxed execution are
  configurable; several are disabled by default in `TaskScheduler::Config`.

## Build, tests, and benchmarks

`CMakePresets.json` defines the `linux-release` configure/build preset; the
module's CMake registration itself is conditional on
`THEMIS_ENABLE_HTTP_SERVER`. Focused tests are discovered from
`tests/scheduler/test_scheduler_*.cpp`; benchmarks are discovered from
`benchmarks/scheduler/bench_*.cpp` when `THEMIS_BUILD_BENCHMARKS` is enabled.

The current focused test set includes concurrency-edge, contract-enum,
coordination-failure, high-cardinality, integration, and burst/retention files.
Presence/registration is not evidence that every scenario exercises production
behavior: the coordination fixture skips, and some stress cases are simulations
or placeholders. Benchmark mappings and their limits are listed in
[performance expectations](PERFORMANCE_EXPECTATIONS.md).

## Further reading

- [Architecture and failure semantics](ARCHITECTURE.md)
- [Security boundaries](SECURITY.md)
- [Production-operation gaps](PRODUCTION_REQUIREMENTS.md)
- [Roadmap](ROADMAP.md) and [future work](FUTURE_ENHANCEMENTS.md)
- [Public-header index](../../include/scheduler/README.md)
