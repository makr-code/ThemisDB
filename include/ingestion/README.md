> **Build (Linux example):** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: ../../src/ingestion/README.md · ../../src/ingestion/ROADMAP.md · ../../src/ingestion/FUTURE_ENHANCEMENTS.md · ../../docs/de/ingestion/README.md -->

# Ingestion Module — Public Headers

**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot
**Human_Review:** false
**Human_Author:** ThemisDB Team

> Exact model backend not verifiable from final document text alone.

**Module Path:** `include/ingestion/`
**Implementation:** `../../src/ingestion/`
**Status:** Public declarations exist; deployment readiness is caller-specific.

Public C++ declarations for ingestion source management, connectors, workflow
steps, validation, coordination, and sink adapters. The declarations do not
establish that every connector uses every control or that every execution path
persists output. See the implementation guide for the observed runtime limits.

## Header Listing

| Header | Purpose |
|--------|---------|
| [`ingestion_manager.h`](ingestion_manager.h) | Core API (`IngestionManager`, `IngestionBuilder`, `IngestionAdminApi`, `SourceConfig`, `RetryConfig`, `RateLimitConfig`) |
| [`ingestion_coordinator.h`](ingestion_coordinator.h) | Source partitioning/worker coordinator; checkpoint and leader election are injectable |
| [`api_connector.h`](api_connector.h) | API source connector; protocol and options are implementation-specific |
| [`filesystem_ingester.h`](filesystem_ingester.h) | Filesystem connector and file processing hooks |
| [`huggingface_connector.h`](huggingface_connector.h) | HuggingFace dataset ingestion source |
| [`kafka_connector.h`](kafka_connector.h) | Kafka topic ingestion source |
| [`database_connector.h`](database_connector.h) | Database connector; do not infer JDBC/ODBC support without backend evidence |
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
| [`ingestion_api_contract.h`](ingestion_api_contract.h), [`ingestion_error_contract.h`](ingestion_error_contract.h) | Versioned write-path/error contract declarations; check include conflicts and the interface actually used |
| [`reindex_decision_engine.h`](reindex_decision_engine.h), [`canary_deployment_manager.h`](canary_deployment_manager.h) | Reindex decision and canary helper APIs; not proof of integrated index migration |

## Public API Entry Points

| API | Header | Runtime Role |
|-----|--------|--------------|
| `IngestionBuilder` | `ingestion_manager.h` | Fluent module entry-point to configure sources and build a manager |
| `IngestionManager` | `ingestion_manager.h` | Executes source ingestion, retries, rate-limits, checkpoints, quarantine |
| `IngestionAdminApi` | `ingestion_manager.h` | Runtime admin control (`list`, `pause`, `resume`, quarantine operations) |
| `IngestionCoordinator` | `ingestion_coordinator.h` | Source partitioning, worker dispatch, and report aggregation |
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

- Source execution and option support vary by connector; check its header and implementation.
- Checkpoint behavior depends on the selected source and checkpoint implementation; coordinator defaults are in-memory.
- Per-document failures may be quarantined according to the configured path.
- `IngestionManager`, `ingestion_api_contract.h`, and `ingestion_error_contract.h` declare conflicting `IngestionErrorCode` names in the same namespace; do not include the competing declarations together until reconciled.
- Several connector limits use zero as unlimited; verify the specific option and set explicit bounds for untrusted or large sources.
- Connector and source availability is exposed through `isAvailable()` checks.

## Usage

```cpp
#include "ingestion/ingestion_manager.h"

using namespace themis::ingestion;

// IngestionBuilder delegates this storage URI to the document-store backend.
auto manager = IngestionBuilder("rocksdb://./data")
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
Check the selected connector's documented credential option and token scope/expiry. OAuth refresh is connector-specific; do not assume a generic connector implementation.

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

The filesystem workflow branch currently reports successfully executed files as
processed without passing its assembled result to a sink. A positive processed
count on that path is not a persistence guarantee. The in-process admin API also
does not provide HTTP authentication or authorization.
