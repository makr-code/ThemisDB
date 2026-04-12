<!-- Status: current | validated: 2026-04-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Stable Diffusion Plugin

All notable changes to the Stable Diffusion image generation plugin are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Real stable-diffusion.cpp inference integration (`THEMIS_ENABLE_STABLE_DIFFUSION=ON`)
- ControlNet support for image conditioning
- LoRA adapter support for diffusion models
- Image-to-image (img2img) generation
- Scheduler selection beyond euler_a / dpm++

## [2.1.0] — 2026-04-10

### Added
- `generateBatch(prompts, cfg)` on `IImageGenerationBackend` (default: sequential loop) and
  `SDPlugin` (mutex-serialised; returns one `GeneratedImage` per prompt in order)
- `generateImg2Img(prompt, cfg)` on `IImageGenerationBackend` (default: falls back to
  `generate()`) and `SDPlugin` (full content-policy + provenance path)
- `Img2ImgConfig` struct in `image_generation_interface.h`: inherits `SDGenerationConfig`,
  adds `input_image_rgb`, `input_width`, `input_height`, `strength` (0.75 default), `mask_rgb`
- `ISDGenerator::generateImg2Img()` virtual method with default fallback to `generate()`
- `InMemorySDGenerator::generateImg2Img()` with inspection helpers:
  `img2imgCalled()`, `lastImg2ImgStrength()`, `lastImg2ImgInputWidth/Height()`
- Thread-safety: `SDPlugin::generate()`, `generateBatch()`, `generateImg2Img()` all
  serialised by `generate_mutex_` (internal `std::mutex`)
- `negative_prompt` content-policy enforcement: `SDPlugin` now calls
  `SDPromptSanitizer::isAllowed()` on `cfg.negative_prompt` before every generate call;
  blocked negative prompts return `success=false` with `error_message` containing
  `"negative_prompt blocked by content policy"` (fixes SECURITY.md gap SD-NP-01)
- `SDPlugin::generateLocked()` internal helper to avoid recursive locking in batch path
- 15 new unit tests (Groups K–O) in `SDPluginFocusedTests` (total: 45)

### Changed
- `plugin_version` bumped from `"2.0.0"` to `"2.1.0"`
- `SDPlugin` now includes `<mutex>` header; removed unused `<atomic>` include



### Added
- `SDPlugin` — top-level `IImageGenerationBackend` implementation with provenance stamps
  (`plugin_version`, `generation_timestamp`, `prompt_hash`)
- `SDPromptSanitizer` — case-insensitive keyword blocklist with `isAllowed()` and
  `sanitize()` methods; loadable from a plain-text file (`fromFile()`)
- `ISDGenerator` — strategy interface separating model inference from plugin lifecycle
- `SDStubGenerator` — always-available fallback returning a black pixel buffer
- `InMemorySDGenerator` — test double with `setNextPixels` / pre-set dimensions and seed
- `SDConfig` — runtime config with `fromJson` / `toJson` round-trip; clamps invalid
  dimensions and step counts
- `prompt_hash` = FNV-1a 64-bit hex of sanitised prompt (stable fingerprint without
  libcrypto dependency)
- Minimal PNG encoder producing a valid PNG with IHDR + IEND (stub; real encoder planned)
- `blocked_count`, `generation_count`, `error_count` in `getStatistics()`
- 30 unit tests (`SDPluginFocusedTests`, groups A–J)
- `plugins/stable_diffusion/plugin.json.in` — plugin manifest
- `src/stable_diffusion/CMakeLists.txt` — build target
- `tests/CMakeLists.txt` — `SDPluginFocusedTests` registered
- `plugins/CMakeLists.txt` — `THEMIS_PLUGIN_STABLE_DIFFUSION` option added
