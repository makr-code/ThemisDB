> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Temporal Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**v1.2.0** — All C++ temporal query engine features are production-ready.  Core capabilities include system-versioned tables, bi-temporal tables, full time-travel queries (AS OF, FROM…TO, BETWEEN…AND), application-time queries, temporal joins, SEQUENCED/NON-SEQUENCED semantics, index-accelerated queries, and result caching.  SQL `PERIOD FOR` DDL syntax and AQL clause parsing remain deferred to Phase 3b.

## Completed ✅
- [x] HLC-based temporal conflict resolver with multiple policies (last-write-wins, first-write-wins, node-priority, manual, CRDT-merge) (`temporal_conflict_resolver.cpp`)
- [x] `TemporalConflictDetector`: detects CONCURRENT_UPDATE, OVERLAPPING_PERIODS, REFERENTIAL_INTEGRITY, and UNIQUENESS_VIOLATION conflicts between temporal snapshots (`temporal_conflict_resolver.cpp`)
- [x] Optimistic-locking conflict detection via concurrent HLC comparison (neither snapshot happened-before the other)
- [x] Auto-resolution of detected conflicts with configurable ConflictPolicy delegation
- [x] Manual-resolution queue: queue, retrieve, and clear unresolved conflicts
- [x] System-versioned table: automatic transaction-time versioning, non-destructive updates (`system_versioned_table.cpp`)
- [x] **Full System-Versioned Table Support (v1.1.0):** `Config` struct (retention_period, compress_history, track_user_id, history_table_name), `createVersionedTable` DDL factory, `upsert`, `enforceRetentionPolicy`, user-attribution tracking (`system_versioned_table.cpp`)
- [x] Bi-temporal table: system time + valid time axes, valid-time overlap detection, full-table bi-temporal scan (`bi_temporal.cpp`)
- [x] Time-travel query engine: `AS OF`, `FROM...TO`, `BETWEEN...AND` queries with row filters (`temporal_query_engine.cpp`)
- [x] Period-based temporal index: B-tree on `(sys_start, sys_end)` for efficient range lookups (`temporal_index.cpp`)
- [x] Temporal aggregations: tumbling, sliding, and session window aggregation; GROUP BY partitioned aggregation; snapshot-state aggregation at regular intervals; linear trend analysis (slope, intercept, r²) over windowed aggregates (`temporal_aggregator.cpp`)
- [x] Point-in-time snapshot creation, querying, and release (`snapshot_manager.cpp`)
- [x] Snapshot versioning: monotonically increasing `version_number` in `SnapshotHandle`
- [x] Snapshot garbage collection: TTL-based (`garbageCollect(max_age_ms)`) and max-count-based (`garbageCollect(max_snapshots)`) GC
- [x] Snapshot metadata: `SnapshotMetadata` struct with `getSnapshotMetadata()` API
- [x] Retention manager: time-based and count-based policy enforcement with background cleanup (`retention_manager.cpp`)
- [x] Conflict resolution audit trail: all resolutions logged for auditability
- [x] Thread-safe version writes (per-record lock) with lock-free reads via MVCC
- [x] Unit tests for conflict resolver, query engine, temporal index, aggregator, and bi-temporal table (`tests/temporal/`)
- [x] Bitemporal joins: combined transaction-time + valid-time join predicates (`TemporalQueryEngine::joinBiTemporal`)
- [x] SEQUENCED vs. NON-SEQUENCED temporal query semantics per SQL:2011 §4.16.5 (`TemporalQueryEngine::queryWithSemantics`)
- [x] **Time-Travel Query Engine (v1.2.0):** `queryBetween` (FOR SYSTEM_TIME BETWEEN…AND), `queryApplicationTime` / `queryApplicationTimeRange` (FOR APPLICATION_TIME), `queryAsOfWithIndex` (index-accelerated version pruning), `QueryCache` + `detail::queryAsOfCached` (result caching for frequently accessed historical data) (`temporal_query_engine.cpp`)
- [x] **SQL:2011 Temporal Query Dispatcher (v1.9.0):** `TemporalClause` enum (AS_OF, FROM_TO, BETWEEN_AND, CONTAINED_IN, ALL), `TemporalQuerySpec` struct with factory methods (`asOf`, `fromTo`, `betweenAnd`, `containedIn`, `all`), `TemporalQueryEngine::executeTemporalQuery()` overloads for both `SystemVersionedTable` and `BiTemporalTable`; `include_deleted` flag to suppress logically-deleted rows (`temporal_query_engine.cpp`)

