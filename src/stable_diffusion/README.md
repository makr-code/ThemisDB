> **Build (Linux):** `cmake --preset linux-release && cmake --build --preset linux-release`<br>
> **Build (Windows):** `cmake --preset windows-release && cmake --build --preset windows-release`

<!-- Status: current | validated: 2026-08-09 | Primary: src/stable_diffusion/ -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# Stable Diffusion Image Generation Plugin

Image generation plugin for ThemisDB backed by stable-diffusion.cpp.

## Module Purpose

Implements text-to-image generation for ThemisDB. Provides a policy-safe `IImageGenerationBackend`
interface with a production implementation (`SDPlugin`) backed by
[stable-diffusion.cpp](https://github.com/leejet/stable-diffusion.cpp) and a zero-dependency
stub mode for CI environments without a model file.

Content-policy enforcement is built in: prompts are screened by `SDPromptSanitizer`
before reaching the generator. Blocked prompts return a `GeneratedImage` with
`success=false` without ever calling the model.

## Subsystem Scope

**In scope:** Text-to-image generation, prompt sanitization, content-policy keyword filtering,
PNG output encoding, provenance stamp injection, per-generation statistics.

**Out of scope:** Video/animation generation, image upscaling (external),
integration with the document storage pipeline (handled by `document` module).

## Relevant Interfaces

- `include/plugins/image_generation_interface.h` — `IImageGenerationBackend`, `GeneratedImage`, `SDGenerationConfig`
- `include/stable_diffusion/sd_plugin.h` — `SDPlugin` (top-level)
- `include/stable_diffusion/sd_generator.h` — `ISDGenerator`, `SDStubGenerator`, `InMemorySDGenerator`
- `include/stable_diffusion/sd_prompt_sanitizer.h` — `SDPromptSanitizer`
- `include/stable_diffusion/sd_config.h` — `SDConfig`

## Current Delivery Status

**Maturity:** 🟡 Beta (v2.2.0) — Core pipeline operational in stub mode and with optional
real inference when `THEMIS_ENABLE_STABLE_DIFFUSION=ON` and a model file is present.

## Quick Start

```cpp
#include "stable_diffusion/sd_plugin.h"

// Stub mode (no model required)
auto plugin = std::make_unique<themis::imggen::SDPlugin>();
plugin->initialize("", {});

themis::imggen::SDGenerationConfig cfg;
cfg.width = 512; cfg.height = 512; cfg.steps = 20;
auto img = plugin->generate("a mountain landscape at sunset", cfg);
if (img.success) {
    // img.png_data contains the PNG bytes
}

// With injected generator (tests)
auto gen = std::make_unique<themis::imggen::InMemorySDGenerator>();
gen->setNextPixels({0,0,0}, 1, 1);
auto plugin = std::make_unique<themis::imggen::SDPlugin>(std::move(gen),
    themis::imggen::SDPromptSanitizer({"nsfw"}));
```

## Architecture Overview

```
┌──────────────────────────────────────┐
│            SDPlugin                  │  ← IImageGenerationBackend
│  ┌────────────────────────────────┐  │
│  │     SDPromptSanitizer          │  │  content-policy keyword blocklist
│  └────────────────────────────────┘  │
│  ┌────────────────────────────────┐  │
│  │       ISDGenerator             │  │  ← strategy
│  │  ├── SDStubGenerator           │  │     (no model, CI)
│  │  ├── InMemorySDGenerator       │  │     (test double)
│  │  └── (SDCppGenerator)          │  │     (real model, optional)
│  └────────────────────────────────┘  │
└──────────────────────────────────────┘
```

## Build

```cmake
# Stub mode (default)
cmake -B build && cmake --build build --target test_sd_plugin

# Real inference
cmake -B build -DTHEMIS_ENABLE_STABLE_DIFFUSION=ON
```

## Test Suite

| Suite | Count | Labels |
|---|---|---|
| `SDPluginFocusedTests` | 62 | `plugins;stable_diffusion;image_generation;v2.2.0` |

```bash
ctest -R SDPluginFocusedTests --output-on-failure
```

## Dependencies

| Dependency | Required | Purpose |
|---|---|---|
| `nlohmann_json` | ✅ | config / stats |
| `stable-diffusion.cpp` | ❌ optional | real model inference |

## Provenance Fields

Every `GeneratedImage` carries:

| Field | Value |
|---|---|
| `plugin_version` | `"2.1.0"` |
| `generation_timestamp` | Unix epoch milliseconds |
| `prompt_hash` | FNV-1a 64-bit hex of sanitised prompt |
| `perceptual_hash` | optional 64-bit perceptual hash (non-fatal if unavailable) |
| `model_id` | model path or `"stub"` |

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/stable_diffusion/README.md`](../../include/stable_diffusion/README.md) for the public API.

## See Also

- [`../../include/stable_diffusion/README.md`](../../include/stable_diffusion/README.md) — public API reference (headers, config, usage, troubleshooting)
- [`ARCHITECTURE.md`](./ARCHITECTURE.md) — component diagram and data-flow
- [`ROADMAP.md`](./ROADMAP.md) — delivery status and planned features
- [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) — remaining hardening and E2E follow-ups
- [`SECURITY.md`](./SECURITY.md) — module security notes
- [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md) — production gates and release criteria
- [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) — benchmark expectations
- [`../../docs/en/stable_diffusion/index.md`](../../docs/en/stable_diffusion/index.md) — English secondary overview
- [`../../docs/de/stable_diffusion/index.md`](../../docs/de/stable_diffusion/index.md) — Deutsche Sekundärübersicht
