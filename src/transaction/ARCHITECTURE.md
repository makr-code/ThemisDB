> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Transaction Module - Architecture Guide

**Version:** 1.1
**Last Updated:** 2026-05-31
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

## 5. Threading and Concurrency Model

- `TransactionManager` is designed for concurrent caller access.
- Individual transaction objects are single-owner/single-thread usage.
- Distributed coordinator paths use internal synchronization for shared state.
- Lock and deadlock helper paths are used to bound contention behavior.

## 6. Security and Reliability Considerations

- Invalid transaction transitions are rejected via status/error paths.
- Distributed coordination uses durability hooks and recovery paths to limit in-doubt exposure.
- Compensation flows are expected to be idempotent and replay-safe.
- Timeout and liveness checks are part of runtime guardrails for distributed coordination.

## 7. Known Limitations and Future Work

- Additional benchmark evidence is needed for some high-contention distributed envelopes.
- Some long-tail distributed fault combinations remain under ongoing hardening.
- Documentation and guardrails continue to be aligned with active source changes.

## 8. Sourcecode Verification (Module: transaction/architecture)

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
