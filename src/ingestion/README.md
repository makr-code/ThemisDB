# Ingestion Module

## Module Purpose

The Ingestion module is ThemisDB's data intake layer. It provides a unified pipeline for pulling documents from heterogeneous external sources — local filesystems, HuggingFace datasets, and generic REST APIs — normalizing them, and writing them into the database. It supports parallel multi-source ingestion, configurable retry with exponential back-off, token-bucket rate limiting, incremental checkpointing, a quarantine queue for bad records, and Prometheus metrics export.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `ingestion_manager.cpp` | Orchestrates all ingestion sources; IngestionBuilder and IngestionAdminApi |
| `api_connector.cpp` | Generic REST API connector with retry and rate limiting |
| `filesystem_ingester.cpp` | Directory walker for local filesystem ingestion |
| `huggingface_connector.cpp` | HuggingFace dataset connector |

## Scope

**In Scope:**
- Multi-source document ingestion (filesystem, HuggingFace, generic HTTP APIs)
- Paginated API traversal with exponential back-off retry
- Token-bucket rate limiting per source
- Incremental (checkpoint-based) ingestion to avoid re-processing
- Quarantine queue for documents that repeatedly fail ingestion
- Parallel ingestion across registered sources
- Prometheus-compatible text metrics export
- Dry-run mode for testing pipelines without database writes
- Admin API for source management (list, pause, resume, quarantine)
- Fluent builder API (`IngestionBuilder`) for pipeline construction

**Out of Scope:**
- Document parsing beyond plain text, HTML, and XML extraction (e.g., PDF, DOCX binary formats require external converters)
- Storage schema definition or index management (handled by the storage module)
- Authentication token refresh (callers must supply valid credentials)
- Full-text search indexing (handled by the query module)

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
                │       ├─ FILESYSTEM  → FileSystemIngester
                │       ├─ HUGGINGFACE → HuggingFaceConnector
                │       └─ API         → GenericApiConnector
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

**Current Status: Beta**

- HTTP client implementations are stubs (simulated responses); replace the `apiHttpGet` and `HttpClient` bodies with `libcurl` calls for production
- The filesystem ingester's binary-file detection relies on heuristics; add MIME type sniffing for more robust filtering
- Checkpoint files are written locally; for distributed deployments, back checkpoints with a shared storage backend
- Quarantine retry (`IngestionAdminApi::retryQuarantineItem`) performs per-document retry with exponential back-off when `raw_payload` is populated; falls back to re-running the originating source otherwise
- Known limitations:
  - `max_pages = 0` means unlimited pages; always set a sensible limit for untrusted APIs
  - Parallel ingestion uses `std::thread`; consider migrating to a work-stealing thread pool for better CPU utilization under high source counts
