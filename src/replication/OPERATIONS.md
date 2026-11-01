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
