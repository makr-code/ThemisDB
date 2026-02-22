<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Transaction Module Roadmap

## Current Status
v1.x – Production-grade ACID transaction engine built on RocksDB. MVCC, SAGA pattern, deadlock detection, full Serializable isolation (SSI via predicate locking), Git-like branching/merging, named snapshots, and CDC changefeed integration are all implemented.

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

## In Progress 🚧
- [x] Serializable isolation level (full SSI via predicate locking) (Target: Q2 2026) (Issue: #1439)
- [x] Two-phase commit (2PC) coordinator for cross-shard transactions (Target: Q2 2026) (Issue: #1440)
- [I] Transaction savepoints (partial rollback within a transaction) (Target: Q3 2026) (Issue: #2479)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Optimistic concurrency control (OCC) mode as alternative to pessimistic locking (Issue: #2475)
- [x] Transaction timeout with automatic rollback
- [!] Bulk transaction API (batch insert/update without per-row overhead) (Issue: #2476)
- [I] Transaction explain (show locks acquired, MVCC version chain) (Issue: #2477)
- [I] Per-tenant transaction isolation namespace (Issue: #2325)

### Long-term (6-12 months)
- [I] Distributed SAGA orchestration across multiple nodes (Issue: #2326)
- [I] Global transaction manager for multi-region ACID guarantees (Issue: #2327)
- [I] Calvin protocol for deterministic distributed transactions (Issue: #2328)
- [I] Time-travel queries against snapshot history (Issue: #2329)
- [I] Branch merge conflict resolution UI (Issue: #2478)

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
- [~] Transaction savepoints (partial rollback within a transaction)

### Phase 3: OCC Mode & Bulk API (Status: Planned 📋)
- [x] Optimistic concurrency control (OCC) mode as alternative to pessimistic locking
- [x] Transaction timeout with automatic rollback
- [ ] Bulk transaction API (batch insert/update without per-row overhead)
- [ ] Transaction explain (show locks acquired, MVCC version chain)
- [ ] Per-tenant transaction isolation namespace

### Phase 4: Distributed SAGA & Global Transaction Manager (Status: Planned 📋)
- [ ] Distributed SAGA orchestration across multiple nodes
- [ ] Global transaction manager for multi-region ACID guarantees
- [ ] Calvin protocol for deterministic distributed transactions
- [ ] Time-travel queries against snapshot history
- [ ] Branch merge conflict resolution UI

## Production Readiness Checklist
- [?] Unit tests coverage > 80%
- [?] Integration tests (commit, rollback, SAGA compensation, deadlock detection)
- [?] Performance benchmarks (TPS, lock contention, MVCC overhead)
- [?] Security audit (transaction isolation boundary, SAGA compensating action safety)
- [?] Documentation complete
- [?] API stability guaranteed

## Known Issues & Limitations
- Individual `Transaction` objects are NOT thread-safe; use from a single thread.
- Serializable isolation is implemented via predicate locking (SSI); SERIALIZABLE transactions acquire predicate locks on read ranges and detect write conflicts at write time.
- 2PC coordinator for cross-shard transactions is implemented in `themis::sharding::TwoPhaseCommitCoordinator` (v1.5.0).

## Breaking Changes
- `TransactionManager` public API is stable from v1.x.
- SAGA compensating action interface may gain new lifecycle hooks in v1.5.0.
- Branch/merge API is new; no stability guarantees until v1.7.0.
