---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1; SOT domain: api-contract. Canonical references: `transaction_manager.h`, `isolation_level.h`, `lock_manager.h`, `distributed_transaction_manager.h`, `transaction_coordinator.h`, and `saga.h`. Related issue: #7390.

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
| `transaction_coordinator.h` | `ITransactionCoordinator` | Protocol-agnostic interface; its protocol enum names 2PC, 3PC, SAGA, Percolator, Calvin, and custom protocols |
| `transaction_manager.h` | `TransactionManager` | Concrete local transaction manager |
| `global_transaction_manager.h` | `GlobalTransactionManager` | Cross-node transaction coordinator |
| `distributed_transaction_manager.h` | `DistributedTransactionManager`, `IDistributedParticipantCallback` | Concrete participant-based distributed transaction manager and callback contract |
| `isolation_level.h` | `IsolationLevel` (enum) | Isolation level enumeration |
| `snapshot_manager.h` | `SnapshotManager` | MVCC snapshot lifecycle |
| `lock_manager.h` | `LockManager` | Row- and range-level lock management |
| `deadlock_predictor.h` | `DeadlockPredictor` | Wait-for graph cycle detection |
| `crash_recovery_manager.h` | `CrashRecoveryManager` | WAL replay and in-doubt tx resolution |
| `in_doubt_recovery_coordinator.h` | `IInDoubtRecoveryCoordinator` | In-doubt recovery interface |
| `recoverable_two_phase_coordinator.h` | `IRecoverableTwoPhaseCoordinator` | Unified cross-coordinator 2PC recovery and reporting |
| `transaction_auditor.h` | `TransactionAuditor` | Per-transaction audit trail |
| `transaction_batcher.h` | `TransactionBatcher` | Micro-batch grouping for write throughput |
| `transaction_semantic_advisor.h` | `TransactionSemanticAdvisor` | Application-level conflict advice |
| `merge_engine.h` | `MergeEngine` | MVCC merge resolution engine |
| `branch_manager.h` | `BranchManager` | Named branch lifecycle management |
| `saga.h` | `Saga` | Compensation callback collection and execution |
| `saga_orchestrator.h` | `SagaOrchestrator` | Long-running saga coordination |
| `distributed_saga.h` | `DistributedSaga` | Cross-shard saga with compensation |
| `saga_plugin_bridge.h` | `SagaPluginBridge` | Plugin-host bridge for saga steps |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis` | Local `TransactionManager`, `IsolationLevel`, `LockManager`, and `Saga` |
| `themis::transaction` | Distributed coordinator and recovery-related interfaces |
| `themis::transaction::saga` | TODO: verify namespace before documenting |

---

## 4. Isolation Levels

`isolation_level.h` defines the canonical `themis::IsolationLevel` enum:

```
READ_UNCOMMITTED → READ_COMMITTED → REPEATABLE_READ → SERIALIZABLE
```

`Snapshot` is a legacy alias for `REPEATABLE_READ`; `SerializableSnapshot` is an alias for `SERIALIZABLE`. **TODO:** identify a current configuration source before claiming a distributed default-isolation environment variable.

---

## 5. Saga Protocol

The saga pattern is split across three headers for extensibility:
- `saga.h` — `Saga` compensation callbacks (`addStep()`, `compensate()`)
- `saga_orchestrator.h` — ordered saga execution with compensation on failure
- `distributed_saga.h` — cross-shard saga with RPC-based compensation
- `saga_plugin_bridge.h` — host-side bridge for dynamically loaded saga step plugins

---

## 6. Relationship to Strategic Architecture

The header/source set does not establish the strategic-layer guarantees described in earlier versions of this section. **TODO:** add concrete consumer/source references before documenting cross-layer responsibilities or ANN/LLM integration as implemented behavior.

---

## 7. Unified Coordinator Interface

`transaction_coordinator.h` declares `ITransactionCoordinator` and a `CommitProtocol`
enum naming multiple protocols. This declaration alone does not establish concrete
implementations for all those protocols. See
[`docs/ITRANSACTION_COORDINATOR.md`](../../docs/ITRANSACTION_COORDINATOR.md) for
the interface specification, ASCII-UML hierarchy, and the phased migration plan
for existing coordinators.
