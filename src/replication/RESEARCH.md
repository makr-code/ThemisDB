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

# Replication Research Notes

## Purpose

This document explains the design problem addressed by the replication subsystem in ThemisDB and why the module is intentionally narrow: it owns replication state, leader/follower transitions, WAL and logical change propagation, and failover observability for cluster-level consistency and recovery.

It is grounded in the concrete implementation surfaces in `include/replication/replication_manager.h`, `include/replication/async_wal_shipper.h`, `include/replication/lag_alert_manager.h`, `src/replication/replication_manager.cpp`, `src/replication/raft_v2.cpp`, `src/replication/logical_replication.cpp`, and `src/replication/conflict_resolution.cpp`.

## Problem statement

The replication module exists because ThemisDB must preserve durable multi-node state under failures without making the rest of the database runtime depend on a single node or a single storage path. In practice, the module must answer four questions reliably:

1. Which node is authoritative for writes right now?
2. How are changes serialized and replayed across replicas?
3. What do we do when replication lag or leader loss appears?
4. How do we keep operational responses explicit instead of silently choosing a wrong state?

The source shows that the answer is a bounded set of behaviors, not a generic distributed systems framework: `ReplicationManager` orchestrates leader election, health checks, and placement rules; `AsyncWalShipper` ships WAL segments asynchronously; `LagAlertManager` emits alerts and failover signals; and conflict and logical replication paths manage divergence and consumer-facing CDC semantics.

## Research questions

### 1. What behavior must be deterministic under failure?

The replication runtime is designed to keep state transitions explicit. For example, `ReplicationManager::promoteToLeader()`, `demoteToFollower()`, and `triggerFailover()` are modeled as discrete transitions rather than implicit background side effects. `validateConfig()`, `hasQuorum()`, and `detectNetworkPartition()` provide explicit gates before the system claims a safe/authoritative state.

### 2. What are the relevant consistency trade-offs?

The code makes the trade-offs visible in the config and API surface. `ReplicationMode` explicitly supports `SYNC`, `SEMI_SYNC`, and `ASYNC`, and `ReplicationConfig` exposes the tuning knobs that decide whether the system waits for replicas or proceeds under lag. This is a direct representation of the classic consistency-vs-throughput decision in replicated systems.

### 3. How is failure surfaced to operators?

The module uses observable signals, not silent retry loops. `LagAlertManager` checks threshold crossings and emits `AlertEvent` levels (`ALERT`, `CRITICAL`, `FAILOVER`). The WAL shipper records queue depth, lag, and dropped-segment counts. These surfaces allow failover logic and monitoring logic to act on clearly bounded events.

## Assumptions and constraints

- The module assumes a single leader abstraction for the primary write path, even where multi-master or conflict-resolution helpers are present.
- Consistency is strongest when the system uses quorum-aware and timeout-aware logic; missing quorum or unhealthy replicas must not be treated as healthy state.
- Durable writes rely on WAL semantics and sequence ordering; sequence_number and LSN-like ordering are part of the correctness model.
- External transport failures are handled as fail-safe conditions around the replication boundary, not as hidden write success.
- The lock hierarchy documented in `src/replication/ARCHITECTURE.md` is integral to safety: lock acquisition is ordered and blocking I/O stays outside the lock scope.

## Evidence from implementation

The module is consistent with these implementation patterns:

- `ReplicationManager` owns cluster membership, election, and failover flows and exposes them through configuration and runtime state checks.
- `GeoReplicaPlacementManager` and placement validation provide constrained leader selection when datacenter or zone policies apply.
- `AsyncWalShipper` chooses bounded queue depth, lag measurement, and a separate worker loop to keep the write path from becoming a blocking transport operation.
- `logical_replication.cpp` converts WAL entries to logical change objects, maintains slot state, and supports filter/transform behavior for CDC-style replication.
- `ReplicationSlot` and `ReplicationSlotManager` provide persisted slot lifecycle state for acknowledged or paused replication work.

## Evaluation criteria

The design is credible when the following remain true:

- a leader is elected or rejected based on explicit quorums and health signals,
- lag and queue conditions are visible in metrics and alerts,
- failed transport or failed replica paths do not silently mark success,
- slot and logical replication state remain durable and recoverable,
- multi-writer or conflict paths preserve deterministic outcomes under concurrent updates.

## Trade-offs

### Safety vs. throughput

The replication configuration prefers explicit fail-safe behavior over throughput-only progress. `SEMI_SYNC` and `SYNC` modes intentionally wait for acknowledgment, while `ASYNC` remains a throughput-optimized mode. The code makes that performance trade-off visible in configuration, not hidden in runtime behavior.

### Determinism vs. flexibility

Conflict resolution is intentionally deterministic by strategy: HLC, LWW, and CRDT-like logic produce stable merge behavior. This reduces ambiguous data outcomes but increases the need to plan and validate topology, replica affinity, and failover policies.

### Operational clarity vs. convenience

The module exposes observability, timeout guards, and slot/lag state to operators instead of burying cluster health inside a single opaque manager object. The selected trade-off favors maintainability and recovery over a minimal but opaque control plane.

## Conclusion

The replication module is best understood as ThemisDB's production-grade durability and failover boundary. It is not a general-purpose distributed consensus engine; it is a controlled replication runtime that keeps consistency, lag, recovery, and operator visibility. The implementation choices are consistent with that role and with the explicit contracts described in the rest of the replication documentation set.
