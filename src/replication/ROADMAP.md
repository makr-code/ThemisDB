<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Replication Module Roadmap

## Current Status
v1.x – Production-grade high-availability infrastructure. Leader-follower replication with Raft-like consensus, multi-master with CRDT conflict resolution, WAL shipping, CDC, and automatic failover are all implemented.

## Completed ✅
- [x] ReplicationManager – Raft-like leader election and follower management
- [x] Replication modes: SYNC, SEMI_SYNC, ASYNC
- [x] WAL shipping to followers with guaranteed durability
- [x] Automatic leader failover with health monitoring and heartbeat
- [x] Configurable `min_sync_replicas` for quorum writes
- [x] Multi-master replication with write-anywhere semantics
- [x] Conflict detection using vector clocks and Hybrid Logical Clocks (HLC)
- [x] Conflict resolution strategies: Last-Write-Wins (LWW), CRDT, custom resolvers
- [x] Change Data Capture (CDC) for event-driven ETL pipelines
- [x] Replication lag monitoring with threshold alerting
- [x] Read replica routing (primary, secondary, nearest)
- [x] Cross-datacenter and cross-region replication
- [x] Point-in-time recovery (PITR) via WAL replay
- [x] Cascading replication for hierarchical topologies
- [x] Selective replication (filter by collection, tenant, or pattern)
- [x] Prometheus metrics export

## In Progress 🚧
- [ ] Raft leader lease reads for linearizable read-scale-out (Target: Q2 2026)
- [ ] Replication topology visualizer (web UI) (Target: Q2 2026)
- [ ] Compressed WAL shipping (Zstd) for bandwidth reduction (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Witness node support (vote-only, no data) for quorum in 2-node clusters
- [ ] Replication slot management API (pause/resume individual slots)
- [ ] CDC event filtering by operation type (INSERT/UPDATE/DELETE)
- [ ] Automated lag-based read traffic shifting
- [ ] Cross-cluster logical replication (publish/subscribe model)

### Long-term (6-12 months)
- [ ] Full Raft v2 implementation (joint consensus for membership changes)
- [ ] Multi-region active-active with bounded staleness guarantees
- [ ] Schema-aware CDC with Avro/Protobuf schema registry integration
- [ ] Conflict-free Replicated Data Types (CRDT) library expansion
- [ ] Kubernetes operator for automated topology management

## Implementation Phases

### Phase 1: Leader-Follower & Multi-Master Replication (Status: Completed ✅)
- [x] `ReplicationManager` – Raft-like leader election and follower management
- [x] SYNC, SEMI_SYNC, and ASYNC replication modes
- [x] WAL shipping to followers with guaranteed durability
- [x] Automatic leader failover with health monitoring and heartbeat
- [x] Configurable `min_sync_replicas` for quorum writes
- [x] Multi-master replication with write-anywhere semantics
- [x] Conflict detection using vector clocks and Hybrid Logical Clocks (HLC)
- [x] Conflict resolution strategies: Last-Write-Wins (LWW), CRDT, custom resolvers
- [x] Change Data Capture (CDC) for event-driven ETL pipelines
- [x] Replication lag monitoring with threshold alerting
- [x] Read replica routing (primary, secondary, nearest)
- [x] Cross-datacenter and cross-region replication
- [x] Point-in-time recovery (PITR) via WAL replay
- [x] Cascading replication for hierarchical topologies
- [x] Selective replication (filter by collection, tenant, or pattern)
- [x] Prometheus metrics export

### Phase 2: Raft Lease Reads & WAL Compression (Status: In Progress 🚧)
- [~] Raft leader lease reads for linearizable read-scale-out
- [~] Replication topology visualizer (web UI)
- [~] Compressed WAL shipping (Zstd) for bandwidth reduction

### Phase 3: Witness Nodes & Slot Management (Status: Planned 📋)
- [ ] Witness node support (vote-only, no data) for quorum in 2-node clusters
- [ ] Replication slot management API (pause/resume individual slots)
- [ ] CDC event filtering by operation type (INSERT/UPDATE/DELETE)
- [ ] Automated lag-based read traffic shifting
- [ ] Cross-cluster logical replication (publish/subscribe model)

### Phase 4: Full Raft v2 & Multi-Region Active-Active (Status: Planned 📋)
- [ ] Full Raft v2 implementation (joint consensus for membership changes)
- [ ] Multi-region active-active with bounded staleness guarantees
- [ ] Schema-aware CDC with Avro/Protobuf schema registry integration
- [ ] Conflict-free Replicated Data Types (CRDT) library expansion
- [ ] Kubernetes operator for automated topology management

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (failover, lag detection, PITR restoration)
- [ ] Performance benchmarks (replication lag p99, WAL throughput)
- [ ] Security audit (WAL encryption in transit, CDC stream authentication)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- Raft implementation is Raft-like (not a full specification-compliant implementation); joint consensus for membership changes is planned.
- Cascading replication increases end-to-end lag proportionally to chain depth.
- CDC stream authentication is the responsibility of downstream consumers.

## Breaking Changes
- `ReplicationConfig` struct is stable from v1.x; new optional fields only.
- CDC event format may gain new metadata fields in v1.5.0; consumers should use open-ended deserialization.
