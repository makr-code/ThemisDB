# Scheduler Troubleshooting Guide

The `scheduler` module provides distributed task scheduling for ThemisDB, including cron-based job scheduling, event-driven trigger execution, anomaly detection on task performance, audit management, and integration with external schedulers.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| Scheduled task never runs | Cron expression invalid | Validate with `themisdb-admin scheduler validate-cron` |
| Task runs but result not stored | Task result store not configured | Set `scheduler.result_store.enabled: true` |
| `EventTrigger: no handler registered` | Handler not registered before trigger | Register handler at startup |
| Task anomaly detector too noisy | Threshold too sensitive | Increase `scheduler.anomaly.threshold_sigma` |
| Distributed coordinator election fails | Not enough nodes for quorum | Check `scheduler.coordinator.quorum_size` |
| Task audit log fills disk | No audit log rotation | Enable `scheduler.audit.rotation` |
| Task runs more than once (duplicate execution) | No distributed lock | Enable `scheduler.distributed_lock: true` |
| External scheduler adapter timeout | External scheduler slow or down | Increase `scheduler.external.timeout_ms` |
| Retention manager not cleaning up tasks | Retention not enabled | Enable `scheduler.retention.enabled: true` |
| Task stuck in `RUNNING` state | Worker crashed mid-task | Enable `scheduler.timeout_ms` for tasks |

## Common Issues

### Issue 1: Cron Task Never Executes

**Description:** A scheduled cron task is configured but never runs.

**Symptoms:**
- Log: `DistributedTaskCoordinator: task=daily_report scheduled but never triggered`
- No task execution records in audit log

**Cause:** Invalid cron expression or coordinator not elected as leader.

**Solution:**
```bash
# Validate cron expression
themisdb-admin scheduler validate-cron "0 2 * * *"

# Check coordinator status
themisdb-admin scheduler coordinator status

# List scheduled tasks
themisdb-admin scheduler tasks list
```
```yaml
scheduler:
  tasks:
    - name: daily_report
      cron: "0 2 * * *"            # 02:00 UTC daily
      handler: generate_daily_report
      timeout_ms: 300000
      enabled: true
```

---

### Issue 2: Task Stuck in RUNNING State

**Description:** A task shows `RUNNING` state permanently because the worker crashed.

**Symptoms:**
- `themisdb-admin scheduler tasks list` shows a task running for 2 hours
- Expected duration is 5 minutes

**Cause:** Worker process crashed; task lock not released.

**Solution:**
```bash
# Force-cancel a stuck task
themisdb-admin scheduler task cancel --name daily_report --force

# Clean up stale locks
themisdb-admin scheduler cleanup-stale-locks --older-than 1h
```
```yaml
scheduler:
  task_timeout_ms: 600000          # 10 min; auto-cancel if exceeded
  stale_lock_cleanup_interval_ms: 300000
  heartbeat_interval_ms: 5000      # worker must heartbeat every 5s
```

---

### Issue 3: Duplicate Task Execution

**Description:** A scheduled task runs twice simultaneously on different nodes.

**Symptoms:**
- Two identical task records in audit log at same timestamp
- Downstream system receives duplicate data

**Cause:** Distributed lock not enabled; both nodes pick up the task.

**Solution:**
```yaml
scheduler:
  distributed_lock:
    enabled: true
    backend: rocksdb               # "rocksdb" | "redis" | "etcd"
    ttl_ms: 60000
    retry_interval_ms: 1000
```

---

### Issue 4: Event Trigger Fires Without Executing Handler

**Description:** Event trigger is fired but no action occurs.

**Symptoms:**
- Log: `EventTrigger: trigger=new_order fired but no handler registered`
- Expected side effects (e.g., email notification) do not happen

**Cause:** Handler was not registered before the trigger fired.

**Solution:**
```yaml
scheduler:
  event_triggers:
    - event: new_order
      handler: send_confirmation_email
      collection: orders
      filter: "doc.total_amount > 100"
      async: true
```
```bash
# List registered handlers
themisdb-admin scheduler handlers list

# Manually fire a trigger for testing
themisdb-admin scheduler trigger fire \
  --event new_order \
  --payload '{"order_id": "test-123"}'
```

---

### Issue 5: Task Anomaly Detector Too Noisy

