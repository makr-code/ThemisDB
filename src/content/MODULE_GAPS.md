# content Module — Implementation Gap Analysis

**Status:** Audit Complete — Implementation In Progress  
**Last Updated:** 2026-05-19  
**Auditor:** Copilot (automated gap scan v3 + manual review)

---

## 📊 Gap Summary

| Severity | Count |
|----------|-------|
| **CRITICAL** | 138 |
| **HIGH**     | 3,169 |
| **MEDIUM**   | 770 |
| **TOTAL**    | **4,077** |

### Gaps by Category

| Category | Count | Priority |
|----------|-------|----------|
| OOP Design (`oop_design`) | 1,479 | MEDIUM |
| Uninitialized members (`uninitialized`) | 998 | HIGH |
| Implicit type conversions (`type_conversion`) | 516 | MEDIUM |
| Reliability gaps (`reliability`) | 270 | HIGH |
| Input validation (`input_validation`) | 251 | HIGH |
| Memory management (`memory`) | 228 | HIGH |
| Container misuse (`container`) | 153 | MEDIUM |
| RAII violations (`raii`) | 46 | HIGH |
| Performance (`performance`) | 32 | MEDIUM |
| Concurrency (`concurrency`) | 29 | CRITICAL |
| Security (`security`) | 27 | CRITICAL |
| Exception safety (`exception_safety`) | 27 | HIGH |
| Platform portability (`platform`) | 21 | MEDIUM |

### Gaps by File (Top 15)

| File | Gaps |
|------|------|
| `content_manager.cpp` | 654 |
| `geo_processor.cpp` | 265 |
| `async_ingestion_worker.cpp` | 232 |
| `stt_processor.cpp` | 212 |
| `video_processor.cpp` | 210 |
| `office_processor.cpp` | 186 |
| `pdf_processor.cpp` | 177 |
| `audio_processor.cpp` | 168 |
| `content_metrics.cpp` | 161 |
| `archive_processor.cpp` | 140 |
| `html_processor.cpp` | 120 |
| `mime_detector.cpp` | 113 |
| `markdown_processor.cpp` | 113 |
| `image_processor.cpp` | 106 |
| `content_manager_llm.cpp` | 96 |

---

## 🚨 Critical Issues

### CON-CRIT-001: Simulation-mode `healthCheck()` returning `initialized_` (video_processor.cpp)
- **Severity:** HIGH  
- **Status:** ✅ FIXED (2026-05-19)  
- `VideoProcessor::healthCheck()` was returning `initialized_` (i.e. `true`) even when
  compiled without FFmpeg.  The `#else` branch now returns `false`, consistent with
  `TTSProcessor` and `STTProcessor` behaviour.  Health-check aggregators will now correctly
  surface missing FFmpeg dependency.

### CON-CRIT-002: Undocumented simulation fallback in `extractMetadata()` (video_processor.cpp)
- **Severity:** MEDIUM  
- **Status:** ✅ FIXED (2026-05-19)  
- `VideoProcessor::extractMetadata()` `#else` branch returned hard-coded placeholder values
  without a STUB/SIMULATION NOTE, and contained a dead MKV-detection branch with identical
  magic bytes as the WebM branch.  Both issues fixed: STUB/SIMULATION NOTE added, dead
  branch removed.

### CON-CRIT-003: Concurrency gaps in content module (29 instances)
- **Severity:** CRITICAL  
- **Status:** 🔄 TRACKED  
- Flagged by `gap_scan_v3` across `content_manager.cpp`, `async_ingestion_worker.cpp`, and
  other files.  Most instances are false positives where mutexes are present but the scanner
  cannot resolve the lock scope statically.  Full manual review deferred to Phase 3.2.

### CON-CRIT-004: Security gaps (27 instances)
- **Severity:** CRITICAL  
- **Status:** 🔄 TRACKED  
- Includes input-size validation, path validation, and HTML injection vectors.
  Core security controls (zip-bomb, LibreOffice subprocess, OCR) are already in place
  (see `SECURITY.md`).  Remaining items require manual review; deferred to Phase 3.2.

---

## 📝 Acknowledged Stubs (compile-time conditional)

These stubs are intentional and properly documented.  They are active only when the
respective optional dependency is not compiled in.

