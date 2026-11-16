---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-11-16
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
  - ../../include/replication/lag_alert_manager.h
  - ../../include/replication/replication_slot.h
  - ../../src/replication/replication_manager.cpp
  - ../../src/replication/async_wal_shipper.cpp
  - ../../src/replication/lag_alert_manager.cpp
  - ../../tests/replication/test_replication_geo_placement_wal_shipping_focused.cpp
Related_Issue: "#7373"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Replication operations: observable surfaces

This page identifies component APIs useful for operations and development. It is not a
deployment runbook: command-line procedures, admin endpoints, alert routing, and
production thresholds are not established by these module headers.

## Cluster and placement state

`ReplicationManager` exposes replica snapshots, health status, quorum checks, health
checks, partition detection, role transitions, and placement-policy validation.
`GeoReplicaPlacementManager` evaluates a topology snapshot and returns a candidate;
the caller is responsible for carrying out a promotion. Do not treat
`LagAlertManager` failover eligibility as proof that an automatic failover callback is
connected.

## WAL shipping and lag alerts

`AsyncWalShipper` exposes `enqueueSegment()`, `stats()`, `currentLagMs()`,
`exportPrometheusMetrics()`, and `stop()`. A full queue is signaled by a false enqueue
result; its statistics track accepted, shipped, and queue-full dropped segments. Its
transport is injected through `setShipHandler()`.

`LagAlertManager` receives lag values from its caller and evaluates thresholds when
`checkAndAlertLagViolations()` is invoked. Header defaults are 10,000 ms alert,
30,000 ms critical, 60,000 ms failover threshold, and 300,000 ms failover duration.
These are API defaults, not deployment recommendations or verified production SLOs.

The two components export their own metric text. Metric names and labels should be
checked in the relevant header/implementation; this document does not claim that a
server endpoint or dashboard exports them.

## Replication WAL recovery and readiness

`WALManager` delegates record framing, physical WAL checksums, and truncated-tail
repair to `WALStorage`, then validates each replication entry's payload checksum and
serialized sequence against the canonical WAL sequence. Deserialization, payload
checksum, sequence, and physical-record corruption abort recovery; entries after the
first invalid record are not replayed.

`ReplicationManager::initialize()` succeeds only for `COMPLETE` or
`TRUNCATED_TAIL` recovery. A truncated final physical record is removed by
`WALStorage`; the valid prefix is replayed and the outcome remains visible as
`TRUNCATED_TAIL`. Corrupted, timed-out, and other failed recovery outcomes block
initialization, keep local cluster health false, and are exposed by
`getWALRecoveryStatus()` and the one-hot
`themisdb_replication_wal_recovery_status{status="..."}` metric. The recovery scan
uses `ReplicationConfig::file_io_timeout_ms` (default 5,000 ms; zero disables the
timeout).

Election terms, including higher terms learned from peers, are stored in a separate
`raft_term` WAL below the replication WAL directory and fsynced before voting,
accepting a higher-term heartbeat, or promoting a replica, regardless of
`wal_sync_on_commit`. Term recovery participates in startup readiness; malformed or
corrupted term records block initialization. The term WAL does not advance the
data-entry LSN.

`ReplicationManager::replicate()` uses a durable append: with
`wal_sync_on_commit=true`, the append's fsync must succeed; with it disabled, an
explicit flush is required before the entry becomes visible to replication readers.
An unconfirmed flush fails the write and makes that WAL manager unavailable until
restart and recovery. Listener notifications occur only after local durability and
the selected replication acknowledgement condition succeed.

The current `ReplicationStream` has no network transport. It retries without
advancing its acknowledged sequence, so SYNC and SEMI_SYNC writes cannot satisfy
their follower acknowledgement condition in this configuration. Compression alone
is not delivery or a follower ACK. ASYNC mode can acknowledge after local WAL
durability, but does not establish a follower copy.

These states report startup recovery only. They do not establish end-to-end
multi-process crash evidence, automatic failover, or topology-specific RPO/RTO
guarantees. The module has no approved numeric lag or recovery SLO; set such targets
only after recording a representative baseline and obtaining an operational owner.
Coordinator/participant decision logs are separate recovery domains; these
replication metrics do not replace their in-doubt recovery checks.

## Slots and logical consumers

`ReplicationSlot` exposes pause/resume/drop, confirmed-LSN advancement, state, and
lag. `ReplicationSlotManager` lists slots and the minimum confirmed LSN.
`LogicalReplicationManager` exposes slot management and `readChanges()`. Its header
notes that changes are removed from the in-memory buffer when returned; consumers
should follow the documented `advanceSlot()` contract and validate persistence and
replay behavior in the source/test before operating a real deployment.

## Operational cautions and gaps

- Check each return value and status; do not infer success from an attempted operation.
- The module's C++ config structs do not establish YAML parser mappings.
- No production escalation, restore, rollback, or node-rejoin procedure is verified by
  these module APIs.
- **TODO:** Define deployment-specific dashboards, alert routing, recovery actions,
  configuration schema, and tested operator procedures in the appropriate operations
  documentation.

See [RUNBOOK.md](RUNBOOK.md) for a limited source-bounded incident checklist.
