---
Author: ThemisDB Contributors
Created: 2026-06-01
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: architecture-governance
Canonical_References:
  - README.md
  - replication_manager.h
  - async_wal_shipper.h
  - logical_replication.h
  - replication_slot.h
  - ../../src/replication/ARCHITECTURE.md
  - ../../src/replication/
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication public-header architecture

`include/replication/` declares 18 public interfaces and data types. The corresponding
runtime directory contains 13 `.cpp` files. Use
[`../../src/replication/ARCHITECTURE.md`](../../src/replication/ARCHITECTURE.md) for
the source-backed component flow; this page describes header groupings, not proof that
all declarations are wired into the same runtime.

## Header groups

| Group | Headers | Principal declarations |
|---|---|---|
| Manager/consensus | `replication_manager.h`, `raft_v2.h` | `ReplicationManager`, `ReplicationConfig`, `WALEntry`, `RaftV2ClusterConfig`, `MembershipChangeManager` |
| Geographic placement/diagnostics | `geo_placement.h`, `replication_failover_diagnostics.h` | `GeoReplicaPlacementManager`, placement constraints and diagnostic data structures |
| Logical stream/slots | `logical_replication.h`, `replication_slot.h`, `event_stream.h` | `LogicalReplicationManager`, `ReplicationSlotManager`, `ReplicationSlot`, `ReplicationEventStream` |
| WAL shipping/lag | `async_wal_shipper.h`, `lag_alert_manager.h` | `AsyncWalShipper`, `WalShippingConfig`, `LagAlertManager`, `SLOThresholds` |
| Conflict and CRDT | `conflict_resolution.h`, `multi_master_replication.h`, `crdt_types.h` | resolver classes, vector/HLC types, CRDT value types |
| Policy/tiering/observability | `policy.h`, `multi_tier_replication.h`, `observability.h` | `ReplicationPolicy`, `MultiTierReplicationManager`, `ReplicationObserver` |
| CDC/integration | `schema_cdc.h`, `kafka_change_stream.h` | `SchemaAwareCDCBridge` and Kafka exporter declarations |
| Contract | `replication_api_contract.h` | `ReplicationErrorCode` and contract constants |

The headers use the `themisdb::replication` namespace except
`replication_api_contract.h`, which declares its contract in `themis::replication`.

## Boundaries and implementation evidence

- `ReplicationManager` exposes lifecycle, replication, role/failover, health/quorum,
  read-preference, and placement-policy methods.
- `LogicalReplicationManager` and slot APIs expose independent logical-stream and
  consumer-progress operations.
- `AsyncWalShipper` accepts a caller-provided transport handler; the header's default
  handler is a no-op counter, not a network transport.
- `GeoReplicaPlacementManager` evaluates a topology snapshot and returns a candidate;
  the caller remains responsible for promotion.
- No same-named `.cpp` exists under `src/replication/` for
  `kafka_change_stream.h` or `multi_master_replication.h`. **TODO:** Determine whether
  their definitions or integrations are supplied elsewhere; do not infer runtime
  availability from declaration presence alone.
- C++ config structs do not establish user-facing YAML parser support. See the module
  README's configuration caveat.
