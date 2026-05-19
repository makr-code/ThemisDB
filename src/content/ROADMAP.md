> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Content Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production** — Multi-format content ingestion, MIME type detection, text extraction, image metadata extraction, geospatial data processing, and zstd compression are fully functional. PDF extraction (poppler-cpp), Office document extraction (DOCX/XLSX/PPTX/ODF), HTML boilerplate removal, Markdown/frontmatter parsing, audio metadata extraction, audio transcription (Whisper / STT), TTS generation, video metadata extraction, perceptual deduplication, language detection, and LLM-augmented content analysis are all implemented.

## Completed ✅
- [x] Content manager with ingestion pipeline
- [x] MIME type / content type detection
- [x] Text extraction and processing
- [x] Image metadata extraction
- [x] Geospatial data processing
- [x] JSON ingestion
- [x] Content compression using zstd
- [x] Content type detection for routing to specialized processors
- [x] Embedding generation pipeline (text → vector embeddings) (Issue: #1691)
- [x] PDF text extraction (poppler-cpp, `pdf_processor.cpp`) (Issue: #1681)
- [x] Office document text extraction — OOXML/ODF via libzip+pugixml (`office_processor.cpp`) (Issue: #1694)
- [x] Legacy Office text extraction — DOC/XLS/PPT via LibreOffice headless fallback (`office_processor.cpp`) (CON-001)
- [x] HTML content extraction with boilerplate removal (`html_processor.cpp`) (Issue: #1682)
- [x] Markdown processing and frontmatter parsing (`markdown_processor.cpp`) (Issue: #1683)
- [x] Audio metadata extraction (`audio_processor.cpp`) (Issue: #1679)
- [x] Audio transcription / STT integration (`stt_processor.cpp`) (Issue: #1687)
- [x] Text-to-speech generation (`tts_processor.cpp`)
- [x] Video metadata extraction (`video_processor.cpp`) (Issue: #1680)
- [x] OCR for image-embedded text (Tesseract integration, `ocr_processor.cpp`) (Issue: #1689)
- [x] Multi-language text detection and routing (`language_detector.cpp`) (Issue: #1690)
- [x] Perceptual deduplication — pHash (images) and MinHash+LSH (text) (`deduplication_checker.cpp`) (Issue: #1702)
- [x] LLM-augmented content analysis — summary, topics, sentiment, category (`content_manager_llm.cpp`)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Configurable processing pipeline (plugin-based processor chain) (Issue: #1686)
  - Inputs: `ProcessorChainConfig` per `ContentCategory`; dynamic registration via `IIngestionPlugin`
  - Outputs: ordered stage list, per-stage enable/disable, fallback policy
  - Constraints: no routing logic changes to `content_manager.cpp`; plugins register via `ingestion_plugin.cpp`
  - Tests: test_content_processor_chain.cpp must cover runtime enable/disable of stages
  - Target: Q3 2026

### Long-term (6-12 months)
- [I] Video frame extraction and scene detection (Issue: #1688) (Target: Q4 2026)
  - Inputs: video stream via FFmpeg
  - Outputs: keyframe images + scene boundary timestamps
  - Constraints: FFmpeg optional (`THEMIS_ENABLE_FFMPEG`); graceful stub without FFmpeg
  - Target: Q4 2026
- [x] LibreOffice headless fallback for legacy `.doc`/`.xls`/`.ppt` (CON-001)
  - Inputs: `.doc`, `.xls`, `.ppt` via OLE/Compound Document
  - Implementation: `office_processor.cpp::extractLegacyViaLibreOffice()` — `posix_spawn` (no `system()`); 30 s timeout with SIGTERM→SIGKILL escalation; `POSIX_SPAWN_RESETIDS`+`POSIX_SPAWN_SETPGROUP`+`POSIX_SPAWN_SETSIGDEF`; RAII temp-file cleanup; full 8-byte OLE header validation; minimal sandboxed environment (`HOME=tmpdir`)
  - Tests: `tests/test_office_processor.cpp` — `LegacyOfficeExtractionTest` (11 tests), `LegacyOfficeMetricsTest` (2 tests), `LibreOfficeSecurityTest` (7 security tests: path-hijacking prevention, shell-metacharacter injection, malformed binary input, large-blob handling, RAII temp-dir cleanup)
- [x] MimeDetector-triggered OCR activation via ContentPolicy::ocrEnabled() (CON-002)
- [x] OCR DPI pre-processing — rescale to 300 DPI + adaptive binarization via Leptonica (CON-003)
- [x] Back-pressure for streaming ingestion when worker queue depth exceeds max_queue_depth (CON-005)
- [x] Zip-bomb protection in content_security.cpp — max 100× decompression ratio, max 1 000 entries (CON-006)
- [x] Content manager reliability hardening block: broad catch-all handlers removed from filter/config fallback code paths in `content_manager.cpp` (`buildChunkWhitelist`, duplicate-hash lookup, compression/encryption config+metrics parsing) and replaced with typed exception handling (`const std::exception&`). (Target: Q2 2026)
- [x] Content manager reliability hardening block: remaining broad catch-all handlers removed from `content_manager.cpp` metadata decrypt/re-encryption, chunk retrieval, search-expansion/VFS scan-list, and `ingestStream()` config paths; replaced with typed exception handling (`const std::exception&`) while preserving fail-safe defaults. (Target: Q2 2026)

## Implementation Phases

### Phase 1: Core Ingestion Pipeline (Status: Completed ✅)
- [x] Implemented content manager with multi-format ingestion pipeline (`content/content_manager.cpp`)
- [x] Implemented MIME type detection for routing to specialized processors
- [x] Implemented text extraction and normalization processor
- [x] Implemented image metadata extraction (dimensions, color space, EXIF)
- [x] Implemented geospatial data processing (GeoJSON ingestion)
- [x] Implemented JSON ingestion with schema-less document storage
- [x] Implemented zstd compression for stored content blobs

### Phase 2: Pipeline Hardening (Status: Completed ✅)
- [x] Harden pipeline orchestration with per-stage error recovery and retry logic (`content_manager.cpp`) (Issue: #1701)
- [x] Implement content deduplication via SHA-256 hash before storage (`deduplication_checker.cpp`) (Issue: #1702)
- [x] Add configurable processor chain to enable/disable stages per content type (`content_policy.cpp`) (Issue: #1703)

### Phase 3: Advanced Format Support (Status: Completed ✅)
- [x] Integrate poppler-cpp for PDF text extraction with layout preservation (`content/pdf_processor.cpp`) (Issue: #1678)
- [x] Add Office document text extraction via libzip+pugixml for OOXML and ODF formats (`content/office_processor.cpp`) (Issue: #1694)
- [x] Implement chunked streaming ingestion for files larger than 100 MB (Issue: #1695)
- [x] Integrate Tesseract OCR for text extraction from image content (`content/ocr_processor.cpp`) (Issue: #1696)
- [x] Implement embedding generation pipeline (text → vector via local model) (Issue: #1697)
- [x] HTML content extraction with boilerplate removal (`html_processor.cpp`) (Issue: #1682)
- [x] Markdown processing and frontmatter parsing (`markdown_processor.cpp`) (Issue: #1683)
- [x] Audio metadata extraction + STT integration (`audio_processor.cpp`, `stt_processor.cpp`) (Issue: #1679, #1687)
- [x] Text-to-speech generation (`tts_processor.cpp`)
- [x] Video metadata extraction (`video_processor.cpp`) (Issue: #1680)
- [x] Multi-language text detection and routing (`language_detector.cpp`) (Issue: #1690)
- [x] Perceptual deduplication — pHash for images, MinHash+band-LSH for text (`deduplication_checker.cpp`) (Issue: #1702)
- [x] LLM-augmented analysis: summary, topics, sentiment, category (`content_manager_llm.cpp`)

### Phase 4: Tests (Status: In Progress 🚧)
- [I] Unit test coverage > 80% for all new-format processors (Issue: #1698)
  - PDFProcessor: synthetic 2-page PDF fixture; ≥ 6 test cases
  - OfficeProcessor: DOCX/XLSX/PPTX fixture extraction; malformed-file error paths
  - DeduplicationChecker: pHash near-duplicate images; MinHash Jaccard threshold
  - LanguageDetector: per-script detection accuracy; unknown-language fallback
  - TTSProcessor: init/shutdown cycle; output format validation; fallback without Piper
  - ContentManagerLLM: analyzeContent with mock EmbeddedLLM; error handling
- [x] Standalone focused test targets for: `test_content_deduplication.cpp`, `test_content_language_detector.cpp`, `test_content_audio_processor.cpp`, `test_content_metrics.cpp`, `test_content_security.cpp`, `test_content_processor_chain.cpp` — registered in `tests/CMakeLists.txt` (Target: Q3 2026)
- [x] test_content_pipeline.cpp, test_content_pipeline_hardening.cpp — pipeline orchestration and retry logic
- [x] test_content_streaming_ingestion.cpp — streaming chunk boundary handling
- [x] test_content_version_manager.cpp — delta storage and rollback
- [x] test_content_embedding_pipeline.cpp — embedding generation batch API

### Phase 5: Performance / Hardening (Status: Planned 📋)
- [P] Performance benchmarks per content type (benchmarks/bench_content_versioning.cpp) (Issue: #1699) (Target: Q3 2026)
  - PDF extraction: 100-page, 500 KB PDF in < 2 s on a single CPU core
  - Streaming ingestion throughput (NDJSON): ≥ 100 MB/s on NVMe
  - pHash computation for a 4 MP JPEG in < 5 ms
  - OCR (A4 page, 300 DPI): < 3 s per page
  - Embedding batch (32 docs, 384-dim, CPU): < 50 ms
- [x] Zip-bomb protection in `archive_processor.cpp` — max 100× decompression ratio, max 1 000 extracted files (CON-006)
- [x] Back-pressure for `ingestStream()` when `max_queue_depth` is exceeded (CON-005)
- [x] LibreOffice headless fallback for legacy `.doc`/`.xls`/`.ppt` via `posix_spawn` (CON-001) (Target: Q3 2026)
- [x] OCR DPI pre-processing: rescale to 300 DPI + adaptive binarization via Leptonica (CON-003)
- [x] MimeDetector-triggered OCR routing via `ContentPolicy::ocrEnabled()` (CON-002)

### Phase 6: Documentation & Release (Status: In Progress 🚧)
- [x] Architecture guide (`src/content/ARCHITECTURE.md`)
- [x] FUTURE_ENHANCEMENTS.md with design constraints and required interfaces
- [x] Missing-implementations audit report (`docs/de/content/MISSING_IMPLEMENTATIONS.md`)
- [x] German developer docs (`docs/de/content/`)
- [x] OCR language-pack path convention documented and defaulted to `config/ai_ml/tesseract_lang/` (CON-004)
- [x] API reference for `ContentManager::ingestStream()` back-pressure behaviour

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1698)
- [x] Integration tests (ingestion pipeline, content type routing)
- [P] Performance benchmarks (throughput per content type) (Issue: #1699)
- [P] Security audit (file upload validation, path traversal, zip-bomb protection) (Issue: #1700)
- [x] Documentation complete (content manager, content pipeline, architecture guide)
- [x] API stability guaranteed for ingestion pipeline

## Known Issues & Limitations
- Video metadata and thumbnail extraction is available via FFmpeg integration; scene detection, subtitle extraction, and keyframe extraction stubs exist for non-FFmpeg builds
- OCR integrated via Tesseract (`ocr_processor.cpp`, `THEMIS_ENABLE_OCR=ON`); DPI pre-processing (rescaling + Sauvola binarisation, CON-003) implemented. MimeDetector-triggered OCR routing via `ContentPolicy::ocrEnabled()` implemented (CON-002). OCR language-pack data directory defaults to `config/ai_ml/tesseract_lang/` (CON-004, resolved).
- Large file streaming ingestion:
  - Streaming-capable types (text/plain, CSV, NDJSON, Markdown): processed in configurable chunks (default 4 MB) without full-file buffering; peak RSS ≤ 2× chunk size
  - Non-streaming types (image, PDF, binary, etc.): buffered up to `max_buffered_bytes` (default 256 MB) before delegating to `ingestRawBlob`; files exceeding the limit are rejected
  - Back-pressure (blocking on `max_queue_depth`) implemented via `submitStream()` blocking and `ingestStream()` returning `std::future<std::string>` (CON-005 resolved)
- Zip-bomb protection (`ContentSecurityManager::checkZipBomb()`) is enforced: ratio threshold 100×, max 1 000 archive entries; called in `ArchiveProcessor::process()` before extraction (CON-006)

## Breaking Changes
- Processor plugin API may be refined when the plugin-based pipeline (Issue: #1686) is introduced

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🧪 NUR_TESTS (implementiert, kein Produktions-Aufrufer)

- `PhotoDNAAbuseDetector` – Erkennt CSAM via PhotoDNA-Hash-Abgleich; nur in Content-Security-Tests geprüft
  > **Aktion:** ROADMAP-Ticket für Produktions-Integration ergänzen oder als CANDIDATE_FOR_REMOVAL markieren.

### 🟡 UNGENUTZT (kein Test, kein externer Aufrufer)

- `detectorType` – Gibt den Typ des aktiven Abuse-Detectors zurück (PhotoDNA/Text/…)
- `createPdfExtractorAdapter` – Factory-Funktion für den PDF-Format-Extractor; noch nicht in Pipeline verdrahtet
  > **Aktion:** Für jedes Symbol entscheiden: (1) Verdrahten, (2) Testen oder (3) als CANDIDATE_FOR_REMOVAL einplanen.
