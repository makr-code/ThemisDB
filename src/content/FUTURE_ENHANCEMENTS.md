<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Content Module - Future Enhancements

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
**Target Version:** v1.7.0 ✅ Implemented

`pdf_processor.cpp` and `office_processor.cpp` are fully implemented. PDF text extraction uses **poppler-cpp** (not pdfium); DOCX/XLSX/PPTX/ODF extraction uses built-in minizip + pugixml. Legacy `.doc`/`.xls`/`.ppt` via LibreOffice headless is implemented via `extractLegacyViaLibreOffice()` (CON-001 ✅).

**Implementation Notes:**
- `[x]` PDF: `pdf_processor.cpp` uses poppler-cpp (`THEMIS_ENABLE_PDF=ON`); extracts text per page with layout preservation; `page_number` preserved as metadata field; Quality Score 100/100, 0 stubs (poppler was chosen over pdfium for its C++ API).
- `[x]` DOCX: `office_processor.cpp::extractDOCX()` — unzips `.docx` with minizip/libzip; parses `word/document.xml` extracting `<w:t>` nodes via pugixml.
- `[x]` XLSX: `office_processor.cpp::extractXLSX()` — extracts cell values from `xl/worksheets/sheet*.xml`; returns JSON array-of-arrays; row/column cap enforced.
- `[x]` LibreOffice headless fallback (`.doc`, `.ppt`, `.xls`): `office_processor.cpp::extractLegacyViaLibreOffice()` spawns `soffice --headless --convert-to txt` via `posix_spawn`; 30 s timeout (SIGTERM→SIGKILL); `POSIX_SPAWN_RESETIDS`+`POSIX_SPAWN_SETPGROUP`; RAII temp-file cleanup; full 8-byte OLE header validation (CON-001 ✅).
- `[x]` Prometheus counters `content_pdf_extracted_total`, `content_office_extracted_total`, `content_extract_errors_total` implemented in `content_metrics.cpp`.

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

### Abuse Detection Stub Replacement
**Priority:** High
**Target Version:** v1.8.0

`content_security.cpp` has **2 confirmed stubs**: line 150 ("Check 3: Abuse detection (stub for future implementation)") and line 421 ("Stub implementation for future abuse detection"). Every content item passes abuse detection unconditionally. Malicious content (CSAM hashes, spam fingerprints) is not detected.

**Implementation Notes:**
- `[x]` Define `IAbuseDetector` interface with `detect(content_data, metadata) → AbuseDetectionResult`.
- `[x]` Implement `PhotoDNAAbuseDetector` backed by the PhotoDNA SDK (or open-source perceptual hash comparison against a blocklist) for image content; inject into `ContentSecurity` via constructor.
- `[x]` Implement `TextAbuseDetector` using a configurable blocklist + regex patterns loaded from `config/security/abuse_patterns.yaml`; support `BLOCK` and `FLAG` actions per pattern.
- `[x]` Wire both detectors into `ContentSecurity::check()` at line 150 (the stub location).
- `[x]` Add unit tests for both `BLOCK` (content rejected) and `FLAG` (content stored with flag) outcomes.
- `[x]` Audit log every detection event via `AuditLogger::logEvent()` with content hash, detector type, and action taken.

---

### `AsyncIngestionWorker`: YAML Config Loading and User Context
**Priority:** Medium
**Target Version:** v1.8.0

`async_ingestion_worker.cpp` has 2 TODOs: line 969 (`job.user_context = ""; // TODO: Add user context support`) and line 1010 (`// TODO: Implement YAML config loading`). Worker pool configuration is hardcoded; user context is not propagated to downstream audit logs.

**Implementation Notes:**
- `[ ]` Implement YAML config loading at line 1010: parse `config/content/async_worker.yaml` (keys: `worker_threads`, `queue_depth`, `batch_size`, `retry_attempts`) via `ConfigPathResolver::resolve()` + `ConfigSchemaValidator`.
- `[ ]` Propagate `user_context` from the caller's request metadata at line 969 into the `IngestionJob`; use it in downstream `AuditLogger::logEvent()` calls so ingestion events are attributable to the originating user.

---


**Priority:** High
**Target Version:** v1.7.0

Currently `ContentManager::ingest()` buffers the entire content in memory before processing. Files larger than `config_.max_content_size_bytes` are rejected. Implement chunked streaming ingestion in `async_ingestion_worker.cpp` that processes content in configurable chunks, enabling ingestion of files up to several GB.

