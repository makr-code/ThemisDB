---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Transaction Domain Modeling

## Domain model

The transaction module models state changes as a bounded, recoverable workflow across several layers of the database:

- transactional writes against the storage layer,
- lock acquisition and release for resource-level correctness,
- compensating actions for long-running or partial failures,
- distributed prepare/commit coordination for multi-node workflows,
- operational bookkeeping for deadlocks, timeouts, and rollback decisions.

This model is visible directly in the source tree. `TransactionManager` owns the lifecycle, `LockManager` owns concurrency control, `Saga` owns compensation, and `DistributedTransactionManager` owns multi-participant coordination.

## Core entities

### 1. Transaction

A transaction is the unit of change for a single logical worker. It tracks:

- a unique transaction id,
- isolation level,
- timeout and deadline metadata,
- read/write state,
- mutation buffers and guarded coordination with the storage/index layers.

This is the primary state machine in `include/transaction/transaction_manager.h` and implemented in `src/transaction/transaction_manager.cpp`.

### 2. Lock entry

Lock state is modeled as a relationship between a transaction id, a key, and a lock type. The source distinguishes between `SHARED`, `EXCLUSIVE`, `INTENT_SHARED`, and `INTENT_EXCLUSIVE` values in `lock_manager.h`, with compatibility checks modeling when lock grants can proceed safely.

### 3. SAGA step

A SAGA step is a unit of forward work plus the compensating action needed to undo it. `Saga::Step` stores the operation name, callback, execution timestamp, and compensation status. The module carries explicit metrics for failed and retried compensations.

### 4. Distributed transaction

The distributed model adds a multi-node state machine: `INIT → PREPARING → PREPARED → COMMITTING/ABORTING → COMMITTED/ABORTED`. The state transitions are documented in `distributed_transaction_manager.h` and implemented through prepare and commit/abort logic in `src/transaction/distributed_transaction_manager.cpp`.

## Invariants

The module depends on a few invariants that must remain true:

1. A transaction must not commit while holding invalid or unreconciled state.
2. A key cannot be granted an incompatible lock while another conflicting lock is still active.
3. A SAGA compensation path must execute in reverse order and must be observable for retries or failures.
4. Distributed coordination must either reach a consistent terminal state or be recovered via in-doubt reconciliation.
5. Timeout and deadlock resolution must be deterministic enough for operators to reason about them.

## Failure-domain model

The code separates the main concerns in a way that maps cleanly to failure domains:

- lifecycle and isolation: `transaction_manager.*`
- lock contention and 2PL semantics: `lock_manager.*`
- partial-work recovery: `saga.*`
- multi-node coordination: `distributed_transaction_manager.*`
- detection and reconciliation hooks: `deadlock_predictor.*`, `crash_recovery_manager.*`, `snapshot_manager.*`

These are intentionally distinct responsibilities. A failure in one domain should not silently become a different domain's problem.

## Modeling decisions that matter

### Fail-fast is preferred over silent acceptance

The transaction module rejects invalid or unsupported transitions and surfaces explicit errors. A timed-out transaction, deadlock winner, or failed compensation is not treated as a silent no-op.

### Recovery awareness is part of the model

The distributed path has explicit recovery logic, and the SAGA path has retry-aware compensation. This means the model expects restart and partial failure to be normal conditions, not exceptional losses of human oversight.

### Observability is part of correctness

The module records deadlock metrics, compensation metrics, timeouts, and transaction state transitions. This is not optional reporting; it is part of making correctness verifiable in production.

## Data flow summary

1. A caller starts a transaction with a chosen isolation level.
2. The manager tracks the tx and associated lock state.
3. Mutation and read operations are staged under the active transaction.
4. Lock checks reject incompatible access under contention.
5. Commit flows to durable writes; rollback discards staged changes or triggers saga compensation.
6. For distributed work, prepare/commit logic drives participant voting and recovery.

## Operational implications

This model matters because higher-level correctness depends on transaction semantics being stable and explicit. The module is intentionally designed so that operator-visible issues such as lock wait time, timeout, deadlock, or compensation failure are discoverable rather than hidden under generic errors.
