> **Build:** `cmake --preset release && cmake --build build/release`

<!-- Status: current | validated: 2026-04-06 -->

# Ingestion Module — Public Headers

Multi-source ingestion pipeline for ThemisDB. Provides connectors, coordinators, and semantic validation interfaces.

## Header Listing

| Header | Purpose |
|--------|---------|
| `ingestion_manager.h` | Pipeline lifecycle |
| `ingestion_coordinator.h` | Worker coordination + checkpointing |
| `api_connector.h` | REST/GraphQL connector |
| `cdc_connector.h` | CDC connector |
| `kafka_connector.h` | Kafka topic ingestion |
| `database_connector.h` | Generic database connector |
| `s3_connector.h` | AWS S3 connector |
| `object_storage_connector.h` | Generic object storage |
| `filesystem_ingester.h` | File system + OCR |
| `huggingface_connector.h` | HuggingFace datasets |
| `web_crawler_connector.h` | Web crawler |
| `llm_adapter.h` | LLM enrichment adapter |
| `semantic_validator.h` | Semantic validation |
| `deontic_extractor.h` | Deontic obligation extraction |
| `agentic_reference_validator.h` | Agentic reference verification |
| `base_entity.h` | Base entity type for ingestion records |
| `builtin_step_factories.h` | Built-in ingestion step factory registry |
| `entity_assembler.h` | Assembles entities from extracted fields |
| `extraction_context.h` | Shared context for extraction steps |
| `file_manifest.h` | File manifest tracking for batch ingestion |
| `format_extractor.h` | Format-specific content extraction |
| `inference_backend.h` | Inference backend interface for ML enrichment |
| `ingestion_quality_judge.h` | Quality scoring and filtering for ingested records |
| `ingestion_sinks.h` | Output sink abstractions for ingested data |
| `ingestion_step.h` | Base interface for pipeline step implementations |
| `legal_domain.h` | Legal domain entity types and classifiers |
| `oauth_token_manager.h` | OAuth 2.0 token management for connectors |
| `workflow_engine.h` | Workflow orchestration engine for pipelines |

## Links

- Implementation: `../../src/ingestion/`
- ARCHITECTURE.md · AUDIT.md · CHANGELOG.md · ROADMAP.md · SECURITY.md

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "ingestion/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
