---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Maintenance Research Notes

## Purpose and scope

This note records the evidence and evaluation approach for the maintenance
orchestrator. It is for maintainers deciding whether scheduling, persistence,
and dispatch behavior is sufficiently deterministic and observable. The module
is a runtime coordinator, not a research or machine-learning subsystem; it does
not train or evaluate a predictive maintenance model.

## Questions and assumptions

- Does a schedule mutation either persist and become visible, or return an
  explicit storage error without changing the in-memory schedule?
- Does execution preserve declared task order, enforce declared dependencies,
  and avoid re-entering the same schedule concurrently?
- Can an operator distinguish handler absence, dispatch failure, persistence
  failure, and intentional skips from job state and health diagnostics?
- The `TaskScheduler` supplies recurring triggers. Task-specific work is
  performed directly for selected index operations or by registered handlers.
- Persistence is optional. A null storage engine makes schedule state
  in-memory-only; it is not evidence of durable recovery.

## Evidence and evaluation method

The primary evidence is the implementation and focused tests, not a synthetic
dataset. Review changes against:

| Property | Source evidence | Evaluation |
|---|---|---|
| Schedule validation and mutation | `database_maintenance_orchestrator.cpp`: `validateEntry`, `createSchedule`, `updateSchedule`, `patchSchedule`, `deleteSchedule` | Reject invalid names, empty task lists, missing custom cron expressions, out-of-range hours, and invalid dependency graphs. Inject a persistence failure and verify the old in-memory entry remains. |
| Dependency order | `resolveTaskExecutionOrder` in `database_maintenance_orchestrator.cpp` | Verify positional order without dependencies, stable topological order with dependencies, and rejection of missing references and cycles. |
| Persistence recovery | `maintenance_schedule_store.cpp` and `MaintenanceScheduleStoreTest.LoadAll_SkipsCorruptEntry` in `tests/test_database_maintenance_orchestrator.cpp` | Save and reload valid schedules; confirm corrupt records are skipped while valid entries remain available; distinguish scan errors from malformed records. |
| Dispatch outcomes | `maintenance_health_report.h` and `test_maintenance_diagnostics_focused.cpp` | Verify outcome serialization, the bounded 256-entry FIFO ring, and visibility of failure/skip categories. |
| Churn and concurrency | `test_maintenance_churn_hardening_focused.cpp` | Exercise configured mutation limits and duplicate in-flight schedule dispatch. |

Focused regression coverage is in
[`tests/maintenance/`](../../tests/maintenance/), with production store cases in
[`tests/test_database_maintenance_orchestrator.cpp`](../../tests/test_database_maintenance_orchestrator.cpp).
These files describe available test evidence; their presence alone does not
assert that a particular build or test run passed.

Some stress tests in `tests/maintenance/test_maintenance_stress_focused.cpp`
exercise a mock store with a stricter all-or-nothing corruption policy. That
fixture is not the production `MaintenanceScheduleStore`; use the production
store implementation and its focused test for the actual partial-recovery
behavior.

## Measurements and trade-offs

The module does not publish a measured universal latency or throughput result.
Release comparisons belong to the mapped cases and baselines in
[`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md). Those expectations
include p99 limits for the in-flight guard, persistence/reload, ring-buffer
write, distributed-lock dispatch, and schedule listing. Compare results only
under the recorded release profile and representative hardware; do not infer a
benchmark result from a test name or source-level threshold.

Key trade-offs are:

- Optional storage permits lightweight in-memory deployments but loses schedules
  at restart.
- The task handler boundary keeps module dependencies separate, but a missing
  handler means that task is skipped rather than executed.
- A schedule's task list is not a transaction. Completed task side effects are
  not compensated when a later task fails or the job is cancelled.
- Health probes are synchronous, so probe implementations must be cheap and
  non-blocking.

## Reproducibility

For behavior changes, add or extend focused tests in `tests/maintenance/` and
run the corresponding maintenance test target from the configured build. For
performance changes, run the mapped release benchmark and preserve its
machine-readable result and baseline comparison. Do not report a performance
claim without that run evidence.

## Related documentation

- [Model and execution semantics](MODELING.md)
- [Implementation contract](IMPLEMENTATION.md)
- [Performance expectations](PERFORMANCE_EXPECTATIONS.md)
- [Architecture and failure boundaries](ARCHITECTURE.md)
