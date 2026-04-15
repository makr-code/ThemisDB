# Ingestion Module — Architecture Guide

<!-- Status: current | validated: 2026-04-06 | Primary: src/ingestion/ | Secondary: docs/de/ingestion/ -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../docs/de/ingestion/README.md -->

**Version:** 1.1  
**Last Updated:** 2026-04-06  
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
| `ingestion_manager.cpp` | Orchestrator: manages all sources, parallel execution, admin API, schema validation, plugin registry, dynamic reconfiguration |
| `api_connector.cpp` | Generic REST API connector with pagination, retry, rate limiting (real `curl_easy_perform`) |
| `filesystem_ingester.cpp` | Directory walker with binary MIME detection (PDF, DOCX) |
| `huggingface_connector.cpp` | HuggingFace Hub dataset connector (HTTP simulated; see Known Limitations) |
| `kafka_connector.cpp` | Apache Kafka consumer source connector (librdkafka) |
| `object_storage_connector.cpp` | S3 / GCS / Azure Blob object-storage connector |
| `database_connector.cpp` | JDBC-compatible database source connector (ODBC) |
| `web_crawler_connector.cpp` | HTTP web crawler with BFS, sitemap, and robots.txt support |
| `cdc_connector.cpp` | Change-data-capture source for live database streams (stream backend gated behind `THEMIS_ENABLE_CDC_STREAM`) |
| `ingestion_coordinator.cpp` | Distributed ingestion coordinator with work-stealing thread pool |

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
└──────┬──────────┬──────────┬─────────────┬──────────┬──────────┘
       │          │          │             │          │
┌──────▼──────┐  ┌▼──────────▼─┐  ┌───────▼──────┐  ┌▼─────────────────┐
│ API         │  │  FileSystem  │  │  HuggingFace │  │  WebCrawler      │
│ Connector   │  │  Ingester    │  │  Connector   │  │  Connector       │
│             │  │  (MIME:      │  │  (datasets,  │  │  (BFS, sitemap,  │
│ Token bucket│  │   PDF/DOCX)  │  │   splits)    │  │   robots.txt,    │
│ Retry/exp   │  │              │  │              │  │   http/https     │
│ backoff     │  └──────────────┘  └──────────────┘  │   only / SSRF   │
└─────────────┘                                       │   prevention)   │
                                                      └─────────────────┘
  Also: KafkaConnector · ObjectStorageConnector · DatabaseConnector
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
    │       ├─ HuggingFace: dataset split → row batch
    │       └─ WebCrawler: BFS from seed → fetch page → strip HTML → extract text
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

- PDF/DOCX ingestion requires external converters (pdftotext, pandoc) to be installed; native parsing not supported.
- WebSocket/real-time source connectors are planned.
- OCR for scanned PDF pages (Tesseract integration) is planned.

---

## 12. References

- `src/ingestion/README.md` — module overview
- `docs/ingestion_roadmap.md` — roadmap
- `ARCHITECTURE.md` (root) — full system architecture

---

## 13. Ingestion v2.0 — Universal File Ingestion with Workflow Orchestration

> **Status:** Implementation started 2026-04-15 | First use case: Legal Documents

### 13.1 Architecture Overview

```
INPUT: Beliebige Datei (PDF, DOCX, ZIP, SHP, PNG, XLSX, HTML, EPUB, TXT, ...)
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│  1. FILE INTAKE LAYER                                   │
│     FileTypeDetector (magic bytes + extension)          │
│     MetadataExtractor (EXIF, MIME, filename, dates)     │
│     → FileManifest { file_id, format, mime, meta }      │
└─────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│  2. WORKFLOW ORCHESTRATOR (JSON/YAML-driven)            │
│     WorkflowEngine  — loadProfile / selectProfile       │
│     StepRegistry    — name → IIngestionStep plugin      │
└─────────────────────────────────────────────────────────┘
         │  runs steps sequentially
         ▼
┌─────────────────────────────────────────────────────────┐
│  3. STEP PLUGINS (builtin or DLL/IIngestionStep)        │
│                                                         │
│  builtin.parse_text          raw text extraction        │
│  builtin.decompress          ZIP/tar/gzip unpack        │
│  builtin.chunk_text          fixed/sentence/section     │
│  builtin.legal_metadata      regex norm/date/Az         │
│  builtin.deontic_extractor   obligation/prohibition/... │
│  builtin.legal_ref_extractor §-cross-reference graph    │
│  builtin.chunk_embed         vector embeddings          │
│  builtin.base_entity_assembler dedup + canonicalise     │
└─────────────────────────────────────────────────────────┘
         │  enriches ExtractionContext
         ▼
┌─────────────────────────────────────────────────────────┐
│  4. BASE-ENTITY ASSEMBLER                               │
│     EntityNormalizer  (canonical IDs)                   │
│     RelationBuilder   (cross-refs → graph edges)        │
│     → BaseEntitySet { nodes[], edges[], chunks[] }      │
└─────────────────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│  5. SINK                                                │
│     GraphWriter  → ThemisDB Graph Store                 │
│     VectorWriter → ThemisDB Vector Index                │
│     DocWriter    → IDocumentStore                       │
└─────────────────────────────────────────────────────────┘
```

### 13.2 Key Types

