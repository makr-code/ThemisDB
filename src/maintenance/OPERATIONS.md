---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-01"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Maintenance Operations Guide

## Purpose

This guide covers routine monitoring and safe controls for `DatabaseMaintenanceOrchestrator`. Use [RUNBOOK.md](RUNBOOK.md) for incident response. Routes below are defined by the orchestrator API contract; use only the authenticated server routes enabled in the deployment.

## Health and metrics

Check `GET /api/v1/maintenance/status`, `GET /api/v1/maintenance/health`, `GET /api/v1/maintenance/jobs`, and `GET /api/v1/maintenance/task-handlers`. The health report provides `overall_status`, module health signals, active/total/enabled schedules, success and failed jobs over 24 hours, and a bounded ring buffer of recent dispatch outcomes (default capacity 256).

| Metric | Operational use |
|---|---|
| `maintenance_jobs_triggered_total`, `maintenance_jobs_succeeded_total`, `maintenance_jobs_failed_total`, `maintenance_jobs_skipped_total`, `maintenance_jobs_cancelled_total` | Compare counter rates/deltas and correlate with job state and dispatch outcome. |
| `maintenance_tasks_succeeded_total`, `maintenance_tasks_failed_total` | Identify failing task types; correlate with handler result and schedule ID. |
| `maintenance_job_duration_ms`, `maintenance_task_duration_ms` | Detect duration changes against workload-specific baselines. |
| `maintenance_schedules_created_total`, `maintenance_schedules_deleted_total` | Detect schedule churn; correlate with authorized API changes and audit events. |

Metric identifiers are emitted through `MetricsCollector`; the module does not define alert thresholds. Alert on sustained failed-job/task increases, unexpected skips, stale health reports, or jobs exceeding deployment-specific duration budgets.

## Routine operating checks

- Verify registered task handlers before enabling schedules that use them.
- Confirm schedule persistence is configured for restart survival; storage is optional and null means in-memory-only.
- Review recent `FAILED_DISPATCH`, `SKIPPED_NO_HANDLER`, and `SKIPPED_CONCURRENT` dispatch outcomes. The enum also declares `FAILED_PERSISTENCE`, but the current implementation does not record that outcome; persistence errors are returned by schedule mutations or logged during startup.
- Check tenant UTC maintenance windows, max concurrent job quotas, and distributed-lock availability.
- Inspect audit events for forced runs and CRUD operations. The
  `max_schedule_changes_per_interval` cap applies per schedule in a fixed
  60-second in-memory window. Existing-schedule attempts consume a slot before
  storage writes; the counter resets after restart and is not shared across
  instances. Use deployment-side controls if limits must survive restarts or
  apply across instances.

## Safe controls and boundaries

- Prefer a normal scheduled run or `triggerNow` without `force`. A forced run bypasses the UTC window and requires `maintenance:admin`.
- `cancelJob` does not undo tasks already completed. `stop()` allows active jobs to finish; deleting a schedule does not cancel its active jobs.
- A handler failure may leave earlier tasks' side effects in place; there is no cross-task transaction or automatic compensating rollback.
- On load, corrupt persisted schedule entries are skipped while healthy entries are retained. Investigate each warning and reconcile the inventory rather than assuming a clean full restore.
- Completed job history is retained for 24 hours by default; export evidence earlier if a longer incident record is needed.

## References

- [RUNBOOK.md](RUNBOOK.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- `include/maintenance/database_maintenance_orchestrator.h`
- `include/maintenance/maintenance_health_report.h`
