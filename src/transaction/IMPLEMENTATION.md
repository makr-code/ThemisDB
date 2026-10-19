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

# Transaction Implementation Guide

## Implementation scope

This document reflects the implementation surfaces in:

- `src/transaction/transaction_manager.cpp`
- `src/transaction/lock_manager.cpp`
- `src/transaction/saga.cpp`
- `src/transaction/distributed_transaction_manager.cpp`
- `src/transaction/global_transaction_manager.cpp`
- `src/transaction/crash_recovery_manager.cpp`
- `src/transaction/deadlock_predictor.cpp`

It describes how the module approximates durable correctness under concurrency and partial failure.

## Local transaction lifecycle

The local path is built around `TransactionManager` and `Transaction` objects. The manager:

- owns the active transaction map,
- creates timed-out and deadlock detection loops,
- tracks lock ownership and waiting state,
- drives mutation operations, commit, and rollback semantics.

The `Transaction` object is stateful and uses explicit completion markers, timeout tracking, and isolation metadata. The manager is responsible for shared coordination; the transaction instance stays focused on one unit of work.

## Locking behavior

`LockManager` implements a compatibility matrix for shared/exclusive and intent locks. It tracks holders and waiters, processes waiting queues, and performs upgrade handling via `upgradeLock()`. In the implementation, lock acquisition is guarded by a central mutex and waiters are released when a lock becomes available or times out.

Key behaviors in the implementation are:

- avoid re-entrant incompatible grants,
- enforce 2PL safeguards via the shrinking-phase gate,
- process waiters after release,
- emit timeout and warning diagnostics rather than silently swallowing lock starvation.

## SAGA compensation path

`saga.cpp` implements reverse-order compensation with explicit retry logic. `Saga::compensate()` walks the steps from newest to oldest, invoking each compensating callback and recording whether it succeeded. `Saga::compensateWithRetry()` adds bounded retries and backoff, with counters for metrics and failure counts.

Implementation rules from source:

- compensation is skipped if a saga is already marked compensated,
- failed compensations are logged per step,
- retries are bounded to avoid runaway loops,
- metrics remain cumulative across compensation attempts.

## Distributed transaction flow

The distributed manager is a 2PC-aware coordinator with participant callbacks and state transitions. The code explicitly models:

- participant registration,
- prepare-time voting,
- commit or abort decisions,
- timeout and recovery paths,
- WAL-backed in-doubt handling.

The distributed model is intentionally explicit: add participants, verify votes, choose commit or abort, and reconcile after coordinator or participant failure.

## Failure handling and edge cases

The implementation treats the following as normal operational scenarios:

- lock timeout while waiting for conflicting resources,
- transaction deadline expiry,
- deadlock cycle detection and victim selection,
- compensation callback failure after write partial success,
- coordinator crash during prepare or commit.

For each of these, the design pushes the failure outward into clear status or recovery behavior instead of silently massaging the state.

## Performance and hardening notes

From the source, the module is built to be observable under pressure:

- deadlock detection runs on a background thread,
- lock stats are tracked with atomic counters,
- SAGA metrics record retry and failure counts,
- transaction timeouts are explicit and configurable.

The result is not only concurrency control but also manageable operator visibility when contention or failure rises.

## Risks to watch

- exclusive lock upgrade patterns must remain conflict-aware and not deadlock by default,
- compensation callbacks must be idempotent enough to be safe under retries,
- distributed commit decisions must remain recoverable even when the coordinator restarts in an uncertain state,
- lock wait timeouts must not silently loosen correctness guarantees.

## Implementation principle

The transaction module should be judged by whether it preserves correct state transitions under contention and failure. This is the central design tenant behind the source and the reason the module is central to ThemisDB's reliability story.
