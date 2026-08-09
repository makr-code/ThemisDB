> **Build (Linux):** `cmake --preset linux-release && cmake --build --preset linux-release`<br>
> **Build (Windows):** `cmake --preset windows-release && cmake --build --preset windows-release`

# stable_diffusion Module Headers

<!-- Status: current | validated: 2026-08-09 | Primary: include/stable_diffusion/ | Secondary: docs/de/stable_diffusion/ -->
<!-- Links: ../../src/stable_diffusion/README.md · ../../src/stable_diffusion/ROADMAP.md · ../../src/stable_diffusion/FUTURE_ENHANCEMENTS.md -->

Public headers for Stable Diffusion image-generation plugin integration.

## Headers

| Header | Purpose |
|---|---|
| `sd_config.h` | `SDConfig` struct with JSON round-trip (`fromJson` / `toJson`) for plugin initialization |
| `sd_generator.h` | `ISDGenerator` strategy interface; `SDStubGenerator` (CI/no-model); `InMemorySDGenerator` (test double); `SDCppGenerator` (real backend, compiled only when `THEMIS_ENABLE_STABLE_DIFFUSION` is defined) |
| `sd_plugin.h` | `SDPlugin` — top-level `IImageGenerationBackend` implementation wiring sanitizer, generator, and provenance stamps |
| `sd_plugin_registrar.h` | `SDPluginAdapter` (`IThemisPlugin` wrapper) and `SDPluginRegistrar` factory / hot-plug helpers |
| `sd_prompt_sanitizer.h` | `SDPromptSanitizer` — case-insensitive keyword blocklist; file-loadable |

## Public API Surface

### `SDPlugin` (`sd_plugin.h`)

- `SDPlugin()` — default constructor; uses `SDStubGenerator` (no model required)
- `SDPlugin(std::unique_ptr<ISDGenerator>, SDPromptSanitizer)` — injection constructor for tests
- `initialize(model_path, config)` — load model and apply `SDConfig`
- `isInitialized() const` — readiness check
- `generate(prompt, cfg)` — text-to-image; returns `GeneratedImage` with provenance stamps
- `generateBatch(prompts, cfg)` — generate multiple images; results in same order as input
- `generateImg2Img(prompt, cfg)` — image-to-image denoising; stub/in-memory generators fall back to text-to-image
- `isPromptAllowed(prompt) const` — content-policy pre-check without generating
- `getModelId() const` — active model path or `"stub"`
- `getPluginVersion() const` — returns `"2.3.0"`
- `getStatistics() const` — JSON: `generation_count`, `blocked_count`, `error_count`, `model_path`

### `ISDGenerator` / concrete generators (`sd_generator.h`)

- `ISDGenerator::initialize(cfg)` / `isInitialized()` / `getModelId()`
- `ISDGenerator::generate(prompt, cfg, out_width, out_height, out_seed)` — returns raw RGB byte buffer
- `ISDGenerator::generateImg2Img(prompt, cfg, ...)` — default falls back to `generate()`
- `SDStubGenerator` — always available; returns zero-filled RGB; img2img passes input image through
- `InMemorySDGenerator` — pre-settable pixel data; records img2img call details for assertions
- `SDCppGenerator` — delegates to `stable-diffusion.cpp` C API; available only when `THEMIS_ENABLE_STABLE_DIFFUSION` is defined

### `SDPromptSanitizer` (`sd_prompt_sanitizer.h`)

- `SDPromptSanitizer(blocked_keywords)` — construct from keyword vector
- `SDPromptSanitizer::fromFile(path)` — load one keyword per line (`#` = comment)
- `isAllowed(prompt) const` — `false` if any blocked keyword found (case-insensitive)
- `sanitize(prompt) const` — returns prompt with all blocked keywords removed
- `blockedCount() const` — number of active keywords

### `SDConfig` (`sd_config.h`)

- `SDConfig::fromJson(j)` — construct from `nlohmann::json`
- `SDConfig::toJson() const` — serialize to `nlohmann::json`

### `SDPluginRegistrar` / `SDPluginAdapter` (`sd_plugin_registrar.h`)

