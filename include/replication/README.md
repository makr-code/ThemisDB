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
SOT_Domain: api-contract
Canonical_References:
  - replication_manager.h
  - raft_v2.h
  - logical_replication.h
  - replication_slot.h
  - async_wal_shipper.h
  - lag_alert_manager.h
  - geo_placement.h
  - ../../src/replication/
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication public headers

This page indexes declarations in `include/replication/`. A declaration is not by
itself evidence of a complete runtime integration; check the corresponding source and
tests before relying on behavior.

## Header inventory

There are 18 headers in this directory:

| Area | Headers | Key declared API |
|---|---|---|
| Manager and consensus | `replication_manager.h`, `raft_v2.h` | `ReplicationManager`, `ReplicationConfig`, `WALEntry`, `RaftV2ClusterConfig`, `MembershipChangeManager` |
| Placement and diagnostics | `geo_placement.h`, `replication_failover_diagnostics.h` | `GeoReplicaPlacementManager`, `PlacementConstraints`, diagnostic records |
| Logical stream and slots | `logical_replication.h`, `replication_slot.h`, `event_stream.h` | `LogicalReplicationManager`, `ReplicationSlotManager`, `ReplicationSlot`, `ReplicationEventStream` |
| WAL shipping and lag | `async_wal_shipper.h`, `lag_alert_manager.h` | `AsyncWalShipper`, `WalShippingConfig`, `LagAlertManager`, `SLOThresholds` |
| Conflict and data types | `conflict_resolution.h`, `multi_master_replication.h`, `crdt_types.h` | conflict resolvers, vector/HLC types, CRDT data types |
| Policy, tiers, observability | `policy.h`, `multi_tier_replication.h`, `observability.h` | `ReplicationPolicy`, `MultiTierReplicationManager`, `ReplicationObserver` |
| CDC and integration declarations | `schema_cdc.h`, `kafka_change_stream.h` | `SchemaAwareCDCBridge`, Kafka exporter/config declarations |
| Contract constants | `replication_api_contract.h` | Error-code enum and documented contract constants |

`src/replication/` contains 13 `.cpp` files. There is no same-named implementation
file there for `kafka_change_stream.h` or `multi_master_replication.h`; this inventory
does not determine whether their definitions or integration are located elsewhere.
**TODO:** Confirm implementation ownership and supported link/runtime status for those
two interfaces.

## Selected caller-facing methods

- `ReplicationManager`: `initialize()`, `shutdown()`, `replicate()`,
  `waitForReplication()`, `addReplica()`, `removeReplica()`, `addWitnessNode()`,
  `triggerFailover()`, `promoteToLeader()`, `demoteToFollower()`,
  `hasQuorum()`, and placement-policy methods.
- `LogicalReplicationManager`: `createSlot()`, `advanceSlot()`, `listSlots()`,
  `readChanges()`, and `recordDDLChange()`.
- `ReplicationSlot`: `pause()`, `resume()`, `drop()`, `advance()`, and state accessors.
- `AsyncWalShipper`: `setShipHandler()`, `enqueueSegment()`, `stats()`,
  `currentLagMs()`, `exportPrometheusMetrics()`, and `stop()`.
- `GeoReplicaPlacementManager`: `selectLeaderCandidate()`,
  `selectFailoverCandidate()`, and `validatePlacement()`.

Exact parameter types, return values, failure behavior, and thread-safety are defined
by the individual header comments; this summary is not a substitute for them.

## Configuration and integration caveat

`ReplicationConfig`, `WalShippingConfig`, and `SLOThresholds` are C++ structures. The
headers do not prove a YAML parser exists for every field. The `config/replication.*`
examples and `src/main_server.cpp` refer to a server-side `themis::sharding::WALShipper`
path; do not treat that as configuration wiring for `themisdb::replication::AsyncWalShipper`.

See [`../../src/replication/README.md`](../../src/replication/README.md) for module
boundaries and build/test references.
