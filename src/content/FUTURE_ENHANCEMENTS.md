# Content Module - Future Enhancements

## Scope

This document covers implementation-specific future enhancements for the Content module (`src/content/`), which provides multi-format content ingestion through `content_manager.cpp` (1,948 lines), MIME detection via `mime_detector.cpp`, and a family of format-specific processors: `text_processor.cpp`, `image_processor.cpp`, `pdf_processor.cpp`, `office_processor.cpp`, `geo_processor.cpp`, `audio_processor.cpp`, `video_processor.cpp`, `archive_processor.cpp`, and `cad_processor.cpp`. Enhancements to downstream vector embedding generation (`acceleration/`) and AQL query execution are out of scope; this document focuses on the ingestion pipeline, format support, and content processing performance.

## Design Constraints

- `[ ]` The `ContentManager` ingestion pipeline routes by `ContentCategory` enum (`TEXT`, `IMAGE`, `GEO`, `CAD`, `AUDIO`, `STRUCTURED`, `BINARY`); new format processors must register via the existing `ingestion_plugin.cpp` plugin interface and must not add routing logic to `content_manager.cpp` directly.
- `[ ]` `mime_detector.cpp` loads its type registry from `config/data_management/mime_types.yaml` via `ConfigPathResolver::resolve()`; new MIME types must be added to the YAML config, not hardcoded in `MimeDetector`.
- `[ ]` `utils/zstd_codec.h` is the sole compression codec in the pipeline; alternative codecs (LZ4, Brotli) may be added as new `ICompressionCodec` implementations but must not replace zstd as the default without a migration path.
- `[ ]` `content_security.cpp` and `content_validator.cpp` are applied to all content before storage; new processors must not bypass these stages.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `ContentManager::ingest(data, metadata)` | REST API ingestion endpoint, `async_ingestion_worker.cpp` | Returns `ContentId`; must remain the single entry point |
| `MimeDetector::detect(data, filename_hint)` | `ContentManager::ingest()` | YAML-driven; reload via `MimeDetector::reloadConfig()` without restart |
| `ingestion_plugin.cpp` `IIngestionPlugin` interface | Format processors | Plugin registration must be idempotent; plugins load via `base/module_loader.cpp` |
| `content_validator.cpp` `ContentValidator::validate()` | All processors pre-storage | Validation failures must surface typed errors from `content_errors.cpp` |
| `async_ingestion_worker.cpp` | Large-file and batch ingestion | Worker pool size configurable; back-pressure signalled to callers |
| `version_manager.cpp` | Content update/versioning path | Delta storage and rollback must integrate with `ContentManager::update()` |

## Planned Features

### PDF and Office Document Text Extraction
**Priority:** High
**Target Version:** v1.7.0

`pdf_processor.cpp` and `office_processor.cpp` exist as stubs. Implement full text extraction for PDF (using PDFium or pdfmium), DOCX/XLSX/PPTX (using a libzip-based OOXML parser), and legacy `.doc`/`.xls` (via LibreOffice headless subprocess), with layout-preserving paragraph segmentation.

**Implementation Notes:**
- `[ ]` PDF: integrate `pdfium` (static linkage preferred) in `pdf_processor.cpp`; extract text per page with `FPDF_GetPageText`; preserve page number as metadata field `page_number` in the output JSON.
- `[ ]` DOCX: unzip `.docx` with `libzip`; parse `word/document.xml` extracting `<w:t>` nodes; preserve heading levels as `{"heading": 2, "text": "..."}` blocks for structured retrieval.
- `[ ]` XLSX: extract cell values from `xl/worksheets/sheet*.xml`; return as JSON array-of-arrays; cap at 1,000 rows × 100 columns to prevent memory exhaustion.
- `[ ]` LibreOffice headless fallback (`.doc`, `.ppt`, `.xls`): spawn `soffice --headless --convert-to txt` in a sandboxed subprocess with a 30 s timeout; clean up temp files on completion or timeout.
- `[ ]` All processors must report `content_metrics.cpp` counters: `content_pdf_extracted_total`, `content_office_extracted_total`, `content_extract_errors_total`.

