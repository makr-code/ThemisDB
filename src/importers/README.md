---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "src/importers/*.cpp; include/importers/*.h; include/importers/*.hpp; tests/importers/CMakeLists.txt; benchmarks/importers/CMakeLists.txt"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Exact model backend not verifiable from final document text alone.

# ThemisDB Importers Module

<!-- This page inventories source surfaces; it is not a maturity or release-readiness certification. -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · BUILD_STATUS.md · PERFORMANCE_EXPECTATIONS.md -->

## Module Purpose

Importer implementations cover relational, document, streaming, file, and object-source paths plus shared schema, conflict, quality, audit, and optional MDM helpers. Availability and behavior vary by connector, build options, and runtime capability. Documentation and registration are not evidence of production readiness or successful runtime tests.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| postgres_importer.cpp | PostgreSQL dump import execution and schema extraction |
| mysql_importer.cpp | MySQL/MariaDB import paths |
| sqlite_importer.cpp | SQLite import paths |
| oracle_importer.cpp | Oracle import paths |
| mongo_importer.cpp | MongoDB document import paths |
| kafka_importer.cpp | streaming import entry points |
| flatfile_importer.cpp | CSV/TSV/JSONL/NDJSON and conditional Parquet ingestion |
| s3_importer.cpp | object-storage backed import surfaces |
| schema_validator.cpp | schema validation logic |
| schema_inference.cpp | schema inference and relationship hints |
| conflict_resolver.cpp | duplicate/conflict strategy behavior |
| data_quality.cpp | data quality scoring and checks |
| audit_trail.cpp | import audit evidence tracking |
| mdm_engine.cpp | MDM orchestration for imported entities |
| entity_linker.cpp | entity matching/linking behavior |
| canonical_resolver.cpp | canonical/golden-record resolution |
| huggingface_ingest_plugin.cpp | HuggingFace legal-data ingestion pipeline (raw→canonical→projections→AdaLoRA export) |
| postgres_cdc.cpp | CDC-oriented import interfaces |
| adaptive_import.cpp | adaptive batch/import planning |
| polyglot_mapper.cpp | polyglot mapping recommendations |
| temporal_support.cpp | temporal import support |
| blockchain_integrity.cpp | integrity verification paths |
| federated_learning.cpp | federated ingest-related helper flows |
| graphql_federation.cpp | GraphQL federation-related import metadata paths |
| wikipedia_plugin.cpp + wikipedia_pipeline.cpp | Wikipedia full-import / delta-ingest orchestration and plugin lifecycle |
| wikipedia_dump_reader.cpp + wikipedia_xml_parser.cpp | streaming-friendly Wikimedia dump page parsing |
| wikipedia_transform.cpp + wikipedia_project_*.cpp | canonical Wikipedia core mapping and graph/vector/process/timeseries projections |
| wikipedia_checkpoint.cpp + wikipedia_validator.cpp | checkpoint/resume, validation, manifest, and portable export for `wikipedia.db` |

## Scope

In scope:
- importer execution across supported source classes
- schema, conflict, quality, and audit behavior in import runtime
- optional MDM/advanced helper flows that post-process imported data
- Wikipedia dump ingestion with canonical page/revision/link/category/redirect state, dirty-page delta refresh, and portable manifest export

Out of scope:
- non-import storage/query ownership outside importer interfaces
- external scheduler/orchestrator ownership outside module contracts
- UI ownership beyond importer-exposed integration surfaces

## Runtime Behavior and Limits

- behavior depends on enabled connectors and build/runtime feature flags; `ARROW_ENABLED` gates Parquet code, while some connector sources are conditionally included by CMake.
- conflict strategy and validation options strongly influence import outcomes.
- connector capability failures and fallback behavior are implementation-specific;
  verify the selected connector rather than assuming deterministic degradation.
- Wikipedia MVP exports a portable `wikipedia.db` JSON artifact plus `manifest.json` and `wikipedia.db.verify.json` sidecars for verification.

## Installation

