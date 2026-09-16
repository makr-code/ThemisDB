# RUNBOOK: Maintenance Module — Operator Remediation Guide

**Module:** `maintenance`
**Wave:** D (Q1 2027)
**Maintainers:** ThemisDB Platform Team
**Last Updated:** 2026-09-16

---

## Overview

This runbook covers operator-critical incident scenarios for the ThemisDB
`maintenance` module. Each scenario includes diagnostic log patterns,
triage steps, and remediation actions.

Log patterns use structured prefixes of the form `[MAINTENANCE:<EVENT>]` and
appear in the application log stream (spdlog, JSON lines format).

---

## Scenario 1 — Vacuum Failed (`[MAINTENANCE:VacuumFailed]`)

### Symptoms
- Log line: `[MAINTENANCE:VacuumFailed] task_id=<id> reason=<reason>`
- Dead rows accumulating; storage usage growing unexpectedly
- `DatabaseMaintenanceOrchestrator` error metrics rising

### Triage
1. Check orchestrator logs for the specific error reason.
2. Verify disk space: `df -h <data_dir>`. Vacuum requires free space ≥ table size × 0.3.
3. Confirm no long-running transactions are blocking vacuum:
   query `pg_stat_activity` (or equivalent).
4. Check `MaintenanceScheduleStore` for stale or conflicting schedules.

### Remediation
- If disk full: free space and re-trigger vacuum via admin API.
- If blocked by long transactions: identify and terminate blocking query.
- If schedule is corrupted: reset via `MaintenanceScheduleStore::resetSchedule(task_id)`.
- If persistent failure: escalate to on-call DBA.

---

## Scenario 2 — Compaction Stall (`[MAINTENANCE:CompactionStall]`)

### Symptoms
- Log line: `[MAINTENANCE:CompactionStall] shard_id=<id> stall_ms=<ms>`
- Read latency degraded on affected shard
- Compaction throughput metric `themis_maintenance_compaction_ops` plateaued

### Triage
1. Identify the stalled shard from the log `shard_id` field.
2. Check I/O utilization: `iostat -x 1 10`.
3. Inspect WAL backlog size for the affected shard.
4. Check for lock contention in the compaction thread.

### Remediation
- If I/O saturated: throttle write workload or move shard to faster storage.
- If WAL backlog: trigger a manual checkpoint and allow compaction to drain.
- If lock contention: restart the compaction thread via admin API:
  `POST /admin/maintenance/restart_compaction?shard_id=<id>`
- Monitor until compaction throughput returns to baseline.

---

## Scenario 3 — Retention Violation (`[MAINTENANCE:RetentionViolation]`)

### Symptoms
- Log line: `[MAINTENANCE:RetentionViolation] record_id=<id> age_ms=<ms> ttl_ms=<ms>`
- Records older than TTL still present in storage
- Compliance alert triggered (if retention policy is regulatory)

### Triage
1. Confirm retention policy TTL values are correctly configured.
2. Check retention task schedule: `MaintenanceScheduleStore::getSchedule(RETENTION)`.
3. Verify the retention handler is registered in `DatabaseMaintenanceOrchestrator`.
4. Identify if a retention run was skipped (check task execution logs).

### Remediation
- If policy misconfigured: update TTL values and restart retention scheduler.
- If handler missing: register the retention handler and trigger a manual run.
- If records not deleted: force a retention sweep via:
  `POST /admin/maintenance/run_retention?policy=<name>`
- For compliance incidents: notify security/compliance team immediately.

---

## Scenario 4 — Task Queue Overflow (`[MAINTENANCE:TaskQueueOverflow]`)

### Symptoms
- Log line: `[MAINTENANCE:TaskQueueOverflow] queue_depth=<n> limit=<n>`
- Maintenance tasks backing up; scheduled tasks delayed or dropped
- `themis_maintenance_queue_depth` metric at or above configured limit

### Triage
1. Identify which task types are filling the queue (vacuum, compaction, retention).
2. Check for slow handlers: look for tasks with high `execution_ms` in logs.
3. Verify worker thread count: `themis_maintenance_worker_threads` metric.
4. Check for a spike in scheduled tasks triggered by a large data event.

### Remediation
- Increase queue limit temporarily: `maintenance_task_queue_limit` config key.
- Increase worker thread count: `maintenance_worker_count` config key, then reload.
- Identify and optimize the slowest handler.
- If queue cannot drain: pause low-priority tasks (non-critical vacuum) to unblock
  high-priority tasks (retention compliance).

---

## Scenario 5 — General Module Degradation / Orchestrator Stuck

### Symptoms
- Multiple log patterns firing simultaneously
- Orchestrator health check failing: `GET /health/maintenance` → `503`
- No tasks executing despite non-empty queue

### Triage
1. Check logs for root cause (VacuumFailed, CompactionStall, etc.).
2. Verify orchestrator thread is alive: check `themis_maintenance_orchestrator_alive` metric.
3. Inspect for deadlock using `gdb` or a thread dump if the process is live.

### Remediation
1. Resolve the root cause (see relevant scenario above).
2. If deadlocked: restart the maintenance module via:
   `POST /admin/maintenance/restart`
3. Replay any dropped tasks from the schedule store after restart.
4. Validate task queue depth returns to normal before re-enabling full load.
5. File a post-incident report and update this runbook.

---

## Alert Reference

| Alert Name                        | Log Pattern                            | Severity |
|-----------------------------------|----------------------------------------|----------|
| `MaintenanceVacuumFailed`         | `[MAINTENANCE:VacuumFailed]`           | High     |
| `MaintenanceCompactionStall`      | `[MAINTENANCE:CompactionStall]`        | High     |
| `MaintenanceRetentionViolation`   | `[MAINTENANCE:RetentionViolation]`     | Critical |
| `MaintenanceTaskQueueOverflow`    | `[MAINTENANCE:TaskQueueOverflow]`      | Medium   |

---

## Related Documents

- `src/maintenance/ROADMAP.md` — Wave D contribution
- `tests/integration/test_maintenance_soak.cpp` — soak test coverage
- `tests/maintenance/test_maintenance_highcardinality_stress.cpp` — stress coverage
- `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md` — sign-off checklist
