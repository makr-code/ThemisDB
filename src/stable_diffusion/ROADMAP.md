# Stable Diffusion Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-04-07 | Primary: src/stable_diffusion/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

v2.0.0 — Core pipeline operational. Content-policy sanitizer functional. Stub mode fully
functional. stable-diffusion.cpp integration compiled when
`THEMIS_ENABLE_STABLE_DIFFUSION=ON`.

## Completed ✅

- [x] `IImageGenerationBackend` interface + `THEMIS_IMGGEN_PLUGIN()` export macro
- [x] `SDPromptSanitizer` — keyword blocklist (case-insensitive, file-loadable)
- [x] `ISDGenerator` strategy interface
- [x] `SDStubGenerator` (CI / no model)
- [x] `InMemorySDGenerator` test double
- [x] `SDPlugin` — provenance stamps, blocked_count, DL entry points
- [x] `SDConfig::fromJson` / `toJson` with clamping
- [x] Minimal PNG stub encoder
- [x] 30 unit tests (`SDPluginFocusedTests`)
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
- [x] 30 unit tests across groups A–J

### Phase 5 — Performance / Hardening
- [ ] Real PNG encoder (libpng or stb_image_write) (Target: Q3 2026)
- [ ] Benchmark: time-to-PNG for 512×512 stub vs. real model
- [ ] Thread-safety audit (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

## Production Readiness Checklist

- [x] Unit tests present (30 tests)
- [x] Stub mode for CI without model file
- [x] Injection constructor for test doubles
- [x] Content-policy sanitizer before every generate call
- [x] Provenance stamps on every result
- [ ] Real PNG encoder verified
- [ ] Real stable-diffusion.cpp integration validated end-to-end
- [ ] Thread-safety verified

## Known Issues & Limitations

- Stub PNG encoder produces a structurally valid but pixel-content-free PNG (no IDAT chunk).
- `SDCppGenerator` is not yet implemented; `SDStubGenerator` is the only generator in v2.0.0.
- `SDPlugin` is not thread-safe for concurrent `generate()` calls.

## Breaking Changes

None (v2.0.0 is the initial release).
