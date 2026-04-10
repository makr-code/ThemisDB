# Stable Diffusion Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-04-07 | Primary: src/stable_diffusion/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

v2.1.0 — Batch generation, img2img stub interface, thread-safe plugin, and negative-prompt
content policy enforcement complete. Core pipeline operational. Content-policy sanitizer
functional. Stub mode fully functional. stable-diffusion.cpp integration compiled when
`THEMIS_ENABLE_STABLE_DIFFUSION=ON`.

## Completed ✅

- [x] `IImageGenerationBackend` interface + `THEMIS_IMGGEN_PLUGIN()` export macro
- [x] `SDPromptSanitizer` — keyword blocklist (case-insensitive, file-loadable)
- [x] `ISDGenerator` strategy interface
- [x] `SDStubGenerator` (CI / no model)
- [x] `InMemorySDGenerator` test double (with img2img inspection helpers)
- [x] `SDPlugin` — provenance stamps, blocked_count, DL entry points
- [x] `SDConfig::fromJson` / `toJson` with clamping
- [x] Minimal PNG stub encoder
- [x] `generateBatch(prompts, cfg)` on `IImageGenerationBackend` and `SDPlugin`
- [x] `generateImg2Img(prompt, cfg)` + `Img2ImgConfig` struct
- [x] `ISDGenerator::generateImg2Img()` default fallback
- [x] Thread-safety: `generate_mutex_` serialises all generate paths
- [x] `negative_prompt` content-policy enforcement (SECURITY.md gap SD-NP-01 resolved)
- [x] 45 unit tests (`SDPluginFocusedTests`, groups A–O)
- [x] Plugin manifest + CMake registration

## In Progress

- [~] Integration with `PluginManager` hot-plug monitor

## Planned Features

- [ ] Real stable-diffusion.cpp inference (`SDCppGenerator`) (Target: Q3 2026)
- [ ] ControlNet image conditioning (Target: Q4 2026)
- [ ] LoRA adapters for diffusion models (Target: Q4 2026)
- [ ] Image-to-image (img2img) generation (Target: Q4 2026)
- [ ] Real libpng/stb PNG encoder replacing stub encoder (Target: Q3 2026)
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
- [x] 30 unit tests across groups A–J (v2.0.0)
- [x] 15 additional tests groups K–O (v2.1.0): negative_prompt policy, batch, img2img

### Phase 5 — Performance / Hardening
- [x] Thread-safe generate/generateBatch/generateImg2Img (v2.1.0)
- [x] `negative_prompt` content-policy enforcement (v2.1.0)
- [ ] Real PNG encoder (libpng or stb_image_write) (Target: Q3 2026)
- [ ] Benchmark: time-to-PNG for 512×512 stub vs. real model
- [ ] Thread-safety audit (completed for current methods; SDCppGenerator pending)

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

## Production Readiness Checklist

- [x] Unit tests present (45 tests)
- [x] Stub mode for CI without model file
- [x] Injection constructor for test doubles
- [x] Content-policy sanitizer before every generate call
- [x] Provenance stamps on every result
- [x] Thread-safe generate/generateBatch/generateImg2Img
- [x] `negative_prompt` content-policy enforcement
- [x] Batch generation API
- [x] Img2img interface with stub default
- [ ] Real PNG encoder verified
- [ ] Real stable-diffusion.cpp integration validated end-to-end

## Known Issues & Limitations

- Stub PNG encoder produces a structurally valid but pixel-content-free PNG (no IDAT chunk).
- `SDCppGenerator` is not yet implemented; `SDStubGenerator` is the only generator in v2.1.0.
- `generateImg2Img` with stub/in-memory generators ignores the input image entirely.
- `SDPlugin` thread-safety applies to individual method calls; external callers must not
  share an `SDPlugin` instance across threads without external synchronisation for
  `initialize()` calls.

## Breaking Changes

None (v2.0.0 is the initial release).
