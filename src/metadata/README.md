# ThemisDB Metadata Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The metadata module provides schema discovery, metadata management, statistics surfaces, consistency tooling, and metadata export/integration behavior for ThemisDB.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| schema_manager.cpp | central schema discovery and metadata orchestration |
| statistics_collector.cpp | metadata statistics collection surfaces |
| information_schema.cpp | information-schema style metadata exposure |
| schema_version_manager.cpp | schema versioning and migration support |
| schema_audit_log.cpp | metadata change audit history |
| schema_consistency_checker.cpp | consistency validation and diagnostics |
| schema_constraints.cpp | metadata/schema constraint enforcement |
| column_lineage.cpp | lineage and provenance metadata traversal |
| er_diagram_exporter.cpp | ER export behavior |
| catalog_exporter.cpp | metadata export and integration endpoints |
| distributed_catalog.cpp | distributed metadata catalog surfaces |
| index_recommender.cpp | metadata-driven index recommendation support |

## Scope

In scope:
- metadata and schema discovery/lifecycle behavior
- metadata consistency, lineage, and export surfaces
- information-schema and metadata statistics behavior

Out of scope:
- storage-engine ownership beyond metadata interactions
- query runtime ownership beyond metadata support boundaries
- non-metadata operational domains outside schema and catalog scope

## Runtime Behavior and Limits

- behavior depends on discovered schema state, configured policies, and runtime feature flags.
- unsupported metadata export/integration paths degrade deterministically with explicit outcomes.
- consistency and lineage behavior remains bounded by metadata graph and schema state.

## Sourcecode Verification (Module: metadata/readme)

- Verified files:
  - src/metadata/schema_manager.cpp
  - src/metadata/statistics_collector.cpp
  - src/metadata/information_schema.cpp
  - src/metadata/schema_version_manager.cpp
  - src/metadata/schema_audit_log.cpp
  - src/metadata/schema_consistency_checker.cpp
  - src/metadata/schema_constraints.cpp
  - src/metadata/column_lineage.cpp
  - src/metadata/er_diagram_exporter.cpp
  - src/metadata/catalog_exporter.cpp
  - src/metadata/distributed_catalog.cpp
  - src/metadata/index_recommender.cpp
- Verified behavior surfaces:
  - schema/metadata orchestration, consistency, and export paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md