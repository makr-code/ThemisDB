> **Build (Linux example):** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: ../../src/ingestion/README.md · ../../src/ingestion/ROADMAP.md · ../../src/ingestion/FUTURE_ENHANCEMENTS.md · ../../docs/de/ingestion/README.md -->

# Ingestion Module — Public Headers

**Module Path:** `include/ingestion/`
**Implementation:** `../../src/ingestion/`
**Status:** ✅ Production Ready

Public C++ API for ThemisDB ingestion: source connectors, pipeline orchestration, retries/rate-limits, checkpointing, quarantine handling, and legal-text extraction components.

## Header Listing

| Header | Purpose |
|--------|---------|
| [`ingestion_manager.h`](ingestion_manager.h) | Core API (`IngestionManager`, `IngestionBuilder`, `IngestionAdminApi`, `SourceConfig`, `RetryConfig`, `RateLimitConfig`) |
| [`ingestion_coordinator.h`](ingestion_coordinator.h) | Multi-node/distributed coordinator (`IngestionCoordinator`, leader election, shared checkpoints) |
| [`api_connector.h`](api_connector.h) | Generic paginated REST ingestion source |
| [`filesystem_ingester.h`](filesystem_ingester.h) | Local filesystem ingestion with format handling/OCR/converter hooks |
| [`huggingface_connector.h`](huggingface_connector.h) | HuggingFace dataset ingestion source |
| [`kafka_connector.h`](kafka_connector.h) | Kafka topic ingestion source |
| [`database_connector.h`](database_connector.h) | JDBC/ODBC database ingestion source |
| [`s3_connector.h`](s3_connector.h) | S3-compatible object storage ingestion source |
| [`object_storage_connector.h`](object_storage_connector.h) | Generic object storage ingestion source |
| [`web_crawler_connector.h`](web_crawler_connector.h) | HTTP crawler + sitemap ingestion source |
| [`cdc_connector.h`](cdc_connector.h) | Change-data-capture ingestion source |
| [`llm_adapter.h`](llm_adapter.h) | LLM-assisted extraction adapter |
| [`deontic_extractor.h`](deontic_extractor.h) | Deontic logic extraction for legal text |
| [`semantic_validator.h`](semantic_validator.h) | Semantic quality validation |
| [`agentic_reference_validator.h`](agentic_reference_validator.h) | Reference extraction/validation |
| [`ingestion_step.h`](ingestion_step.h) | Workflow step interface |
| [`builtin_step_factories.h`](builtin_step_factories.h) | Built-in workflow step registry |
| [`workflow_engine.h`](workflow_engine.h) | Workflow orchestration and profile execution |
| [`ingestion_sinks.h`](ingestion_sinks.h) | Sink abstractions (`IDocWriter`, graph/vector/doc sinks) |
| [`entity_assembler.h`](entity_assembler.h) | Base-entity assembly and relation generation |
| [`base_entity.h`](base_entity.h) | Canonical ingestion entity model |
| [`extraction_context.h`](extraction_context.h) | Cross-step extraction context |
| [`file_manifest.h`](file_manifest.h) | Manifest/tracking metadata for batches |
| [`format_extractor.h`](format_extractor.h) | Format-specific extraction helpers |
| [`file_format.h`](file_format.h) | File format enum/types |
| [`ingestion_quality_judge.h`](ingestion_quality_judge.h) | Quality scoring policies |
| [`inference_backend.h`](inference_backend.h) | Backend abstraction for AI inference |
| [`legal_domain.h`](legal_domain.h) | Legal domain entity and classifier definitions |
| [`oauth_token_manager.h`](oauth_token_manager.h) | OAuth token management interfaces |

## Public API Entry Points

| API | Header | Runtime Role |
|-----|--------|--------------|
| `IngestionBuilder` | `ingestion_manager.h` | Fluent module entry-point to configure sources and build a manager |
| `IngestionManager` | `ingestion_manager.h` | Executes source ingestion, retries, rate-limits, checkpoints, quarantine |
| `IngestionAdminApi` | `ingestion_manager.h` | Runtime admin control (`list`, `pause`, `resume`, quarantine operations) |
| `IngestionCoordinator` | `ingestion_coordinator.h` | Distributed orchestration across worker nodes |
| `ISourceConnector` | `ingestion_manager.h` | Base interface implemented by all connectors |

