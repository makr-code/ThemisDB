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

# Sharding Implementation Guide

## Module map

The implementation is organized around clear runtime responsibilities:

- `consistent_hash.cpp`: placement and ring behavior.
- `shard_router.cpp`: key/URN route resolution and fallback ordering.
- `distributed_coordinator.cpp`: coordination and decisions on cluster state.
- `cross_shard_transaction.cpp`: 2PC state transitions, WAL checkpoint logic, and recovery behavior.
- `wal_manager.cpp`: WAL append, rotation, replay, and LSN management.
- `data_migrator.cpp` and `auto_rebalancer.cpp`: data movement and topology-aware redistribution.
- `health_monitor.cpp` and `quorum_manager.cpp`: health detection, failover, and quorum enforcement.
- `shard_repair_engine.cpp` and `shard_load_balancer.cpp`: repair and balancing under degraded conditions.

## Core implementation invariants

### Route safety

`shard_router.cpp` resolves a route by checking the valid shard topology before routing work. It uses explicit fallbacks and keeps the route decision deterministic. The code makes intentional choices around request normalization and version-aware routing rather than permitting arbitrary backend hopping.

### Transaction safety

`cross_shard_transaction.cpp` encodes transaction safety as a state machine. The coordinator must not silently continue when a decision is ambiguous, and it blocks conflicting writes while a transaction remains `IN_DOUBT`. This is critical to preserving correctness under coordinator failure and partial participant recovery.

### Durability safety

`wal_manager.cpp` is designed to make durability explicit: LSNs are assigned, serialized, and replayed in order; format validity is enforced; and replay is aborted on corruption rather than best-effort recovered. This enforces the public contract in `sharding_api_contract.h` that WAL corruption is a fail-closed condition.

### Migration safety

`RebalanceOperation` and migration-related code explicitly separate planning, execution, verification, and rollback. A migration is not considered complete merely because data transfer started. Completion is a verified state transition with bounded throughput and discrepancy checks.

## Edge cases the code deliberately handles

- missing or invalid topology information,
- quorum loss during coordinator or participant activity,
- partial migration or failed verification during rebalance,
- inconsistent or stale health results,
- cross-shard transaction timeout or in-doubt state,
- WAL corruption or replay interruption,
- shard outage followed by recovery and anti-entropy repair.

## Implementation conventions

- The design prefers explicit failure codes over silent defaulting.
- State transitions are observable and should be diagnosable from logs.
- Rebalance and repair operations are bounded and recoverable.
- The module treats topology inconsistency as critical infrastructure state, not a soft warning.

## Validation expectations

Implementation changes in this module should validate at least:

1. The route remains stable under static topology.
2. A missing quorum blocks writes and surfaces the proper error.
3. A cross-shard transaction reaches a clear terminal status.
4. WAL replay can recover or fail closed without silent data acceptance.
5. Rebalance and repair can complete or roll back without losing a shard's safety guarantees.

## Summary

The sharding implementation is explicit by design: routes, transactions, migration steps, and health transitions are all modeled as stateful components with bounded failure semantics. This is the correct foundation for a production-grade sharded database runtime.
