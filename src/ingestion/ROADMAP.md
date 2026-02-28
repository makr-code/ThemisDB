# Ingestion Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.x – Production-grade data intake layer. Multi-source ingestion with filesystem, HuggingFace, and generic HTTP API connectors is fully implemented with rate limiting, checkpointing, quarantine, and Prometheus metrics.

## Completed ✅
- [x] GenericApiConnector – paginated JSON REST API ingestion with exponential back-off
- [x] FileSystemIngester – recursive directory walk with HTML/XML extraction (pugixml)
- [x] HuggingFaceConnector – dataset split ingestion with API token auth
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
  - 28 unit tests; path-traversal protection; JSON `text_field` extraction; credentials never logged 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [x] Kafka consumer source connector (Issue: #1892) ✅ Completed Q1 2026
- [x] S3/GCS/Azure Blob object-storage source (Issue: #1893)
- [P] JDBC-compatible database source (Issue: #1894)
- [P] Web crawler / sitemap ingestion source (Issue: #1895)
- [P] Per-source schema validation before write (Issue: #1896)

### Long-term (6-12 months)
- [I] Distributed ingestion coordinator across nodes (Issue: #1897)
- [x] Change-data-capture (CDC) source for live database streams (Issue: #2199)
- [I] Plugin API for third-party source connectors (Issue: #1908)
- [I] Dynamic source reconfiguration without restart (Issue: #1900)
- [I] End-to-end ingestion lineage tracking (Issue: #1901)

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

### Phase 2: HTTP Hardening & Binary Formats (Status: In Progress 🚧)
- [P] Replace libcurl stubs with real `curl_easy_perform` calls (`ingestion/api_connector.cpp`) (Target: Q2 2026) (Issue: #1915)
- [x] Per-document quarantine retry with exponential back-off (Target: Q2 2026) (Issue: #1916)
- [x] Binary MIME type detection (PDF, DOCX) before dispatch to converters (Target: Q2 2026) (Issue: #1917)
- [x] OAuth 2.0 token refresh handling within connectors (Target: Q3 2026) (Issue: #2408)
- [x] Cursor-based pagination support alongside offset/limit (Target: Q3 2026)

### Phase 3: Distributed Sources & Connectors (Status: Planned 📋)
- [x] Kafka consumer source connector (`ingestion/kafka_connector.cpp`) (Issue: #1904)
- [x] S3 / GCS / Azure Blob object-storage source connector (`ingestion/object_storage_connector.cpp`) (Issue: #1905)
  - Providers: AWS S3 (`THEMIS_ENABLE_S3`), GCS (`THEMIS_ENABLE_GCS`), Azure Blob (`THEMIS_ENABLE_AZURE`); graceful `CONNECTOR_NOT_SUPPORTED` when no SDK is compiled
  - Features: prefix filtering, `max_keys` cap, JSON `text_field` extraction, path-traversal rejection, `RetryConfig` passthrough, throughput metrics
  - Test coverage: 28 unit tests via mock-injection in `tests/test_ingestion_object_storage.cpp` (no cloud credentials required)
  - Security: `..` path components rejected; credentials never logged
- [I] Distributed ingestion coordinator across nodes (work-stealing thread pool) (Issue: #1906)
- [x] Change-data-capture (CDC) source for live database streams (Issue: #2199)
- [P] JDBC-compatible database source connector (`ingestion/database_connector.cpp`) (Issue: #1894)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1909)
- [I] Integration tests (filesystem, HuggingFace, generic API) (Issue: #1910)
- [I] Performance benchmarks (docs/sec, MB/sec) (Issue: #1911)
- [x] Security audit (path traversal, API key storage) (Issue: #1912)
- [I] Documentation complete (Issue: #1913)
- [I] API stability guaranteed (Issue: #1914)

## Known Issues & Limitations
- PDF/DOCX require external converters; not handled natively.
- libcurl HTTP calls in `HuggingFaceConnector` are stubbed; replace with `curl_easy_perform` in a follow-up PR.
- `GenericApiConnector` now uses real `curl_easy_perform` calls (PR #1915).

## Breaking Changes
- `IngestionBuilder` fluent API is stable from v1.x.
- Source connector interface may gain new lifecycle hooks in v1.5.0.