- Build ThemisDB with the standard importer/plugin targets enabled.
- The Wikipedia importer is compiled into the existing importer/plugin runtime; no extra vendor embedding backend is required for the MVP.

## Usage

- Full import: initialize `WikipediaIngestionPlugin`, then call `runFullImport(...)` with a Wikimedia XML dump path.
- Incremental update: call `runIncrementalUpdate(...)` on a later dump; dirty pages trigger selective graph/vector/process/timeseries refresh.
- Verify/export: call `validateDatabase()` and `exportPortable(\"./wikipedia.db\", \"./manifest.json\")`.

## HuggingFace Legal Ingestion (MVP)

- `init()`/`shutdown()` lifecycle for plugin boot and teardown
- `runFullImport(...)` normalizes supplied `seed_rows` into in-memory catalog, document, annotation, training-example, and compliance-audit maps
- `runIncrementalUpdate(...)` normalizes supplied `changed_rows` and tracks dirty records; its checkpoint token is in-memory, and the `resume_from_checkpoint` flag does not restore a saved source position
- `validateQuality()` for quality/compliance gates (license, minimal content quality)
- `exportAdaLoraJsonl(...)` for deterministic AdaLoRA JSONL output (`instruction/input/target/system` or `prompt/response`)
- `fetchRawRows(...)` currently returns the rows supplied by the caller; this is not a live HuggingFace API connector or durable database writer

## Source References (inventory, not a full audit)

- Selected implementation files referenced above:
  - src/importers/postgres_importer.cpp
  - src/importers/mysql_importer.cpp
  - src/importers/sqlite_importer.cpp
  - src/importers/oracle_importer.cpp
  - src/importers/mongo_importer.cpp
  - src/importers/kafka_importer.cpp
  - src/importers/flatfile_importer.cpp
  - src/importers/s3_importer.cpp
  - src/importers/schema_validator.cpp
  - src/importers/schema_inference.cpp
  - src/importers/conflict_resolver.cpp
  - src/importers/data_quality.cpp
  - src/importers/audit_trail.cpp
  - src/importers/mdm_engine.cpp
  - src/importers/entity_linker.cpp
  - src/importers/canonical_resolver.cpp
  - src/importers/postgres_cdc.cpp
  - src/importers/adaptive_import.cpp
  - src/importers/polyglot_mapper.cpp
  - src/importers/temporal_support.cpp
  - src/importers/blockchain_integrity.cpp
  - src/importers/federated_learning.cpp
  - src/importers/graphql_federation.cpp
  - src/importers/huggingface_ingest_plugin.cpp
  - src/importers/wikipedia_plugin.cpp
  - src/importers/wikipedia_pipeline.cpp
  - src/importers/wikipedia_dump_reader.cpp
  - src/importers/wikipedia_xml_parser.cpp
  - src/importers/wikipedia_transform.cpp
  - src/importers/wikipedia_project_graph.cpp
  - src/importers/wikipedia_project_vector.cpp
  - src/importers/wikipedia_project_process.cpp
  - src/importers/wikipedia_project_timeseries.cpp
  - src/importers/wikipedia_checkpoint.cpp
  - src/importers/wikipedia_validator.cpp
- Behavior surfaces represented in these sources:
  - source import execution, schema/validation/conflict handling, audit/MDM and helper components
  - Wikipedia XML full/delta processing, dirty-page projection rebuild and portable validation/export
  - HuggingFace caller-supplied row normalization, validation, checkpoint token, and JSONL export behavior
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md; release history is recorded separately in CHANGELOG.md
  - TODO: reconcile contract wording and mock behavior for duplicate `import_id` replay before claiming connector-wide idempotency


## Developer Documentation

- [Research and evaluation](./RESEARCH.md)
- [Schema and data modeling](./MODELING.md)
- [Embedding boundary](./EMBEDDING.md)
- [Implementation workflow](./IMPLEMENTATION.md)
- [Public API reference](./API.md)
- [Developer module guide](./MODULE_GUIDE.md)
- [Incident runbook](./RUNBOOK.md)
- [Operations guide](./OPERATIONS.md)
- [Trust model](./TRUST.md)