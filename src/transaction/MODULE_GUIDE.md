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

# Transaction Module Guide

## What this module owns

The transaction module owns the correctness boundaries for atomic state change in ThemisDB. It is responsible for:

- transaction lifecycle management and isolation semantics,
- lock acquisition and contention handling,
- rollback and compensation logic,
- distributed 2PC-style coordination,
- deadlock prediction, timeout resolution, and recovery-oriented state tracking.

It is not the storage engine itself, nor the network façade, nor the query planner. It sits in the middle: it makes multi-step state changes safe and explainable.

## Primary references

- [README.md](./README.md)
- [ARCHITECTURE.md](./ARCHITECTURE.md)
- [ROADMAP.md](./ROADMAP.md)
- [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md)
- [SECURITY.md](./SECURITY.md)

## Module flow

1. a caller starts a transaction with an isolation mode,
2. the manager tracks the active transaction and deadline metadata,
3. locks are acquired or wait for compatibility,
4. mutations are added to the transaction context,
5. commit persists the state atomically or rollback discards it,
6. distributed flows continue through prepare and commit/abort decisions,
7. recovery and compensation paths handle partial or failed execution.

## Main implementation surfaces

| File | Responsibility |
|---|---|
| `src/transaction/transaction_manager.cpp` | lifecycle orchestration, timeout, deadlock detection |
| `src/transaction/lock_manager.cpp` | compatibility and wait semantics |
| `src/transaction/saga.cpp` | compensation ordering and retry logic |
| `src/transaction/distributed_transaction_manager.cpp` | prepare/commit/abort coordination |
| `src/transaction/crash_recovery_manager.cpp` | recovery-oriented state reconciliation |
| `src/transaction/deadlock_predictor.cpp` | deadlock analysis and exit policy |

## Operational expectations

- timeout and deadlock behavior must remain explicit and traceable,
- lock waiters must not silently disappear,
- compensation must leave an auditable trail of retry and failure metrics,
- distributed flows must reconcile in-doubt transactions after restart or split-brain scenarios,
- invalid state transitions must fail representatively instead of being guessed around.

## Common maintainer tasks

### When changing isolation behavior

Verify that the transaction manager still enforces state transitions, timeout semantics, and deadlock checks before commit. Isolation changes are not just performance tuning; they alter correctness guarantees.

### When changing lock behavior

Review the compatibility matrix and upgrade semantics carefully. A lock change can affect contention, deadlock, and transaction starvation in ways that are not obvious from the public API alone.

### When changing SAGA compensation

Ensure compensation remains reverse-order and retry-safe. Changes to step ordering, backoff, or failure handling must be validated against the compensation metrics and rollback semantics.

### When changing distributed transaction behavior

Treat in-doubt recovery as a core contract. The distributed flow must remain recoverable after restart or partial participant failure.

## Maintainer checklist

- [ ] transaction state transitions remain explicit,
- [ ] lock compatibility and upgrade semantics match source expectations,
- [ ] compensation retries remain bounded and observable,
- [ ] distributed commit/abort remains recoverable,
- [ ] deadlock and timeout diagnostics remain actionable.

## Practical guidance

The transaction module is easiest to maintain if the work always asks: "What is the failure mode if this step fails or times out?" The source repeatedly favors explicit status and recovery behavior over a fuzzy best-effort path, and that is the correct standard for future work.