**Description:** Anomaly detection fires alerts for tasks that complete within normal variation.

**Symptoms:**
- Alert: `TaskAnomalyDetector: task=sync_users duration=45s exceeds threshold`
- Normal duration varies between 30s and 60s

**Cause:** Anomaly threshold too tight; seasonal variation not modelled.

**Solution:**
```yaml
scheduler:
  anomaly:
    enabled: true
    threshold_sigma: 3.5           # increase from 2.0
    window_size: 100               # use 100 samples for baseline
    seasonal_model: true
    min_samples: 20
```

---

### Issue 6: Audit Log Fills Disk

**Description:** The scheduler audit log grows without bound.

**Symptoms:**
- Disk usage for audit log > 10 GB
- Log: `TaskAuditManager: log write failed: ENOSPC`

**Cause:** Audit log rotation not configured.

**Solution:**
```yaml
scheduler:
  audit:
    enabled: true
    path: /var/lib/themisdb/audit/scheduler/
    rotation:
      enabled: true
      max_size_mb: 500
      max_files: 10
      compress: true
    retention_days: 90
```

---

### Issue 7: External Scheduler Adapter Timeout

**Description:** Integration with an external scheduler (e.g., Apache Airflow) times out.

**Symptoms:**
- Log: `ExternalSchedulerAdapter: request timeout after 5000ms`
- Tasks not being dispatched to external scheduler

**Cause:** External scheduler is slow or network latency is high.

**Solution:**
```yaml
scheduler:
  external:
    enabled: true
    adapter: airflow               # "airflow" | "kubernetes" | "custom"
    endpoint: http://airflow:8080/api/v1
    timeout_ms: 30000             # increase from 5000
    retry_max: 3
    retry_backoff_ms: 1000
```

---

### Issue 8: Retention Manager Not Cleaning Up Old Task Records

**Description:** Completed task records accumulate in the task result store.

**Symptoms:**
- Task result store grows to GB over months
- Log: `HybridRetentionManager: retention disabled for task_results`

**Cause:** Retention not enabled for task results.

**Solution:**
```yaml
scheduler:
  retention:
    enabled: true
    task_results_retention_days: 30
    audit_retention_days: 365
    run_interval_hours: 24
```
```bash
# Run retention manually
themisdb-admin scheduler retention run --dry-run
themisdb-admin scheduler retention run
```

## Diagnostic Commands

```bash
# List all scheduled tasks
themisdb-admin scheduler tasks list

# Task execution history
themisdb-admin scheduler tasks history --name daily_report --last 7d

# Coordinator election status
themisdb-admin scheduler coordinator status

# Anomaly alerts
themisdb-admin scheduler anomaly-alerts --last 24h

# Live scheduler metrics
curl -s http://localhost:9100/metrics | grep themisdb_scheduler

# Tail scheduler logs
journalctl -u themisdb -f | grep -E "scheduler|task|cron|trigger|anomaly|audit"
```

## Configuration Reference

```yaml
scheduler:
  enabled: true
  coordinator:
    quorum_size: 2
  distributed_lock:
    enabled: true
    backend: rocksdb
    ttl_ms: 60000
  task_timeout_ms: 300000
  anomaly:
    enabled: true
    threshold_sigma: 3.0
  audit:
    enabled: true
    rotation:
      enabled: true
      max_size_mb: 500
  retention:
    enabled: true
    task_results_retention_days: 30
```

## Known Limitations

- Cron scheduling precision is ±1 second; sub-second scheduling is not supported.
- Distributed lock backend `redis` requires a running Redis instance; `rocksdb` is recommended for simplicity.
- External scheduler adapters are synchronous; slow external schedulers block the coordinator thread.
- Event triggers do not support complex cross-collection join conditions; only single-document filters.

## Related Documentation

- [Scheduler Module ROADMAP](../../src/scheduler/ROADMAP.md)
- [Task Scheduler Cron CDC](../TASK_SCHEDULER_CRON_CDC.md)
- [Task Scheduler SIEM Integration](../ARCHIVED/implementation-summaries/TASK_SCHEDULER_SIEM_INTEGRATION.md)
- [Task Audit Events](../TASK_AUDIT_EVENTS.md)
- [Task Audit Quick Reference](../Audit/TASK_AUDIT_QUICK_REF.md)
