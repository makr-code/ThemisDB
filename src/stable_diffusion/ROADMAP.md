# Stable Diffusion Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-04-07 | Primary: src/stable_diffusion/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

v2.2.0 — `SDCppGenerator` wrapping the `stable-diffusion.cpp` C API shipped. Real PNG encoder (`encodeMinimalPng`) produces valid IDAT via stored-deflate + CRC32 + Adler32. `SDStubGenerator::generateImg2Img` returns input-image pass-through. 51 unit tests (groups A–Q).

## Completed ✅

- [x] `IImageGenerationBackend` interface + `THEMIS_IMGGEN_PLUGIN()` export macro
- [x] `SDPromptSanitizer` — keyword blocklist (case-insensitive, file-loadable)
- [x] `ISDGenerator` strategy interface
- [x] `SDStubGenerator` (CI / no model)
- [x] `InMemorySDGenerator` test double (with img2img inspection helpers)
- [x] `SDPlugin` — provenance stamps, blocked_count, DL entry points
- [x] `SDConfig::fromJson` / `toJson` with clamping
- [x] Minimal PNG stub encoder (v2.0.0)
- [x] `generateBatch(prompts, cfg)` on `IImageGenerationBackend` and `SDPlugin`
- [x] `generateImg2Img(prompt, cfg)` + `Img2ImgConfig` struct
- [x] `ISDGenerator::generateImg2Img()` default fallback
- [x] Thread-safety: `generate_mutex_` serialises all generate paths
- [x] `negative_prompt` content-policy enforcement (SECURITY.md gap SD-NP-01 resolved)
- [x] 45 unit tests (`SDPluginFocusedTests`, groups A–O) — v2.1.0
- [x] Plugin manifest + CMake registration
- [x] **`SDCppGenerator`** — wraps `stable-diffusion.cpp` C API under `THEMIS_ENABLE_STABLE_DIFFUSION` guard (Issue: #4590) (2026-04-12)
- [x] **Real PNG IDAT encoder** — `encodeMinimalPng()` produces valid IDAT block via stored-deflate + CRC32 + Adler32 (Issue: #4590) (2026-04-12)
- [x] **`SDStubGenerator::generateImg2Img`** — returns input-image pass-through (Issue: #4590) (2026-04-12)
- [x] **51 unit tests** (`SDPluginFocusedTests`, groups A–Q) — 6 new tests groups P–Q for SDCppGenerator + real PNG encoder (Issue: #4590) (2026-04-12)

## In Progress

- [~] Integration with `PluginManager` hot-plug monitor

## Planned Features

- [ ] Real stable-diffusion.cpp inference (`SDCppGenerator`) (Target: Q3 2026) — **completed as of v2.2.0 (Issue: #4590)**
- [ ] ControlNet image conditioning (Target: Q4 2026)
- [ ] LoRA adapters for diffusion models (Target: Q4 2026)
- [x] Image-to-image (img2img) generation — `SDStubGenerator::generateImg2Img` pass-through shipped v2.2.0 (Issue: #4590)
- [x] Real libpng/stb PNG encoder replacing stub encoder — `encodeMinimalPng` real IDAT shipped v2.2.0 (Issue: #4590)
- [ ] Batch generation (`generateBatch(prompts, cfg)`) (Target: Q4 2026)
- [ ] Perceptual hash (`pHash`) of output for deduplication (Target: Q4 2026)

## Implementation Phases

### Phase 1 — Design / API Contract ✅
- [x] `IImageGenerationBackend`, `GeneratedImage`, `SDGenerationConfig` defined
- [x] `SDPromptSanitizer` separating content policy from inference

### Phase 2 — Core Implementation ✅
- [x] `SDPlugin` wiring sanitizer → generator → provenance → PNG
- [x] `SDConfig` with JSON round-trip

### Phase 3 — Error Handling & Edge Cases ✅
- [x] Blocked prompts: `success=false`, `blocked_count++`, no generator call
- [x] Uninitialised state: `success=false`, `error_message`
- [x] Generator exception isolation

### Phase 4 — Tests ✅
- [x] 30 unit tests (`SDPluginFocusedTests`, groups A–J) (v2.0.0)
- [x] 15 additional tests groups K–O (v2.1.0): negative_prompt policy, batch, img2img
- [x] 6 additional tests groups P–Q (v2.2.0): SDCppGenerator + real PNG IDAT encoder (Issue: #4590)

### Phase 5 — Performance / Hardening
- [x] Thread-safe generate/generateBatch/generateImg2Img (v2.1.0)
- [x] `negative_prompt` content-policy enforcement (v2.1.0)
- [x] `SDCppGenerator` wrapping `stable-diffusion.cpp` C API (v2.2.0, Issue: #4590)
- [x] Real PNG encoder: `encodeMinimalPng()` with stored-deflate IDAT + CRC32 + Adler32 (v2.2.0, Issue: #4590)
- [x] `SDStubGenerator::generateImg2Img` input-image pass-through (v2.2.0, Issue: #4590)
- [ ] Benchmark: time-to-PNG for 512×512 stub vs. real model
- [ ] Thread-safety audit (completed for current methods; SDCppGenerator pending)

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

## Production Readiness Checklist

- [x] Unit tests present (51 tests, groups A–Q)
- [x] Stub mode for CI without model file
- [x] Injection constructor for test doubles
- [x] Content-policy sanitizer before every generate call
- [x] Provenance stamps on every result
- [x] Thread-safe generate/generateBatch/generateImg2Img
- [x] `negative_prompt` content-policy enforcement
- [x] Batch generation API
- [x] Img2img interface with stub default
- [x] Real PNG encoder (`encodeMinimalPng` stored-deflate IDAT + CRC32 + Adler32) — v2.2.0 (Issue: #4590)
- [x] `SDCppGenerator` wrapping `stable-diffusion.cpp` C API — v2.2.0 (Issue: #4590)
- [ ] Real stable-diffusion.cpp integration validated end-to-end (model file required)

## Known Issues & Limitations

- `SDCppGenerator` is not yet implemented; `SDStubGenerator` is the only generator in v2.1.0.
- Stub PNG encoder produces a structurally valid PNG with real IDAT data (v2.2.0: stored-deflate + CRC32 + Adler32); pixel content is white/blank for stub mode.
- `generateImg2Img` with stub/in-memory generators returns input image as pass-through (v2.2.0).
- `SDPlugin` thread-safety applies to individual method calls; external callers must not
  share an `SDPlugin` instance across threads without external synchronisation for
  `initialize()` calls.

## Breaking Changes

None (v2.0.0 is the initial release).
