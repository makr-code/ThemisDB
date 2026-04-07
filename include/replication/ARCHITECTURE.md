<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Replication Module — Architecture Guide

## Overview

The replication module provides all data synchronization infrastructure for ThemisDB: Raft v2 consensus, logical replication with schema-aware slots and DDL streaming, multi-master CRDT-based conflict resolution, multi-tier replication, snapshot replication, read replicas, change data capture (schema CDC), and event streaming.

## Design Principles

- **Raft v2 consensus** — `raft_v2.h` provides leader election, log replication, and membership reconfiguration.
- **Schema-aware slots** — `LogicalReplicationManager` tracks DDL changes alongside data; consumers receive coherent schema + data streams.
- **CRDT conflict resolution** — `crdt_types.h` provides FLAG_EW (Enable-Wins) and FLAG_DW (Disable-Wins) flags for multi-master.
- **Policy-driven** — `policy.h` defines replication policies (sync/async, quorum, geo-affinity) separate from mechanism.
- **Observable** — `observability.h` exposes per-slot lag, throughput, and error counters.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `conflict_resolution.h` | `ConflictResolver` | Multi-master write conflict resolution strategies |
| `crdt_types.h` | `FlagEW`, `FlagDW`, CRDT primitives | FLAG_EW (Enable-Wins) and FLAG_DW (Disable-Wins) flags |
| `event_stream.h` | `EventStream` | Change event streaming to consumers |
| `logical_replication.h` | `LogicalReplicationManager` | Schema-aware logical replication slots; DDL streaming; data transformation hooks |
| `multi_master_replication.h` | `MultiMasterReplication` | Bidirectional multi-master replication |
| `multi_tier_replication.h` | `MultiTierReplication` | Tiered replication (primary → regional → edge) |
| `observability.h` | `ReplicationObservability` | Per-slot lag, throughput, and error metrics |
| `policy.h` | `ReplicationPolicy` | Sync/async, quorum, geo-affinity policy definitions |
| `raft_v2.h` | `RaftConsensus` | Raft v2 leader election and log replication |
| `replication_manager.h` | `ReplicationManager` | Top-level replication orchestrator |
| `replication_slot.h` | `ReplicationSlot` | Individual logical replication slot lifecycle |
| `schema_cdc.h` | `SchemaCdc` | Schema change data capture (DDL event streaming) |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `network` | `RaftConsensus`, `LogicalReplicationManager` | Raft log replication over gRPC transport |
| `storage` | `ReplicationSlot`, `SchemaCdc` | WAL-based slot consumption |
| `observability` | `ReplicationObservability` | Replication lag and throughput metrics |
| `scheduler` | `ReplicationManager` | Replication task scheduling |

## Implementation

Implementation in `../../src/replication/`.
