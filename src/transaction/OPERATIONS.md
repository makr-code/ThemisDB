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

# Transaction Operations Guide

## Operational scope

This guide covers the operator-visible behavior of the transaction module as implemented in the transaction manager, lock manager, deadlock monitor, and distributed coordinator. It is intended to support safe operation and maintenance under load.

## Core signals

Operators should watch the following categories:

- transaction queue depth and lock wait time,
- deadlock detection frequency and victim churn,
- transaction timeout counts,
- compensation retry and failure counters,
- distributed in-doubt transaction recovery state.

These are not secondary metrics; they are the clearest operational signals that the correctness boundary is under stress.

## Daily operations

### Verify transaction health

Review active transactions and whether they are stalling or timing out. A healthy local transaction path should resolve commits or rollbacks without long queues or unexplained deadlock churn.

### Review lock pressure

Check lock waiters and lock ownership. If the same keys repeatedly block transactions, the failure is likely structural rather than transient.

### Review SAGA compensation health

The compensation path is operationally important because a failed rollback can leave downstream state inconsistent. Review step history, retry counts, and failed compensation logs.

### Review distributed recovery

Distributed transaction coordinators should show either a final commit or a final abort after recovery. Unresolved prepared states must be treated as operationally urgent because they require decision-making under uncertainty.

## Incident response flow

1. identify whether the failure is local lock contention, deadlock, SAGA rollback, or distributed uncertainty,
2. gather transaction state, timeout, lock, and deadlock signals,
3. decide whether the correct path is rollback, recovery, or participant reconciliation,
4. verify cross-layer consistency after resolution,
5. document the event and update the recovery pattern for the next cycle.

## Readiness checks

Before release or after a change in transaction semantics, verify:

- timeout behavior remains bounded and deterministic,
- deadlock resolution remains explicit and reviewer-visible,
- compensation remains reverse-order and retry-aware,
- distributed state reconciliation still resolves in-doubt transactions,
- operator metrics are sufficient to diagnose failures without code inspection.

## Escalation triggers

Escalate to the transaction owner or release governance when:

- lock timeout spikes under ordinary workloads,
- deadlock rates stay elevated across multiple cycles,
- saga compensation fails repeatedly on known steps,
- distributed recovery cannot resolve a prepared state.

## Operating principle

The transaction module performs best when its failure modes are observable and recoverable. The operator's job is not to guess whether the system is safe; it is to confirm that the module has a deterministic path to lock resolution, rollback, or coordinated recovery.
