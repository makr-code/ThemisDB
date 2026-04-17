<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/transaction/ -->

# Transaction Module — Public Header Architecture

## Overview

The `include/transaction/` headers expose ThemisDB's full ACID transaction layer.
They cover single-node and distributed transactions, multi-version concurrency
control (MVCC), deadlock prevention, saga-based distributed compensation, branch
workflows, and throughput-oriented batching.  All implementation details live in
`../../src/transaction/`.

## Design Principles

- **Separation of concerns** — each header owns exactly one well-defined subsystem.
- **MVCC-first isolation** — snapshots are the primary currency; locks are a last resort.
- **Saga / compensation pattern** — distributed failures trigger structured rollback
  chains rather than two-phase-commit blocking.
- **Read-only fast path** — transactions flagged `read_only_=true` skip WAL writes
  and call `mvcc_txn_->rollback()` (snapshot release only) on commit.
- **Predictive deadlock prevention** — the deadlock predictor detects cycles before
  they form rather than detecting them post-hoc.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `branch_manager.h` | `BranchManager` | Branch-based transaction workflow isolation |
| `crash_recovery_manager.h` | `CrashRecoveryManager` | WAL-driven crash recovery and redo/undo |
| `deadlock_predictor.h` | `DeadlockPredictor` | Waits-for graph analysis and cycle prevention |
| `distributed_saga.h` | `DistributedSaga`, `SagaStep` | Distributed compensation sequences |
| `distributed_transaction_manager.h` | `DistributedTransactionManager` | Cross-shard 2PC coordination |
| `global_transaction_manager.h` | `GlobalTransactionManager` | Cluster-wide transaction ID registry |
| `isolation_level.h` | `IsolationLevel` (enum) | READ_COMMITTED, REPEATABLE_READ, SERIALIZABLE |
| `lock_manager.h` | `LockManager`, `LockHandle` | Row/range locking with escalation |
| `merge_engine.h` | `MergeEngine` | Optimistic merge of concurrent write sets |
| `saga.h` | `Saga`, `SagaTransaction` | Local saga pattern base types |
| `saga_orchestrator.h` | `SagaOrchestrator` | Durable orchestration of saga step sequences |
| `snapshot_manager.h` | `SnapshotManager`, `Snapshot` | MVCC snapshot lifecycle |
| `transaction_auditor.h` | `TransactionAuditor` | Per-transaction audit trail emission |
| `transaction_batcher.h` | `TransactionBatcher` | Group-commit batching for throughput |
| `transaction_manager.h` | `TransactionManager`, `Transaction` | Core ACID transaction API |
| *(planned)* `transaction_semantic_advisor.h` | `TransactionSemanticAdvisor`, `BatchAffinityHint` | Layer 5: conflict-aware batch ordering and retry-reduction hints (IMPL-B5) |

## Notes

- `transaction_manager.h` is the primary entry point for application code.
- `isolation_level.h` is a dependency-free enum header safe to include anywhere.
- Guards on all 11 write paths ensure `read_only_` is enforced at the API surface.
- `TransactionSemanticAdvisor` (planned, IMPL-B5) consults `DeadlockPredictor` scores and writes `DecisionRecord` to `AIDecisionAuditor` for every non-trivial hint.

---
*Implementation in `../../src/transaction/`*
