# Metadata Module Roadmap

**Module:** `src/metadata`  
**Maintained by:** ThemisDB Contributors  
**Last Updated:** February 2026  
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
- [ ] Wire `StatisticsCollector` into `QueryEngine` for cost-based optimiser (cardinality estimates)
- [ ] Prometheus / OpenTelemetry metrics: `themis_stats_collection_duration_ms`, `themis_stats_cache_hits_total`, `themis_stats_errors_total`
- [ ] Configurable automatic refresh schedule (default: hourly)

### Schema Constraints Enforcement (v1.6.0)

- [x] `include/metadata/schema_constraints.h` — `NOT NULL`, `UNIQUE`, `CHECK`, `DEFAULT`, `FOREIGN_KEY` constraint types
- [x] `src/metadata/schema_constraints.cpp` — in-memory enforcement engine with `applyDefaults()` and `enforce()` APIs
- [x] `tests/test_schema_constraints.cpp` — unit tests for each constraint type and violation reporting
- [x] `SchemaConstraints::persistTo(db)` / `loadFrom(db)` — durable storage under `config:constraints:` prefix
- [x] `tests/test_schema_constraints_persistence.cpp` — round-trip persistence tests
- [x] REST endpoint: `GET /api/v1/metadata/constraints/:table`
- [ ] Integrate `SchemaConstraints` into the write path of `RocksDBWrapper` for transparent enforcement
- [ ] Expose constraint violations as structured HTTP 422 error codes
- [ ] Prometheus metric: `themis_constraint_violations_total{type, table}`

### Observability

- [ ] Cache hit-rate metric: `themis_schema_cache_hit_ratio`
- [ ] Discovery / refresh latency histogram: `themis_schema_discovery_duration_ms`
- [ ] Structured log fields: `table`, `duration_ms`, `rows_sampled`, `error_code` on every stats collection

---

## Q2 2026 — INFORMATION_SCHEMA & Schema Versioning

**Theme:** SQL-standard metadata access and safe schema evolution.

### INFORMATION_SCHEMA (v1.7.0)

- [x] `include/metadata/information_schema.h` — `ISTable`, `ISColumn`, `ISStatistic`, `ISKeyColumnUsage` row types
- [x] `src/metadata/information_schema.cpp` — derived from `SchemaManager` on demand; no separate persistence
- [x] `tests/test_information_schema.cpp` — unit tests for all four views and JSON serialisation
- [x] REST endpoints: `GET /api/v1/information_schema`, `/tables`, `/columns[/:table]`, `/statistics[/:table]`
- [ ] AQL functions: `INFORMATION_SCHEMA_TABLES()`, `INFORMATION_SCHEMA_COLUMNS(tableName)`, etc.
- [ ] `INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS` view for foreign-key metadata

### Schema Versioning / Migration / Rollback (v1.8.0)

- [x] `include/metadata/schema_version_manager.h` — `createSchemaVersion()`, `getCurrentVersion()`, `getChangeHistory()`, `rollbackToVersion()`, `diffVersions()`
- [x] `src/metadata/schema_version_manager.cpp` — persistence under `config:schema_version:` prefix; full diff engine
- [x] `tests/test_schema_version_manager.cpp` — version creation, rollback, diff, and JSON tests
- [x] REST endpoints: `GET /api/v1/schema/versions/:table`, `POST /api/v1/schema/versions/:table`, `GET /api/v1/schema/diff/:table?from=V&to=V`
- [ ] CLI command: `themisdb schema diff <table> <v1> <v2>`
- [ ] Dry-run mode for migrations: validate without applying changes
- [ ] Runbook: `docs/metadata/schema_migration_runbook.md`

### Storage Integration for Constraints

- [ ] Cross-row `UNIQUE` enforcement via secondary index lookup on write
- [ ] Cross-table `FOREIGN KEY` enforcement with configurable `ON DELETE / ON UPDATE` actions
- [ ] Batch constraint validation API for bulk imports

---

