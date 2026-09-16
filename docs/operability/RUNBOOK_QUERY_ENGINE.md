# Runbook: Query Engine — Operator Remediation Guide

<!-- Status: current | Created: 2026-09-16 | Wave D delivery -->
<!-- Links: src/query/ROADMAP.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Overview

This runbook provides operator procedures for the five most critical incident
classes in the ThemisDB **Query Engine** (AQL parser, cost-based planner,
vectorised executor, plan cache, and join optimizer).  Each scenario includes
detection signals, root-cause analysis steps, and remediation actions.

**Module error codes:** QE-6000–QE-6199  
**Log prefix format:** `[QUERY:<IncidentClass>]`

---

## Scenario 1 — Planner Out-of-Memory

**Log pattern:** `[QUERY:PlannerOOM]`

### Detection

```
[QUERY:PlannerOOM] query_id=<ID> plan_size_bytes=<N> threshold_bytes=<M> correlation_id=<C>
```

- Alert fires when `plan_size_bytes > plan_oom_threshold_bytes` (default 256 MB)
  or when the process RSS crosses the configured `query.planner_mem_limit_mb`.
- Grafana panel: *Query / Planner Memory* → spike above the threshold line.

### Root Cause Analysis

1. Identify the offending query: search logs for `[QUERY:PlannerOOM] query_id=<ID>`.
2. Retrieve the plan cost estimate: look for `[QUERY:PlanCost] query_id=<ID>`.
3. Check for cartesian product or unguarded join fan-out:
   ```bash
   grep "JoinExplosion\|CartesianProduct" /var/log/themisdb/query.log | tail -20
   ```
4. Inspect collection sizes for the referenced collections.
5. Check `query.planner_mem_limit_mb` in `config/themisdb.yaml`.

### Remediation

| Step | Action |
|------|--------|
| 1 | Terminate the offending query via the admin API: `DELETE /admin/queries/<ID>`. |
| 2 | Add a `LIMIT` clause or index hint to the originating query. |
| 3 | Increase `query.planner_mem_limit_mb` if all queries are well-formed. |
| 4 | Enable `query.planner_oom_guard: true` to auto-abort oversized plans. |
| 5 | If persistent: escalate with `themis-diag dump query-planner`. |

### Escalation

Escalate to on-call engineering if OOM persists after step 4 for > 3 min.  
Capture `themis-diag dump query` before escalating.

---

## Scenario 2 — Executor Deadlock

**Log pattern:** `[QUERY:ExecutorDeadlock]`

### Detection

```
[QUERY:ExecutorDeadlock] worker_id=<W> blocked_ms=<T> query_id=<ID> lock_key=<K>
```

- Alert fires when any executor worker has been blocked for > 5 000 ms with
  zero query progress (error code QE-6103).
- Throughput counter `themis_query_executor_ops_total` stops incrementing.

### Root Cause Analysis

1. Capture thread dump: `kill -SIGQUIT <themisdb_pid>` (writes stacks to stderr).
2. Identify blocked `query-worker-*` threads.  Look for mutual lock acquisition.
3. Check `[QUERY:PlanCacheLock]` log lines — plan cache lock contention often
   precedes executor deadlocks.
4. Look for long-running transactions holding row locks referenced by the executor.

### Remediation

| Step | Action |
|------|--------|
| 1 | Abort the blocked query: `DELETE /admin/queries/<ID>`. |
| 2 | Restart the query executor thread pool (graceful drain): `POST /admin/executor/restart`. |
| 3 | Reduce `query.executor_lock_timeout_ms` to force early abort. |
| 4 | Enable deadlock detection: `query.deadlock_detection_enabled: true`. |

### Escalation

Escalate immediately if the process hangs after step 2.

---

## Scenario 3 — Plan Cache Corruption

**Log pattern:** `[QUERY:PlanCacheCorruption]`

### Detection

```
[QUERY:PlanCacheCorruption] cache_key=<K> expected_hash=<H1> actual_hash=<H2> evicted=<true|false>
```

- Alert fires when a cache hit produces a plan hash mismatch.
- Error code QE-6120.  May follow a schema DDL change or storage restart.

### Root Cause Analysis

1. Check recent DDL events: `grep 'DDL\|SCHEMA' /var/log/themisdb/query.log | tail -30`.
2. Check for storage engine restart events in the last 10 min.
3. Verify plan cache version against schema version:
   ```bash
   curl -s http://localhost:8529/admin/plan-cache/version
   ```
4. Look for memory corruption indicators: `[QUERY:MemoryError]` or OOM events.

### Remediation

