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

# Replication Trust and Security Boundaries

## Purpose

This document records the trust model for the replication subsystem so that developers and operators understand where valid cluster state originates, what is allowed to affect it, and which failure conditions must be rejected.

## Trust boundary

The replication module sits between durable local state and the wider cluster runtime. It decides whether a node is eligible to lead, whether a replica is healthy enough to accept reads or writes, and whether lag or placement constraints make a cluster state unsafe.

Because of this, the module is part of ThemisDB's trust-critical path. It handles state that can materially affect correctness, failover, and operator perception of data freshness.

## Key trust boundaries

### Leader election boundary

`ReplicationManager` is the authority for cluster role transitions. It decides whether a node can become leader or remain follower. The main trust risk is accidentally trusting a stale or unhealthy node.

### WAL and slot boundary

`WALEntry`, `ReplicationSlot`, and the associated persistence logic define the durability boundary for writes that are considered replicated or recoverable. The risk here is silent loss of ordering or replay continuity.

### Logical replication and CDC boundary

`LogicalReplicationManager` translates low-level WAL state into consumer-facing logical changes. Trust risk appears when schema compatibility or filter semantics do not hold, because downstream consumers will act on the produced stream.

### Transport and queue boundary

`AsyncWalShipper` is the remote transport boundary. Its risk is queue saturation, stale shipping state, or silent dropped segments. It must therefore preserve explicit lag and drop accounting.

## Controls explicitly present

The implementation includes several trust controls:

- explicit quorum checks,
- health and lag thresholds,
- fail-safe behavior when a replica fails or is unavailable,
- placement policy validation,
- slot state handling and persistence,
- deterministic conflict resolution semantics,
- metrics that make lag and queue behavior visible to operators.

## Security and reliability assumptions

The replication module assumes the following:

- node identity and endpoint metadata are not spoofed without cluster-level validation,
- the cluster topology is configured consistently,
- the transport path is not silently allowed to continue when seat-of-availability is degraded,
- failover decisions are not made by stale health data or policy mismatch,
- logical and WAL state remain consistent with the source-of-truth ordering and slot metadata.

## Incident handling

If the replication subsystem appears unhealthy, the operator should classify the issue before acting:

1. Is it a leader election problem or a replica health problem?
2. Is the issue queue saturation, lag, failover condition, or slot divergence?
3. Is the problem confined to a single replica or is it systemic across the cluster?
4. Are there explicit metrics or alerts showing the failure mode?
5. Is rollback or leader demotion required before rejoining a replica?

The module's design expects clear operational classification rather than implicit recovery.

## Summary

The replication module is security-relevant because it determines whether a data path is still authoritative, recoverable, and safe. Its trust posture depends on explicit health checks, deterministic merge rules, fail-safe lag handling, and visible operational signals rather than permissive fallback behavior.
