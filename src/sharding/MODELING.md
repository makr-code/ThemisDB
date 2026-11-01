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
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References: `../../include/sharding/sharding_api_contract.h`, `../../include/sharding/rebalance_operation.h`
Related_Issue: "#7381"
Related_Milestone: DOC-WEEKLY-2026-W44
---

Exact model backend not verifiable from final document text alone.

# Sharding Modeling Notes

> **Contract/implementation boundary:** Several invariants below restate the intended public
> contract. This audit did not verify conformance across all coordinator, WAL, migration, or
> consensus implementations. **TODO:** label an invariant as runtime-guaranteed only after tracing
> the relevant implementation and registered regression test.

## Scope

This document describes the model used by the sharding subsystem in `src/sharding` and the supporting public contract in `include/sharding/sharding_api_contract.h`. The goal is to make the core domain model understandable to engineers who need to reason about data placement, topology change, transaction safety, and failover behavior.

## Domain model

The module models a distributed database as a set of shards with a topology view and a consistent hash ring.

### Shard topology

The topology is built around the concept of a shard being a cooperative unit of ownership for a range of logical keys or records. The public contract emphasizes:

- ring stability under steady state,
- minimal key remapping under membership change,
- bounded fallback depth on shard unavailability,
- fail-closed handling when a valid route is unavailable.

This is reflected in `shard_router.cpp`, `consistent_hash.cpp`, and `shard_topology.cpp`.

### Coordination model

Distributed coordination is modeled as a set of explicit consensus and transaction states. `distributed_coordinator.cpp` and `cross_shard_transaction.cpp` treat the state as a finite-state process rather than a permissive operation. The canonical states are:

- prepared,
- committed,
- aborted,
- in doubt,
- quorum lost,
- recovered.

The module does not allow a transaction to finish silently without a terminal state.

### Durability model

The WAL layer is the module's safety boundary. `wal_manager.cpp` serializes ordered LSNs and enforces an append/replay model where stable storage is required before the write is considered accepted. The consistent hash and routing logic can be re-evaluated, but the durability path is authoritative on replay and crash recovery.

### Migration model

Migration and topology rebalancing are modeled as explicit, stateful operations. The `RebalanceOperation` type in `include/sharding/rebalance_operation.h` is a concrete expression of this model: each action has a lifecycle state, progress snapshot, rollback logic, and explicit validation of operator authorization. This is precisely the sort of work the sharding module cannot treat as a background best-effort task.

## Key invariants

1. Routing is deterministic for a given key and topology.
2. A transaction must end in a terminal state that is observable.
3. WAL tells the truth about durable state.
4. Quorum loss is a safety event, not a convenience fallback.
5. Migration and repair are bounded, observable, and capable of rollback.

## Relationship between core components

| Concern | Dominant abstractions | Concrete files |
|---|---|---|
| placement | ring, shard ownership, virtual nodes | `consistent_hash.cpp`, `shard_router.cpp`, `shard_topology.cpp` |
| semantics | topology update and coordinator decisions | `distributed_coordinator.cpp`, `quorum_manager.cpp` |
| transaction safety | 2PC, WAL, in-doubt recovery | `cross_shard_transaction.cpp`, `transaction_wal.cpp`, `transaction_snapshot.cpp` |
| repair/rebalance | migration state, rollback, resharding | `rebalance_operation.h`, `auto_rebalancer.cpp`, `data_migrator.cpp` |
| operational safety | health checks and failover | `health_monitor.h`, `health_check.cpp`, `slo_monitor.cpp` |

## Model constraints and edge conditions

- invalid topology should not be silently accepted,
- lost quorum blocks writes,
- stale health state can trigger failover only under explicit monitoring logic,
- migration conflict must not be merged into a silent best effort path,
- WAL corruption is not recoverable by implicit fallback.

## Why this model matters

This model matters because sharding sits between the public API and the storage layer. A small inconsistency in route ownership or a partial migration can produce cross-shard correctness issues that are very difficult to detect after the fact. The code deliberately makes the failure modes explicit so that they are diagnosable at runtime and in post-incident analysis.
