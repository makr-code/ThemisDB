# Metadata Module Roadmap

**Module:** `src/metadata`  
**Maintained by:** ThemisDB Contributors  
**Last Updated:** April 2026  
**Current Version:** v1.6.0

---

## Overview

This document tracks the planned evolution of ThemisDB's metadata module from its current v1.6.0 baseline towards a production-ready, fully observable, constraint-enforcing, and SQL-standard-compliant state.

For the gap analysis that generated this roadmap see issue [#1353](https://github.com/makr-code/ThemisDB/issues/1353).

---

## Q1 2026 — Core Production Readiness

**Theme:** Wire the Statistics Collector, enforce schema constraints, and add structured observability.

### Statistics Collector (v1.6.0)

- [x] `include/metadata/statistics_collector.h` — public API (cardinality, histograms, selectivity, null-fraction)
- [x] `src/metadata/statistics_collector.cpp` — implementation with sampled key scans, equi-height histograms, RocksDB persistence under `stats:` prefix
- [x] `tests/test_statistics_collector.cpp` — unit tests covering collection, caching, persistence, and error paths
- [x] REST endpoint: `GET /api/v1/metadata/stats/:table` and `POST /api/v1/metadata/stats/:table`
- [x] Wire `StatisticsCollector` into `QueryEngine` for cost-based optimiser – `setStatisticsCollector()` injects the collector; equality predicates are sorted by ascending selectivity inside `executeAndKeysRangeAware_()`
- [x] Prometheus / OpenTelemetry metrics hook interface `IMetricsHook` with `onCollect`, `onCacheHit`, `onCacheMiss`, `onError` – call `setMetricsHook()` to plug in Prometheus / OTel sinks
- [x] Configurable automatic refresh schedule – `setRefreshInterval(std::chrono::seconds)` starts a background thread that re-collects stats for all known tables at the given interval; `stopRefresh()` shuts it down cleanly

### Schema Constraints Enforcement (v1.6.0)

- [x] `include/metadata/schema_constraints.h` — `NOT NULL`, `UNIQUE`, `CHECK`, `DEFAULT`, `FOREIGN_KEY` constraint types
- [x] `src/metadata/schema_constraints.cpp` — in-memory enforcement engine with `applyDefaults()` and `enforce()` APIs
- [x] `tests/test_schema_constraints.cpp` — unit tests for each constraint type and violation reporting
- [x] `SchemaConstraints::persistTo(db)` / `loadFrom(db)` — durable storage under `config:constraints:` prefix
- [x] `tests/test_schema_constraints_persistence.cpp` — round-trip persistence tests
- [x] REST endpoint: `GET /api/v1/metadata/constraints/:table`
- [x] Batch constraint validation: `POST /api/v1/metadata/constraints/validate/:table` – validates a JSON array of rows and returns HTTP 422 with structured violations
- [ ] Integrate `SchemaConstraints` into the write path of `RocksDBWrapper` for transparent enforcement
- [ ] Prometheus metric: `themis_constraint_violations_total{type, table}` (plug in via `IMetricsHook`)

### Observability

- [x] Structured log fields on every stats collection: `table`, `duration_ms`, `rows_sampled`, `total_rows`, `cols`, `error_code` (spdlog INFO)
- [x] Metrics hook interface in `StatisticsCollector` — cache-hit, cache-miss, collect-duration, error hooks
- [ ] Cache hit-rate metric exported to Prometheus / OTel (requires hook implementation)
- [ ] Discovery / refresh latency histogram exported (requires hook implementation)

---

## Q2 2026 — INFORMATION_SCHEMA & Schema Versioning

**Theme:** SQL-standard metadata access and safe schema evolution.

### INFORMATION_SCHEMA (v1.7.0)

- [x] `include/metadata/information_schema.h` — `ISTable`, `ISColumn`, `ISStatistic`, `ISKeyColumnUsage` row types
- [x] `src/metadata/information_schema.cpp` — derived from `SchemaManager` on demand; no separate persistence
- [x] `tests/test_information_schema.cpp` — unit tests for all four views and JSON serialisation
- [x] REST endpoints: `GET /api/v1/information_schema`, `/tables`, `/columns[/:table]`, `/statistics[/:table]`
- [x] `INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS` view — `ISReferentialConstraint` struct + `getReferentialConstraints()` + `referentialConstraintsToJSON()`
- [ ] AQL functions: `INFORMATION_SCHEMA_TABLES()`, `INFORMATION_SCHEMA_COLUMNS(tableName)`, etc.

### Schema Versioning / Migration / Rollback (v1.8.0)

- [x] `include/metadata/schema_version_manager.h` — `createSchemaVersion()`, `getCurrentVersion()`, `getChangeHistory()`, `rollbackToVersion()`, `diffVersions()`
- [x] `src/metadata/schema_version_manager.cpp` — persistence under `config:schema_version:` prefix; full diff engine
- [x] `tests/test_schema_version_manager.cpp` — version creation, rollback, diff, and JSON tests
- [x] REST endpoints: `GET /api/v1/schema/versions/:table`, `POST /api/v1/schema/versions/:table`, `GET /api/v1/schema/diff/:table?from=V&to=V`
- [x] Runbook: `docs/metadata/schema_migration_runbook.md`
- [ ] CLI command: `themisdb schema diff <table> <v1> <v2>`
- [x] Dry-run mode for migrations: `SchemaVersionManager::validateMigration()` validates name, columns, uniqueness, and idempotency without persisting

### Storage Integration for Constraints

- [ ] Cross-row `UNIQUE` enforcement via secondary index lookup on write
- [ ] Cross-table `FOREIGN KEY` enforcement with configurable `ON DELETE / ON UPDATE` actions
- [x] Batch constraint validation API: `POST /api/v1/metadata/constraints/validate/:table` — validates JSON array of rows, returns HTTP 422 + structured violations

---

## Q3 2026 — Auto Index Recommendations & Big-DB / Chaos Testing

**Theme:** Intelligent indexing advice and test-suite hardening.

### Column Lineage & Data Provenance

- [x] `include/metadata/column_lineage.h` — `ColumnLineageTracker`, `ColumnRef`, `ColumnLineageEntry`, `ColumnLineageRecord`, `TransformationType`
- [x] `src/metadata/column_lineage.cpp` — thread-safe append-only BFS DAG implementation; dual-index store (`target→entries`, `source→targets`); auto-assigns `entry_id` and `timestamp_ms`
- [x] `tests/test_column_lineage.cpp` — 34 unit tests: `recordDerivation`, `getColumnLineage`, `getUpstreamColumns`, `getDownstreamColumns`, `getColumnProvenance`, `exportTableLineage`, `exportAllLineage`, diamond-DAG deduplication, JSON round-trips
- [x] `cmake/CMakeLists.txt` — `column_lineage.cpp` added to `themisdb` build target
- [x] `docs/metadata/operations_guide.md` — usage section with code examples and `TransformationType` reference table
- [x] `include/metadata/README.md` — `column_lineage.h` API reference section
- [x] `src/metadata/README.md` — `ColumnLineageTracker` listed in components and architecture diagram
- [x] REST endpoints wired into `SchemaApiHandler` and `HttpServer`:
  - `GET  /api/v1/metadata/lineage/:table` — export all lineage for a table
  - `GET  /api/v1/metadata/lineage/:table/:column` — provenance for one column
  - `POST /api/v1/metadata/lineage` — record a derivation entry
- [ ] Persist lineage graph to RocksDB under `lineage:col:` prefix for durability across restarts (follow-up)

### Auto Index Recommendations (v1.9.0)

- [x] `include/metadata/index_recommender.h` — `IndexRecommender` with `recordAccess()`, `recommend()`, `recommendAll()`, benefit-score model
- [x] `src/metadata/index_recommender.cpp` — thread-safe implementation; ADD/DROP recommendations sorted by benefit score
- [x] `tests/test_index_recommender.cpp` — unit tests for access recording, scoring, recommendations, and JSON export
- [x] REST endpoint: `GET /api/v1/metadata/index_recommendations[/:table]`
- [x] Wire `IndexRecommender::recordAccess()` into the AQL query execution path – `QueryApiHandler::setIndexRecommender()` injects the recommender; filter/sort predicates are recorded after every successful AQL translation
- [x] CLI: `themisctl index recommend [table]` — added to `tools/themisctl.cpp`; calls `GET /api/v1/metadata/index_recommendations[/:table]`; 8 unit tests in `tests/test_themisctl.cpp`

### CDC / WAL Integration for Hot Reload

- [ ] Subscribe to CDC events for structural changes (CREATE / DROP TABLE)
- [ ] Live cache invalidation without full restart
- [ ] Incremental statistics refresh on high-write-rate tables

### Test Coverage Expansion

- [ ] Big-DB benchmark: schema discovery and stats collection for 10 000-table / 1 000-column schemas
- [x] Fuzz tests: random/malformed JSON inputs into `SchemaManager::parseTableSchema()` — `tests/test_schema_manager_fuzz.cpp` (40+ edge-case tests)
- [ ] Chaos tests: stats collection under concurrent writes and RocksDB compaction
- [x] Migration regression tests: forward / rollback across 5 schema versions — `tests/test_schema_migration_regression.cpp`

---

## Q4 2026 — Admin / Recovery Tooling & Operator DX

**Theme:** Auditability, observability dashboards, and operator runbooks.

### Admin & Recovery APIs

- [x] Audit log for schema changes: `SchemaAuditLog` stored under `audit:schema:` prefix; wired into `SchemaVersionManager`
- [x] `GET /api/v1/metadata/audit[/:table]` — per-table or full audit history endpoint
- [x] Schema import: `PUT /api/v1/metadata/schema_import` (bulk JSON — imports multiple table schemas in one request)
- [x] Background consistency checker: `SchemaConsistencyChecker` – checks orphan keys, stale stats, missing constraints; periodic background thread via `startBackgroundCheck(interval)`; wired into HttpServer (6-hour interval)
- [x] Recovery runbook: `docs/metadata/recovery_runbook.md`

### Observability Dashboards & Alerts

- [ ] Grafana dashboard: schema discovery rate, stats freshness, cache hit-rate, constraint violations
- [ ] Alerting rules: `schema_stale_stats_age > 24h`, `constraint_violations_per_min > threshold`
- [ ] Heatmap: per-table write frequency correlated with stats freshness

### Operator Documentation

- [x] `docs/metadata/operations_guide.md` — tuning cache TTL, sample sizes, refresh schedules, metrics hooks
- [x] `docs/metadata/troubleshooting.md` — common error codes, recovery steps
- [x] `docs/metadata/recovery_runbook.md` — 6-scenario recovery guide
- [ ] API reference auto-generated from OpenAPI spec

---

## Version Summary

| Version | Target | Key Deliverable |
|---------|--------|-----------------|
| v1.5.0  | ✅ Done | SchemaManager, cache, JSON export |
| v1.6.0  | ✅ Done | StatisticsCollector (+ OTel hook + auto-refresh + QueryEngine wiring), SchemaConstraints + persistence + batch validate, REST endpoints |
| v1.7.0  | ✅ Done | INFORMATION_SCHEMA + REFERENTIAL_CONSTRAINTS, REST endpoints |
| v1.8.0  | ✅ Done | Schema Versioning, diff, rollback + dry-run validate + REST + runbook + audit log |
| v1.9.0  | ✅ Done | IndexRecommender (wired into AQL path), SchemaConsistencyChecker, schema import, operator docs |
| v2.0.0  | Q4 2026 | CDC/hot-reload, Grafana dashboards, AQL functions, CLI, cross-row UNIQUE/FK enforcement |

---

## See Also

- [`src/metadata/FUTURE_ENHANCEMENTS.md`](../../src/metadata/FUTURE_ENHANCEMENTS.md)
- [`include/metadata/statistics_collector.h`](../../include/metadata/statistics_collector.h)
- [`include/metadata/information_schema.h`](../../include/metadata/information_schema.h)
- [`include/metadata/schema_constraints.h`](../../include/metadata/schema_constraints.h)
- [`include/metadata/schema_version_manager.h`](../../include/metadata/schema_version_manager.h)
- [`include/metadata/index_recommender.h`](../../include/metadata/index_recommender.h)
- [`include/metadata/schema_consistency_checker.h`](../../include/metadata/schema_consistency_checker.h)
- [Architecture Guide](../../ARCHITECTURE.md)

