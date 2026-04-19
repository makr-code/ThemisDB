# Transaction Troubleshooting Guide

The `transaction` module implements ACID transaction semantics for ThemisDB, including MVCC snapshot isolation, distributed 2PC, saga-based compensation, deadlock detection, crash recovery via WAL replay, and branch-based transaction management.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `Transaction: deadlock detected` | Two transactions waiting on each other's locks | Retry with exponential backoff; reduce lock scope |
| `CrashRecoveryManager: WAL replay failed` | Corrupt WAL entry | Run `themisdb-admin wal repair` |
| `SagaTransaction: compensation failed` | Compensation step not idempotent | Implement idempotent compensating transactions |
| 2PC transaction hangs indefinitely | Coordinator node crashed | Enable `auto_abort_timeout_ms`; check coordinator |
| `LockManager: lock wait timeout` | Lock held too long | Reduce transaction scope; increase `lock_timeout_ms` |
| `SnapshotManager: snapshot too old` | Snapshot predates vacuum | Reduce transaction lifetime; tune `vacuum_interval` |
| `BranchManager: merge conflict` | Concurrent writes to same key on different branches | Use three-way merge; resolve manually |
| High transaction abort rate | Contention on hot keys | Use optimistic locking; distribute writes |
| `TransactionManager: max active limit` | Too many concurrent transactions | Increase `transaction.max_active` |
| Write after read conflict (SSI) | Serializable isolation preventing anomaly | Retry; consider Read Committed if acceptable |

## Common Issues

### Issue 1: Deadlock Between Two Transactions

**Description:** Two concurrent transactions each hold a lock the other needs.

**Symptoms:**
- Log: `LockManager: deadlock cycle detected: txn-A → txn-B → txn-A`
- One transaction is automatically aborted

**Cause:** Transactions acquire locks in different orders.

**Solution:**
```bash
# Check deadlock frequency
curl -s http://localhost:9100/metrics | grep themisdb_transaction_deadlock
```
```yaml
transaction:
  deadlock:
    detection_enabled: true
    detection_interval_ms: 100
    victim_selection: youngest    # abort youngest txn to maximize useful work
  lock_timeout_ms: 5000
```
```python
# Application-level fix: always acquire locks in consistent order
# and use retry with backoff
import time, random

def transact_with_retry(fn, max_retries=5):
    for attempt in range(max_retries):
        try:
            return fn()
        except DeadlockError:
            time.sleep(random.uniform(0.01, 0.1) * (2 ** attempt))
    raise MaxRetriesExceeded()
```

---

### Issue 2: WAL Replay Fails After Crash

**Description:** ThemisDB cannot replay the WAL to recover committed data after a crash.

**Symptoms:**
- Log: `CrashRecoveryManager: WAL checksum mismatch at offset=1048576`
- Startup aborts with `Recovery: unrecoverable WAL corruption`

**Cause:** Power loss or OS crash truncated a WAL entry mid-write.

**Solution:**
```bash
# Attempt WAL repair (safe: skips corrupt entries from uncommitted txns)
themisdb-admin wal repair --data-dir /var/lib/themisdb/data --dry-run
themisdb-admin wal repair --data-dir /var/lib/themisdb/data

# If repair fails, restore from latest PITR snapshot
themisdb-admin pitr restore --snapshot-id $(themisdb-admin pitr list --latest)
```
```yaml
transaction:
  wal:
    sync_mode: fdatasync           # "none" | "fdatasync" | "fsync"
    group_commit_interval_ms: 2    # batch WAL writes for performance
    checksum_algorithm: crc32c
```

---

### Issue 3: Saga Compensation Fails

**Description:** A saga transaction fails during compensation, leaving data in an inconsistent state.

**Symptoms:**
- Log: `SagaTransaction: compensation step 3 failed: duplicate key on rollback`
- Business process is stuck in COMPENSATING state

**Cause:** Compensation step is not idempotent; retried compensation causes duplicate key error.

**Solution:**
```yaml
transaction:
  saga:
    compensation_retry_max: 5
    compensation_retry_backoff_ms: 1000
    idempotency_key_required: true     # enforce idempotent compensations
    dead_letter_queue: true            # move failed compensations to DLQ
```
```bash
# View stuck sagas
themisdb-admin transaction saga list --state COMPENSATING

# Manually advance a stuck saga
themisdb-admin transaction saga advance \
  --saga-id <id> \
  --step 3 \
  --action compensate
```

---

### Issue 4: 2PC Transaction Hangs in PREPARED State

**Description:** A distributed transaction remains in `PREPARED` state indefinitely.

**Symptoms:**
- Log: `TransactionManager: txn-xyz stuck in PREPARED (age=300s)`
- Locks held by the transaction block other writes

**Cause:** 2PC coordinator crashed after sending `PREPARE` but before receiving all responses.

**Solution:**
```yaml
transaction:
  two_phase_commit:
    coordinator_timeout_ms: 30000
    auto_abort_timeout_ms: 60000   # force-abort PREPARED txns after 60s
    recovery_enabled: true
    coordinator_heartbeat_ms: 5000
```
```bash
# Force-abort a stuck transaction
themisdb-admin transaction abort --txn-id txn-xyz --force

# Check 2PC coordinator status
themisdb-admin transaction coordinator status
```

