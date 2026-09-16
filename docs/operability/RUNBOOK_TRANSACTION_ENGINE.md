# Runbook: Transaction Engine — Operator Remediation Guide

<!-- Status: current | Created: 2026-09-16 | Wave D delivery -->
<!-- Links: src/transaction/ROADMAP.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Overview

This runbook provides operator procedures for the five most critical incident
classes in the ThemisDB **Transaction Engine** (ACID lifecycle manager, 2PC/3PC
distributed coordinator, SAGA orchestrator, MVCC lock manager, and WAL writer).
Each scenario includes detection signals, root-cause analysis steps, and
remediation actions.

**Module error codes:** TX-4000–TX-4199  
**Log prefix format:** `[TRANSACTION:<IncidentClass>]`

---

## Scenario 1 — 2PC Coordinator Failure

**Log pattern:** `[TRANSACTION:CoordinatorFailed]`

### Detection

```
[TRANSACTION:CoordinatorFailed] coordinator_id=<C> tx_id=<T> phase=<prepare|commit|abort> elapsed_ms=<M>
```

- Alert fires when the coordinator does not receive all participant votes within
  `transaction.coordinator_timeout_ms` (default 30 000 ms) (error code TX-4000).
- Grafana panel: *Transaction / Coordinator Health* → `coordinator_failures_total` increments.
- Symptom: distributed transactions remain in `PREPARING` state indefinitely.

### Root Cause Analysis

1. Identify the affected coordinator and transaction:
   ```bash
   grep 'CoordinatorFailed' /var/log/themisdb/transaction.log | tail -20
   ```
2. Check participant node availability: `GET /admin/transaction/participants`.
3. Check network connectivity between coordinator and participant nodes.
4. Look for WAL write failures on the coordinator: `[TRANSACTION:WALSyncFailed]` events.
5. Check for in-doubt transactions: `GET /admin/transaction/in-doubt`.

### Remediation

| Step | Action |
|------|--------|
| 1 | Identify all in-doubt transactions: `GET /admin/transaction/in-doubt`. |
| 2 | For each in-doubt transaction, force abort: `POST /admin/transaction/<T>/force-abort`. |
| 3 | Restart the coordinator process if it is unavailable. |
| 4 | After coordinator recovery, WAL replay resolves remaining in-doubt transactions automatically. |
| 5 | Verify no data loss: check `transaction.wal_replay_complete` event in logs. |

### Escalation

Escalate immediately if WAL replay does not complete within 5 min of coordinator
restart.  Capture `themis-diag dump transaction-coordinator` before escalating.

---

## Scenario 2 — Orphan Transaction Cleanup

**Log pattern:** `[TRANSACTION:OrphanTx]`

### Detection

```
[TRANSACTION:OrphanTx] tx_id=<T> age_ms=<A> state=<ACTIVE|PREPARING> last_heartbeat_ms=<H>
```

- Alert fires when a transaction has been ACTIVE or PREPARING for >
  `transaction.orphan_timeout_ms` (default 60 000 ms) with no heartbeat
  (error code TX-4030).
- Grafana panel: *Transaction / Orphan Gauge* → non-zero value.

### Root Cause Analysis

1. List orphan transactions: `GET /admin/transaction/orphans`.
2. Check client connection state for the originating session.
3. Look for coordinator crash events near the transaction start timestamp.
4. Inspect lock table for keys held by the orphan transaction.

### Remediation

| Step | Action |
|------|--------|
| 1 | Attempt graceful abort: `POST /admin/transaction/<T>/abort`. |
| 2 | If graceful abort fails: force-abort: `POST /admin/transaction/<T>/force-abort`. |
| 3 | Release any held locks: `POST /admin/transaction/<T>/release-locks`. |
| 4 | Review `transaction.orphan_timeout_ms` — reduce if orphans are accumulating. |
| 5 | Enable orphan auto-cleanup: `transaction.orphan_auto_cleanup: true`. |

---

## Scenario 3 — Deadlock Cycle

**Log pattern:** `[TRANSACTION:Deadlock]`

### Detection

```
[TRANSACTION:Deadlock] cycle_length=<N> tx_ids=[<T1>,<T2>,...] victim_tx_id=<V> keys=[<K1>,<K2>,...]
```

- Alert fires when the deadlock detector identifies a wait-for cycle (error code TX-4060).
- The deadlock victim transaction is automatically aborted.
- Grafana panel: *Transaction / Deadlock Rate* → spikes indicate lock contention.

### Root Cause Analysis

1. Identify the cycle members from the log: `grep 'Deadlock' /var/log/themisdb/transaction.log`.
2. Check the access pattern of the involved transactions — look for reverse lock ordering.
3. Inspect lock contention metrics: `GET /admin/transaction/lock-stats`.
4. Determine whether the same cycle recurs (hotspot vs one-off).

### Remediation

| Step | Action |
|------|--------|
| 1 | Retry the aborted victim transaction (standard retry logic). |
| 2 | Review lock acquisition order in application code — impose a consistent ordering. |
| 3 | Reduce transaction scope to hold locks for shorter durations. |
| 4 | If hotspot: shard the contested key or use optimistic concurrency (OCC). |
| 5 | Tune `transaction.lock_timeout_ms` to fail fast and reduce cycle depth. |

