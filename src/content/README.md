> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · SECURITY.md · CHANGELOG.md · PERFORMANCE_EXPECTATIONS.md · ../../include/content/README.md -->

# Content Module

Content management, ingestion, and processing implementation for ThemisDB.

## Module Purpose

Provides multi-format content ingestion and processing for ThemisDB, handling JSON documents,
images, PDF, Office files, audio, video, CAD, geospatial data, archives, and raw binary blobs.
Transforms binary uploads into structured records that can be stored, searched, and embedded
in the database.

## Subsystem Scope

**In scope:** Multi-format content ingestion (JSON, images, PDF, Office OOXML/ODF, legacy
Office via LibreOffice headless, HTML, Markdown, audio, video, CAD, geo, archives), MIME type
detection (magic bytes + libmagic + YAML config), text extraction and chunking, image metadata
extraction (EXIF, pHash), OCR (Tesseract), audio transcription (STT/Whisper), video metadata
(FFmpeg), geospatial normalization, content deduplication (SHA-256 + pHash + MinHash), embedding
generation, LLM-augmented content analysis, async multi-threaded ingestion, zstd compression,
virtual filesystem abstraction, content versioning.

**Out of scope:** Full-text indexing (handled by `src/search/`), acceleration/CUDA kernels
(handled by `src/acceleration/`), authentication/access control (handled by `src/auth/`).

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — Core content ingestion, PDF (poppler-cpp), Office OOXML/ODF
(libzip+pugixml), legacy Office formats (.doc/.xls/.ppt via LibreOffice headless, CON-001),
OCR (Tesseract, CON-002/CON-003), streaming ingestion with back-pressure (CON-005), archive
bomb protection (CON-006), perceptual deduplication, and embedding pipeline are all operational.

---

## Components

### Core Pipeline

| File | Role |
|------|------|
| `content_manager.cpp` | Pipeline orchestrator: MIME detect → policy → security → route to processor → storage + index |
| `content_manager_llm.cpp` | LLM-augmented extraction: summary, topics, sentiment, category (`THEMIS_ENABLE_LLM`) |
| `content_manager_embedding.cpp` | `ContentManager::generateEmbedding()` — delegates to `EmbeddingPipeline` |
| `async_ingestion_worker.cpp` | Background worker thread pool; back-pressure via `max_queue_depth`; batch and streaming jobs |
| `ingestion_plugin.cpp` | Plugin interface for custom content type processors (idempotent registration) |

### Content Type Detection and Classification

| File | Role |
|------|------|
| `mime_detector.cpp` | MIME type detection: magic bytes + libmagic + YAML config (`config/data_management/mime_types.yaml`) |
| `content_type.cpp` | MIME → `ContentCategory` mapping (`TEXT`, `IMAGE`, `AUDIO`, `VIDEO`, `GEO`, `CAD`, `ARCHIVE`, `STRUCTURED`, `BINARY`) |
| `language_detector.cpp` | Multi-language text detection and routing |
| `content_classifier.cpp` | (via header) ML-based content classification |

### Validation, Policy, and Security

| File | Role |
|------|------|
| `content_policy.cpp` | Configurable ingestion policy: allowed MIME types, max sizes, archive depth, OCR activation |
| `content_validator.cpp` | Pre-ingestion format validation (JSON schema, image decode, dimension checks) |
| `content_security.cpp` | Archive bomb detection (max 100× ratio, max 1 000 entries), malware signature scanning |
| `abuse_detector.cpp` | `PhotoDNAAbuseDetector` (CSAM hashes) and `TextAbuseDetector` (blocklist + regex patterns) |

### Format-Specific Processors

