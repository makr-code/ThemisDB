# Content Module Roadmap

## Current Status
Production-ready for multi-format content ingestion, MIME type detection, text extraction, image metadata extraction, geospatial data processing, and zstd compression.

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
