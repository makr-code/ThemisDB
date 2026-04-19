# Content Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/content/`

---

## 1. Overview

The Content module provides ThemisDB's multi-format content ingestion and processing pipeline.
It accepts binary or text uploads across a wide range of MIME types (JSON, PDF, images, audio,
video, Office documents, CAD files, geospatial data, archives) and transforms them into
structured records that can be stored, searched, and embedded in the database.

The module sits between the API layer (upload endpoints) and the storage/index/LLM layers
(where processed content lands).

---

## 2. Design Principles

- **MIME-First Routing** – `mime_detector.cpp` identifies the content type before any
  processing begins; routing to processors is purely based on detected type, not file extension.
- **Policy Enforcement** – `content_policy.cpp` applies configurable rules (max file size,
  allowed MIME types, archive depth limits) before any expensive processing.
- **Security Scan** – `content_security.cpp` checks for malware signatures and archive bombs
  early in the pipeline.
- **Async Processing** – `async_ingestion_worker.cpp` processes content in background worker
  threads so upload responses are non-blocking.
- **Pluggable Processors** – `ingestion_plugin.cpp` provides a plugin interface for adding
  new content types without modifying core pipeline code.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `content_manager.cpp` | Pipeline orchestrator: MIME detect → policy → security → route |
| `content_manager_llm.cpp` | LLM-augmented extraction: summary, topics, sentiment, category (requires `THEMIS_ENABLE_LLM`) |
| `deduplication_checker.cpp` | pHash deduplication for images; MinHash+band-LSH deduplication for text |
| `language_detector.cpp` | Multi-language text detection and routing |
| `mime_detector.cpp` | MIME type detection via magic bytes, extension, and libmagic |
| `content_type.cpp` | MIME → ContentCategory mapping (TEXT, IMAGE, AUDIO, VIDEO, GEO, CAD, …) |
| `content_policy.cpp` | Configurable ingestion policy: allowed types, max sizes, depth limits |
| `content_security.cpp` | Archive bomb detection, malware signature scanning |
| `content_validator.cpp` | Format-level validation (JSON schema, image dimensions, etc.) |
| `text_processor.cpp` | Text extraction, chunking, MinHash perceptual deduplication |
| `html_processor.cpp` | HTML content extraction with boilerplate removal |
| `markdown_processor.cpp` | Markdown parsing and frontmatter extraction |
| `image_processor.cpp` | Image metadata extraction, pHash perceptual deduplication |
| `audio_processor.cpp` | Audio metadata extraction and optional STT transcription (`stt_processor.cpp`) |
| `video_processor.cpp` | Video metadata extraction and frame extraction (FFmpeg) |
| `pdf_processor.cpp` | PDF text extraction via poppler-cpp (layout-preserving, per-page) |
| `office_processor.cpp` | Office document text extraction (DOCX, XLSX, PPTX, ODF) |
| `ocr_processor.cpp` | Tesseract OCR text extraction from images (`THEMIS_ENABLE_OCR`) |
| `geo_processor.cpp` | Geospatial data extraction and GeoJSON normalization |
| `cad_processor.cpp` | CAD file metadata extraction |
| `stt_processor.cpp` | Speech-to-text processing interface |
| `tts_processor.cpp` | Text-to-speech generation interface |
| `archive_processor.cpp` | Archive unpacking (ZIP, tar) with bomb protection |
| `async_ingestion_worker.cpp` | Background worker pool for async content processing |
| `ingestion_plugin.cpp` | Plugin interface for custom content type processors |
| `version_manager.cpp` | Content versioning: track revisions of ingested documents |
| `content_metrics.cpp` | Processing throughput, error rate, format breakdown metrics |
| `content_errors.cpp` | Typed error hierarchy for content processing failures |
| `content_fs.cpp` | Filesystem utilities for temp file management |
| `content_logger.cpp` | Content-specific structured logging |
| `mock_clip_processor.cpp` | Mock CLIP-based image embedding processor (testing / no-GPU path) |
| `embedding_pipeline.cpp` | Text-to-vector embedding pipeline; batch generation, model dispatch |
| `content_manager_embedding.cpp` | ContentManager::generateEmbedding() — delegates to EmbeddingPipeline |
| `pipeline/content_chunker.cpp` | Fixed-size and sentence-boundary content chunking |
| `pipeline/multimodal_chunker.cpp` | Multi-modal chunk assembly (text + image + metadata) |
| `pipeline/zstd_compression.cpp` | zstd codec wrapper for content blob compression |
| `pipeline/bulk_upload_interface.cpp` | Bulk-upload request/response types and validation |
| `pipeline/async_bulk_uploader.cpp` | Async worker for batch bulk upload operations |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                    Upload API (src/server/)                      │
│            POST /content/upload  →  ContentManager              │
└──────────────────────────┬──────────────────────────────────────┘
                           │ binary content + metadata
