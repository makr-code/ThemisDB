> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Transaction Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/transaction/`

---

## 1. Overview

The Transaction module provides ThemisDB's ACID-compliant transaction management: MVCC
via RocksDB TransactionDB, the SAGA pattern for distributed transactions, 2PC (Two-Phase
Commit), deadlock detection, Git-like branch/merge/snapshot operations, and crash recovery.
Transactions span all data models atomically: relational rows, document collections, graph
edges, secondary indexes, and vector embeddings.

---

## 2. Design Principles

- **ACID on a Single Node** – within one ThemisDB instance, transactions use RocksDB
  TransactionDB for full ACID guarantees.
- **SAGA for Distributed** – for multi-shard or multi-service transactions, SAGA
  (compensating transactions) is the default; 2PC is available for strong atomicity.
- **MVCC Isolation** – ReadCommitted (default) and Snapshot isolation levels are
  supported; writers never block readers.
- **Git-Like Branching** – `branch_manager.cpp` enables experimental writes on a branch
  without affecting the main data; `merge_engine.cpp` merges branches.
- **Crash Recovery** – `crash_recovery_manager.cpp` replays the WAL and SAGA log on
  startup to recover to a consistent state.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `transaction_manager.cpp` | ACID transaction lifecycle: begin, commit, rollback |
| `saga.cpp` | SAGA pattern: step execution + compensating action on failure |
| `lock_manager.cpp` | Lock acquisition and wait-for deadlock detection |
| `snapshot_manager.cpp` | Named MVCC snapshots / tags for PITR |
| `branch_manager.cpp` | Git-like branch creation and management |
| `merge_engine.cpp` | Branch merge with conflict resolution |
| `crash_recovery_manager.cpp` | WAL and SAGA log replay on restart |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│           Caller (query engine, server API)                     │
│   tx = transaction_manager.begin(); tx.put(...); tx.commit()    │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                  TransactionManager                              │
│                                                                  │
│  RocksDB TransactionDB WriteBatch:                              │
│    relational + document + graph + index + vector layers        │
│                                                                  │
│  Isolation: ReadCommitted | Snapshot                            │
│  LockManager: deadlock detection (wait-for graph)              │
└──────────────────────────┬──────────────────────────────────────┘
                           │ distributed txn?
         ┌─────────────────┴───────────────────────────┐
         │                                             │
┌────────▼──────────────────┐             ┌────────────▼──────────┐
│   SAGA Coordinator         │             │   2PC Coordinator     │
│   step1 → step2 → ...      │             │   prepare → commit    │
│   failure → compensate     │             │   (strong atomic)     │
└────────────────────────────┘             └───────────────────────┘
         │
┌────────▼──────────────────┐
│  CrashRecoveryManager      │
│  WAL replay + SAGA log     │
│  on restart                │
└────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 ACID Local Transaction

```
tx = transaction_manager.begin(isolation=SNAPSHOT)
    │
    ├─ tx.put("doc:users:123", {...})       → add to WriteBatch
    ├─ tx.put("idx:users:email:a@b:123")   → add secondary index
    ├─ tx.put("vec:embeddings:123")         → add vector
    │
    ├─ lock_manager: acquire row lock on "doc:users:123"
    │
    ├─ tx.commit() → RocksDB WriteBatch (atomic)
    │
    └─ cdc module notified of changes
```

### 4.2 SAGA (Distributed)

```
SAGA: transfer funds from account_A (shard 1) to account_B (shard 3)
    │
    ├─ Step 1: debit account_A (shard 1)
    │       success → record compensating action: credit account_A
    │
    ├─ Step 2: credit account_B (shard 3)
    │       failure →
    │               compensating: credit account_A (shard 1)
    │               SAGA marked FAILED + rollback complete
    │
    └─ all steps success → SAGA marked COMPLETE
```

### 4.3 Git-Like Branch

```
branch = branch_manager.create("experiment-2026")
    │
    ├─ all writes on this branch are isolated (copy-on-write)
    │
    ├─ query on branch: see branch writes + main data
    │
    └─ merge_engine.merge("experiment-2026" → "main"):
           conflict detection → resolve → apply WriteBatch to main
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/storage/` | RocksDB TransactionDB for MVCC |
| **Uses** | `src/cdc/` | Post-commit change notifications |
| **Used by** | `src/sharding/` | Distributed transaction coordination |
| **Used by** | `src/query/` | Transaction context for writes |
| **Used by** | `src/server/` | Transaction API endpoints |
| **Provides to** | `src/replication/` | WAL entries for replication |

---

## 6. Threading & Concurrency Model

- `TransactionManager` is thread-safe; each transaction uses its own RocksDB transaction.
- `Transaction` objects are NOT thread-safe; use from a single thread per transaction.
- `LockManager` uses a wait-for graph updated atomically; deadlock detection runs on a
  background thread.
- `BranchManager` uses a read-write lock on the branch registry.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| WriteBatch | All layers updated atomically in one RocksDB WriteBatch (no per-layer overhead) |
| MVCC | Read-only transactions never wait for writers |
| Snapshot isolation | Point-in-time reads are lock-free |
| SAGA async | Distributed SAGA steps execute concurrently per-shard |

---

## 8. Security Considerations

- Transaction IDs are globally unique (UUID); no sequential IDs that could be predicted.
- SAGA compensating actions are logged; compensation cannot be bypassed.
- Crash recovery replays WAL; encrypted WAL (at-rest encryption) protects sensitive data.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `transaction.isolation_level` | "read_committed" | Default isolation level |
| `transaction.deadlock_timeout_s` | 30 | Deadlock detection timeout |
| `transaction.saga.max_retries` | 3 | SAGA step retry count |
| `transaction.snapshot.retention_s` | 3600 | Snapshot retention |
| `transaction.branch.max_count` | 100 | Max active branches |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Deadlock detected | Abort lower-priority transaction; caller retries |
| SAGA step failure | Execute compensating actions; log FAILED |
| 2PC participant timeout | Abort; execute compensating actions |
| WAL replay failure (crash) | Log critical; alert operator; enter read-only mode |

---

## 11. Known Limitations & Future Work

- 2PC and 3PC are available but not the default (SAGA is preferred for resilience).
- Branch merge conflict resolution is manual (no auto-merge for write-write conflicts).
- Long-running SAGA transactions (hours) are supported but monitoring UI is planned.

---

## 12. References

- `src/transaction/README.md` — module overview
- `docs/transactions/` — transaction documentation
- `ARCHITECTURE.md` (root) — full system architecture
