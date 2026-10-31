**Author:** ThemisDB Contributors
**Created:** 2026-10-31
**Last Updated:** 2026-10-31
**Status:** active
**Source level:** Level 1 — `include/content/`
**SOT domain:** `api-contract`
**Canonical source references:** [manager API](content_manager.h), [manager implementation](../../src/content/content_manager.cpp), [processor-chain config](processor_chain_config.h), [module architecture](../../src/content/ARCHITECTURE.md)
**Related milestone:** DOC-WEEKLY-2026-44
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** themisdb-doc-orchestrator
**Human_Review:** false
**Human_Author:** ThemisDB Contributors
**Provenance note:** Exact model backend not verifiable from final document text alone.

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: README.md · ../../src/content/ARCHITECTURE.md · ../../src/content/ROADMAP.md · ../../src/content/FUTURE_ENHANCEMENTS.md -->

# Content Module — Public Header Architecture

**Module Path:** `include/content/`
**Implementation:** `../../src/content/`
**Canonical architecture doc:** [`../../src/content/ARCHITECTURE.md`](../../src/content/ARCHITECTURE.md)

---

## 1. Overview

The `include/content/` directory contains the **public C++ header contract** for ThemisDB's
multi-modal content ingestion, processing, and management pipeline. Headers define types,
interfaces, and configuration structures consumed by:

- Internal implementation files in `src/content/`
- The REST/gRPC API layer (`src/server/`, `src/api/`)
- Plugin authors registering custom content processors
- Embedders integrating ThemisDB as a library

Headers define the public interfaces and defaults; most implementation is in `src/content/`.
Some configuration, serialization, and helper methods are inline in public headers.

For full architectural details — data-flow diagrams, threading model, integration point map —
see the canonical document:

→ [`../../src/content/ARCHITECTURE.md`](../../src/content/ARCHITECTURE.md)

---

## 2. Namespace Layout

All public types live under `themis::content` or the pipeline sub-namespace:

| Namespace | Headers | Purpose |
|-----------|---------|---------|
| `themis::content` | `content_manager.h`, `content_processor.h`, `content_type.h`, `content_validator.h`, `content_policy.h`, `content_security.h`, `content_errors.h`, `content_metrics.h`, `content_logger.h`, `content_fs.h`, `content_plugin_interface.h`, `ingestion_plugin.h`, `processor_chain_config.h`, `async_ingestion_worker.h`, `deduplication_checker.h`, `embedding_pipeline.h`, `version_manager.h`, `language_detector.h`, `mime_detector.h`, `content_classifier.h`, `abuse_detector.h`, `pii_redactor.h`, and all format-specific processor headers | Core content lifecycle, processing interfaces, policy, security, observability |
| `themis::content::pipeline` | `pipeline/content_chunker.h`, `pipeline/multimodal_chunker.h`, `pipeline/zstd_compression.h`, `pipeline/bulk_upload_interface.h`, `pipeline/async_bulk_uploader.h` | Pipeline infrastructure: chunking, compression, bulk upload |

---

## 3. Public Type Hierarchy

### 3.1 Content Type System (`content_type.h`)

```
content::ContentCategory          — enum: TEXT / IMAGE / AUDIO / VIDEO / GEO / CAD /
                                           ARCHIVE / STRUCTURED / BINARY / UNKNOWN
content::ContentType              — MIME type descriptor: category, extensions, feature flags
content::ContentType::Features    — geospatial / temporal / hierarchical / versioned / multimodal
```

### 3.2 Core Pipeline (`content_manager.h`)