| File | Role |
|------|------|
| `text_processor.cpp` | Text extraction, sentence chunking, MinHash near-duplicate detection |
| `html_processor.cpp` | HTML extraction with boilerplate removal |
| `markdown_processor.cpp` | Markdown parsing and frontmatter extraction |
| `pdf_processor.cpp` | PDF text extraction via poppler-cpp (layout-preserving, per-page; `THEMIS_ENABLE_PDF`) |
| `office_processor.cpp` | Office extraction: DOCX/XLSX/PPTX (libzip+pugixml), ODF, legacy DOC/XLS/PPT (LibreOffice headless, CON-001) |
| `image_processor.cpp` | Image metadata (EXIF, dimensions, colour space), pHash perceptual deduplication |
| `ocr_processor.cpp` | Tesseract OCR; 300 DPI rescaling + Sauvola binarization via Leptonica (CON-002/CON-003; `THEMIS_ENABLE_OCR`) |
| `audio_processor.cpp` | Audio metadata extraction (ID3, duration, codec) |
| `stt_processor.cpp` | Speech-to-text integration (Whisper backend) |
| `tts_processor.cpp` | Text-to-speech generation (Piper backend) |
| `video_processor.cpp` | Video metadata extraction; frame extraction via FFmpeg (`THEMIS_ENABLE_FFMPEG`) |
| `geo_processor.cpp` | GeoJSON normalization, GPX, coordinate extraction |
| `cad_processor.cpp` | CAD file metadata extraction (DXF/DWG/STEP, assembly hierarchy) |
| `archive_processor.cpp` | ZIP/TAR extraction with bomb protection (CON-006) |
| `mock_clip_processor.cpp` | Mock CLIP embedding processor (test / no-GPU path only) |

### Deduplication and Embeddings

| File | Role |
|------|------|
| `deduplication_checker.cpp` | SHA-256 exact dedup; pHash (images, `isDuplicateImage()`); MinHash+band-LSH (text, `isDuplicateText()`) |
| `embedding_pipeline.cpp` | Text/image embedding generation: batch API, model dispatch, L2-normalised vectors |

### Observability and Utilities

| File | Role |
|------|------|
| `content_metrics.cpp` | Prometheus-style counters: ingestion throughput, error rates, format breakdown, compression ratios |
| `content_errors.cpp` | Typed content error hierarchy (`ContentError`, `ContentException`) |
| `content_fs.cpp` | Filesystem utilities: temp file management, VFS path resolution |
| `content_logger.cpp` | Structured content audit logger |
| `content_policy.cpp` | Retention, access, and processing policy enforcement |
| `version_manager.cpp` | Content version history: track revisions, delta storage, rollback |

### Pipeline Utilities (`pipeline/`)

| File | Role |
|------|------|
| `pipeline/content_chunker.cpp` | Fixed-size and overlap byte-based chunking |
| `pipeline/multimodal_chunker.cpp` | Multi-modal chunk assembly (text + image + metadata) for RAG pipelines |
| `pipeline/zstd_compression.cpp` | zstd codec wrapper (levels 1–22, max 1 GB input, 4 GB decompressed) |
| `pipeline/bulk_upload_interface.cpp` | Bulk-upload request/response types and validation |
| `pipeline/async_bulk_uploader.cpp` | Async worker for batch bulk upload operations |

### Adapters (`adapters/`)

| File | Role |
|------|------|
| `adapters/pdf_extractor_adapter.cpp` | PDF extractor adapter for pipeline integration |
| `adapters/office_extractor_adapter.cpp` | Office extractor adapter |
| `adapters/audio_extractor_adapter.cpp` | Audio extractor adapter |
| `adapters/image_extractor_adapter.cpp` | Image extractor adapter |
| `adapters/archive_extractor_adapter.cpp` | Archive extractor adapter |
| `adapters/text_extractor_adapter.cpp` | Text extractor adapter |
| `adapters/format_extractor_factory.cpp` | Factory: instantiates the correct extractor adapter by MIME type |

---

## Public API Reference

The primary entry point is `ContentManager`. Include it via:

```cpp
#include "content/content_manager.h"
using namespace themis::content;
```

### Ingestion

