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
- [x] Parallel replication — multi-threaded WAL application on followers with dependency tracking (v1.6.0)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Replication slot management API (pause/resume individual slots) (Issue: #2249)

### Long-term (6-12 months)
- [x] Full Raft v2 implementation (joint consensus for membership changes) (Issue: #2441)
- [x] Multi-region active-active with bounded staleness guarantees (Issue: #2254)
- [x] Schema-aware CDC with Avro/Protobuf schema registry integration (Issue: #2255)
- [x] Conflict-free Replicated Data Types (CRDT) library expansion (Issue: #2442)

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

## New Modules (v1.8.0 – Phase 4)
- [x] `include/replication/raft_v2.h` + `src/replication/raft_v2.cpp` — Full Raft v2: RaftV2ClusterConfig (joint consensus quorum), MembershipChangeManager (two-phase membership transitions), RaftV2State
- [x] `include/replication/crdt_types.h` — Standalone type-safe CRDT library: GrowOnlyCounter, PNCounter, LWWRegister, MVRegister, GrowOnlySet, TwoPSet, ORSet, LWWMap, RGArray, EnableWinsFlag, DisableWinsFlag
- [x] `include/replication/schema_cdc.h` + `src/replication/schema_cdc.cpp` — SchemaAwareCDCBridge: bridges replication WAL CDC events with CDC SchemaRegistryClient / CdcSchemaEncoder for Confluent-compatible schema-encoded output

### Phase 4: Full Raft v2 & Multi-Region Active-Active (Status: Completed ✅)
- [x] Full Raft v2 implementation (joint consensus for membership changes)
- [x] Multi-region active-active with bounded staleness guarantees (MultiRegionActiveActiveManager implemented)
- [x] Schema-aware CDC with Avro/Protobuf schema registry integration
- [x] Conflict-free Replicated Data Types (CRDT) library expansion

### Phase 4.5: Build System Audit (Status: Completed ✅ — March 2026)
- [x] All `src/replication/*.cpp` files verified registered in `cmake/CMakeLists.txt`
- [x] 3 focused standalone test targets added in `tests/CMakeLists.txt`: ReplicationHA, ReplicationNewFeatures, ReplicationTopologyApiHandler

### Phase 5: Parallel Replication (Status: Completed ✅ — v1.6.0)
- [x] `ParallelReplicationWorker` class in `include/replication/replication_manager.h` + `src/replication/replication_manager.cpp`
- [x] Multi-threaded WAL application on followers (configurable 1–64 worker threads)
- [x] Per-document dependency tracking to maintain causal consistency
- [x] Conflict-free parallel writes for independent document keys
- [x] `submit()` / `sync()` / `getStats()` public API
- [x] Config: `worker_threads`, `queue_size`, `use_dependency_tracking`, `group_transactions`
- [x] Stats: `entries_applied`, `dependencies_detected`, `average_latency_us`, `parallel_batches`, `parallelism_factor`
- [x] 16 unit tests covering: single entry, multi-entry, dependency tracking, no-tracking, mixed docs, stats, single/sixteen threads, large batches, group_transactions (enabled/disabled), average_latency_us (populated/zero)

### Phase 6: Quorum-Based Reads (Status: Completed ✅ — v1.6.0)
- [x] `QuorumReadManager` class in `include/replication/replication_manager.h` + `src/replication/replication_manager.cpp`
- [x] Concurrent per-replica reads with configurable `read_quorum` (default 2) and `read_timeout_ms`
- [x] Automatic conflict resolution: highest-version response wins; `had_conflicts` flag set on divergence
- [x] `repair_on_read`: stale replicas logged for background repair when divergence is detected
- [x] Session consistency: `session_token` returned from every successful read, accepted on next read to enforce monotonic (read-your-writes) guarantees
- [x] Session tokens encode embedded version + expiry timestamp; expired tokens degrade gracefully to no-session reads
- [x] `setReplicas()` for dynamic topology updates (thread-safe via `shared_mutex`)
- [x] Config: `read_quorum`, `read_timeout_ms`, `repair_on_read`, `session_token_ttl_ms`
- [x] 13 unit tests: basic quorum, highest-version selection, conflict detection, no-healthy-replicas failure, single-node mode, topology update, session token returned, monotonic reads, stale quorum failure, partial version satisfaction, session quorum satisfied by fresh replicas, repair-on-read conflict flagging, single-node session token

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (238+ test cases: 225 previous + 13 QuorumReadManager tests)
- [x] Integration tests (failover, lag detection, PITR restoration, cross-cluster end-to-end)
- [x] Performance benchmarks (WAL append > 50 000 entries/s, WAL readFrom 1000 < 5 ms, serialize/deserialize < 2 µs) — `benchmarks/bench_replication_throughput.cpp`
- [x] Focused standalone test targets: `ReplicationHAFocusedTests`, `ReplicationNewFeaturesFocusedTests`, `MultiRegionActiveActiveTests`, `CacheReplicationTests`
- [?] Security audit (WAL encryption in transit, CDC stream authentication)
- [x] Documentation complete (replication-ha-guide.md, REPLICATION_IMPLEMENTATION_STATUS.md)
- [x] API stability guaranteed (ReplicationConfig stable; new classes are additive)

## Known Issues & Limitations
- Cascading replication increases end-to-end lag proportionally to chain depth.
- CDC stream authentication is the responsibility of downstream consumers.
- SchemaAwareCDCBridge start()/stop() lifecycle relies on the ReplicationManager remaining alive; users should call stop() before destroying the ReplicationManager.

## Breaking Changes
- `ReplicationConfig` struct is stable from v1.x; new optional fields only.
- CDC event format may gain new metadata fields in v1.5.0; consumers should use open-ended deserialization.
