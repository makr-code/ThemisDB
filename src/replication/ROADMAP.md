<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Replication Module Roadmap

## Current Status
v1.x – Production-grade high-availability infrastructure. Leader-follower replication with Raft-like consensus, multi-master with CRDT conflict resolution, WAL shipping with Zstd compression, CDC, and automatic failover are all implemented.

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
- [x] Raft leader lease reads for linearizable read-scale-out (Issue: #2258)
- [x] Replication topology visualizer (web UI) (Issue: #2443)
- [x] Compressed WAL shipping (Zstd) for bandwidth reduction (Issue: #2444)
- [x] Automated lag-based read traffic shifting (Issue: #2251)
- [x] Cross-cluster logical replication (publish/subscribe model) (Issue: #2440)
- [x] Kubernetes operator for automated topology management (Issue: #2257)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Replication slot management API (pause/resume individual slots) (Issue: #2249)

### Long-term (6-12 months)
- [ ] Full Raft v2 implementation (joint consensus for membership changes) (Issue: #2441)
- [x] Multi-region active-active with bounded staleness guarantees (Issue: #2254)
- [P] Schema-aware CDC with Avro/Protobuf schema registry integration (Issue: #2255)
- [I] Conflict-free Replicated Data Types (CRDT) library expansion (Issue: #2442)

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

### Phase 2: Raft Lease Reads & WAL Compression (Status: Completed ✅)
- [x] Raft leader lease reads for linearizable read-scale-out
- [x] Replication topology visualizer (web UI)
- [x] Compressed WAL shipping (Zstd) for bandwidth reduction

### Phase 3: Witness Nodes & Slot Management (Status: Completed ✅)
- [x] Witness node support (vote-only, no data) for quorum in 2-node clusters (Issue: #2154, #2001)
- [x] Replication slot management API (pause/resume individual slots)
- [x] CDC event filtering by operation type (INSERT/UPDATE/DELETE)
- [x] Automated lag-based read traffic shifting
- [x] Cross-cluster logical replication (publish/subscribe model)

## New Modules (v1.7.0)
- [x] `include/replication/observability.h` + `src/replication/observability.cpp` — ReplicationObserver with lag snapshots, topology, bottleneck detection, and health scores
- [x] `include/replication/conflict_resolution.h` + `src/replication/conflict_resolution.cpp` — ThreeWayMergeResolver (git-style) and FieldLevelMergeResolver (UNION/INTERSECT/LEFT_BIAS/RIGHT_BIAS)
- [x] `include/replication/event_stream.h` + `src/replication/event_stream.cpp` — ReplicationEventStream with RAII subscription handles
- [x] `include/replication/policy.h` + `src/replication/policy.cpp` — ReplicationPolicy with per-collection policy assignment and topology validation
- [x] `include/replication/replication_slot.h` + `src/replication/replication_slot.cpp` — ReplicationSlot / ReplicationSlotManager
- [x] All 5 v1.7.0 files registered in `cmake/ModularBuild.cmake` THEMIS_TRANSACTION_SOURCES

### Phase 4: Full Raft v2 & Multi-Region Active-Active (Status: In Progress 🚧)
- [ ] Full Raft v2 implementation (joint consensus for membership changes)
- [x] Multi-region active-active with bounded staleness guarantees (MultiRegionActiveActiveManager implemented)
- [P] Schema-aware CDC with Avro/Protobuf schema registry integration
- [ ] Conflict-free Replicated Data Types (CRDT) library expansion

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (209 test cases: 177 original + 30 new v1.7.0 feature tests + 2 commit-index tests)
- [x] Integration tests (failover, lag detection, PITR restoration, cross-cluster end-to-end)
- [x] Performance benchmarks (WAL append > 50 000 entries/s, WAL readFrom 1000 < 5 ms, serialize/deserialize < 2 µs) — `benchmarks/bench_replication_throughput.cpp`
- [x] Focused standalone test targets: `ReplicationHAFocusedTests`, `ReplicationNewFeaturesFocusedTests`, `MultiRegionActiveActiveTests`, `CacheReplicationTests`
- [?] Security audit (WAL encryption in transit, CDC stream authentication)
- [x] Documentation complete (replication-ha-guide.md, REPLICATION_IMPLEMENTATION_STATUS.md)
- [x] API stability guaranteed (ReplicationConfig stable; new classes are additive)

## Known Issues & Limitations
- Raft implementation is Raft-like (not a full specification-compliant implementation); joint consensus for membership changes is planned.
- Cascading replication increases end-to-end lag proportionally to chain depth.
- CDC stream authentication is the responsibility of downstream consumers.

## Breaking Changes
- `ReplicationConfig` struct is stable from v1.x; new optional fields only.
- CDC event format may gain new metadata fields in v1.5.0; consumers should use open-ended deserialization.
