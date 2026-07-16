# Ingestion Module – Production Readiness Roadmap

**Version:** 1.0  
**Last Updated:** 2026-04-06  
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
| `IngestionManager` – preview mode | ✅ Implemented (`previewSource()`, `SourcePreview`) |
| `IngestionManager` – pause / resume | ✅ Implemented (`pauseSource()`, `resumeSource()`) |
| `IngestionManager` – rate limiting | ✅ Per-source token-buckets (`RateLimitConfig`, `QUOTA_EXCEEDED`) |
| Structured error codes / severity | ✅ Implemented (`IngestionErrorCode`, `IngestionErrorSeverity`) |
| Correlation IDs | ✅ Implemented (`IngestionStats::correlation_id`) |
| Quarantine / Dead-Letter Queue | ✅ In-memory quarantine with Admin API |
| Observability metrics | ✅ Counters including `quota_violations` |
| Prometheus text-format exporter | ✅ `IngestionMetricsExporter` with `source_type`+`error_code` labels |
| Grafana dashboard | ✅ `grafana/ingestion-dashboard.json` |
| Grafana alert rules | ✅ `grafana/ingestion-alerts.json` (5 alert rules) |
| `IngestionBuilder` fluent API | ✅ Implemented |
| `IngestionAdminApi` operator layer | ✅ Implemented (listSources, start, pause, resume, quarantine, healthJson) |
| `CheckpointStore` (file-based) | ✅ Implemented (write/read/clear/exists; key=value format) |
| `IngestionManager` incremental mode | ✅ Implemented (`setCheckpointDir`, `enableIncrementalMode`, `getCheckpoint`, `clearCheckpoint`) |
| `GenericApiConnector` | ✅ Implemented (pagination cursor, retry, configurable `text_field`, `max_pages`) |
| Resilience / fuzz-style tests | ✅ `tests/test_ingestion_resilience.cpp` |
| Prometheus / OpenTelemetry push | Not yet integrated |
| API connector | ✅ `GenericApiConnector` (simulated HTTP; real libcurl planned) |
| Database connector | Not yet implemented |
| Admin HTTP REST endpoints | Not yet implemented |

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

### 2.2 Checkpoint & Resume ✅ DONE
- [x] Persist ingestion offset / cursor to a file-based `CheckpointStore` after each successful source run
- [x] `CheckpointStore`: write / read / clear / exists per source; key=value text format; thread-safe
- [x] `IngestionManager::setCheckpointDir(dir)` – configure checkpoint directory at runtime
- [x] `IngestionManager::enableIncrementalMode(bool)` / `isIncrementalMode()` – opt-in to skip re-ingested docs
- [x] `IngestionManager::getCheckpoint(source_id, out)` – inspect current checkpoint
- [x] `IngestionManager::clearCheckpoint(source_id)` – force full re-ingest on next run
- [x] `IngestionCheckpoint` struct: `source_id`, `processed_count`, `byte_offset`, `cursor`, `timestamp`
- [ ] On restart, skip already-ingested ranges (connector-level integration per HF/API stream offset)
- [ ] Support `incremental=true` flag in `SourceConfig::options`

### 2.3 Quarantine & Dead-Letter Queue ✅ PARTIAL
- [x] In-memory quarantine list (`QuarantineEntry`) populated from FATAL errors
- [x] Admin API: `getQuarantineItems()`, `dismissQuarantineItem()`, `clearQuarantine()`
- [x] Quarantine snapshot attached to `IngestionReport`
- [ ] Persist quarantine to RocksDB across restarts
- [ ] Admin HTTP API to list / retry / dismiss quarantine items

### 2.4 Quota & Rate-Limit Guard ✅ DONE
- [x] `RateLimitConfig` struct: `requests_per_second`, `max_bytes_per_hour`, `enabled`
- [x] Token-bucket rate limiter (`TokenBucket`) with blocking back-pressure
- [x] Per-source independent token buckets keyed by `source_id`
- [x] `IngestionManager::setRateLimitConfig()` + `IngestionBuilder::withRateLimitConfig()`
- [x] `QUOTA_EXCEEDED` (error_code 1401) emitted when `max_bytes_per_hour` is breached
- [x] `IngestionMetrics::quota_violations` counter incremented on breach

