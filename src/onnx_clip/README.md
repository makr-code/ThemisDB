---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`
>
> **Status:** current | source-checked: 2026-10-31

<!-- Status: current | source-checked: 2026-10-31 -->
<!-- Links: ../../include/onnx_clip/README.md · ARCHITECTURE.md · ROADMAP.md · PRODUCTION_REQUIREMENTS.md · SECURITY.md · RESEARCH.md · MODELING.md · EMBEDDING.md · IMPLEMENTATION.md · API.md · MODULE_GUIDE.md · RUNBOOK.md · OPERATIONS.md · TRUST.md -->

# ThemisDB ONNX CLIP Plugin

**Version:** 0.3.0
**Status:** contract-validated reference implementation
**Last Updated:** 2026-10-31
**Module Path:** `src/onnx_clip/`
**Namespace:** `themis::plugins::image`

---

## Module Purpose

The `onnx_clip` module exposes an `IImageAnalysisBackend` implementation with
deterministic reference vectors for image bytes and text. The current source does
not decode images, execute an ONNX graph, load CLIP weights, or establish semantic
alignment between image and text vectors. It exercises configuration, batching,
statistics, health checks, optional digest verification, and lifecycle contracts;
the model label and backend selection do not prove that a model/provider is active.

## Subsystem Scope

**In scope:** deterministic vectors derived from raw image bytes or text,
per-plugin statistics, sequential batch chunking, optional SHA-256 verification,
dynamic plugin export, and focused contract tests.

**Out of scope:** pretrained CLIP inference, image decoding/preprocessing, a
learned shared image-text embedding space, runtime provider probing, and
model-backed quality evaluation using assets not shipped with the repository.

## Current Delivery Status

**Current state:** the source implements the deterministic reference behavior in
`onnx_clip_plugin.cpp`. It does not instantiate an ONNX Runtime session or consume
the configured model bytes for embedding generation.

**Practical interpretation:** the interface and lifecycle are testable, but
returned vectors are not semantic CLIP embeddings and must not be used to claim
retrieval quality.

## Governance and Archive Boundary

The active governance set for this module is limited to:

- `README.md`
- `ARCHITECTURE.md`
- `ROADMAP.md`
- `PRODUCTION_REQUIREMENTS.md`
- `SECURITY.md`
- `TRUST.md`
- `RESEARCH.md`
- `MODELING.md`
- `EMBEDDING.md`
- `IMPLEMENTATION.md`
- `API.md`
- `MODULE_GUIDE.md`
- `RUNBOOK.md`
- `OPERATIONS.md`
- `FUTURE_ENHANCEMENTS.md`
- `PERFORMANCE_EXPECTATIONS.md`
- `CHANGELOG.md`
- `AUDIT.md`

Historical phase deliverables and generated meta-docs are retained only under
`src/onnx_clip/archive/` and are intentionally excluded from the live production
contract unless a specific release or audit task requires them.

## Components

| File | Role |
|---|---|
| `onnx_clip_plugin.h` | Public class declaration for `ONNXClipPlugin`, including lifecycle, embedding APIs, stats APIs, and `setModelHashFn()` |
| `onnx_clip_plugin.cpp` | Deterministic embedding implementation, text tokenization, stats counters, batch splitting, config validation, and optional integrity verification |
| `CMakeLists.txt` | Module build gating and optional integration points for ONNX Runtime/OpenCV discovery |

## Public API & Entry Points

- Public header overview: [`../../include/onnx_clip/README.md`](../../include/onnx_clip/README.md)
- Source-local entry point: [`onnx_clip_plugin.h`](./onnx_clip_plugin.h)
- Dynamic plugin export: `THEMIS_IMAGE_PLUGIN(themis::plugins::image::ONNXClipPlugin)`
- Key methods:
  - `initialize(config, backend)`
  - `generateEmbedding(image_data, metadata)`
  - `generateEmbeddingBatch(images)`
  - `generateTextEmbedding(text)`
  - `reloadModel(config)`
  - `healthCheck()`, `warmup()`, `getStatistics()`
  - `setModelHashFn(fn)` for non-OpenSSL integrity-check injection

## Configuration Options

### `PluginConfig` keys read by `initialize()`

| Key | Type | Default | Runtime effect |
|---|---|---|---|
| `model.name` | string | `clip-vit-base-patch32` | Label propagated to results/statistics |
| `model.embedding_dim` | integer | `512` | Embedding dimension; invalid or non-positive values are corrected back to `512` |
| `max_batch_size` | integer | `16` on CPU, `64` otherwise | Maximum sub-batch size processed per `generateEmbeddingBatch()` chunk |
| `model.path` | string | empty | Optional model file used for integrity verification |
| `model.expected_sha256` | string | empty | Enables hash verification when paired with `model.path` |
| `enable_mmap_loading` | boolean | false | Top-level option to map a non-empty model path; the mapped bytes are not used by inference |

### Current runtime contract

- `BackendType::AUTO` resolves to `CPU` in the current portable implementation.
- `generateEmbeddingBatch()` preserves request order while processing sub-batches up to `max_batch_size`.
- `reloadModel()` publishes a validated replacement before draining requests using the old snapshot. A `false` return caused by the 30-second drain timeout does not roll back the already-published replacement.
- The optional model-hash validation runs only when `model.path` and `model.expected_sha256` are both present.
- In builds without OpenSSL, an installed `ModelHashFn` performs that check; if no callback is installed, verification is skipped.
- Batch chunking bounds each processing chunk, not the total number of inputs or result allocation.

## Runtime Behavior, Error Cases, and Limits

- Empty image payloads return `success=false` with `"Image data is empty"`.
- Empty text payloads return `success=false` with `"Text input is empty"`.
- Calling embedding methods before `initialize()` returns `success=false` with `"ONNXClipPlugin not initialized"`.
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
plugin.initialize(cfg, BackendType::AUTO);

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

This module is built as part of ThemisDB. For in-tree targets, ensure the
repository `include/` and `src/` directories are available on the include path.

```cmake
target_include_directories(your_target PRIVATE
    ${THEMISDB_INCLUDE_DIR}
    ${THEMISDB_SOURCE_DIR}/src
)
```

## Troubleshooting

- **`AUTO` resolves to CPU**: expected in the current portable implementation; pass an explicit backend enum if the surrounding build/runtime supports a native provider.
- **Large batches behave sequentially**: the implementation sub-splits the batch by `max_batch_size`; it does not yet use a single native batched ONNX call.
- **`initialize()` fails when a hash is configured**: verify both `model.path` and `model.expected_sha256`; in non-OpenSSL builds, register `setModelHashFn()` when verification must run.
- **Embedding calls fail immediately**: call `initialize()` first and check `isReady()` / `healthCheck()`.

## Main Source References

- [`RESEARCH.md`](./RESEARCH.md) — evaluation assumptions and required model-backed evidence
- [`MODELING.md`](./MODELING.md) — deterministic vector-generation method
- [`EMBEDDING.md`](./EMBEDDING.md) — inputs, outputs, and embedding semantics
- [`IMPLEMENTATION.md`](./IMPLEMENTATION.md) — state, reload, integrity, and build details
- [`API.md`](./API.md) — callable interface and configuration contract
- [`MODULE_GUIDE.md`](./MODULE_GUIDE.md) — integration and change workflow
- [`RUNBOOK.md`](./RUNBOOK.md) — symptom diagnosis and recovery
- [`OPERATIONS.md`](./OPERATIONS.md) — routine checks and metric interpretation
- [`TRUST.md`](./TRUST.md) — trust boundaries and security assumptions
- [`ARCHITECTURE.md`](./ARCHITECTURE.md) — component layout and runtime contract
- [`ROADMAP.md`](./ROADMAP.md) — delivery phases and work items
- [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md) — follow-up implementation work
- [`SECURITY.md`](./SECURITY.md) — threat model and controls
- [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) — benchmark targets
- [`AUDIT.md`](./AUDIT.md) — source inventory and focused verification
- [`../../docs/en/onnx_clip/index.md`](../../docs/en/onnx_clip/index.md) — English secondary overview
- [`../../docs/de/onnx_clip/index.md`](../../docs/de/onnx_clip/index.md) — German secondary overview
