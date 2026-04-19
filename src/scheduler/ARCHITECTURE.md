# Scheduler Module — Architecture Guide

> **Status:** 2026-04-19 – Architekturtext gegen realen Sourcecode verifizieren; Abweichungen mit `<!-- TODO -->` markiert.

**Version:** 1.1
**Last Updated:** 2026-04-06
**Module Path:** `src/scheduler/`

---

## 1. Overview

The Scheduler module provides ThemisDB's task scheduling and automation infrastructure.
It enables cron-like periodic execution of AQL queries and custom functions for data
processing, maintenance, backup, retention, and analytics workflows. It also includes a
specialized hybrid retention manager for three-stage time-series data lifecycle management
and an event-trigger system for event-driven task execution.

---

## 2. Design Principles

- **Full Cron Expression Support** – the CronExpression parser supports the complete
  standard cron syntax including ranges, lists, steps, named aliases, and @-specials.
- **Security-First** – AQL tasks are injection-checked before execution; resource limits
  (CPU time, memory) are enforced per task.
- **Three-Stage Retention** – time-series data lifecycle is managed through Gorilla
  compression → adaptive variance-based downsampling → daily aggregates, achieving
  99.9% storage reduction over 1+ years.
- **Observable** – every task execution is traced (OpenTelemetry), audited, and metered.
- **Work-Stealing Thread Pool** – tasks are dispatched to a work-stealing thread pool
  for efficient CPU utilization under variable load.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `task_scheduler.cpp` | Core scheduler: cron parsing, thread pool, task dispatch, dynamic scaling, DAG execution, SLA alerts |
| `hybrid_retention_manager.cpp` | Three-stage time-series retention lifecycle |
| `event_trigger.cpp` | Event-driven task triggers (CDC events, metrics thresholds) |
| `distributed_task_coordinator.cpp` | Distributed leader election for scheduled tasks |
| `external_scheduler_adapter.cpp` | Integration with external schedulers (Airflow, Kubernetes CronJob) |
| `task_audit_manager.cpp` | Searchable task execution audit log |
| `task_audit_event.cpp` | Audit event schema |
| `task_anomaly_detector.cpp` | Detects anomalous task execution patterns |
| `task_result_store.cpp` | Persists task execution results and history |

*(CronExpression parser lives in `src/utils/cron_parser.cpp`)*
<!-- TODO: verify symbol exists in source -->

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│              Task Registration (API / config file)              │
│   schedule("* * * * *", "FOR doc IN x RETURN doc")             │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                    TaskScheduler                                 │
│                                                                  │
│  CronExpression::parse("* * * * *") → next_execution            │
│  Timer: fire at next_execution                                   │
│  SecurityValidator: AQL injection check + resource limits       │
│  DynamicScaling: adjust concurrency limit from queue depth      │
│  Work-stealing thread pool: dispatch task                       │
└──────────────────────────┬──────────────────────────────────────┘
                           │
       ┌───────────────────┴──────────────────────────┐
       │                                              │
┌──────▼──────────────────┐           ┌──────────────▼────────────┐
│  Task Execution          │           │  HybridRetentionManager   │
│  AQL → QueryEngine       │           │  Stage 1: Gorilla (0-7d) │
│  or custom fn            │           │  Stage 2: Downsample (1y) │
│  DAG dependency order    │           │  Stage 3: Daily agg (>1y) │
└──────┬──────────────────┘           └───────────────────────────┘
       │
┌──────▼────────────────────────────────────┐
│  Post-execution                           │
│  TaskAuditManager  (searchable audit log) │
│  TaskResultStore   (persistent results)   │
│  Alertmanager      (failure / SLA alerts) │
│  OpenTelemetry trace                      │
└───────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Scheduled Task Execution

