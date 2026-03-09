# Content Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Production** — Multi-format content ingestion, MIME type detection, text extraction, image metadata extraction, geospatial data processing, and zstd compression are fully functional. PDF extraction (poppler-cpp), Office document extraction (DOCX/XLSX/PPTX/ODF), HTML boilerplate removal, Markdown/frontmatter parsing, audio metadata extraction, audio transcription (Whisper / STT), and video metadata extraction are all implemented.

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
- [x] HTML content extraction with boilerplate removal (`html_processor.cpp`) (Issue: #1682)
- [x] Markdown processing and frontmatter parsing (`markdown_processor.cpp`) (Issue: #1683)
- [x] Audio metadata extraction (`audio_processor.cpp`) (Issue: #1679)
- [x] Audio transcription / STT integration (`stt_processor.cpp`) (Issue: #1687)
- [x] Video metadata extraction (`video_processor.cpp`) (Issue: #1680)
- [x] OCR for image-embedded text (Tesseract integration, `ocr_processor.cpp`) (Issue: #1689)
- [x] Multi-language text detection and routing (`language_detector.cpp`) (Issue: #1690)

## In Progress 🚧
*(none currently in progress)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] Configurable processing pipeline (plugin-based processor chain) (Issue: #1686)

### Long-term (6-12 months)
- [I] Video frame extraction and scene detection (Issue: #1688)
- [x] OCR for image-embedded text (Tesseract integration, `ocr_processor.cpp`) (Issue: #1689)
- [x] Multi-language text detection and routing (`content/language_detector.cpp`) (Issue: #1690)

## Implementation Phases

### Phase 1: Core Ingestion Pipeline (Status: Completed)
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
- [x] Audio metadata extraction (`audio_processor.cpp`) (Issue: #1679)
- [x] Audio transcription / STT integration (`stt_processor.cpp`) (Issue: #1687)
- [x] Video metadata extraction (`video_processor.cpp`) (Issue: #1680)
- [x] Multi-language text detection and routing (`language_detector.cpp`) (Issue: #1690)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1698)
- [x] Integration tests (ingestion pipeline, content type routing)
- [I] Performance benchmarks (throughput per content type) (Issue: #1699)
- [P] Security audit (file upload validation, path traversal, zip-bomb protection) (Issue: #1700)
- [x] Documentation complete (content manager, content pipeline)
- [x] API stability guaranteed for ingestion pipeline

## Known Issues & Limitations
- Legacy Office formats (DOC/XLS/PPT via OLE/Compound Document) not fully supported; OOXML (DOCX/XLSX/PPTX) and ODF are handled
- Video metadata and thumbnail extraction is available via FFmpeg integration; scene detection, subtitle extraction, and keyframe extraction stubs exist for non-FFmpeg builds
- OCR integrated via Tesseract (`ocr_processor.cpp`, `THEMIS_ENABLE_OCR=ON`); MimeDetector-triggered OCR routing and DPI pre-processing not yet implemented (see `missing-implementations.md`)
- Large file streaming ingestion:
  - Streaming-capable types (text/plain, CSV, NDJSON, Markdown): processed in configurable chunks (default 4 MB) without full-file buffering; peak RSS ≤ 2× chunk size
  - Non-streaming types (image, PDF, binary, etc.): buffered up to `max_buffered_bytes` (default 256 MB) before delegating to `ingestRawBlob`; files exceeding the limit are rejected

## Breaking Changes
- Processor plugin API may be refined when the plugin-based pipeline is introduced
