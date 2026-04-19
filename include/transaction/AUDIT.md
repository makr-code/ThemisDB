<!-- Status: current | validated: 2026-04-19 -->

# Transaction Module — Public Header Audit

## Last Audit

| Field | Value |
|---|---|
| Date | 2026-04-19 |
| Auditor | ThemisDB Core Team |
| Status | ✅ Pass |
| Headers audited | 16 |

## Summary

All 16 public headers compile cleanly under C++17 with `-Wall -Wextra -Wpedantic`.
No internal implementation types are leaked through public interfaces.  All classes
provide documented move semantics or are explicitly non-copyable.

## Header Files Audited

| File | Exported Symbols | Notes |
|---|---|---|
| `branch_manager.h` | `BranchManager` | Stable API; v1.6+ |
| `crash_recovery_manager.h` | `CrashRecoveryManager` | Stable API |
| `deadlock_predictor.h` | `DeadlockPredictor` | Stable API; cycle-detection O(V+E) |
| `distributed_saga.h` | `DistributedSaga`, `SagaStep` | Stable API |
| `distributed_transaction_manager.h` | `DistributedTransactionManager` | Stable API |
| `global_transaction_manager.h` | `GlobalTransactionManager` | Stable API |
| `isolation_level.h` | `IsolationLevel` | Enum; ABI-stable |
| `lock_manager.h` | `LockManager`, `LockHandle` | Stable API |
| `merge_engine.h` | `MergeEngine` | Stable API |
| `saga.h` | `Saga`, `SagaTransaction` | Stable API |
| `saga_orchestrator.h` | `SagaOrchestrator` | Stable API; v1.7+ |
| `snapshot_manager.h` | `SnapshotManager`, `Snapshot` | Stable API |
| `transaction_auditor.h` | `TransactionAuditor` | Stable API |
| `transaction_batcher.h` | `TransactionBatcher` | Stable API; v1.5+ |
| `transaction_manager.h` | `TransactionManager`, `Transaction` | Core API; stable |
| `transaction_semantic_advisor.h` | `TransactionSemanticAdvisor` | ✅ Reviewed |

## Findings

- **PASS** — No raw pointer ownership leaks in public APIs; all owning pointers use `std::unique_ptr` or `std::shared_ptr`.
- **PASS** — `setReadOnly(bool)` correctly documented to throw if writes already exist (v1.8.0).
- **PASS** — `isReadOnly()` and `hasWrites()` accessors added in v1.8.0 and reflected in header docs.
- **PASS** — All saga headers document compensation-step ordering guarantees.
- **NOTE** — `distributed_transaction_manager.h` references an internal `ShardMap` type via forward declaration only; acceptable.