```
Task registered: { id: "nightly_cleanup", cron: "0 2 * * *", aql: "..." }
    │
    ├─ CronExpression.parse("0 2 * * *") → next_execution: 2026-02-25T02:00:00Z
    │
    ├─ at 02:00:00: dispatch to thread pool
    │
    ├─ SecurityValidator: check AQL for injection patterns
    │       → rejected → audit log; no execution
    │       → OK → proceed
    │
    ├─ QueryEngine.execute(aql) → result
    │
    ├─ TaskResultStore.save(task_id, result, duration)
    ├─ TaskAuditManager.record(task_id, success, user, timestamp)
    └─ OTel span ended
```

### 4.2 Hybrid Retention (Time-Series)

```
TimeSeries data lifecycle (daily background job):
    │
    ├─ Stage 1 (0-7 days): Gorilla compression (10-20× reduction)
    │       raw 1-second samples → compressed delta encoding
    │
    ├─ Stage 2 (7-365 days): Variance-based downsampling
    │       low-variance periods → sparse samples
    │       high-variance (anomalies) → preserve density
    │
    └─ Stage 3 (>1 year): Daily aggregates
               compute min/max/avg/sum/p50/p99 per day
               drop raw samples → 99.9% storage reduction
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/query/` | AQL task execution |
| **Uses** | `src/storage/` | Retention data lifecycle |
| **Uses** | `src/cdc/` | Event triggers from changefeed |
| **Uses** | `src/utils/` | CronExpression parser |
| **Uses** | `src/observability/` | OpenTelemetry tracing and metrics |
| **Called by** | `src/server/` | Task management API |

---

## 6. Threading & Concurrency Model

- Work-stealing thread pool (default: `hardware_concurrency` threads).
- Timer fires on a dedicated scheduler thread; dispatch to pool is non-blocking.
- `HybridRetentionManager` runs as a background task (scheduled nightly).
- `DistributedTaskCoordinator` uses leader election to ensure tasks run on exactly one node.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Work-stealing thread pool | Efficient CPU utilization; idle threads steal work |
| CronExpression pre-computation | Next execution pre-computed on registration |
| Gorilla compression | First-order delta encoding for time-series (10-20×) |
| Variance-based downsampling | Preserves anomalies; reduces storage by 50-90× |

---

## 8. Security Considerations

- AQL tasks are injection-checked before execution (same checks as user queries).
- Resource limits (max CPU time, max memory) are enforced per task to prevent runaway tasks.
- Task results may contain sensitive data; access is restricted to the task owner.
- Distributed task coordinator prevents duplicate execution under network partitions.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `scheduler.thread_pool_size` | cpu_count | Worker thread pool size |
| `scheduler.max_concurrent_tasks` | 10 | Max simultaneously running tasks |
| `scheduler.task_timeout_s` | 3600 | Max task execution duration |
| `retention.stage1.days` | 7 | Gorilla compression window |
| `retention.stage2.days` | 365 | Downsampling window |
| `retention.stage3.enabled` | true | Enable daily aggregates |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Cron parse error | Reject task registration; return structured error |
| AQL injection detected | Reject execution; log security event |
| Task timeout | Kill execution; log audit event; mark task as failed |
| Resource limit exceeded | Kill execution; log; backoff next execution |
| Retention compression failure | Log error; keep raw data; retry next cycle |

---

## 11. Known Limitations & Future Work

- Distributed coordination requires `DistributedCoordinator` (sharding module) for gossip-based leader election.
- Raft-based leader election (stronger consistency than the current gossip implementation) is a planned future enhancement.
- Task result streaming for long-running AQL tasks is planned but not yet implemented.
- Dynamic resource allocation (cgroups-based CPU/memory quota enforcement independent of timeout) is a planned future enhancement.

---

## 12. References

- `src/scheduler/README.md` — module overview
- `src/utils/cron_parser.cpp` — CronExpression parser implementation
- `docs/scheduler/` — scheduler documentation
- `ARCHITECTURE.md` (root) — full system architecture
