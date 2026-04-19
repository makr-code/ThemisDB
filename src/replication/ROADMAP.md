> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Replication Module Roadmap

## Current Status
v1.x – Production-grade high-availability infrastructure. Leader-follower replication with Raft-like consensus, multi-master with CRDT conflict resolution, WAL shipping with Zstd compression, CDC, and automatic failover are all implemented.  WAL Archival to Object Storage (v1.6.0) adds AES-256-GCM encryption at rest, pluggable cloud backends (S3/GCS/Azure via IArchivalBackend), storage-tier lifecycle management (standard→cold→glacier), and configurable retention policy.

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
- [x] LogicalReplicationManager — schema-aware logical slots with per-collection filters, row predicates, DDL streaming, cross-version transforms, and parallel decoding for high throughput (Issue: #95)
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
- [x] `BidirectionalReplicationManager` in `include/replication/replication_manager.h` + `src/replication/replication_manager.cpp` — Active-active bidirectional replication with origin tracking, per-collection conflict resolution (LWW/FIRST_WRITE/VECTOR_CLOCK/CUSTOM), DDL replication, and loop prevention

### Phase 7: Bidirectional Replication (Status: Completed ✅ — v1.7.0)
- [x] `BidirectionalReplicationManager` class with `BidiConfig` (local/remote node IDs, conflict strategy, origin tracking, DDL replication flags)
- [x] Symmetric replication: both nodes are primary; `submitWrite()` enqueues changes for peer forwarding
- [x] Conflict detection: concurrent writes from different origin nodes detected via `detectConflict()`; handled by `handleConflict()`
- [x] Configurable conflict resolution per collection: `setCollectionStrategy()` / `getEffectiveStrategy()` with LWW, FIRST_WRITE_WINS, VECTOR_CLOCK, and CUSTOM strategies
- [x] Origin tracking: every write tagged with `origin_node` + `origin_seq` + `timestamp_ms`; loop prevention via `replicate_foreign_changes=false` (default)
- [x] DDL replication: `applyRemoteDDL()` forwards schema changes with conflict detection; `is_ddl_conflict` flag on conflict records
- [x] `SyncStatus`: `local_sequence`, `remote_sequence`, `lag_ms`, `conflicts_detected`, `conflicts_resolved`, `conflicts_last_hour` (rolling 60-min window), `is_running`, `is_synchronized`
- [x] `bidirectional_sync = false` flag honoured: `applyRemoteWrite()` returns false when disabled
- [x] `replicate_ddl = false` flag honoured: `applyRemoteDDL()` returns false when disabled
- [x] Manual conflict resolution: `resolveConflict(document_id, winner_node)` for CUSTOM strategy pending conflicts
- [x] `BidiConflictRecord`: full audit trail with local/remote/resolved writes, strategy used, timestamp, DDL flag
- [x] `updateRemoteSequence()` + `applyRemoteWrite()` for simulation/integration without a live network layer
- [x] 21 unit tests in `tests/test_replication_ha.cpp` (BidirectionalReplicationTest suite):
  - AC-1: start/stop lifecycle, double-start idempotency, invalid config (same node IDs, empty IDs)
  - AC-1/6: submitWrite advances local sequence; returns 0 when stopped
  - AC-4: origin tracking rejects own change bouncing back; accepts peer changes; updates remote_sequence
  - AC-2: concurrent writes from different origins detected as conflict; LWW picks higher-timestamp winner
  - AC-3: per-collection strategy override (FIRST_WRITE_WINS); CUSTOM strategy produces pending conflict
  - AC-7: manual resolveConflict picks winner node; returns false for unknown node
  - AC-5: DDL replication accepted; DDL conflict recorded with is_ddl_conflict=true
  - AC-8: SyncStatus reflects lag, remote_sequence, is_synchronized; not synchronized under high lag
  - Audit-1: `bidirectional_sync=false` blocks incoming remote writes
  - Audit-2: `replicate_ddl=false` blocks DDL apply
  - Audit-3: `conflicts_last_hour` rolling window correctly counted in SyncStatus
- [x] CI: `.github/workflows/bidirectional-replication-ci.yml`

### Phase 8: Geo-Replication with Consistency Levels (Status: Completed ✅ — v1.7.0)
- [x] `GeoReplicationManager` class in `include/replication/replication_manager.h` + `src/replication/replication_manager.cpp`
- [x] `GeoConfig`: `local_region`, `regions`, `replication_factor`, `local_replicas`, `global_replicas`, `default_consistency`, `max_staleness_ms`, `session_token_ttl_ms`
- [x] `write(key, value, consistency)` — STRONG write rejected when local staleness > 0; all other levels always succeed locally; advances monotonic sequence
- [x] `read(key, consistency, session_token)` — returns routed region string or `std::nullopt` on constraint violation
- [x] `selectReadRegion()` — automatic routing per consistency level:
  - STRONG: local region only when `staleness_ms == 0`; falls back to any healthy region with zero lag
  - BOUNDED_STALENESS: prefers local region if within `max_staleness_ms`; picks freshest eligible peer otherwise
  - SESSION: local region when `last_applied_sequence >= token_sequence`; uses `parseSessionToken()` for expiry check
  - EVENTUAL: always returns local region (no routing overhead)
- [x] `getSessionToken()` — opaque `seq=N;region=R;exp=T` token for read-your-writes guarantee
- [x] `parseSessionToken()` — validates TTL expiry before returning embedded sequence
- [x] `updateRegionStaleness()` — feeds WAL acknowledgement / heartbeat data from replication layer
- [x] `exportPrometheusMetrics()` — per-level read counters, accepted write counter, rejection counter, per-region lag gauges
- [x] 34 unit tests in `tests/test_geo_replication_consistency.cpp` → `GeoReplicationConsistencyFocusedTests`:
  - Construction, initial state (local fresh, peers unknown)
  - Staleness updates (refresh, add new region, update local)
  - Session tokens (non-empty, contains seq/region, advances after write, invalid token returns 0)
  - Automatic routing for all four consistency levels
  - Write behaviour (STRONG success/failure, SESSION/EVENTUAL/BOUNDED always succeed)
  - Read behaviour (per-level success and rejection)
  - SESSION read-your-writes guarantee
  - Prometheus metrics (key presence, counter accuracy)
  - Thread safety (concurrent writes, reads, staleness updates)
- [x] CI: `.github/workflows/geo-replication-consistency-ci.yml`

## New Modules (v1.8.0 – Phase 4)
- [x] `include/replication/raft_v2.h` + `src/replication/raft_v2.cpp` — Full Raft v2: RaftV2ClusterConfig (joint consensus quorum), MembershipChangeManager (two-phase membership transitions), RaftV2State
- [x] `include/replication/crdt_types.h` — Standalone type-safe CRDT library: GrowOnlyCounter, PNCounter, LWWRegister, MVRegister, GrowOnlySet, TwoPSet, ORSet, LWWMap, RGArray, EnableWinsFlag, DisableWinsFlag
- [x] `include/replication/schema_cdc.h` + `src/replication/schema_cdc.cpp` — SchemaAwareCDCBridge: bridges replication WAL CDC events with CDC SchemaRegistryClient / CdcSchemaEncoder for Confluent-compatible schema-encoded output
- [x] `include/replication/multi_tier_replication.h` + `src/replication/multi_tier_replication.cpp` — MultiTierReplicationManager: hierarchical replication with TIER_1_CRITICAL (3+ replicas, sync, <10ms), TIER_2_STANDARD (2 replicas, semi-sync, <50ms), TIER_3_ARCHIVAL (1 replica, async); per-collection tier assignment; automatic tier promotion/demotion based on access patterns

### Phase 4: Full Raft v2 & Multi-Region Active-Active (Status: Completed ✅)
- [x] Full Raft v2 implementation (joint consensus for membership changes)
- [x] Multi-region active-active with bounded staleness guarantees (MultiRegionActiveActiveManager implemented)
- [x] Schema-aware CDC with Avro/Protobuf schema registry integration
- [x] Conflict-free Replicated Data Types (CRDT) library expansion

### Phase 4.5: Build System Audit (Status: Completed ✅ — March 2026)
- [x] All `src/replication/*.cpp` files verified registered in `cmake/CMakeLists.txt` (including `logical_replication.cpp`)
- [x] 3 focused standalone test targets added in `tests/CMakeLists.txt`: ReplicationHA, ReplicationNewFeatures, ReplicationTopologyApiHandler
- [x] `logical_replication.cpp` added to `THEMIS_CORE_SOURCES` in `cmake/CMakeLists.txt` (was only in `ModularBuild.cmake`)
- [x] 4 parallel-decoding tests added to `tests/test_logical_replication.cpp` (AC-6–AC-9)
- [x] CI workflow added: `.github/workflows/02-feature-modules_replication_logical-replication-parallel-decoding-ci.yml`

### Phase 5: Parallel Replication (Status: Completed ✅ — v1.6.0)
- [x] `ParallelReplicationWorker` class in `include/replication/replication_manager.h` + `src/replication/replication_manager.cpp`
- [x] Multi-threaded WAL application on followers (configurable 1–64 worker threads)
- [x] Per-document dependency tracking to maintain causal consistency
- [x] Conflict-free parallel writes for independent document keys
- [x] `submit()` / `sync()` / `getStats()` public API
- [x] Config: `worker_threads`, `queue_size`, `use_dependency_tracking`, `group_transactions`
- [x] Stats: `entries_applied`, `dependencies_detected`, `average_latency_us`, `parallel_batches`, `parallelism_factor`
- [x] 16 unit tests covering: single entry, multi-entry, dependency tracking, no-tracking, mixed docs, stats, single/sixteen threads, large batches, group_transactions (enabled/disabled), average_latency_us (populated/zero)

### Phase 6: Compressed Replication (Status: Completed ✅ — v1.6.0)
- [x] `CompressedReplicationStream` class in `include/replication/replication_manager.h` + `src/replication/replication_manager.cpp`
- [x] Multiple compression algorithms: `NONE`, `LZ4` (fast, moderate ratio), `ZSTD` (best ratio), `SNAPPY` (very fast), `AUTO` (size-based selection)
- [x] Adaptive compression via `CompressionConfig::adaptive` flag: `adaptive=true` (default) skips compression below `min_batch_size`; `adaptive=false` always compresses in AUTO mode
- [x] Configurable compression level (1–9, applies to ZSTD; ignored by LZ4/Snappy)
- [x] `CompressionStats` for monitoring: `bytes_uncompressed`, `bytes_compressed`, `compression_ratio`, `algorithm_used`
- [x] `sendBatch()` serialises WAL entries, selects algorithm, compresses, and tracks statistics
- [x] `decompress()` for received network buffers (LZ4/ZSTD/Snappy/NONE round-trip verified)
- [x] `resetStats()` to clear accumulated statistics
- [x] `ReplicationConfig` integration: `enable_wal_compression`, `wal_compression_algorithm`, `wal_compression_level`, `wal_compression_min_batch_bytes`
- [x] `ReplicationStream` delegates `sendBatch()` to `CompressedReplicationStream` when `enable_wal_compression` is set
- [x] 22 unit tests: 15 `CompressedStreamTest` + 7 `ReplicationStreamCompressionTest`
  - JSON-like data achieves ≥ 5x ratio with ZSTD level 6 (AC: JSON 5-10x)
  - Already-compressed data yields ≤ 1.2x (AC: ~1x for already compressed)
  - LZ4 and Snappy round-trip correctness verified
  - `adaptive=false` forces compression even below `min_batch_size`
- [x] CI: `.github/workflows/compressed-replication-ci.yml`

## New Modules (v1.8.0 – Phase 8: Multi-Tier Replication)
- [x] `include/replication/multi_tier_replication.h` + `src/replication/multi_tier_replication.cpp` — MultiTierReplicationManager: hierarchical replication with TIER_1_CRITICAL (3+ replicas, sync, <10ms), TIER_2_STANDARD (2 replicas, semi-sync, <50ms), TIER_3_ARCHIVAL (1 replica, async); per-collection tier assignment; automatic tier promotion/demotion based on access patterns

### Phase 8: Multi-Tier Replication (Status: Completed ✅ — v1.8.0)
- [x] `MultiTierReplicationManager` class with `MultiTierConfig` (auto-tiering thresholds, default tier, per-tier overrides)
- [x] Three-tier model: `TIER_1_CRITICAL` (3+ replicas, SYNC, <10ms SLA), `TIER_2_STANDARD` (2 replicas, SEMI_SYNC, <50ms SLA), `TIER_3_ARCHIVAL` (1 replica, ASYNC)
- [x] `assignTier(collection, tier)` / `removeTier(collection)` / `getTier(collection)` for per-collection tier management
- [x] `getTierConfig(collection)` returns full `TierConfig` (replica_count, mode, max_latency_ms, min_availability_pct)
- [x] `getDefaultTierConfig(tier)` returns built-in or overridden `TierConfig` per tier
- [x] `enableAutoTiering(bool)` / `isAutoTieringEnabled()` to toggle automatic tier adjustment
- [x] `recordAccess(collection)` tracks per-collection access rates for auto-tiering decisions
- [x] `evaluateTierPromotion(collection)`: hot data (rate ≥ hot_access_threshold) → TIER_1_CRITICAL; cold data (rate < cold_access_threshold) → TIER_3_ARCHIVAL; moderate → TIER_2_STANDARD
- [x] `getStats()` → `MultiTierStats` with per-tier collection counts, lifetime promotions/demotions, auto-tiering flag
- [x] `getCollectionStats()` → per-collection `CollectionAccessStats` (total/recent accesses, rate, current tier, last promotion/demotion timestamps)
- [x] `getCollectionsForTier(tier)` enumerates all explicitly assigned collections for a tier
- [x] Thread-safe: all public methods protected via `shared_mutex` (assignments + stats independently locked)
- [x] 19 unit tests in `tests/test_replication_ha.cpp` (MultiTierReplicationTest suite):
  - AC-1: Tier 1 default config has 3+ replicas, SYNC mode, ≤10ms latency
  - AC-2: Tier 2 default config has 2 replicas, SEMI_SYNC, ≤50ms latency
  - AC-3: Tier 3 default config has 1 replica, ASYNC
  - AC-4: assignTier/getTier/removeTier per-collection; unassigned returns default; override existing assignment; getCollectionsForTier; getTierConfig reflects assignment
  - AC-5: auto-tiering disabled by default; enableAutoTiering toggle; recordAccess no-op when disabled; hot collection promoted to Tier 1; cold collection demoted to Tier 3; moderate access normalised to Tier 2; evaluateTierPromotion no-op when auto-tiering disabled
  - Stats: getStats reflects assignments; promotions and demotions counted; getCollectionStats includes all tracked collections
  - Custom tier config override replaces built-in defaults
- [x] CI: `.github/workflows/multi-tier-replication-ci.yml`


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
### Phase 6: WAL Archival to Object Storage (Status: Completed ✅ — v1.6.0)
- [x] `IArchivalBackend` interface for pluggable cloud backends (S3, GCS, Azure Blob)
- [x] `WALArchivalManager::ArchivalConfig` extended with cloud fields: `storage_type`, `bucket_name`, `prefix`
- [x] AES-256-GCM encryption at rest: `encrypt_at_rest` + `encryption_key_hex` (32-byte key as 64 hex chars)
- [x] Storage tier tracking per archived segment: `ArchivedSegment::storage_tier` ("standard", "cold", "glacier")
- [x] Lifecycle management: `transitionStorageTiers()` promotes segments through tiers based on `transition_to_cold_after_days`
- [x] Backward-compatible index format: old 7-field index.txt lines load with `storage_tier="standard"`, `encrypted=false`
- [x] 8 new focused tests in `tests/test_replication_ha.cpp` (WALArchivalTest suite): encryption round-trip, compress+encrypt, missing-key fallback, default tier, lifecycle disabled, cold transition, glacier transition, index persistence
- [x] CI workflow: `.github/workflows/wal-archival-object-storage-ci.yml`

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (272+ test cases: previous 268 + 4 perf tests for Design Constraints #1/#2/#4)
- [x] Integration tests (failover, lag detection, PITR restoration, cross-cluster end-to-end)
- [x] Performance benchmarks (WAL append > 50 000 entries/s, WAL readFrom 1000 < 5 ms, serialize/deserialize < 2 µs) — `benchmarks/bench_replication_throughput.cpp`
- [x] Performance tests for Design Constraint #4 (VectorClock+HLC overhead < 5 µs) — `VectorClockPerfTest` + `HLCPerfTest` in `tests/test_replication_ha.cpp` (set `THEMIS_RUN_PERF_TESTS=1`)
- [x] Focused standalone test targets: `ReplicationHAFocusedTests`, `ReplicationNewFeaturesFocusedTests`, `MultiRegionActiveActiveTests`, `CacheReplicationTests`
- [x] WAL encryption at rest (AES-256-GCM) via `WALArchivalManager::ArchivalConfig::encrypt_at_rest`
- [?] Security audit (WAL encryption in transit, CDC stream authentication)
- [x] Documentation complete (replication-ha-guide.md, REPLICATION_IMPLEMENTATION_STATUS.md)
- [x] API stability guaranteed (ReplicationConfig stable; new classes are additive)
- [x] Usage example added (`examples/replication/example_replication.cpp`)

## Known Issues & Limitations
- Cascading replication increases end-to-end lag proportionally to chain depth.
- CDC stream authentication is the responsibility of downstream consumers.
- SchemaAwareCDCBridge start()/stop() lifecycle relies on the ReplicationManager remaining alive; users should call stop() before destroying the ReplicationManager.

## Breaking Changes
- `ReplicationConfig` struct is stable from v1.x; new optional fields only.
- CDC event format may gain new metadata fields in v1.5.0; consumers should use open-ended deserialization.
