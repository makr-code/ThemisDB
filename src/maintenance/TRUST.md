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

# Maintenance Trust Boundaries

## Purpose and trust model

This document is for security reviewers and operators assessing maintenance
schedule changes and task execution. A schedule is privileged operational
configuration: it selects work, its order, its maintenance window, and whether a
run can bypass that window. The orchestrator coordinates execution; it does not
make arbitrary handler code safe or roll back its effects.

## Trust boundaries and controls

| Boundary | Risk | Implemented control and remaining responsibility |
|---|---|---|
| HTTP caller → maintenance routes | Unauthorized reads, schedule changes, or job control | `http_server.cpp` checks `maintenance:read` for observations, `maintenance:write` for mutations/runs/cancellation, and additionally `maintenance:admin` for a forced run. Keep these route checks in the server layer. |
| JSON request → schedule state | Malformed names, tasks, windows, cron, or dependency graphs | The handler parses JSON and the orchestrator validates structural invariants, dependency references, and cycles before mutation. Validation is not a substitute for authorization. |
| Orchestrator → task handler | Missing, faulty, non-idempotent, or concurrently called handler | Only registered handlers are invoked; missing handlers produce an explicit skip. Implementations must be thread-safe and must define their own idempotency and side-effect boundaries. |
| Orchestrator → distributed lock | Duplicate execution across nodes | Locking is optional. Without an injected lock, there is no cross-node coordination. `InProcessDistributedLock` is process-local and is not a cluster lock. |
| Orchestrator → storage engine | Loss, corruption, or disclosure of persisted schedules | The module writes schedule JSON through `IStorageEngine` and reports storage errors. Storage access control, encryption-at-rest, backup, and retention are owned by the configured storage deployment. |
| Runtime → logs and audit | Sensitive values copied into diagnostics | The optional audit logger records schedule events; patch events include the submitted patch JSON. Do not put credentials, tokens, or other secrets in schedule fields or patch requests. Restrict and retain audit/log data according to deployment policy. |

The `MaintenanceApiHandler` class itself contains no authentication layer.
Non-HTTP callers must enforce equivalent authorization before calling it or the
orchestrator directly.

## Privileged actions and partial effects

- A forced `triggerNow` bypasses the UTC maintenance window and is explicitly
  marked in the job and audit event. Require the additional admin scope, approval
  appropriate to the deployment, and post-run review.
- `cancelJob` signals cancellation; it does not reverse tasks already completed
  and cannot interrupt a handler that does not return control to the
  orchestrator.
- `stop()` deregisters scheduled work but allows already-running jobs to finish.
- Deleting or disabling a schedule is not cancellation of jobs it already
  started.
- Schedules with `halt_on_task_failure = false` may continue after a task fails.
  Review task dependencies and partial side effects before retrying.
- Corrupt records are skipped individually during prefix load while valid
  schedules are retained. A partial restore must be treated as an incident to
  investigate, not as proof that the schedule inventory is complete.

## Security review and incident handling

For an unexpected run, preserve the schedule and job response, recent health
report outcomes, relevant audit/log records, and handler identity before
changing configuration. If recurring work is unsafe, use an authorized
schedule-disable or equivalent deployment control, then assess already-running
jobs and task side effects separately. Do not delete persisted records as a
first response to corruption.

Escalate unauthorized access or possible secret exposure through the
repository-level security reporting process. Escalate data-integrity concerns
to the owner of the affected task handler and storage owner; maintenance
diagnostics cannot establish whether a delegated operation's side effects were
reversed.

## Sources

- [`http_server.cpp`](../../src/server/http_server.cpp)
- [`maintenance_api_handler.cpp`](../../src/server/maintenance_api_handler.cpp)
- [`database_maintenance_orchestrator.h`](../../include/maintenance/database_maintenance_orchestrator.h)
- [`database_maintenance_orchestrator.cpp`](database_maintenance_orchestrator.cpp)
- [`maintenance_schedule_store.cpp`](maintenance_schedule_store.cpp)
- [Existing module security summary](SECURITY.md)
- [Operations](OPERATIONS.md) · [Runbook](RUNBOOK.md)
