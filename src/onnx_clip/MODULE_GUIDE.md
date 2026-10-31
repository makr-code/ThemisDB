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

# ONNX CLIP Developer Guide

Exact model backend not verifiable from final document text alone.

## Start here

`onnx_clip` is a plugin implementation of `IImageAnalysisBackend`. Its current
source provides deterministic reference embeddings and lifecycle behavior; it
does not run pretrained CLIP inference. Read
[`README.md`](./README.md) for the module overview,
[`API.md`](./API.md) for call/config contracts, and
[`IMPLEMENTATION.md`](./IMPLEMENTATION.md) before changing lifecycle or
concurrency behavior.

## Integrate the interface

Include `onnx_clip/onnx_clip_plugin.h` and expose both repository `include/`
and `src/` paths to an in-tree target. Construct `PluginConfig` from JSON,
initialize the plugin, check the bool result and `isReady()`, then inspect
`EmbeddingResult.success` on each call. Call `shutdown()` when the plugin is
being stopped; destruction also releases implementation-owned resources.

```cpp
#include "onnx_clip/onnx_clip_plugin.h"

using namespace themis::plugins::image;

ONNXClipPlugin plugin;
PluginConfig config;
if (!plugin.initialize(config, BackendType::AUTO)) {
    // Do not send embedding requests.
}
auto result = plugin.generateEmbedding(image_bytes);
if (result.success) {
    // Store/use result.embedding only under the caller's data policy.
}
```

The example's `image_bytes` must be a `std::vector<uint8_t>`. The current
implementation hashes these bytes; it does not parse image formats.

## Configure safely

- Set `model.embedding_dim` to match the consuming index schema; non-positive
  values silently become `512`.
- Set `max_batch_size` with resource constraints in mind. It controls sequential
  chunking, but has no upper clamp and does not limit total input size.
- Set both `model.path` and `model.expected_sha256` to request digest checking.
  Confirm OpenSSL or an injected `ModelHashFn` is available; without either,
  non-OpenSSL builds skip this check.
- Do not mistake `model.name` or a stored backend enum for loaded model/provider
  evidence.

## Build and test

The plugin target is conditional on `THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX` and the
`onnxruntime` CMake package. The focused test harness can compile the source
directly when the shared target is absent; see
[`tests/onnx_clip/README.md`](../../tests/onnx_clip/README.md) for the exact
targets and filters available in the checkout.

For the source-level contract, run the focused ONNX CLIP tests configured by
the repository build. For this documentation change, the issue's required
check is:

```bash
python -m unittest discover -s tests -p "test_doc_metadata_gate.py"
```

Generated test data verifies determinism and interface behavior, not semantic
CLIP quality. Model-backed tests require explicit model assets and should be
reported separately.

## Change checklist

When modifying behavior, update the API and implementation docs with any
changed inputs, outputs, defaults, errors, or concurrency semantics. If adding
real inference, update the research/model evaluation, trust boundary, runtime
requirements, and operator procedures in the same change. Do not claim
pretrained model behavior based solely on a successful CMake link or a
deterministic-vector test.

## Source map

- [`onnx_clip_plugin.h`](./onnx_clip_plugin.h) — class declaration
- [`onnx_clip_plugin.cpp`](./onnx_clip_plugin.cpp) — current behavior
- [`CMakeLists.txt`](./CMakeLists.txt) — plugin build conditions
- [`tests/onnx_clip/`](../../tests/onnx_clip/) — focused tests
- [`ARCHITECTURE.md`](./ARCHITECTURE.md) — module boundaries and flow
