> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/ingestion/ARCHITECTURE.md -->

# Ingestion Module — Public Header Architecture

**Module Path:** `include/ingestion/`  
**Implementation:** `../../src/ingestion/`  
**Canonical architecture doc:** [`../../src/ingestion/ARCHITECTURE.md`](../../src/ingestion/ARCHITECTURE.md)

---

## 1. Overview

`include/ingestion/` defines the **public multi-connector ingestion pipeline (API, CDC, database, filesystem, HuggingFace, Kafka, S3, web crawler), entity assembly, semantic validation, and workflow orchestration API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/ingestion/ARCHITECTURE.md`](../../src/ingestion/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Ingestion

| Header | Public Type | Purpose |
|--------|------------|---------|
| `ingestion_manager.h` | `IngestionManager` | Primary ingestion lifecycle manager |
| `ingestion_coordinator.h` | `IngestionCoordinator` | Multi-source ingestion coordination |
| `ingestion_step.h` | `IIngestionStep` | Composable ingestion step interface |
| `builtin_step_factories.h` | `BuiltinStepFactories` | Factory for built-in step implementations |
| `workflow_engine.h` | `WorkflowEngine` | DAG-based ingestion workflow engine |
| `ingestion_sinks.h` | `IngestionSinks` | Output sink registry |
### 2.2 Connectors

| Header | Public Type | Purpose |
|--------|------------|---------|
| `api_connector.h` | `APIConnector` | REST/GraphQL API ingestion connector |
| `cdc_connector.h` | `CDCConnector` | CDC-based streaming connector |
| `database_connector.h` | `DatabaseConnector` | Generic database connector |
| `filesystem_ingester.h` | `FilesystemIngester` | Local/network filesystem ingestion |
| `huggingface_connector.h` | `HuggingFaceConnector` | HuggingFace dataset connector |
| `kafka_connector.h` | `KafkaConnector` | Kafka topic connector |
| `s3_connector.h` | `S3Connector` | S3-compatible object storage connector |
| `object_storage_connector.h` | `ObjectStorageConnector` | Generic object storage connector |
| `web_crawler_connector.h` | `WebCrawlerConnector` | Web crawler and scraper connector |
### 2.3 Entity and Schema

| Header | Public Type | Purpose |
|--------|------------|---------|
| `entity_assembler.h` | `EntityAssembler` | Multi-source entity assembly |
| `base_entity.h` | `BaseEntity` | Base entity type for ingestion records |
| `extraction_context.h` | `ExtractionContext` | Field extraction context and state |
| `file_format.h` | `FileFormat` | File format detection and routing |
| `file_manifest.h` | `FileManifest` | Batch file manifest management |
| `format_extractor.h` | `FormatExtractor` | Format-specific field extraction |
| `deontic_extractor.h` | `DeonticExtractor` | Legal/deontic clause extraction |
| `legal_domain.h` | `LegalDomain` | Legal domain entity and concept registry |
### 2.4 Quality and Validation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `semantic_validator.h` | `SemanticValidator` | Semantic consistency validation |
| `ingestion_quality_judge.h` | `IngestionQualityJudge` | LLM-based quality scoring |
| `agentic_reference_validator.h` | `AgenticReferenceValidator` | Agentic reference and citation validator |
### 2.5 Auth and Inference

| Header | Public Type | Purpose |
|--------|------------|---------|
| `oauth_token_manager.h` | `OAuthTokenManager` | OAuth 2.0 token lifecycle for connectors |
| `inference_backend.h` | `IIngestionInferenceBackend` | LLM inference backend for ingestion |
| `llm_adapter.h` | `IngestionLLMAdapter` | LLM adapter for ingestion enrichment |

---

## 3. Namespace Layout

All public types reside in the `themis::ingestion` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/ingestion/` expose the **stable public API**; internal types live in `src/ingestion/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **ANN Frontdoor**.
