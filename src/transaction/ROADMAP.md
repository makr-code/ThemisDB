# Transaction Module Roadmap

## Current Status
v1.x – Production-grade ACID transaction engine built on RocksDB. MVCC, SAGA pattern, deadlock detection, Git-like branching/merging, named snapshots, and CDC changefeed integration are all implemented.

## Completed ✅
- [x] TransactionManager – ACID guarantees via RocksDB WriteBatch
- [x] Isolation levels: ReadCommitted (default) and Snapshot
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

## In Progress 🚧
- [ ] Serializable isolation level (full SSI via predicate locking) (Target: Q2 2026)
- [ ] Two-phase commit (2PC) coordinator for cross-shard transactions (Target: Q2 2026)
- [ ] Transaction savepoints (partial rollback within a transaction) (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Optimistic concurrency control (OCC) mode as alternative to pessimistic locking
- [ ] Transaction timeout with automatic rollback
- [ ] Bulk transaction API (batch insert/update without per-row overhead)
- [ ] Transaction explain (show locks acquired, MVCC version chain)
- [ ] Per-tenant transaction isolation namespace

### Long-term (6-12 months)
- [ ] Distributed SAGA orchestration across multiple nodes
- [ ] Global transaction manager for multi-region ACID guarantees
- [ ] Calvin protocol for deterministic distributed transactions
- [ ] Time-travel queries against snapshot history
- [ ] Branch merge conflict resolution UI

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (commit, rollback, SAGA compensation, deadlock detection)
- [ ] Performance benchmarks (TPS, lock contention, MVCC overhead)
- [ ] Security audit (transaction isolation boundary, SAGA compensating action safety)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- Individual `Transaction` objects are NOT thread-safe; use from a single thread.
- Serializable isolation is not yet implemented; ReadCommitted and Snapshot only.
- 2PC for cross-shard distributed transactions is planned for v1.5.0.

## Breaking Changes
- `TransactionManager` public API is stable from v1.x.
- SAGA compensating action interface may gain new lifecycle hooks in v1.5.0.
- Branch/merge API is new; no stability guarantees until v1.7.0.
