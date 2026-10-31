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
SOT domain: api-contract
Canonical references: include/scheduler/*.h; src/scheduler/README.md; src/scheduler/ARCHITECTURE.md
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler public headers

This page is an index to the declarations under `include/scheduler/`. The
headers and implementations are authoritative; this summary does not promise
ABI stability or imply that optional integrations are enabled in every build.
For implementation limitations, see
[src/scheduler/README.md](../../src/scheduler/README.md).

## Entry points

| Header | Public types / role |
|---|---|
| [`task_scheduler.h`](task_scheduler.h) | `ScheduledTask` and `TaskScheduler`; task configuration, registration/management, immediate and DAG execution, stats, metrics, and optional integrations |
| [`scheduler_api_contract.h`](scheduler_api_contract.h) | `SchedulerError`, scheduler constants, and `isSchedulerFailClosed()` |
| [`distributed_task_coordinator.h`](distributed_task_coordinator.h) | `DistributedTaskCoordinator`; coordinates a local `TaskScheduler` with a `sharding::DistributedCoordinator` |
| [`event_trigger.h`](event_trigger.h) | `CDCTriggerConfig`, `EventTrigger`, and `EventTriggerManager` for changefeed-backed triggers |
| [`external_scheduler_adapter.h`](external_scheduler_adapter.h) | `ExternalSchedulerAdapter`, platform/config types for Kubernetes CronJob and Airflow conversion/status operations |
| [`hybrid_retention_manager.h`](hybrid_retention_manager.h) | Three configured retention stages and `HybridRetentionManager` lifecycle/manual execution/status APIs |
| [`task_audit_event.h`](task_audit_event.h) | Task audit/security event types, resource/anomaly payloads, and serialization helpers |
| [`task_audit_manager.h`](task_audit_manager.h) | Audit configuration/query/export types and `TaskAuditManager` |
| [`task_result_store.h`](task_result_store.h) | `TaskExecutionResult` and `TaskResultStore` APIs |
| [`task_anomaly_detector.h`](task_anomaly_detector.h) | `AnomalyDetectorConfig`, statistics, and `TaskAnomalyDetector` |

Most core task-scheduler types are in namespace `themis`; the audit, result,
anomaly, external-adapter, and error-contract types use
`themis::scheduler`. See each header for exact namespace and signature.

## TaskScheduler quick reference

`TaskScheduler` is constructed with a `QueryEngine*` and `Config`; optional
constructor parameters include a `Changefeed*`, audit logger, and result
storage pointer. The query engine must be non-null. CDC tasks need a changefeed,
and result storage is needed when that feature is enabled.

Public methods include:

- lifecycle: `start()`, `stop()`, `isRunning()`;
- task operations: `registerTask`, `unregisterTask`, `enableTask`,
  `disableTask`, `updateTask`;
- execution: `executeTaskNow` and `executeDAG`;
- function registration: `registerFunction`, `unregisterFunction`;
- observation: `getStats`, `exportMetrics`, `listTasks`, `getTask`;
- optional audit/result/alert accessors and dynamic-concurrency accessors.

Refer to [`task_scheduler.h`](task_scheduler.h) for exact arguments, defaults,
preconditions, and result types. This guide intentionally does not copy an
abbreviated class definition that could drift from the API.

## API and security cautions

- `SchedulerError` is not a universal result type: some public operations throw
  or return JSON errors instead.
- The `WEBHOOK` task trigger enum value is declared in `task_scheduler.h`; do
  not infer a working webhook integration from the declaration alone.
- The permission helpers allow when no request context is present. The caller
  must establish the intended request/security context.
- The distributed coordinator health-check code in `executeTaskNow` is
  commented out in the implementation.
- A declared configurable feature is not necessarily enabled by default.
  Consult `TaskScheduler::Config` and the implementation for defaults and
  requirements.

## Build and tests

The repository defines the `linux-release` preset in `CMakePresets.json`.
Scheduler source registration is conditional on `THEMIS_ENABLE_HTTP_SERVER`.
Focused tests under `tests/scheduler/` and benchmarks under
`benchmarks/scheduler/` are registered by their local CMake files; some tests
are placeholders or skipped and some benchmarks use stubs. See the
[test/benchmark audit](../../src/scheduler/AUDIT.md).

## Related documentation

- [Public-header architecture](ARCHITECTURE.md)
- [Header roadmap](ROADMAP.md)
- [Header future enhancements](FUTURE_ENHANCEMENTS.md)
- [Scheduler implementation overview](../../src/scheduler/README.md)
- [Scheduler implementation architecture](../../src/scheduler/ARCHITECTURE.md)
- [Scheduler security notes](../../src/scheduler/SECURITY.md)
