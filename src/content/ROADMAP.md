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
- [ ] PDF text extraction with layout preservation (Target: Q2 2026)
- [ ] Audio metadata extraction (Target: Q2 2026)
- [ ] Video metadata and thumbnail extraction (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] PDF and Office document text extraction (pdfmium / LibreOffice headless)
- [ ] HTML content extraction with boilerplate removal
- [ ] Markdown processing and frontmatter parsing
- [ ] Streaming ingestion for large files (chunked processing)
- [ ] Content deduplication via perceptual hashing
- [ ] Configurable processing pipeline (plugin-based processor chain)

### Long-term (6-12 months)
- [ ] Audio transcription integration (Whisper / speech-to-text)
- [ ] Video frame extraction and scene detection
- [ ] OCR for image-embedded text (Tesseract integration)
- [ ] Multi-language text detection and routing
- [ ] Embedding generation pipeline (text → vector embeddings)
- [ ] Content versioning and delta storage

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
- [~] Harden pipeline orchestration with per-stage error recovery and retry logic
- [~] Implement content deduplication via SHA-256 hash before storage
- [~] Add configurable processor chain to enable/disable stages per content type

### Phase 3: Advanced Format Support (Status: Planned)
- [ ] Integrate PDFium for PDF text extraction with layout preservation (`content/pdf_processor.cpp`)
- [ ] Add Office document text extraction via LibreOffice headless subprocess
- [ ] Implement chunked streaming ingestion for files larger than 100 MB
- [ ] Integrate Tesseract OCR for text extraction from image content (`content/ocr_processor.cpp`)
- [ ] Implement embedding generation pipeline (text → vector via local model)

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (ingestion pipeline, content type routing)
- [ ] Performance benchmarks (throughput per content type)
- [ ] Security audit (file upload validation, path traversal, zip-bomb protection)
- [x] Documentation complete (content manager, content pipeline)
- [x] API stability guaranteed for ingestion pipeline

## Known Issues & Limitations
- PDF and Office document extraction not yet implemented
- Video and audio processing not yet available
- OCR is not yet integrated
- Large file streaming ingestion may buffer entire file in memory

## Breaking Changes
- Processor plugin API may be refined when the plugin-based pipeline is introduced
