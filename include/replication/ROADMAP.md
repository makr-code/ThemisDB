<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Replication Module Roadmap

## Current Status

v1.7.0 — production. Raft v2 consensus, schema-aware logical replication, multi-master CRDT resolution, multi-tier replication, CDC, event streaming, and observability are operational.

## Completed

- [x] `ReplicationManager` top-level orchestrator
- [x] `ReplicationPolicy` sync/async/quorum/geo-affinity
- [x] `EventStream` change event streaming
- [x] `MultiTierReplication` primary → regional → edge
- [x] `ReplicationObservability` lag/throughput/error metrics
- [x] `RaftConsensus` v2 leader election and log replication
- [x] `FlagEW` / `FlagDW` CRDT primitives
- [x] `MultiMasterReplication` bidirectional multi-master
- [x] `ConflictResolver` multi-master strategies
- [x] Read replicas and snapshot replication
- [x] `ReplicationSlot` individual slot lifecycle
- [x] `LogicalReplicationManager` schema-aware slots + DDL streaming
- [x] `SchemaCdc` DDL change data capture

## Implementation Phases

### Phase 1 — Core Replication ✅
- [x] `ReplicationManager` orchestrator
- [x] `ReplicationPolicy` definitions

### Phase 2 — Consensus & Multi-Master ✅
- [x] `RaftConsensus` v2
- [x] `MultiMasterReplication` + CRDT types
- [x] `ConflictResolver`

### Phase 3 — Logical Replication ✅
- [x] `ReplicationSlot` lifecycle
- [x] `LogicalReplicationManager` schema-aware
- [x] `SchemaCdc` DDL CDC

### Phase 4 — Observability & Policy ✅
- [x] `ReplicationObservability`
- [x] Geo-affinity policy
- [x] `EventStream` streaming

### Phase 5 — Future Enhancements (Planned)
- [ ] Global transaction ordering (Spanner-style TrueTime) (Target: Q4 2026)
- [x] Change stream Kafka export (Target: Q3 2026)
- [ ] Replication lag SLO alerting integration (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] All 12 headers documented
- [x] AUDIT.md — 0 open stubs

## Production Readiness Checklist

- [x] Raft v2 tested with leader partition and split-brain scenarios
- [x] CRDT FLAG_EW/FLAG_DW validated with concurrent multi-master writes
- [x] Logical replication DDL streaming tested across schema migrations
- [ ] Kafka change stream export (Target: Q3 2026)