| Method | Signature | Description |
|--------|-----------|-------------|
| `ingestRawBlob` | `IngestResult ingestRawBlob(blob, filename, mime_type="", user_context="", config={})` | Ingest a single binary blob; auto-detects MIME type; handles archives recursively |
| `ingestStream` | `IngestResult ingestStream(stream, filename, mime_type="", user_context="", config={})` | Streaming ingestion for large files; back-pressure aware (CON-005) |
| `importContent` | `Status importContent(spec, blob=nullopt, user_context="")` | Import pre-processed JSON spec `{ content: {...}, chunks: [...], edges?: [...] }` without re-extraction |

### Retrieval

| Method | Signature | Description |
|--------|-----------|-------------|
| `getContentMeta` | `optional<ContentMeta> getContentMeta(content_id, user_context="")` | Retrieve content metadata by UUID |
| `getContentBlob` | `optional<string> getContentBlob(content_id, user_context="")` | Retrieve raw binary blob by UUID |
| `getContentChunks` | `vector<ChunkMeta> getContentChunks(content_id)` | All chunks for a content item, ordered by `seq_num` |
| `getChunk` | `optional<ChunkMeta> getChunk(chunk_id)` | Single chunk by UUID |
| `getChunkRange` | `vector<ChunkMeta> getChunkRange(content_id, start_seq, count)` | Paginated chunk range |
| `getNextChunk` | `optional<ChunkMeta> getNextChunk(chunk_id)` | Next chunk in sequence |
| `getPreviousChunk` | `optional<ChunkMeta> getPreviousChunk(chunk_id)` | Previous chunk in sequence |
| `assembleContent` | `optional<ContentAssembly> assembleContent(content_id, include_text=false)` | Reconstruct full content with lazy-loaded chunks |
| `deleteContent` | `Status deleteContent(content_id)` | Cascade delete content and all chunks |

### Search

| Method | Signature | Description |
|--------|-----------|-------------|
| `searchContent` | `vector<pair<string,float>> searchContent(query_text, k, filters={})` | Vector (HNSW) similarity search |
| `searchContentHybrid` | `vector<pair<string,float>> searchContentHybrid(query_text, k, filters={}, vector_weight=0.5, fulltext_weight=0.5, rrf_k=60)` | Hybrid vector + fulltext (BM25) with Reciprocal Rank Fusion |
| `searchWithExpansion` | `vector<pair<string,float>> searchWithExpansion(query_text, k, expansion_hops, filters={})` | RAG-style search with graph expansion to neighbours |

### Virtual Filesystem

| Method | Signature | Description |
|--------|-----------|-------------|
| `resolvePath` | `optional<string> resolvePath(virtual_path)` | Resolve VFS path like `/documents/report.pdf` → content UUID |
| `listDirectory` | `vector<ContentMeta> listDirectory(virtual_path)` | List VFS directory children |
| `createDirectory` | `Status createDirectory(virtual_path, recursive=false)` | Create VFS directory |
| `registerPath` | `Status registerPath(content_id, virtual_path)` | Register VFS path for existing content |

### Embeddings and LLM Analysis

| Method | Signature | Description |
|--------|-----------|-------------|
| `generateEmbedding` | `vector<float> generateEmbedding(text, model_name="")` | Generate L2-normalised embedding; delegates to `EmbeddingPipeline` if set |
| `setEmbeddingPipeline` | `void setEmbeddingPipeline(pipeline)` | Attach embedding pipeline; auto-embeds text chunks on import |
| `analyzeContent` | `json analyzeContent(content_id)` | LLM-augmented analysis: summary, topics, sentiment, category (`THEMIS_ENABLE_LLM`) |
| `summarizeContent` | `string summarizeContent(content_id, max_words=100)` | LLM summary (`THEMIS_ENABLE_LLM`) |
| `classifyContent` | `string classifyContent(content_id)` | LLM classification (`THEMIS_ENABLE_LLM`) |
| `generateTags` | `vector<string> generateTags(content_id, max_tags=10)` | LLM tag generation (`THEMIS_ENABLE_LLM`) |
| `extractEntities` | `json extractEntities(content_id)` | LLM entity extraction (`THEMIS_ENABLE_LLM`) |