**Implementation Notes:**
- `[x]` Add `ContentManager::ingestStream(std::istream& stream, const ContentMetadata& meta)` overload.
- `[x]` `async_ingestion_worker.cpp` reads chunks of `chunk_size_bytes` (default: 4 MB, configurable) from the stream; each chunk is processed by the appropriate `IIngestionPlugin::processChunk()` method.
- `[x]` Processors that support streaming (text, CSV, NDJSON) implement `processChunk()`; processors that require full data (PDF, image) buffer up to a configurable `max_buffered_bytes` limit (default: 256 MB) before falling back to error.
- `[x]` Back-pressure: `ingestStream()` blocks the caller when the worker queue depth exceeds `config_.max_queue_depth`; returns a `std::future<ContentId>` for async callers (CON-005 ✅).
- `[x]` Partial failure: if a chunk fails validation in `content_validator.cpp`, the entire ingestion transaction is rolled back and the partial content is purged from storage.

**Performance Targets:**
- 1 GB NDJSON file ingested at ≥ 100 MB/s sustained throughput on NVMe storage.
- Peak RSS increase during streaming ingestion < 2× `chunk_size_bytes` (i.e., two chunks in-flight at most).

---

### Content Deduplication via Perceptual Hashing
**Priority:** Medium
**Target Version:** v1.8.0

Exact duplicate detection (SHA-256 of raw bytes) is already performed in `content_manager.cpp`. Add near-duplicate detection using perceptual hashing (pHash for images, MinHash for text documents) to reject semantically identical content before storage.

**Implementation Notes:**
- `[x]` Images: compute pHash (DCT-based 64-bit hash) in `image_processor.cpp` using a pure C++ implementation (no OpenCV dependency); store hash in content metadata as `phash_hex`.
- `[x]` Text documents: compute MinHash signature (128 hash functions, Jaccard threshold 0.85) in `text_processor.cpp`; use a band LSH index stored in `cache::BoundedLRUCache` for fast lookup.
- `[x]` `ContentManager::ingest()` calls `DeduplicationChecker::isDuplicate(content_id, phash_or_minhash)` before committing; returns `DuplicateOf{existing_id}` if a near-duplicate is found.
- `[x]` Deduplication is opt-in per collection via `ContentPolicy` in `content_policy.cpp`; default off.
- `[x]` Expose `content_dedup_hits_total` and `content_dedup_checks_total` Prometheus counters.

**Performance Targets:**
- pHash computation for a 4 MP JPEG in < 5 ms.
- MinHash + LSH lookup for a 10 KB text document in < 1 ms (with warm band index of 100K entries).
- Near-duplicate detection adds < 10% overhead to total ingestion latency when deduplication is enabled.

---

### OCR for Image-Embedded Text (Tesseract Integration)
**Priority:** Medium
**Target Version:** v1.8.0 ✅ Partially Implemented

`ocr_processor.cpp` is implemented with Tesseract integration (Quality Score 97/100, 1 stub: `generateEmbedding` delegates to pipeline). The following items remain open.

**Implementation Notes:**
- `[x]` `ocr_processor.cpp` implementing `IIngestionPlugin` created; wraps `tesseract::TessBaseAPI` (enabled via `THEMIS_ENABLE_OCR=ON`).
- `[x]` `MimeDetector` triggers OCR for `image/png`, `image/jpeg`, `image/tiff` when `ContentPolicy::ocrEnabled() == true` for the collection.
- `[x]` Pre-process image before OCR: rescale to 300 DPI if metadata indicates lower resolution; apply adaptive binarisation via Leptonica (`pixSauvolaBinarize`). Controlled by `Config::enable_dpi_rescaling` / `Config::enable_adaptive_binarization`; results surfaced in `ocr_input_dpi`, `ocr_rescaled`, `ocr_binarized` metadata fields.
- `[x]` Language packs loaded from `config/ai_ml/tesseract_lang/`; default `eng`; configurable per-collection (language via `config_.language`; data directory resolved via `ConfigPathResolver::tryResolve("config/ai_ml/tesseract_lang")` in `runTesseract()` when `Config::data_dir` is empty; falls back to Tesseract auto-detect when directory absent).
- `[x]` OCR output stored as `content_ocr_text` metadata field alongside image (`result.metadata["content_ocr_text"] = result.text` in `ocr_processor.cpp:220`).
- `[x]` If `libtesseract.so` is absent at runtime, `ocr_processor.cpp` returns a skipped/unavailable `ContentProcessResult` and logs the absence.

**Performance Targets:**
- A4 scanned page at 300 DPI OCR'd in < 3 s per page on a single CPU core.
- Tesseract initialization (warm): `TessBaseAPI::Init()` takes < 500 ms per language pack.

