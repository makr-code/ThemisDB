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

# Replication Embedding and Integration Notes

## Purpose

This document explains how the replication module fits into ThemisDB as an operational and data-plane subsystem. The module is not an isolated library; it is the runtime boundary that coordinates cluster state, local mutation durability, and downstream CDC/logical replication consumers.

## Embedded responsibilities

The replication subsystem is the place where the system coordinates the following responsibilities:

- durable local write record creation through WAL entries,
- replicate/failover orchestration across cluster peers,
- containment of lag and health events,
- logical change generation for downstream CDC consumers,
- deterministic conflict resolution for multi-master or concurrent-write contexts.

The actual implementation distribution is visible across the following files:

- `include/replication/replication_manager.h`
- `include/replication/async_wal_shipper.h`
- `include/replication/lag_alert_manager.h`
- `src/replication/replication_manager.cpp`
- `src/replication/logical_replication.cpp`
- `src/replication/raft_v2.cpp`
- `src/replication/conflict_resolution.cpp`
- `src/replication/replication_slot.cpp`
- `src/replication/geo_placement.cpp`

## Upstream and downstream relationships

### Upstream relationships

The module depends on the durable write and state semantics it receives from storage and WAL-oriented infrastructure. The source and architecture docs clearly treat replication as a consumer of durable state and a producer of cluster-wide behavior.

### Downstream relationships

The downstream consumers are the parts of ThemisDB that need either:

- a leader/follower or multi-node synchronized state,
- a CDC stream of logical changes,
- a failover or placement-aware read routing decision,
- alerting / metrics / operational controls for cluster health.

The server and operational surfaces rely on the module's state for clean behavior under load, while consumer modules can use the logical replication stream without directly owning replication policy.

## CDC and logical stream embedding

`LogicalReplicationManager` is the bridge from raw WAL state to logical row- or operation-level changes. That means the module is deeply embedded in change-data-capture semantics even when the primary writer state continues to live in the storage and replication managers.

This is the exact place where ThemisDB turns a low-level append stream into a semantically useful consumer-facing replication stream, filtering and transforming the changes based on configured logical filters and collection keys.

## Failover and observability embedding

The module also embeds failover and observability into the operational core:

- `LagAlertManager` monitors replica lags and raises operational alerts,
- `ReplicationManager` provides the health model and failover transitions,
- `exportPrometheusMetrics()` surfaces metrics for operator dashboards and SLO checks,
- `readiness`-type patterns are encoded in cluster health and read routing decisions.

This keeps operational concerns inside the replication subsystem instead of scattering them across unrelated codepaths.

## Embedded invariants

The integration contract depends on a few critical invariants:

- the module must not silently claim success when a durability or quorum condition is not met,
- lag reporting must be explicit and comparable across replicas,
- slot, WAL, and logical change states must not drift silently,
- failover must only occur after health and placement checks pass,
- partition or timeout conditions must be surfaced as operational decisions.

## Summary

The replication module is embedded in ThemisDB as the authoritative runtime for availability and smooth continuity across replicas. It sits at the boundary between durable local state and cluster-wide operational behavior, and it carries the operational signals necessary for correct failover, CDC consumption, and health management.
