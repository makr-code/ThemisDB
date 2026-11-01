---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-01"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Maintenance Module Guide

## Audience and module boundary

Use this guide when wiring maintenance into a ThemisDB service or adding a
maintenance task. The module owns schedule lifecycle, dispatch coordination,
schedule persistence, job history, and health aggregation. It does not own the
implementation semantics of every dispatched task.

| Path | Responsibility |
|---|---|
| `include/maintenance/` | Public schedule, task, handler, lock, and health contracts |
| `src/maintenance/` | Orchestration, persistence adapter, and default schedule registration |
| `include/server/maintenance_api_handler.h` and `src/server/maintenance_api_handler.cpp` | Translate HTTP-level maintenance operations to typed orchestrator calls |
| `src/server/http_server.cpp` | Route registration, authorization checks, and HTTP status mapping |
| `tests/maintenance/` and `tests/database/test_database_maintenance_orchestrator.cpp` | Focused contract, regression, and stress tests |

## Integrating a task handler

1. Select the existing `MaintenanceTaskType` that matches the operation. Do not
   create a second task token for the same operation.
2. Implement `IMaintenanceTaskHandler::execute(job_id, task_type)` and return a
   `Result<std::string>` with a diagnostic summary on success or an error on
   failure.
3. Make the handler safe for concurrent calls and for retries. The orchestrator
   can invoke handlers from execution threads; it does not provide a transaction
   or compensation for task side effects.
4. Register the handler during service startup, before schedules are enabled.
   Verify it with `GET /api/v1/maintenance/task-handlers`.
5. Add tests for success, failure, missing dependencies, and any idempotency
   expectations specific to the task.

Some task types are handled directly by the orchestrator through
`IndexMaintenanceManager`; delegated task types require a registered handler.
Check the `executeTask` switch in `database_maintenance_orchestrator.cpp`
before adding a new handler, and never rely on the missing-handler skip as
successful execution.

## Starting and configuring schedules

Construct the orchestrator with a live `TaskScheduler*`. Pass a non-null
`IStorageEngine*` when schedule definitions must survive restarts; both
dependencies are non-owning and must outlive the orchestrator. Register
handlers, health probes, and optional distributed-lock implementation before
calling `start()`.

Default schedules registered by `registerDefaultMaintenanceSetup` are disabled.
Review each schedule's task handlers, UTC window, tenant, task dependency graph,
and failure policy before enabling it. For multi-tenant work, set explicit
window and concurrency overrides with `setTenantMaintenanceConfig`. For
multi-node work, inject a genuinely shared `IDistributedLock`; the provided
`InProcessDistributedLock` coordinates only one process.

## Development and validation path

- Read [architecture](ARCHITECTURE.md) for flow and failure boundaries.
- Read [implementation](IMPLEMENTATION.md) for mutation and dispatch invariants.
- Read [API](API.md) before changing the HTTP or C++ surface.
- Read [operations](OPERATIONS.md) and [runbook](RUNBOOK.md) before changing
  health signals, recovery semantics, or operator controls.
- Add focused coverage in `tests/maintenance/` for the changed contract, then
  run the repository's configured maintenance test target.
- For performance-sensitive changes, use the mapped benchmarks in
  [performance expectations](PERFORMANCE_EXPECTATIONS.md); report actual run
  results, not just threshold definitions.

## Known integration limits

- Null storage means schedule definitions are lost on process restart.
- The health report calls registered probes synchronously; keep probes cheap
  and non-blocking.
- A forced run bypasses the window but requires `maintenance:admin` at the HTTP
  layer. It does not imply a rollback or bypass the other dispatch guards.
- Job history is in memory and completed jobs are pruned after 24 hours by
  default. Export incident evidence when longer retention is required.
- `max_schedule_changes_per_interval` is enforced by create/update/patch/delete
  using the existing schedule's limit for mutations after create. Counters are
  in-memory fixed 60-second windows; existing-schedule attempts consume a slot
  before persistence, successful deletes clear their counters, and restart clears
  remaining counts.
- A task skipped for a missing handler can appear in the dispatch diagnostics
  without making the aggregate job state `FAILED`; inspect task-level outcomes.

## Related sources

- [`include/maintenance/README.md`](../../include/maintenance/README.md)
- [`database_maintenance_orchestrator.cpp`](database_maintenance_orchestrator.cpp)
- [`maintenance_schedule_store.cpp`](maintenance_schedule_store.cpp)
- [Trust boundaries](TRUST.md)
- [Research and evaluation](RESEARCH.md)
