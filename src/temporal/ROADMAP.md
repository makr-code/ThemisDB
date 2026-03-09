# Temporal Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — Transaction-time tracking, time-travel queries (`AS OF`, `FROM...TO`, `BETWEEN...AND`), HLC-based conflict resolution, bitemporal joins, and SEQUENCED/NON-SEQUENCED query semantics are fully operational. SQL `PERIOD FOR` DDL syntax is not yet supported.

## Completed ✅
- [x] HLC-based temporal conflict resolver with multiple policies (last-write-wins, first-write-wins, node-priority, manual, CRDT-merge) (`temporal_conflict_resolver.cpp`)
- [x] System-versioned table: automatic transaction-time versioning, non-destructive updates (`system_versioned_table.cpp`)
- [x] Bi-temporal table: system time + valid time axes, valid-time overlap detection, full-table bi-temporal scan (`bi_temporal.cpp`)
- [x] Time-travel query engine: `AS OF`, `FROM...TO`, `BETWEEN...AND` queries with row filters (`temporal_query_engine.cpp`)
- [x] Period-based temporal index: B-tree on `(sys_start, sys_end)` for efficient range lookups (`temporal_index.cpp`)
- [x] Temporal aggregations: tumbling and sliding window aggregation over version history (`temporal_aggregator.cpp`)
- [x] Point-in-time snapshot creation, querying, and release (`snapshot_manager.cpp`)
- [x] Retention manager: time-based and count-based policy enforcement with background cleanup (`retention_manager.cpp`)
- [x] Conflict resolution audit trail: all resolutions logged for auditability
- [x] Thread-safe version writes (per-record lock) with lock-free reads via MVCC
- [x] Unit tests for conflict resolver, query engine, temporal index, aggregator, and bi-temporal table (`tests/temporal/`)
- [x] Bitemporal joins: combined transaction-time + valid-time join predicates (`TemporalQueryEngine::joinBiTemporal`)
- [x] SEQUENCED vs. NON-SEQUENCED temporal query semantics per SQL:2011 §4.16.5 (`TemporalQueryEngine::queryWithSemantics`)

## In Progress 🚧
*(no items currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] SQL `PERIOD FOR` DDL syntax for system-time and application-time declarations (Target: Q3 2026)
- [I] `FOR SYSTEM_TIME` / `FOR APPLICATION_TIME` clause in AQL query parser (Target: Q3 2026)
- [I] Temporal uniqueness constraints and gap/overlap detection for valid-time periods (Target: Q3 2026)
- [I] Storage-based and archive-to-cold-storage retention policy variants (Target: Q3 2026)
- [I] Delta compression for historical versions (ZSTD / Gorilla for numeric time-series data) (Target: Q4 2026)

### Long-term (6-12 months)
- [I] Temporal foreign keys with period-aware referential integrity (`CASCADE`, `RESTRICT`) (Target: Q4 2026)
- [I] Temporal CDC: version-aware change event streaming with before/after diff (Target: Q1 2027)
- [I] Temporal migration tooling: convert existing tables to system-versioned with history backfill (Target: Q1 2027)
- [I] Interval-tree index for efficient overlapping-period detection (Target: Q1 2027)

## Implementation Phases

### Phase 1: Core Temporal Infrastructure (Status: Completed ✅)
- [x] HLC timestamp generation and distributed causal ordering (`storage/hlc.cpp` dependency)
- [x] `TemporalConflictResolver` with five resolution policies and conflict audit log
- [x] `TemporalSnapshot` serialization/deserialization (JSON round-trip)
- [x] Unit tests for all conflict resolution policies (`tests/temporal/test_temporal_conflict_resolver.cpp`)

### Phase 2: Full Temporal Data Model (Status: Completed ✅)
- [x] `SystemVersionedTable`: insert, update (close old / open new version), delete, `scan(as_of)`, `getHistoryInRange()`, `getAllKeys()` (`system_versioned_table.cpp`)
- [x] `BiTemporalTable`: valid-time period management, overlap rejection on insert, bitemporal scan (`bi_temporal.cpp`); new: `scanBiTemporal(sys_as_of, valid_at)`, `getAllKeys()`
- [x] `TemporalIndex`: period B-tree index with `insert`, `queryAsOf`, `queryRange`, `stats` (`temporal_index.cpp`)
- [x] `TemporalQueryEngine`: `queryAsOf`, `queryFromTo`, `queryBetween` with composable row filters (`temporal_query_engine.cpp`)
- [x] `TemporalSnapshotManager`: consistent multi-table snapshots, query-by-snapshot, LRU release (`snapshot_manager.cpp`)
- [x] `RetentionManager`: per-table policies, `enforceRetention()`, `RetentionStats` reporting (`retention_manager.cpp`)
- [x] `TemporalAggregator`: tumbling/sliding window aggregation over version history, `AggregateResult` output (`temporal_aggregator.cpp`)
- [x] Unit tests: aggregator, index, query engine, bi-temporal (`tests/temporal/`)

### Phase 3: SQL Syntax & Temporal Constraints (Status: C++ layer complete ✅; SQL syntax deferred to Phase 3b 📋)
- [x] Bitemporal join operator (combined transaction-time + valid-time predicates) — `TemporalQueryEngine::joinBiTemporal()`
- [x] SEQUENCED / NON-SEQUENCED query semantics — `TemporalQueryEngine::queryWithSemantics()` + `TemporalSemantics` enum
- [I] `PERIOD FOR SYSTEM_TIME` / `PERIOD FOR APPLICATION_TIME` DDL in AQL parser (deferred → Phase 3b)
- [I] `FOR SYSTEM_TIME AS OF` / `FOR APPLICATION_TIME` temporal clause parsing (deferred → Phase 3b)
- [I] Temporal uniqueness constraints (no valid-time overlaps per key) (deferred → Phase 3b)

### Phase 4: Advanced Retention, Compression & CDC (Status: Planned 📋)
- [I] Archive-to-cold-storage retention variant (`s3://` or filesystem archive before purge)
- [I] Storage-based retention (cap history to N GB per table)
- [I] Delta and Gorilla compression for historical versions
- [I] Temporal CDC: `ChangeEvent` stream (INSERT / UPDATE / DELETE / VERSION_CREATED) with Kafka integration
- [I] Temporal foreign keys with period-aware cascade/restrict actions
- [I] Interval-tree index for `O(log n + k)` overlap detection

### Phase 5: Tooling & Migration (Status: Planned 📋)
- [I] `TemporalMigrator`: analyze, migrate, and verify existing tables to system-versioned
- [I] History backfill from audit log during migration
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
- History table compression is not yet implemented; historical data storage overhead is proportional to version count.
- Temporal CDC (streaming change events) is not yet available.

## Breaking Changes
- The `TemporalConflictResolver` and `SystemVersionedTable` C++ APIs are stable at v1.0 and will not change without a major version bump.
- Future SQL-layer additions (`PERIOD FOR`, `FOR SYSTEM_TIME`) will be additive and backward-compatible with existing API-level usage.
