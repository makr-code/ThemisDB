# ThemisDB Ingestion Module

<!-- Status: Production Ready (Wave B Performance) | validated: 2026-08-14 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS_BATCH5.md · PERFORMANCE_EXPECTATIONS.md -->
<!-- Wave Context: Wave B (Performance Consolidation Q3-Q4 2026) — Backpressure + Ordering Guarantees + Schema Enforcement -->

## Module Purpose

Production-capable data ingestion paths providing batch/streaming entry points, backpressure and flow control, ordering guarantees, and schema validation/enforcement for ThemisDB. **Batch 5 enhancement focus: Backpressure determinism, ordering guarantee reliability, schema enforcement under sustained load**.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| ingestion_manager.cpp | central ingestion orchestration and source lifecycle |
| ingestion_coordinator.cpp | parallel/distributed ingestion coordination |
| ingestion_sinks.cpp | sink-layer output behavior |
| ingestion_quality_judge.cpp | ingestion quality evaluation and feedback control |
| filesystem_ingester.cpp | local filesystem source ingestion |
| api_connector.cpp | generic API source connector |
| kafka_connector.cpp | Kafka source connector |
| cdc_connector.cpp | CDC source connector behavior |
| database_connector.cpp | database/JDBC style source connector |
| object_storage_connector.cpp | object storage source connector |
| s3_connector.cpp | S3-focused source connector |
| web_crawler_connector.cpp | crawl/sitemap ingestion source |
| huggingface_connector.cpp | HuggingFace dataset source connector |
| schema_validator.cpp | schema validation behavior |
| semantic_validator.cpp | semantic quality validation |
| deontic_extractor.cpp | legal deontic extraction support |
| agentic_reference_validator.cpp | reference validation support |
| llm_adapter.cpp | LLM adapter bridge for ingestion extraction |
| workflow_engine.cpp | workflow profile/step execution orchestration |

## Scope

In scope:
- source connector execution and ingestion orchestration
- validation/retry/rate limiting/checkpoint/quarantine behavior
- quality/judge/workflow support for structured ingestion pipelines

Out of scope:
- storage-layer ownership beyond ingestion sink interfaces
- query/planner ownership outside ingestion integration boundaries
- client/UI ownership beyond ingestion API and workflow interfaces

## Runtime Behavior and Limits

- behavior depends on configured connector capabilities and runtime feature flags.
- unsupported connector paths degrade deterministically with explicit outcomes.
- quality and workflow phases depend on configured thresholds and enabled adapters.

## Sourcecode Verification (Module: ingestion/readme)

- Verified files:
  - src/ingestion/ingestion_manager.cpp
  - src/ingestion/ingestion_coordinator.cpp
  - src/ingestion/ingestion_sinks.cpp
  - src/ingestion/ingestion_quality_judge.cpp
  - src/ingestion/filesystem_ingester.cpp
  - src/ingestion/api_connector.cpp
  - src/ingestion/kafka_connector.cpp
  - src/ingestion/cdc_connector.cpp
  - src/ingestion/database_connector.cpp
  - src/ingestion/object_storage_connector.cpp
  - src/ingestion/s3_connector.cpp
  - src/ingestion/web_crawler_connector.cpp
  - src/ingestion/huggingface_connector.cpp
  - src/ingestion/schema_validator.cpp
  - src/ingestion/semantic_validator.cpp
  - src/ingestion/deontic_extractor.cpp
  - src/ingestion/agentic_reference_validator.cpp
  - src/ingestion/llm_adapter.cpp
  - src/ingestion/workflow_engine.cpp
- Verified behavior surfaces:
  - connector orchestration, validation/quality, and workflow execution paths
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md