> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: current | validated: 2026-08-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Stable Diffusion Plugin

**Last Audit:** 2026-08-09
**Auditor:** Copilot
**Status:** ✅ Pass

## Summary

| Metric | Result |
|--------|--------|
| Source files audited | 4 (`sd_config.cpp`, `sd_prompt_sanitizer.cpp`, `sd_plugin.cpp`, `sd_plugin_registrar.cpp`) |
| Test targets | 3 (`SDPluginFocusedTests`, `SDPluginRegistrarTests`, `SDPluginRealBackendE2ETests` opt-in) |
| Test count | 76 (62 `SDPluginFocusedTests` + 12 `SDPluginRegistrarTests` + 2 `SDPluginRealBackendE2ETests` opt-in) |
| Open security issues | 0 |
| Open functional issues | 0 (model SHA-256 gate implemented) |
| Build system registration | ✅ `tests/CMakeLists.txt` + `plugins/CMakeLists.txt` |
| Documentation completeness | ✅ All 7 docs present |

## Latest Validation Evidence (2026-08-09)

- Configure: `cmake --preset community-release-allow-missing-rocksdb -DCMAKE_TOOLCHAIN_FILE= -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_BUILD_BENCHMARKS=ON -DTHEMIS_ENABLE_GPU=OFF`
- Build: `cmake --build build-community-debug-allow-missing-rocksdb --target test_sd_plugin test_sd_plugin_registrar --parallel 4`
- Run:
  - `./build-community-debug-allow-missing-rocksdb/bin_out/test_sd_plugin` → **62/62 passed**
  - `./build-community-debug-allow-missing-rocksdb/bin_out/test_sd_plugin_registrar` → **12/12 passed**
- Benchmark build attempt:
  - `cmake --build build-community-debug-allow-missing-rocksdb --target bench_stable_diffusion_release_gates --parallel 4`
  - **Blocked outside this module** by missing `rocksdb/db.h` in `src/storage/backup_manager.cpp` and an existing compile error in `src/storage/rocksdb_wrapper.cpp`.

## Build System

Registered in:
- `src/stable_diffusion/CMakeLists.txt` — `stable_diffusion_plugin` static library
- `tests/CMakeLists.txt` — `SDPluginFocusedTests` and `SDPluginRegistrarTests` test targets (+ opt-in real-backend E2E target)
- `plugins/CMakeLists.txt` — `THEMIS_PLUGIN_STABLE_DIFFUSION` option

Dependencies: `nlohmann_json` (required), `stable-diffusion.cpp` (optional, `THEMIS_ENABLE_STABLE_DIFFUSION=ON`).

## Source Files Audited

| File | Responsibility | Finding |
|------|---------------|---------|
| `sd_config.cpp` | Config deserialization | ✅ Clamps width/height/steps to ≥ 1; graceful on missing JSON keys |
| `sd_prompt_sanitizer.cpp` | Keyword blocklist | ✅ Case-insensitive; file loader skips comment lines; multi-occurrence removal |
| `sd_plugin.cpp` | Plugin lifecycle + policy | ✅ `isAllowed()` called before every `generate()`; `negative_prompt` also screened; mutex serialises all generate paths; exception caught; provenance unconditional |
| `sd_plugin_registrar.cpp` | Plugin registration with plugin manager | ✅ Reviewed |

## Interface Compliance

| Interface | Implemented | Notes |
|-----------|-------------|-------|
| `IImageGenerationBackend::initialize` | ✅ | Loads sanitizer from file if configured |
| `IImageGenerationBackend::generate` | ✅ | Policy-first (positive + negative prompt); provenance stamped; mutex-guarded |
| `IImageGenerationBackend::generateBatch` | ✅ | Sequential loop; each prompt screened independently |
| `IImageGenerationBackend::generateImg2Img` | ✅ | Policy-first; provenance stamped; delegates to `ISDGenerator::generateImg2Img()` |
| `IImageGenerationBackend::isPromptAllowed` | ✅ | Delegates to `SDPromptSanitizer` |
| `IImageGenerationBackend::getStatistics` | ✅ | `generation_count`, `blocked_count`, `error_count` |
| `THEMIS_IMGGEN_PLUGIN()` export | ✅ | `themis_imggen_create` + `themis_imggen_destroy` |

## Known Gaps

| ID | Description | Severity | Status |
|----|-------------|----------|--------|
| SD-01 | Stub PNG encoder omits IDAT — pixel data not encoded | Medium | Resolved v2.2.0 |
| SD-02 | `SDCppGenerator` not yet implemented — no real inference | Medium | Resolved v2.2.0 |
| SD-03 | `generateImg2Img` stub ignores input image | Low | Resolved v2.2.0 (input-image pass-through) |
| SD-04 | No model file integrity check (SHA-256 before model load) | Low | Resolved |
