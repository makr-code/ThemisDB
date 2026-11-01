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
  - include/replication/replication_manager.h
  - include/replication/async_wal_shipper.h
  - include/replication/lag_alert_manager.h
  - include/replication/logical_replication.h
  - src/replication/replication_manager.cpp
  - src/replication/async_wal_shipper.cpp
  - tests/CMakeLists.txt
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication module

## Purpose and boundaries

`include/replication/` declares replication APIs; the corresponding runtime code is in
`src/replication/`. The implemented source files cover manager and Raft-v2 paths,
logical replication, slots, asynchronous WAL shipping, geographic placement, conflict
resolution, policy, event streams, lag alerts, observability, tiering, and schema CDC.
Storage and network transports are dependencies or injected boundaries, not all owned
by this module.

## Public API map

| Interface | Current surface |
|---|---|
| `ReplicationManager` (`replication_manager.h`) | `initialize()`, `shutdown()`, `replicate()`, `waitForReplication()`, replica/witness management, promotion/failover, health/quorum, read preference, and placement-policy methods. |
| `RaftV2ClusterConfig`, `MembershipChangeManager` (`raft_v2.h`) | Joint-consensus membership transitions and quorum checks. |
| `LogicalReplicationManager` (`logical_replication.h`) | Slot creation/listing, filtered `readChanges()`, `advanceSlot()`, and DDL recording. |
| `ReplicationSlotManager`, `ReplicationSlot` (`replication_slot.h`) | Persisted slot lifecycle and confirmed-LSN progress. |
| `AsyncWalShipper` (`async_wal_shipper.h`) | Background segment dispatch through an injected handler; bounded queue and `enqueueSegment()` backpressure result. |
| `LagAlertManager` (`lag_alert_manager.h`) | Caller-updated lag tracking, threshold evaluation, callbacks, snapshots, and metric export. |
| `GeoReplicaPlacementManager` (`geo_placement.h`) | Stateless candidate selection and placement validation over caller-supplied topology. |
| Other declared surfaces | `ReplicationEventStream`, `SchemaAwareCDCBridge`, `MultiTierReplicationManager`, `ReplicationPolicy`, `ReplicationObserver`, conflict/CRDT types, diagnostics, and Kafka change-stream declarations. See the matching headers. |

These are component APIs, not a guarantee that every surface is wired into one
end-to-end production topology. In particular, `kafka_change_stream.h` and
`multi_master_replication.h` have no same-named implementation file under
`src/replication/`; their implementation/integration boundary needs confirmation.

## WAL shipping and lag behavior

`WalShippingConfig` defaults are `max_lag_ms = 1000`, `max_queue_depth = 4096`, and
`histogram_buckets = 16`. `AsyncWalShipper` starts a worker at construction. Its default
ship handler counts bytes without sending them; callers requiring transport must install
a real handler with `setShipHandler()`. `enqueueSegment()` returns `false` at queue
capacity. The API reports statistics through `stats()` and exports text metrics through
`exportPrometheusMetrics()`.

`LagAlertManager` defaults are 10,000 ms alert, 30,000 ms critical, 60,000 ms failover
threshold, and 300,000 ms failover duration. The caller updates lag and invokes
`checkAndAlertLagViolations()`; `replicasEligibleForFailover()` reports eligible IDs.
The header does not establish that this class automatically invokes
`ReplicationManager::triggerFailover()`.

## Configuration boundary

The `WalShippingConfig`, `SLOThresholds`, and `ReplicationConfig` types are C++ APIs.
The reviewed module source does not establish a YAML parser mapping
`replication.wal_shipping.*` or `replication.lag_alert.*` into these types. Treat such
YAML snippets in older material as illustrative only until parser wiring is verified.

[`../../config/distributed/replication/basic.example.yaml`](../../config/distributed/replication/basic.example.yaml) is a
separate server-side configuration path:
`src/main_server.cpp` reads keys such as `shipper_enabled`, `primary_id`, `replicas`,
batch/retry values, and compression into `themis::sharding::WALShipperConfig`. It is not
evidence that the example configures `themisdb::replication::AsyncWalShipper`.

**TODO:** Confirm and document the supported user-facing configuration schema and the
runtime wiring for the replication module's C++ config types.

## Build, tests, and benchmarks

The root `tests/CMakeLists.txt` contains explicit replication test registrations,
including focused tests for manager guards, geo placement/WAL shipping, Raft v2, CRDT
types, and newer module features. `tests/replication/CMakeLists.txt` also contains
module-local registration logic; whether that extracted file is included in every
current configure path is not established here.

The benchmark files under `benchmarks/replication/` are added by the replication
benchmark CMake directory when `THEMIS_BUILD_BENCHMARKS` is enabled. See
[`../../benchmarks/replication/README.md`](../../benchmarks/replication/README.md) for
the current benchmark inventory.

No build, benchmark, or test execution is claimed by this documentation update.

## Related module documentation

- [API contract notes](API.md)
- [Architecture](ARCHITECTURE.md)
- [Implementation map](IMPLEMENTATION.md)
- [Maintainer guide](MODULE_GUIDE.md)
- [Operations notes](OPERATIONS.md)
- [Module roadmap/backlog](ROADMAP.md)
