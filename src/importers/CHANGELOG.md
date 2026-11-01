---
Author: "ThemisDB Team"
Created: "2026-08-02"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "release-versioning"
Canonical_References: "src/importers/*.cpp; include/importers/*.h; tests/importers/CMakeLists.txt; benchmarks/importers/CMakeLists.txt"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.
> Historical changelog entries describe implementation state at the time recorded.

<!-- Historical changelog status marker; entries remain time-bound. -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · BUILD_STATUS.md -->

# Changelog - Importers Module

All notable changes to the importers module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- #7353 documentation sync: importer API, implementation, test registration, build presets, and benchmark-source claims checked against current repository files; runtime build/test/benchmark execution is not claimed.
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