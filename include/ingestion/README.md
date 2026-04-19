> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

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
