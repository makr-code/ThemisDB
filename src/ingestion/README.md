# ThemisDB Ingestion Module

<!-- Status: implementation present; deployment readiness is caller-specific | validated: 2026-10-31 -->
<!-- Links: ARCHITECTURE.md · API.md · IMPLEMENTATION.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

## Module Purpose

The module provides source connectors, `IngestionManager` orchestration, optional
workflow execution, validation/quarantine, coordinator interfaces, and sink
contracts. These interfaces do not imply production durability, universal
backpressure, or global ordering guarantees. Actual behavior depends on the
selected connector, configuration, injected backends, and caller wiring.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| ingestion_manager.cpp | central ingestion orchestration and source lifecycle |
| ingestion_coordinator.cpp | worker dispatch and aggregation via coordinator interfaces |
| ingestion_sinks.cpp | implementations of sink adapters |
| ingestion_quality_judge.cpp | ingestion quality evaluation and feedback control |
| filesystem_ingester.cpp | local filesystem source ingestion |
| api_connector.cpp | generic API source connector |
| kafka_connector.cpp | Kafka source connector |
| cdc_connector.cpp | CDC source connector behavior |
| database_connector.cpp | database source connector; backend/protocol support is implementation-specific |
| object_storage_connector.cpp | object storage source connector |
| s3_connector.cpp | S3-focused source connector |
| web_crawler_connector.cpp | crawl/sitemap ingestion source |
| huggingface_connector.cpp | HuggingFace dataset source connector |
| ingestion_manager.cpp | manager-applied schema validation configuration |
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

- `IngestionManager::ingestSource()` dispatches through a connector, or through
  the filesystem workflow branch when that path is enabled/configured.
- The workflow engine executes ordered steps and returns extraction/entity
  output. The current filesystem workflow branch does not hand that output to a
  sink; its processed count is not evidence of persistence.
- `IngestionCoordinator` partitions source configurations among workers; durable
  cross-process checkpointing/leadership requires injected implementations.
- Reports can represent partial success. Review per-source counts, errors, and
  quarantine entries rather than treating a non-empty report as a transaction.
- `IngestionAdminApi` is in-process; network exposure, authentication, and
  authorization belong to the owning service.
- Limits, fallback behavior, and inference evaluation are connector- and
  configuration-specific; verify the concrete implementation and tests.

## Developer Documentation

- [Module guide](./MODULE_GUIDE.md) — onboarding, build, and test entry points.
- [Implementation](./IMPLEMENTATION.md) and [API contract](./API.md) — runtime behavior and integration boundaries.
- [Research](./RESEARCH.md), [modeling](./MODELING.md), and [embedding](./EMBEDDING.md) — evaluation assumptions and representation constraints.
- [Runbook](./RUNBOOK.md) and [operations](./OPERATIONS.md) — diagnosis, recovery, and observability.
- [Trust model](./TRUST.md) — trust boundaries, risks, and controls.

Public API, implementation, build, and test evidence: [`API.md`](./API.md),
[`IMPLEMENTATION.md`](./IMPLEMENTATION.md), and [`MODULE_GUIDE.md`](./MODULE_GUIDE.md).
The older scanner and Batch 5 reports below are dated historical snapshots, not
current code-status or release-readiness evidence.

> Exact model backend not verifiable from final document text alone.

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