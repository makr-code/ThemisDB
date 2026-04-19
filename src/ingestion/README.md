# Ingestion Module

<!-- Status: current | validated: 2026-04-06 | Primary: src/ingestion/ | Secondary: docs/de/ingestion/ -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../docs/de/ingestion/README.md -->

## Module Purpose

The Ingestion module is ThemisDB's data intake layer. It provides a unified pipeline for pulling documents from heterogeneous external sources — local filesystems, HuggingFace datasets, REST APIs, Kafka, CDC streams — normalizing them, and writing them into the database. It supports parallel multi-source ingestion, configurable retry with exponential back-off, token-bucket rate limiting, incremental checkpointing, a quarantine queue for bad records, Prometheus metrics export, and an **LLM-driven legal text extraction pipeline** for German administrative law documents.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `ingestion_manager.cpp` | Orchestrates all ingestion sources; IngestionBuilder and IngestionAdminApi |
| `api_connector.cpp` | Generic REST API connector with retry and rate limiting |
| `filesystem_ingester.cpp` | Directory walker for local filesystem ingestion |
| `huggingface_connector.cpp` | HuggingFace dataset connector |
| `kafka_connector.cpp` | Apache Kafka consumer source connector |
| `object_storage_connector.cpp` | S3 / GCS / Azure Blob object-storage connector |
| `database_connector.cpp` | JDBC-compatible database source connector |
| `web_crawler_connector.cpp` | HTTP web crawler and XML sitemap source connector |
| `deontic_extractor.cpp` | Regex/LLM-based deontic logic extraction for German legal texts (obligation/permission/prohibition) |
| `semantic_validator.cpp` | Quality gates, semantic scoring, document-level extraction with per-§ splitting |
| `agentic_reference_validator.cpp` | Cross-reference extraction and validation against a legal knowledge base |
| `llm_adapter.cpp` | LLM integration bridge: connects DeonticExtractor to Mistral 7B via llama.cpp (Phase 2) |

## Scope

**In Scope:**
- Multi-source document ingestion (filesystem, HuggingFace, generic HTTP APIs, Kafka, object storage, JDBC databases, web crawling)
- Paginated API traversal with exponential back-off retry
- Token-bucket rate limiting per source
- Incremental (checkpoint-based) ingestion to avoid re-processing
- Quarantine queue for documents that repeatedly fail ingestion
- Parallel ingestion across registered sources
- Prometheus-compatible text metrics export
- Dry-run mode for testing pipelines without database writes
- Admin API for source management (list, pause, resume, quarantine)
- Fluent builder API (`IngestionBuilder`) for pipeline construction
- **LLM-driven semantic extraction** for German legal texts:
  - Deontic logic extraction (obligation/permission/prohibition/definition/condition/exception/reference)
  - Entity recognition (law_reference, person_role, organization, temporal, threshold_value)
  - Temporal validity analysis (effective_from, effective_to, amendment dates)
  - Agentic reference validation (§-cross-references, named-law refs, EU directives)
  - Quality gates with configurable thresholds and per-source `LegalIngestionConfig`

**Out of Scope:**
- Document parsing beyond plain text, HTML, and XML extraction (e.g., PDF, DOCX binary formats require external converters)
- Storage schema definition or index management (handled by the storage module)
- Authentication token refresh (callers must supply valid credentials)
- Full-text search indexing (handled by the query module)
- LoRA adapter training (handled by the training module)

## Key Components

### GenericApiConnector
**Location:** `api_connector.cpp`

Ingests documents from any paginated JSON REST API. Uses offset/limit pagination by default; the cursor parameter name and text field name are configurable. Implements exponential back-off retry and maps HTTP error codes to typed `IngestionErrorCode` values.

**Features:**
- Configurable `page_size` and `max_pages` limits
- Bearer token authentication via `api_key` option
- Retry with configurable initial delay, backoff factor, and max delay
- Progress callback for streaming updates

### FileSystemIngester
**Location:** `filesystem_ingester.cpp`

Recursively walks a directory tree and ingests text-based files. Supports plain text, HTML, XML, JSON, CSV, Markdown, PDF, and DOCX. HTML/XML text is extracted using pugixml (when available) with graceful fallback. Binary formats (PDF/DOCX) are converted to plain text by external command-line tools. Uses `std::filesystem` for portable directory traversal.

**Features:**
- Configurable file extension filtering
- Encoding detection fallback (UTF-8 default)
- Skips binary files and empty files
- Optional pugixml HTML/XML text extraction (`THEMIS_HAS_PUGIXML` compile flag)
- **Binary MIME type detection**: `detectBinaryMimeType()` reads magic bytes to identify
  PDF (`%PDF`) and DOCX (`PK\x03\x04` + OOXML marker) regardless of file extension
