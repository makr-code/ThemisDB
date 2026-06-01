> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/transaction/ARCHITECTURE.md -->

# Transaction Module — Public Header Architecture

**Module Path:** `include/transaction/`
**Implementation:** `../../src/transaction/`
**Canonical architecture doc:** [`../../src/transaction/ARCHITECTURE.md`](../../src/transaction/ARCHITECTURE.md)

---

## 1. Overview

`include/transaction/` defines the **public transaction management contract** for ThemisDB. Headers cover local and distributed transaction managers, MVCC snapshot isolation, deadlock detection, saga orchestration, lock management, crash recovery, and branch management.

For full protocol details — 2PC/3PC flows, Raft log integration, saga rollback chains — see:
→ [`../../src/transaction/ARCHITECTURE.md`](../../src/transaction/ARCHITECTURE.md)

---

## 2. Header Inventory

| Header | Public Type | Purpose |
|--------|------------|---------|
| `transaction_manager.h` | `ITransactionManager` | Core local transaction interface |
| `global_transaction_manager.h` | `GlobalTransactionManager` | Cross-node transaction coordinator |
| `distributed_transaction_manager.h` | `IDistributedTransactionManager` | Distributed 2PC/3PC interface |
| `isolation_level.h` | `IsolationLevel` (enum) | Isolation level enumeration |
| `snapshot_manager.h` | `SnapshotManager` | MVCC snapshot lifecycle |
| `lock_manager.h` | `LockManager` | Row- and range-level lock management |
| `deadlock_predictor.h` | `DeadlockPredictor` | Wait-for graph cycle detection |
| `crash_recovery_manager.h` | `CrashRecoveryManager` | WAL replay and in-doubt tx resolution |
| `in_doubt_recovery_coordinator.h` | `InDoubtRecoveryCoordinator` | In-doubt 2PC transaction recovery |
| `transaction_auditor.h` | `TransactionAuditor` | Per-transaction audit trail |
| `transaction_batcher.h` | `TransactionBatcher` | Micro-batch grouping for write throughput |
| `transaction_semantic_advisor.h` | `TransactionSemanticAdvisor` | Application-level conflict advice |
| `merge_engine.h` | `MergeEngine` | MVCC merge resolution engine |
| `branch_manager.h` | `BranchManager` | Named branch lifecycle management |
| `saga.h` | `ISaga` | Saga step interface |
| `saga_orchestrator.h` | `SagaOrchestrator` | Long-running saga coordination |
| `distributed_saga.h` | `DistributedSaga` | Cross-shard saga with compensation |
| `saga_plugin_bridge.h` | `SagaPluginBridge` | Plugin-host bridge for saga steps |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::txn` | Core transaction manager types |
| `themis::txn::saga` | Saga and distributed saga types |
| `themis::txn::recovery` | Crash recovery and in-doubt resolution |

---

## 4. Isolation Levels

`isolation_level.h` defines the canonical `IsolationLevel` enum used across all transaction interfaces:

```
ReadUncommitted → ReadCommitted → RepeatableRead
→ SnapshotIsolation → Serializable
```

The default isolation for distributed transactions is configurable via `THEMIS_DTXN_DEFAULT_ISOLATION` (see `src/server/distributed_txn_api_handler.cpp`); per-request isolation takes precedence.

---

## 5. Saga Protocol

The saga pattern is split across three headers for extensibility:
- `saga.h` — the step interface (`ISaga::execute() / compensate()`)
- `saga_orchestrator.h` — ordered saga execution with compensation on failure
- `distributed_saga.h` — cross-shard saga with RPC-based compensation
- `saga_plugin_bridge.h` — host-side bridge for dynamically loaded saga step plugins

---

## 6. Relationship to Strategic Architecture

- **Graph Truth Layer**: `lock_manager.h` and `snapshot_manager.h` protect graph-truth store mutations
- **Tensor Mid-Layer**: `transaction_batcher.h` enables efficient tensor batch writes with MVCC semantics
- **LLM/LoRA Final Layer**: `transaction_semantic_advisor.h` can expose conflict semantics to an LLM decision layer
- **ANN Frontdoor**: index mutations are wrapped via `ITransactionManager` to ensure ANN index consistency
