<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/importers/ -->

# Importers Module — Public Header Architecture
**Version:** 2.1.0
**Module Path:** `include/importers/`
**Implementation:** `../../src/importers/`

---

## Overview

The Importers module provides a comprehensive set of public headers for multi-source data ingestion: relational databases (PostgreSQL, MySQL, Oracle, SQLite), streaming (Kafka, Postgres CDC), object storage (S3), flat files, GraphQL federation, CRDT-based conflict resolution, MDM (Master Data Management), blockchain integrity anchoring, federated learning, and a GUI import wizard.

## Design Principles

- **Plugin Architecture** — `importer_plugin.h` / `importer_plugin_api.h` enable third-party importers without core changes.
- **MDM Integration** — `mdm_engine.h` orchestrates entity resolution, deduplication, and golden record construction across sources.
- **Schema Intelligence** — `schema_inference.h` auto-derives schema from raw data; `schema_validator.h` enforces it.
- **Audit-First** — `audit_trail.h` and `mdm_audit_trail.h` record all import operations with blockchain-anchored integrity (optional).

## Interface Inventory

| Header | Classes / Structs | Purpose |
|--------|-------------------|---------|
| `importer_interface.h` / `importer_interfaces.h` | `IImporter` | Base importer interface |
| `importer_plugin.h` / `importer_plugin_api.h` | — | Plugin registration API |
| `postgres_importer.h` | — | PostgreSQL full-load importer |
| `postgres_importer_mdm.h` | — | PostgreSQL importer with MDM golden record merge |
| `postgres_cdc.h` | — | PostgreSQL CDC (logical replication) connector |
| `mysql_importer.h` | — | MySQL importer |
| `oracle_importer.h` | — | Oracle DB importer |
| `sqlite_importer.h` | — | SQLite importer |
| `kafka_importer.h` | — | Kafka consumer importer |
| `mongo_importer.h` | — | MongoDB document importer |
| `s3_importer.h` | — | AWS S3 object storage importer |
| `flatfile_importer.h` | — | CSV/TSV/fixed-width flat file importer |
| `graphql_federation.h` | — | GraphQL federated source importer |
| `crdt_importer.h` | — | CRDT-based conflict resolution during import |
| `conflict_resolver.h` | — | Configurable conflict resolution strategies |
| `canonical_resolver.h` | — | Canonical entity form resolution |
| `entity_linker.h` | — | Cross-source entity linking |
| `entity_matcher.h` | — | Entity matching with configurable similarity |
| `relationship_mapper.h` | — | Relationship extraction between imported entities |
| `polyglot_mapper.h` | — | Multi-format schema mapping |
| `schema_inference.h` | — | Automatic schema inference |
| `schema_validator.h` | — | Schema validation |
| `data_quality.h` | — | Data quality scoring and reporting |
| `column_importance.h` | — | Column importance ranking for import prioritisation |
| `adaptive_import.h` | — | Adaptive import rate limiting |
| `mdm_engine.h` | — | MDM orchestration: entity resolution, deduplication, golden record |
| `mdm_audit_trail.h` | — | MDM-specific audit trail |
| `mdm_metrics.h` | — | MDM performance metrics |
| `temporal_support.h` | — | Bi-temporal data import (valid-time + transaction-time) |
| `audit_trail.h` | — | Full import audit trail |
| `blockchain_integrity.h` | — | Blockchain-anchored integrity verification |
| `federated_learning.h` | — | Federated learning data partitioning |
| `gui_import_wizard.h` | — | GUI import wizard interface |

## References

- Implementation details: `../../src/importers/`
- MDM guide: `../../src/importers/README.md`
