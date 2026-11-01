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

# Maintenance Implementation Guide

## Purpose and ownership

This guide is for developers implementing maintenance tasks or changing the
orchestrator. Public declarations live under `include/maintenance/`; runtime
orchestration and schedule persistence live under `src/maintenance/`. HTTP
routing and access checks are owned by `src/server/http_server.cpp`; request
translation is in `src/server/maintenance_api_handler.cpp`.

## Construction and startup

Construct `DatabaseMaintenanceOrchestrator` with a live `TaskScheduler*`.
Optional dependencies are the index-maintenance manager, audit logger, and
storage engine. The scheduler and non-owning storage pointer must outlive the
orchestrator. A null storage engine is explicitly in-memory-only.

At startup:

1. Register task handlers and health probes before schedules can dispatch.
   Handler implementations may be called from execution threads and are
   expected to be thread-safe.
2. Optionally call `registerDefaultMaintenanceSetup`. It attempts to create daily,
   weekly, monthly, and quarterly schedules disabled; the current implementation
   ignores individual create errors. Operators must explicitly enable any
   defaults they intend to run.
3. Call `start()`. It reloads persisted schedules when a store exists, then
   re-registers enabled entries. A failed storage scan is logged and startup
   continues with existing in-memory schedules; a null scheduler returns an
   error.
4. On orderly shutdown, call `stop()`. It deregisters schedules; already-running
   jobs are allowed to finish.

## Schedule mutation invariants

`createSchedule`, `updateSchedule`, and `patchSchedule` validate a non-empty
name, a non-empty task list, custom cron presence, hour values in `[0, 23]`,
and dependency references/cycles. Create assigns identity and timestamps and
derives cron for built-in frequencies. Update is full replacement of mutable
fields; patch changes only supplied fields. `created_at_ms` and `created_by`
are preserved on update.

When persistence is enabled, mutation writes occur before the in-memory state is
published. If storage fails, return the error and do not update the in-memory
entry. Delete also removes the durable record before erasing the in-memory
entry. Preserve this order when changing the mutation code; otherwise a restart
can resurrect or lose a schedule.

## Task execution contract

Task execution defaults to the order in `tasks`. If dependencies are present,
`resolveTaskExecutionOrder` validates the graph and computes a stable
topological order. Invalid references and cycles are rejected before execution.
Do not silently substitute an execution order when graph validation fails.

`executeTask` handles selected index operations through
`IndexMaintenanceManager`; other task types use registered
`IMaintenanceTaskHandler` implementations. Register a real handler for each
delegated task type. Missing handlers generate a `SKIPPED_NO_HANDLER` diagnostic
instead of invoking a no-op implementation. A task error marks that task failed;
`halt_on_task_failure` controls whether subsequent tasks run.

The runtime may skip a dispatch because a schedule is already in flight, a
distributed lock is held, a tenant concurrency quota is full, or the UTC
maintenance window is closed. A forced run bypasses only the window check; it
does not bypass missing handlers, schedule validation, or the in-flight guard.
Cancellation is observed between tasks and cannot undo completed side effects.

## Persistence and diagnostics

`MaintenanceScheduleStore` stores JSON at `maint_sched::<schedule-id>`. Its
engine pointer is non-owning and the store has no internal synchronization;
coordinate shared schedule state with the orchestrator's lock. A prefix scan
can succeed while malformed records are skipped. Diagnose scan failures
separately from per-record corruption, and reconcile skipped records instead of
assuming every schedule was restored.

Use the health report and recent dispatch outcomes to distinguish successful,
skipped, and failed execution paths. The ring buffer is bounded (default 256)
and evicts the oldest outcomes. Metrics are counters/histograms, not retained
per-job history; completed job records are pruned after 24 hours by default.

## Validation expectations

When changing implementation, extend the focused test in
[`tests/maintenance/`](../../tests/maintenance/) that matches the contract:

- CRUD, JSON conversion, windows, cancellation, and handlers:
  [`tests/database/test_database_maintenance_orchestrator.cpp`](../../tests/database/test_database_maintenance_orchestrator.cpp)
- DAG, defaults, and serialization:
  [`test_maintenance_hardening_phase23_focused.cpp`](../../tests/maintenance/test_maintenance_hardening_phase23_focused.cpp)
- churn and concurrent invocation:
  [`test_maintenance_churn_hardening_focused.cpp`](../../tests/maintenance/test_maintenance_churn_hardening_focused.cpp)
- mock persistence lifecycle and stress (not the production store's corruption contract):
  [`test_maintenance_stress_focused.cpp`](../../tests/maintenance/test_maintenance_stress_focused.cpp)
- health and dispatch outcome diagnostics:
  [`test_maintenance_diagnostics_focused.cpp`](../../tests/maintenance/test_maintenance_diagnostics_focused.cpp)

For performance-sensitive changes, use the mapped release benchmarks and
thresholds in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md).
For production corruption recovery, use `MaintenanceScheduleStoreTest` in
[`tests/database/test_database_maintenance_orchestrator.cpp`](../../tests/database/test_database_maintenance_orchestrator.cpp);
the separate stress fixture uses a mock store with different corruption
semantics.

## Related contracts

- [Public HTTP and C++ API](API.md)
- [Module integration guide](MODULE_GUIDE.md)
- [Architecture and failure boundaries](ARCHITECTURE.md)
- [Trust boundaries](TRUST.md)

`maintenance_api_contract.h` now identifies its taxonomy, descriptor, health
snapshot, and fail-closed predicate as compatibility helpers for tests and
benchmarks, not runtime guarantees. The runtime contract remains
`MaintenanceScheduleEntry`, `MaintenanceHealthReport`, and repository
`Result<T>`/`Error`.