---

## Scenario 4 — Write Conflict Storm

**Log pattern:** `[TRANSACTION:ConflictStorm]`

### Detection

```
[TRANSACTION:ConflictStorm] conflict_rate_per_s=<R> threshold=<T> window_s=<W> hot_keys=[<K1>,<K2>,...]
```

- Alert fires when the conflict rate exceeds `transaction.conflict_storm_threshold`
  conflicts/s for `transaction.conflict_storm_window_s` seconds
  (error code TX-4090).
- Symptom: high abort rate, increased retry pressure, latency spike.

### Root Cause Analysis

1. Identify hot keys: `GET /admin/transaction/hot-keys?top=10`.
2. Correlate with incoming write traffic patterns.
3. Check whether multiple application instances are retrying without backoff.
4. Look for SAGA compensation storms: `[TRANSACTION:SagaRetryStorm]` events.

### Remediation

| Step | Action |
|------|--------|
| 1 | Enable write rate limiting for the hot key namespace. |
| 2 | Introduce exponential backoff with jitter in the retry logic. |
| 3 | Use compare-and-swap (CAS) semantics instead of read-modify-write where possible. |
| 4 | Partition the hot key across multiple logical shards. |
| 5 | Enable circuit breaker: `transaction.conflict_circuit_breaker: true`. |

---

## Scenario 5 — WAL Sync Failure

**Log pattern:** `[TRANSACTION:WALSyncFailed]`

### Detection

```
[TRANSACTION:WALSyncFailed] wal_segment=<S> fsync_errno=<E> elapsed_ms=<T> node_id=<N>
```

- Alert fires when `fsync()` on a WAL segment returns a non-zero errno
  (error code TX-4120).
- **Severity: CRITICAL** — data durability is at risk.
- Grafana panel: *Transaction / WAL Sync Failures* → any non-zero value is critical.

### Root Cause Analysis

1. Identify the error code from `fsync_errno`:
   - `errno=28` (ENOSPC): disk full.
   - `errno=5` (EIO): disk I/O error.
   - `errno=30` (EROFS): read-only filesystem.
2. Check disk health: `dmesg | grep -i 'error\|fail\|bad sector'`.
3. Check available disk space: `df -h /data/themisdb/wal`.
4. Check filesystem mount options for `sync`/`dsync`.

### Remediation

| Step | Action |
|------|--------|
| 1 | **Immediately stop accepting new writes**: `PATCH /admin/config {"write_mode":"readonly"}`. |
| 2 | Free disk space or add capacity. |
| 3 | For I/O errors: fail over to a healthy replica immediately. |
| 4 | After disk is healthy: restart the WAL writer: `POST /admin/wal/restart`. |
| 5 | Replay the WAL to confirm no committed transactions were lost. |
| 6 | Return to read-write mode: `PATCH /admin/config {"write_mode":"readwrite"}`. |

### Escalation

Escalate immediately for any WAL sync failure.  Do not restart without capturing
`themis-diag dump transaction-wal` first.

---

## Quick Reference

| Scenario               | Log Pattern                          | Error Code | First Action                            |
|------------------------|--------------------------------------|------------|-----------------------------------------|
| 2PC Coordinator Failed | `[TRANSACTION:CoordinatorFailed]`    | TX-4000    | Force-abort in-doubt txns; restart coord|
| Orphan Tx Cleanup      | `[TRANSACTION:OrphanTx]`             | TX-4030    | Force-abort; release locks              |
| Deadlock Cycle         | `[TRANSACTION:Deadlock]`             | TX-4060    | Retry victim; fix lock order            |
| Write Conflict Storm   | `[TRANSACTION:ConflictStorm]`        | TX-4090    | Rate-limit; add backoff; partition keys |
| WAL Sync Failure       | `[TRANSACTION:WALSyncFailed]`        | TX-4120    | Stop writes; fix disk; replay WAL       |

---

## Diagnostics Commands

```bash
# Dump current transaction engine state
themis-diag dump transaction

# List active transactions
curl -s http://localhost:8529/admin/transaction/active | jq .

# List in-doubt transactions
curl -s http://localhost:8529/admin/transaction/in-doubt | jq .

# Force-abort a specific transaction
curl -s -X POST http://localhost:8529/admin/transaction/<TX_ID>/force-abort

# Show lock table statistics
curl -s http://localhost:8529/admin/transaction/lock-stats | jq .

# List orphan transactions
curl -s http://localhost:8529/admin/transaction/orphans | jq .

# WAL health check
curl -s http://localhost:8529/admin/wal/health | jq .

# Tail transaction engine log
tail -f /var/log/themisdb/transaction.log | grep '\[TRANSACTION:'
```

---

## Related Documents

- `src/transaction/ROADMAP.md` — module roadmap and Wave D closure
- `ARCHITECTURE.md` — transaction engine architecture
- `SECURITY.md` — authentication and authorization for transaction endpoints
- `tests/integration/test_transaction_engine_soak.cpp` — Wave D soak test
- `benchmarks/transaction/bench_transaction_dedicated_gates.cpp` — Wave D benchmark gates