**Performance Targets:**
- PDF extraction: 100-page, 500 KB PDF in < 2 s on a single CPU core.
- DOCX extraction: 500 KB document in < 200 ms.
- LibreOffice subprocess: spawned and completed in < 30 s; subprocess pool of 2 pre-warmed instances to avoid cold-start penalty.

**API Sketch:**
```cpp
// pdf_processor.cpp — completed interface
class PDFProcessor : public IIngestionPlugin {
public:
    ContentProcessResult process(
        std::span<const std::byte> data,
        const ContentMetadata& meta) override;
    // Returns structured JSON: {"pages": [{"page": 1, "text": "..."}, ...]}
};
```

---

### Streaming Ingestion for Large Files
**Priority:** High
**Target Version:** v1.7.0

Currently `ContentManager::ingest()` buffers the entire content in memory before processing. Files larger than `config_.max_content_size_bytes` are rejected. Implement chunked streaming ingestion in `async_ingestion_worker.cpp` that processes content in configurable chunks, enabling ingestion of files up to several GB.

**Implementation Notes:**
- `[ ]` Add `ContentManager::ingestStream(std::istream& stream, const ContentMetadata& meta)` overload.
- `[ ]` `async_ingestion_worker.cpp` reads chunks of `chunk_size_bytes` (default: 4 MB, configurable) from the stream; each chunk is processed by the appropriate `IIngestionPlugin::processChunk()` method.
- `[ ]` Processors that support streaming (text, CSV, NDJSON) implement `processChunk()`; processors that require full data (PDF, image) buffer up to a configurable `max_buffered_bytes` limit (default: 256 MB) before falling back to error.
- `[ ]` Back-pressure: `ingestStream()` blocks the caller when the worker queue depth exceeds `config_.max_queue_depth`; returns a `std::future<ContentId>` for async callers.
- `[ ]` Partial failure: if a chunk fails validation in `content_validator.cpp`, the entire ingestion transaction is rolled back and the partial content is purged from storage.

**Performance Targets:**
- 1 GB NDJSON file ingested at ≥ 100 MB/s sustained throughput on NVMe storage.
- Peak RSS increase during streaming ingestion < 2× `chunk_size_bytes` (i.e., two chunks in-flight at most).

---

### Content Deduplication via Perceptual Hashing
**Priority:** Medium
**Target Version:** v1.8.0

Exact duplicate detection (SHA-256 of raw bytes) is already performed in `content_manager.cpp`. Add near-duplicate detection using perceptual hashing (pHash for images, MinHash for text documents) to reject semantically identical content before storage.

**Implementation Notes:**
- `[ ]` Images: compute pHash (DCT-based 64-bit hash) in `image_processor.cpp` using a pure C++ implementation (no OpenCV dependency); store hash in content metadata as `phash_hex`.
- `[ ]` Text documents: compute MinHash signature (128 hash functions, Jaccard threshold 0.85) in `text_processor.cpp`; use a band LSH index stored in `cache::BoundedLRUCache` for fast lookup.
- `[ ]` `ContentManager::ingest()` calls `DeduplicationChecker::isDuplicate(content_id, phash_or_minhash)` before committing; returns `DuplicateOf{existing_id}` if a near-duplicate is found.
- `[ ]` Deduplication is opt-in per collection via `ContentPolicy` in `content_policy.cpp`; default off.
- `[ ]` Expose `content_dedup_hits_total` and `content_dedup_checks_total` Prometheus counters.

**Performance Targets:**
- pHash computation for a 4 MP JPEG in < 5 ms.
- MinHash + LSH lookup for a 10 KB text document in < 1 ms (with warm band index of 100K entries).
- Near-duplicate detection adds < 10% overhead to total ingestion latency when deduplication is enabled.

---

### OCR for Image-Embedded Text (Tesseract Integration)
**Priority:** Medium
**Target Version:** v1.8.0

`image_processor.cpp` extracts EXIF/IPTC metadata but does not extract embedded text from scanned documents or diagrams. Integrate Tesseract OCR as an optional plugin loaded at runtime; fall back gracefully if Tesseract is not installed.