---

### Embedding Generation Pipeline (Text → Vector)
**Priority:** High
**Target Version:** v1.8.0

After text extraction (from documents, PDF, OCR output), automatically generate vector embeddings for semantic search. Wire `content_manager_llm.cpp` into the ingestion pipeline so that every ingested text document optionally receives an embedding stored alongside the content.

**Implementation Notes:**
- `[x]` Add `EmbeddingStage` to the ingestion pipeline in `content_manager.cpp`; activated when `ContentPolicy::embeddingModel` is set for a collection.
- `[x]` `content_manager_embedding.cpp` exposes `ContentManager::generateEmbedding(text, model_name)` returning `std::vector<float>`; delegates to `EmbeddingPipeline::generateEmbedding()` when a pipeline is attached, falls back to the registered `TextProcessor::generateEmbedding()`. (`content_manager_llm.cpp` handles LLM analysis; embedding is separate.)
- `[x]` Store embedding under `emb:<ContentId>` in RocksDB for direct lookup by ContentId; also registered in the vector index via `vector_index_->addEntity()` under `chunks:<chunk_id>`.
- `[x]` Batch API available: `EmbeddingPipeline::generateEmbeddingBatch()` processes up to `batch_size=32` texts per call.
- `[x]` On model failure (timeout > 5 s or error), content is stored without embedding and `content_embedding_failures_total` is incremented via `ContentMetrics::recordEmbeddingFailure()` when a metrics sink is configured.

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

- `[x]` `content_security.cpp` scans all uploaded archives (ZIP, tar) for zip-bomb patterns before extraction in `archive_processor.cpp`; enforces a maximum decompressed-to-compressed ratio of 100× and a maximum extracted file count of 1,000 via `ContentSecurityManager::checkZipBomb()` (CON-006).
- `[x]` LibreOffice headless subprocess spawned by `office_processor.cpp` uses `posix_spawn` with `POSIX_SPAWN_RESETIDS` (drops SUID/SGID), `POSIX_SPAWN_SETPGROUP` (isolated process group), minimal sanitised environment (`HOME=tmpdir`), and no write access to ThemisDB data directory (CON-007 ✅).
- `[x]` OCR output from `ocr_processor.cpp` must pass through `content_validator.cpp` before indexing to prevent injection of control characters or oversized text fields into the document store.

---

### Audio Transcription Integration (Whisper / Speech-to-Text)
**Priority:** Medium
**Target Version:** v2.0.0

`audio_processor.cpp` now delegates optional speech-to-text transcription to `stt_processor.cpp` (`STTProcessor`). When `transcription.enabled=true` in the `AudioProcessor` configuration, an internal `STTProcessor` instance is initialised and invoked during `extract()`. Without the Whisper.cpp model the processor returns a descriptive placeholder; with a valid model path and `THEMIS_ENABLE_WHISPER=ON` it produces real transcriptions.

**Implementation Notes:**
- `[x]` `AudioProcessor::initialize()` constructs and initialises an `STTProcessor` when `transcription.enabled=true`; `AudioProcessor::shutdown()` tears it down.
- `[x]` `AudioProcessor::transcribe()` delegates to `STTProcessor::transcribe()` and returns `full_text` on success, empty string on failure or when no STT processor is configured.
- `[x]` `STTProcessor` is built as part of the content module when `THEMIS_ENABLE_CONTENT=ON` and `THEMIS_ENABLE_VOICE_ASSISTANT=OFF`; when voice assistant is enabled it is already included by `VoiceAssistant.cmake`.
- `[x]` Configuration keys forwarded: `transcription.model_path`, `transcription.model` (size), `transcription.language`.
- `[ ]` Full Whisper.cpp model integration requires `THEMIS_ENABLE_WHISPER=ON` and a valid GGML model file at `transcription.model_path`.
- `[ ]` Speaker diarization (via `STTProcessor::performSpeakerDiarization`) remains a stub; real implementation requires a clustering library.

**Performance Targets:**
- Transcription latency for a 60 s audio clip (base model, CPU): < 30 s.
- Memory overhead per transcription request: < 512 MB RSS with the `base` Whisper model.

**Test Strategy:**
- Unit: `AudioProcessorTranscriptionTest` in `tests/test_content_audio_processor.cpp` covers init with transcription enabled, stat counter increments, and shutdown/reinit cycle; no external model required (fallback placeholder text).
- Integration: enable `THEMIS_ENABLE_WHISPER=ON` with a bundled `ggml-tiny.bin` fixture to exercise the real Whisper path.