- `SDPluginRegistrar::createPlugin(config)` — create standalone `SDPlugin`
- `SDPluginRegistrar::createAdapter(config)` — create `SDPluginAdapter` for `plugins::PluginManager`
- `SDPluginRegistrar::defaultReloadCallback()` — default hot-plug reload handler
- `SDPluginRegistrar::enableHotPlug(manager, directory)` / `disableHotPlug(manager)`
- `SDPluginAdapter::initialize(config_json)` / `shutdown()` / `getInstance()`

## Runtime Configuration Keys

`SDConfig` (used in `SDPlugin::initialize()` and `SDPluginRegistrar`) accepts:

| Key | Type | Default | Notes |
|---|---|---|---|
| `model_path` | string | `""` | Path to GGUF/safetensors model; empty → `SDStubGenerator` used |
| `width` | integer | `512` | Output image width in pixels; clamped to minimum 1 |
| `height` | integer | `512` | Output image height in pixels; clamped to minimum 1 |
| `steps` | integer | `20` | Diffusion steps; clamped to minimum 1 |
| `cfg_scale` | float | `7.0` | Classifier-free guidance scale |
| `sampler` | string | `"euler_a"` | Sampler name: `euler`, `euler_a` (recommended for general use), `heun`, `dpm2`, `dpm++2s_a`, `dpm++2m`, `dpm++2mv2`, `lcm`; unknown names fall back to `euler_a` |
| `seed` | integer | `-1` | Fixed seed; `-1` = random |
| `blocked_keywords_file` | string | `""` | Path to plain-text keyword blocklist (one per line) |
| `negative_prompt` | string | `""` | Negative conditioning text; screened by `SDPromptSanitizer` |
| `model_sha256` | string | `""` | Optional expected SHA-256 digest for fail-closed model integrity verification |

## Runtime Behavior, Errors, and Limits

- **Stub mode** is always available without a model file; `SDStubGenerator` returns a
  zero-filled RGB buffer (solid black) and records the seed actually used.
- **Real inference** (`SDCppGenerator`) requires `THEMIS_ENABLE_STABLE_DIFFUSION=ON` at
  build time and a valid GGUF or safetensors model at the path given to `initialize()`.
- **Content policy**: every `generate()`, `generateBatch()`, and `generateImg2Img()` call
  screens both `prompt` and `negative_prompt` through `SDPromptSanitizer::isAllowed()`
  before reaching the generator. Blocked prompts return `success=false` without calling
  the model and increment `blocked_count`.
- **Uninitialized plugin**: calling `generate*()` before `initialize()` returns
  `GeneratedImage{success=false, error_message="...not initialized..."}`.
- **Generator exception**: exceptions thrown by the generator are caught; the result is
  `success=false` with `error_message=exception.what()` and `error_count` is incremented.
- **Thread safety**: `generate()`, `generateBatch()`, and `generateImg2Img()` are
  serialized by an internal `std::mutex`. `initialize()` and `shutdown()` are not
  concurrently safe with each other or with generate paths.
- **`SDCppGenerator` parallel calls** are serialized within the generator instance and
  validated by an opt-in real-backend concurrency audit test target
  (`SDPluginRealBackendE2ETests`); external deployment policy may still choose stricter
  process-level serialization depending on GPU runtime constraints.
- **Provenance stamps** (`plugin_version`, `generation_timestamp`, `prompt_hash`) are
  always set by `SDPlugin` on every result path regardless of generator implementation.
- **`prompt_hash`** uses FNV-1a 64-bit (not cryptographic; the algorithm is fixed across
  plugin versions so hash values are stable and reproducible for the same sanitized prompt).

## Usage

### Stub mode (no model required)

```cpp
#include "stable_diffusion/sd_plugin.h"

auto plugin = std::make_unique<themis::imggen::SDPlugin>();
plugin->initialize("", {});

themis::imggen::SDGenerationConfig cfg;
cfg.width = 512; cfg.height = 512; cfg.steps = 20;

auto img = plugin->generate("a mountain landscape at sunset", cfg);
if (img.success) {
    // img.png_data contains valid PNG bytes
    // img.prompt_hash holds the FNV-1a fingerprint of the sanitised prompt
}
```

### Batch generation

