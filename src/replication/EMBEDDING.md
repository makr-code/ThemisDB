---
Author: ThemisDB Contributors
Created: 2026-10-21
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
  - ../../include/replication/async_wal_shipper.h
  - ../../include/replication/logical_replication.h
  - ../../include/replication/replication_slot.h
  - ../../include/replication/geo_placement.h
  - ../../src/replication/ARCHITECTURE.md
  - ../../src/main_server.cpp
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication integration boundaries

## In-module responsibilities

The module-local sources implement or support the declared manager/Raft-v2,
logical-replication, slot, event-stream, async-shipping, placement, conflict, policy,
observability, tier, lag-alert, and schema-CDC components. Read
[ARCHITECTURE.md](ARCHITECTURE.md) for the current component map.

## Dependencies and injected behavior

- `ReplicationManager` includes and uses WAL-related interfaces; the storage engine
  and durable storage lifecycle remain outside this module's documented ownership.
- `AsyncWalShipper` delegates actual segment transmission to its injected
  `ShipHandler`; its default handler only accounts bytes.
- `GeoReplicaPlacementManager` evaluates topology supplied by the caller and returns
  an optional candidate. The caller performs role transition/promotion.
- `LogicalReplicationManager` exposes logical change and slot operations; downstream
  consumers must follow the `readChanges()`/`advanceSlot()` contracts.

## Distinct server configuration path

`src/main_server.cpp` reads the `replication.shipper_enabled`, `primary_id`, `replicas`,
batch/retry, compression, and TLS-path fields into
`themis::sharding::WALShipperConfig`. That code constructs the server's sharding
`WALShipper` and `ReplicationCoordinator`; it is not evidence that the same YAML
configures `themisdb::replication::AsyncWalShipper` or its `WalShippingConfig`.

**TODO:** Document the concrete integration path from database writes to each
replication implementation, including the transport, API callers, lifecycle ownership,
and supported config schema. The source reviewed here does not substantiate a single
module-wide authoritative availability or durability guarantee.
