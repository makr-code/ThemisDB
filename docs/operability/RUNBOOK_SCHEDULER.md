# Runbook: Scheduler — Operator Incident Triage

<!-- Wave D deliverable | Module: scheduler | Validated: 2026-09-16 -->
<!-- Related: src/scheduler/ROADMAP.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Overview

This runbook covers the five most operator-critical incident classes for the
Scheduler module.  Each scenario includes detection signals, log-pattern
anchors, immediate mitigation steps, and escalation guidance.

---

## Scenario 1 — Task Queue Overflow

**Log pattern**: `[SCHEDULER:QueueOverflow]`

### Symptoms
- The scheduler task queue depth metric grows without bound.
- New task registrations are rejected or silently dropped.
- Worker threads appear saturated; CPU and lock contention metrics are elevated.

### Detection
```
grep '\[SCHEDULER:QueueOverflow\]' /var/log/themis/scheduler.log | tail -50
```
Check metric:
```
scheduler_task_queue_depth > 50000
scheduler_task_registration_rejected_total > 0
```

### Immediate mitigation
1. Inspect the current queue depth and worker utilisation:
   ```
   themis-admin scheduler status --verbose
   ```
2. Scale out worker threads (if the host has available CPU):
   ```
   themis-admin scheduler config set workers.count 16
   themis-admin scheduler workers reload
   ```
3. Temporarily shed non-critical tasks by priority class:
   ```
   themis-admin scheduler queue drain --priority low --confirm
   ```
4. If the queue is caused by a burst from a specific producer, rate-limit it:
   ```
   themis-admin scheduler ratelimit set --producer <id> --rate 1000/s
   ```

### Escalation
Escalate if task drops are observed or if the queue depth does not recover
within 5 minutes.  Attach `themis-admin scheduler stats` output.

---

## Scenario 2 — Trigger Deadlock

**Log pattern**: `[SCHEDULER:TriggerDeadlock]`

### Symptoms
- One or more trigger handlers stop making progress; goroutine/thread stacks
  show a circular wait.
- Triggered tasks never transition to the `EXECUTING` state.
- System-level lock-wait metrics (`scheduler_lock_wait_ms`) spike.

### Detection
```
grep '\[SCHEDULER:TriggerDeadlock\]' /var/log/themis/scheduler.log | tail -50
```
Capture a thread dump:
```
themis-admin scheduler debug thread-dump
```

### Immediate mitigation
1. Identify which triggers are involved:
   ```
   themis-admin scheduler trigger list --stuck
   ```
2. Cancel and re-enqueue the stuck triggers:
   ```
   themis-admin scheduler trigger cancel --stuck --confirm
   themis-admin scheduler trigger requeue --stuck --confirm
   ```
3. If the deadlock involves external adapter locks, restart the adapter:
   ```
   themis-admin scheduler adapter restart --name <adapter_name>
   ```
4. Reduce trigger concurrency to minimise future deadlock exposure:
   ```
   themis-admin scheduler config set trigger.concurrency 4
   ```

### Escalation
Escalate immediately if a full deadlock requires process restart.  Attach the
thread dump and trigger list output.

---

## Scenario 3 — Distributed Coordinator Timeout

**Log pattern**: `[SCHEDULER:CoordinatorTimeout]`

### Symptoms
- Distributed task coordination requests time out before receiving quorum.
- Tasks assigned to remote nodes remain in `PENDING` state indefinitely.
- Network or consensus layer metrics show elevated round-trip times.

### Detection
```
grep '\[SCHEDULER:CoordinatorTimeout\]' /var/log/themis/scheduler.log | tail -50
```
Check metric:
```
scheduler_coordinator_rtt_p99_ms > 2000
scheduler_coordinator_timeout_total > 0
```

### Immediate mitigation
1. Check coordinator node health:
   ```
   themis-admin scheduler coordinator status
   ```
2. Verify network connectivity between scheduler and coordinator nodes:
   ```
   themis-admin scheduler coordinator ping --all
   ```
