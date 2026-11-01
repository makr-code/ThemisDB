> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- API names and namespaces reconciled 2026-10-31; remaining interface claims require header-level review -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/metadata/ARCHITECTURE.md -->

# Metadata Module — Public Header Architecture

**Author:** ThemisDB Contributors  
**Created:** 2026-10-31  
**Last Updated:** 2026-10-31  
**Status:** review  
**AI_Used:** true  
**AI_Models:** unknown  
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator  
**Human_Review:** false  
**Human_Author:** ThemisDB Contributors  
**Source level:** level1 — module-local  
**SOT domain:** api-contract  
**Canonical source references:** [metadata public headers](./), [runtime architecture](../../src/metadata/ARCHITECTURE.md), [metadata API reference](../../src/metadata/API.md)  
**Related issue / milestone:** [#7362](https://github.com/makr-code/ThemisDB/issues/7362) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

**Module Path:** `include/metadata/`
**Implementation:** `../../src/metadata/`
**Canonical architecture doc:** [`../../src/metadata/ARCHITECTURE.md`](../../src/metadata/ARCHITECTURE.md)

---

## 1. Overview

`include/metadata/` contains 20 headers covering schema discovery and
versioning, constraints and consistency, lineage and export, information-schema
views, distributed catalog access, snapshots, and provider interfaces. Public
type names and namespaces are header-specific; the owning header is authoritative.

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
| `distributed_catalog.h` | `DistributedMetadataCatalog` | Distributed metadata access through a shard router |
| `metadata_snapshot.h` | `MetadataSnapshot`, `IMetadataSnapshotStore`, `InMemoryMetadataSnapshotStore` | Snapshot value and store API; supplied store is in-memory |

### 2.2 Validation and Governance

| Header | Public Type | Purpose |
|--------|------------|---------|
| `schema_constraints.h` | `SchemaConstraints` | Constraint-definition contract |
| `schema_consistency_checker.h` | `SchemaConsistencyChecker` | Consistency validation and diagnostics |
| `schema_audit_log.h` | `SchemaAuditLog` | Metadata-change audit trail |
| `schema_diff.h` | `SchemaDiffEngine`, `SchemaDiff` | Schema delta computation |
| `statistics_collector.h` | `StatisticsCollector` | Metadata/statistics collection |

### 2.3 Lineage and Export

| Header | Public Type | Purpose |
|--------|------------|---------|
| `column_lineage.h` | `ColumnLineageTracker`, `ColumnRef` | Provenance and lineage traversal |
| `er_diagram_exporter.h` | `ERDiagramExporter` | ER-diagram export |
| `catalog_exporter.h` | `CatalogExporter` | Catalog export surfaces |
| `index_recommender.h` | `IndexRecommender` | Metadata-driven index recommendations |
| `aql_schema_bridge.h` | `themis::aql::fromTableSchema()` | Inline schema-to-AQL metadata adapter |

### 2.4 Integration and Security Providers

| Header | Public Type | Purpose |
|--------|------------|---------|
| `imetadata_change_listener.h` | `IMetadataChangeListener` | Metadata event listener contract |
| `imetadata_export_policy.h` | `IMetadataExportPolicy` | Export authorization/policy control |
| `imetadata_security_provider.h` / `imetadata_encryption_provider.h` | Security provider interfaces | Metadata security and encryption integration |
| `metadata_api_contract.h` | `MetaError` | Frozen artifact; see the stale-contract warning in `../../src/metadata/API.md` |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis` | Core schema, statistics, catalog, and exporter declarations |
| `themis::metadata` | Provider, snapshot, diff, lineage, and index-recommendation declarations |
| `themis::aql` | `fromTableSchema()` adapter |

---

## 4. Public Contract Notes

- Read each owning header for method signatures, return/error behavior,
  lifetime, and thread-safety; do not infer one class's guarantees from another.
- `IMetadataSecurityProvider` and `IMetadataExportPolicy` are separate
  interfaces. Their declarations do not establish that all callers enforce
  authorization or that `CatalogExporter` applies export policies.
- `metadata_api_contract.h` declares `MetaError`, but its method names and
  behavioral text do not describe the current runtime API. See
  [`../../src/metadata/API.md`](../../src/metadata/API.md).
- The AQL bridge is the inline free function
  `themis::aql::fromTableSchema()`, not an `AQLSchemaBridge` class.
