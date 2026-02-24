# Ingestion Module — Architecture Guide

**Version:** 1.0  
**Last Updated:** 2026-02-24  
**Module Path:** `src/ingestion/`

---

## 1. Overview

The Ingestion module is ThemisDB's data intake layer. It provides a unified pipeline for
pulling documents from heterogeneous external sources — local filesystems, HuggingFace
datasets, and generic REST APIs — normalizing them, and writing them into the database.

Key operational features: parallel multi-source ingestion, configurable retry with
exponential back-off, token-bucket rate limiting, incremental checkpointing, quarantine
queue for bad records, and Prometheus metrics export.

---

## 2. Design Principles

- **Source Abstraction** – each source (filesystem, API, HuggingFace) implements the same
  `IIngestionSource` interface; the `IngestionManager` drives all sources uniformly.
- **Fault Tolerance** – failed records are quarantined rather than aborting the pipeline;
  transient errors are retried with exponential back-off.
- **Incremental by Default** – checkpoint-based tracking prevents re-processing of
  already-ingested records on restart.
- **Rate-Limited** – per-source token bucket prevents overwhelming external APIs.
- **Dry-Run Mode** – full pipeline execution without database writes, for testing.
- **Observable** – Prometheus-compatible text metrics for throughput, errors, and lag.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `ingestion_manager.cpp` | Orchestrator: manages all sources, parallel execution, admin API |
| `api_connector.cpp` | Generic REST API connector with pagination, retry, rate limiting |
| `filesystem_ingester.cpp` | Directory walker with binary MIME detection (PDF, DOCX) |
| `huggingface_connector.cpp` | HuggingFace Hub dataset connector |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                  Admin API / CLI / Scheduled Jobs               │
│   POST /ingestion/sources/{id}/start  |  IngestionBuilder        │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                    IngestionManager                              │
│                                                                  │
│  Parallel source execution (configurable concurrency)           │
│  Checkpoint persistence (last_cursor per source)                │
│  Quarantine queue (failed records → quarantine collection)      │
│  Prometheus metrics (throughput, errors, lag, retry counts)     │
│  Admin API (list/pause/resume/quarantine per source)            │
└──────┬──────────────────┬────────────────────────┬──────────────┘
       │                  │                        │
┌──────▼──────┐  ┌────────▼──────────┐  ┌─────────▼──────────────┐
│ API         │  │  FileSystem       │  │  HuggingFace           │
│ Connector   │  │  Ingester         │  │  Connector             │
│             │  │  (MIME detect:    │  │  (dataset splits,      │
│ Token bucket│  │   PDF/DOCX/XML)   │  │   pagination, auth)    │
│ Retry/exp   │  │                   │  │                        │
│ backoff     │  └───────────────────┘  └────────────────────────┘
└─────────────┘
```

---

## 4. Data Flow

### 4.1 Normal Ingestion

```
IngestionManager::start()
    │
    for each registered source (parallel):
    │
    ├─ load checkpoint: last_cursor
    │
    ├─ source.fetchBatch(from: cursor, size: page_size)
    │       ├─ API: paginated JSON → extract text_field per record
    │       ├─ Filesystem: walk dir → read file → MIME detect → extract text
    │       └─ HuggingFace: dataset split → row batch
    │
    ├─ for each document:
    │       ├─ validate (not empty, max size)
    │       ├─ valid → write to ThemisDB storage
    │       └─ invalid (N retries exhausted) → quarantine queue
    │
    ├─ update checkpoint to current cursor
    └─ metrics.record(batch_size, errors, duration)
```

### 4.2 Binary File Ingestion (PDF/DOCX)

```
FileSystemIngester: encounter file "report.pdf"
    │
    ├─ detectBinaryMimeType(): read first 8 bytes
    │       ├─ starts with "%PDF" → PDF
    │       └─ starts with "PK\x03\x04" + OOXML marker → DOCX
    │
    ├─ PDF → invoke pdftotext converter (if configured)
    │       → extract plain text → ingest as document
    └─ DOCX → invoke pandoc converter (if configured)
               → extract plain text → ingest as document
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Writes to** | `src/storage/` | Document persistence |
| **Uses** | `src/content/` | Binary content processing (when enabled) |
| **Uses** | `src/observability/` | Prometheus metrics export |
| **Consumed by** | `src/server/` | Ingestion admin API |
| **Consumed by** | `src/scheduler/` | Scheduled ingestion jobs |

---

## 6. Threading & Concurrency Model

- Each source runs on a dedicated worker thread; `IngestionManager` uses a configurable
  thread pool.
- Checkpoint writes are serialized per source using a per-source mutex.
- Quarantine queue uses a thread-safe lock-free queue.
- Rate limiter (token bucket) is per-source and thread-safe.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Parallel sources | Multiple sources ingest concurrently |
| Batch fetching | Configurable page size (default: 100 docs) |
| Incremental checkpoints | Avoid full re-scan on restart |
| Token bucket | Prevents API rate-limit errors from external sources |

---

## 8. Security Considerations

- API keys and bearer tokens are stored in config; not logged.
- External converters (pdftotext, pandoc) are invoked as subprocesses; paths are
  validated to prevent command injection.
- Filesystem ingester validates paths to prevent directory traversal.
- Dry-run mode enables testing without writing to the database.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `ingestion.concurrency` | 4 | Max parallel source threads |
| `ingestion.batch_size` | 100 | Documents per fetch batch |
| `ingestion.retry.max_attempts` | 3 | Max retries per document |
| `ingestion.retry.initial_delay_ms` | 1000 | Initial retry delay |
| `ingestion.retry.backoff_factor` | 2.0 | Exponential backoff multiplier |
| `ingestion.rate_limit.rps` | 10 | Requests per second per source |
| `ingestion.quarantine.enabled` | true | Enable quarantine for failed docs |
| `ingestion.dry_run` | false | Skip database writes |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| HTTP 429 (rate limit) | Respect Retry-After header; token bucket limits proactive rate |
| HTTP 5xx / network error | Exponential back-off retry; quarantine after max_attempts |
| File read error | Log error; skip file; continue directory walk |
| Converter failure (pdftotext) | Skip conversion; log warning; ingest raw text if available |

---

## 11. Known Limitations & Future Work

- PDF/DOCX ingestion requires external converters (pdftotext, pandoc) to be installed.
- WebSocket/real-time source connectors are planned.
- Kafka and MQTT source connectors are planned.
- Full-text extraction for complex PDFs (scanned images) requires OCR (Tesseract integration planned).

---

## 12. References

- `src/ingestion/README.md` — module overview
- `docs/ingestion_roadmap.md` — roadmap
- `ARCHITECTURE.md` (root) — full system architecture