```
content::ContentMeta              — universal content metadata (UUID, MIME, category, hash, VFS path)
content::ChunkMeta                — per-chunk metadata (seq_num, text, embedding, positional offsets)
content::ContentAssembly          — reconstructed content with lazy-loaded chunks
content::Status                   — lightweight ok/error result
content::ContentManager           — pipeline orchestrator:
    ├── ingestRawBlob()           — single-file ingestion with auto-routing
    ├── ingestStream()            — streaming ingestion for large files (back-pressure aware)
    ├── importContent()           — import pre-processed JSON spec without re-extraction
    ├── getContentMeta()          — retrieve metadata by UUID
    ├── getContentBlob()          — retrieve raw binary blob
    ├── getContentChunks()        — retrieve ordered chunks
    ├── assembleContent()         — reconstruct full content from chunks
    ├── searchContent()           — vector search through the supplied vector-index manager
    ├── searchContentHybrid()     — hybrid vector + fulltext (RRF)
    ├── searchWithExpansion()     — RAG-style graph-expanded search
    ├── deleteContent()           — cascade delete content and all chunks
    ├── resolvePath()             — virtual filesystem path → content UUID
    ├── listDirectory()           — virtual filesystem directory listing
    ├── createDirectory()         — virtual filesystem directory creation
    ├── registerPath()            — register VFS path for existing content
    ├── generateEmbedding()       — embedding for arbitrary text (delegates to EmbeddingPipeline)
    ├── analyzeContent()          — LLM-augmented analysis (requires THEMIS_ENABLE_LLM)
    ├── summarizeContent()        — LLM summary (requires THEMIS_ENABLE_LLM)
    ├── classifyContent()         — LLM classification (requires THEMIS_ENABLE_LLM)
    ├── generateTags()            — LLM tag generation (requires THEMIS_ENABLE_LLM)
    ├── extractEntities()         — LLM entity extraction (requires THEMIS_ENABLE_LLM)
    ├── setEmbeddingPipeline()    — attach EmbeddingPipeline
    ├── setDeduplicationChecker() — attach DeduplicationChecker
    ├── setMalwareFilter()        — attach MalwareFilterManager
    ├── setProcessorChainConfig() — configure per-stage pipeline
    └── getStats() / getMetrics() — statistics and manager-owned atomic counters
content::ContentManager::IngestResult       — ingestion outcome + per-stage diagnostics
content::ContentManager::IngestResult::StageOutcome — per-stage name/success/attempts/error
content::ContentManager::Stats              — total counts by category + storage bytes
content::ContentManager::Metrics            — atomic compression and dedup counters; not a Prometheus serializer
```

### 3.3 Processor Interface (`content_processor.h`)

```
content::ExtractionResult         — extracted text, metadata, embedding, geo/media/CAD data
content::ExtractionResult::GeoData       — coordinates, projection, GeoJSON properties
content::ExtractionResult::MediaData     — duration, dimensions, codec, bitrate
content::ExtractionResult::CADData       — part IDs, bill of materials, bounding dimensions
content::IContentProcessor        — pure-virtual base: extract() / chunk() / generateEmbedding()
content::TextProcessor            — TEXT: sentence chunking, MinHash, configurable EmbeddingFn
content::LegacyImageProcessor     — IMAGE: EXIF metadata, dimensions (legacy path)
content::LegacyGeoProcessor       — GEO: GeoJSON, GPX parsing (legacy path)
content::LegacyAudioProcessor     — AUDIO: ID3 tags, duration (legacy path)
content::LegacyCADProcessor       — CAD: STEP, assembly hierarchy (legacy path)
content::StructuredProcessor      — STRUCTURED: CSV schema extraction
content::BinaryProcessor          — BINARY/ARCHIVE/UNKNOWN: hash + size fallback
```

### 3.4 Plugin and Chain Configuration

```
content::IContentPlugin           — plugin extension interface (content_plugin_interface.h)
content::IIngestionPlugin         — ingestion-stage plugin: process() / processChunk() (ingestion_plugin.h)
content::ProcessorChainConfig     — per MIME-type / per-category stage enable/disable map (processor_chain_config.h)
```

### 3.5 Policy, Validation, and Security

```
content::ContentPolicy            — configurable gates: allowed MIME types, max size, OCR, archive depth, dedup (content_policy.h)
content::ContentValidator         — pre-ingestion format validation (content_validator.h)
content::ContentSecurity / ContentSecurityManager — archive bomb check, malware scan (content_security.h)
content::AbuseDetector / PhotoDNAAbuseDetector / TextAbuseDetector — harmful-content detection (abuse_detector.h)
content::PIIRedactor              — PII redaction (pii_redactor.h)
```

### 3.6 Deduplication and Embedding

```
content::DeduplicationChecker     — SHA-256 exact dedup; pHash (images); MinHash+band-LSH (text) (deduplication_checker.h)
content::EmbeddingPipeline        — batch text/image embedding generation, model dispatch (embedding_pipeline.h)
content::TextProcessor::EmbeddingFn — injected embedding backend callback type
```

