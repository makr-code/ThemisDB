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

# Replication Operations Guide

## Purpose

This document captures the operational model of the replication module: what to observe, which surfaces matter, how to recognize unhealthy state, and what the module is designed to expose to operators.

## Core operational surfaces

### Cluster health

The main cluster health model is in `ReplicationManager` and `ReplicaInfo`. The operational question is whether each replica is healthy, degraded, failed, or unknown, and whether quorum can still be established.

### Lag/cross-region transport

`AsyncWalShipper` and `LagAlertManager` are the primary surfaces for cross-region lag and queue health. They report:

- queue depth and resulting backpressure,
- lag alerts (warning, critical, failover),
- total bytes and segments shipped or dropped,
- maximum observed lag and recent lag histogram stats.

### Logical slot and CDC stream health

`LogicalReplicationManager` and `ReplicationSlotManager` provide the operator-facing visibility into slot lifecycle and logical change production. These surfaces matter when data is consumed downstream or when recovery requires replay or reinitialization.

## Key metrics and signals

The key operational metrics are:

- `replication_wal_lag_ms`
- `replication_wal_segments_enqueued_total`
- `replication_wal_segments_shipped_total`
- `replication_wal_segments_dropped_total`
- `replication_lag_ms{replica_id}`
- `lag_alert_triggered_total{replica_id}`
- `lag_critical_triggered_total{replica_id}`
- `failover_initiated_total{replica_id}`

This is consistent with the export logic in the replication manager and lag manager implementations.

## Operational thresholds

The module uses explicit thresholds rather than opaque tuning. The default thresholds in `SLOThresholds` are:

- alert threshold: 10s
- critical threshold: 30s
- failover threshold: 60s
- failover duration: 5 minutes

These values are part of the operator contract and may be tuned deliberately for a deployment.

## Operational checklists

### Healthy cluster checklist

- replication manager initialized and healthy,
- quorum exists,
- no unresolved lag alerts,
- no growing queue backlog,
- slot and logical replication output advancing,
- placement policy validation passes.

### Degraded cluster checklist

- replica is degraded or failing,
- lag exceeds warning threshold,
- queue is backing up or dropping segments,
- failover may be required if the condition persists across configured windows.

### Critical incident checklist

- failover trigger active,
- no healthy quorum,
- persistent slot or logical stream drift,
- widespread lag or cross-region transport failure.

## Recovery posture

The module prefers explicit operational action, not silent continuation. If a replica is unhealthy, the recovery path is to isolate, validate, and rejoin rather than to continue processing stale state as though it were healthy.

## Summary

The replication module is operationally visible through explicit cluster health, lag, queue, and failover signals. The module keeps those conditions observable so operators can act before a silent state divergence becomes a full cluster issue.
