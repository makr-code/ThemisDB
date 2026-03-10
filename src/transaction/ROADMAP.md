<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Transaction Module Roadmap

## Current Status
v1.x – Production-grade ACID transaction engine built on RocksDB. MVCC, SAGA pattern (with fully-implemented compensating actions for relational, secondary-index, graph, and vector operations), deadlock detection, full Serializable isolation (SSI via predicate locking), Git-like branching/merging, named snapshots, CDC changefeed integration, transaction explain (locks + write-set), and multi-region Global Transaction Manager (TrueTime 2PC) are all implemented across Phases 1–4.

## Completed ✅
- [x] TransactionManager – ACID guarantees via RocksDB WriteBatch
- [x] Isolation levels: ReadCommitted (default), Snapshot, and Serializable (SSI via predicate locking)
- [x] MVCC via RocksDB native transactions
- [x] Atomic multi-layer updates (relational, graph, vector, secondary indexes)
- [x] SAGA pattern with compensating actions for distributed transactions
- [x] Deadlock detection via background lock wait-graph analysis
- [x] Configurable deadlock timeout
- [x] Transaction statistics (begun, committed, aborted, active count, duration)
- [x] Lock-free stats with sequence lock pattern
- [x] Session-based long-lived transaction lifecycle
- [x] BranchManager – Git-like branching and merging
- [x] MergeEngine – conflict-aware branch merge
- [x] SnapshotManager – named snapshots/tags for PITR
- [x] Changefeed integration for CDC
- [x] Transaction savepoints – named partial rollback (`createSavepoint`, `rollbackToSavepoint`, `releaseSavepoint`, `getSavepoints`, `hasSavepoint`)
- [x] Transaction timeout with automatic rollback (`setTimeout`, `isTimedOut`, `setDefaultTransactionTimeout`, `getTimeoutCount`)
- [x] Optimistic concurrency control (OCC) – `getEntityVersion`, `optimisticPut`, `optimisticErase` with per-entity version numbers
- [x] Bulk transaction API – `bulkPutEntities`, `bulkEraseEntities` for batch insert/update/delete without per-row overhead
- [x] Serializable isolation level (full SSI via predicate locking) (Target: Q2 2026) (Issue: #1439)
- [x] Two-phase commit (2PC) coordinator for cross-shard transactions (Target: Q2 2026) (Issue: #1440)
- [x] Transaction savepoints (partial rollback within a transaction) (Target: Q3 2026) (Issue: #2479)
- [x] Optimistic concurrency control (OCC) mode as alternative to pessimistic locking (Issue: #2475)
- [x] Transaction timeout with automatic rollback
- [x] Bulk transaction API (batch insert/update without per-row overhead) (Issue: #2476)
- [x] Branch merge conflict resolution UI (Issue: #2478)
- [x] Transaction explain – `explain()` / `explainTransaction()` (show locks held, write set / MVCC version chain) (Issue: #2477)
- [x] SAGA compensation for secondary index operations (fixed: `SagaOperation::indexPutWithCompensation` now calls `idx.erase()`)
- [x] SAGA compensation for graph edge additions (fixed: `SagaOperation::graphAddWithCompensation` now calls `graph.deleteEdge()`)
- [x] Distributed SAGA orchestration across multiple nodes (Issue: #2326)
- [x] Global transaction manager for multi-region ACID guarantees with TrueTime 2PC (Issue: #2327)

## In Progress 🚧
> All Phase 3 and Phase 4 items are now complete.


## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Transaction explain (show locks acquired, MVCC version chain) (Issue: #2477)
  — implemented in `Transaction::explain()` / `TransactionManager::explainTransaction()`;
  tests in `tests/test_transaction_manager.cpp` and `tests/test_transaction_manager_comprehensive.cpp`

### Long-term (6-12 months)
- [x] Distributed SAGA orchestration across multiple nodes (Issue: #2326)
  — implemented in `include/transaction/distributed_saga.h`, `src/transaction/distributed_saga.cpp`,
  tests in `tests/test_distributed_saga.cpp`
- [x] Global transaction manager for multi-region ACID guarantees (Issue: #2327)
  — implemented in `include/transaction/global_transaction_manager.h`,
  `src/transaction/global_transaction_manager.cpp`, tests in `tests/test_global_transaction_manager.cpp`

## Implementation Phases

### Phase 1: ACID Engine & SAGA Pattern (Status: Completed ✅)
- [x] `TransactionManager` – ACID guarantees via RocksDB `WriteBatch`
- [x] Isolation levels: ReadCommitted (default) and Snapshot
- [x] MVCC via RocksDB native transactions
- [x] Atomic multi-layer updates (relational, graph, vector, secondary indexes)
- [x] SAGA pattern with compensating actions for distributed transactions
- [x] Deadlock detection via background lock wait-graph analysis
- [x] Configurable deadlock timeout
- [x] Transaction statistics with lock-free sequence lock pattern
- [x] Session-based long-lived transaction lifecycle
- [x] `BranchManager` – Git-like branching and merging
- [x] `MergeEngine` – conflict-aware branch merge
- [x] `SnapshotManager` – named snapshots/tags for PITR
- [x] Changefeed integration for CDC

### Phase 2: Serializable Isolation & Two-Phase Commit (Status: Completed ✅)
- [x] Serializable isolation level (full SSI via predicate locking)
- [x] Two-phase commit (2PC) coordinator for cross-shard transactions
- [x] Transaction savepoints (partial rollback within a transaction)

### Phase 3: OCC Mode & Bulk API (Status: Completed ✅)
- [x] Optimistic concurrency control (OCC) mode as alternative to pessimistic locking
- [x] Transaction timeout with automatic rollback
- [x] Bulk transaction API (batch insert/update without per-row overhead)
- [x] Transaction explain (show locks acquired, MVCC version chain)
- [x] Per-tenant transaction isolation namespace

### Phase 4: Distributed SAGA & Global Transaction Manager (Status: Completed ✅)
- [x] Distributed SAGA orchestration across multiple nodes
- [x] Global transaction manager for multi-region ACID guarantees
- [x] Calvin protocol for deterministic distributed transactions
- [x] Time-travel queries against snapshot history
- [x] Branch merge conflict resolution UI

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Verified: Q1 2026) — Primary: `tests/test_savepoints.cpp` (20 savepoint tests); bulk API: `tests/test_transaction_bulk.cpp` (12 tests); SagaOperation: `tests/test_saga_operation.cpp` (8 tests covering `indexPutWithCompensation`, `graphAddWithCompensation`, `putEntityWithCompensation`, `deleteEntityWithCompensation`, `vectorAddWithCompensation`); supplementary: `tests/test_transaction_isolation_levels.cpp`, `tests/test_transaction_manager.cpp`, `tests/test_postgres_transactions.cpp`
- [x] Integration tests (commit, rollback, SAGA compensation, deadlock detection) — savepoint+SAGA integration covered in `test_savepoints.cpp`; bulk API atomicity in `test_transaction_bulk.cpp`; DistributedSAGA in `test_distributed_saga.cpp` (631 lines, DAG execution, retry, compensation ordering, metrics); concurrent SAGA in `test_saga_concurrent_execution.cpp`
- [x] Performance benchmarks (TPS, lock contention, MVCC overhead) — `OccOptimisticPut`, `OccReadVersionAndUpdate`, `OccOptimisticErase`, `SavepointCreateAndRollback`, `SavepointNested`, `SavepointRelease` in `benchmarks/bench_transaction_throughput.cpp`
- [x] Security audit (transaction isolation boundary, SAGA compensating action safety) — isolation boundary enforced via `LockManager` (EXCLUSIVE locks block SHARED readers; shrinking-phase enforcement prevents new lock acquisitions after first release, tested in `TransactionIsolationLevelsFocusedTests`); SAGA compensation safety verified via idempotent compensating functions in `test_saga_operation.cpp` and `test_distributed_saga.cpp`
- [x] Documentation complete — named savepoint API documented in `src/transaction/README.md`; bulk API documented in `include/transaction/transaction_manager.h`; time-travel query API documented in `include/transaction/transaction_manager.h`; `FUTURE_ENHANCEMENTS.md` updated; `ROADMAP.md` updated
- [x] API stability guaranteed — `TransactionManager` public API stable from v1.x; savepoint API added as non-breaking extension

## Known Issues & Limitations
- Individual `Transaction` objects are NOT thread-safe; use from a single thread.
- Serializable isolation is implemented via predicate locking (SSI); SERIALIZABLE transactions acquire predicate locks on read ranges and detect write conflicts at write time.
- 2PC coordinator for cross-shard transactions is implemented in `themis::sharding::TwoPhaseCommitCoordinator` (v1.5.0).

## Breaking Changes
- `TransactionManager` public API is stable from v1.x.
- SAGA compensating action interface may gain new lifecycle hooks in v1.5.0.
- Branch/merge API is new; no stability guarantees until v1.7.0.
