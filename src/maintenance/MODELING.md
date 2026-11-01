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

# Maintenance Scheduling and Execution Model

## Purpose and scope

This document describes the deterministic runtime model implemented by
`src/maintenance`. It is for developers changing schedule validation, task
ordering, job state, or persistence. It describes no statistical model or
learned maintenance policy.

## Schedule model

`MaintenanceScheduleEntry` is the schedule definition. It combines identity,
frequency or cron expression, an ordered task list, optional dependency edges,
maintenance-window settings, tenant policy, and runtime timestamps. The
orchestrator assigns the ID and creation/update timestamps at creation; callers
must not treat runtime fields supplied on create as authoritative.

For built-in frequencies, the orchestrator derives cron from the schedule
frequency and window start hour. A custom frequency requires a non-empty cron
expression. Validation also requires a non-empty name, at least one task, and
window hours in `[0, 23]`.

When `task_dependencies` is empty, the task vector is the execution order.
Otherwise, dependencies form a directed acyclic graph: each referenced task
must also appear in `tasks`, cycles are rejected, and a stable topological sort
preserves original list order among tasks that are equally eligible.

## Execution and state transitions

The job lifecycle is represented by `MaintenanceJobState`:

```text
PENDING -> RUNNING -> SUCCEEDED
                   -> FAILED
                   -> CANCELLED
                   -> SKIPPED
```

Recurring invocations originate from `TaskScheduler`; `triggerNow` creates an
ad-hoc job and dispatches it asynchronously. Before running tasks, the
orchestrator checks the in-flight guard, optional distributed lock, tenant
concurrency quota, and effective UTC maintenance window. A forced trigger
bypasses the window check; the HTTP route requires the additional
`maintenance:admin` scope for that option.

Each task is dispatched in the resolved order. `halt_on_task_failure` stops
later tasks after a task returns `FAILED`. A cancellation is checked between
tasks; it does not undo finished work and does not provide a compensating
transaction for handler side effects. Schedule deletion likewise does not
cancel a job already started from that schedule.

The aggregate job is marked failed when a task reports `FAILED`. A task skipped
for lack of a registered handler has an explicit `SKIPPED_NO_HANDLER` dispatch
outcome, but the current aggregate loop does not treat that task state as a
failure. Operators should inspect task/dispatch outcomes as well as the final
job state.

## Persistence model

When an `IStorageEngine` is supplied, schedule mutations are written through
using keys prefixed with `maint_sched::`. Create, update, patch, and delete
persist before publishing the new in-memory schedule state; a failed write
returns an error and leaves the prior state unchanged. `start()` loads stored
entries before re-registering enabled schedules. The store skips malformed
records, logs them, retains valid records, and returns a hard error if the
prefix scan itself fails.

Without storage, schedule state is process-local. Do not interpret a successful
in-memory mutation as restart-safe.

## Evaluation and limits

The model is validated through schedule CRUD, JSON round-trip, dependency-cycle,
execution-order, persistence, model-based churn fixtures, and dispatch-diagnostic tests. See
[`RESEARCH.md`](RESEARCH.md) for the evidence map and
[`tests/maintenance/test_maintenance_hardening_phase23_focused.cpp`](../../tests/maintenance/test_maintenance_hardening_phase23_focused.cpp)
for serialization and task-order coverage.

Relevant bounds and policies are implementation-defined: completed-job history
is pruned after 24 hours, the dispatch-outcome ring buffer defaults to 256
entries, and `max_schedule_changes_per_interval` is enforced by schedule CRUD
in a fixed 60-second in-memory window. Existing-schedule attempts consume a
slot before persistence; failed creates discard their new-ID counter, successful
deletes clear it, and all counters reset after restart. These bounds do not make
task side effects transactional or guarantee that a handler will complete within
a particular latency.

## Sources

- [`maintenance_schedule.h`](../../include/maintenance/maintenance_schedule.h)
- [`maintenance_task.h`](../../include/maintenance/maintenance_task.h)
- [`database_maintenance_orchestrator.h`](../../include/maintenance/database_maintenance_orchestrator.h)
- [`database_maintenance_orchestrator.cpp`](database_maintenance_orchestrator.cpp)
- [`maintenance_schedule_store.cpp`](maintenance_schedule_store.cpp)
