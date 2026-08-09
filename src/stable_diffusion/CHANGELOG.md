> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-08-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Stable Diffusion Plugin

All notable changes to the Stable Diffusion image generation plugin are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [2.3.0] — 2026-08-09

- Added `SDPluginRegistrarTests` wiring in `tests/CMakeLists.txt` and restored registrar gate coverage in CTest.
- Added opt-in real-backend E2E suite `SDPluginRealBackendE2ETests` (`text2img`, `img2img`, SHA-256 init gate, parallel generate audit).
- Added internal serialization for `SDCppGenerator` API calls to harden concurrent request paths.
- Added stable_diffusion benchmark baseline artifact scaffold at `benchmarks/stable_diffusion/BASELINE.md`.
- Aligned plugin manifest capability metadata with runtime behavior (`supports_batching=true`, `thread_safe=true`).
- Aligned module/library version metadata to `2.3.0` and refreshed related docs.
- ControlNet request fields and validation in generate/img2img paths
- LoRA request application (`applyLoRA`) with validation/error propagation
- Perceptual hash (`perceptual_hash`) metadata on successful outputs (non-fatal fallback)
- `model_sha256` initialization gate for model integrity verification
- Dimension guards (`<=8192`, overflow-safe checks) for generation and control/img2img buffers
- Focused test coverage expanded to 62 tests in `SDPluginFocusedTests`
- SDCppGenerator parallel-call thread-safety audit

## [2.2.0] — 2026-04-12

### Added
- `SDCppGenerator` backend integration for real stable-diffusion.cpp inference (`THEMIS_ENABLE_STABLE_DIFFUSION=ON`)
- Real PNG IDAT encoding path in `encodeMinimalPng()` (stored-deflate, CRC-32, Adler-32)
- `SDStubGenerator::generateImg2Img()` input-image pass-through behavior
- 6 additional focused tests (Groups P–Q) in `SDPluginFocusedTests` (total: 51)

## [2.1.0] — 2026-04-10

### Added
- `generateBatch(prompts, cfg)` on `IImageGenerationBackend` (default: sequential loop) and `SDPlugin`
- `generateImg2Img(prompt, cfg)` on `IImageGenerationBackend` (default: falls back to `generate()`) and `SDPlugin`
- `Img2ImgConfig` struct in `image_generation_interface.h`
- `ISDGenerator::generateImg2Img()` virtual fallback to `generate()`
- `InMemorySDGenerator::generateImg2Img()` inspection helpers
- Thread-safety guard: `generate_mutex_` serializes `generate()`, `generateBatch()`, and `generateImg2Img()`
- `negative_prompt` content-policy enforcement in all generate paths
- 15 new unit tests (Groups K–O), bringing the total to 45 for v2.1.0

### Changed
- `plugin_version` updated to `2.1.0`

## [2.0.0] — 2026-04-07

### Added
- `SDPlugin` top-level `IImageGenerationBackend` implementation
- `SDPromptSanitizer` keyword blocklist with `isAllowed()` / `sanitize()` and `fromFile()`
- `ISDGenerator` strategy interface
- `SDStubGenerator` fallback generator
- `InMemorySDGenerator` test double
- `SDConfig` JSON round-trip (`fromJson` / `toJson`) with clamping
- `prompt_hash` (FNV-1a 64-bit hex of sanitized prompt)
- Initial PNG encoder path
- Plugin statistics (`generation_count`, `blocked_count`, `error_count`)
- Initial focused test suite (30 tests, Groups A–J)