## Configuration Quick Reference

### Core structs (`ingestion_manager.h`)

| Type | Key fields |
|------|------------|
| `SourceConfig` | `source_id`, `type`, `location`, `priority`, `enabled`, `options` |
| `RetryConfig` | `max_attempts`, `initial_delay_ms`, `backoff_factor`, `max_delay_ms`, `timeout_ms`, `max_quarantine_retries`, `ca_bundle_path` |
| `RateLimitConfig` | `requests_per_second`, `max_bytes_per_hour`, `enabled` |

### Frequently used `SourceConfig::options`

| Connector | Common options |
|-----------|----------------|
| API (`api_connector.h`) | `api_key`, `page_size`, `max_pages`, `cursor_param`, `pagination_mode`, `cursor_response_field`, `text_field`, `ca_bundle_path` |
| Filesystem (`filesystem_ingester.h`) | `extensions`, `recursive`, `ocr_enabled`, `ocr_language`, `pdf_converter`, `docx_converter` |
| HuggingFace (`huggingface_connector.h`) | `api_key`, `split`, `text_field`, `page_size`, `max_pages`, `ca_bundle_path` |
| Web crawler (`web_crawler_connector.h`) | `max_depth`, `max_pages`, `user_agent`, `follow_sitemaps`, `respect_robots`, `same_domain_only` |

## Runtime Behavior, Errors, and Limits

- Sources run sequentially or in parallel depending on manager configuration.
- Incremental ingestion uses checkpoints to avoid re-processing already ingested ranges.
- Persistent per-document failures are moved into quarantine for later retry.
- Error handling is normalized via `IngestionError`, `IngestionErrorCode`, and `IngestionErrorSeverity`.
- `max_pages = 0` means unlimited pagination/crawling; set explicit limits for untrusted or large sources.
- Connector and source availability is exposed through `isAvailable()` checks.

## Usage

```cpp
#include "ingestion/ingestion_manager.h"

using namespace themis::ingestion;

auto manager = IngestionBuilder("rocksdb://./data")
    // Storage URI consumed by the underlying document store backend.
    .withFilesystemSource("local", "/data/docs", {{"extensions", ".txt,.md"}}, 1)
    .withRetryConfig({.max_attempts = 3, .initial_delay_ms = 500, .backoff_factor = 2.0})
    .withRateLimitConfig({.requests_per_second = 10.0, .enabled = true})
    .build();

IngestionReport report = manager->ingestAll(nullptr);
```

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Troubleshooting

### `FILE_NOT_FOUND` / source unavailable
Check `SourceConfig::location` and connector-specific credentials/options. Use `IngestionAdminApi::listSources()` to inspect source availability.

### API/HuggingFace returns `HTTP_UNAUTHORIZED`
Set `options["api_key"]` or configure OAuth refresh in `GenericApiConnector` and verify token scope/expiry.

### Very long crawl/API runs
Set `options["max_pages"]` and (for web) `options["max_depth"]`; `0` means unlimited and can lead to unbounded runtimes.

### PDF/DOCX files are skipped
Install external converters (`pdftotext`, `pandoc`) or configure `pdf_converter` / `docx_converter` in `SourceConfig::options`.

## Documentation Links

- Module overview: [`../../src/ingestion/README.md`](../../src/ingestion/README.md)
- Architecture: [`../../src/ingestion/ARCHITECTURE.md`](../../src/ingestion/ARCHITECTURE.md)
- Roadmap: [`../../src/ingestion/ROADMAP.md`](../../src/ingestion/ROADMAP.md)
- Future enhancements: [`../../src/ingestion/FUTURE_ENHANCEMENTS.md`](../../src/ingestion/FUTURE_ENHANCEMENTS.md)
- Security notes: [`../../src/ingestion/SECURITY.md`](../../src/ingestion/SECURITY.md)
- German docs overview: [`../../docs/de/ingestion/README.md`](../../docs/de/ingestion/README.md)