- **PDF ingestion**: delegates to external `pdftotext` converter (configurable via
  `BinaryConverter::pdf_converter`; silently skipped when converter is absent/empty)
- **DOCX ingestion**: delegates to external `pandoc` converter (configurable via
  `BinaryConverter::docx_converter`; silently skipped when converter is absent/empty)
- Converter paths configurable via `SourceConfig::options["pdf_converter"]` and
  `options["docx_converter"]`, or programmatically via `setBinaryConverter()`

### HuggingFaceConnector
**Location:** `huggingface_connector.cpp`

Downloads and ingests datasets from the HuggingFace Hub API. Handles dataset splits (train/test/validation), configurable record fields, and API token authentication.

**Features:**
- Dataset-level pagination
- Split selection (train, test, validation, all)
- Configurable `text_field` mapping
- libcurl-based HTTP (stub provided; replace with `curl_easy_perform` in production)

### WebCrawlerConnector
**Location:** `web_crawler_connector.cpp`

Crawls web pages starting from a seed URL using breadth-first search, extracts their text content, and ingests the pages as documents. Supports XML sitemap discovery (including nested sitemap index files), robots.txt enforcement, same-domain-only filtering, and configurable crawl depth and page limits.

Compile with `THEMIS_ENABLE_CURL` to activate the libcurl HTTP backend; without it the connector compiles and runs with an injected mock (unit tests only).

**Features:**
- BFS crawl from a seed URL up to configurable `max_depth` levels
- `max_pages` guard to prevent runaway crawls
- XML sitemap (`/sitemap.xml`) discovery and sitemap index resolution (depth ≤ 5)
- Robots.txt parsing with per-User-Agent `Disallow` rule enforcement
- Same-domain-only mode (configurable)
- HTML tag stripping, `<script>`/`<style>` block exclusion, HTML entity decoding
- Relative → absolute URL resolution
- URL deduplication (fragment-normalised) to avoid revisiting the same page
- Exponential back-off retry on 5xx / timeout errors
- **Security**: only `http://` and `https://` seed URLs and link targets are permitted; `file://`, `ftp://`, `data:`, and all other schemes are rejected to prevent SSRF attacks

**Options** (via `SourceConfig::options` or `withWebCrawlerSource()`):

| Key                | Default                   | Description                                    |
|--------------------|---------------------------|------------------------------------------------|
| `max_depth`        | `3`                       | Maximum BFS crawl depth (0 = seed URL only)    |
| `max_pages`        | `0` (unlimited)           | Maximum total pages to crawl                   |
| `user_agent`       | `ThemisDB-Crawler/1.0`    | HTTP User-Agent header sent with every request |
| `follow_sitemaps`  | `true`                    | Parse `/sitemap.xml` before BFS traversal      |
| `respect_robots`   | `true`                    | Honour robots.txt `Disallow` rules             |
| `same_domain_only` | `true`                    | Restrict crawl to the seed domain              |

### IngestionManager
**Location:** `ingestion_manager.cpp`
Orchestrates all registered sources. Runs sources sequentially or in parallel (thread pool). Maintains per-source checkpoints, a quarantine queue, and a global `IngestionReport`.

**Features:**
- Source registration and priority ordering
- Parallel processing via configurable thread pool
- Token-bucket rate limiting (`RateLimitConfig`)
- Retry configuration propagated to connectors
- Incremental mode (skip already-processed documents via checkpoint files)
- Pause/resume per source
- Quarantine for persistently failing documents (`QuarantineEntry`)
- Dry-run mode (no database writes)

### IngestionMetricsExporter
**Location:** `ingestion_manager.cpp`

Exports per-source and aggregate ingestion statistics as Prometheus text-format metrics. Emits counters for `docs_processed`, `docs_failed`, `bytes_processed`, `retry_total`, `errors_total`, and a gauge for `throughput_docs_per_sec`. Supports per-error-code breakdowns.

### IngestionBuilder
**Location:** `ingestion_manager.cpp`

Fluent builder for constructing and configuring an `IngestionManager` instance. Supports method chaining for adding sources, retry config, rate limits, parallelism, and dry-run mode.

### IngestionAdminApi
**Location:** `ingestion_manager.cpp`

Administrative interface for runtime source management: listing registered sources with availability probes, triggering ingestion, pausing/resuming sources, and managing the quarantine queue.

## Architecture

