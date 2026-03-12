<!-- Status: current | validated: 2026-03-12 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Importers Module

All notable changes to the Importers module are documented here.  
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Federated learning-based schema drift detection
- Expanded GraphQL federation source support
- Kafka consumer group rebalance-aware import

## [1.5.0] — 2026-03-xx
### Added
- GUI import wizard with step-by-step source configuration and preview
- MDM audit trail with immutable append-only log per entity lifecycle event
- MDM metrics exporter (Prometheus-compatible) for deduplication rate, match confidence, and merge counts
### Changed
- CRDT conflict resolution upgraded to support set-union, LWW, and multi-value register strategies
- Adaptive import now supports dynamic batch-size tuning based on source throughput telemetry
### Fixed
- Oracle importer failing on CLOB columns larger than 32 KB
- Schema inference producing incorrect nullable detection for MongoDB sparse fields

## [1.4.0] — 2025-09-01
### Added
- BigQuery importer with streaming and batch read modes
- Entity linking via deterministic matcher and semantic matcher (embedding-based)
- Blockchain integrity verification for audit trail records
- Canonical resolver for MDM golden-record selection
### Changed
- PostgreSQL CDC importer migrated to logical replication slot (pgoutput plugin)
- S3 importer supports multi-part download and checksum verification
### Fixed
- MySQL importer deadlocking on high-concurrency bulk inserts
- Flatfile importer misidentifying BOM-prefixed UTF-8 files as binary

## [1.3.0] — 2025-03-01
### Added
- MDM engine with probabilistic matching, deduplication, and merge policy engine
- Schema validator with strict and lenient modes
- Polyglot mapper for cross-source schema alignment
- Temporal support for bi-temporal import with valid-time and transaction-time tracking
### Changed
- Column importance scoring now uses mutual information in addition to frequency heuristics
- Conflict resolver supports pluggable strategy registry
### Fixed
- CRDT importer merging tombstoned records incorrectly on re-import

## [1.2.0] — 2024-09-01
### Added
- MongoDB importer with change-stream support
- Kafka importer with exactly-once semantics via idempotent producer acknowledgement
- Data quality checks (completeness, uniqueness, referential integrity)
### Changed
- Adaptive import pipeline refactored to decouple extraction from transformation
### Fixed
- PostgreSQL importer not honouring schema search path for non-public schemas

## [1.1.0] — 2024-06-01
### Added
- SQLite and MySQL importers
- Schema inference engine for CSV, JSON, and Parquet flat files
- Credential masking in all import log outputs
### Changed
- Parameterised query interface enforced across all SQL importers
### Fixed
- Path traversal vulnerability in flatfile importer (relative `../` paths now rejected)

## [1.0.0] — 2024-01-01
### Added
- Initial implementation: PostgreSQL and Oracle importers
- Basic schema mapping and type coercion
- Audit trail foundation
