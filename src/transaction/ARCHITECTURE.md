> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Transaction Module - Architecture Guide

**Version:** 1.1
**Last Updated:** 2026-09-09
**Module Path:** `src/transaction/`

## 1. Overview

The transaction module provides ACID-oriented lifecycle handling, isolation semantics, distributed coordination, SAGA orchestration, and operational helpers for batching/auditing.

## 2. Design Principles

- Correctness first for state transitions (`begin -> prepare -> commit/abort`).
- Isolation-level behavior is explicit in transaction manager APIs.
- Distributed coordination paths are durability-aware and recovery-aware.
- Compensation-based orchestration remains replay-safe and observable.

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `transaction_manager.cpp` | Transaction lifecycle and isolation behavior |
| `lock_manager.cpp` | Locking and contention/deadlock support |
| `distributed_transaction_manager.cpp` | Distributed prepare/commit/abort coordination |
| `saga.cpp` / `saga_orchestrator.cpp` | SAGA step and compensation orchestration |
| `distributed_saga.cpp` | Multi-node/distributed SAGA paths |
| `transaction_batcher.cpp` | Batching and throughput smoothing for commit units |
| `transaction_auditor.cpp` | Transaction audit recording/query surface |
| `deadlock_predictor.cpp` | Deadlock risk estimation helpers |

### 3.2 Data Flow (Simplified)

```
Caller
  -> TransactionManager::beginTransaction(...)
  -> Transaction operations (entity/index/graph/vector paths)
  -> commit/rollback

Distributed path
  -> DistributedTransactionManager::beginDistributed(...)
  -> prepareDistributed(...)
  -> commitDistributed(...) / abortDistributed(...)
  -> recovery helpers for in-doubt states

Compensation path
  -> Saga/SagaOrchestrator executes steps
  -> on failure: compensation in reverse-safe order
```

## 4. Integration Points

| Direction | Module | Interface |
|---|---|---|
| Uses | `src/storage/` | underlying storage and transactional persistence surfaces |
| Uses | `src/index/` | index update participation in transaction flows |
| Used by | `src/query/` | query-driven mutation transactions |
| Used by | `src/server/` | API-driven transaction endpoints |
| Used by | `src/sharding/` | distributed coordination and WAL-related integration |

## 5. Module Dependencies

### Direct Upstream Dependencies (this module uses)

| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| `storage` | `include/storage/rocksdb_wrapper.h`, `include/storage/history_manager.h` | Durable KV writes, MVCC history reads, WAL |
| `index` | `include/index/secondary/`, `include/index/graph/`, `include/index/vector/` | Index updates co-ordinated within transaction boundaries |
| `sharding` | `include/sharding/truetime.h`, `include/sharding/wal_manager.h` | TrueTime timestamps for distributed ordering; WAL append |
| `plugins` | `include/plugins/` | Plugin hooks called during commit/rollback lifecycle |
| `utils` | `include/utils/` | Utility helpers (error codes, logging) |

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/transaction/transaction_manager.h` | API-driven transaction endpoints |
| `query` | `include/transaction/transaction_manager.h` | Mutation transactions for DML statements |
| `sharding` | `include/transaction/recoverable_two_phase_coordinator.h` | Cross-shard 2PC coordination (managed circular dep) |
| `storage` | `include/transaction/snapshot_manager.h` | Storage calls back to snapshot manager for MVCC snapshots |

> **DOC-WEEKLY tracking note:** Batch-4 consumer entries (analytics, process, content, aql, cache, replication consumers of this module) have not yet been audited and added to this table. Extend in next DOC-WEEKLY pass.

---

## 6. Integration Points (Detailed)

### Critical Integration: Transaction → Storage (Persistence)
**Files:** `src/transaction/transaction_manager.cpp` ↔ `include/storage/rocksdb_wrapper.h`
**Contract:** Each `commit()` call drives a batch write to RocksDB via the wrapper; `rollback()` discards in-memory mutations. WAL entry written before commit return.
**Thread Safety:** `TransactionManager` is concurrent-safe; individual `Transaction` objects are single-owner.
**Failure Mode:** Storage write failure → transaction aborted; WAL write failure → `kDurabilityError` returned, no partial commit.

### Critical Integration: Transaction ↔ Sharding (2PC Circular)
**Files:** `src/transaction/distributed_transaction_manager.cpp` ↔ `include/sharding/truetime.h`; `src/sharding/cross_shard_transaction.cpp` ↔ `include/transaction/recoverable_two_phase_coordinator.h`
**Contract:** Sharding layer invokes `RecoverableTwoPhaseCoordinator` from transaction module to drive distributed prepare/commit; TrueTime from sharding provides commit timestamps. Managed circular dependency — sharding imports transaction header; transaction imports sharding TrueTime only.
**Thread Safety:** Coordinator uses internal mutex for shared state; TrueTime reads are lock-free.
**Failure Mode:** Prepare failure → automatic abort broadcast; coordinator crash → recovery via WAL replay on restart.

### Critical Integration: Transaction → Index (Co-ordinated Updates)
**Files:** `src/transaction/transaction_manager.cpp` ↔ `include/index/secondary/`, `include/index/graph/`, `include/index/vector/`
**Contract:** Index updates are enqueued within the transaction scope and applied atomically on commit (or discarded on rollback). Index update errors trigger transaction abort.
**Thread Safety:** Index update queues are per-transaction; no shared mutable state.
**Failure Mode:** Index write error → transaction aborted; index inconsistency triggers reconciliation on next startup.

---

## 7. Threading and Concurrency Model

- `TransactionManager` is designed for concurrent caller access.
- Individual transaction objects are single-owner/single-thread usage.
- Distributed coordinator paths use internal synchronization for shared state.
- Lock and deadlock helper paths are used to bound contention behavior.

## 8. Security and Reliability Considerations

- Invalid transaction transitions are rejected via status/error paths.
- Distributed coordination uses durability hooks and recovery paths to limit in-doubt exposure.
- Compensation flows are expected to be idempotent and replay-safe.
- Timeout and liveness checks are part of runtime guardrails for distributed coordination.

## 8.1 Memory Management & RAII Patterns

### Core Principles
- **Prefer `std::unique_ptr` and `std::make_unique`** for exclusive ownership.
- **Use `std::shared_ptr` only when shared ownership is semantically required** (e.g., TrueTime in distributed paths).
- **Avoid raw `new`/`delete` pairs** except in C plugin interfaces (where documented).
- **All resource cleanup must be exception-safe** via RAII destructors.

### Applied Patterns
| Component | Pattern | Example |
|-----------|---------|---------|
| TransactionManager | std::unique_ptr for detector thread | `deadlock_detector_thread_ = std::make_unique<std::thread>(...)` |
| DistributedTransactionManager | std::unique_ptr for WAL | `wal_ = std::make_unique<WALManager>(wal_cfg)` |
| GlobalTransactionManager | std::shared_ptr for distributed state | `truetime_` shared across replicas |
| SAGAOrchestratorGuard | std::unique_ptr for orchestrator lifecycle | Internal RAII wrapper in plugin |
| Saga/SAGA steps | Lambda capture (stack-based lifetime) | Compensation actions captured by value |

### C Plugin Interface Exception
- **File:** `src/transaction/saga_plugin/saga_orchestrator_plugin.cpp`
- **Reason:** Dynamic loading via C interface requires raw pointers.
- **Mitigation:** 
  - `createPlugin()` wraps allocation in try-catch with allocation safety checks.
  - `destroyPlugin()` validates null pointers before deletion (double-delete safe).
  - Internal RAII (`SAGAOrchestratorGuard`) ensures orchestrator lifecycle safety.
  - All exceptions logged; no silent failures.

### Validation & Testing
- Unit tests verify resource cleanup on exception paths.
- Saga orchestrator tests validate plugin creation/destruction cycles.
- No manual cleanup code in application paths (all RAII-based).

## 9. Known Limitations and Future Work

- Additional benchmark evidence is needed for some high-contention distributed envelopes.
- Some long-tail distributed fault combinations remain under ongoing hardening.
- Documentation and guardrails continue to be aligned with active source changes.
- C plugin interface pattern in saga_orchestrator_plugin.cpp may be refactored to use a factory in future versions.

## 10. Sourcecode Verification (Module: transaction/architecture)

- Verified files:
  - `src/transaction/transaction_manager.cpp`
  - `src/transaction/lock_manager.cpp`
  - `src/transaction/distributed_transaction_manager.cpp`
  - `src/transaction/saga.cpp`
  - `src/transaction/saga_orchestrator.cpp`
  - `src/transaction/distributed_saga.cpp`
  - `src/transaction/transaction_batcher.cpp`
  - `src/transaction/transaction_auditor.cpp`
- Verified interfaces/behaviors:
  - lifecycle and isolation entry points
  - distributed prepare/commit/abort and recovery paths
  - compensation/orchestration and operational utility surfaces
