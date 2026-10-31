---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/replication/replication_manager.h"
  - "../../include/replication/raft_v2.h"
  - "../../include/replication/logical_replication.h"
  - "../../include/replication/replication_slot.h"
  - "../../include/replication/async_wal_shipper.h"
  - "../../include/replication/conflict_resolution.h"
  - "replication_manager.cpp"
  - "logical_replication.cpp"
  - "async_wal_shipper.cpp"
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication Module Architecture

## Scope, boundaries, and ownership

`src/replication/` owns replica coordination and promotion paths, logical replication slots/streams, asynchronous WAL shipping, conflict-resolution helpers, and replication status/lag reporting. It consumes storage log data and exposes replication events to its subscribers. Shard placement belongs to sharding; the underlying storage engine and network transport implementation are not owned here.

## Components and public interfaces

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| Manager and consensus | [`ReplicationManager`](../../include/replication/replication_manager.h), [`RaftV2`](../../include/replication/raft_v2.h) | Replication configuration, member/leader state, and Raft-v2 cluster configuration paths. |
| Logical stream and slots | [`LogicalReplicationManager`](../../include/replication/logical_replication.h), [`ReplicationSlotManager`](../../include/replication/replication_slot.h), [`ReplicationEventStream`](../../include/replication/event_stream.h) | Filterable logical changes, subscriber/slot lifecycle, and stream events. |
| WAL shipping | [`AsyncWalShipper`](../../include/replication/async_wal_shipper.h) | Bounded asynchronous segment queue, worker dispatch, lag accounting, and configured callbacks. |
| Conflict and status | [`ConflictResolution`](../../include/replication/conflict_resolution.h), [`LagAlertManager`](../../include/replication/lag_alert_manager.h), [`Observability`](../../include/replication/observability.h) | Conflict resolver choices, lag alerts, and replication health snapshots. |

`observability.h` declares `ReplicationObserver`; the table's `Observability` label is
the component, not a public class name. `kafka_change_stream.h` and
`multi_master_replication.h` have no same-named `.cpp` in this module directory. This
inventory does not establish whether their behavior is implemented elsewhere or wired
into the current runtime.

## Data and event flow

1. Replication configuration and membership are managed through the manager/consensus surfaces.
2. A storage WAL record or logical change is encoded/selected for the configured replication path.
3. Logical subscribers consume changes through slots/streams; the async shipper queues WAL segments and dispatches them through its configured handler.
4. Conflict resolver APIs can resolve competing writes when invoked by their caller; lag and health components expose state through separate APIs.

The manager includes `storage/wal_storage.h`; see [`replication_manager.cpp`](replication_manager.cpp), [`logical_replication.cpp`](logical_replication.cpp), and [`async_wal_shipper.cpp`](async_wal_shipper.cpp).

## Failure domains and edge cases

- Replica heartbeat/election/ack timeouts and quorum requirements affect promotion or synchronous write acknowledgement according to configuration.
- A full async shipping queue returns `false` from `enqueueSegment()`; callers must handle this backpressure result and the shipper statistics.
- A paused replication slot retains its progress position; stale consumers can therefore constrain log retention.
- Ship-handler, schema-event, or conflict-resolver errors are separate component concerns. Exact propagation/retry semantics are not uniform; consult each public interface and implementation.

**TODO:** Verify whether the module-local test CMake file is included by the active top-level
configure path, and identify any out-of-module implementation/wiring for the two
header-only declarations noted above.

## Explicit non-goals

- Owning durable record encoding, RocksDB lifecycle, or storage recovery.
- Choosing shard partitions, coordinating cross-shard transactions, or implementing client-facing admin endpoints.
- Guaranteeing a single consistency level independent of the configured mode, acknowledgement policy, and available peers.

## Verified cross-module references

| Boundary | Evidence and link |
|---|---|
| Storage WAL | `replication_manager.cpp` includes `storage/wal_storage.h`; see [storage architecture](../storage/ARCHITECTURE.md). |
| CDC/schema | `schema_cdc` is a replication-owned bridge; its public contract is [`schema_cdc.h`](../../include/replication/schema_cdc.h). |
| Shard ownership | Partitioning and routing are outside this module; see [sharding architecture](../sharding/ARCHITECTURE.md) for the separate owner. |

## References

- [Module README](README.md) · [API notes](API.md) · [module roadmap](ROADMAP.md)
- [`replication_api_contract.h`](../../include/replication/replication_api_contract.h) · [`replication_manager.h`](../../include/replication/replication_manager.h) · [`async_wal_shipper.h`](../../include/replication/async_wal_shipper.h)
