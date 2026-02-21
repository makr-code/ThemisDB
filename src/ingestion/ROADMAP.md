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

## In Progress 🚧
- [ ] PDF and DOCX binary format ingestion via external converters (Target: Q2 2026)
- [ ] Cursor-based pagination support alongside offset/limit (Target: Q2 2026)
- [ ] OAuth 2.0 token refresh handling within connectors (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Kafka consumer source connector
- [ ] S3/GCS/Azure Blob object-storage source
- [ ] JDBC-compatible database source
- [ ] Web crawler / sitemap ingestion source
- [ ] Per-source schema validation before write

### Long-term (6-12 months)
- [ ] Distributed ingestion coordinator across nodes
- [ ] Change-data-capture (CDC) source for live database streams
- [ ] Plugin API for third-party source connectors
- [ ] Dynamic source reconfiguration without restart
- [ ] End-to-end ingestion lineage tracking

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
- [~] Replace libcurl stubs with real `curl_easy_perform` calls (`ingestion/api_connector.cpp`) (Target: Q2 2026)
- [~] Per-document quarantine retry with exponential back-off (Target: Q2 2026)
- [~] Binary MIME type detection (PDF, DOCX) before dispatch to converters (Target: Q2 2026)
- [ ] OAuth 2.0 token refresh handling within connectors (Target: Q3 2026)
- [ ] Cursor-based pagination support alongside offset/limit (Target: Q3 2026)

### Phase 3: Distributed Sources & Connectors (Status: Planned 📋)
- [ ] Kafka consumer source connector (`ingestion/kafka_connector.cpp`)
- [ ] S3 / GCS / Azure Blob object-storage source connector
- [ ] Distributed ingestion coordinator across nodes (work-stealing thread pool)
- [ ] Change-data-capture (CDC) source for live database streams
- [ ] Plugin API for third-party source connectors

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (filesystem, HuggingFace, generic API)
- [ ] Performance benchmarks (docs/sec, MB/sec)
- [ ] Security audit (path traversal, API key storage)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- PDF/DOCX require external converters; not handled natively.
- OAuth token refresh must be handled by callers; connectors use static tokens.
- libcurl HTTP calls in HuggingFaceConnector are stubbed; replace with `curl_easy_perform` in production builds.

## Breaking Changes
- `IngestionBuilder` fluent API is stable from v1.x.
- Source connector interface may gain new lifecycle hooks in v1.5.0.
