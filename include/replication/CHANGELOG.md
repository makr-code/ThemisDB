<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Replication Module

All notable changes to public headers are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation details in `../../src/replication/CHANGELOG.md`.

## [1.7.0] — 2026-01

### Added
- `logical_replication.h` — `LogicalReplicationManager` with schema-aware slots, DDL streaming, and data transformation hooks.
- `schema_cdc.h` — `SchemaCdc` DDL change data capture.

## [1.5.0] — 2025-09

### Added
- `raft_v2.h` — `RaftConsensus` Raft v2 leader election and log replication.
- `crdt_types.h` — `FlagEW` (Enable-Wins) and `FlagDW` (Disable-Wins) CRDT primitives.
- `multi_master_replication.h` — `MultiMasterReplication` bidirectional multi-master.
- `conflict_resolution.h` — `ConflictResolver` multi-master conflict strategies.
- Read replica support in `ReplicationManager`.
- `replication_slot.h` — `ReplicationSlot` individual slot lifecycle.
- Snapshot replication in `ReplicationManager`.

## [1.3.0] — 2025-06

### Added
- `multi_tier_replication.h` — `MultiTierReplication` primary → regional → edge.
- `observability.h` — `ReplicationObservability` lag/throughput/error metrics.
- `policy.h` — `ReplicationPolicy` sync/async/quorum/geo-affinity.
- `event_stream.h` — `EventStream` change event streaming.

## [1.0.0] — 2025-01

### Added
- `replication_manager.h` — `ReplicationManager` top-level orchestrator.