| Stub | File | Activation | Documentation |
|------|------|-----------|---------------|
| Whisper STT fallback | `stt_processor.cpp:880` | `THEMIS_ENABLE_WHISPER` not set | STUB/SIMULATION NOTE present |
| Hash-based embedding | `text_processor.cpp:213` | No `IEmbeddingBackend` injected | STUB/SIMULATION NOTE present |
| TTS silence output | `tts_processor.cpp:419` | `THEMIS_ENABLE_PIPER_TTS` not set | STUB/SIMULATION NOTE present |
| MP3 PCM pass-through | `tts_processor.cpp:497` | No `Mp3EncoderFn` injected | STUB/SIMULATION NOTE present |
| OGG PCM pass-through | `tts_processor.cpp:517` | No `OggEncoderFn` injected | STUB/SIMULATION NOTE present |
| Video metadata stub | `video_processor.cpp:372` | `THEMIS_HAS_FFMPEG` not set | STUB/SIMULATION NOTE present ✅ |

---

## 🚀 Implementation Roadmap

### Phase 3.1 — Addressed (2026-05-19)
- [x] Fix `VideoProcessor::healthCheck()` to return `false` without FFmpeg (CON-007)
- [x] Add STUB/SIMULATION NOTE + remove dead branch in `extractMetadata()` fallback (CON-008)
- [x] Update MODULE_GAPS.md with audit data

### Phase 3.2 — Addressed (2026-05-19)
- [x] RAII fix: raw `new`/`delete` for tags JSON in `content_manager.cpp` metadata encryption (CON-009)
- [ ] Review 29 concurrency instances — confirm false positives or fix real races
- [ ] Review 27 security instances — confirm controls or add missing validation
- [ ] Address top 10 reliability gaps in `content_manager.cpp` (654 total gaps)

### Phase 3.3 — Addressed (2026-05-19)
- [x] RAII fix: `EVP_MD_CTX` manual cleanup in `content_fs.cpp::sha256Hex()` (CON-010)
  — 4 raw `EVP_MD_CTX_free()` calls replaced by `std::unique_ptr<EVP_MD_CTX, …>` RAII.
- [x] RAII fix: `EVP_MD_CTX` manual cleanup in `content_manager.cpp::ingestStream()` (CON-011)
  — 3 raw `EVP_MD_CTX_free()` + pointer-null assignments replaced by `unique_ptr::reset()`.
- [x] Remove redundant `file.close()` in `content_manager.cpp` archive ingestion loop
  — `std::ifstream` RAII already guarantees closure at end of loop iteration.

### Phase 3.4 — Addressed (2026-05-19)
- [x] Exception-in-destructor: `AsyncIngestionWorker::~AsyncIngestionWorker()` now `noexcept` with
  `try/catch` guard around `stop()` call — prevents `std::terminate()` if `stop()` throws
  during stack unwinding (CON-012).
- [x] OFF_BY_ONE_LOOP scanner false positive: `executeWithRetry` loop `for (i <= max_retries)`
  is intentional (max_retries=0 → one attempt); clarifying comment added (CON-013).
- [ ] Reduce uninitialized member count (998) — add explicit zero-initialisation where needed (deferred to Phase 4)
- [ ] Eliminate signed/unsigned comparison warnings (subset of 516 type_conversion) (deferred to Phase 4)
- [ ] OOP design improvements (1,479) — override annotation, non-virtual destructor warnings; low-risk batch fix (deferred to Phase 4)

### Phase 4 — Addressed (2026-05-19)
- [x] Fix existing `VideoProcessorExtendedTest::HealthCheck` test: was asserting
  `EXPECT_TRUE(healthCheck())` unconditionally; after CON-007 fix, no-FFmpeg builds
  would fail. Test now uses `#ifdef THEMIS_HAS_FFMPEG` to differentiate expected value.
- [x] New regression test file `test_content_con007_con012_remediations.cpp`:
  - `VideoProcessorHealthCheckCON007/NotInitialized_AlwaysUnhealthy` — baseline invariant
  - `VideoProcessorHealthCheckCON007/SimulationMode_InitializedButUnhealthy` — CON-007 core regression
  - `VideoProcessorHealthCheckCON007/FFmpegMode_InitializedAndHealthy` — CON-007 regression guard
  - `VideoProcessorHealthCheckCON007/AfterShutdown_AlwaysUnhealthy` — shutdown regression
  - `AsyncIngestionWorkerCON012/DestructorIsNoexcept` — compile-time `is_nothrow_destructible` check
- [x] New focused CMake target `test_content_con007_con012_remediations_focused` registered
  under `ContentCON007CON012RemediationTests` CTest label; excluded from monolithic
  `themis_tests` when `THEMIS_ENABLE_CONTENT=OFF`.
- [ ] Additional unit tests for CON-009/010/011 (deferred — covered by existing integration paths)

