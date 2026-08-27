# Transaction Module Contract

Datum: 2026-08-03  
**Status:** Active  
**Module:** transaction (Coordinator, 2PC/3PC/SAGA patterns)  
**Primary:** include/transaction/transaction_coordinator.h, src/transaction/ROADMAP.md

## Public API Surface

| API | Namespace | Input Contract | Output Contract | Errors | Thread-Safety | Ownership/Lifetime | Notes |
|---|---|---|---|---|---|---|---|
| `begin()` | `themis::transaction::ITransactionCoordinator` | Isolation level (SERIALIZABLE/REPEATABLE_READ/READ_COMMITTED), timeout_ms | TransactionHandle (TxnID, snapshot, options) | TxnTimeoutError if too many pending | ✅ Thread-safe (handle is lightweight token) | Handle owned by caller; release via commit/rollback | P0; GATE-TXN-01 ≤10µs |
| `put()` | `themis::transaction::ITransaction` | Key (≤8KB), Value (≤128MB), flags (upsert/insert_only) | Status (OK, ConflictError, ValidationError) | WriteConflictError if write-skew detected, LockTimeout | ✅ Serialized (write-lock acquired) | Key/Value copied internally | P0; GATE-TXN-02 ≤20µs |
| `get()` | `themis::transaction::ITransaction` | Key (UTF-8, ≤8KB), ConsistencyLevel (default: isolation_level) | std::optional<Value> (read view depends on isolation level) | ReadError (if underlying storage fails), KeyNotFound OK | ✅ Read-only; no locking (snapshot view) | Value owned by return; stable for txn duration | P0; GATE-TXN-03 ≤15µs |
| `commit()` | `themis::transaction::ITransaction` | N/A | CommitResult (committed=true or false + conflict details) | AbortReason (write conflict, timeout, validation failed) | 🔒 Coordinator serializes commits (2PC) | N/A (transaction closed after) | P0; GATE-TXN-04 ≤50µs (2-phase) |
| `rollback()` | `themis::transaction::ITransaction` | N/A | void (all writes discarded; snapshot released) | RollbackError (rare; already aborted) | ✅ Safe to call multiple times (idempotent) | N/A (transaction invalidated) | P0; GATE-TXN-05 ≤5µs |
| `executeSaga()` | `themis::transaction::ISagaOrchestrator` | Saga steps (Vec<Step>), compensation steps, context | SagaResult (success or failed step index + error) | StepError (if step fails; compensation runs), TimeoutError | ✅ Thread-safe (step execution queued) | Context borrowed during execution; ownership unchanged | P1 (distributed ops); timeout per step |
| `savepoint()` | `themis::transaction::ITransaction` | SavepointName (string, unique per txn) | SavepointHandle | SavepointError (duplicate name) | ✅ Safe; nested savepoints allowed | Handle valid until rollback to or txn end | Internal; for complex txns |
| `rollbackToSavepoint()` | `themis::transaction::ITransaction` | SavepointHandle | void (undoes all writes after savepoint) | SavepointNotFoundError | ✅ Thread-safe | N/A (mutation only) | Internal; partial rollback |
| `isolationLevel()` | `themis::transaction::ITransaction` | N/A | IsolationLevel (SERIALIZABLE / REPEATABLE_READ / READ_COMMITTED) | None | ✅ Lock-free (immutable) | Enum value-owned | Utility; P2 |

## Transaction Isolation Levels

| Level | Dirty Read | Non-Repeatable Read | Phantom | Conflict Detection | Cost |
|---|---|---|---|---|---|
| **SERIALIZABLE** | No | No | No | Predicate locks (2PC) | High; GATE-TXN-04 ≤50µs |
| **REPEATABLE_READ** | No | No | Yes (possible) | Version visibility (MVCC) | Medium; ≤40µs |
| **READ_COMMITTED** | No | Yes | Yes | Read lock (short-lived) | Low; ≤20µs |

## 2PC (Two-Phase Commit) Protocol

| Phase | Behavior | Timeout | Test |
|---|---|---|---|
| **Prepare** | Coordinator locks all involved keys; participants vote ready/abort | 30s | test_txn_2pc_prepare.cpp |
| **Commit** | Coordinator broadcasts commit; all participants write + ack | 30s | test_txn_2pc_commit.cpp |
| Recovery | On participant failure, coordinator retries or aborts; idempotent | N/A | test_txn_2pc_recovery.cpp |

## SAGA (Distributed Transactions)

| Aspect | Contract | Notes |
|---|---|---|
| Steps | Executed sequentially; each step may fail independently | No atomicity across steps (eventual consistency) |
| Compensation | Run in reverse order if any step fails | Must be idempotent; partial compensation OK |
| Timeout | Per-step (default 60s); cascade to saga if step times out | Compensation runs regardless |
| Idempotency | Each step must be idempotent or use dedup tokens | Required for fault tolerance |

## Concurrency & Lock Management

| Scenario | Behavior | Test |
|---|---|---|
| Write conflict (key updated by another txn) | SERIALIZABLE: abort; READ_COMMITTED: wait or fail | test_txn_write_conflict.cpp |
| Deadlock between 2 txns | Deadlock detector kills one (younger preferred) + rollback | test_txn_deadlock_detection.cpp |
| Long-running txn + compaction | Txn holds snapshot; compaction waits; no data loss | test_txn_snapshot_compaction.cpp |

## Invariants & ACID Guarantees

| Guarantee | Enforcement | Notes |
|---|---|---|
| **Atomicity** | Commit writes all-or-nothing to storage | Via WAL + write batching |
| **Consistency** | Isolation + validation rules prevent inconsistent state | Schema + foreign key checks (if enabled) |
| **Isolation** | MVCC + locking per isolation level | See table above |
| **Durability** | Committed writes survive crashes (fsync on commit) | SyncWrite default for txns |

## Error Categories

| Error | When | Recovery |
|---|---|---|
| WriteConflictError | Serializable txn finds write conflict during commit | Rollback + retry; exponential backoff recommended |
| DeadlockError | Circular lock dependency detected | Kill younger txn; retry with backoff |
| ValidationError | Schema/constraint violation | Fix data; retry |
| TimeoutError | Txn exceeds timeout_ms or 2PC phase exceeds 30s | Increase timeout or split txn |
| SavepointNotFoundError | Invalid savepoint handle | Check handle validity; use rollback instead |

## Performance Commitments (Release Gates)

| Gate | Latency | Isolation | Concurrency | Test |
|---|---|---|---|---|
| GATE-TXN-01 | begin() ≤10 µs | Any | 100 concurrent | bench_txn_release_gates.cpp |
| GATE-TXN-02 | put() ≤20 µs | SERIALIZABLE | 1 writer | bench_txn_write_gates.cpp |
| GATE-TXN-03 | get() ≤15 µs | SERIALIZABLE | 100 readers | bench_txn_read_gates.cpp |
| GATE-TXN-04 | commit() ≤50 µs | SERIALIZABLE (2PC) | Local commit | bench_txn_commit_gates.cpp |
| GATE-TXN-05 | rollback() ≤5 µs | Any | Any | (part of commit benchmark) |

## API Stability

| Item | Status | Notes |
|---|---|---|
| ITransactionCoordinator interface | Public v1.x | Frozen (breaking change = major version) |
| Isolation levels | Public v1.x | Stable; new levels require major bump |
| 2PC protocol | Internal | May evolve; external API unchanged |
| SAGA interface | Beta | Finalizes in Q4 2026 |
| SavepointHandle | Internal | Implementation detail |

---

**Zuletzt geprueft (Transaction contracts):** 2026-08-03
