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
| `FileSystemIngester` – .txt | ✅ Real read |
| `FileSystemIngester` – PDF / DOCX / OCR | Stub / placeholder |
| `IngestionManager` – sequential | ✅ Working |
| `IngestionManager` – parallel | ✅ Implemented (`std::async` wave scheduler) |
| Structured error codes / severity | ✅ Implemented (`IngestionErrorCode`, `IngestionErrorSeverity`) |
| Observability metrics | ✅ Basic counters in `IngestionMetrics` |
| Prometheus / OpenTelemetry | Not yet integrated |
| API / Database connector | Stub (not implemented) |
| Admin / Operator API | Not yet implemented |

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
- [ ] Propagate error list to API / admin endpoint
- [ ] Add correlation IDs for cross-component tracing

### 1.3 Real File Format Extraction (FileSystemIngester)
- [x] Plain text (`.txt`) – real `std::ifstream` read
- [ ] PDF – integrate `libpoppler` or `pdfium` for native text extraction
- [ ] DOCX – integrate `libdocx` / `pugixml`-based Office Open XML parser
- [ ] HTML / XML – reuse `pugixml` already in vcpkg dependencies
- [ ] Encoding detection – integrate `uchardet` or `ICU`
- [ ] OCR – wire Tesseract for scanned PDFs (flag: `ocr_enabled=true`)

### 1.4 Prometheus / OpenTelemetry Hooks
- [ ] Expose `IngestionMetrics` counters as Prometheus gauge/counter via
  `prometheus-cpp` (already a vcpkg dependency)
- [ ] Labels: `source_id`, `source_type`, `error_code`
- [ ] Metrics: `ingestion_docs_processed_total`, `ingestion_errors_total`,
  `ingestion_retry_total`, `ingestion_throughput_docs_per_sec`

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

### 2.3 Quarantine & Dead-Letter Queue
- [ ] Files / records that fail after all retries go to a quarantine collection
- [ ] Quarantine entry includes: path/URL, error code, timestamp, last error message
- [ ] Admin API to list, retry, or dismiss quarantine items

### 2.4 Quota & Rate-Limit Guard
- [ ] Per-source rate limit: max requests/second, max bytes/hour
- [ ] Global quota: total memory / CPU reserved for ingestion workers
- [ ] Emit `CONTENT_RATE_LIMIT_EXCEEDED` / `QUOTA_EXCEEDED` errors on breach

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

### 3.1 Prometheus / Grafana Dashboard
- [ ] Pre-built Grafana dashboard JSON for ingestion health
- [ ] Panels: throughput, error rate, retry rate, active sources, queue depth
- [ ] Alert rules: error spike, throughput drop, quota breach

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

### 3.4 Preview / DryRun Mode
- [ ] `dryRun=true` flag: scan source, report document count and estimated bytes
  without actual insertion
- [ ] Source preview: return first N documents for inspection

### 3.5 End-to-End Developer Experience
- [ ] CLI command: `themis ingest --source hf:lexlms/ger_legal_data --dry-run`
- [ ] SDK helper: `IngestionBuilder` fluent API
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
