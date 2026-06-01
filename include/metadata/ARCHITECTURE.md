> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/metadata/ARCHITECTURE.md -->

# Metadata Module — Public Header Architecture

**Module Path:** `include/metadata/`
**Implementation:** `../../src/metadata/`
**Canonical architecture doc:** [`../../src/metadata/ARCHITECTURE.md`](../../src/metadata/ARCHITECTURE.md)

---

## 1. Overview

`include/metadata/` defines the **public schema, catalog, and metadata-governance contract** for ThemisDB. The 19 headers cover schema discovery and versioning, constraint and consistency validation, lineage and export tooling, information-schema exposure, distributed catalog access, and metadata-security/provider interfaces.

For runtime composition details — schema orchestration, consistency checks, and export pipelines — see:
→ [`../../src/metadata/ARCHITECTURE.md`](../../src/metadata/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Schema and Catalog Core

| Header | Public Type | Purpose |
|--------|------------|---------|
| `schema_manager.h` | `SchemaManager` | Primary schema discovery and mutation contract |
| `schema_version_manager.h` | `SchemaVersionManager` | Version tracking and migration coordination |
| `information_schema.h` | `InformationSchema` | SQL-style metadata exposure |
| `distributed_catalog.h` | `DistributedCatalog` | Distributed metadata access |
| `metadata_snapshot.h` | `MetadataSnapshot` | Stable metadata snapshot DTO |

### 2.2 Validation and Governance

| Header | Public Type | Purpose |
|--------|------------|---------|
| `schema_constraints.h` | `SchemaConstraints` | Constraint-definition contract |
| `schema_consistency_checker.h` | `SchemaConsistencyChecker` | Consistency validation and diagnostics |
| `schema_audit_log.h` | `SchemaAuditLog` | Metadata-change audit trail |
| `schema_diff.h` | `SchemaDiff` | Schema delta computation |
| `statistics_collector.h` | `StatisticsCollector` | Metadata/statistics collection |

### 2.3 Lineage and Export

| Header | Public Type | Purpose |
|--------|------------|---------|
| `column_lineage.h` | `ColumnLineage` | Provenance and lineage traversal |
| `er_diagram_exporter.h` | `ERDiagramExporter` | ER-diagram export |
| `catalog_exporter.h` | `CatalogExporter` | Catalog export surfaces |
| `index_recommender.h` | `IndexRecommender` | Metadata-driven index recommendations |
| `aql_schema_bridge.h` | `AQLSchemaBridge` | Query/AQL schema integration |

### 2.4 Integration and Security Providers

| Header | Public Type | Purpose |
|--------|------------|---------|
| `imetadata_change_listener.h` | `IMetadataChangeListener` | Metadata event listener contract |
| `imetadata_export_policy.h` | `IMetadataExportPolicy` | Export authorization/policy control |
| `imetadata_security_provider.h` / `imetadata_encryption_provider.h` | Security provider interfaces | Metadata security and encryption integration |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::metadata` | Schema, lineage, and catalog types |

---

## 4. Public Contract Notes

- Schema and catalog headers remain public because non-core modules and embedders query metadata directly.
- Validation and diff/audit contracts expose explicit governance outcomes instead of forcing consumers through implementation-only callbacks.
- Export, lineage, and provider interfaces stay public to support deployment-specific integrations and security policies.
- `AQLSchemaBridge` is part of the public metadata contract because query-facing code depends on stable schema translation semantics.
