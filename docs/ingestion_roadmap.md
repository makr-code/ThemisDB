# Ingestion Module – Production Readiness Roadmap

**Version:** 1.0  
**Last Updated:** 2026-02-20  
**Scope:** `src/ingestion/` – HuggingFaceConnector, FileSystemIngester, IngestionManager

---

## Overview

This roadmap tracks the phased evolution of ThemisDB's ingestion module from its current
state (functional but partially stubbed) to a production-grade, observable, and
resilient pipeline capable of handling large-scale, multi-format, multi-source data
ingestion.

**Target Timeline:** Q1–Q3 2026  
**Primary Goals:** Reliability, Observability, Real Extraction, Parallelism, DX

---

## Current State (Baseline)

| Component | Status |
|---|---|
| `HuggingFaceConnector` – HTTP | Simulated (libcurl stub) |
| `HuggingFaceConnector` – Retry / back-off | ✅ Implemented (`RetryConfig`) |
| `FileSystemIngester` – .txt / .md / .csv | ✅ Real read |
| `FileSystemIngester` – HTML / XML | ✅ Implemented (pugixml when available, raw fallback) |
| `FileSystemIngester` – JSON | ✅ Implemented (string-value extraction, no external dep) |
| `FileSystemIngester` – PDF / DOCX / OCR | Stub / skipped cleanly |
| `IngestionManager` – sequential | ✅ Working |
| `IngestionManager` – parallel | ✅ Implemented (`std::async` wave scheduler) |
| `IngestionManager` – dry-run mode | ✅ Implemented (`setDryRun()`) |
| `IngestionManager` – rate limiting | ✅ Token-bucket (`RateLimitConfig`, `setRateLimitConfig()`) |
| Structured error codes / severity | ✅ Implemented (`IngestionErrorCode`, `IngestionErrorSeverity`) |
| Correlation IDs | ✅ Implemented (`IngestionStats::correlation_id`) |
| Quarantine / Dead-Letter Queue | ✅ Basic in-memory quarantine (`QuarantineEntry`, `getQuarantineItems()`) |
| Observability metrics | ✅ Basic counters in `IngestionMetrics` |
| Prometheus text-format exporter | ✅ Implemented (`IngestionMetricsExporter`, `source_type`+`error_code` labels) |
| Grafana dashboard | ✅ `grafana/ingestion-dashboard.json` |
| `IngestionBuilder` fluent API | ✅ Implemented |
| Prometheus / OpenTelemetry push | Not yet integrated |
| API / Database connector | Stub (not implemented) |
| Admin HTTP Operator API | Not yet implemented |

---

## Q1 2026 – Foundation & Reliability

**Goal:** Replace stubs with production-viable implementations; add full error
taxonomy and basic observability.

### 1.1 Real HTTP Client (HuggingFaceConnector) ✅ PARTIAL
- [x] `RetryConfig` struct: max attempts, exponential back-off, timeout
- [x] `getWithRetry()` helper with structured error accumulation
- [ ] Replace `HttpClient::get()` stub with real libcurl / cpp-httplib calls
- [ ] Support HTTPS, proxy, and Bearer token via `Authorization` header
- [ ] Parse HuggingFace Hub JSON metadata for real row count
- [ ] Support Parquet streaming via Arrow Flight or row-group iteration

### 1.2 Structured Error Taxonomy ✅ DONE
- [x] `IngestionErrorCode` enum (source, HTTP, file, processing, retry, internal)
- [x] `IngestionErrorSeverity` (INFO, WARNING, ERROR, FATAL)
- [x] `IngestionError` struct with `isRetryable()` predicate
- [x] `IngestionStats::addError()` accumulates structured log + backward-compat field
- [x] `IngestionStats::correlation_id` – unique run ID for cross-component tracing
- [ ] Propagate error list to Admin HTTP API endpoint
- [ ] Add structured logging with correlation ID to spdlog

### 1.3 Real File Format Extraction (FileSystemIngester) ✅ PARTIAL
- [x] Plain text (`.txt`) – real `std::ifstream` read
- [x] Markdown (`.md`) and CSV (`.csv`) – treated as plain text
- [x] HTML / HTM – text node extraction via pugixml; raw fallback when unavailable
- [x] XML – text node extraction via pugixml; raw fallback when unavailable
- [x] JSON – string-value extraction (no external JSON lib required)
- [ ] PDF – integrate `libpoppler` or `pdfium` for native text extraction
- [ ] DOCX – integrate `libdocx` / `pugixml`-based Office Open XML parser
- [ ] Encoding detection – integrate `uchardet` or `ICU`
- [ ] OCR – wire Tesseract for scanned PDFs (flag: `ocr_enabled=true`)

### 1.4 Prometheus / OpenTelemetry Hooks ✅ PARTIAL
- [x] `IngestionMetricsExporter`: converts `IngestionReport` / `IngestionStats` to
  Prometheus text exposition format (no external dependency required)
- [x] Labels: `source_id`, `source_type` (optional third parameter), `error_code`
- [x] Metrics: `docs_processed_total`, `docs_failed_total`, `bytes_processed_total`,
  `elapsed_seconds`, `retry_total`, `errors_total`, `throughput_docs_per_sec`,
  `total_documents`, `total_failures`, `total_time_seconds`, `quarantine_size`,
  `errors_by_code_total` (per-error-code breakdown)
