> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: current | validated: 2026-05-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Content Module

**Last Audit:** 2026-05-19  
**Auditor:** Copilot  
**Status:** ✅ Pass (with tracked open items)

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 34 (`.cpp` in `src/content/`) |
| Test Coverage | ✅ Production-ready; security tests cover LibreOffice, archive, OCR paths |
| Open TODOs | 38 files contain TODOs (processor chain plugin API, video frames, LLM PII scrubbing) |
| Open Stubs | 6 compile-time conditional stubs (all documented with STUB/SIMULATION NOTE); see MODULE_GAPS.md §Acknowledged Stubs |
| Gap Scan (v3) | 4,077 flagged items; 27 security, 29 concurrency (see MODULE_GAPS.md) |
| Security Issues | None critical (security hardening complete for LibreOffice and zip-bomb paths); 27 lower-priority items tracked |

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
- **`VideoProcessor::healthCheck()` simulation-mode return** — was returning `initialized_` (always `true`) without FFmpeg; now returns `false`, consistent with `TTSProcessor`/`STTProcessor` pattern (2026-05-19).
- **Undocumented simulation fallback in extractMetadata()** — `video_processor.cpp` `#else` branch lacked a STUB/SIMULATION NOTE and contained an unreachable MKV detection branch (same magic bytes as WebM); both corrected (2026-05-19).
- **MODULE_GAPS.md unpopulated** — gap analysis document was a placeholder; populated with gap scan v3 results (4,077 items, categorized) and implementation roadmap (2026-05-19).
- **RAII violation: raw `new`/`delete` for tags JSON in metadata encryption** — `content_manager.cpp` metadata-encryption loop allocated a temporary `nlohmann::json` on the heap and performed manual `delete` in every exit path, including the exception handler. Replaced with a local stack variable (`tags_tmp`); all three raw `delete` call sites eliminated (2026-05-19, CON-009).
- **RAII violation: manual `EVP_MD_CTX_free` in `content_fs.cpp::sha256Hex()`** — 4 raw `EVP_MD_CTX_free()` early-return call sites replaced by a `std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>` RAII wrapper; context is freed automatically on all exit paths including exception (2026-05-19, CON-010).
- **RAII violation: manual `EVP_MD_CTX_free` in `content_manager.cpp::ingestStream()`** — 3 raw `EVP_MD_CTX_free(sha256_ctx); sha256_ctx = nullptr;` call sites (init-fail, update-fail, finalize) replaced by `unique_ptr::reset()`. Exception-unsafe leak path during stream processing eliminated (2026-05-19, CON-011).
- **Redundant explicit `file.close()` in archive ingestion loop** — `std::ifstream` RAII already guarantees closure at end of loop iteration; explicit call removed (2026-05-19).
- **Exception-in-destructor: `AsyncIngestionWorker::~AsyncIngestionWorker()`** — destructor called `stop()` without exception guard; if `stop()` throws (e.g. `mutex` op or `promise::set_exception` during stack unwinding), `std::terminate()` would be invoked. Destructor is now declared `noexcept` and wraps `stop()` in a `try/catch(...)` block (2026-05-19, CON-012).
- **Scanner false positive: `executeWithRetry` loop uses `<= max_retries`** — static scanner flagged `for (i <= max_retries)` as `OFF_BY_ONE`; loop is correct (`max_retries=0` → one initial attempt, no retries). Clarifying comment added (2026-05-19, CON-013).

### Open
- **Plugin processor chain** — `IIngestionPlugin` API not yet implemented (Issue #1686); processor dispatch is hardcoded.
- **Video frame extraction** — FFmpeg integration pending (Issue #1688).
- **LLM content analysis PII** — document summaries sent to LLM without PII scrubbing; operators should restrict LLM analysis to non-sensitive document categories.
- **Concurrency gaps** — 29 instances flagged by gap scan v3; pending manual review to confirm or refute false positives.
- **Security gaps** — 27 instances flagged by gap scan v3; all critical security controls (zip-bomb, subprocess) are in place; remaining items tracked in MODULE_GAPS.md.

## Compliance

- Zip-bomb protection (CON-006) aligns with denial-of-service protection requirements.
- LibreOffice subprocess isolation limits blast radius of parser vulnerabilities.
- Embedding pipeline supports pseudonymization workflows by converting text to opaque vectors before storage.
- Audio transcription output may contain PII; apply governance module masking before storage.
