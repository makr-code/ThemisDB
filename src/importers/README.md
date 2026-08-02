# ThemisDB Importers Module

<!-- Status: current | validated: 2026-08-02 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · BUILD_STATUS.md -->

## Module Purpose

The importers module provides source-to-ThemisDB ingestion capabilities for relational, document, stream, and file/object sources, including schema handling, conflict resolution, validation, auditability, and optional MDM/post-processing paths.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| postgres_importer.cpp | PostgreSQL import execution and schema extraction |
| mysql_importer.cpp | MySQL/MariaDB import paths |
| sqlite_importer.cpp | SQLite import paths |
| oracle_importer.cpp | Oracle import paths |
| mongo_importer.cpp | MongoDB document import paths |
| kafka_importer.cpp | streaming import entry points |
| flatfile_importer.cpp | CSV/TSV/Parquet/flat-file ingestion |
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

- behavior depends on enabled connectors and build/runtime feature flags.
- conflict strategy and validation options strongly influence import outcomes.
- optional CDC/streaming/object-source paths degrade deterministically when unavailable.
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
- `runFullImport(...)` for snapshot ingest with canonical tables (`hf_dataset_catalog`, `legal_document`, `legal_annotation`, `training_example`, `compliance_audit`)
- `runIncrementalUpdate(...)` for delta refresh with dirty-record tracking, idempotent upsert, checkpoint/resume
- `validateQuality()` for quality/compliance gates (license, minimal content quality)
- `exportAdaLoraJsonl(...)` for deterministic AdaLoRA JSONL output (`instruction/input/target/system` or `prompt/response`)

## Sourcecode Verification (Module: importers/readme)

- Verified files:
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
- Verified behavior surfaces:
  - source import execution, schema/validation/conflict handling, audit/MDM and advanced helper integration
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md