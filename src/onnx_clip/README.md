> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: ../../include/onnx_clip/README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# ThemisDB ONNX CLIP Plugin

**Version:** 0.3.0 (v0.2.0 hardening release)
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-08-09
**Module Path:** `src/onnx_clip/`
**Namespace:** `themis::plugins::image`

---

## Module Purpose

The `onnx_clip` module provides a deterministic `IImageAnalysisBackend`
implementation for CLIP-style image and text embeddings. In the current portable
build, the module simulates ONNX-backed inference behavior while preserving the
runtime contract that the rest of ThemisDB relies on: initialization, backend
selection, image/text embedding generation, bounded batch handling, statistics,
health checks, and optional model-integrity verification.

## Subsystem Scope

**In scope:** image embeddings from raw bytes, text embeddings for cross-modal
search, per-plugin statistics, bounded sub-batch processing, optional SHA-256
model verification, dynamic plugin export, and focused unit coverage.

**Out of scope:** real model loading guarantees in every build profile, a
single native batched ONNX session call, automatic GPU probing in `AUTO` mode,
and golden-vector integration tests with real ONNX model assets.

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — the module implements the full
`IImageAnalysisBackend` surface exercised by focused tests, including image and
text embeddings, stats, health checks, batching, and integrity-check control
paths. Open follow-up work remains in benchmarking and real-model integration.

## Components

| File | Role |
|---|---|
| `onnx_clip_plugin.h` | Public class declaration for `ONNXClipPlugin`, including lifecycle, embedding APIs, stats APIs, and `setModelHashFn()` |
| `onnx_clip_plugin.cpp` | Deterministic embedding implementation, text tokenization, stats counters, batch splitting, backend selection, and optional SHA-256 verification |
| `CMakeLists.txt` | Module build gating (`THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX`), ONNX Runtime lookup, OpenCV fallback handling, shared-library target wiring |

## Public API & Entry Points

- Public header overview: [`../../include/onnx_clip/README.md`](../../include/onnx_clip/README.md)
- Source-local entry point: [`onnx_clip_plugin.h`](./onnx_clip_plugin.h)
- Dynamic plugin export: `THEMIS_IMAGE_PLUGIN(themis::plugins::image::ONNXClipPlugin)`
- Key methods:
  - `initialize(config, backend)`
  - `generateEmbedding(image_data, metadata)`
  - `generateEmbeddingBatch(images)`
  - `generateTextEmbedding(text)`
  - `reloadModel(config)` **(v0.3.0)** — Dynamic model reloading
  - `healthCheck()`, `warmup()`, `getStatistics()`
  - `setModelHashFn(fn)` for non-OpenSSL integrity-check injection

## Configuration Options

### `PluginConfig` keys read by `initialize()`

| Key | Type | Default | Runtime effect |
|---|---|---|---|
| `model.name` | string | `clip-vit-base-patch32` | Label propagated to results/statistics |
| `model.embedding_dim` | integer | `512` | Embedding size; non-positive values are corrected back to `512` |
| `max_batch_size` | integer | `16` on CPU, `64` otherwise | Maximum sub-batch size processed per `generateEmbeddingBatch()` chunk |
| `model.path` | string | empty | Optional model file path used for integrity checks |
| `model.expected_sha256` | string | empty | Enables hash verification when paired with `model.path`; mismatch causes `initialize()` to fail |
| `enable_mmap_loading` | boolean | false | **(v0.3.0)** Enable memory-mapped model loading (Linux/Windows); reduces peak memory for large models |

### v0.3.0 New APIs

| Method | Signature | Effect |
|--------|-----------|--------|
| `reloadModel()` | `bool reloadModel(const PluginConfig& new_config)` | **(v0.3.0)** Dynamically reload model without server restart; no in-flight request interruption; automatic rollback on failure |

**Memory-mapped loading:** When `enable_mmap_loading=true`, model file is memory-mapped (Linux mmap + Windows MapViewOfFile) to reduce peak memory usage. Gracefully falls back to traditional loading on unsupported platforms.

**Hot-swap (dynamic reload):** Call `reloadModel(new_config)` to switch models at runtime. In-flight requests complete normally, drain completes within 30 seconds, old model retained until new one validates.

### Build/runtime gates