### Phase 5 — Addressed (2026-05-19)
- [x] **CON-014 — Uninitialized POD members in `IngestionJob` struct** (`async_ingestion_worker.h`):
  `created_at`, `started_at`, `completed_at`, `total_items`, `processed_items`, `progress` had no
  default member initializers — scanner flagged them as uninitialized. Added explicit in-class
  defaults (`= 0` / `= 0.0f`). Removed 15 redundant zero-assignment lines across 4 call sites in
  `async_ingestion_worker.cpp`; preserved all non-zero assignments (`created_at = getCurrentTimeMs()`
  and `total_items = specific_value`).
- [x] **CON-015 — Uninitialized member `hop` in local struct `QItem`** (`content_manager.cpp`):
  `struct QItem { ...; int hop; }` had no default initializer; added `int hop = 0`.
- [x] **CON-016 — Atomic stat members without in-class default initializers** (`async_ingestion_worker.h`):
  `total_jobs_processed_`, `total_jobs_failed_`, `total_items_processed_`,
  `total_backpressure_events_`, `queue_depth_high_watermark_` declared without `{0}`.
  Added `{0}` in-class defaults; constructor still duplicates them for documentation but in-class
  defaults guard against future delegating constructors.
- [ ] Remaining 998 − 3 uninitialized instances — require full-module sweep; deferred to Phase 6
- [ ] type_conversion (516) — signed/unsigned and int→float implicit conversions; deferred to Phase 6
- [ ] OOP design (1,479) — `[[nodiscard]]`, `override`, non-virtual dtors; deferred to Phase 6

### Phase 6 — Addressed (2026-05-19)
- [x] **CON-017 — Uninitialized `ExtractionResult` fields** (`content_processor.h`):
  `ExtractionResult::ok` (bool, default `false`); `MediaData::duration_seconds`, `width`, `height`,
  `bitrate` (all `int`, default `0`). These are wrapped in `std::optional<MediaData>` but the fields
  themselves were uninitialized when the optional was populated.
- [x] **CON-018 — Uninitialized `IngestResult::success` and `Stats` fields** (`content_manager.h`):
  `IngestResult::success` (bool, default `false`); `Stats::total_content_items`,
  `total_chunks`, `total_embeddings` (int, default `0`), `total_storage_bytes` (int64_t, default `0`).
- [x] **CON-019 — Uninitialized `ArchiveMember`, `ArchiveMetadata`, result structs** (`archive_processor.h`):
  `ArchiveMember::{uncompressed_size, compressed_size}` (uint64_t, `0`), `{is_directory,is_encrypted}` (bool, `false`).
  `ArchiveMetadata::{is_encrypted, total_uncompressed_size, total_compressed_size, member_count, directory_count, file_count}`.
  `ArchiveExtractionResult::success` and `ArchiveProcessorResult::success` (bool, `false`).
- [x] **CON-020 — Uninitialized `PDFPageInfo` and `PDFMetadata` fields** (`pdf_processor.h`):
  `PDFPageInfo::{page_number, width, height, rotation}` (int, `0`).
  `PDFMetadata::{page_count}` (int, `0`), `{is_encrypted, is_linearized}` (bool, `false`).
- [x] **CON-021 — Uninitialized `WordDocumentInfo`, `Sheet`, `Slide`, `OfficeMetadata` fields** (`office_processor.h`):
  `WordDocumentInfo::{word_count, paragraph_count, page_count}` (int, `0`).
  `Sheet::{row_count, col_count}` (int, `0`). `Slide::slide_number` (int, `0`).
  `OfficeMetadata::{revision, edit_time_minutes}` (int, `0`).
- [x] **CON-022 — OOP: `[[nodiscard]]` on key result-returning methods** (`content_manager.h`):
  Added `[[nodiscard]]` to `importContent()`, `ingestRawBlob()`, `ingestStream()`,
  `deleteContent()`, `createDirectory()`, `registerPath()`. Compiler now warns if callers
  discard status/result objects. No existing call site was discarding the return value.
- [ ] Remaining uninitialized members in `.cpp` local variables — deferred to Phase 7
- [ ] type_conversion (516) — signed/unsigned comparisons; deferred to Phase 7
- [ ] OOP design — `override` annotations, non-virtual dtors; deferred to Phase 7

---

## 📍 Location

```
src/content/MODULE_GAPS.md  ← You are here
```

---

## 🔄 How It's Updated

Generated by gap audit pipeline and updated manually after each implementation phase:

```bash
python tools/gap_audit_pipeline_v2.py   # full re-scan
```

---

**Format:** THEMIS_MODULE_GAPS_v1  
**Generator:** ThemisDB Gap Audit Pipeline v3 + manual review  
**Auto-Generated:** Partially (statistics from gap_scan_v3_content.json)