```cpp
#include "stable_diffusion/sd_plugin.h"

themis::imggen::SDPlugin plugin;
plugin.initialize("", {});

themis::imggen::SDGenerationConfig cfg;
cfg.width = 256; cfg.height = 256; cfg.steps = 10;

auto results = plugin.generateBatch({"a cat", "a dog", "a house"}, cfg);
for (const auto& img : results) {
    if (img.success) { /* process img.png_data */ }
}
```

### Injecting a test double

```cpp
#include "stable_diffusion/sd_plugin.h"
#include "stable_diffusion/sd_generator.h"
#include "stable_diffusion/sd_prompt_sanitizer.h"

auto gen = std::make_unique<themis::imggen::InMemorySDGenerator>();
gen->setNextPixels({0xFF, 0x00, 0x00}, 1, 1);  // 1×1 red pixel

themis::imggen::SDPlugin plugin(std::move(gen),
    themis::imggen::SDPromptSanitizer({"nsfw"}));
plugin.initialize("", {});

auto img = plugin.generate("a red square", {});
// img.success == true, img.png_data == valid 1×1 PNG
```

### PluginManager integration via registrar

```cpp
#include "stable_diffusion/sd_plugin_registrar.h"
#include "plugins/plugin_manager.h"

nlohmann::json cfg = {{"model_path", "/models/sd-v1-5.gguf"}};
auto adapter = themis::imggen::SDPluginRegistrar::createAdapter(cfg);
// hand to plugins::PluginManager::registerPlugin() or enable hot-plug:
themis::imggen::SDPluginRegistrar::enableHotPlug(manager, "/models/");
```

## Installation

This module ships with ThemisDB. Add the repository `include/` directory to your
target's include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Troubleshooting

- **`initialize()` always succeeds but images are black**: no real model is loaded;
  `SDStubGenerator` is active. Pass a non-empty `model_path` and build with
  `THEMIS_ENABLE_STABLE_DIFFUSION=ON`.
- **`generate()` returns `success=false` with "not initialized"**: call `initialize()`
  before any generate path.
- **Prompt blocked unexpectedly**: inspect `isPromptAllowed(prompt)` and check the
  keyword list passed to the constructor or loaded via `blocked_keywords_file`.
- **`SDCppGenerator` not available**: the build does not include
  `THEMIS_ENABLE_STABLE_DIFFUSION=ON` or the `stable-diffusion.cpp` submodule is not
  present; the plugin automatically falls back to `SDStubGenerator`.
- **`generateImg2Img()` ignores the input image**: stub and in-memory generators fall back
  to text-to-image; real denoising requires `SDCppGenerator` (see ROADMAP Phase 5).
- **Thread-safety concerns with `SDCppGenerator`**: parallel calls depend on the
  downstream `stable-diffusion.cpp` runtime; serialize externally or await the
  concurrency audit tracked in ROADMAP.md.

## See Also

- [`../../src/stable_diffusion/README.md`](../../src/stable_diffusion/README.md) — implementation overview and quick start
- [`../../src/stable_diffusion/ARCHITECTURE.md`](../../src/stable_diffusion/ARCHITECTURE.md) — component diagram and data-flow
- [`../../src/stable_diffusion/ROADMAP.md`](../../src/stable_diffusion/ROADMAP.md) — delivery status and planned features
- [`../../src/stable_diffusion/FUTURE_ENHANCEMENTS.md`](../../src/stable_diffusion/FUTURE_ENHANCEMENTS.md) — remaining hardening and E2E follow-ups
- [`../../src/stable_diffusion/SECURITY.md`](../../src/stable_diffusion/SECURITY.md) — module security notes
- [`../../src/stable_diffusion/PRODUCTION_REQUIREMENTS.md`](../../src/stable_diffusion/PRODUCTION_REQUIREMENTS.md) — production gates and release criteria
- [`../../src/stable_diffusion/PERFORMANCE_EXPECTATIONS.md`](../../src/stable_diffusion/PERFORMANCE_EXPECTATIONS.md) — benchmark expectations
- [`../../docs/en/stable_diffusion/index.md`](../../docs/en/stable_diffusion/index.md) — English secondary overview
- [`../../docs/de/stable_diffusion/index.md`](../../docs/de/stable_diffusion/index.md) — Deutsche Sekundärübersicht
