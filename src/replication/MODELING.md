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

# Replication Modeling Notes

## Purpose

This document models the replication subsystem as a set of stateful runtime planes and explicit contracts. It is grounded in the concrete implementation of `ReplicationManager`, `ReplicationSlotManager`, `LogicalReplicationManager`, `MultiTierReplicationManager`, and the auxiliary shipping and alerting components.

## High-level model

The replication module is organized into four primary execution planes:

1. Cluster control plane: leader election, health checks, promotion, and failover state.
2. Data propagation plane: WAL write propagation, slot advancement, and logical change streaming.
3. Conflict/merge plane: multi-writer divergence handling and deterministic merge semantics.
4. Observability and policy plane: lag accounting, placement validation, and operator-facing metrics and alerts.

This model is consistent with the source layout in `src/replication/*` and the architecture summary in `src/replication/ARCHITECTURE.md`.

## State model

### Node state

Each replica is represented by `ReplicaInfo` in the replication manager API. The record includes:

- node identity and endpoint,
- role (leader, follower, candidate, observer, witness),
- heartbeats and failure counters,
- health status,
- datacenter and priority metadata.

This is enough to model leader election, placement-aware failover, and read-preference routing.

### WAL state

`WALEntry` is the atomic replication record. It includes:

- `sequence_number`
- `term`
- `timestamp`
- operation metadata (`operation`, `collection`, `document_id`, `data`, `checksum`)

This gives the module a durable ordering primitive for consistent replay.

### Slot state

`ReplicationSlot` and `ReplicationSlotManager` hold per-slot lifecycle state: active, paused, dropped, and LSN/lag metadata. This is the mechanism by which the system avoids silent data loss and supports controlled recovery after lag or transport issues.

### Logical change model

`LogicalReplicationManager::makeLogicalChange()` turns WAL entries into `LogicalChange` records with collection, document, operation, and filter metadata. That is the point where CDC or logical propagation becomes semantically meaningful for downstream consumers.

## Interaction model

### Leader election and failover

`ReplicationManager` tracks quorum, health, and leader lease semantics. The cluster chooses a leader using explicit health and placement policy checks, then transitions to failover only when the conditions justify that move.

Key model features:

- health checks are periodic and explicit,
- failover is not automatic for any replica by default,
- quorum and placement constraints gate whether a new leader is valid,
- the cluster emits health and lag signals while the state changes.

### Data propagation

The flow is:

1. A write is committed as a WAL entry.
2. The entry is replicated to peers or shipped asynchronously.
3. Followers or downstream subscribers apply or acknowledge the write.
4. Slot or logical replication state advances according to ack and LSN rules.

The implementation makes the flow observable through lag metrics and slot state while keeping blocking transport outside lock scopes.

### Conflict handling

The conflict model is deterministic at the strategy boundary. `ConflictResolution` enumerates the supported strategies, and the relevant merge logic resolves concurrent changes using ordering or conflict metadata rather than a silent last-write-wins default hidden in the transport pipeline.

## Failure model

The key failure domains are:

- network partitions and stale leader detection,
- lag spikes and failover thresholds,
- queue saturation in asynchronous WAL shipping,
- slot persistence or replay failures,
- mismatched or unresolvable conflict states,
- schema or CDC change mismatches in logical replication.

The design treats each as explicit and diagnosable. It does not model failure as a normal success path.

## Invariants

The implementation relies on a tight set of invariants:

- sequence numbers must be monotonic within the replication domain,
- leader or replica state changes must be validated before the system accepts them,
- lag and queue events must be measurable and alertable,
- lock ordering must remain safe and ordered,
- blocking I/O must not execute while core replication locks are held,
- data must not be silently overwritten when conflict or quorum policy is unresolved.

## Operational consequences

This model makes replica behavior explainable for operators and maintainers:

- if lag is rising, `LagAlertManager` is the primary signal,
- if failover is required, `ReplicationManager` is the state machine,
- if the stream is stalled, `ReplicationSlot` and `LogicalReplicationManager` reveal the bounded state,
- if the queue backs up, `AsyncWalShipper` is the recovery surface,
- if a multi-writer conflict occurs, the conflict strategy and merge semantics are the final deterministic boundary.

## Summary

The replication module models cluster state as an explicit set of node, slot, WAL, and policy states rather than as a passive set of background tasks. That is the core reason it can provide recoverable consistency and accountable failover behavior in production.
