<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Importers Module (Public Headers)

All notable changes to the Importers module public headers are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/importers/CHANGELOG.md`.

## [Unreleased]
- Microsoft SQL Server importer header (Issue #1845)
- FedProx aggregation algorithm (`federated_learning.h`)
- Ethereum smart contract production anchor (`blockchain_integrity.h`)
- Quantum-safe cryptography for audit trail signatures (NIST PQC CRYSTALS-Kyber)

## [2.1.0] — 2026-03-21
### Added
- `gui_import_wizard.h`: GUI import wizard interface for visual source configuration
- `federated_learning.h`: Federated learning data partitioning interface (FedAvg; FedProx planned)
- `blockchain_integrity.h`: Blockchain-anchored integrity verification (stub Ethereum anchor)

## [1.5.0] — 2026-03-16
### Added
- `mdm_engine.h`, `mdm_audit_trail.h`, `mdm_metrics.h`: MDM orchestration, audit, and metrics
- `postgres_importer_mdm.h`: PostgreSQL importer with MDM golden record merge
- `crdt_importer.h`: CRDT-based conflict resolution
- `conflict_resolver.h`, `canonical_resolver.h`: Conflict and canonical resolution

## [1.0.0] — 2024-01-01
### Added
- `importer_interface.h`, `importer_plugin.h`, `importer_plugin_api.h`
- PostgreSQL, MySQL, Oracle, SQLite, Kafka, MongoDB, S3, flat file importers
- `schema_inference.h`, `schema_validator.h`, `data_quality.h`
- `entity_linker.h`, `entity_matcher.h`, `relationship_mapper.h`
- `audit_trail.h`, `temporal_support.h`, `adaptive_import.h`
