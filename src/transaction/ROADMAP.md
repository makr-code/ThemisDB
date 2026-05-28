> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

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
- [x] Write Batching and Coalescing – `TransactionBatcher` with configurable batch window (1–100 ms), per-table/per-key policies, fair FIFO scheduling, adaptive batch sizing, and aggregate stats (Target: v1.8.0)
- [x] Read-Only Transaction Optimization – `Transaction::setReadOnly()`, `isReadOnly()`, `hasWrites()` with write-guard on all mutation paths and no-op WAL commit fast-path (Target: v1.8.0)
- [x] Transaction Audit Trail – `TransactionAuditor` with append-only in-memory log, `enableAuditing()`, `record()`, `queryAuditLog()` (filters: user_id, start/end time, limit), `size()`, `clear()`, thread-safe concurrent recording; `exportToKafka()` / `exportToS3()` stubs (Target: v1.8.0)
- [x] Distributed SAGA Coordinator – multi-cluster orchestration (`RemoteStep` + `executeDistributed()`), crash recovery (`recoverInProgressSAGAs()`), SAGA visualization (`visualize()`), manual intervention (`forceCompensate()` / `forceComplete()`), pluggable `RemoteStepExecutor` transport (Target: v1.9.0) (Issue: #124)

## In Progress 🚧
> All Phase 3, Phase 4, Phase 5, Phase 6, and Phase 7 items are now complete.


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
- [x] Write Batching and Coalescing (Target: v1.8.0)
  — implemented in `include/transaction/transaction_batcher.h`, `src/transaction/transaction_batcher.cpp`;
  standalone `TransactionBatcher` with background flush thread, `submitAsync(commit_fn, table_hint)`,
  `flush()`, per-table `BatchPolicy`, adaptive window, and `Stats`; 26 tests in
  `tests/test_transaction_batcher.cpp` (`TransactionBatcherFocusedTests`);
  CI: `.github/workflows/transaction-write-batching-ci.yml`
- [x] Read-Only Transaction Optimization (Target: v1.8.0)
  — implemented in `include/transaction/transaction_manager.h`, `src/transaction/transaction_manager.cpp`;
  `Transaction::setReadOnly(bool)`, `isReadOnly()`, `hasWrites()` with write-guard on all mutation
  paths (`putEntity`, `eraseEntity`, `addEdge`, `deleteEdge`, `addVector`, `updateVector`, `removeVector`,
  `optimisticPut`, `optimisticErase`, `bulkPutEntities`, `bulkEraseEntities`) and a no-op WAL commit
  fast-path for read-only transactions.
- [x] Transaction Audit Trail (Target: v1.8.0)
  — implemented in `include/transaction/transaction_auditor.h`, `src/transaction/transaction_auditor.cpp`;
  standalone `TransactionAuditor` with `enableAuditing()`, `record()`, `queryAuditLog()` (filters:
  user_id, start/end time, limit; sorted most-recent-first), `size()`, `clear()`, and thread-safe
  concurrent recording; `exportToKafka()` / `exportToS3()` placeholder stubs; 25 tests in
  `tests/test_transaction_auditor.cpp` (`TransactionAuditorFocusedTests`);
  CI: `.github/workflows/transaction-audit-trail-ci.yml`
- [x] Distributed SAGA Coordinator (Target: v1.9.0) (Issue: #124)
  — extended `DistributedSagaCoordinator` with:
  `executeDistributed()` (multi-cluster via `RemoteStep` + `RemoteStepExecutor`),
  `recoverInProgressSAGAs()` (journal-based crash recovery),
  `getDistributedStatus()` (cross-cluster status query),
  `visualize()` (Graphviz DOT + text-summary SAGA graph),
  `forceCompensate()` / `forceComplete()` (manual intervention API);
  tests added in `tests/test_distributed_saga.cpp`

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
- [x] Tests: `tests/test_adaptive_deadlock_prevention.cpp` (34 tests, `AdaptiveDeadlockPreventionFocusedTests`)
- [x] CI: `.github/workflows/adaptive-deadlock-prevention-ci.yml`

### Phase 6: Distributed Transaction Coordinator (2PC) (Status: Completed ✅)
- [x] `DistributedTransactionManager` – Two-Phase Commit coordinator for multi-shard distributed transactions (Target: v1.9.0) (Issue: #123)
  — implemented in `include/transaction/distributed_transaction_manager.h`, `src/transaction/distributed_transaction_manager.cpp`
- [x] `IDistributedParticipantCallback` – shard participant interface (`onPrepare`, `onCommit`, `onAbort`)
- [x] Coordinator API: `beginDistributed`, `prepareDistributed`, `commitDistributed`, `abortDistributed`
- [x] Participant API: `voteOnPrepare`, `applyCommit`, `applyAbort`
- [x] Phase 1 (prepare): parallel calls to all participants with configurable timeout
- [x] Phase 2 (commit/abort): parallel broadcast with deadline; COMMIT_TX/ABORT_TX durably logged to WAL before broadcasting
- [x] WAL logging via `themis::sharding::WALManager` (BEGIN_TX/PREPARE_TX/COMMIT_TX/ABORT_TX)
- [x] Coordinator crash recovery: `recoverInDoubtTransactions()` re-drives PREPARED-but-undecided txns → ABORT
- [x] Timeout-based abort: `checkTimeouts()` non-blocking scan for network partition detection
- [x] Failure detection: `isParticipantAlive()` for participant health checks
- [x] Participant crash: prepare exception treated as ABORT vote (safe conservative choice)
- [x] Configurable timeouts: `prepare_timeout`, `commit_timeout`, `default_txn_timeout`
- [x] Statistics: `getStatistics()` returns committed/aborted/timeout_aborts/recovered/in_doubt counts
- [x] Tests: `tests/test_transaction_distributed_2pc.cpp` (32 tests, `DistributedTxnManagerTest`; 43 total after PERF-D4)
- [x] CI: `.github/workflows/transaction-distributed-2pc-ci.yml`

### Phase 6b: PERF-D4 – Batched Prepare & Lock-Free Coordination (Status: Completed ✅)
- [x] Thread pool (`worker_thread_count`, default 4): fixed-size pool replaces per-call `std::async`, eliminates OS thread-creation overhead (Target: v2.0.0) (Issue: PERF-D4)
  — `startThreadPool()` / `stopThreadPool()` / `submitTask<F>()` in `distributed_transaction_manager.h/.cpp`
- [x] `prepare_batch_window` config (0–100 ms): when > 0ms, `prepareDistributed()` callers are queued and flushed in one parallel wave by the background `batchFlushLoop()` thread
- [x] `batchFlushLoop()`: background thread that drains the batch queue every `prepare_batch_window` ms; all queued Phase-1 calls submitted concurrently to the thread pool
- [x] O(1) transaction lookup: `transactions_` changed from `std::map` (O(log n)) to `std::unordered_map` (O(1))
- [x] Legacy mode (`worker_thread_count=0`): falls back to `std::async(launch::async)` per call (no regression)
- [x] Graceful shutdown: destructor stops batch thread first (delivering `false` to any pending futures), then drains thread pool
- [x] Throughput: ≥ 10k ops/s (measured: ~29k ops/s, 8 workers) — satisfies PERF-D4 SLO
- [x] P99 latency: < 100 ms for 5-shard transactions (measured: 0.13 ms)
- [x] Tests: 11 new tests in `Distributed2PCPerfTests` suite (43 tests total), 2 performance tests gated by `THEMIS_RUN_PERF_TESTS=1`

### Phase 7: Write Batching and Coalescing (Status: Completed ✅)
- [x] `TransactionBatcher` – automatic batching of concurrent commit operations for high-throughput ingestion (Target: v1.8.0)
  — implemented in `include/transaction/transaction_batcher.h`, `src/transaction/transaction_batcher.cpp`
- [x] `BatchConfig` – configurable `window` (1–100 ms), `max_batch_size`, `min_batch_size`, `enable_adaptive`
- [x] `setBatchConfig()` / `getBatchConfig()` – validated and clamped configuration API
- [x] `submitAsync(commit_fn, table_hint)` – non-blocking submission returning `std::future<Status>`
- [x] `flush()` – force-drain all pending items immediately (blocks until queue is empty)
- [x] Background flush thread with condition_variable for efficient timed wait
- [x] Immediate flush when pending queue reaches `max_batch_size` (throughput protection)
- [x] Exception safety: all exceptions thrown by commit_fn are caught and returned as `Status::Error`
- [x] FIFO ordering: items processed in submission order within each batch (fair scheduling, no starvation)
- [x] Per-table `BatchPolicy` – `setTablePolicy(table, policy)` / `getTablePolicy(table)` with zero-field inheritance from global config
- [x] Adaptive window adjustment: widens under low load (+10%), narrows under near-overflow load (-10%)
- [x] `Stats` – `batches_flushed`, `transactions_committed`, `transactions_failed`, `avg_batch_size`, `avg_latency_ms`, `adaptive_adjustments`
- [x] Destructor drains remaining items before exiting (prevents lost commits on shutdown)
- [x] Thread-safe: `submitAsync`, `setBatchConfig`, `setTablePolicy`, `flush`, `getStats` all safe for concurrent callers
- [x] Tests: `tests/test_transaction_batcher.cpp` (26 tests, `TransactionBatcherFocusedTests`)
- [x] CI: `.github/workflows/transaction-write-batching-ci.yml`

### Phase 8: Read-Only Optimization & Transaction Audit Trail (Status: Completed ✅)
- [x] `Transaction::setReadOnly(bool)` – mark a transaction as read-only; returns error if writes already exist (Target: v1.8.0)
  — implemented in `include/transaction/transaction_manager.h`, `src/transaction/transaction_manager.cpp`
- [x] `Transaction::isReadOnly()` – returns the current read-only flag
- [x] `Transaction::hasWrites()` – returns true when the write set is non-empty
- [x] Write-guard on all mutation paths: `putEntity`, `eraseEntity`, `addEdge`, `deleteEdge`, `addVector`, `updateVector`, `removeVector`, `optimisticPut`, `optimisticErase`, `bulkPutEntities`, `bulkEraseEntities` all reject writes when `read_only_` is set
- [x] Read-only fast-path in `commit()`: releases the RocksDB snapshot without writing to the WAL when `read_only_` is true
- [x] `TransactionAuditor` – standalone, thread-safe, append-only in-memory transaction audit log (Target: v1.8.0)
  — implemented in `include/transaction/transaction_auditor.h`, `src/transaction/transaction_auditor.cpp`
- [x] `AuditRecord` – full transaction record: txn_id, user_id, session_id, timestamp, isolation level, operations, result, duration_us
- [x] `Operation` – per-mutation record: type (PUT/DELETE/ADD_EDGE/DELETE_EDGE/ADD_VECTOR), table, key, old_value, new_value
- [x] `enableAuditing(bool)` / `isEnabled()` – toggle and query auditing state
- [x] `record(AuditRecord)` – append-only record under mutex; no-op when auditing is disabled
- [x] `queryAuditLog(user_id, start_time, end_time, limit)` – filtered query, sorted most-recent-first, capped at limit (default 1000; 0 = all)
- [x] `size()` / `clear()` – count and reset operations for the in-memory log
- [x] `exportToKafka(topic)` / `exportToS3(bucket, prefix)` – placeholder stubs returning `Status::Error` (future integration)
- [x] Thread-safe: `enableAuditing`, `record`, `queryAuditLog`, `size`, `clear` all safe for concurrent callers
- [x] Tests: `tests/test_transaction_auditor.cpp` (25 tests, `TransactionAuditorFocusedTests`)
- [x] CI: `.github/workflows/transaction-audit-trail-ci.yml`

### Phase 9: Distributed SAGA Coordinator (Status: Completed ✅)
- [x] Multi-cluster orchestration via `RemoteStep` + `DistributedSAGADefinition` structs (Target: v1.9.0) (Issue: #124)
  — implemented in `include/transaction/distributed_saga.h`, `src/transaction/distributed_saga.cpp`
- [x] `RemoteStep` – per-step record with `service_endpoint`, `operation`, `params`, `compensate_operation`, `compensate_params`, dependency DAG, per-step timeouts/retries
- [x] `DistributedSAGADefinition` – saga_id + vector of `RemoteStep` + shared `context` map
- [x] `RemoteStepExecutor` – pluggable transport `std::function<DistributedSagaStatus(endpoint, op, params)>`; nil = no-op (test mode)
- [x] `executeDistributed(DistributedSAGADefinition)` – converts remote steps to local steps via `remoteStepToLocal()` then delegates to `execute()`
- [x] `getDistributedStatus(saga_id)` – alias for `getReport()` for cross-cluster status queries
- [x] `recoverInProgressSAGAs()` – journal-based crash recovery: parses JSON-lines journal, identifies SAGAs with STARTED/COMPENSATING but no terminal entry, creates synthetic FAILED reports; logs RECOVERED event
- [x] `SagaVisualization` – Graphviz DOT graph + plain-text summary; nodes colour-coded by step phase (green=DONE, red=FAILED, yellow=COMPENSATED, blue=in-progress)
- [x] `visualize(DistributedSagaDefinition)` – generates DOT + text for both pre-execution (dependency structure) and post-execution (annotated with phase, attempts, errors)
- [x] `forceCompensate(saga_id)` – manual intervention: marks report as COMPENSATED without executing compensation; returns false if unknown
- [x] `forceComplete(saga_id)` – manual intervention: marks report as COMPLETED; returns false if unknown
- [x] `remote_executor` field added to `DistributedSagaCoordinatorConfig`
- [x] Tests: `tests/test_distributed_saga.cpp` — 15+ new tests covering `DistributedSagaDistributedTest`, `DistributedSagaStatusTest`, `DistributedSagaRecoveryTest`, and new `DistributedSagaTest` cases for visualize/forceCompensate/forceComplete

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Verified: Q1 2026) — Primary: `tests/test_savepoints.cpp` (20 savepoint tests); bulk API: `tests/test_transaction_bulk.cpp` (12 tests); SagaOperation: `tests/test_saga_operation.cpp` (8 tests covering `indexPutWithCompensation`, `graphAddWithCompensation`, `putEntityWithCompensation`, `deleteEntityWithCompensation`, `vectorAddWithCompensation`); supplementary: `tests/test_transaction_isolation_levels.cpp`, `tests/test_transaction_manager.cpp`, `tests/test_postgres_transactions.cpp`; standalone focused targets: `TransactionManagerFocusedTests`, `TransactionIsolationLevelsFocusedTests`, `SAGALoggerFocusedTests`, `SAGACompactorFocusedTests`, `ShardingTransactionWALFocusedTests`, `MultiShardTransactionFocusedTests`, `DistributedTransactionsFocusedTests`, `PostgresTransactionFocusedTests`, `AQLMultiStatementTransactionFocusedTests`, `DbTransactionIsolationFocusedTests`, `TransactionDistributed2PCFocusedTests`, `TransactionBatcherFocusedTests`, `TransactionAuditorFocusedTests`
- [x] Integration tests (commit, rollback, SAGA compensation, deadlock detection) — savepoint+SAGA integration covered in `test_savepoints.cpp`; bulk API atomicity in `test_transaction_bulk.cpp`; DistributedSAGA in `test_distributed_saga.cpp` (DAG execution, retry, compensation ordering, metrics, executeDistributed, recoverInProgressSAGAs, visualize, forceCompensate, forceComplete); concurrent SAGA in `test_saga_concurrent_execution.cpp`; 2PC coordinator in `test_transaction_distributed_2pc.cpp` (concurrent transactions, partial commit rollback, prepare timeout)
- [x] Performance benchmarks (TPS, lock contention, MVCC overhead) — `OccOptimisticPut`, `OccReadVersionAndUpdate`, `OccOptimisticErase`, `SavepointCreateAndRollback`, `SavepointNested`, `SavepointRelease` in `benchmarks/bench_transaction_throughput.cpp`
- [x] Security audit (transaction isolation boundary, SAGA compensating action safety) — isolation boundary enforced via `LockManager` (EXCLUSIVE locks block SHARED readers; shrinking-phase enforcement prevents new lock acquisitions after first release, tested in `TransactionIsolationLevelsFocusedTests`); SAGA compensation safety verified via idempotent compensating functions in `test_saga_operation.cpp` and `test_distributed_saga.cpp`
- [x] Documentation complete — named savepoint API documented in `src/transaction/README.md`; bulk API documented in `include/transaction/transaction_manager.h`; time-travel query API documented in `include/transaction/transaction_manager.h`; 2PC coordinator API documented in `include/transaction/distributed_transaction_manager.h`; `TransactionBatcher` API documented in `include/transaction/transaction_batcher.h`; `TransactionAuditor` API documented in `include/transaction/transaction_auditor.h`; read-only optimization documented in `include/transaction/transaction_manager.h`; `FUTURE_ENHANCEMENTS.md` updated; `ROADMAP.md` updated
- [x] API stability guaranteed — `TransactionManager` public API stable from v1.x; savepoint API added as non-breaking extension; `DistributedTransactionManager` API stable from v1.9.0; `TransactionBatcher` API stable from v1.8.0; `TransactionAuditor` API stable from v1.8.0; read-only optimization API stable from v1.8.0

## Known Issues & Limitations
- Individual `Transaction` objects are NOT thread-safe; use from a single thread.
- Serializable isolation is implemented via predicate locking (SSI); SERIALIZABLE transactions acquire predicate locks on read ranges and detect write conflicts at write time.
- Cross-shard 2PC is available at three levels: `themis::sharding::TwoPhaseCommitCoordinator` (v1.5.0, sharding-layer), `themis::storage::DistributedTransactionManager` (v1.7.0, storage-layer), and `themis::transaction::DistributedTransactionManager` (v1.9.0, transaction-domain — this implementation).
- `TransactionBatcher` uses a type-erased `std::function<Status()>` as the commit unit, allowing full flexibility at the cost of one virtual dispatch per submitted item.
- **[DTM-1]** ✅ **Resolved (2026-05-28)** — `runPhase1Unlocked()` now invokes remote participants via injected `phase1_rpc_fn`, `remote_phase1_dispatch`, or the static `setRpcPhase1Fn()` bridge. Fail-closed ABORT when no bridge is configured. Backwards-compat path retained with a WARN when only a Phase-2 bridge is available.
- **[DTM-2]** ✅ **Resolved** — `recoverInDoubtTransactions()` now logs ABORT to WAL first, then broadcasts ABORT to any in-memory participants via `runPhase2Unlocked()` so they release locks instead of remaining PREPARED indefinitely.
- **[DTM-3]** ✅ **Resolved (2026-05-28)** — `isParticipantAlive()` now supports injectable liveness bridges: per-instance `liveness_check_fn` in `DistributedTxnManagerConfig`, process-wide `setLivenessCheckFn()` static bridge, and conservative `false` default for remote participants when no bridge is configured. In-process participants (non-null callback) remain unconditionally alive.

## Breaking Changes
- `TransactionManager` public API is stable from v1.x.
- SAGA compensating action interface may gain new lifecycle hooks in v1.5.0.
- Branch/merge API is new; no stability guarantees until v1.7.0.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### ✅ Aktiv (implementiert + externer Aufrufer bestätigt)

- `BranchManager` – Verwaltet Branch-Transaktionen; genutzt in BranchApiHandler + HttpServer

