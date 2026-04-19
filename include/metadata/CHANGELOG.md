<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Metadata Module (Public Headers)

All notable public API changes are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.6.0] — 2026-03-12
### Added
- `IMetadataSecurityProvider` — decoupled auth/ACL interface for metadata access control
- `IMetadataChangeListener` — observer interface for schema mutation events
- `IMetadataExportPolicy` — policy interface for metadata export filtering and masking
- `er_diagram_exporter.h` — ER diagram generation (DOT, Mermaid)
- `aql_schema_bridge.h` — AQL↔schema type mapping bridge

## [1.5.0] — 2026-02-01
### Added
- `column_lineage.h` — column-level data lineage tracking
- `index_recommender.h` — workload-driven index recommendation
- `statistics_collector.h` — table/column statistics (histogram, NDV)

## [1.4.0] — 2026-01-15
### Added
- `schema_consistency_checker.h` — cross-schema validation
- `distributed_catalog.h` — multi-node catalog coordination

## [1.3.0] — 2025-12-01
### Added
- `schema_audit_log.h`, `schema_constraints.h`, `catalog_exporter.h`, `information_schema.h`

## [1.0.0] — 2025-10-01
### Added
- Initial: `schema_manager.h`, `schema_version_manager.h`