- `THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX` must be enabled or the module is skipped at CMake time.
- `onnxruntime` must be discoverable by CMake or the plugin target is not built.
- `OpenCV` is optional; if absent, the module keeps a fallback path and logs a status message during configuration.
- `THEMIS_HAS_OPENSSL` enables built-in SHA-256 verification. Without it, `setModelHashFn()` is the non-OpenSSL verification hook.

## Runtime Behavior, Error Cases, and Limits

- `BackendType::AUTO` currently resolves to `CPU` for deterministic, portable behavior.
- `generateEmbeddingBatch()` preserves request order and processes items in sequential sub-batches capped by `max_batch_size`.
- Empty image payloads return `success=false` with `"Image data is empty"`.
- Empty text payloads return `success=false` with `"Text input is empty"`.
- Calling image/text embedding methods before `initialize()` returns `success=false` with `"ONNXClipPlugin not initialized"`.
- `healthCheck()` reports healthy only when the plugin is initialized and the embedding dimension is positive.
- `getStatistics()` returns readiness, backend, model name, `max_batch_size`, totals, latency, and Prometheus-style counters:
  - `clip_embeddings_total`
  - `clip_text_embeddings_total`
  - `clip_batch_embeddings_total`

## Usage Snippets

### Minimal initialization and image embedding

```cpp
#include "onnx_clip/onnx_clip_plugin.h"
#include <nlohmann/json.hpp>

using namespace themis::plugins::image;

ONNXClipPlugin plugin;
PluginConfig cfg;
plugin.initialize(cfg, BackendType::AUTO);  // resolves to CPU in current build

auto result = plugin.generateEmbedding(std::vector<uint8_t>{1, 2, 3, 4});
if (result.success) {
    // result.embedding contains a normalized float vector
}
```

### Explicit dimension, batching, and text embedding

```cpp
nlohmann::json settings = {
    {"model", {
        {"name", "clip-vit-large-patch14"},
        {"embedding_dim", 768}
    }},
    {"max_batch_size", 3}
};

PluginConfig config(settings);
ONNXClipPlugin plugin;
plugin.initialize(config, BackendType::CPU);

auto batch = plugin.generateEmbeddingBatch({
    std::vector<uint8_t>{1, 2, 3},
    std::vector<uint8_t>{4, 5, 6},
    std::vector<uint8_t>{7, 8, 9},
    std::vector<uint8_t>{10, 11, 12}
});

auto text = plugin.generateTextEmbedding("a photo of a dog");
auto stats = plugin.getStatistics();
```

### Optional integrity verification

```cpp
nlohmann::json settings = {
    {"model", {
        {"path", "/models/clip.onnx"},
        {"expected_sha256", "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"}
    }}
};

PluginConfig config(settings);
ONNXClipPlugin plugin;
bool ok = plugin.initialize(config, BackendType::CPU);
```

## Installation

This module is built as part of ThemisDB. When consuming it in-tree, the target
include path must expose both the repository `include/` directory and the
repository `src/` directory because the current header lives in `src/onnx_clip/`.

```cmake
target_include_directories(your_target PRIVATE
    ${THEMISDB_INCLUDE_DIR}
    ${THEMISDB_SOURCE_DIR}/src
)
```

## Troubleshooting

- **`AUTO` never picks a GPU backend**: expected in the current generic implementation; pass an explicit backend enum if your surrounding build/runtime supports it.
- **Large batches still behave sequentially**: current behavior uses sub-batch splitting, not a single native ONNX batch call.
- **`initialize()` fails when a hash is configured**: verify both `model.path` and `model.expected_sha256`; in non-OpenSSL builds, ensure a `setModelHashFn()` callback is registered if verification must run.
- **Embedding calls fail immediately**: call `initialize()` first and check `isReady()` / `healthCheck()`.

## Main Source References

- [`ARCHITECTURE.md`](./ARCHITECTURE.md) — component layout and inference flow
- [`ROADMAP.md`](./ROADMAP.md) — delivery phases and open work
- [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) — implementable follow-up items
- [`SECURITY.md`](./SECURITY.md) — threat model and controls
- [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) — benchmark targets
- [`AUDIT.md`](./AUDIT.md) — source inventory and focused test coverage
- [`../../docs/en/onnx_clip/index.md`](../../docs/en/onnx_clip/index.md) — English secondary overview
- [`../../docs/de/onnx_clip/index.md`](../../docs/de/onnx_clip/index.md) — Deutsche Sekundärübersicht
