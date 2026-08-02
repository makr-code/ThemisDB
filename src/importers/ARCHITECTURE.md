# Architecture - Importers Module

<!-- Status: current | validated: 2026-08-02 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · BUILD_STATUS.md -->

## Overview

The importers module composes connector-specific ingestion, schema handling, conflict/quality controls, and audit/MDM orchestration into a bounded import subsystem for ThemisDB.

## Main Execution Planes

1. Connector and source plane
- relational/document/stream/file/object source ingestion adapters
- capability-aware connector selection and bounded fallback behavior

2. Schema and transformation plane
- source schema extraction/inference/validation
- mapping and normalization behavior before persistence

3. Conflict/quality/audit plane
- conflict strategies, data quality checks, and immutable audit paths
- deterministic error reporting for invalid/unsafe import outcomes

4. MDM and advanced helper plane
- canonicalization/entity-linking and post-import enrichment helpers
- temporal, CDC, integrity, and federation-related support flows

## Core Contracts

| Contract | Behavior |
|---|---|
| connector contract | deterministic source validation and import execution |
| schema contract | explicit validation/inference and mapping semantics |
| integrity contract | bounded conflict, quality, and auditability behavior |
| enrichment contract | explicit MDM/advanced helper post-processing semantics |

## Failure Semantics

- invalid source/schema input fails with explicit structured outcomes.
- unavailable connector/runtime capability paths degrade deterministically.
- conflict strategy violations or unsafe quality checks fail before unsafe commit behavior.

## Sourcecode Verification (Module: importers/architecture)

- Verified files:
  - src/importers/postgres_importer.cpp
  - src/importers/mysql_importer.cpp
  - src/importers/mongo_importer.cpp
  - src/importers/flatfile_importer.cpp
  - src/importers/schema_validator.cpp
  - src/importers/conflict_resolver.cpp
  - src/importers/data_quality.cpp
  - src/importers/audit_trail.cpp
  - src/importers/mdm_engine.cpp
  - src/importers/postgres_cdc.cpp
- Verified architecture claims:
  - explicit connector/schema/integrity/enrichment planes
  - deterministic failure and fallback behavior boundaries
  - module-local ownership of import orchestration surfaces