- [ ] Push to prometheus-cpp registry (already a vcpkg dependency)

---

## Q2 2026 – Scalability & Resilience

**Goal:** Handle large, faulty, and concurrent ingestion workloads reliably.

### 2.1 Parallel Processing ✅ PARTIAL
- [x] `std::async` wave scheduler in `IngestionManager::ingestAll()` bounded by
  `max_threads_`
- [ ] Replace wave scheduler with a proper thread-pool (`std::jthread` or TBB)
- [ ] Back-pressure: block submission when queue is full
- [ ] Per-source concurrency cap (to avoid overwhelming single sources)

### 2.2 Checkpoint & Resume
- [ ] Persist ingestion offset / cursor to RocksDB after each successful batch
- [ ] On restart, skip already-ingested ranges (resume from checkpoint)
- [ ] Support `incremental=true` flag in `SourceConfig` / `IngestionSource`

### 2.3 Quarantine & Dead-Letter Queue ✅ PARTIAL
- [x] In-memory quarantine list (`QuarantineEntry`) populated from FATAL errors
- [x] Admin API: `getQuarantineItems()`, `dismissQuarantineItem()`, `clearQuarantine()`
- [x] Quarantine snapshot attached to `IngestionReport`
- [ ] Persist quarantine to RocksDB across restarts
- [ ] Admin HTTP API to list / retry / dismiss quarantine items

### 2.4 Quota & Rate-Limit Guard ✅ PARTIAL
- [x] `RateLimitConfig` struct: `requests_per_second`, `max_bytes_per_hour`, `enabled`
- [x] Token-bucket rate limiter (`TokenBucket`) in `IngestionManager` with blocking
  back-pressure
- [x] `IngestionManager::setRateLimitConfig()` + `IngestionBuilder::withRateLimitConfig()`
- [ ] Per-source independent buckets (currently global per-manager)
- [ ] Emit `QUOTA_EXCEEDED` errors on `max_bytes_per_hour` breach

### 2.5 API & Database Connectors
- [ ] `ApiConnector`: generic REST/GraphQL source with pagination cursor
- [ ] `DatabaseConnector`: JDBC-style ODBC / PostgreSQL / MySQL bulk export

### 2.6 Fuzz & Chaos Testing
- [ ] Fuzz corpus for corrupt PDF, truncated DOCX, malformed JSON
- [ ] Chaos tests: random network failures, slow responses, OOM conditions
- [ ] Race-condition regression suite for parallel ingestion

---

## Q3 2026 – Observability, DX & Operator Tooling

**Goal:** Operational excellence – dashboards, admin API, plug-in DX.

### 3.1 Prometheus / Grafana Dashboard ✅ DONE
- [x] Pre-built Grafana dashboard JSON: `grafana/ingestion-dashboard.json`
- [x] Panels: Overview stats (docs, failures, quarantine, retries, throughput),
  throughput time-series, byte throughput, error rate, retry rate, error-by-code,
  quarantine size over time, ingestion duration per source
- [x] Template variable: `source_id` (multi-select) and `DS_PROMETHEUS` datasource
- [ ] Alert rules: error spike, throughput drop, quota breach (requires Grafana Alerting)

### 3.2 Admin / Operator API
- [ ] `GET /ingestion/sources` – list registered sources with status
- [ ] `POST /ingestion/sources/{id}/start` – trigger manual run
- [ ] `POST /ingestion/sources/{id}/pause` – pause scheduled runs
- [ ] `GET /ingestion/jobs/{job_id}` – job status, progress, errors
- [ ] `GET /ingestion/quarantine` – list quarantined items
- [ ] `POST /ingestion/quarantine/{id}/retry` – re-enqueue quarantined item

### 3.3 Plug-In System Polish
- [ ] `IngestionPlugin` loader: discover shared libraries in `plugins/` directory
- [ ] Plugin sandboxing: memory / CPU limits per plugin
- [ ] Plugin health: report `isHealthy()` to operator API

### 3.4 Preview / DryRun Mode ✅ PARTIAL
- [x] `setDryRun(true)`: scan source via `getDocumentCount()`, report count, no insertion
- [x] `IngestionReport::dry_run` flag set when run in dry-run mode
- [ ] Source preview: return first N documents for inspection

### 3.5 End-to-End Developer Experience ✅ PARTIAL
- [x] `IngestionBuilder` fluent API: `withHuggingFaceSource()`, `withFilesystemSource()`,
  `withRetryConfig()`, `withRateLimitConfig()`, `withParallelProcessing()`,
  `withTargetCollection()`, `withDryRun()`, `build()`
- [ ] CLI command: `themis ingest --source hf:lexlms/ger_legal_data --dry-run`
- [ ] Health-check endpoint: `GET /ingestion/health`

---

## Contribution Guide

Interested in implementing a phase item?  Please:

1. Open an issue referencing this roadmap section (e.g., "Q1 1.3 – PDF extraction").
2. Discuss approach (library choice, API surface) in the issue.
3. Submit a PR with implementation + tests + updated roadmap checkbox.

All ingestion changes must:
- Maintain backward compatibility with existing `IngestionStats` / `IngestionReport`
- Add unit tests for new error paths
- Update `IngestionMetrics` counters where applicable
