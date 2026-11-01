---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Maintenance Runbook

## Purpose and scope

Use this runbook for scheduled-job, task-handler, schedule-persistence, and maintenance-health incidents. The module orchestrates handlers; it does not make a multi-task schedule transactional.

## Initial triage

1. Read `GET /api/v1/maintenance/status` and `/api/v1/maintenance/health`; record overall status, active jobs, failed jobs in the last 24 hours, module signals, and recent dispatch outcomes.
2. Read the affected job and schedule using the maintenance API. Capture task types, dependencies, tenant/window settings, `forced`, last run, and the error message before changing state.
3. Check `GET /api/v1/maintenance/task-handlers` for each task type. A missing handler is a configuration/wiring issue, not a reason to bypass dispatch checks.
4. Check storage and scheduler logs if status indicates persistence or registration failure.

## Incident procedures

### Job failed or was skipped

Use `recent_dispatch_outcomes` and the job state to distinguish:

- `skipped_no_handler`: register/restore the correct handler; do not replace it with a no-op.
- `skipped_concurrent`: confirm the current run is healthy; the orchestrator guards against schedule re-entry.
- Persistence errors: schedule mutations return an error and startup scan errors
  are logged. `FAILED_PERSISTENCE` is declared as an outcome type but is not
  emitted into the current dispatch-outcome ring; diagnose from the API result or
  startup logs instead.
- `failed_dispatch`: inspect the handler error and any already-completed tasks.
- `skipped` due to tenant quota or maintenance window: confirm the applicable UTC window and concurrency settings.

Correct the cause, then use the normal schedule or an approved `triggerNow`. A forced trigger bypasses the maintenance window and requires `maintenance:admin`; use it only with explicit approval and an audit record.

### Schedule disappeared or differs after restart

1. Confirm the orchestrator was constructed with an `IStorageEngine`; a null storage pointer means in-memory-only schedules and restart loss is expected.
2. Review `MaintenanceScheduleStore::loadAll` warnings for malformed JSON or key/payload ID mismatches. Corrupt records are skipped while valid entries are loaded.
3. Compare the recovered schedule count and IDs with the expected inventory. Preserve the underlying store before repair; do not delete suspect records as a first step.
4. Restore the schedule from an approved configuration source and verify it through the API.

### Cancel, stop, or delete during a run

- `cancelJob` signals the running job to abort, but completed tasks are not rolled back.
- `stop()` deregisters schedules and lets running jobs finish.
- Deleting a schedule does not cancel jobs already spawned by it.

Before any retry, identify completed side effects and confirm the handler is safe to rerun. Do not describe cancellation as transaction rollback.

## Recovery verification

Confirm the schedule has its intended persisted definition, required handlers are registered, health probes are no worse than expected, and the next job completes with a successful dispatch outcome. Verify `maintenance_jobs_failed_total` and task failure counters stop increasing; these are cumulative counters, so compare deltas rather than expecting them to return to zero.

## Rollback and escalation

For a bad schedule change, restore the last approved schedule definition through the API and verify persistence before re-enabling it. Avoid broad schedule deletion or store rewrites. Escalate to the maintenance owner and storage owner for persistence corruption, unexpected missing schedules, repeated failed handlers, or uncertainty about partial task effects. Escalate to the task's owning module when handler behavior or data integrity is implicated.

## References

- [OPERATIONS.md](OPERATIONS.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- `include/maintenance/database_maintenance_orchestrator.h`
- `include/maintenance/maintenance_health_report.h`
- `src/maintenance/database_maintenance_orchestrator.cpp`
- `src/maintenance/maintenance_schedule_store.cpp`
