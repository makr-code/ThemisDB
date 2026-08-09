> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# Stable Diffusion Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-08-09 | Primary: src/stable_diffusion/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Core pipeline is operational in stub mode and optional real-backend mode.
ControlNet request fields, LoRA request handling, perceptual hash metadata,
model SHA-256 integrity validation, and request dimension guards are implemented
in `SDPlugin` and covered by focused tests.

## Completed ✅

- [x] `IImageGenerationBackend` interface + `THEMIS_IMGGEN_PLUGIN()` export macro
- [x] `SDPromptSanitizer` — keyword blocklist (case-insensitive, file-loadable)
- [x] `ISDGenerator` strategy interface
- [x] `SDStubGenerator` (CI / no model) with img2img pass-through
- [x] `InMemorySDGenerator` test double (with img2img inspection helpers)
- [x] `SDPlugin` — provenance stamps, blocked_count, DL entry points
- [x] `SDConfig::fromJson` / `toJson` with clamping
- [x] Real PNG encoder with IDAT chunk (stored-deflate, CRC-32, Adler-32 — no external deps)
- [x] `generateBatch(prompts, cfg)` on `IImageGenerationBackend` and `SDPlugin`
- [x] `generateImg2Img(prompt, cfg)` + `Img2ImgConfig` struct
- [x] `ISDGenerator::generateImg2Img()` default fallback
- [x] `SDStubGenerator::generateImg2Img()` — input-image pass-through
- [x] `SDCppGenerator` — real stable-diffusion.cpp wrapper (`THEMIS_ENABLE_STABLE_DIFFUSION` guard)
- [x] Thread-safety: `generate_mutex_` serialises all generate paths
- [x] `negative_prompt` content-policy enforcement (SECURITY.md gap SD-NP-01 resolved)
- [x] ControlNet request fields (`control_image_rgb`, `control_model_path`, `control_strength`)
- [x] LoRA request application (`applyLoRA`) with per-request scale validation
- [x] Perceptual hash metadata (`GeneratedImage::perceptual_hash`) with non-fatal fallback
- [x] Model SHA-256 verification gate in `initialize()` (`model_sha256`)
- [x] Dimension guards (`<=8192`, overflow-safe, positive dimensions)
- [x] 62 unit tests (`SDPluginFocusedTests`, groups A–Q)
- [x] Plugin manifest + CMake registration
- [x] **`SDCppGenerator`** — wraps `stable-diffusion.cpp` C API under `THEMIS_ENABLE_STABLE_DIFFUSION` guard (Issue: #4590) (2026-04-12)
- [x] **Real PNG IDAT encoder** — `encodeMinimalPng()` produces valid IDAT block via stored-deflate + CRC32 + Adler32 (Issue: #4590) (2026-04-12)
- [x] **`SDStubGenerator::generateImg2Img`** — returns input-image pass-through (Issue: #4590) (2026-04-12)
- [x] **51 unit tests** (`SDPluginFocusedTests`, groups A–Q) — 6 new tests groups P–Q for SDCppGenerator + real PNG encoder (Issue: #4590) (2026-04-12)
- [x] **`SDPluginAdapter` + `SDPluginRegistrar`** — `IThemisPlugin` adapter wrapping `SDPlugin`; `createPlugin`, `createAdapter`, `defaultReloadCallback`, `enableHotPlug`, `disableHotPlug`; 12 unit tests (`SDPluginRegistrarTests`, groups A–D) (2026-04-16)

## In Progress

*(none)*

## Planned Features

- [~] Benchmark gate: stable_diffusion benchmark target added; baseline publication pending (Target: Q3 2026)
- [ ] End-to-end real-model gate (`THEMIS_ENABLE_STABLE_DIFFUSION=ON`) in CI (Target: Q3 2026)
- [ ] Dedicated parallel-call audit for `SDCppGenerator` backend semantics (Target: Q4 2026)

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
- [x] Focused coverage expanded to 62 tests across groups A–Q
- [x] Added coverage for model SHA-256 gate, dimension guards, ControlNet+LoRA request flow, pHash behavior

### Phase 5 — Performance / Hardening
- [x] Thread-safe generate/generateBatch/generateImg2Img (v2.1.0)
- [x] `negative_prompt` content-policy enforcement (v2.1.0)
- [x] Real PNG encoder — stored-deflate zlib with CRC-32 and Adler-32 (v2.2.0)
- [x] Dimension guard enforcement for generation/img2img/control image paths
- [x] Model SHA-256 verification gate at initialization
- [~] Benchmark: time-to-PNG target added (`bench_stable_diffusion_release_gates`); real-model baseline pending
- [ ] Thread-safety audit for SDCppGenerator (parallel calls)

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

## Production Readiness Checklist

- [x] Unit tests present (62 tests)
- [x] Stub mode for CI without model file
- [x] Injection constructor for test doubles
- [x] Content-policy sanitizer before every generate call
- [x] Provenance stamps on every result
- [x] Thread-safe generate/generateBatch/generateImg2Img
- [x] `negative_prompt` content-policy enforcement
- [x] Batch generation API
- [x] Img2img interface with stub pass-through
- [x] Real PNG encoder (IDAT chunk, CRC-32, Adler-32 — no external deps)
- [x] `SDCppGenerator` — real stable-diffusion.cpp integration (compiled when `THEMIS_ENABLE_STABLE_DIFFUSION=ON`)
- [ ] End-to-end integration test with a real GGUF model file
- [ ] `SDCppGenerator` thread-safety audit for parallel calls

### Phase 7 — PluginManager Hot-Plug Integration ✅ (v2.3.0)
- [x] `SDPluginAdapter : IThemisPlugin` — wraps `SDPlugin`, implements `initialize(config_json)`, `shutdown()`, `getType()`, `getCapabilities()`, `getInstance()`; `PluginType::IMAGE_GENERATION`
- [x] `SDPluginRegistrar` — `createPlugin()`, `createAdapter()`, `defaultReloadCallback()`, `enableHotPlug()`, `disableHotPlug()`
- [x] 12 unit tests (`SDPluginRegistrarTests`, groups A–D) in `src/stable_diffusion/tests/test_sd_plugin_registrar.cpp`

## Known Issues & Limitations

- `SDCppGenerator` requires the `stable-diffusion.cpp` submodule and `THEMIS_ENABLE_STABLE_DIFFUSION=ON`;
  without them `SDStubGenerator` is used automatically.
- `SDPlugin` thread-safety applies to individual method calls; external callers must not
  share an `SDPlugin` instance across threads without external synchronisation for
  `initialize()` calls.
- `SDCppGenerator` parallel-call safety depends on stable-diffusion.cpp's own thread model;
  a full audit is pending.

## Breaking Changes

None (v2.0.0 is the initial release).

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🧪 NUR_TESTS (implementiert, kein Produktions-Aufrufer)

- `SDPlugin` – Stable-Diffusion-Plugin-Implementierung; Tests + Plugin-Registrar vorhanden
  > **Aktion:** ROADMAP-Ticket für Produktions-Integration ergänzen oder als CANDIDATE_FOR_REMOVAL markieren.

### 🟡 UNGENUTZT (kein Test, kein externer Aufrufer)

- `samplerFromString` – Parst Sampler-Namen (euler, ddim, …) zu Enum; Header-only Helper
  > **Aktion:** Für jedes Symbol entscheiden: (1) Verdrahten, (2) Testen oder (3) als CANDIDATE_FOR_REMOVAL einplanen.