### Configuration

| Method | Signature | Description |
|--------|-----------|-------------|
| `setProcessorChainConfig` | `void setProcessorChainConfig(config)` | Per-stage pipeline config (per MIME type or `ContentCategory`) |
| `getProcessorChainConfig` | `const ProcessorChainConfig& getProcessorChainConfig()` | Get current chain config |
| `setDeduplicationChecker` | `void setDeduplicationChecker(checker)` | Attach dedup checker; `ingestRawBlob()` auto-checks near-duplicates |
| `setMalwareFilter` | `void setMalwareFilter(filter)` | Attach malware scanner (BSI C5 / ISO 27001 compliance) |
| `registerProcessor` | `void registerProcessor(processor)` | Register a custom `IContentProcessor` for a `ContentCategory` |

### Observability

| Method | Signature | Description |
|--------|-----------|-------------|
| `getStats` | `Stats getStats()` | Total content items, chunks, embeddings, storage bytes by category |
| `getMetrics` | `const Metrics& getMetrics()` | Atomic Prometheus counters (compression, dedup, error rates) |

---

## Configuration Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `content.max_file_size_mb` | 100 | Maximum upload file size in MB |
| `content.allowed_mime_types` | all | Comma-separated MIME type allowlist (`*` = all) |
| `content.archive.max_depth` | 5 | Maximum archive nesting depth |
| `content.archive.max_size_mb` | 1000 | Maximum uncompressed archive size in MB |
| `content.worker_threads` | 8 | Async ingestion worker pool size |
| `content.max_queue_depth` | 256 | Back-pressure threshold for `ingestStream()` |
| `content.dedup_enabled` | true | Enable SHA-256 + perceptual deduplication |
| `content.ocr_enabled` | false | Enable OCR for image content (requires `THEMIS_ENABLE_OCR`) |
| `content.chunk_size_bytes` | 4 MB | Read chunk size for streaming ingestion |
| `content.max_buffered_bytes` | 256 MB | Max buffer for non-streaming types in `ingestStream()` |

OCR language-pack data directory defaults to `config/ai_ml/tesseract_lang/` (CON-004).
MIME type registry loaded from `config/data_management/mime_types.yaml`.

---

## Runtime Behavior

### Ingestion Pipeline (synchronous stages)

1. **MIME Detection** — `MimeDetector` identifies content type from magic bytes + libmagic + YAML config; `Content-Type` header is **not** trusted.
2. **Policy Enforcement** — `ContentPolicy` checks: MIME allowlist, file size limit, archive depth limit; rejects with typed error on violation.
3. **Security Scan** — `ContentSecurityManager` checks decompression ratio (max 100×) and archive entry count (max 1 000) for archives; malware signature scan if `MalwareFilterManager` is attached.
4. **Format Validation** — `ContentValidator` validates format: JSON schema, image decodability, dimension bounds.
5. **Queue for Async Processing** — `AsyncIngestionWorker` enqueues the job; `ingestRawBlob()` returns immediately; `ingestStream()` blocks when `max_queue_depth` is reached.

### Ingestion Pipeline (async stages, background)

6. **Processor Dispatch** — `ContentManager` routes to the appropriate `IContentProcessor` or `IIngestionPlugin` by `ContentCategory`.
7. **Extraction** — Processor extracts text, metadata, and type-specific data into `ExtractionResult`.
8. **Deduplication** — If `DeduplicationChecker` is attached: pHash check for images; MinHash+band-LSH check for text.
9. **Chunking** — Processor chunks extracted content with configured overlap.
10. **Embedding** — `EmbeddingPipeline` (if set) generates normalised embedding vectors for each chunk.
11. **Storage** — Metadata written to RocksDB (`content:<uuid>`); chunks written (`chunk:<uuid>`); blob stored (`content_blob:<uuid>`).
12. **Index Registration** — Chunks registered in `VectorIndexManager` (HNSW) and `GraphIndexManager` (chunk relationships).
13. **Status Update** — `ContentManager` marks ingestion as complete; webhook or polling notifies caller.

