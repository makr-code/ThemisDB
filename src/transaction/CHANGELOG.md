> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-05-31 -->
# Changelog — Transaction Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Fixed
- **Stub #279: Distributed Transaction Manager Phase-2 fail-closed behavior**
  - Added per-transaction validation in `beginDistributed()` to fail-fast when remote participants are registered without a Phase-2 transport bridge (RPC/mTLS decision fanout)
  - Modified Phase-2 delivery logic to implement fail-closed behavior instead of silently skipping participants, preventing them from remaining indefinitely in PREPARED state
  - Enhanced error handling in `commitDistributed()` and `abortDistributed()` to provide clear error details when Phase-2 bridge is missing
  - Added comprehensive security tests to verify fail-fast behavior and error propagation
  - Coordinator crash recovery now properly handles transactions aborted due to missing Phase-2 bridge

### Changed
- Documentation governance alignment pass:
	- `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md` kept future-focused.
	- `AUDIT.md`, `README.md`, `ARCHITECTURE.md`, `SECURITY.md`, and `PERFORMANCE_EXPECTATIONS.md` refreshed with sourcecode verification evidence blocks.
	- Historical implementation record remains centralized in `CHANGELOG.md`.

## [1.8.0] — 2026-03-15
### Added
- Named savepoints with partial rollback (`createSavepoint`, `rollbackToSavepoint`, `releaseSavepoint`, `getSavepoints`, `hasSavepoint`) — backs each named savepoint with RocksDB `SetSavePoint`/`RollbackToSavePoint`/`PopSavePoint`
- SAGA step trimming on `rollbackToSavepoint` — compensating actions registered after the savepoint are discarded, preventing incorrect compensation during full rollback
- Savepoint stacking with correct LIFO ordering — multiple overlapping savepoints are resolved in creation order
- Automatic savepoint cleanup on rollback and release — the target savepoint and all newer ones are always removed together
- CI workflow for Transaction Savepoints (`transaction-savepoints-ci.yml`) covering named savepoint API, SAGA trimming, and anonymous stack API on gcc-12, clang-15, and gcc-13

## [1.5.0] — 2026-03-12
### Added
- SAGA orchestration engine with fully-implemented compensating actions (relational, secondary-index, graph, vector)
- Serializable Snapshot Isolation (SSI) via predicate locking
- Git-like transaction branching and merging
- Named snapshots for point-in-time isolation
- Deadlock detection with wait-for graph cycle detection
- CDC changefeed integration for transaction events
- Transaction EXPLAIN: lock inspection and write-set analysis
- Multi-region Global Transaction Manager with TrueTime 2PC
- Distributed 2PC across shards via Raft coordination

## [1.0.0] — 2024-01-01
### Added
- ACID transactions on RocksDB via `TransactionDB`
- MVCC snapshot isolation
- Optimistic concurrency with write-conflict detection
- Savepoints within transactions
