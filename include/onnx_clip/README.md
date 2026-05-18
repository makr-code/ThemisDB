> **Build (Linux):** `cmake --preset linux-release && cmake --build --preset linux-release`
> **Build (Windows):** `cmake --preset windows-release && cmake --build --preset windows-release`

# onnx_clip Module Headers

<!-- Status: current | validated: 2026-05-13 | Primary: src/onnx_clip/ | Secondary: docs/de/onnx_clip/ -->
<!-- Links: ../../src/onnx_clip/README.md · ../../src/onnx_clip/ROADMAP.md · ../../src/onnx_clip/FUTURE_ENHANCEMENTS.md -->

This directory currently contains module-facing documentation only. The ONNX-CLIP public
entry point is still implemented as the source-local header
[`src/onnx_clip/onnx_clip_plugin.h`](../../src/onnx_clip/onnx_clip_plugin.h), which
in-tree targets include as `onnx_clip/onnx_clip_plugin.h` because the build exposes the
repository `src/` directory on the include path.

## Public Entry Point

| Header / Entry Point | Purpose |
|---|---|
| `onnx_clip/onnx_clip_plugin.h` | `ONNXClipPlugin` implementation of `IImageAnalysisBackend` for deterministic image and text embeddings, batching, health/status, and optional model hash validation |
| `THEMIS_IMAGE_PLUGIN(themis::plugins::image::ONNXClipPlugin)` | Dynamic plugin export entry point, disabled for focused unit-test binaries |

## Public API Surface

- Lifecycle: `ONNXClipPlugin()`, `initialize(config, backend)`, `shutdown()`, `isReady()`
- Capability / metadata: `getInfo()`, `getBackend()`, `healthCheck()`, `getStatistics()`, `warmup()`
- Embedding APIs: `generateEmbedding(image_data, metadata)`, `generateEmbeddingBatch(images)`, `generateTextEmbedding(text)`
- Integrity fallback hook: `setModelHashFn(fn)` for non-OpenSSL builds that still need SHA-256 verification

## Runtime Configuration Keys

The `PluginConfig` consumed by `initialize()` currently reads:

| Key | Type | Default | Behavior |
|---|---|---|---|
| `model.name` | string | `clip-vit-base-patch32` | Model label reported in results/statistics |
| `model.embedding_dim` | integer | `512` | Embedding dimension; invalid or non-positive values fall back to `512` |
| `max_batch_size` | integer | `16` on CPU, `64` otherwise | Upper bound for sub-batch splitting in `generateEmbeddingBatch()` |
| `model.path` | string | empty | Optional model file path used for integrity verification |
| `model.expected_sha256` | string | empty | If set together with `model.path`, `initialize()` verifies the file hash and fails on mismatch |

## Runtime Behavior, Errors, and Limits

- `BackendType::AUTO` resolves to `CPU` in the current portable implementation.
- `generateEmbeddingBatch()` preserves input order and processes items in sub-batches up to `max_batch_size`; it does not issue a single native batched ONNX call yet.
- Empty image payloads return `EmbeddingResult{success=false, error_message="Image data is empty"}`.
- Empty text input returns `EmbeddingResult{success=false, error_message="Text input is empty"}`.
- Calling embedding APIs before `initialize()` returns `success=false` results and increments error counters.
- Statistics include readiness, backend, model name, `max_batch_size`, totals for image/text/batch calls, and Prometheus-style counters.
- Model-hash verification succeeds via OpenSSL when available; without OpenSSL, `setModelHashFn()` can inject a verifier and otherwise the check is skipped.

## Usage

```cpp
#include "onnx_clip/onnx_clip_plugin.h"
#include <nlohmann/json.hpp>

using namespace themis::plugins::image;

ONNXClipPlugin plugin;
nlohmann::json settings = {
    {"model", {
        {"name", "clip-vit-large-patch14"},
        {"embedding_dim", 768}
    }},
    {"max_batch_size", 8}
};

PluginConfig config(settings);
plugin.initialize(config, BackendType::CPU);

auto image_result = plugin.generateEmbedding(std::vector<uint8_t>{1, 2, 3, 4});
auto text_result = plugin.generateTextEmbedding("a photo of a lighthouse");
auto stats = plugin.getStatistics();
```

## Installation

This module ships with ThemisDB. For in-tree targets, make sure the repository `include/`
and `src/` directories are available on the include path for the current target.

```cmake
target_include_directories(your_target PRIVATE
    ${THEMISDB_INCLUDE_DIR}
    ${THEMISDB_SOURCE_DIR}/src
)
```

## Troubleshooting

- **`initialize()` selects CPU although `AUTO` was requested**: expected in the current generic implementation; choose `BackendType::CUDA`, `DIRECTML`, or `TENSORRT` explicitly if the surrounding build supports them.
- **`generateEmbeddingBatch()` is slower than expected for large batches**: the module currently sub-splits into sequential batches; see `FUTURE_ENHANCEMENTS.md` and `PERFORMANCE_EXPECTATIONS.md` for the native-batch follow-up.
- **Integrity verification does not run in local builds**: set `model.path` and `model.expected_sha256`, then ensure OpenSSL is available or register `setModelHashFn()` in non-OpenSSL builds.

## See Also

- [`../../src/onnx_clip/README.md`](../../src/onnx_clip/README.md) — implementation overview
- [`../../src/onnx_clip/ARCHITECTURE.md`](../../src/onnx_clip/ARCHITECTURE.md) — architecture and data flow
- [`../../src/onnx_clip/ROADMAP.md`](../../src/onnx_clip/ROADMAP.md) — delivery status and phased roadmap
- [`../../src/onnx_clip/FUTURE_ENHANCEMENTS.md`](../../src/onnx_clip/FUTURE_ENHANCEMENTS.md) — open enhancement work
- [`../../src/onnx_clip/SECURITY.md`](../../src/onnx_clip/SECURITY.md) — module security notes
- [`../../src/onnx_clip/PERFORMANCE_EXPECTATIONS.md`](../../src/onnx_clip/PERFORMANCE_EXPECTATIONS.md) — benchmark targets
- [`../../docs/en/onnx_clip/index.md`](../../docs/en/onnx_clip/index.md) — English secondary overview
- [`../../docs/de/onnx_clip/index.md`](../../docs/de/onnx_clip/index.md) — Deutsche Sekundärübersicht