## In Progress 🚧
*(no items currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] SQL `PERIOD FOR` DDL syntax for system-time and application-time declarations (Target: Q3 2026)
- [I] `FOR SYSTEM_TIME` / `FOR APPLICATION_TIME` clause in AQL query parser (Target: Q3 2026)
- [x] Temporal uniqueness constraints and gap/overlap detection for valid-time periods (`BiTemporalTable::hasUniquenessConflict`, `BiTemporalTable::findGaps`, `BiTemporalTable::findOverlaps`)
- [I] Storage-based and archive-to-cold-storage retention policy variants (Target: Q3 2026)
- [x] Delta compression for historical versions (DELTA, ZSTD, Gorilla, DICTIONARY algorithms via `TemporalCompressor`)
  - `[x]` LZ4 compression added to `TemporalCompressor` (Issue: #4575) (2026-04-12): `CompressionAlgorithm::LZ4`, `applyLz4()`/`decompressLz4()` via `<lz4.h>`
  - JSON payload: `{"__compressed":"lz4","__original_size":N,"__data":"<base64>"}` wired into `algorithmName()`, `decompress()`, `compressHistory()`
  - 5 focused tests (TC-LZ4-01…TC-LZ4-05) appended to `tests/temporal/test_temporal_compressor.cpp`

### Long-term (6-12 months)
- [x] Temporal foreign keys with period-aware referential integrity (`TemporalForeignKey::validate()`)
- [I] Temporal CDC: version-aware change event streaming with before/after diff (Target: Q1 2027)
- [x] Temporal migration tooling: convert existing tables to system-versioned with history backfill (`temporal_migrator.h/cpp`)
- [I] Interval-tree index for efficient overlapping-period detection (Target: Q1 2027)

## Implementation Phases

### Phase 1: Core Temporal Infrastructure (Status: Completed ✅)
- [x] HLC timestamp generation and distributed causal ordering (`storage/hlc.cpp` dependency)
- [x] `TemporalConflictResolver` with five resolution policies and conflict audit log
- [x] `TemporalConflictDetector`: four ConflictType categories, `detectConflicts`, `autoResolveConflict`, `queueForManualResolution`, `getQueuedConflicts`, `clearQueue`
- [x] `TemporalSnapshot` serialization/deserialization (JSON round-trip)
- [x] Unit tests for all conflict resolution policies and all detector conflict types (`tests/temporal/test_temporal_conflict_resolver.cpp`)

### Phase 2: Full Temporal Data Model (Status: Completed ✅)
- [x] `SystemVersionedTable`: insert, update (close old / open new version), delete, `scan(as_of)`, `getHistoryInRange()`, `getAllKeys()` (`system_versioned_table.cpp`)
- [x] `SystemVersionedTable::Config`: `history_table_name`, `compress_history`, `retention_period`, `track_user_id`; constructors + `createVersionedTable` DDL factory; `upsert`; `enforceRetentionPolicy`; `getConfig`; `getStatistics` extended with config fields (`system_versioned_table.cpp`, v1.1.0)
- [x] `BiTemporalTable`: valid-time period management, overlap rejection on insert, bitemporal scan (`bi_temporal.cpp`); new: `scanBiTemporal(sys_as_of, valid_at)`, `getAllKeys()`
- [x] **Application-Versioned Tables (Bi-Temporal, v1.2.0):** `findGaps(key, from, to)` for gap detection, `hasUniquenessConflict(key, period)` for temporal uniqueness constraint checks, `TemporalForeignKey::validate()` for period-aware referential integrity (`bi_temporal.cpp`)
- [x] `TemporalIndex`: period B-tree index with `insert`, `queryAsOf`, `queryRange`, `stats` (`temporal_index.cpp`)
- [x] `TemporalQueryEngine`: `queryAsOf`, `queryFromTo`, `queryBetween` with composable row filters (`temporal_query_engine.cpp`)
- [x] `TemporalSnapshotManager`: consistent multi-table snapshots, query-by-snapshot, LRU release (`snapshot_manager.cpp`); snapshot versioning (`version_number`), TTL-based and max-count GC, `SnapshotMetadata`
- [x] `RetentionManager`: per-table policies, `enforceRetention()`, `RetentionStats` reporting (`retention_manager.cpp`)
- [x] `TemporalAggregator`: tumbling/sliding/session window aggregation over version history; `aggregateByGroup()` for temporal GROUP BY; `aggregateSnapshots()` for state-based snapshot aggregation at regular intervals; `analyzeTrend()` linear regression over windowed aggregates; `AggregateResult` with `group_values` output; `TrendResult` with slope/intercept/r² (`temporal_aggregator.cpp`)
- [x] Unit tests: aggregator, index, query engine, bi-temporal (`tests/temporal/`)

### Phase 3: SQL Syntax & Temporal Constraints (Status: C++ layer complete ✅; SQL syntax deferred to Phase 3b 📋)
- [x] Bitemporal join operator (combined transaction-time + valid-time predicates) — `TemporalQueryEngine::joinBiTemporal()`
- [x] SEQUENCED / NON-SEQUENCED query semantics — `TemporalQueryEngine::queryWithSemantics()` + `TemporalSemantics` enum
- [x] Temporal uniqueness constraints — `BiTemporalTable::hasUniquenessConflict(key, period)` (C++ layer; SQL syntax deferred → Phase 3b)
- [x] Gap detection in valid-time coverage — `BiTemporalTable::findGaps(key, from, to)` (C++ layer; SQL syntax deferred → Phase 3b)
- [x] Temporal foreign keys — `TemporalForeignKey::validate(parent, key, period)` (C++ layer; CASCADE/RESTRICT at SQL layer deferred → Phase 4)
- [x] `FOR SYSTEM_TIME BETWEEN…AND` — `TemporalQueryEngine::queryBetween()` (closed-interval variant of `queryFromTo`)
- [x] `FOR APPLICATION_TIME AS OF` — `TemporalQueryEngine::queryApplicationTime()` (valid-time point query over `BiTemporalTable`)
- [x] `FOR APPLICATION_TIME FROM…TO` — `TemporalQueryEngine::queryApplicationTimeRange()` (valid-time range query over `BiTemporalTable`)
- [x] Index-accelerated AS-OF query — `TemporalQueryEngine::queryAsOfWithIndex()` (version pruning via `TemporalIndex`)
- [x] Result caching — `QueryCache` + `detail::queryAsOfCached()` (LRU cache for frequently accessed historical snapshots)
- [I] `PERIOD FOR SYSTEM_TIME` / `PERIOD FOR APPLICATION_TIME` DDL in AQL parser (deferred → Phase 3b)
- [I] `FOR SYSTEM_TIME AS OF` / `FOR APPLICATION_TIME` temporal clause parsing (deferred → Phase 3b)

### Phase 4: Advanced Retention, Compression & CDC (Status: Partial ⚙️)
- [I] Archive-to-cold-storage retention variant (`s3://` or filesystem archive before purge)
- [I] Storage-based retention (cap history to N GB per table)
- [x] Delta and Gorilla compression for historical versions
- [x] Temporal CDC: `ChangeEvent` stream (INSERT / UPDATE / DELETE / VERSION_CREATED) with Kafka integration
- [I] Temporal foreign keys CASCADE/RESTRICT at SQL layer
- [x] Interval-tree index for `O(log n + k)` overlap detection

### Phase 5: Tooling & Migration (Status: Partial ⚙️)
- [x] `TemporalMigrator`: analyze, migrate, and verify existing tables to system-versioned (`temporal_migrator.h/cpp`)
- [x] History backfill from audit log during migration (`TemporalMigrator::backfillHistory`)
- [I] Integration with `src/scheduler/` for fully automated retention enforcement cycles
- [I] Temporal query metrics exposed via `src/observability/`

## Production Readiness Checklist
- [x] Unit tests for core components (conflict resolver, query engine, index, aggregator, bi-temporal)
- [x] Integration tests present (time-travel queries, temporal aggregation, temporal graph, geo-temporal)
- [~] Performance benchmarks (time-travel query latency, retention throughput, index speedup)
- [I] Security audit (temporal history immutability, RBAC/RLS on time-travel queries)
- [~] Documentation complete (ARCHITECTURE.md and README.md present; API docs pending)
- [~] API stability guaranteed (C++ API is stable at v1.0; SQL syntax layer not yet stable)

## Known Issues & Limitations
- SQL `PERIOD FOR` DDL syntax is not yet supported; application-time periods must be managed via the C++ API.
- No automatic SQL-level retention syntax (`ALTER TABLE … SET RETENTION_PERIOD`); retention policies must be set programmatically via `RetentionManager::setPolicy()`.
- History table compression is implemented via TemporalCompressor (DELTA, ZSTD, Gorilla, DICTIONARY, LZ4 algorithms). LZ4 added 2026-04-12 via `<lz4.h>`.
- Temporal CDC is available via TemporalCDC (in-process pub/sub with bounded ring-buffer; external Kafka integration deferred to Phase 5).

## Breaking Changes
- The `TemporalConflictResolver` and `SystemVersionedTable` C++ APIs are stable at v1.0 and will not change without a major version bump.
- Future SQL-layer additions (`PERIOD FOR`, `FOR SYSTEM_TIME`) will be additive and backward-compatible with existing API-level usage.
