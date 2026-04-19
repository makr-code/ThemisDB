<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/metadata/ -->

# Metadata Module — Architecture Guide (Public Headers)

**Version:** 1.6.0
**Module Path:** `include/metadata/` | Implementation: `../../src/metadata/`

## Overview
Public header interfaces for schema management, catalog distribution, column lineage tracking,
metadata security, and change notification in ThemisDB.

## Design Principles
- **Interface-First** – all cross-module contracts are pure abstract classes (`I*`).
- **Security Provider Pattern** – `IMetadataSecurityProvider` decouples auth from metadata logic.
- **Observer Pattern** – `IMetadataChangeListener` notifies consumers of schema mutations.
- **Export Policy** – `IMetadataExportPolicy` controls which metadata leaves the system.

## Interface Inventory

| Header | Key Type | Purpose |
|--------|----------|---------|
| `imetadata_security_provider.h` | `IMetadataSecurityProvider` | Auth/ACL for metadata access |
| `imetadata_change_listener.h` | `IMetadataChangeListener` | Schema change observer |
| `imetadata_export_policy.h` | `IMetadataExportPolicy` | Export filtering/masking |
| `schema_manager.h` | `SchemaManager` | CRUD for schemas and tables |
| `schema_version_manager.h` | `SchemaVersionManager` | Schema versioning and migrations |
| `schema_audit_log.h` | `SchemaAuditLog` | Immutable DDL audit trail |
| `schema_constraints.h` | `SchemaConstraints` | Constraint definitions |
| `schema_consistency_checker.h` | `SchemaConsistencyChecker` | Cross-schema validation |
| `catalog_exporter.h` | `CatalogExporter` | Catalog serialisation |
| `distributed_catalog.h` | `DistributedCatalog` | Multi-node catalog coordination |
| `information_schema.h` | `InformationSchema` | ANSI information_schema view |
| `column_lineage.h` | `ColumnLineage` | Column-level data lineage |
| `index_recommender.h` | `IndexRecommender` | Workload-driven index advice |
| `statistics_collector.h` | `StatisticsCollector` | Table/column statistics |
| `aql_schema_bridge.h` | `AqlSchemaBridge` | AQL↔schema integration |
| `er_diagram_exporter.h` | `ErDiagramExporter` | ER diagram generation |

## Reference
- Implementation: `../../src/metadata/`
- Tests: `../../tests/test_metadata_*.cpp`