## Q3 2026 — Auto Index Recommendations & Big-DB / Chaos Testing

**Theme:** Intelligent indexing advice and test-suite hardening.

### Auto Index Recommendations (v1.9.0)

- [x] `include/metadata/index_recommender.h` — `IndexRecommender` with `recordAccess()`, `recommend()`, `recommendAll()`, benefit-score model
- [x] `src/metadata/index_recommender.cpp` — thread-safe implementation; ADD/DROP recommendations sorted by benefit score
- [x] `tests/test_index_recommender.cpp` — unit tests for access recording, scoring, recommendations, and JSON export
- [ ] REST endpoint: `GET /api/v1/metadata/index_recommendations[/:table]`
- [ ] Wire `IndexRecommender::recordAccess()` into the AQL query execution path
- [ ] CLI: `themisdb index recommend <table>`

### CDC / WAL Integration for Hot Reload

- [ ] Subscribe to CDC events for structural changes (CREATE / DROP TABLE)
- [ ] Live cache invalidation without full restart
- [ ] Incremental statistics refresh on high-write-rate tables

### Test Coverage Expansion

- [ ] Big-DB benchmark: schema discovery and stats collection for 10 000-table / 1 000-column schemas
- [ ] Fuzz tests: random JSON schema inputs into `SchemaManager::parseTableSchema()`
- [ ] Chaos tests: stats collection under concurrent writes and RocksDB compaction
- [ ] Migration regression tests: forward / rollback across 5 schema versions

---

## Q4 2026 — Admin / Recovery Tooling & Operator DX

**Theme:** Auditability, observability dashboards, and operator runbooks.

### Admin & Recovery APIs

- [ ] Audit log for schema changes: who changed what and when (stored under `audit:schema:`)
- [ ] Schema import: `PUT /api/v1/metadata/schema_import` (bulk JSON)
- [ ] Background consistency checker: periodic scan for orphan keys, missing constraints, stale stats
- [ ] Recovery runbook: `docs/metadata/recovery_runbook.md`

### Observability Dashboards & Alerts

- [ ] Grafana dashboard: schema discovery rate, stats freshness, cache hit-rate, constraint violations
- [ ] Alerting rules: `schema_stale_stats_age > 24h`, `constraint_violations_per_min > threshold`
- [ ] Heatmap: per-table write frequency correlated with stats freshness

### Operator Documentation

- [ ] `docs/metadata/operations_guide.md` — tuning cache TTL, sample sizes, refresh schedules
- [ ] `docs/metadata/troubleshooting.md` — common error codes, recovery steps
- [ ] API reference auto-generated from OpenAPI spec

---

## Version Summary

| Version | Target | Key Deliverable |
|---------|--------|-----------------|
| v1.5.0  | ✅ Done | SchemaManager, cache, JSON export |
| v1.6.0  | ✅ Done | StatisticsCollector, SchemaConstraints + persistence, REST endpoints |
| v1.7.0  | ✅ Done | INFORMATION_SCHEMA views + REST endpoints |
| v1.8.0  | ✅ Done | Schema Versioning, Migration, diff, rollback + REST endpoints |
| v1.9.0  | ✅ Done | IndexRecommender (auto index recommendations) |
| v2.0.0  | Q4 2026 | Admin/Recovery tooling, full observability, CDC integration |

---

## See Also

- [`src/metadata/FUTURE_ENHANCEMENTS.md`](../../src/metadata/FUTURE_ENHANCEMENTS.md)
- [`include/metadata/statistics_collector.h`](../../include/metadata/statistics_collector.h)
- [`include/metadata/information_schema.h`](../../include/metadata/information_schema.h)
- [`include/metadata/schema_constraints.h`](../../include/metadata/schema_constraints.h)
- [`include/metadata/schema_version_manager.h`](../../include/metadata/schema_version_manager.h)
- [`include/metadata/index_recommender.h`](../../include/metadata/index_recommender.h)
- [Architecture Guide](../../ARCHITECTURE.md)

