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

# Replication Implementation Notes

## Purpose

This document describes the implementation structure of the replication subsystem and the safety invariants that are enforced in production code. It is grounded in `src/replication/*.cpp` and in the public interfaces under `include/replication/*`.

## Module implementation surfaces

The module is implemented across the following primary sources:

- `src/replication/replication_manager.cpp` — cluster lifecycle, leader election, health, read routing, and failover orchestration
- `src/replication/raft_v2.cpp` — Raft-like consensus and membership behavior
- `src/replication/logical_replication.cpp` — logical change generation, filters, transforms, and slot persistence
- `src/replication/replication_slot.cpp` — slot lifecycle and persisted state
- `src/replication/async_wal_shipper.cpp` — queue, lag tracking, and dispatch worker
- `src/replication/lag_alert_manager.cpp` — per-replica lag and alert evaluation
- `src/replication/conflict_resolution.cpp` — conflict detection and merge behavior
- `src/replication/multi_tier_replication.cpp` — tier-based replication placement and metadata
- `src/replication/geo_placement.cpp` — datacenter placement and failover candidate selection

## Implementation invariants

### 1. Explicit state transitions

Implementation code in `ReplicationManager` exposes explicit transitions for lifecycle and failover. The cluster does not silently move between leader/follower states without a change in health and quorum conditions or an explicit promotion/demotion action.

### 2. Timeout and fail-safe semantics

The code includes timeout-wrapped operations and guarded worker loops. The explicit timeout path in `replication_manager.cpp` demonstrates the design intent: long-running operations must fail safely and not hang the system indefinitely. This is reinforced by the `AsyncWalShipper` worker's bounded wait and queue behavior.

### 3. Lock ordering discipline

The architecture notes describe the intended hierarchy: manager collection locks, per-resource locks, then background I/O and callback paths. This is reflected in the code comments and the lock ordering descriptions inside the source files. The design makes blocking I/O happen after the relevant lock is released to avoid deadlocks and lock contention.

### 4. Durable write semantics

WAL entry serialization and sequence ordering make the write path deterministic. `WALEntry::serialize()` and `deserialize()` handle binary encoding, validation, and length checking. This is essential because replication integrity depends on consistent ordering and interpretable metadata.

## Failure handling patterns

The implementation distinguishes several operator-visible failure domains:

- `AsyncWalShipper` queue overflow -> backpressure and drop accounting,
- WAL shipping lag -> alert callback and lag tracking,
- lost quorum -> safe read-only or leader election path,
- conflict mismatch -> explicit diagnostics rather than silent overwrite,
- slot persistence failure -> fail-safe state rather than hidden state drift,
- schema/consumer mismatch -> logical replication filter or version-aware rejection.

## Edge cases and expected responses

The implementation is designed to handle the following edge cases explicitly:

- empty replica IDs or invalid endpoints,
- lag threshold crossing without failover readiness,
- a stale or repeated remote sequence,
- duplicate or missing logical replication updates,
- placement constraint violations, including required or forbidden datacenter sets,
- queue saturation while the background worker is still alive.

These edge cases are not implicitly ignored; they are either validated in code or surfaced via metrics and alert events.

## Practical operational notes

When working in this module, the most important behavior to preserve is the combination of:

- deterministic state transitions,
- explicit lag and health reporting,
- fail-safe refusal rather than silent recovery,
- bounded, measured queue and transport behavior.

This is what makes the implementation production-grade rather than just structurally complete.

## Summary

The implementation is intentionally conservative: it tries to expose and validate failures rather than hide them. That is the central design principle of the replication subsystem and the reason its recovery, observability, and failover hooks stay useful under real operators' workloads.