| Step | Action |
|------|--------|
| 1 | Flush the plan cache: `DELETE /admin/plan-cache`. |
| 2 | Verify schema version matches cache version after flush. |
| 3 | If corruption re-appears: disable the plan cache temporarily (`query.plan_cache_enabled: false`) and restart. |
| 4 | File a bug with the corrupted key hash pair and DDL timeline. |

---

## Scenario 4 — Slow Query Detection

**Log pattern:** `[QUERY:SlowQuery]`

### Detection

```
[QUERY:SlowQuery] query_id=<ID> elapsed_ms=<T> threshold_ms=<M> plan_hash=<H> collections=<C>
```

- Alert fires when `elapsed_ms > query.slow_query_threshold_ms` (default 1 000 ms).
- Grafana panel: *Query / p99 Latency* → spike above SLA line.

### Root Cause Analysis

1. Retrieve the full query text: `GET /admin/queries/<ID>/text`.
2. Retrieve the execution plan: `GET /admin/queries/<ID>/plan`.
3. Identify the bottleneck stage: parser, planner, or executor.
4. Check index usage: look for `FULL_SCAN` in the plan JSON.
5. Check for cache thrashing: `grep 'PlanCacheEvict' /var/log/themisdb/query.log | tail -20`.

### Remediation

| Step | Action |
|------|--------|
| 1 | Add a covering index for the filter predicate. |
| 2 | Add `LIMIT` or pagination to bound result set size. |
| 3 | Use the `WITH INDEX <name>` hint to force index selection. |
| 4 | Increase `query.plan_cache_size` to reduce re-planning overhead. |
| 5 | If systematic: run `EXPLAIN` and share with the engineering team. |

---

## Scenario 5 — Join Explosion

**Log pattern:** `[QUERY:JoinExplosion]`

### Detection

```
[QUERY:JoinExplosion] query_id=<ID> estimated_rows=<N> actual_rows=<M> join_depth=<D> correlation_id=<C>
```

- Alert fires when `actual_rows > query.join_explosion_row_limit` (default 10 000 000).
- Error code QE-6150.  May trigger downstream OOM.

### Root Cause Analysis

1. Retrieve the query plan: `GET /admin/queries/<ID>/plan`.
2. Identify the join producing the cartesian product (look for `HashJoin` or
   `NestedLoopJoin` with no filter predicate).
3. Check cardinality estimates: look for `estimated_rows` vs `actual_rows` mismatch.
4. Verify that an equi-join predicate exists for every collection pair.

### Remediation

| Step | Action |
|------|--------|
| 1 | Abort the query immediately: `DELETE /admin/queries/<ID>`. |
| 2 | Add an explicit join predicate (equi-join on indexed fields). |
| 3 | Restrict the input collection with a pre-filter `FILTER` clause. |
| 4 | Enable `query.join_explosion_guard: true` to auto-abort exploding joins. |
| 5 | Refresh collection statistics: `POST /admin/collections/<C>/analyze`. |

---

## Quick Reference

| Scenario              | Log Pattern                       | Error Code | First Action                             |
|-----------------------|-----------------------------------|------------|------------------------------------------|
| Planner OOM           | `[QUERY:PlannerOOM]`              | QE-6000    | Abort query; add LIMIT                   |
| Executor Deadlock     | `[QUERY:ExecutorDeadlock]`        | QE-6103    | Abort query; restart executor pool       |
| Plan Cache Corruption | `[QUERY:PlanCacheCorruption]`     | QE-6120    | Flush plan cache; verify schema version  |
| Slow Query            | `[QUERY:SlowQuery]`               | N/A        | Add index; add LIMIT                     |
| Join Explosion        | `[QUERY:JoinExplosion]`           | QE-6150    | Abort query; add equi-join predicate     |

---

## Diagnostics Commands

```bash
# Dump current query engine state
themis-diag dump query

# List active (running) queries
curl -s http://localhost:8529/admin/queries?state=active | jq .

# Abort a specific query
curl -s -X DELETE http://localhost:8529/admin/queries/<QUERY_ID>

# Flush plan cache
curl -s -X DELETE http://localhost:8529/admin/plan-cache

# Show plan cache stats
curl -s http://localhost:8529/admin/plan-cache/stats | jq .

# Tail query engine log
tail -f /var/log/themisdb/query.log | grep '\[QUERY:'
```

---

## Related Documents

- `src/query/ROADMAP.md` — module roadmap and Wave D closure
- `ARCHITECTURE.md` — query engine architecture
- `SECURITY.md` — authentication and authorization for query endpoints
- `tests/integration/test_query_engine_soak.cpp` — Wave D soak test
- `benchmarks/query/bench_query_dedicated_gates.cpp` — Wave D benchmark gates
