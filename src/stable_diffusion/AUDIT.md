<!-- Status: current | validated: 2026-04-10 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Stable Diffusion Plugin

**Last Audit:** 2026-04-10
**Auditor:** Copilot
**Status:** ✅ Pass

## Summary

| Metric | Result |
|--------|--------|
| Source files audited | 3 (`sd_config.cpp`, `sd_prompt_sanitizer.cpp`, `sd_plugin.cpp`) |
| Test targets | 1 (`SDPluginFocusedTests`) |
| Test count | 45 |
| Open security issues | 0 |
| Open functional issues | 2 (stub PNG encoder, no real model) |
| Build system registration | ✅ `tests/CMakeLists.txt` + `plugins/CMakeLists.txt` |
| Documentation completeness | ✅ All 7 docs present |

## Build System

Registered in:
- `src/stable_diffusion/CMakeLists.txt` — `stable_diffusion_plugin` static library
- `tests/CMakeLists.txt` — `SDPluginFocusedTests` test target (45 tests)
- `plugins/CMakeLists.txt` — `THEMIS_PLUGIN_STABLE_DIFFUSION` option

Dependencies: `nlohmann_json` (required), `stable-diffusion.cpp` (optional, `THEMIS_ENABLE_STABLE_DIFFUSION=ON`).

## Source Files Audited

| File | Responsibility | Finding |
|------|---------------|---------|
| `src/stable_diffusion/sd_config.cpp` | Config deserialization | ✅ Clamps width/height/steps to ≥ 1; graceful on missing JSON keys |
| `src/stable_diffusion/sd_prompt_sanitizer.cpp` | Keyword blocklist | ✅ Case-insensitive; file loader skips comment lines; multi-occurrence removal |
| `src/stable_diffusion/sd_plugin.cpp` | Plugin lifecycle + policy | ✅ `isAllowed()` called before every `generate()`; `negative_prompt` also screened; mutex serialises all generate paths; exception caught; provenance unconditional |

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

| ID | Description | Severity | Target |
|----|-------------|----------|--------|
| SD-01 | Stub PNG encoder omits IDAT — pixel data not encoded | Medium | v2.2.0 |
| SD-02 | `SDCppGenerator` not yet implemented — no real inference | Medium | Q3 2026 |
| SD-03 | `generateImg2Img` stub ignores input image in non-SDCppGenerator path | Low | Q3 2026 |
| SD-04 | No model file integrity check | Low | v2.2.0 |

