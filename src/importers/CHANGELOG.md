> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Importers Module

All notable changes to the Importers module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Microsoft SQL Server importer (Issue: #1845)
- FedProx aggregation algorithm in `federated_learning.cpp`
- Ethereum smart contract production anchor for `blockchain_integrity.cpp`
- Quantum-safe cryptography (NIST PQC CRYSTALS-Kyber) for audit trail signatures
- Zero-Knowledge Proofs for privacy-preserving data validation
- Kafka consumer group rebalance-aware import
- Performance benchmarks: schema inference vs. manual mapping

## [2.2.0] — 2026-03-24
### Added
- `ozg_service_registry.h`: `OZGServiceEntry`, `IOZGServiceRegistry`, `InMemoryOZGServiceRegistry` — thread-safe in-memory registry for German OZG 2.0 (Onlinezugangsgesetz) service catalog; supports findById, findByStatus (OZGServiceStatus), findByState (Bundesland AGS code), findByComplianceTag, SDGR/FIM fields; 30 tests in `tests/test_ozg_service_registry.cpp`; CI: `ozg-service-registry-ci.yml`
- `xoev_importer.h`: `XOEVRecord`, `XOEVImportResult`, `XOEVExportResult`, `IXOEVImporter`, `InMemoryXOEVImporter` — import/export for XÖV (XML in der öffentlichen Verwaltung) data models covering XPersonenstand, XMeld, XBau, XKfz, XFinanz, XGewerbeanmeldung; lightweight XML parse/emit; 30 tests in `tests/test_xoev_importer.cpp`; CI: `xoev-importer-ci.yml`

## [1.5.0] — 2026-03-16
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
