# content Module — Implementation Gap Analysis

**Status:** In Progress  
**Last Updated:** 2026-05-19  

---

## 📊 Gap Summary (from ai_working/gap_scan_v3_content.json)

| Category | Count |
|---|---|
| oop_design | 1479 |
| uninitialized | 998 |
| type_conversion | 516 |
| reliability | 270 |
| input_validation | 251 |
| determinism | 236 |
| memory | 228 |
| performance_patterns | 144 |
| container | 153 |
| observability | 170 |
| concurrency | 29 |
| security | 27 |
| raii | 46 |
| **Total** | **4647** |

---

## ✅ Recent Remediation (2026-05-19)

### Phase 16 — Server handler reliability hardening

- **`server/monitoring_api_handler.cpp` (SRV-001)**: Replaced 10 catch-all handlers:
  - JSON parse fallbacks in `handleStats` and `handlePrometheus` → `std::exception` catch
  - Build-info best-effort continue block → `std::exception` catch
  - Schema-caps best-effort continue block → `std::exception` catch
  - Duration JSON parse best-effort in alert-silence handler → `std::exception` catch
  - 5 redundant double-catch tails (after `catch(std::exception&)`) in Prometheus metric collectors → removed
- **`server/content_api_handler.cpp` (SRV-002)**: Removed 7 redundant `catch(...)` double-tails after typed `std::exception` handlers in hybrid/fusion/fulltext search and config read/write handlers.
- **`server/changefeed_api_handler.cpp` (SRV-003)**: Replaced 6 catch-all handlers in SSE query-param/header integer parse paths (`max_seconds`, `heartbeat_ms`, `retry_ms`, `max_events`, `ack_timeout_ms`, `Last-Event-ID`) with `std::exception` catches; best-effort-ignore semantics preserved.
- **`server/vector_api_handler.cpp` (SRV-004)**: Replaced 5 catch-all handlers in cursor parse, enc-config schema iteration, enc-config outer guard, batch item processing, and prefix-scan key extraction paths with `std::exception` catches; fail-safe and error-count semantics preserved.

### Phase 15 — VideoProcessor / GeoProcessor reliability hardening

- **`video_processor.cpp` (CON-035)**: Replaced 4 catch-all handlers in FFmpeg-backed `extractMetadataFFmpeg`, `generateThumbnailFFmpeg`, `extractKeyframesFFmpeg`, and `detectScenesFFmpeg` with typed exception handling:
  - `std::filesystem::filesystem_error` for temp-file cleanup guards
  - `std::exception` for all other std::exception-derived throws
  - Cleanup-and-rethrow semantics preserved; best-effort swallow semantics preserved in non-rethrowing paths.
- **`geo_processor.cpp` (CON-036)**: Replaced 3 catch-all handlers in GDAL-backed `parseShapefile`, `parseGeoPackage`, and `parseGeoTIFF` with `std::exception` catch + rethrow, preserving GDAL resource cleanup contract.

### Phase 14 — Archive/HTML/Embedding reliability hardening

- **`archive_processor.cpp`, `html_processor.cpp`, `embedding_pipeline.cpp` (CON-034)**: Replaced remaining catch-all handlers in archive write/parse/extract, HTML numeric entity decode, and embedding timeout/get path with typed exception handling:
  - `std::invalid_argument` / `std::out_of_range` for numeric parsing conversions
  - `std::exception` fallback for non-fatal best-effort paths
  - `std::filesystem::filesystem_error` for best-effort archive directory creation
- Preserved runtime behavior: parse failures still fall back to prior defaults (`false` / `file_size=0` / literal entity passthrough / empty embedding) and archive extraction directory creation remains non-fatal best-effort.

### Phase 13 — ContentFS metadata reliability hardening

- **`content_fs.cpp` (CON-033)**: Replaced remaining catch-all handlers in ContentFS metadata parse/cleanup path (lines ~125–287) with typed exception handling:
  - `nlohmann::json::exception` for malformed CBOR/JSON metadata payloads
  - `std::exception` fallback for non-fatal best-effort branches
- Preserved runtime behavior: metadata decode failures in `get/getRange/head` still return corruption errors; `put/remove` legacy cleanup remains best-effort and idempotent.

### Phase 12 — VFS/stream search reliability hardening