---

## Error Handling

| Error Type | HTTP Equivalent | Strategy |
|-----------|----------------|----------|
| Unsupported MIME type | 415 | Reject early; log; `ContentError::UNSUPPORTED_TYPE` |
| File too large | 413 | Reject early; log; `ContentError::FILE_TOO_LARGE` |
| Archive bomb detected | 400 | Reject; security log; `ContentError::SECURITY_VIOLATION` |
| Format validation failure | 422 | Reject; log; `ContentError::VALIDATION_FAILURE` |
| Near-duplicate detected | 200 | Return existing `content_id`; no storage |
| Processing failure | 500 | Mark status failed; log; no automatic retry by default |
| Worker queue full | 429 | `ingestStream()` blocks until queue drains; async callers receive back-pressure signal |

---

## Known Limitations

- **Video scene detection / subtitle / keyframe extraction** — stub implementations exist for non-FFmpeg builds; full extraction requires `THEMIS_ENABLE_FFMPEG=ON` (Issue #1688, Target Q4 2026).
- **Streaming ingestion for non-streaming types** — PDF, images, and binary formats are buffered up to `max_buffered_bytes` (default 256 MB) before delegation to `ingestRawBlob`; files exceeding the limit are rejected.
- **Configurable processor chain (plugin API)** — `ProcessorChainConfig` stage enable/disable is wired; dynamic runtime plugin registration via `IIngestionPlugin` is planned (Issue #1686, Target Q3 2026).
- **AsyncIngestionWorker YAML config** — worker pool configuration currently hardcoded; YAML loading planned (see `FUTURE_ENHANCEMENTS.md`).
- **`PhotoDNAAbuseDetector`** — implemented; only tested via content security tests; production integration with live hash database is a planned follow-up.

---

## Usage Examples

### Basic File Ingestion

```cpp
#include "content/content_manager.h"
using namespace themis::content;

auto manager = std::make_shared<ContentManager>(
    storage, vector_index, graph_index, secondary_index
);

std::string pdf_bytes = read_file("report.pdf");
auto result = manager->ingestRawBlob(pdf_bytes, "report.pdf", "application/pdf", "user-123");

if (result.success) {
    auto meta = manager->getContentMeta(result.primary_content_id);
    auto chunks = manager->getContentChunks(result.primary_content_id);
}
```

### Streaming Ingestion for Large Files

```cpp
#include "content/content_manager.h"
#include <fstream>

std::ifstream stream("large_dataset.ndjson", std::ios::binary);
nlohmann::json config = {
    {"chunk_size_bytes", 4 * 1024 * 1024},   // 4 MB read chunks
    {"chunk_size", 512}                        // 512-character text segments
};
auto result = manager->ingestStream(stream, "large_dataset.ndjson", "", "user-123", config);
```

### Semantic and Hybrid Search

```cpp
// Pure vector search
auto hits = manager->searchContent("vector database architecture", 10);

// Hybrid vector + BM25 with RRF
auto hybrid = manager->searchContentHybrid("vector database architecture", 10,
    nlohmann::json::object(), 0.6f, 0.4f, 60.0f);

// RAG with graph expansion
auto rag = manager->searchWithExpansion("vector database architecture", 5, 2);
```

### Custom Processor Registration

```cpp
#include "content/ingestion_plugin.h"

class MyXMLProcessor : public IIngestionPlugin {
public:
    ContentProcessResult process(std::span<const std::byte> data,
                                  const ContentMetadata& meta) override { /* ... */ }
    std::string getMimeType() const override { return "application/xml"; }
};

manager->registerProcessor(std::make_unique<MyXMLProcessor>());
```

### Async Batch Upload

```cpp
#include "content/pipeline/async_bulk_uploader.h"
using namespace themis::content::pipeline;

AsyncBulkUploader uploader(manager, {.worker_threads = 4});
uploader.set_progress_callback([](const std::string& id, size_t done, size_t total) {
    // progress reporting
});

std::vector<std::vector<uint8_t>> files = load_batch();
std::vector<BulkUploadInterface::ContentMetadata> metas = build_metadata();
auto results = uploader.bulk_upload(files, metas);
```

---

## Troubleshooting

| Symptom | Likely Cause | Resolution |
|---------|-------------|------------|
| `ingestRawBlob()` returns `success = false`, "Unsupported MIME type" | MIME not in allowlist or undetectable | Add MIME type to `config/data_management/mime_types.yaml` |
| PDF extraction fails | Built without `THEMIS_ENABLE_PDF` | Rebuild with `-DTHEMIS_ENABLE_PDF=ON` and install poppler-cpp |
| OCR not triggered on image content | `ContentPolicy::ocrEnabled()` is false | Enable OCR in content policy; set `-DTHEMIS_ENABLE_OCR=ON` |
| `ingestStream()` blocks permanently | Worker queue at `max_queue_depth` | Increase `max_queue_depth` in `AsyncIngestionConfig` or reduce ingestion rate |
| Archive rejected as zip bomb | Decompression ratio > 100× or > 1 000 entries | Split archive or increase `content.archive.max_size_mb` limit |
| LLM analysis methods return empty JSON | Built without `THEMIS_ENABLE_LLM` | Rebuild with `-DTHEMIS_ENABLE_LLM=ON` and configure the LLM backend |
| LibreOffice extraction fails | LibreOffice not installed or `THEMIS_ENABLE_LIBREOFFICE` not set | Install LibreOffice runtime; rebuild with `-DTHEMIS_ENABLE_LIBREOFFICE=ON` |
| `generateEmbedding()` returns empty | No `EmbeddingPipeline` attached | Call `manager->setEmbeddingPipeline(pipeline)` during setup |
| Near-duplicate content re-ingested | `DeduplicationChecker` not attached | Call `manager->setDeduplicationChecker(checker)` during setup |

---

## Scientific References

1. Robertson, S., & Zaragoza, H. (2009). **The Probabilistic Relevance Framework: BM25 and Beyond**. *Foundations and Trends in Information Retrieval*, 3(4), 333–389. https://doi.org/10.1561/1500000019

2. Salton, G., & McGill, M. J. (1983). **Introduction to Modern Information Retrieval**. McGraw-Hill. ISBN: 978-0-070-54484-0

3. Dublin Core Metadata Initiative. (2012). **DCMI Metadata Terms**. DCMI Recommendation. https://www.dublincore.org/specifications/dublin-core/dcmi-terms/

4. W3C. (2013). **PROV-O: The PROV Ontology**. W3C Recommendation. https://www.w3.org/TR/prov-o/

---

## Documentation

- **Architecture:** [`ARCHITECTURE.md`](ARCHITECTURE.md)
- **Roadmap:** [`ROADMAP.md`](ROADMAP.md)
- **Future enhancements:** [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- **Security:** [`SECURITY.md`](SECURITY.md)
- **Audit:** [`AUDIT.md`](AUDIT.md)
- **Changelog:** [`CHANGELOG.md`](CHANGELOG.md)
- **Performance:** [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
- **Pipeline README:** [`pipeline/README.md`](pipeline/README.md)
- **Public API headers:** [`../../include/content/README.md`](../../include/content/README.md)
- **Public header architecture:** [`../../include/content/ARCHITECTURE.md`](../../include/content/ARCHITECTURE.md)
- **German developer docs:** [`../../docs/de/content/`](../../docs/de/content/)

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