### 2.5 Source Pause / Resume ✅ DONE
- [x] `IngestionManager::pauseSource(id)` – mark source disabled in-memory
- [x] `IngestionManager::resumeSource(id)` – re-enable a paused source
- [x] Paused sources skipped in `ingestAll()` (inherited from `enabled` flag logic)

### 2.5 API & Database Connectors ✅ PARTIAL
- [x] `GenericApiConnector`: generic REST/JSON source with pagination cursor (`cursor_param`),
  configurable `text_field`, `page_size`, `max_pages`, Bearer token auth, retry back-off
- [x] `SourceType::API` wired through `IngestionManager::ingestSource()` (creates `GenericApiConnector`)
- [x] `include/ingestion/api_connector.h` + `src/ingestion/api_connector.cpp`
- [ ] Real HTTP implementation (libcurl) for `GenericApiConnector` (currently simulated)
- [ ] `DatabaseConnector`: JDBC-style ODBC / PostgreSQL / MySQL bulk export

### 2.6 Fuzz & Chaos Testing ✅ PARTIAL
- [x] `tests/test_ingestion_resilience.cpp`: empty files, truncated JSON, malformed
  HTML/XML, binary noise, non-UTF-8 bytes, path-traversal attempts, 1 MiB stress,
  concurrent register/quarantine access, non-existent directory
- [ ] Fuzz corpus for corrupt PDF, truncated DOCX (requires libfuzzer integration)
- [ ] Chaos tests: random network failures, slow responses, OOM conditions

---

## Q3 2026 – Observability, DX & Operator Tooling

**Goal:** Operational excellence – dashboards, admin API, plug-in DX.

### 3.1 Prometheus / Grafana Dashboard ✅ DONE
- [x] Pre-built Grafana dashboard JSON: `grafana/ingestion-dashboard.json`
- [x] Panels: Overview stats (docs, failures, quarantine, retries, throughput),
  throughput time-series, byte throughput, error rate, retry rate, error-by-code,
  quarantine size over time, ingestion duration per source
- [x] Template variable: `source_id` (multi-select) and `DS_PROMETHEUS` datasource
- [x] Alert rules JSON: `grafana/ingestion-alerts.json` – error spike, throughput
  drop, quota breach (error_code=1401), quarantine growing, high retry rate

### 3.2 Admin / Operator API ✅ PARTIAL
- [x] `IngestionAdminApi` in-process operator class wrapping `IngestionManager`
- [x] `listSources()` – list all registered sources with enabled/available/doc_count status
- [x] `startSource(id)` – trigger immediate ingestion run
- [x] `pauseSource(id)` / `resumeSource(id)` – disable / re-enable a source
- [x] `listQuarantine()` – list quarantined items
- [x] `retryQuarantineItem(path)` – dismiss from quarantine and re-run source
- [x] `dismissQuarantineItem(path)` – permanently dismiss a quarantine entry
- [x] `healthJson()` – compact JSON health status (`status`, `registered_sources`,
  `enabled_sources`, `quarantine_size`)
- [ ] Expose as HTTP REST endpoints via cpp-httplib (Q3 final milestone)

### 3.3 Plug-In System Polish
- [ ] `IngestionPlugin` loader: discover shared libraries in `plugins/` directory
- [ ] Plugin sandboxing: memory / CPU limits per plugin
- [ ] Plugin health: report `isHealthy()` to operator API

### 3.4 Preview / DryRun Mode ✅ DONE
- [x] `setDryRun(true)`: scan source via `getDocumentCount()`, report count, no insertion
- [x] `IngestionReport::dry_run` flag set when run in dry-run mode
- [x] `IngestionManager::previewSource(source_id, max_docs)` – returns first N
  document contents (capped at 100) without writing; includes `total_available` and
  `truncated` flag; `SourcePreview` struct

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