---

### Issue 5: Lock Wait Timeout

**Description:** A transaction waiting for a lock times out.

**Symptoms:**
- Error: `LockManager: lock acquisition timed out after 5000ms`
- Application receives `{"error": "lock_timeout"}`

**Cause:** A long-running transaction holds the required lock.

**Solution:**
```bash
# Find which transaction holds the lock
themisdb-admin transaction locks --collection orders --key order-42

# Kill the blocking transaction (after investigation)
themisdb-admin transaction kill --txn-id <blocking-txn-id>
```
```yaml
transaction:
  lock_timeout_ms: 15000           # increase from 5000
  lock_manager:
    max_locks_per_transaction: 10000
    lock_table_size: 1000000
```

---

### Issue 6: Snapshot Too Old for Read

**Description:** A long-running read transaction fails because its snapshot has been vacuumed.

**Symptoms:**
- Error: `SnapshotManager: snapshot version=12345 no longer available (vacuumed)`

**Cause:** MVCC vacuum ran and removed old row versions that the transaction needs.

**Solution:**
```yaml
transaction:
  mvcc:
    max_transaction_lifetime_ms: 300000    # reject txns older than 5 min
    vacuum_interval_ms: 60000
    vacuum_horizon_pct: 80                 # vacuum only when >80% of txns completed
    long_running_txn_warning_ms: 60000
```

---

### Issue 7: Branch Merge Conflict

**Description:** Merging a transaction branch back to the main branch fails with conflicts.

**Symptoms:**
- Log: `BranchManager: merge conflict on key=product-42 (both branches modified)`
- Merge API returns `409 Conflict`

**Cause:** Both the branch and main modified the same document.

**Solution:**
```bash
# Show conflicting keys
themisdb-admin transaction branch conflicts --branch feature-pricing

# Three-way merge (use base version as ancestor)
themisdb-admin transaction branch merge \
  --branch feature-pricing \
  --strategy three_way \
  --on-conflict prefer_branch    # or "prefer_main" | "manual"
```

---

### Issue 8: High Transaction Abort Rate Under Contention

**Description:** Many transactions are being aborted, reducing throughput.

**Symptoms:**
- `themisdb_transaction_abort_total` is 30% of total transactions
- Log: `TransactionManager: txn aborted due to write-write conflict`

**Cause:** Multiple transactions trying to write the same hot keys.

**Solution:**
```yaml
transaction:
  isolation_level: read_committed  # relax from serializable if acceptable
  optimistic:
    enabled: true                  # use OCC instead of pessimistic locking
    validation_retries: 3
  hot_key_mitigation:
    enabled: true
    partition_hot_keys: true
    hot_key_threshold_tps: 1000
```

## Diagnostic Commands

```bash
# Active transactions
themisdb-admin transaction list --state active

# Lock table status
themisdb-admin transaction locks --stats

# Deadlock history
themisdb-admin transaction deadlocks --last 24h

# WAL status
themisdb-admin wal status

# Transaction throughput metrics
curl -s http://localhost:9100/metrics | grep themisdb_transaction

# Tail transaction logs
journalctl -u themisdb -f | grep -E "transaction|txn|lock|deadlock|saga|2pc|wal"
```

## Configuration Reference

```yaml
transaction:
  isolation_level: snapshot         # "read_committed" | "snapshot" | "serializable"
  lock_timeout_ms: 5000
  max_active: 10000
  mvcc:
    vacuum_interval_ms: 60000
    max_transaction_lifetime_ms: 600000
  two_phase_commit:
    auto_abort_timeout_ms: 60000
    recovery_enabled: true
  saga:
    compensation_retry_max: 5
    dead_letter_queue: true
  wal:
    sync_mode: fdatasync
    group_commit_interval_ms: 2
  deadlock:
    detection_enabled: true
    victim_selection: youngest
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `wal.sync_mode` | `none` | `fdatasync` for durability |
| `isolation_level` | `serializable` with high contention | `snapshot` or `read_committed` |
| `two_phase_commit.auto_abort_timeout_ms` | unset | `60000` |
| `mvcc.max_transaction_lifetime_ms` | unset | `600000` |

## Known Limitations

- Distributed saga transactions do not support nested sagas in the current implementation.
- Branch transactions are limited to a single level of branching (no branch-of-branch).
- 2PC recovery requires coordinator state to be persisted; in-memory coordinator cannot recover after crash.
- Lock manager does not support intention locks; all locks are document-level.
- MVCC vacuum does not reclaim space from aborted large write transactions automatically; run manual vacuum.

## Related Documentation

- [Transaction Auto-Retry](../ARCHIVED/implementation-summaries/TRANSACTION_AUTO_RETRY.md)
- [Distributed Transactions](../DISTRIBUTED_TRANSACTIONS.md)
- [Branching Strategy](../ci-cd/branching-release-history/BRANCHING_STRATEGY.md)
- [Branching Docs Index](../BRANCHING_DOCS_INDEX.md)
- [Race Condition Analysis](../RACE_CONDITION_ANALYSIS.md)
