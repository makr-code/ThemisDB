> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-08-02 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · BUILD_STATUS.md -->

# Changelog - Importers Module

All notable changes to the importers module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified importer benchmark scenario symbols from importer throughput and process-import benchmark sources.
- #5184 remediation slice: reduced importer hot-path allocation/search overhead (pre-reserved JSONL/CSV/Parquet vectors, cached FK target-column sets and delta-hash key-column index) and replaced C-style hash-file formatting with stream-based zero-padded hex output.

## [2.2.0] - 2026-03-24

### Added
- service-registry and XOEV import-related extension surfaces with focused validation coverage.

## [1.5.0] - 2026-03-16

### Added
- GUI import wizard, MDM audit trail, and importer metric/export support paths.

### Changed
- adaptive import and CRDT conflict behavior improvements.

### Fixed
- importer edge issues for large-field and sparse-schema scenarios.

## [1.4.0] - 2025-09-01

### Added
- advanced importer and MDM/canonicalization capabilities.

### Changed
- CDC and object-source behavior hardening.

### Fixed
- concurrency and format-detection issues in importer paths.

## [1.3.0] - 2025-03-01

### Added
- MDM engine, schema validator, polyglot mapping, and temporal import support.

### Changed
- scoring and conflict strategy behavior refinement.

### Fixed
- CRDT tombstone merge correctness issue.

## [1.2.0] - 2024-09-01

### Added
- Mongo and Kafka importer support plus quality checks.

### Changed
- extraction/transformation decoupling in adaptive import pipeline.

### Fixed
- PostgreSQL schema search-path handling issue.

## [1.1.0] - 2024-06-01

### Added
- SQLite/MySQL importers and schema inference foundations.

### Changed
- parameterized query enforcement across SQL importers.

### Fixed
- flat-file path traversal vulnerability handling.

## [1.0.0] - 2024-01-01

### Added
- foundational PostgreSQL/Oracle importers with schema mapping and audit baseline.