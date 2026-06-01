> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/importers/ARCHITECTURE.md -->

# Importers Module — Public Header Architecture

**Module Path:** `include/importers/`  
**Implementation:** `../../src/importers/`  
**Canonical architecture doc:** [`../../src/importers/ARCHITECTURE.md`](../../src/importers/ARCHITECTURE.md)

---

## 1. Overview

`include/importers/` defines the **public multi-source data import (PostgreSQL, MySQL, Oracle, MongoDB, Kafka, S3, flat files, CRDT, MDM), schema inference, entity matching, federated learning, and e-government standards API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/importers/ARCHITECTURE.md`](../../src/importers/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Importer Interfaces

| Header | Public Type | Purpose |
|--------|------------|---------|
| `importer_interface.h` | `IImporter` | Base importer contract |
| `importer_interfaces.h` | `ImporterInterfaces` | Extended importer interface set |
| `importer_plugin.h` | `IImporterPlugin` | Plugin-based importer interface |
| `importer_plugin_api.h` | `ImporterPluginAPI` | Plugin API registration |
| `importer_common.h` | `ImporterCommon` | Shared importer utilities |
| `adaptive_import.h` | `AdaptiveImport` | Adaptive batch-size import |
### 2.2 Database Importers

| Header | Public Type | Purpose |
|--------|------------|---------|
| `postgres_importer.h` | `PostgresImporter` | PostgreSQL bulk import |
| `postgres_cdc.h` | `PostgresCDC` | PostgreSQL CDC-based streaming import |
| `postgres_importer_mdm.h` | `PostgresImporterMDM` | PostgreSQL MDM-aware importer |
| `mysql_importer.h` | `MySQLImporter` | MySQL bulk import |
| `oracle_importer.h` | `OracleImporter` | Oracle Database import |
| `mongo_importer.h` | `MongoImporter` | MongoDB import |
| `sqlite_importer.h` | `SQLiteImporter` | SQLite import |
### 2.3 File and Cloud Importers

| Header | Public Type | Purpose |
|--------|------------|---------|
| `flatfile_importer.h` | `FlatFileImporter` | CSV/TSV/fixed-width flat file import |
| `kafka_importer.h` | `KafkaImporter` | Kafka topic streaming import |
| `s3_importer.h` | `S3Importer` | S3-compatible object storage import |
### 2.4 Data Quality and Matching

| Header | Public Type | Purpose |
|--------|------------|---------|
| `schema_inference.h` | `SchemaInference` | Automatic schema inference from samples |
| `schema_validator.h` | `SchemaValidator` | Schema validation before import |
| `data_quality.h` | `DataQuality` | Data quality scoring and profiling |
| `entity_linker.h` | `EntityLinker` | Cross-source entity linking |
| `entity_matcher.h` | `EntityMatcher` | Fuzzy entity matching |
| `conflict_resolver.h` | `ConflictResolver` | Multi-source conflict resolution |
| `canonical_resolver.h` | `CanonicalResolver` | Canonical record selection |
| `column_importance.h` | `ColumnImportance` | Column-level importance scoring |
| `relationship_mapper.h` | `RelationshipMapper` | Automatic relationship inference |
| `polyglot_mapper.h` | `PolyglotMapper` | Cross-format schema mapping |
### 2.5 MDM and Advanced

| Header | Public Type | Purpose |
|--------|------------|---------|
| `mdm_engine.h` | `MDMEngine` | Master data management engine |
| `mdm_audit_trail.h` | `MDMAuditTrail` | MDM change audit trail |
| `mdm_metrics.h` | `MDMMetrics` | MDM telemetry |
| `federated_learning.h` | `FederatedLearning` | Privacy-preserving federated import |
| `crdt_importer.h` | `CRDTImporter` | CRDT-based conflict-free replicated import |
| `blockchain_integrity.h` | `BlockchainIntegrity` | Blockchain-based import integrity verification |
| `audit_trail.h` | `AuditTrail` | General import audit trail |
| `temporal_support.h` | `TemporalSupport` | Bi-temporal import support |
| `graphql_federation.h` | `GraphQLFederation` | GraphQL Federation source import |
| `gui_import_wizard.h` | `GUIImportWizard` | UI-driven import configuration wizard |
| `xoev_importer.h` | `XOEVImporter` | XÖV German e-government standard importer |
| `ozg_service_registry.h` | `OZGServiceRegistry` | OZG service registry connector |

---

## 3. Namespace Layout

All public types reside in the `themis::importers` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/importers/` expose the **stable public API**; internal types live in `src/importers/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **ANN/Tensor**.