┌──────────────────────────▼──────────────────────────────────────┐
│                    ContentManager                                │
│                                                                  │
│  ┌──────────────┐  ┌───────────────┐  ┌────────────────────┐   │
│  │ MimeDetector │→ │ContentPolicy  │→ │  ContentSecurity   │   │
│  │ (magic bytes)│  │(size/type/depth│  │(bomb detect/AV scan│   │
│  └──────────────┘  └───────────────┘  └────────────────────┘   │
│                                                │                 │
│  ┌─────────────────────────────────────────────▼──────────────┐ │
│  │              Processor Dispatch (by ContentCategory)        │ │
│  └─┬──────────┬──────────┬──────────┬──────────┬─────────────┘ │
│    │          │          │          │          │               │
│  ┌─▼──┐  ┌───▼───┐  ┌───▼──┐  ┌───▼──┐  ┌───▼────┐          │
│  │Text│  │Image  │  │Audio │  │Video │  │Geo/CAD │          │
│  │    │  │       │  │      │  │      │  │        │          │
│  └─┬──┘  └───┬───┘  └───┬──┘  └───┬──┘  └───┬────┘          │
│    └──────────┴──────────┴──────────┴──────────┘              │
│                           │                                    │
│  ┌────────────────────────▼────────────────────────────────┐  │
│  │          AsyncIngestionWorker (background pool)          │  │
│  └────────────────────────┬────────────────────────────────┘  │
└──────────────────────────┬─────────────────────────────────────┘
                           │ structured records + embeddings
          ┌────────────────┴──────────────────┐
          │                                   │
   src/storage/                        src/index/ (LLM embeddings)
```

---

## 4. Data Flow

```
Client: POST /content/upload  (binary body, Content-Type header)
    │
    ▼
MimeDetector: magic bytes + libmagic → MIME type
    │
    ▼
ContentPolicy: allowed type? size within limit? archive depth ok?
    │ reject → 415 Unsupported Media Type / 413 Too Large
    │
    ▼
ContentSecurity: archive bomb scan / malware signature check
    │ threat detected → 400 Bad Request + security log
    │
    ▼
ContentValidator: format validation (JSON schema, image decode, etc.)
    │
    ▼
AsyncIngestionWorker: enqueue for background processing
    │
    ▼ (async)
Processor (Text/Image/Audio/Video/Geo/CAD/Archive/PDF/Office)
    ├─ extract text / metadata / embeddings
    ├─ generate SHA-256 hash (deduplication)
    └─ write to storage + index
    │
    ▼
ContentManager: update status → notify caller (webhook or polling)
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Receives uploads from** | `src/server/` | Content upload API handlers |
| **Sends to** | `src/storage/` | Persists processed content |
| **Sends to** | `src/index/` | Registers content for full-text/vector search |
| **Uses** | `src/llm/` | Embedding generation, OCR assistance |
| **Uses** | `src/search/` | Full-text index integration |
| **Uses** | `src/observability/` | Content metrics and tracing |

---

## 6. Threading & Concurrency Model

- `ContentManager` processes synchronously up to the security/validation stage; processing
  is then handed off to the `AsyncIngestionWorker` thread pool.
- `AsyncIngestionWorker` uses a configurable thread pool (default: 8 workers).
- Individual processors are stateless and safe for concurrent invocation.
- `content_metrics.cpp` uses lock-free atomic counters.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Async processing | Upload response is immediate; processing happens in background |
| zstd compression | Processed text content is compressed before storage |
| SHA-256 deduplication | Identical content is stored once; hash-based lookup |
| Chunked processing | Large documents split into chunks to avoid OOM |

---

## 8. Security Considerations

- **Archive bomb protection**: `archive_processor.cpp` enforces max uncompressed size and
  max nesting depth.
- **MIME spoofing prevention**: content type is determined by magic bytes, not the
  `Content-Type` header.
- **Malware scanning**: `content_security.cpp` checks against known malicious file signatures.
- **PII in content**: content is not scanned for PII before storage; a separate PII
  detection step is available via the utils module if enabled.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `content.max_file_size_mb` | 100 | Maximum upload file size |
| `content.allowed_mime_types` | all | Comma-separated MIME type allowlist |
| `content.archive.max_depth` | 5 | Max archive nesting depth |
| `content.archive.max_size_mb` | 1000 | Max uncompressed archive size |
| `content.worker_threads` | 8 | Async ingestion worker pool size |
| `content.dedup_enabled` | true | Enable SHA-256 deduplication |

---

## 10. Error Handling

| Error Type | HTTP Code | Strategy |
|---|---|---|
| Unsupported MIME type | 415 | Reject early; log |
| File too large | 413 | Reject early; log |
| Archive bomb detected | 400 | Reject; log security event |
| Processing failure | 500 | Mark status as failed; log; no retry by default |
| Duplicate content | 200 | Return existing record (deduplication hit) |

---

## 11. Known Limitations & Future Work

- Legacy Office formats (`.doc`/`.xls`/`.ppt` via OLE/Compound Document) are supported via LibreOffice headless fallback (`extractLegacyViaLibreOffice()`, `posix_spawn`, 30 s timeout, RAII temp-file cleanup, sandboxed env) (CON-001 ✅).
- MimeDetector-triggered OCR routing via `ContentPolicy::ocrEnabled()` is wired in `content_manager.cpp` (CON-002 ✅).
- OCR DPI pre-processing (300 DPI rescaling + adaptive binarisation via Leptonica) implemented in `ocr_processor.cpp` (CON-003 ✅).
- Back-pressure for `ingestStream()` (blocking on `max_queue_depth`) is implemented in `async_ingestion_worker.cpp` (CON-005 ✅).
- Zip-bomb protection (`ContentSecurityManager::checkZipBomb()`) is enforced in `archive_processor.cpp`: max 100× decompression ratio, max 1 000 archive entries, called before extraction (CON-006 ✅).
- Video scene detection, subtitle extraction, and keyframe extraction are stub implementations in non-FFmpeg builds.

---

## 12. References

- `src/content/README.md` — module overview
- `src/content/FUTURE_ENHANCEMENTS.md` — roadmap
- `docs/architecture/architecture_content.md` — content architecture (German)
- `docs/architecture/architecture_content_pipeline.md` — pipeline architecture
- `ARCHITECTURE.md` (root) — full system architecture
