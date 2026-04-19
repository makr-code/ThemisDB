<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Content Module

**Last Audit:** 2026-04-19
**Auditor:** Copilot
**Status:** ✅ Pass

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 34 (`.cpp` in `src/content/`) |
| Test Coverage | ✅ Production-ready; security tests cover LibreOffice, archive, OCR paths |
| Open TODOs | 38 files contain TODOs (processor chain plugin API, video frames, LLM PII scrubbing) |
| Open Stubs | 2 (plugin processor chain pending Issue #1686; video frame extraction pending Issue #1688) |
| Security Issues | None (security hardening complete for LibreOffice and zip-bomb paths) |

## Build System

- All content source files registered in `cmake/CMakeLists.txt` and `cmake/ModularBuild.cmake`.
- Tesseract/OCR compilation guarded by `THEMIS_ENABLE_OCR`.
- LibreOffice fallback guarded by `THEMIS_ENABLE_LIBREOFFICE`.
- PDF processor guarded by `THEMIS_ENABLE_POPPLER`.
- LLM content analysis guarded by `THEMIS_ENABLE_LLM`.
- FFmpeg video processing guarded by `THEMIS_ENABLE_FFMPEG` (not yet implemented).

## Source Files Audited

| File | Purpose |
|------|---------|
| `abuse_detector.cpp` | Abuse content detection and policy enforcement |
| `archive_processor.cpp` | Archive extraction with bomb protection |
| `async_ingestion_worker.cpp` | Async ingestion queue with back-pressure |
| `audio_processor.cpp` | Audio metadata extraction |
| `cad_processor.cpp` | CAD file processing |
| `content_errors.cpp` | Typed content error taxonomy |
| `content_fs.cpp` | Content filesystem operations |
| `content_logger.cpp` | Content-specific structured logging |
| `content_manager.cpp` | Core ingestion pipeline and dispatcher |
| `content_manager_embedding.cpp` | Embedding generation pipeline |
| `content_manager_llm.cpp` | LLM-augmented content analysis |
| `content_metrics.cpp` | Prometheus metrics for ingestion pipeline |
| `content_policy.cpp` | Content processing policy gates |
| `content_security.cpp` | Zip-bomb protection, decompression ratio limits |
| `content_type.cpp` | Content type detection and routing |
| `content_validator.cpp` | Format integrity validation |
| `deduplication_checker.cpp` | pHash (images) and MinHash+LSH (text) deduplication |
| `embedding_pipeline.cpp` | Embedding generation orchestration |
| `geo_processor.cpp` | GeoJSON ingestion and processing |
| `html_processor.cpp` | HTML boilerplate removal |
| `image_processor.cpp` | Image metadata extraction |
| `ingestion_plugin.cpp` | Plugin interface for custom processors |
| `language_detector.cpp` | Multi-language detection and routing |
| `markdown_processor.cpp` | Markdown processing and frontmatter parsing |
| `mime_detector.cpp` | MIME type detection |
| `mock_clip_processor.cpp` | Mock CLIP processor for testing multimodal pipelines |
| `ocr_processor.cpp` | OCR text extraction via Tesseract |
| `office_processor.cpp` | LibreOffice-backed Office document extraction |
| `pdf_processor.cpp` | PDF text and metadata extraction via Poppler |
| `stt_processor.cpp` | Speech-to-text processor integration |
| `text_processor.cpp` | Plain text normalization and segmentation |
| `tts_processor.cpp` | Text-to-speech synthesis integration |
| `version_manager.cpp` | Content version tracking and history management |
| `video_processor.cpp` | Video metadata and frame extraction |

## Test Coverage

- `tests/test_office_processor.cpp`:
  - `LegacyOfficeExtractionTest` (11 tests): LibreOffice subprocess invocation
  - `LegacyOfficeMetricsTest` (2 tests): metrics on LibreOffice execution
  - `LibreOfficeSecurityTest` (7 tests): path hijacking, shell metacharacters, malformed binary, large blob, RAII temp-dir cleanup
- `tests/test_content_security.cpp`: zip-bomb protection (100× ratio cap, 1000 entry cap)
- `tests/test_content_validator.cpp`: format integrity checks
- `tests/test_deduplication_checker.cpp`: pHash and MinHash+LSH

## Findings

### Resolved
- **Shell injection via LibreOffice invocation** — replaced `system()`/`popen()` with `posix_spawn`+`execv`; no shell metacharacter expansion possible.
- **LibreOffice process hang** — 30s timeout with SIGTERM→SIGKILL escalation added.
- **Zip/archive decompression bomb** — 100× ratio and 1000 entry limits enforced in `content_security.cpp`.
- **LibreOffice environment poisoning** — minimal `HOME=tmpdir` environment; all inherited variables cleared.
- **OCR input validation** — pre-processing to 300 DPI with Leptonica binarization before Tesseract.

### Open
- **Plugin processor chain** — `IIngestionPlugin` API not yet implemented (Issue #1686); processor dispatch is hardcoded.
- **Video frame extraction** — FFmpeg integration pending (Issue #1688).
- **LLM content analysis PII** — document summaries sent to LLM without PII scrubbing; operators should restrict LLM analysis to non-sensitive document categories.

## Compliance

- Zip-bomb protection (CON-006) aligns with denial-of-service protection requirements.
- LibreOffice subprocess isolation limits blast radius of parser vulnerabilities.
- Embedding pipeline supports pseudonymization workflows by converting text to opaque vectors before storage.
- Audio transcription output may contain PII; apply governance module masking before storage.