**Implementation Notes:**
- `[ ]` Create `ocr_processor.cpp` implementing `IIngestionPlugin`; wraps `tesseract::TessBaseAPI`.
- `[ ]` `MimeDetector` triggers OCR for `image/png`, `image/jpeg`, `image/tiff` when `ContentPolicy::ocrEnabled() == true` for the collection.
- `[ ]` Pre-process image before OCR: rescale to 300 DPI if metadata indicates lower resolution; apply adaptive binarisation via Leptonica (bundled with Tesseract).
- `[ ]` Language packs loaded from `config/ai_ml/tesseract_lang/`; default `eng`; configurable per-collection.
- `[ ]` OCR output stored as `content_ocr_text` metadata field alongside image; also routed to `text_processor.cpp` for text indexing.
- `[ ]` If `libtesseract.so` is absent at runtime, `ocr_processor.cpp` returns a `ContentProcessResult` with `skipped=true` and logs a DEBUG message.

**Performance Targets:**
- A4 scanned page at 300 DPI OCR'd in < 3 s per page on a single CPU core.
- Tesseract initialization (warm): `TessBaseAPI::Init()` takes < 500 ms per language pack.

---

### Embedding Generation Pipeline (Text → Vector)
**Priority:** High
**Target Version:** v1.8.0

After text extraction (from documents, PDF, OCR output), automatically generate vector embeddings for semantic search. Wire `content_manager_llm.cpp` into the ingestion pipeline so that every ingested text document optionally receives an embedding stored alongside the content.

**Implementation Notes:**
- `[ ]` Add `EmbeddingStage` to the ingestion pipeline in `content_manager.cpp`; activated when `ContentPolicy::embeddingModel` is set for a collection.
- `[ ]` `content_manager_llm.cpp` exposes `generateEmbedding(text, model_name)` returning `std::vector<float>`; pipeline calls this after text extraction.
- `[ ]` Store embedding in a separate RocksDB column family (`cf_embeddings`) keyed by `ContentId`; also register in the vector index via `acceleration::BackendRegistry::instance().vectorBackend()`.
- `[ ]` Batch embedding: `async_ingestion_worker.cpp` accumulates up to `batch_size=32` text chunks before calling the embedding model in one batched inference call to amortise model overhead.
- `[ ]` On model failure (timeout > 5 s or error), content is stored without embedding and a `content_embedding_failures_total` metric is incremented; re-embedding can be triggered via admin API.

**Performance Targets:**
- Embedding latency (384-dim model, batch=32): < 50 ms on CPU; < 5 ms on CUDA GPU.
- Ingestion pipeline with embedding adds < 100 ms overhead vs ingestion without embedding (batch amortised).

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Test `PDFProcessor` with synthetic 2-page PDF fixture; test `StreamingIngestionWorker` chunk boundary handling; test `DeduplicationChecker` with known near-duplicate images and texts |
| Integration | Full ingestion pipeline for each new format (PDF, DOCX, streaming NDJSON) | `tests/content/content_integration_test.cpp`; include OCR test with a PNG containing known text |
| Performance | Ingestion throughput regression ≤ 5% for existing formats | `benchmarks/content_bench.cpp`; streaming bench with 1 GB synthetic NDJSON file |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| PDF extraction (100-page, 500 KB) | N/A (stub) | < 2 s | `benchmarks/content_bench.cpp` PDF fixture |
| Streaming ingestion throughput (NDJSON) | Buffered only | ≥ 100 MB/s | `benchmarks/content_bench.cpp` 1 GB file |
| pHash computation (4 MP JPEG) | N/A | < 5 ms | `benchmarks/content_bench.cpp` image fixture |
| OCR (A4 page, 300 DPI) | N/A | < 3 s | `benchmarks/content_bench.cpp` scanned page fixture |
| Embedding batch (32 docs, 384-dim, CPU) | N/A | < 50 ms | `benchmarks/embedding_bench.cpp` |

## Security / Reliability

- `[ ]` `content_security.cpp` must scan all uploaded archives (ZIP, tar) for zip-bomb patterns before extraction in `archive_processor.cpp`; enforce a maximum decompressed-to-compressed ratio of 100× and a maximum extracted file count of 1,000.
- `[ ]` LibreOffice headless subprocess spawned by `office_processor.cpp` must run in a separate OS user with no write access to the ThemisDB data directory; use `posix_spawn` with a restricted environment rather than `system()`.
- `[ ]` OCR output from `ocr_processor.cpp` must pass through `content_validator.cpp` before indexing to prevent injection of control characters or oversized text fields into the document store.
