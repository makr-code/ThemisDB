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

Source level: 1; SOT domain: module-behavior. Canonical references: `include/transaction/transaction_manager.h`, `include/transaction/lock_manager.h`, `include/transaction/distributed_transaction_manager.h`, and `include/transaction/saga.h`. Related issue: #7390.

# Transaction Runbook

## Operational scope

This runbook summarizes failure categories visible in transaction APIs. It does not define deployment-specific commands, dashboards, or an operator procedure for force-resolving transactions; TODO: add those only from current runtime/operations interfaces.

## Symptoms and likely causes

### Lock timeout

Symptoms:

- transactions stall waiting for a resource,
- operation latency increases under contention,
- lock waiters/timeouts increase (where `LockManager` statistics are exposed).

Likely causes:

- write-heavy workload on the same keys,
- long-running transactions holding locks too long,
- upgrade-from-shared to exclusive path blocking other workers.

Action:

- check lock ownership and waiter state,
- inspect transaction durations and timeout policy,
- reduce lock scope or force timely rollback for long-running transactions.

### Deadlock

Symptoms:

- transaction aborts after wait cycles,
- deadlock APIs report a cycle/victim (where those diagnostics are surfaced),
- throughput falls sharply under high contention.

Likely causes:

- circular lock acquisition ordering,
- upgrade behavior on shared locks,
- transaction chains waiting on each other's resources.

Action:

- inspect the deadlock predictor output,
- verify lock ordering consistency,
- prefer deterministic victim selection rather than allowing silent waits.

### SAGA compensation failure

Symptoms:

- partial rollback leaves side effects behind,
- compensation retries increase after errors,
- metrics show failed compensation steps.

Likely causes:

- compensation callback throws on downstream failure,
- step ordering or idempotence is incorrect,
- retries are exhausted without a stable rollback outcome.

Action:

- review the saga step history,
- validate the compensating action sequence,
- confirm that each step is safe to retry or reverse.

### Distributed in-doubt state

Symptoms:

- transaction remains prepared or unresolved after a node restart,
- coordinator logs show prepare/commit uncertainty,
- recovery or WAL replay is required.

Likely causes:

- network partition or participant timeout,
- coordinator crash during prepare or commit,
- WAL replay not resolving the transaction state.

Action:

- inspect recovery state and WAL records,
- identify whether the transaction should be committed or aborted,
- re-run the recovery flow with explicit deterministic policy.

## Recovery flow

1. identify the affected transaction id and state,
2. determine whether the issue is local lock contention, deadlock, or distributed uncertainty,
3. inspect timeout, deadlock, or compensation metrics,
4. abort or recover the transaction only through the configured coordinator's documented API; TODO: specify deployment procedure and authorized recovery action,
5. verify that resulting state is consistent across the storage/index layers.

## Escalation triggers

Escalate when:

- the same deadlock pattern recurs under normal workload,
- compensation retries exceed expected limits,
- distributed recovery cannot determine a final state,
- transaction availability risks storage or index consistency.

## Operator guidance

The transaction module is designed for explicit failure signals. If the system is degraded, the correct path is to identify the precise stage of the transaction lifecycle and recover that state deterministically rather than trying to keep ambiguous work alive.
