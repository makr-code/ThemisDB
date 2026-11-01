> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/ingestion/ARCHITECTURE.md -->

# Ingestion Module — Public Header Architecture

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

**Module Path:** `include/ingestion/`  
**Implementation:** `../../src/ingestion/`  
**Canonical architecture doc:** [`../../src/ingestion/ARCHITECTURE.md`](../../src/ingestion/ARCHITECTURE.md)

---

## 1. Overview

`include/ingestion/` declares C++ interfaces for connector intake, source
management, validation, ordered workflow steps, optional worker coordination,
entity assembly, and output adapters. A public type or option does not imply
support by every connector or automatic use by every runtime path.

For runtime composition and implementation internals see:
→ [`../../src/ingestion/ARCHITECTURE.md`](../../src/ingestion/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Ingestion

| Header | Public Type | Purpose |
|--------|------------|---------|
| `ingestion_manager.h` | `IngestionManager`, `IngestionBuilder`, `IngestionAdminApi`, `ISourceConnector` | Source lifecycle, direct execution, in-process admin wrapper |
| `ingestion_coordinator.h` | `IngestionCoordinator` and worker/checkpoint interfaces | Source partitioning and worker dispatch; durable/shared stores are injectable |
| `ingestion_step.h` | `IIngestionStep` | Composable ingestion step interface |
| `builtin_step_factories.h` | `BuiltinStepFactories` | Factory for built-in step implementations |
| `workflow_engine.h` | `WorkflowEngine`, `StepRegistry` | Ordered steps over `ExtractionContext`; no DAG contract is asserted |
| `ingestion_sinks.h` | `IGraphWriter`, `IVectorWriter`, document/tensor sink interfaces | Output adapters; no universal sink registry is asserted |
### 2.2 Connectors

| Header | Public Type | Purpose |
|--------|------------|---------|
| `api_connector.h` | API connector types | API source connector; protocol details are implementation-specific |
| `cdc_connector.h` | `CDCConnector` | CDC-based streaming connector |
| `database_connector.h` | `DatabaseConnector` | Database source connector; backend/protocol support must be verified |
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
| `ingestion_quality_judge.h` | `IngestionQualityJudge` | Optional quality scoring; inspect backend availability and evaluated scores |
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

## 4. Runtime and contract notes

- Headers in `include/ingestion/` declare public surfaces; this page does not certify ABI stability or production readiness.
- The manager's filesystem workflow branch currently increments processed counts after successful workflow execution but does not pass the assembled result to a sink.
- Coordinator defaults for shared checkpoints and leader election are in-memory; multi-process or multi-host guarantees require injected implementations.
- `IngestionAdminApi` is an in-process wrapper, not an HTTP endpoint or authorization layer.
- Three headers declare `themis::ingestion::IngestionErrorCode` with differing declarations (`ingestion_manager.h`, `ingestion_api_contract.h`, `ingestion_error_contract.h`). Including competing declarations in one translation unit is a source-contract conflict requiring reconciliation.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- TODO: verify any module-layer association against a current architecture source before documenting one.
