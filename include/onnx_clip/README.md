---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "api-contract"
Canonical_Sources:
  - "src/onnx_clip/onnx_clip_plugin.h"
  - "src/onnx_clip/onnx_clip_plugin.cpp"
  - "src/onnx_clip/CMakeLists.txt"
  - "tests/onnx_clip/CMakeLists.txt"
Related_Issue: "#7365"
---

Exact model backend not verifiable from final document text alone.

# onnx_clip module documentation

<!-- Links: ../../src/onnx_clip/README.md · ../../src/onnx_clip/ROADMAP.md · ../../src/onnx_clip/FUTURE_ENHANCEMENTS.md -->

This directory contains documentation only; it has no ONNX CLIP C++ header.
The plugin declaration is source-local at
[`src/onnx_clip/onnx_clip_plugin.h`](../../src/onnx_clip/onnx_clip_plugin.h), which
in-tree targets include as `onnx_clip/onnx_clip_plugin.h` when the repository
`src/` directory is on the include path. It declares
`themis::plugins::image::ONNXClipPlugin`, not a type in `themis::onnx_clip`.

## In-tree Entry Point

| Header / Entry Point | Purpose |
|---|---|
| `onnx_clip/onnx_clip_plugin.h` | `ONNXClipPlugin` implementation of `IImageAnalysisBackend` for deterministic image and text embeddings, batching, health/status, and optional model hash validation |
| `THEMIS_IMAGE_PLUGIN(themis::plugins::image::ONNXClipPlugin)` | Dynamic plugin export; disabled in test targets that compile the source directly |

## Source-local API Surface

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
| `enable_mmap_loading` | boolean | `false` | Top-level option to attempt a read-only mapping; mapped bytes are not used for inference |

## Runtime Behavior, Errors, and Limits

- `BackendType::AUTO` resolves to `CPU` in the current portable implementation.
- Backend enum values other than `AUTO` are stored as labels; the implementation
  does not probe or execute those providers.
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

- **A non-CPU backend was requested**: the enum is stored and reported, but does not enable provider inference in the current implementation.
- **`generateEmbeddingBatch()` is slower than expected for large batches**: the module currently sub-splits into sequential batches; see `FUTURE_ENHANCEMENTS.md` for the native-batch follow-up.
- **Integrity verification does not run in local builds**: set `model.path` and `model.expected_sha256`, then ensure OpenSSL is available or register `setModelHashFn()` in non-OpenSSL builds.

## See Also

- [`../../src/onnx_clip/README.md`](../../src/onnx_clip/README.md) — implementation overview
- [`../../src/onnx_clip/ARCHITECTURE.md`](../../src/onnx_clip/ARCHITECTURE.md) — architecture and data flow
- [`../../src/onnx_clip/ROADMAP.md`](../../src/onnx_clip/ROADMAP.md) — delivery status and phased roadmap
- [`../../src/onnx_clip/FUTURE_ENHANCEMENTS.md`](../../src/onnx_clip/FUTURE_ENHANCEMENTS.md) — open enhancement work
- [`../../src/onnx_clip/SECURITY.md`](../../src/onnx_clip/SECURITY.md) — module security notes
- [`../../src/onnx_clip/PERFORMANCE_EXPECTATIONS.md`](../../src/onnx_clip/PERFORMANCE_EXPECTATIONS.md) — performance evidence boundary; no validated module results
- [`../../docs/en/onnx_clip/index.md`](../../docs/en/onnx_clip/index.md) — English secondary overview
- [`../../docs/de/onnx_clip/index.md`](../../docs/de/onnx_clip/index.md) — Deutsche Sekundärübersicht
