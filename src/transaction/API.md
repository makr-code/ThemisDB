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

# Transaction API Contract Summary

## API scope

This document summarizes the public and internal contracts exposed by the transaction module. The primary source of truth is the C++ API in `include/transaction/` with the implementation behavior in `src/transaction/`.

## Primary interfaces

### TransactionManager

`TransactionManager` is the main coordinator for transaction lifecycle, isolation, and deadlock handling. It exposes:

- `beginTransaction(...)` and related lifecycle entry points,
- `commitTransaction(...)` and `rollbackTransaction(...)`,
- state and timeout configuration,
- deadlock detection toggles and reporting,
- transaction query and cleanup surfaces.

From the source, the transaction object and the manager are designed to separate shared coordination from a single transaction's state.

### LockManager

`LockManager` defines the concurrency contract for keys and resources. It exposes:

- `acquireLock(...)`,
- `releaseLock(...)`,
- `releaseAllLocks(...)`,
- `upgradeLock(...)`,
- `acquirePredicateLock(...)`,
- isolation-related phase tracking (`beginShrinkingPhase`, `isInShrinkingPhase`).

The public contract is explicit: lock requests can be granted, denied, or time out.

### Saga

`Saga` captures compensating actions for partially completed workflows. It exposes:

- `addStep(...)`,
- `compensate()`,
- `compensateWithRetry(...)`,
- `clear()`,
- `trimToSize(...)`,
- `getMetrics()`,
- `isFullyCompensated()`.

The contract is that rollback is reversible by step order and observable through metrics.

### DistributedTransactionManager

The distributed manager owns cross-node 2PC semantics and participant-driven commit/abort flow. Its API is centered on participant registration, prepare voting, and recovery-driven finalization.

## Contract invariants

The module's API contract is intentionally strict:

- locks are granted only when compatibility rules permit them,
- transactions must not silently ignore timeout or deadlock conditions,
- compensation is ordered and retried within explicit bounds,
- distributed protocol states are terminal or recoverable, not ambiguous,
- commit and rollback are the only valid completion paths for a transaction.

## Error semantics

The module is designed to return explicit status or error outcomes rather than silently proceeding. Common cases include:

- lock wait timeout,
- transaction abort after deadlock detection,
- invalid state transition,
- compensation failure after partial rollback,
- coordinator or participant recovery after in-doubt state.

## Caller expectations

Callers should assume:

- a transaction may fail under timing or contention,
- invalid lock behavior is surfaced as a deterministic status,
- high-level failure paths are recoverable only through explicit rollback or compensation,
- distributed flows require coordinator and participant agreement before finalization.

## Boundary of this API

This API is subject to the transaction and reliability domain only. It does not replace storage durability semantics or declare query planning rules. Its purpose is to make the correctness boundary explicit and enforceable for higher-level modules.
