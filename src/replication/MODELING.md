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
  - ../../include/replication/raft_v2.h
  - ../../include/replication/logical_replication.h
  - ../../include/replication/replication_slot.h
  - ../../include/replication/async_wal_shipper.h
  - ../../include/replication/geo_placement.h
  - ../../include/replication/conflict_resolution.h
  - ../../src/replication/ARCHITECTURE.md
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication state model

This model groups current public interfaces; it is not a formal guarantee that the
subsystems form a single integrated state machine.

## Entities and state

- **Replica:** `ReplicaInfo` carries node ID, endpoint, role, applied sequence/term,
  heartbeat, voting status, datacenter, priority, health, and failure metadata.
- **Manager config:** `ReplicationConfig` carries mode, timing, batching, WAL,
  quorum, failover, read preference, TLS-related fields, and seed-node values.
- **WAL record:** `WALEntry` contains sequence, term, timestamp, operation,
  collection/document identifiers, data, and checksum; serialize/deserialize methods
  are declared.
- **Raft membership:** `RaftV2ClusterConfig` tracks old/new sets while a joint
  consensus membership transition is in flight.
- **Physical slot:** `ReplicationSlot::SlotState` tracks name, status,
  confirmed/restart LSN, plugin, consumer identity, and timestamps.
- **Logical change:** `LogicalChange` represents typed change data, schema/version
  fields, old/new JSON data, DDL, LSN, and timestamp.
- **Async shipping:** `WalSegment` carries sequence, bytes, enqueue time, and target
  datacenter; a bounded queue and transport handler are owned by `AsyncWalShipper`.

## Component flow (interface-level)

1. A caller invokes manager/WAL APIs with a replication mode and topology.
2. Logical replication can select/read buffered changes through slot APIs.
3. Async shipping queues segments and invokes its configured handler outside the
   caller's enqueue operation.
4. Placement evaluates a supplied replica snapshot; the caller acts on its candidate.
5. Conflict, lag-alert, event, and observer APIs expose additional behavior through
   separate components.

This is not evidence that these steps are wired end-to-end in a particular server
configuration. See [EMBEDDING.md](EMBEDDING.md).

## Limits and explicit TODOs

- A monotonic-sequence or LSN invariant should be claimed only for an operation whose
  implementation enforces it; similarly named fields alone do not prove global order.
- `LagAlertManager` reports threshold eligibility but the reviewed interface does not
  prove an automatic `ReplicationManager` failover call.
- `AsyncWalShipper` queue-full returns `false`; callers must decide how to respond.
- A declared contract header is not conformance evidence for every runtime path.
- **TODO:** Define the integrated consistency, delivery, replay, and failure model
  against actual callers and tests.
- **TODO:** Document how header-only/unmatched implementation surfaces are provided.
