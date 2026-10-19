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

# Sharding Research Notes

## Purpose

This document records the design assumptions that shaped the sharding layer in `src/sharding` and the fixed contracts in `include/sharding/sharding_api_contract.h`. It is not a product brochure; it is a source-grounded engineering record for route selection, durability, and recovery under topology churn.

## Research anchors from the implementation

The current implementation is built around the following concrete code paths:

- `consistent_hash.cpp` defines the ring-based placement behavior used for deterministic shard selection.
- `shard_router.cpp` converts request URNs/keys into shard decisions and resolves fallback order when the primary shard is unavailable.
- `distributed_coordinator.cpp` and `cross_shard_transaction.cpp` model distributed decision-making and 2PC semantics.
- `wal_manager.cpp` and `transaction_wal.cpp` define the durable, ordered write model used for recovery.
- `rebalance_operation.h`, `auto_rebalancer.cpp`, and `data_migrator.cpp` define migration and topology-change behavior under bounded rollback assumptions.
- `health_monitor.h` and `quorum_manager.cpp` encode fail-closed state transitions for availability and quorum loss.

## Primary design questions

### 1. How do we keep routing deterministic?

The ring model in `consistent_hash.cpp` is intentionally stable: a key resolves to a shard using a hash over the logical key and virtual-node layout rather than a mutable map lookup. This makes routing a function of key + topology, not of unrelated server state. The practical effect is predictable placement with bounded remap cost when shards are added or removed.

### 2. How do we keep distributed writes safe?

The implementation treats cross-shard transactions as explicit state machines, not opportunistic best-effort writes. The canonical contract in `include/sharding/sharding_api_contract.h` requires prepare/commit/abort semantics with `IN_DOUBT` handling when a coordinator fails after prepare. This is the core reason the system distinguishes `COORDINATOR_FAILURE`, `CONSENSUS_TIMEOUT`, and `TRANSACTION_IN_DOUBT` instead of silently retrying forever.

### 3. How do we avoid silent corruption and partial repair?

The sharding layer requires durable WAL ordering and bounded, explicit rollback for migration paths. `WALEntry::serialize()` and `WALEntry::deserialize()` enforce an ordered binary record format. `RebalanceOperation` and migration logic treat failed or partial migration as a recoverable but observable fault, not as a silent data-loss path.

### 4. What is the failure model?

The module is intentionally fail-closed for high-risk outcomes:

- quorum loss blocks write safety,
- invalid routing topology fails rather than routing to an arbitrary shard,
- WAL corruption stops replay, and
- failed migration must rollback or surface `MIGRATION_FAULT`.

This matches the error taxonomy in `sharding_api_contract.h` and the rest of the sharding runtime.

## Trade-offs and limitations

### Consistent hashing advantages

- stable routing under steady-state conditions,
- bounded reshuffle impact when topology changes,
- low operational complexity for tracking node ownership.

### Consistent hashing risks

- secondary hot spots can still appear if keys are skewed,
- large topology churn can cause temporary imbalance before rebalancing completes,
- failover and rebalancing can expose quorum and migration load spikes.

### 2PC and quorum trade-off

The module chooses explicit durability over throughput in the failure cases that matter most. A slow-but-correct coordinator and quorum check is safer than a permissive best-effort path that can commit without a clear safety result.

### Recovery design trade-off

Recovery is operationally expensive but safer: WAL replay, topology validation, and migration rollback emphasize correctness over convenience. The module exposes those states to operators instead of hiding them behind fallback logic.

## Evaluation criteria for future changes

Any future enhancement to sharding should be judged against these questions:

1. Does it retain deterministic routing under steady-state topology?
2. Does it preserve explicit fail-closed semantics on quorum loss or WAL corruption?
3. Can migration/rebalance be bounded and rolled back without silent partial writes?
4. Is the operator able to diagnose the failure path from logs and health states?
5. Does the change keep cross-shard transaction state transitions auditable and replayable?

## Conclusion

The sharding design is best understood as a distributed partitioning and durability layer with explicit contracts, not a best-effort coordination layer. The source clearly favors safety, observability, and bounded failure over opportunistic fallback behavior.
