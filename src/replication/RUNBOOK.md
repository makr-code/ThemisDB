---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Replication Runbook

## Purpose

This runbook covers the main operational failure modes in the replication subsystem and the expected first response for each. It is grounded in the real runtime surfaces implemented in `ReplicationManager`, `AsyncWalShipper`, `LagAlertManager`, `ReplicationSlotManager`, and `LogicalReplicationManager`.

## 1. Replication lag incident

### Symptoms

- lag metrics climb above configured thresholds,
- replica health shifts to `DEGRADED` or `FAILED`,
- alert callbacks fire from `LagAlertManager`.

### Immediate diagnosis

1. Identify the affected replica and its current lag value.
2. Check whether the degradation is local to one replica or cluster-wide.
3. Review queue and shipping health for the associated WAL shipper.
4. Determine whether the replica is within placement or quorum constraints.

### Recovery

- Pause or isolate the unhealthy replica if it is unstable and cannot be repaired safely.
- If the failure persists across the failover threshold, use the manager's failover path with explicit health validation.
- Ensure the replica is re-synced or rejoined before returning it to service.

## 2. Leader loss or stale leader

### Symptoms

- health checks fail repeatedly,
- leader endpoint is not responding,
- quorum is absent or inconsistent.

### Immediate diagnosis

1. Check `ReplicationManager` health and quorum state.
2. Confirm the leader lease and election timeout values.
3. Review network partition or failover detection logic.

### Recovery

- Trigger failover only after health, quorum, and placement checks are satisfied.
- Avoid reintroducing a stale leader until its state is reconciled or it is demoted safely.

## 3. WAL shipper queue saturation

### Symptoms

- queue depth increases beyond expected values,
- drop counts rise,
- lag exceeds `max_lag_ms`,
- alert callback repeatedly fires.

### Immediate diagnosis

1. Check the shipper's queue depth and metrics.
2. Confirm whether the transport handler is blocked or returning transient failures.
3. Determine if the queue is experiencing a true overload or a network issue.

### Recovery

- Reduce traffic or isolate the overloaded remote target.
- confirm backpressure is active and the queue is not silently accepting unbounded work.
- validate that dropped segments are recorded and the operator can observe the resulting gap.

## 4. Slot drift or logical replication stall

### Symptoms

- slot state is paused, stale, or inconsistent,
- logical replication output stops or lags materially,
- persistent state files no longer match expected sink state.

### Immediate diagnosis

1. Inspect `ReplicationSlot` state and `LogicalReplicationManager` slot metadata.
2. Revive or re-create the slot only when the underlying issue is understood.
3. Check filter, transform, and schema compatibility before replay.

### Recovery

- Reconcile slot state with the latest durable WAL position before resuming.
- Validate downstream consumer compatibility and any schema changes.

## Rollback and escalation

- Prefer a rollback to the last healthy leader or slot state rather than forcing a risky new leader during partial outage.
- Escalate when lag, failover, or slot health remains elevated across multiple nodes, or when quorum cannot be established.
- Keep the operator runbook tied to explicit metrics and health states; avoid silent retry behavior.

## Summary

The main recovery principle in this module is: diagnose the exact state, then act on that state explicitly. Lag, leader health, queue saturation, and slot drift are all operationally visible in the current implementation and should be treated as first-class operational signals.
