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

Source level: 1; SOT domain: module-behavior. Canonical references: `include/transaction/transaction_manager.h`, `include/transaction/distributed_transaction_manager.h`, `include/transaction/lock_manager.h`, `src/transaction/transaction_manager.cpp`, and `src/transaction/distributed_transaction_manager.cpp`. Related issue: #7390.

> **Architecture note:** Verify classes/types/namespaces against current source. Mark unresolved implementation-level claims with `TODO` rather than inferring them.

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
  -> Transaction operations declared by `TransactionManager::Transaction`
  -> commit/rollback

Distributed path
  -> DistributedTransactionManager::beginDistributed(...)
  -> prepareDistributed(...)
  -> commitDistributed(...) / abortDistributed(...)
  -> separately declared recovery helpers (specific recovery guarantees depend on the coordinator)

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
| `storage` | `include/storage/rocksdb_wrapper.h`, `include/storage/history_manager.h` | RocksDB-backed operations and history integration; exact durability options are configured elsewhere |
| `index` | `include/index/` | TODO: verify which index mutations are enlisted atomically for each transaction operation |
| `sharding` | `include/sharding/wal_manager.h` | TODO: establish the specific transaction-to-sharding call path before asserting dependency direction |
| `plugins` | — | TODO: no transaction-manager plugin hook was established by the inspected manager header/source |
| `utils` | `include/utils/` | Utility helpers (error codes, logging) |

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `server` | Transaction-related headers | TODO: verify registered endpoint ownership before asserting a direct consumer relationship |
| `query` | `include/transaction/transaction_manager.h` | Mutation transactions for DML statements |
| `sharding` | `include/transaction/recoverable_two_phase_coordinator.h`, `include/transaction/distributed_transaction_manager.h` | Cross-shard 2PC coordination and distributed transaction management (managed circular dep) |
| `storage` | `include/transaction/snapshot_manager.h` | Storage calls back to snapshot manager for MVCC snapshots |
| `analytics` | `include/transaction/snapshot_manager.h` | `diff_engine` uses snapshot reads for consistent analytical diff computation (`include/analytics/diff_engine.h:16`) |
| `api` | `include/transaction/transaction_manager.h` | gRPC service wires `TransactionManager` to expose explicit begin/commit/rollback RPC endpoints (`src/api/themisdb_grpc_service.cpp:18,243,265`) |
| `network` | `include/transaction/transaction_manager.h` | Wire-protocol server drives transaction lifecycle (begin, commit, rollback) for native client connections (`src/network/wire_protocol_server.cpp:53,324,2216`) |

---

## 6. Integration Points (Detailed)

### Transaction → Storage
`Transaction::commit()` delegates to its underlying RocksDB transaction. The header/source inspection does not establish a separate WAL append contract, a `kDurabilityError` result, or atomicity across arbitrary index/external-service effects. TODO: cite the RocksDB wrapper's configured write options before documenting durability details.

### Critical Integration: Transaction ↔ Sharding (2PC Circular)
**Files:** `src/transaction/distributed_transaction_manager.cpp` ↔ `include/sharding/truetime.h`; `src/sharding/cross_shard_transaction.cpp` ↔ `include/transaction/recoverable_two_phase_coordinator.h`
**Contract:** A recoverable 2PC interface is declared in the transaction headers and the distributed manager has participant callbacks and RPC injection points. TODO: document concrete sharding callers and recovery behavior from those source paths; this guide does not assert TrueTime timestamps or a managed circular dependency.

### Critical Integration: Transaction → Index (Co-ordinated Updates)
**Files:** `src/transaction/transaction_manager.cpp` ↔ `include/index/secondary/`, `include/index/graph/`, `include/index/vector/`
**Contract:** TODO: verify the current index mutation path for each operation before claiming atomic enlistment, rollback behavior, or startup reconciliation.

---

## 7. Threading and Concurrency Model

- The manager implementation uses synchronization for shared manager state; no blanket thread-safety guarantee is stated for every public operation.
- Individual transaction handles should be treated as single-owner unless a specific API comment states otherwise.
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

### Ownership patterns
| Component | Pattern | Example |
|-----------|---------|---------|
| TransactionManager | TODO: verify current member ownership before documenting a detector thread pointer |
| DistributedTransactionManager | TODO: verify the current WAL member type/configuration |
| GlobalTransactionManager | TODO: verify distributed-state ownership before documenting TrueTime sharing |
| Saga plugin | TODO: verify current plugin source path and guard class before documenting |
| Saga/SAGA steps | Lambda capture (stack-based lifetime) | Compensation actions captured by value |

### Plugin boundary
TODO: verify the current saga plugin implementation and public C ABI before documenting allocation, exception, or deletion guarantees.

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
