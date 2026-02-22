# Content Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
**Beta** — Multi-format content ingestion, MIME type detection, text extraction, image metadata extraction, geospatial data processing, and zstd compression are functional. PDF extraction, OCR, and audio transcription support are planned.

## Completed ✅
- [x] Content manager with ingestion pipeline
- [x] MIME type / content type detection
- [x] Text extraction and processing
- [x] Image metadata extraction
- [x] Geospatial data processing
- [x] JSON ingestion
- [x] Content compression using zstd
- [x] Content type detection for routing to specialized processors

## In Progress 🚧
- [I] PDF text extraction with layout preservation (Target: Q2 2026) (Issue: #1678)
- [x] Audio metadata extraction (Target: Q2 2026) (Issue: #1679)
- [I] Video metadata and thumbnail extraction (Target: Q3 2026) (Issue: #1680)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] PDF and Office document text extraction (pdfmium / LibreOffice headless) (Issue: #1681)
- [I] HTML content extraction with boilerplate removal (Issue: #1682)
- [I] Markdown processing and frontmatter parsing (Issue: #1683)
- [I] Streaming ingestion for large files (chunked processing) (Issue: #1684)
- [I] Content deduplication via perceptual hashing (Issue: #1685)
- [I] Configurable processing pipeline (plugin-based processor chain) (Issue: #1686)

### Long-term (6-12 months)
- [I] Audio transcription integration (Whisper / speech-to-text) (Issue: #1687)
- [I] Video frame extraction and scene detection (Issue: #1688)
- [I] OCR for image-embedded text (Tesseract integration) (Issue: #1689)
- [I] Multi-language text detection and routing (Issue: #1690)
- [I] Embedding generation pipeline (text → vector embeddings) (Issue: #1691)
- [I] Content versioning and delta storage (Issue: #1692)

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
- [I] Harden pipeline orchestration with per-stage error recovery and retry logic (Issue: #1701)
- [I] Implement content deduplication via SHA-256 hash before storage (Issue: #1702)
- [I] Add configurable processor chain to enable/disable stages per content type (Issue: #1703)

### Phase 3: Advanced Format Support (Status: Planned)
- [I] Integrate PDFium for PDF text extraction with layout preservation (`content/pdf_processor.cpp`) (Issue: #1693)
- [I] Add Office document text extraction via LibreOffice headless subprocess (Issue: #1694)
- [I] Implement chunked streaming ingestion for files larger than 100 MB (Issue: #1695)
- [I] Integrate Tesseract OCR for text extraction from image content (`content/ocr_processor.cpp`) (Issue: #1696)
- [I] Implement embedding generation pipeline (text → vector via local model) (Issue: #1697)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1698)
- [x] Integration tests (ingestion pipeline, content type routing)
- [I] Performance benchmarks (throughput per content type) (Issue: #1699)
- [I] Security audit (file upload validation, path traversal, zip-bomb protection) (Issue: #1700)
- [x] Documentation complete (content manager, content pipeline)
- [x] API stability guaranteed for ingestion pipeline

## Known Issues & Limitations
- PDF and Office document extraction not yet implemented
- Video processing not yet available
- OCR is not yet integrated
- Large file streaming ingestion may buffer entire file in memory

## Breaking Changes
- Processor plugin API may be refined when the plugin-based pipeline is introduced