| Type | Header | Purpose |
|---|---|---|
| `FileManifest` | `include/ingestion/file_manifest.h` | SHA-256 ID, MIME, EXIF, timestamps |
| `ExtractionContext` | `include/ingestion/extraction_context.h` | Shared mutable pipeline state |
| `IIngestionStep` | `include/ingestion/ingestion_step.h` | Plugin interface for all steps |
| `StepConfig` | `include/ingestion/ingestion_step.h` | YAML step config (JSON blob) |
| `WorkflowProfile` | `include/ingestion/workflow_engine.h` | Parsed workflow definition |
| `StepRegistry` | `include/ingestion/workflow_engine.h` | Name → step instance, thread-safe |
| `WorkflowEngine` | `include/ingestion/workflow_engine.h` | Orchestrator (load/select/execute) |
| `BaseEntity` | `include/ingestion/base_entity.h` | Graph-RAG node (entity + embedding) |
| `EntityRelation` | `include/ingestion/base_entity.h` | Directed graph edge |
| `VectorRecord` | `include/ingestion/base_entity.h` | Vector index entry |
| `BaseEntitySet` | `include/ingestion/base_entity.h` | Assembler output for all sinks |

### 13.3 Workflow Profile Schema (JSON / YAML)

```json
{
  "apiVersion": "themis.ingestion/v1",
  "kind":       "IngestionWorkflow",
  "name":       "legal-document-de",
  "file_patterns": {
    "mime_types":         ["application/pdf"],
    "filename_patterns":  ["*Gesetz*", "*BImSchG*"]
  },
  "steps": [
    { "name": "parse_text",  "plugin": "builtin.parse_text",  "on_failure": "abort", "config": {} },
    { "name": "chunk",       "plugin": "builtin.chunk_text",  "on_failure": "skip",  "config": {"strategy":"section","size":512,"overlap":64} },
    { "name": "deontic",     "plugin": "builtin.deontic_extractor", "on_failure": "skip", "config": {"confidence_threshold":0.65} },
    { "name": "assemble",    "plugin": "builtin.base_entity_assembler", "on_failure": "abort", "config": {} }
  ],
  "output": { "graph": true, "vector": true, "document_store": true }
}
```

### 13.4 Error Codes (v2.0)

`ERR_WORKFLOW_*` — codes 9600–9619 in `include/utils/error_registry.h`.

| Code | Name | Meaning |
|---|---|---|
| 9600 | `ERR_WORKFLOW_PROFILE_NOT_FOUND` | YAML/JSON profile file not found |
| 9601 | `ERR_WORKFLOW_PROFILE_INVALID` | Profile fails schema validation |
| 9602 | `ERR_WORKFLOW_NO_MATCHING_PROFILE` | No profile matches file MIME/name |
| 9603 | `ERR_WORKFLOW_STEP_NOT_REGISTERED` | Step plugin not in StepRegistry |
| 9604 | `ERR_WORKFLOW_STEP_EXECUTION_FAILED` | Step returned an error |
| 9605 | `ERR_WORKFLOW_STEP_NOT_A_STEP` | Loaded plugin doesn't implement IIngestionStep |
| 9606 | `ERR_WORKFLOW_STEP_ALREADY_REGISTERED` | Duplicate step name |
| 9607–9619 | … | See `error_registry.h` |

### 13.5 Legal Document Use Case

Workflow: `config/ingestion/workflows/legal-document-de.json`

**Input:** `BImSchG_2024.pdf`

1. `parse_text` → `ctx.raw_text` (full PDF text, OCR fallback for scanned pages)
2. `chunk_text` (strategy: `section`) → one `TextChunk` per `§` / `Art.`
3. `legal_metadata` → `LEGAL_PROVISION` entities with `section_ref`, `DATE` entities, `LEGAL_AKTENZEICHEN` entities
4. `deontic_extractor` → `LEGAL_OBLIGATION`, `LEGAL_PROHIBITION`, `LEGAL_PERMISSION` entities per chunk
5. `base_entity_assembler` → dedup by canonical ID, enrich `source_file_id`

**Graph output:**
- Nodes: `LEGAL_PROVISION` (per §), `DATE`, `LEGAL_AUTHORITY`, `LEGAL_AKTENZEICHEN`
- Edges: `CITES`, `REGULATES`, `PART_OF`, `AMENDS`

**Vector output:**
- One `VectorRecord` per `TextChunk` (embedding via `builtin.chunk_embed`, Q3 2026)

### 13.6 Extending with a Custom Step Plugin

```cpp
// my_step.cpp — implements IIngestionStep
class MyStep : public themis::ingestion::IIngestionStep {
    const char* getName() const override { return "vendor.my_step"; }
    ...
    Result<void> execute(ExtractionContext& ctx, const StepConfig& cfg) override {
        // Read ctx.raw_text, append to ctx.entities
        return {};
    }
};

// C ABI entry points required for dynamic loading
extern "C" {
    themis::ingestion::IIngestionStep* themis_create_step() { return new MyStep(); }
    void themis_destroy_step(themis::ingestion::IIngestionStep* p) { delete p; }
}
```

Register at runtime:
```cpp
engine->stepRegistry().loadStepPlugin("vendor.my_step", "/path/to/my_step.so");
```
