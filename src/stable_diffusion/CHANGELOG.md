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

## [2.0.0] — 2026-04-07

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
