# Ingestion Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-03-09 | Primary: src/ingestion/ | Secondary: docs/de/ingestion/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../docs/de/ingestion/README.md -->

## Current Status
v1.5.x – Production-grade data intake layer. All connectors (FileSystem, HuggingFace, GenericAPI, Kafka, ObjectStorage, Database, WebCrawler, CDC) implemented and production-ready. Distributed ingestion coordinator with work-stealing pool available. Dynamic source reconfiguration, schema validation, and Plugin API complete. `HuggingFaceConnector` HTTP client still uses a simulated implementation (see Known Issues).

## Completed ✅
- [x] GenericApiConnector – paginated JSON REST API ingestion with exponential back-off; real `curl_easy_perform` (PR #1915)
- [x] FileSystemIngester – recursive directory walk with HTML/XML extraction (pugixml)
- [x] HuggingFaceConnector – dataset split ingestion with API token auth (HTTP stub, see Known Issues)
- [x] IngestionManager – parallel multi-source orchestration with thread pool
- [x] Token-bucket rate limiting per source
- [x] Incremental checkpoint-based ingestion (skip re-processing)
- [x] Quarantine queue for persistently failing documents
- [x] Pause/resume per-source control
- [x] Dry-run mode (no database writes)
- [x] Prometheus-compatible metrics export (docs_processed, errors, throughput, etc.)
- [x] Admin API (list, pause, resume, quarantine)
- [x] Fluent IngestionBuilder API
- [x] Cursor-based pagination support alongside offset/limit (Issue: #2409)
- [x] PDF and DOCX binary format ingestion via external converters (Issue: #1889)
- [x] S3 / GCS / Azure Blob object-storage source connector (`ingestion/object_storage_connector.cpp`) (Issue: #1905)
  - Providers: AWS S3, GCS, Azure Blob (compile-time SDK flags); mock-injection path for unit tests
  - 28 unit tests; path-traversal protection; JSON `text_field` extraction; credentials never logged
- [x] Kafka consumer source connector (`ingestion/kafka_connector.cpp`) (Issue: #1904)
- [x] JDBC-compatible database source connector (`ingestion/database_connector.cpp`) (Issue: #1894)
- [x] Web crawler / sitemap ingestion source (`ingestion/web_crawler_connector.cpp`) (Issue: #1895)
  - BFS crawl, sitemap, robots.txt, SSRF prevention (http/https only)
- [x] Per-source schema validation before write (`IngestionBuilder::withSchemaValidation`) (Issue: #1896)
- [x] Distributed ingestion coordinator across nodes (`ingestion/ingestion_coordinator.cpp`, work-stealing thread pool) (Issue: #1897, #1906)
- [x] Dynamic source reconfiguration without restart (`IngestionManager::reconfigureSource`) (Issue: #1900)
- [x] Change-data-capture (CDC) source for live database streams (`ingestion/cdc_connector.cpp`) (Issue: #2199; stream backend gated behind `THEMIS_ENABLE_CDC_STREAM` — see Known Issues)
- [x] Plugin API for third-party source connectors (`ingestion_manager.h`: `ConnectorPluginRegistry`, `IngestionManager::registerConnectorPlugin`, `IngestionBuilder::withPluginSource`) (Issue: #1908)
- [x] Per-document quarantine retry with exponential back-off (Issue: #1916)
- [x] Binary MIME type detection (PDF, DOCX) before dispatch to converters (Issue: #1917)
- [x] OAuth 2.0 token refresh handling within connectors (Issue: #2408)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Remaining
- [I] End-to-end ingestion lineage tracking (Issue: #1901)
- [I] Unit tests coverage > 80% (Issue: #1909)
- [I] Integration tests (filesystem, HuggingFace, generic API) (Issue: #1910)
- [I] Performance benchmarks (docs/sec, MB/sec) (Issue: #1911)

## Implementation Phases

### Phase 1: Core Ingestion Infrastructure (Status: Completed ✅)
- [x] FileSystemIngester – recursive directory walk with HTML/XML extraction (pugixml)
- [x] GenericApiConnector – paginated JSON REST with exponential back-off
- [x] HuggingFaceConnector – dataset split ingestion with API token auth
- [x] IngestionManager – parallel multi-source orchestration with thread pool
- [x] IngestionBuilder – fluent configuration API
- [x] IngestionAdminApi – list, pause, resume, quarantine endpoints
- [x] Token-bucket rate limiting per source
- [x] Incremental checkpoint-based ingestion (skip re-processing)
- [x] Quarantine queue for persistently failing documents
- [x] Prometheus-compatible metrics (docs_processed, errors, throughput)

### Phase 2: HTTP Hardening & Binary Formats (Status: Completed ✅)
- [x] Replace libcurl stubs with real `curl_easy_perform` calls in `api_connector.cpp` (Issue: #1915, PR #1915)
- [x] Per-document quarantine retry with exponential back-off (Issue: #1916)
- [x] Binary MIME type detection (PDF, DOCX) before dispatch to converters (Issue: #1917)
- [x] OAuth 2.0 token refresh handling within connectors (Issue: #2408)
- [x] Cursor-based pagination support alongside offset/limit

### Phase 3: Distributed Sources & Connectors (Status: Completed ✅)
- [x] Kafka consumer source connector (`ingestion/kafka_connector.cpp`) (Issue: #1904)
- [x] S3 / GCS / Azure Blob object-storage source connector (`ingestion/object_storage_connector.cpp`) (Issue: #1905)
  - Providers: AWS S3 (`THEMIS_ENABLE_S3`), GCS (`THEMIS_ENABLE_GCS`), Azure Blob (`THEMIS_ENABLE_AZURE`); graceful `CONNECTOR_NOT_SUPPORTED` when no SDK is compiled
  - Features: prefix filtering, `max_keys` cap, JSON `text_field` extraction, path-traversal rejection, `RetryConfig` passthrough, throughput metrics
  - Test coverage: 28 unit tests via mock-injection in `tests/test_ingestion_object_storage.cpp` (no cloud credentials required)
  - Security: `..` path components rejected; credentials never logged
- [x] Distributed ingestion coordinator across nodes with work-stealing thread pool (`ingestion/ingestion_coordinator.cpp`) (Issue: #1906)
- [x] Change-data-capture (CDC) source for live database streams (`ingestion/cdc_connector.cpp`) (Issue: #2199)
- [x] JDBC-compatible database source connector (`ingestion/database_connector.cpp`) (Issue: #1894)
- [x] Web crawler / sitemap ingestion source (`ingestion/web_crawler_connector.cpp`) (Issue: #1895)
- [x] Per-source schema validation before write (Issue: #1896)
- [x] Dynamic source reconfiguration without restart (Issue: #1900)
- [x] Plugin API for third-party source connectors (Issue: #1908)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1909)
- [I] Integration tests (filesystem, HuggingFace, generic API) (Issue: #1910)
- [I] Performance benchmarks (docs/sec, MB/sec) (Issue: #1911)
- [x] Security audit (path traversal, API key storage) (Issue: #1912)
- [x] Documentation complete (Issue: #1913)
- [I] API stability guaranteed (Issue: #1914)

## Known Issues & Limitations
- `HuggingFaceConnector`: HTTP client uses a simulated implementation (hardcoded JSON response); replace with real `curl_easy_perform` calls in a follow-up PR. Note: `GenericApiConnector` already uses real `curl_easy_perform` (PR #1915).
- `CdcConnector`: `ingestFromStream()` is a compile-time gated stub; full replication driver integration (PostgreSQL libpq-fe.h or MySQL Connector/C) is a follow-up task (requires `THEMIS_ENABLE_CDC_STREAM`).
- `IngestionAdminApi::retryQuarantineItem()`: write-success is always `true` in the current implementation (unreachable failure branch); real storage-write result must be wired in.
- PDF/DOCX ingestion requires external converters (pdftotext, pandoc); not handled natively.
- End-to-end lineage tracking not yet implemented (Issue: #1901).

## Breaking Changes
- `IngestionBuilder` fluent API is stable from v1.x.
- Source connector interface may gain new lifecycle hooks in v1.6.0.
