---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1; SOT domain: module-behavior. Canonical references: `include/transaction/transaction_manager.h`, `include/transaction/distributed_transaction_manager.h`, `include/transaction/lock_manager.h`, and `src/transaction/transaction_manager.cpp`. Related issue: #7390.

# Transaction Research Notes

## Scope

This note is grounded in the runtime contract visible in `src/transaction/transaction_manager.cpp`, `src/transaction/lock_manager.cpp`, `src/transaction/saga.cpp`, `src/transaction/distributed_transaction_manager.cpp`, and the public API headers under `include/transaction/`. It explains why the transaction module exists in ThemisDB and which trade-offs are intentional at the ACID and coordination boundaries.

## Problem statement

The transaction module provides local transaction APIs over a RocksDB-backed transaction and separate coordination/compensation components. A local transaction does not establish atomicity for arbitrary external side effects or every separately managed index/service boundary; those guarantees require the specific integration contract.

The source code is explicit about this contract:

- `TransactionManager` owns the transaction lifecycle, isolation mode, and deadlock monitoring for a transaction session.
- `LockManager` enforces compatibility and waiting semantics around resource keys.
- `Saga` keeps compensating actions for partially completed work so failure recovery is explicit and reversible.
- `DistributedTransactionManager` coordinates prepare/commit/abort across multiple participant nodes, with recovery-oriented WAL and timeout behavior.

## Research questions

### 1. What is the correctness contract?

The primary objective is ACID behavior for local transactions and survivable coordination for distributed execution. The module therefore defines a boundary around state ownership: a transaction is started, mutations are staged, and commit or rollback occurs only under explicit status transitions. That is visible in `TransactionManager::Transaction::commit()`, `rollback()`, and the manager-level state tracking around deadlock and timeout detection.

### 2. Why is locking explicit rather than implicit?

The module uses a lock manager to represent read, write, and intent semantics and to block destructive write interleavings. This is not a convenience layer; it is the guardrail that keeps a transaction from violating isolation rules while a competing transaction holds a conflicting lock. The compatibility matrix in `lock_manager.cpp` and the explicit `upgradeLock()` path make the lock semantics deliberate and reviewable.

### 3. How does recovery work under failure?

The code treats failure as a first-class state transition rather than a silent fallback. Deadlock detection aborts timed-out transactions. `Saga` compensates in reverse order. `DistributedTransactionManager` is designed for prepare/commit/abort phases with in-doubt recovery and WAL-aware coordination. The architecture therefore assumes that failure must be observable and recoverable.

## Assumptions and constraints

- Transaction ownership is per session: the transaction object is designed for single-owner use, while the transaction manager itself is shared across callers.
- Read/write conflicts must be visible and handled before commit to avoid hidden anomalies.
- Distributed protocols need deterministic timeout and abort behavior because network partitions and coordinator loss are not safe to ignore.
- Compensation is only practical when steps are ordered and idempotent enough to retry or reverse without data drift.

## Source-based evidence

The design is consistent with the implementation surfaces below:

- `include/transaction/transaction_manager.h` defines `TransactionManager`, isolation-level state, and transaction status semantics.
- `include/transaction/lock_manager.h` declares reader/writer compatibility and two-phase locking behavior.
- `include/transaction/saga.h` captures step ordering and compensation metrics.
- `include/transaction/distributed_transaction_manager.h` formalizes the 2PC participant flow and state machine.
- `src/transaction/transaction_manager.cpp` and `src/transaction/lock_manager.cpp` implement the runtime safety and contention logic.

## Evaluation criteria

The module is healthy when the following are true:

- local transactions commit atomically or rollback without partial writes,
- lock conflicts are resolved deterministically under contention,
- timed-out or deadlocked transactions are aborted in a controlled way,
- SAGA compensation remains reverse-order and observable,
- distributed phase transitions are recoverable after restart or participant failure.

## Trade-offs

### Correctness vs. throughput

The module prefers explicit lock checks, compensation, and recovery paths over speculative fast paths. This can reduce peak throughput under high contention but keeps safety and operator diagnosability intact.

### Local simplicity vs. distributed complexity

The local manager handles transaction semantics in a single process; the distributed manager introduces additional protocol states, retries, and recovery obligations. The trade-off is intentional because cross-node correctness is necessarily more complex than single-node ACID behavior.

### Deterministic failure vs. permissive recovery

The code chooses explicit abort, timeout, and compensation semantics over silent acceptance of uncertain states. That limits convenience in edge cases but reduces the chance of hidden inconsistency.

## Conclusion

The transaction module is best understood as the correctness and recovery substrate for ThemisDB. It is the boundary that converts concurrent writes, cross-index mutations, and distributed orchestration into explicit, reversible outcomes. That is why it is central to both local consistency and cluster-level reliability.
