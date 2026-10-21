---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Replication Module Guide

## Purpose

This guide is the operator and maintainer entry point for the replication module. It explains where to look in the code, which responsibilities the module owns, and which diagnostic surfaces answer common questions during incidents.

## Module ownership

The replication module owns:

- cluster membership, election, and failover state,
- WAL and logical replication behavior,
- slot lifecycle and state persistence,
- replication lag and failover alerting,
- placement and region-aware routing policy.

It does not own the storage engine itself, and it is not the place for unrelated server or auth semantics.

## Primary entry points

| Surface | Main file(s) | Why it matters |
|---|---|---|
| cluster orchestration | `include/replication/replication_manager.h`, `src/replication/replication_manager.cpp` | leader lifecycle, health, failover, read routing |
| WAL shipping | `include/replication/async_wal_shipper.h`, `src/replication/async_wal_shipper.cpp` | async lag tracking and remote shipment |
| lag alerts | `include/replication/lag_alert_manager.h`, `src/replication/lag_alert_manager.cpp` | operator-facing alert events |
| logical replication | `src/replication/logical_replication.cpp` | CDC-style logical stream generation |
| slots | `include/replication/replication_slot.h`, `src/replication/replication_slot.cpp` | slot state, pause/resume/drop |
| placement | `src/replication/geo_placement.cpp` | datacenter- and zone-aware replica selection |
| conflict logic | `src/replication/conflict_resolution.cpp` | deterministic multi-writer resolution |

## Operational workflow

### When replication lag is rising

Check the lag alert manager and replica health status first. If an alert is triggered, the next question is whether the lag is a temporary spike or a failover condition that requires leader switch logic.

### When failover is suspected

Review the manager state, quorum checks, health transitions, and placement policy validation. The failure path should be explicit and visible in metrics rather than a silent fallback.

### When the stream is delayed

Inspect slot state and logical replication progress. Look for state persistence mismatches or a blocked logical consumer path.

### When the queue is saturating

Inspect the WAL shipper metrics: queue depth, total enqueued/shipped/dropped segments, and lag histogram. Backpressure is a normal part of the system, but it must remain observable.

## Recovery principles

The module's recovery model is built around explicit state and bounded behaviors:

- pause or stop only when a clear failure condition exists,
- prefer explicit signal and metrics over silent retry loops,
- make failover decisions only after health and quorum checks,
- preserve slot and WAL state so replay remains deterministic.

## Summary

If a problem is not obviously in storage, query execution, or auth, the replication module is the first place to inspect for: lag, failover, placement, slot state, CDC drift, or queue backpressure. The code is designed to make those states explicit and diagnosable.