- **`content_manager.cpp` (CON-032)**: Replaced remaining catch-all handlers in search/VFS/stream config path (lines ~1545–2694) with typed exception handling:
  - `nlohmann::json::exception` for malformed optional scoring/config/content JSON payloads
  - `std::exception` fallback for non-fatal best-effort branches (whitelist derivation and VFS directory scans)
- Preserved runtime behavior: malformed optional scoring/config and malformed scanned records continue to be ignored while defaults and best-effort listing/search behavior remain unchanged.

### Phase 11 — Metadata/chunk retrieval reliability hardening

- **`content_manager.cpp` (CON-031)**: Replaced catch-all handlers in vector metadata encryption/decryption and chunk/meta retrieval path (lines ~944–1240) with typed exception handling:
  - `nlohmann::json::exception` for malformed metadata/chunk JSON payloads
  - `std::exception` fallback for non-fatal best-effort branches
- Removed redundant nested catch-all wrappers in metadata encryption field patching; field failures continue to be handled by existing surrounding `std::exception` guard and warning logs.
- Preserved runtime behavior: malformed records/config still degrade to `std::nullopt`/empty results or skip optional re-encryption without aborting content reads.

### Phase 10 — Import/config reliability hardening

- **`content_manager.cpp` (CON-030)**: Replaced catch-all handlers in `checkDuplicateByHash()` and `importContent()` config/metrics path (lines ~563–842) with typed exception handling:
  - `nlohmann::json::exception` for malformed JSON configuration/index payloads
  - `std::exception` fallbacks for best-effort metrics/config handling
- Preserved behavior: malformed optional config still falls back to defaults; metrics updates stay non-fatal.
- Improved observability for fulltext config parsing by logging `std::exception::what()` in non-JSON failures.

### Phase 9 — Filter/Scan reliability hardening

- **`content_manager.cpp` (CON-029)**: Replaced catch-all handlers in `buildChunkWhitelist()` filter/scan path (lines ~167–328) with typed exception handling:
  - `nlohmann::json::exception` for malformed filter/schema/chunk-list payloads
  - `std::invalid_argument` / `std::out_of_range` for numeric parsing (`std::stod`)
  - `std::exception` fallback where non-fatal best-effort behavior must be preserved
- Added explicit `<stdexcept>` include for typed conversion exceptions.
- Behavior remains unchanged (best-effort scan, parse failures ignored), but reliability diagnostics and static-analysis quality improve by removing catch-all usage from this hot path.

### Phase 8 — RAII and Performance fixes

- **`content_manager.cpp` (CON-027)**: Replaced raw `new nlohmann::json(arr)` / manual `delete target` pattern (3 deletion sites, CWE-401) with `std::unique_ptr<nlohmann::json>` in the vector-metadata encryption loop. `tags_json_owner` auto-deletes on all exit paths. Eliminates `smart_ptr_misuse`, `allocation_loop`, and `delete_no_nullptr` gaps.
- **`content_security.cpp` (CON-028)**: Replaced O(n²) `std::find()` on `result.pii_types` with `std::unordered_set<std::string>` for O(1) dedup. Added `#include <unordered_set>`. Eliminates `repeated_search` gap.

### Earlier phases (Phases 1–7, see git log)

- CON-001..006: Critical healthCheck, RAII, noexcept, stub notes (Phase 3.1–3.4)
- CON-007..013: EVP_MD_CTX RAII, retry comment, tags JSON RAII (Phase 3.1–3.4)
- CON-014..016: IngestionJob POD defaults, QItem::hop=0, atomic {0} (Phase 5)
- CON-017..022: Uninitialized POD fields, [[nodiscard]] on 6 methods (Phase 6)
- CON-023..026: GeoExtractionData/CADExtractionData init, [[nodiscard]] (Phase 7)

---

## 🔍 False Positives Documented

- `content_manager.cpp:1532` (`o_n_squared`): Already uses `std::unordered_set` for whitelist lookup — false positive.
- `office_processor.cpp:817` (`posix_only_api`): `unlink/rmdir` already inside `#ifndef _WIN32` block — false positive.
- `content_metrics.cpp:300/515` (`deadlock_risk`): Sequential scoped locks (not nested) — no actual deadlock risk — false positive.
- `image_processor.cpp:563` (`array_bounds`): `sorted_freq[31]` and `[32]` in a 64-element array are valid indices — false positive.
- `tts_processor.cpp:305` (`delete_no_nullptr`): `tts_ctx_ = nullptr` follows immediately after delete — false positive.

---

## 📍 Location

```
src/content/MODULE_GAPS.md  ← You are here
```
