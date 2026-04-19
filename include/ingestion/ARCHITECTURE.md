<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/ingestion/ -->

# Ingestion Module — Public Header Architecture
**Version:** 1.5.1
**Module Path:** `include/ingestion/`
**Implementation:** `../../src/ingestion/`

---

## Overview

The Ingestion module provides public headers for multi-source data ingestion pipelines: API connectors, CDC, Kafka, S3, HuggingFace, filesystem, web crawling, database, and semantic/deontic validation. An `IngestionCoordinator` orchestrates workers across sources.

## Design Principles

- **Source Abstraction** — All connectors implement `ISourceConnector`.
- **Semantic Validation** — `SemanticValidator` and `DeonticExtractor` apply NLP-based validation before storage.
- **LLM Pipeline** — `LlmAdapter` bridges ingestion with LLM enrichment (legal document processing).
- **Agentic Verification** — `AgenticReferenceValidator` performs agentic loop verification of ingested references.

## Interface Inventory

| Header | Classes / Structs | Purpose |
|--------|-------------------|---------|
| `ingestion_manager.h` | — | Ingestion pipeline lifecycle management |
| `ingestion_coordinator.h` | `IIngestionWorkerNode`, `InMemorySharedCheckpointStore` | Worker coordination and checkpointing |
| `api_connector.h` | `GenericApiConnector` | REST/GraphQL API source connector |
| `cdc_connector.h` | `CdcConnector` | Change-data-capture connector |
| `kafka_connector.h` | — | Kafka topic ingestion |
| `database_connector.h` | `DatabaseConnector` | Generic database connector |
| `s3_connector.h` | — | AWS S3 object ingestion |
| `object_storage_connector.h` | — | Generic object storage |
| `filesystem_ingester.h` | `FileSystemIngester` | File system ingestion with OCR |
| `huggingface_connector.h` | `HuggingFaceConnector` | HuggingFace datasets ingestion |
| `web_crawler_connector.h` | — | Web crawler ingestion |
| `llm_adapter.h` | — | LLM enrichment adapter |
| `semantic_validator.h` | — | Semantic validation of ingested documents |
| `deontic_extractor.h` | `DeonticExtractor`, `DeonticObligation`, `DeonticExtraction` | Deontic obligation extraction |
| `agentic_reference_validator.h` | `AgenticReferenceValidator`, `ReferenceValidationReport` | Agentic reference validation loop |

## References

- Implementation: `../../src/ingestion/`
- LLM adapter tests: `tests/test_ingestion_llm_adapter.cpp`