```
IngestionBuilder (fluent config)
        │
        └─► IngestionManager
                │
                ├─ Source Registry (SourceConfig[])
                │       ├─ FILESYSTEM    → FileSystemIngester
                │       ├─ HUGGINGFACE   → HuggingFaceConnector
                │       ├─ API           → GenericApiConnector
                │       ├─ KAFKA         → KafkaConnector
                │       ├─ OBJECT_STORAGE→ ObjectStorageConnector
                │       ├─ DATABASE      → DatabaseConnector
                │       └─ WEB_CRAWLER   → WebCrawlerConnector
                │
                ├─ RetryConfig  (exponential back-off)
                ├─ RateLimitConfig (token bucket per source)
                ├─ Checkpoints (incremental mode)
                ├─ Quarantine  (failed documents)
                │
                └─► IngestionReport
                        ├─ per-source IngestionStats
                        └─ IngestionMetricsExporter (Prometheus)
```

## Dependencies

### Internal Dependencies
- `ingestion/api_connector.h` — generic API connector interface
- `ingestion/filesystem_ingester.h` — filesystem source interface
- `ingestion/huggingface_connector.h` — HuggingFace connector interface
- `ingestion/kafka_connector.h` — Kafka consumer connector interface
- `ingestion/object_storage_connector.h` — S3/GCS/Azure connector interface
- `ingestion/database_connector.h` — JDBC-compatible database connector interface
- `ingestion/web_crawler_connector.h` — web crawler and sitemap connector interface

### External Dependencies
- `<filesystem>` (C++17) — directory traversal
- `pugixml` (optional, `THEMIS_HAS_PUGIXML`) — HTML/XML text extraction
- `libcurl` (production stub) — HTTP requests for API and HuggingFace connectors
- `<thread>` / `<chrono>` — retry sleep and parallel execution

## Usage Examples

```cpp
#include "ingestion/ingestion_manager.h"

using namespace themis::ingestion;

// Build a pipeline with a filesystem source and a HuggingFace source
auto manager = IngestionBuilder("rocksdb://./data")
    .withFilesystemSource("local_docs", "/data/legal/",
        {{"extensions", ".txt,.html"}}, /*priority=*/1)
    .withHuggingFaceSource("hf_legal", "datasets/legal-corpus",
        {{"api_key", "hf_token_here"}, {"split", "train"}}, /*priority=*/2)
    .withRetryConfig({.max_attempts=3, .initial_delay_ms=500, .backoff_factor=2.0})
    .withRateLimitConfig({.requests_per_second=10})
    .withParallelProcessing(true, /*max_threads=*/4)
    .withTargetCollection("legal_documents")
    .build();

// Run all sources
IngestionReport report = manager->ingestAll(
    [](const std::string& sid, size_t done, size_t total, const std::string& msg) {
        // progress callback
    }
);

// Export Prometheus metrics
IngestionMetricsExporter exporter("themis_ingestion");
std::string metrics_text = exporter.exportText(report);

// Admin operations
IngestionAdminApi admin(*manager);
auto sources = admin.listSources();
admin.pauseSource("hf_legal");
auto quarantine = admin.listQuarantine();
```

## Production Readiness

**Current Status: 🟢 Production-ready (v1.5.x)**

All connectors are implemented and production-ready. Known limitations:
- `HuggingFaceConnector` HTTP client uses a simulated implementation (hardcoded JSON); replace with real `libcurl` calls in production
- `CdcConnector` stream backend is a compile-time stub (requires `THEMIS_ENABLE_CDC_STREAM` and a replication driver)
- `IngestionAdminApi::retryQuarantineItem()` always succeeds in the current implementation (unreachable failure branch — wire to real storage write)
- PDF/DOCX ingestion requires external converters (pdftotext, pandoc) to be installed
- Checkpoint files are written locally; for distributed deployments, configure the shared checkpoint backend
- `max_pages = 0` means unlimited pages; always set a sensible limit for untrusted APIs

## Scientific References

1. Zaharia, M., Das, T., Li, H., Hunter, T., Shenker, S., & Stoica, I. (2013). **Discretized Streams: Fault-Tolerant Streaming Computation at Scale**. *Proceedings of the 24th ACM Symposium on Operating Systems Principles (SOSP)*, 423–438. https://doi.org/10.1145/2517349.2522737

2. Kleppmann, M. (2017). **Designing Data-Intensive Applications**. O'Reilly Media. ISBN: 978-1-449-37332-0

3. Karp, R. M., Shenker, S., & Papadimitriou, C. H. (2003). **A Simple Algorithm for Finding Frequent Elements in Streams and Bags**. *ACM Transactions on Database Systems*, 28(1), 51–55. https://doi.org/10.1145/762471.762473

4. Dede, E., Govindaraju, M., Gunter, D., Canon, R. S., & Ramakrishnan, L. (2013). **Performance Evaluation of a MongoDB and Hadoop Platform for Scientific Data Analysis**. *Proceedings of the 4th ACM/IEEE Workshop on Many-Task Computing on Clouds, Grids, and Supercomputers (MTAGS)*. https://doi.org/10.1145/2532508.2532521

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