3. If a coordinator node is partitioned, remove it from the active set:
   ```
   themis-admin scheduler coordinator remove --node <node_id> --confirm
   ```
4. Increase the coordination timeout to accommodate network jitter:
   ```
   themis-admin scheduler config set coordinator.timeout_ms 5000
   ```
5. Rebalance pending tasks to healthy nodes:
   ```
   themis-admin scheduler task rebalance --confirm
   ```

### Escalation
Escalate to the network/infrastructure team if coordinator ping fails across
multiple nodes.  This may indicate a broader partition event.

---

## Scenario 4 — Anomaly Detection False Positive

**Log pattern**: `[SCHEDULER:AnomalyFalsePositive]`

### Symptoms
- The anomaly detector fires alerts for tasks that are executing normally.
- Legitimate tasks are quarantined or killed based on erroneous anomaly scores.
- Alert noise degrades on-call reliability; team begins ignoring alerts.

### Detection
```
grep '\[SCHEDULER:AnomalyFalsePositive\]' /var/log/themis/scheduler.log | tail -50
```
Check metric:
```
scheduler_anomaly_false_positive_rate > 0.05   # > 5 %
```

### Immediate mitigation
1. Review the current anomaly detection thresholds:
   ```
   themis-admin scheduler anomaly config show
   ```
2. Temporarily raise the anomaly score threshold to reduce false positives:
   ```
   themis-admin scheduler anomaly config set threshold 0.95
   ```
3. Restore any incorrectly quarantined tasks:
   ```
   themis-admin scheduler task list --quarantined
   themis-admin scheduler task restore --quarantined --confirm
   ```
4. If the model drifted due to a workload change, retrain the baseline:
   ```
   themis-admin scheduler anomaly retrain --window 7d --confirm
   ```

### Escalation
Escalate to the ML/observability team if the false-positive rate remains above
5 % after threshold adjustment.  Attach anomaly score distribution data.

---

## Scenario 5 — Retention Policy Violation

**Log pattern**: `[SCHEDULER:RetentionViolation]`

### Symptoms
- Completed or expired tasks are not being purged per the configured retention
  policy.
- Task metadata storage grows faster than expected.
- Queries against historical task records degrade due to table bloat.

### Detection
```
grep '\[SCHEDULER:RetentionViolation\]' /var/log/themis/scheduler.log | tail -50
```
Check metric:
```
scheduler_task_store_size_bytes > 10737418240   # 10 GiB
scheduler_retention_purge_total{window="7d"} == 0
```

### Immediate mitigation
1. Check the configured retention policy:
   ```
   themis-admin scheduler retention list
   ```
2. Force a retention purge for the most bloated window:
   ```
   themis-admin scheduler retention run --window 7d --force --confirm
   ```
3. If the storage backend is read-only (permissions issue), fix ACLs:
   ```
   themis-admin scheduler storage verify-permissions
   ```
4. Compact the task metadata store after purge to reclaim disk space:
   ```
   themis-admin scheduler storage compact --confirm
   ```

### Escalation
Escalate if storage utilisation exceeds 90 % or if a forced purge fails.
Attach `themis-admin scheduler retention report`.

---

## Reference

| Log Pattern                           | Metric                                         | Owner               |
|---------------------------------------|------------------------------------------------|---------------------|
| `[SCHEDULER:QueueOverflow]`           | `scheduler_task_queue_depth`                   | scheduler-runtime   |
| `[SCHEDULER:TriggerDeadlock]`         | `scheduler_lock_wait_ms`                       | scheduler-runtime   |
| `[SCHEDULER:CoordinatorTimeout]`      | `scheduler_coordinator_rtt_p99_ms`             | scheduler-infra     |
| `[SCHEDULER:AnomalyFalsePositive]`    | `scheduler_anomaly_false_positive_rate`        | scheduler-observ    |
| `[SCHEDULER:RetentionViolation]`      | `scheduler_task_store_size_bytes`              | scheduler-storage   |

---

*Last updated: 2026-09-16 — Wave D operability pass*
