<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Content Module

All notable changes to the Content module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
- Configurable processing pipeline: plugin-based processor chain via `ProcessorChainConfig` and `IIngestionPlugin` (Issue #1686, Target Q3 2026)
- Video frame extraction and scene detection via FFmpeg (Issue #1688, Target Q4 2026)

## [1.7.0] — 2026-03-09
### Added
- Zip-bomb protection in `content_security.cpp`: max 100× decompression ratio, max 1000 entries (CON-006)
- Back-pressure for streaming ingestion when worker queue depth exceeds `max_queue_depth` (CON-005)
- OCR DPI pre-processing: rescale to 300 DPI + adaptive binarization via Leptonica (CON-003)
- `MimeDetector`-triggered OCR activation via `ContentPolicy::ocrEnabled()` (CON-002)
- LibreOffice headless fallback for legacy `.doc`/`.xls`/`.ppt`: `extractLegacyViaLibreOffice()` using `posix_spawn` with 30s SIGTERM→SIGKILL escalation; RAII temp-file cleanup; 8-byte OLE header validation (CON-001)
- Security tests for LibreOffice path: path-hijacking prevention, shell-metacharacter injection, malformed binary, large-blob, RAII temp-dir cleanup (7 security tests)

## [1.6.0] — 2026-02-01
### Added
- LLM-augmented content analysis: summary, topics, sentiment, category (`content_manager_llm.cpp`)
- Perceptual deduplication: pHash (images) and MinHash+LSH (text) (`deduplication_checker.cpp`) (Issue #1702)
- Multi-language text detection and routing (`language_detector.cpp`) (Issue #1690)
- Embedding generation pipeline: text → vector embeddings (`content_manager_embedding.cpp`) (Issue #1691)
- Audio transcription / STT integration (`stt_processor.cpp`) (Issue #1687)
- Text-to-speech generation (`tts_processor.cpp`)
- OCR for image-embedded text via Tesseract (`ocr_processor.cpp`) (Issue #1689)

## [1.5.0] — 2025-12-01
### Added
- Office document extraction (OOXML/ODF) via libzip+pugixml: DOCX, XLSX, PPTX, ODF (`office_processor.cpp`) (Issue #1694)
- PDF text extraction via poppler-cpp (`pdf_processor.cpp`) (Issue #1681)
- HTML content extraction with boilerplate removal (`html_processor.cpp`) (Issue #1682)
- Markdown processing and frontmatter parsing (`markdown_processor.cpp`) (Issue #1683)
- Audio metadata extraction (`audio_processor.cpp`) (Issue #1679)
- Video metadata extraction (`video_processor.cpp`) (Issue #1680)
- CAD file processor (`cad_processor.cpp`)
- Archive processor (`archive_processor.cpp`)

## [1.0.0] — 2024-01-01
### Added
- Content manager with multi-format ingestion pipeline (`content_manager.cpp`)
- MIME type / content type detection and routing to specialized processors
- Text extraction and processing
- Image metadata extraction (dimensions, color space, EXIF)
- Geospatial data processing (GeoJSON ingestion)
- JSON ingestion with schema-less document storage
- Content compression using zstd
- Async ingestion worker (`async_ingestion_worker.cpp`)
- Content validation, policy, security, metrics, and error handling infrastructure
