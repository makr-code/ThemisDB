# Content Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — Multi-format content ingestion, MIME type detection, text extraction, image metadata extraction, geospatial data processing, and zstd compression are functional. PDF extraction (poppler-cpp) and Office document extraction (DOCX/XLSX/PPTX/ODF via libzip+pugixml) are implemented. OCR and audio transcription support are planned.

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

## In Progress 🚧
- [I] Audio metadata extraction (Target: Q2 2026) (Issue: #1679)
- [P] Video metadata and thumbnail extraction (Target: Q3 2026) (Issue: #1680)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] PDF and Office document text extraction (pdfmium / LibreOffice headless) (Issue: #1681)
- [P] HTML content extraction with boilerplate removal (Issue: #1682)
- [P] Markdown processing and frontmatter parsing (Issue: #1683)
- [I] Configurable processing pipeline (plugin-based processor chain) (Issue: #1686)

### Long-term (6-12 months)
- [P] Audio transcription integration (Whisper / speech-to-text) (Issue: #1687)
- [I] Video frame extraction and scene detection (Issue: #1688)
- [I] OCR for image-embedded text (Tesseract integration) (Issue: #1689)
- [~] Multi-language text detection and routing (Issue: #1690)

## Implementation Phases

### Phase 1: Core Ingestion Pipeline (Status: Completed)
- [x] Implemented content manager with multi-format ingestion pipeline (`content/content_manager.cpp`)
- [x] Implemented MIME type detection for routing to specialized processors
- [x] Implemented text extraction and normalization processor
- [x] Implemented image metadata extraction (dimensions, color space, EXIF)
- [x] Implemented geospatial data processing (GeoJSON ingestion)
- [x] Implemented JSON ingestion with schema-less document storage
- [x] Implemented zstd compression for stored content blobs

### Phase 2: Pipeline Hardening (Status: In Progress)
- [P] Harden pipeline orchestration with per-stage error recovery and retry logic (Issue: #1701)
- [I] Implement content deduplication via SHA-256 hash before storage (Issue: #1702)
- [I] Add configurable processor chain to enable/disable stages per content type (Issue: #1703)

### Phase 3: Advanced Format Support (Status: In Progress)
- [x] Integrate poppler-cpp for PDF text extraction with layout preservation (`content/pdf_processor.cpp`) (Issue: #1678)
- [x] Add Office document text extraction via libzip+pugixml for OOXML and ODF formats (`content/office_processor.cpp`) (Issue: #1694)
- [x] Implement chunked streaming ingestion for files larger than 100 MB (Issue: #1695)
- [I] Integrate Tesseract OCR for text extraction from image content (`content/ocr_processor.cpp`) (Issue: #1696)
- [x] Implement embedding generation pipeline (text → vector via local model) (Issue: #1697)

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
- OCR is not yet integrated
- Large file streaming ingestion:
  - Streaming-capable types (text/plain, CSV, NDJSON, Markdown): processed in configurable chunks (default 4 MB) without full-file buffering; peak RSS ≤ 2× chunk size
  - Non-streaming types (image, PDF, binary, etc.): buffered up to `max_buffered_bytes` (default 256 MB) before delegating to `ingestRawBlob`; files exceeding the limit are rejected

## Breaking Changes
- Processor plugin API may be refined when the plugin-based pipeline is introduced
