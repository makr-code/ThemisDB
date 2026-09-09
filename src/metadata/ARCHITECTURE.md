# Architecture - Metadata Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The metadata module composes schema discovery, metadata statistics, consistency verification, and export/integration behavior into a bounded metadata subsystem for ThemisDB.

## Main Execution Planes

1. Schema and catalog plane
- schema discovery and metadata representation surfaces
- information-schema and versioning behavior

2. Validation and consistency plane
- constraints, consistency checks, and audit behavior
- deterministic metadata validation outcomes

3. Lineage and export plane
- lineage graph traversal and ER/catalog export behavior
- distributed metadata and integration interfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| schema contract | deterministic schema discovery and metadata access semantics |
| consistency contract | explicit constraint/consistency outcomes and diagnostics |
| lineage contract | bounded provenance traversal and export behavior |
| integration contract | deterministic catalog export and distributed metadata behavior |

## Failure Semantics

- invalid schema/constraint input fails with explicit outcomes.
- export/integration failures are surfaced explicitly.
- metadata inconsistencies remain observable and non-silent.

## Sourcecode Verification (Module: metadata/architecture)

- Verified files:
  - src/metadata/schema_manager.cpp
  - src/metadata/information_schema.cpp
  - src/metadata/schema_version_manager.cpp
  - src/metadata/schema_consistency_checker.cpp
  - src/metadata/catalog_exporter.cpp
  - src/metadata/distributed_catalog.cpp
- Verified architecture claims:
  - explicit schema/consistency/lineage/integration planes
  - deterministic failure boundaries across metadata workflows
  - module-local ownership of metadata orchestration behavior

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/` | Logging, audit trail, and serialisation helpers |
| storage | `include/storage/` | Persists schema versions, catalog snapshots, and distributed metadata |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `server/schema_*.h`, `statistics_collector.h`, `column_lineage.h`, `index_recommender.h` | Server exposes schema management, statistics, lineage, and recommendation APIs |
| query | `include/metadata/` (StatisticsCollector) | Query planner reads table/column statistics for cost estimation |

## Integration Points

### Critical Integration: Query Statistics Collection
**Files:** `src/metadata/schema_manager.cpp` ↔ `query/` (StatisticsCollector)
**Contract:** Query engine reads `StatisticsCollector` estimates (row counts, cardinalities) from metadata; statistics must be eventually consistent with committed storage state.
**Thread Safety:** Statistics reads are lock-free reads against an atomically updated snapshot; concurrent writes use compare-and-swap.

### Critical Integration: Server Schema Management
**Files:** `server/schema_*.h` ↔ `include/metadata/schema_manager.h`
**Contract:** Server delegates all DDL operations to metadata module; metadata module serialises schema mutations through versioned lock.
**Thread Safety:** Schema mutations are serialised; concurrent read-only schema lookups are safe.