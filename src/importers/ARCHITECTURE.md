---
Author: "ThemisDB Team"
Created: "2026-08-02"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/importers/importer_interface.h; include/importers/importers_api_contract.h; src/importers/*.cpp"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Creation-date note: Original creation date is not recorded in the available checkout; the front-matter `Created` date is a metadata placeholder, not independently verified.

> Exact model backend not verifiable from final document text alone.

# Architecture - Importers Module

<!-- Historical validation marker (2026-08-02); not a current audit result. -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · BUILD_STATUS.md -->

## Overview

The importers module contains connector implementations and shared schema,
conflict, quality, audit, MDM, and specialized ingestion components. Availability
and exact behavior are connector- and build-dependent; the shared interface does
not imply that every connector implements every option.

## Main Execution Planes

1. Connector and source plane
- relational/document/stream/file/object-source ingestion implementations
- connector capability checks and connector-specific unavailable/error paths

2. Schema and transformation plane
- source schema extraction/inference/validation
- mapping and normalization behavior before persistence

3. Conflict/quality/audit plane
- conflict strategies, data quality checks, and immutable audit paths
- structured error reporting for supported validation and failure paths

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

- `importer_interface.h` defines connector-facing error types; `importers_api_contract.h`
  declares a separate contract error enum and semantics.
- The two headers are distinct contracts. The versioned contract specifies
  successful, no-added-row replay for the same source and `import_id`, and
  `IMPORT_DUPLICATE_ID` for a different source under that ID. The focused mock
  models these cases; see [`BUILD_STATUS.md`](./BUILD_STATUS.md). No coordinator
  in `src/importers/` currently consumes the versioned contract enum.
- Check a concrete connector and its focused tests before asserting fallback,
  transaction, checkpoint, or option behavior.

## Source References (selected, not a full implementation audit)

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
- These source files illustrate connector, schema, integrity, and enrichment
  components. They do not establish parity across all connectors or production
  maturity for the full module.
---

### Direct Downstream Consumers (modules that use this module)

| Module | Via | Notes |
|--------|-----|-------|
| `server` | `include/importers/importer_interface.h`, `include/importers/importer_interfaces.h`, `include/importers/s3_importer.h` | Import API handler dispatches ingestion requests to the appropriate importer implementation (`include/server/import_api_handler.h`) |
| `llm_wiki` | `include/importers/wikipedia_pipeline.hpp`, `include/importers/wikipedia_types.hpp` | Wikipedia LLM plugin drives Wikipedia corpus ingestion through the importer pipeline (`src/llm_wiki/wikipedia/llm_wiki_plugin_impl.cpp`) |
