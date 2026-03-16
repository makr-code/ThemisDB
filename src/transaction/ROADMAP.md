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
- [x] Adaptive Deadlock Prevention – `DeadlockPredictor` with probability scoring, lock-order recommendation, and adaptive timeouts (Target: v1.9.0)
- [x] Distributed Transaction Coordinator (2PC) – `DistributedTransactionManager` with parallel prepare/commit phases, WAL-backed coordinator crash recovery, timeout-based abort, and failure detection (Target: v1.9.0) (Issue: #123)

## In Progress 🚧
> All Phase 3, Phase 4, Phase 5, and Phase 6 items are now complete.


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
- [x] Adaptive Deadlock Prevention (Target: v1.9.0)
  — implemented in `include/transaction/deadlock_predictor.h`, `src/transaction/deadlock_predictor.cpp`;
  integrated into `TransactionManager` via `setDeadlockPredictor` / `predictDeadlockProbability` /
  `recommendLockOrder` / `recommendTimeout`; tests in `tests/test_adaptive_deadlock_prevention.cpp`

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

### Phase 5: Adaptive Deadlock Prevention (Status: Completed ✅)
- [x] `DeadlockPredictor` – ML-inspired deadlock probability scoring (Target: v1.9.0)
  — implemented in `include/transaction/deadlock_predictor.h`, `src/transaction/deadlock_predictor.cpp`
- [x] Historical deadlock pattern analysis via pair-conflict weight matrix
- [x] Lock acquire order recommendation (danger-score sort with lexicographic tie-break)
- [x] Dynamic timeout adjustment based on observed per-key hold-time percentiles
- [x] Deadlock probability scoring with active-transaction contention scaling
- [x] `TransactionManager` integration: `setDeadlockPredictor`, `predictDeadlockProbability`,
  `recommendLockOrder`, `recommendTimeout`
- [x] Automatic training: `recordTransaction` on commit/rollback; `recordDeadlock` on cycle resolution
- [x] Tests: `tests/test_adaptive_deadlock_prevention.cpp` (`AdaptiveDeadlockPreventionFocusedTests`)
- [x] CI: `.github/workflows/adaptive-deadlock-prevention-ci.yml`

### Phase 6: Distributed Transaction Coordinator (2PC) (Status: Completed ✅)
- [x] `DistributedTransactionManager` – Two-Phase Commit coordinator for multi-shard distributed transactions (Target: v1.9.0) (Issue: #123)
  — implemented in `include/transaction/distributed_transaction_manager.h`, `src/transaction/distributed_transaction_manager.cpp`
- [x] `IDistributedParticipantCallback` – shard participant interface (`onPrepare`, `onCommit`, `onAbort`)
- [x] Coordinator API: `beginDistributed`, `prepareDistributed`, `commitDistributed`, `abortDistributed`
- [x] Participant API: `voteOnPrepare`, `applyCommit`, `applyAbort`
- [x] Phase 1 (prepare): parallel `std::async` calls to all participants with configurable timeout
- [x] Phase 2 (commit/abort): parallel broadcast with deadline; COMMIT_TX/ABORT_TX durably logged to WAL before broadcasting
- [x] WAL logging via `themis::sharding::WALManager` (BEGIN_TX/PREPARE_TX/COMMIT_TX/ABORT_TX)
- [x] Coordinator crash recovery: `recoverInDoubtTransactions()` re-drives PREPARED-but-undecided txns → ABORT
- [x] Timeout-based abort: `checkTimeouts()` non-blocking scan for network partition detection
- [x] Failure detection: `isParticipantAlive()` for participant health checks
- [x] Participant crash: prepare exception treated as ABORT vote (safe conservative choice)
- [x] Configurable timeouts: `prepare_timeout`, `commit_timeout`, `default_txn_timeout`
- [x] Statistics: `getStatistics()` returns committed/aborted/timeout_aborts/recovered/in_doubt counts
- [x] Tests: `tests/test_transaction_distributed_2pc.cpp` (32 tests, `TransactionDistributed2PCFocusedTests`)
- [x] CI: `.github/workflows/transaction-distributed-2pc-ci.yml`

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Verified: Q1 2026) — Primary: `tests/test_savepoints.cpp` (20 savepoint tests); bulk API: `tests/test_transaction_bulk.cpp` (12 tests); SagaOperation: `tests/test_saga_operation.cpp` (8 tests covering `indexPutWithCompensation`, `graphAddWithCompensation`, `putEntityWithCompensation`, `deleteEntityWithCompensation`, `vectorAddWithCompensation`); supplementary: `tests/test_transaction_isolation_levels.cpp`, `tests/test_transaction_manager.cpp`, `tests/test_postgres_transactions.cpp`; standalone focused targets: `TransactionManagerFocusedTests`, `TransactionIsolationLevelsFocusedTests`, `SAGALoggerFocusedTests`, `SAGACompactorFocusedTests`, `ShardingTransactionWALFocusedTests`, `MultiShardTransactionFocusedTests`, `DistributedTransactionsFocusedTests`, `PostgresTransactionFocusedTests`, `AQLMultiStatementTransactionFocusedTests`, `DbTransactionIsolationFocusedTests`, `TransactionDistributed2PCFocusedTests`
- [x] Integration tests (commit, rollback, SAGA compensation, deadlock detection) — savepoint+SAGA integration covered in `test_savepoints.cpp`; bulk API atomicity in `test_transaction_bulk.cpp`; DistributedSAGA in `test_distributed_saga.cpp` (631 lines, DAG execution, retry, compensation ordering, metrics); concurrent SAGA in `test_saga_concurrent_execution.cpp`; 2PC coordinator in `test_transaction_distributed_2pc.cpp` (concurrent transactions, partial commit rollback, prepare timeout)
- [x] Performance benchmarks (TPS, lock contention, MVCC overhead) — `OccOptimisticPut`, `OccReadVersionAndUpdate`, `OccOptimisticErase`, `SavepointCreateAndRollback`, `SavepointNested`, `SavepointRelease` in `benchmarks/bench_transaction_throughput.cpp`
- [x] Security audit (transaction isolation boundary, SAGA compensating action safety) — isolation boundary enforced via `LockManager` (EXCLUSIVE locks block SHARED readers; shrinking-phase enforcement prevents new lock acquisitions after first release, tested in `TransactionIsolationLevelsFocusedTests`); SAGA compensation safety verified via idempotent compensating functions in `test_saga_operation.cpp` and `test_distributed_saga.cpp`
- [x] Documentation complete — named savepoint API documented in `src/transaction/README.md`; bulk API documented in `include/transaction/transaction_manager.h`; time-travel query API documented in `include/transaction/transaction_manager.h`; 2PC coordinator API documented in `include/transaction/distributed_transaction_manager.h`; `FUTURE_ENHANCEMENTS.md` updated; `ROADMAP.md` updated
- [x] API stability guaranteed — `TransactionManager` public API stable from v1.x; savepoint API added as non-breaking extension; `DistributedTransactionManager` API stable from v1.9.0

## Known Issues & Limitations
- Individual `Transaction` objects are NOT thread-safe; use from a single thread.
- Serializable isolation is implemented via predicate locking (SSI); SERIALIZABLE transactions acquire predicate locks on read ranges and detect write conflicts at write time.
- Cross-shard 2PC is available at three levels: `themis::sharding::TwoPhaseCommitCoordinator` (v1.5.0, sharding-layer), `themis::storage::DistributedTransactionManager` (v1.7.0, storage-layer), and `themis::transaction::DistributedTransactionManager` (v1.9.0, transaction-domain — this implementation).

## Breaking Changes
- `TransactionManager` public API is stable from v1.x.
- SAGA compensating action interface may gain new lifecycle hooks in v1.5.0.
- Branch/merge API is new; no stability guarantees until v1.7.0.
