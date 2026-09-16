# Runbook: Execution Engine — Operator Remediation Guide

<!-- Status: current | Created: 2026-09-16 | Wave D delivery -->
<!-- Links: src/execution/ROADMAP.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Overview

This runbook provides operator procedures for the five most critical incident
classes in the ThemisDB **Execution Engine** (query scheduler and work-stealing
thread pool).  Each scenario includes detection signals, root-cause analysis
steps, and remediation actions.

**Module error codes:** E7100–E7199  
**Log prefix format:** `[EXECUTION:<IncidentClass>]`

---

## Scenario 1 — Queue Overflow

**Log pattern:** `[EXECUTION:QueueOverflow]`

### Detection

```
[EXECUTION:QueueOverflow] queue_depth=<N> max_depth=<M> dropped_query_id=<ID> priority=<P>
```

- Alert fires when `queue_depth / max_depth > 0.90` for > 30 s, or when any
  query is dropped (error code E7100).
- Grafana panel: *Execution / Queue Depth* → threshold line at 90 %.

### Root Cause Analysis

1. Check `max_queue_depth` config — may be undersized for current workload.
2. Identify priority distribution: `ctest -L execution` or query the
   `/metrics` endpoint for `themis_execution_queue_by_priority`.
3. Confirm consumer thread count: `ps -T -p <themisdb_pid>` — look for
   `exec-worker-*` threads.
4. Look for slow queries holding worker slots:
   `[EXECUTION:SlowTask]` log lines within the same time window.

### Remediation

| Step | Action |
|------|--------|
| 1 | Increase `execution.max_queue_depth` in `config/themisdb.yaml` and reload. |
| 2 | Scale up `execution.max_threads` (within hardware limit) and reload. |
| 3 | Shed non-critical (LOW/NORMAL priority) load temporarily via the API rate limiter. |
| 4 | If persistent: enable priority back-pressure (`execution.backpressure_enabled: true`). |

### Escalation

Escalate to on-call engineering if queue overflow continues after step 4 for
> 5 min.  Capture `themis-diag dump execution` output before escalating.

---

## Scenario 2 — Work-Steal Deadlock

**Log pattern:** `[EXECUTION:WorkStealDeadlock]`

### Detection

```
[EXECUTION:WorkStealDeadlock] worker_id=<W> steal_attempts=<N> elapsed_ms=<T>
```

- Alert fires when any worker reports `steal_attempts > 1000` with zero
  progress in the last 5 s (error code E7103).
- All active workers appear blocked; queue depth stops decreasing.

### Root Cause Analysis

1. Capture thread dump:
   ```bash
   kill -SIGQUIT <themisdb_pid>   # writes stack traces to stderr
   ```
2. Identify the blocked `exec-worker-*` thread.  Look for:
   - Circular lock acquisition (two workers each holding a lock the other needs).
   - Spin-wait on an atomic flag that is never cleared.
3. Check recent deployments for changes to `thread_pool_manager.cpp` or
   query-task submission paths.

### Remediation

| Step | Action |
|------|--------|
| 1 | If < 3 workers are affected, use `themis-admin execution restart-workers --ids <W>` to recycle the affected workers without a full restart. |
| 2 | If all workers are blocked, perform a graceful restart: `systemctl restart themisdb`. |
| 3 | Apply hotfix for the identified lock ordering issue and redeploy. |

### Escalation

Always escalate for root-cause fix — deadlocks indicate a code defect.

---

## Scenario 3 — Slow Task Detection

**Log pattern:** `[EXECUTION:SlowTask]`

### Detection

```
[EXECUTION:SlowTask] query_id=<ID> worker_id=<W> elapsed_ms=<T> sla_deadline_ms=<D>
```

- Alert fires when `elapsed_ms > sla_deadline_ms * 0.8` for a running query.
- Metric: `themis_execution_task_latency_p99` breaches SLA threshold.

### Root Cause Analysis

1. Identify the query plan: look up `query_id` in the query log.
2. Check for lock contention on the affected worker (correlate with
   `[EXECUTION:WorkStealDeadlock]` events).
3. Check host resource pressure: CPU steal time, memory swap activity.
4. Review recent schema changes that may have increased plan complexity.

### Remediation

| Step | Action |
|------|--------|
| 1 | Lower `execution.sla_deadline_ms` for LOW-priority queries to shed them early. |
| 2 | Increase `execution.max_threads` to reduce per-worker queue depth. |
| 3 | If caused by a bad query plan: kill the offending query via `themis-admin query kill <ID>` and notify the API consumer. |
| 4 | Long-term: enable adaptive deadline estimation (`FUTURE_ENHANCEMENTS.md § Adaptive Scheduling`). |

---

## Scenario 4 — Executor Thread Crash

**Log pattern:** `[EXECUTION:WorkerCrash]`

### Detection

```
[EXECUTION:WorkerCrash] worker_id=<W> signal=<SIGSEGV|SIGABRT> thread_name=exec-worker-<W>
```

- Monitor `themis_execution_worker_count` — a drop below `min_threads`
  triggers the alert.

### Root Cause Analysis

1. Capture core dump if enabled (`/proc/sys/kernel/core_pattern`).
2. Load with `gdb themisdb <core>` and run `bt full` on the crashed thread.
3. Check for null pointer dereferences in task callback code or task queue
   state corruption.

### Remediation

| Step | Action |
|------|--------|
| 1 | The pool's self-healing spawner replaces crashed workers automatically; verify via `themis_execution_worker_count` metric. |
| 2 | If crash recurs, temporarily reduce `execution.max_threads` to reduce blast radius. |
| 3 | If self-healing fails: `systemctl restart themisdb`. |
| 4 | File a P1 bug with core dump and logs before restarting in production. |

---

## Scenario 5 — Memory Pressure

**Log pattern:** `[EXECUTION:MemoryPressure]`

### Detection

```
[EXECUTION:MemoryPressure] rss_mb=<R> limit_mb=<L> queue_depth=<Q> action=shed_low_priority
```

- Alert fires when `rss_mb / limit_mb > 0.85` and the execution engine begins
  shedding LOW-priority queries.
- Metric: `process_resident_memory_bytes` crosses 85 % of cgroup limit.

### Root Cause Analysis

1. Check for queue depth growth: large backlog keeps query state objects alive.
2. Identify large queries: inspect `themis_execution_task_memory_bytes_p99`.
3. Check for memory leaks in task callback code (valgrind / ASAN build).

### Remediation

| Step | Action |
|------|--------|
| 1 | Reduce `execution.max_queue_depth` to limit in-flight task state. |
| 2 | Enable `execution.memory_pressure_shed: true` to proactively drop LOW tasks at 80 % RSS. |
| 3 | Increase cgroup memory limit if hardware permits. |
| 4 | If caused by a leak: roll back the last deployment and file a P1 bug. |

---

## Reference

| Item | Value |
|------|-------|
| Error codes | E7100–E7199 |
| Config file | `config/themisdb.yaml` — `execution:` block |
| Metrics prefix | `themis_execution_*` |
| Admin CLI | `themis-admin execution` |
| Source | `src/execution/query_scheduler.cpp`, `thread_pool_manager.cpp` |
| Architecture | `src/execution/ARCHITECTURE.md` |
| Wave D test | `tests/integration/test_execution_engine_soak.cpp` |
| Benchmark gates | `benchmarks/execution/bench_execution_dedicated_gates.cpp` |
