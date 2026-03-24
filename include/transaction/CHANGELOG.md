<!-- Status: current | validated: 2026-03-22 -->

# Transaction Module — Public Header Changelog

> Full implementation changelog: [`../../src/transaction/CHANGELOG.md`](../../src/transaction/CHANGELOG.md)
> Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

---

## [1.8.0] — 2026-03-15

### Added
- `Transaction::setReadOnly(bool)` — rejects call if writes already exist in the transaction.
- `Transaction::isReadOnly()` — read accessor for the read-only flag.
- `Transaction::hasWrites()` — returns true if any write path has been exercised.
- Fast-path in `commit()`: when `read_only_=true`, calls `mvcc_txn_->rollback()` for snapshot
  release without touching the WAL.

### Changed
- Guards added to all 11 write paths to enforce `read_only_` semantics.

---

## [1.7.0] — 2026-01-10

### Added
- `saga_orchestrator.h` — durable `SagaOrchestrator` with persistent step journal.
- `distributed_saga.h` — cross-service `DistributedSaga` and `SagaStep` types.

---

## [1.6.0] — 2025-10-05

### Added
- `branch_manager.h` — `BranchManager` for branch-based transaction isolation workflows.

### Changed
- `snapshot_manager.h` — `Snapshot` now exposes `createdAt()` timestamp.

---

## [1.5.0] — 2025-07-20

### Added
- `transaction_batcher.h` — `TransactionBatcher` for group-commit throughput optimisation.

---

## [1.4.0] — 2025-04-15

### Added
- `deadlock_predictor.h` — waits-for graph deadlock predictor.

---

## [1.3.0] — 2025-01-18

### Added
- `transaction_auditor.h` — structured audit trail per transaction.

---

## [1.2.0] — 2024-09-30

### Added
- `merge_engine.h` — optimistic write-set merge for concurrent transactions.

---

## [1.1.0] — 2024-06-01

### Added
- `isolation_level.h` — `IsolationLevel` enum decoupled from `transaction_manager.h`.
- `global_transaction_manager.h` — cluster-wide TID registry.

---

## [1.0.0] — 2024-01-01

### Added
- Initial public headers: `transaction_manager.h`, `lock_manager.h`,
  `snapshot_manager.h`, `crash_recovery_manager.h`,
  `distributed_transaction_manager.h`, `saga.h`.
