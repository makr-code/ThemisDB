> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Metadata Module

All notable changes to the Metadata module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Auto-generated OpenAPI schema export (Target: v2.0.0 / Q3 2027)
- Explicit compatibility-mode policy for schema migrations (Target: v1.9.0 / Q1 2027)
- Apache Atlas and DataHub integration stability improvements

## [1.6.0] — 2026-03-22
### Added
- `IMetadataSecurityProvider`: pluggable RBAC / access-control interface for all metadata
  operations (READ_SCHEMA, WRITE_SCHEMA, READ_STATISTICS, WRITE_STATISTICS, READ_LINEAGE,
  WRITE_LINEAGE, READ_AUDIT_LOG, ADMIN). Shipped with two implementations:
  - `NoOpMetadataSecurityProvider` — permits everything (default; zero overhead).
  - `InMemoryRbacMetadataSecurityProvider` — thread-safe in-memory RBAC with grant/revoke/
    revokeAll; wildcard resource `"*"` and ADMIN-implies-all semantics. Throws
    `MetadataAccessDeniedException` on denial.
- `IMetadataChangeListener`: observer interface for schema and metadata change events
  (`TABLE_CREATED`, `TABLE_MODIFIED`, `TABLE_DROPPED`, `CONSTRAINT_ADDED`,
  `CONSTRAINT_DROPPED`, `STATISTICS_UPDATED`). Shipped with
  `RecordingMetadataChangeListener` — thread-safe in-memory recording implementation with
  optional callback, FIFO ordering, `lastEvent()`, and `clear()`.
- `MetadataChangeEvent`: structured event type with change_type, table_name, actor,
  detail, timestamp, and `toJSON()` serialisation.
- `IMetadataExportPolicy`: pluggable policy controlling which tables are pushed to external
  catalogs (Apache Atlas, DataHub) and with what delay. Shipped with three implementations:
  - `AlwaysExportPolicy` — exports everything immediately (default).
  - `NeverExportPolicy` — suppresses all exports (offline / test mode).
  - `FilteredExportPolicy` — thread-safe exclusion list with configurable uniform delay.
- Focused test suite: 49 acceptance-criteria tests across three test executables
  (`test_metadata_security_provider_focused`, `test_metadata_change_listener_focused`,
  `test_metadata_export_policy_focused`).
- CI workflow `metadata-interfaces-ci.yml` covering all 49 tests on ubuntu-22.04
  (gcc-12) and ubuntu-24.04 (gcc-14).

### Changed
- Production Readiness Checklist: security audit item moved from `[?]` to `[x]` —
  addressed by `IMetadataSecurityProvider` and `InMemoryRbacMetadataSecurityProvider`
  (closes META-MISSING-002).

## [1.5.2] — 2026-03-12
### Fixed
- `SchemaConsistencyChecker`: false positive on nullable column comparison during cross-shard check
- `DistributedMetadataCatalog`: race condition when two coordinators simultaneously discover the same new table

## [1.5.1] — 2025-12-01
### Fixed
- `StatisticsCollector`: equi-height histogram bucket boundary off-by-one for integer columns
- `SchemaVersionManager`: diff script generated incorrect `ALTER TABLE` when column order changed without type change

## [1.5.0] — 2025-09-01
### Added
- `DistributedMetadataCatalog`: multi-node catalog synchronisation with consensus-based conflict resolution
- `ColumnLineageTracker`: tracks data flow from source column through transformations to destination column
- ER diagram export in Mermaid, DOT (Graphviz), and JSON formats via `ErDiagramExporter`
- Schema audit log: all schema modification events (DDL) persisted to RocksDB with timestamp, actor, and change diff
- `SchemaConsistencyChecker`: detects divergence between in-memory schema cache and on-disk RocksDB state
- `IndexRecommender`: analyses query workload statistics and recommends index additions or removals
- External catalog integration: Apache Atlas and DataHub connectors for bi-directional metadata synchronisation
- Adaptive TTL: per-table TTL derived from access frequency and data retention policy

### Changed
- `SchemaManager` auto-discovery now uses RocksDB key-range scanning instead of full iteration; discovery latency reduced by ~60 %
- `StatisticsCollector` histogram precision increased from 64 to 256 buckets

### Fixed
- `InformationSchema`: column ordering non-deterministic across restarts

## [1.4.0] — 2025-03-01
### Added
- `SchemaVersionManager`: schema version tracking with diff generation and migration script output
- Real-time changefeed notifications: subscribers receive schema change events via an internal pub-sub channel
- `INFORMATION_SCHEMA` views: `TABLES`, `COLUMNS`, `INDEXES`, `CONSTRAINTS` compatible with SQL standard naming

### Changed
- Property type detection extended to support `JSONB`, `VECTOR`, and `TIMESTAMP WITH TIME ZONE`

### Fixed
- `SchemaManager`: table discovery missed tables created in non-default RocksDB column families

## [1.3.0] — 2024-09-01
### Added
- `StatisticsCollector`: equi-height histogram construction for numeric and string columns
- Schema version tracking (baseline; diff and migration script generation added in v1.4.0)
- Per-column property type detection with confidence scoring

## [1.2.0] — 2024-04-01
### Added
- `SchemaManager`: auto table discovery via RocksDB key scanning
- Basic `INFORMATION_SCHEMA` stubs (`TABLES`, `COLUMNS`)

## [1.0.0] — 2024-01-01
### Added
- Initial implementation of the Metadata module
- Core schema registry backed by RocksDB
- Basic property type storage and retrieval