### 3.7 Observability

```
content::ContentMetrics           — Prometheus ingestion counters and format histograms (content_metrics.h)
content::ContentLogger            — structured content audit logger (content_logger.h)
content::ContentFS                — virtual filesystem abstraction (content_fs.h)
content::VersionManager           — revision history and rollback (version_manager.h)
content::ContentError / ContentException — typed error hierarchy (content_errors.h)
```

### 3.8 Pipeline Sub-namespace (`themis::content::pipeline`)

```
pipeline::ContentChunker / ChunkConfig   — fixed-size byte chunking with overlap (pipeline/content_chunker.h)
pipeline::MultiModalChunker              — multi-modal chunk assembly (pipeline/multimodal_chunker.h)
pipeline::ZstdCompression                — zstd codec wrapper, levels 1–22 (pipeline/zstd_compression.h)
pipeline::BulkUploadInterface            — batch upload base interface (pipeline/bulk_upload_interface.h)
pipeline::AsyncBulkUploader              — production async batch uploader (pipeline/async_bulk_uploader.h)
```

---

## 4. Build Conditionals

Headers that require optional compile-time dependencies are guarded as follows:

| CMake Symbol | Components Affected | Required Dependency |
|---|---|---|
| `THEMIS_ENABLE_OCR` | `ocr_processor.h` — `OCRProcessor` | Tesseract ≥ 4.1, Leptonica |
| `THEMIS_ENABLE_PDF` | `pdf_processor.h` — `PDFProcessor` | poppler-cpp |
| `THEMIS_HAS_FFMPEG` | `video_processor.h` — FFmpeg-backed paths | CMake defines this when FFmpeg is detected |
| `THEMIS_ENABLE_LLM` | `content_manager.h` — LLM analysis methods | LLM plugin (`src/llm/`) |
| `THEMIS_HAS_ZSTD` | `pipeline/zstd_compression.h` | libzstd |

Legacy DOC/XLS/PPT extraction may use the runtime path in
`OfficeProcessor::Config::libreoffice_path`; the current CMake files do not define a
`THEMIS_ENABLE_LIBREOFFICE` option.

---

## 5. Header Dependencies

The dependency graph between `include/content/` headers is intentionally shallow:

```
content_manager.h
  ├── content_type.h
  ├── content_processor.h
  │     └── content_type.h
  ├── deduplication_checker.h
  ├── embedding_pipeline.h
  ├── mime_detector.h
  └── processor_chain_config.h

content_type.h
  └── (no include/content/ dependencies; uses nlohmann/json + standard library)

All format-specific processor headers (pdf_processor.h, image_processor.h, …)
  └── content_processor.h (or no include/content/ dependencies beyond content_type.h)

pipeline/async_bulk_uploader.h
  └── pipeline/bulk_upload_interface.h
```

---

## 6. Compatibility and Configuration Notes

- **API/ABI policy:** Follow repository [`VERSIONING.md`](../../VERSIONING.md) and the
  declarations in the relevant header; this overview does not assert an independent ABI
  guarantee.
- **Stage defaults:** `StageConfig` defaults to enabled, zero retries, a 100 ms retry
  delay, and `continue_on_error == false`.
- **Configuration precedence:** `ProcessorChainConfig` selects one complete entry in this
  order: exact MIME type, category, then global default. `ContentManager::setProcessorChainConfig()`
  replaces the stored configuration; it does not merge partial entries.
- **Thread safety:** No module-wide thread-safety guarantee is asserted here. Consult each
  API's implementation and caller synchronization requirements.

---

## 7. References

- Full architecture: [`../../src/content/ARCHITECTURE.md`](../../src/content/ARCHITECTURE.md)
- Module overview: [`../../src/content/README.md`](../../src/content/README.md)
- Roadmap: [`../../src/content/ROADMAP.md`](../../src/content/ROADMAP.md)
- Future enhancements: [`../../src/content/FUTURE_ENHANCEMENTS.md`](../../src/content/FUTURE_ENHANCEMENTS.md)
- Security: [`../../src/content/SECURITY.md`](../../src/content/SECURITY.md)
- Public header overview: [`README.md`](README.md)
