---
Author: ThemisDB Contributors
Created: 2026-10-18
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - ../../include/replication/replication_manager.h
  - ../../include/replication/raft_v2.h
  - ../../include/replication/async_wal_shipper.h
  - ../../include/replication/logical_replication.h
  - ../../include/replication/geo_placement.h
  - ../../src/replication/
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication feature map

The following is a map of declared APIs and module-local implementation files, not a
claim that every capability is integrated into one running deployment.

| Area | Public entry points | Source-backed scope |
|---|---|---|
| Manager and WAL | `ReplicationManager`, `WALEntry`, `WALManager` in `replication_manager.h` | Replication lifecycle, replica state, WAL operations, and role/failover methods. |
| Raft membership | `RaftV2ClusterConfig`, `MembershipChangeManager` | Joint-consensus membership change structures and quorum checks. |
| Logical changes | `LogicalReplicationManager`, `LogicalChange` | Slot and logical change APIs, filters, `readChanges()`, DDL recording. |
| Physical slots | `ReplicationSlot`, `ReplicationSlotManager` | Slot state, pause/resume/drop, LSN advancement, and persistence APIs. |
| Async WAL shipping | `AsyncWalShipper`, `WalShippingConfig` | Bounded queue and handler-driven background dispatch; queue saturation is returned to the caller. |
| Lag and placement | `LagAlertManager`, `GeoReplicaPlacementManager` | Lag threshold tracking and caller-supplied topology candidate evaluation. |
| Events and support | Event stream, policy, observer, schema CDC, tier, conflict, and CRDT headers | Additional declarations; see each header and its implementation/tests before assuming runtime support. |

`kafka_change_stream.h` and `multi_master_replication.h` have no same-named `.cpp` in
`src/replication/`; the implementation or integration location is **TODO**. API
declarations and roadmap entries do not prove feature readiness.

See [README.md](README.md) for configuration and build/test caveats.
