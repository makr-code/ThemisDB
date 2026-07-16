> **Build (Linux):** `cmake --preset linux-release && cmake --build --preset linux-release`<br>
> **Build (Windows):** `cmake --preset windows-release && cmake --build --preset windows-release`

<!-- Status: current | validated: 2026-04-07 | Primary: src/llama_cpp/ -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# llama_cpp LLM Backend Plugin

Standalone LLM backend plugin for ThemisDB implementing the `ILLMPlugin` interface
for dynamic loading via `THEMIS_LLM_PLUGIN()`.

## Module Purpose

Exposes the existing LlamaWrapper infrastructure as a dynamically loadable plugin.
`LlamaCppPlugin` supports text generation, RAG augmentation, embeddings, and LoRA
adapter lifecycle management. A stub mode is available for CI environments without
a model file.

## Subsystem Scope

**In scope:** Plugin lifecycle (`loadModel`/`unloadModel`), text generation (`generate`),
RAG-augmented generation (`generateRAG`), sentence embeddings (`embed`), LoRA adapter
management (`loadLoRA`/`unloadLoRA`/`listLoRAs`), capabilities reporting, performance stats.

**Out of scope:** Model weight training (handled by `training` module), KV-cache paging
(handled by `InferenceEngineEnhanced`), gRPC/REST transport (handled by `api` module).

## Relevant Interfaces

- `include/llm/llm_plugin_interface.h` — `ILLMPlugin`, `THEMIS_LLM_PLUGIN()`
- `include/llama_cpp/llama_cpp_plugin.h` — `LlamaCppPlugin`
- `include/llama_cpp/llama_cpp_registrar.h` — registration helpers and hot-reload callback type
- `src/llama_cpp/llama_cpp_plugin.cpp` — plugin implementation and dynamic loading entry points
- `src/llama_cpp/llama_cpp_registrar.cpp` — registrar factory/registration helpers

## Current Delivery Status

**Maturity:** 🟡 Beta (v2.0.0) — Core interface fully implemented in stub mode. Real
llama.cpp inference available via the existing `LlamaWrapper` when a model is present.

## Quick Start

```cpp
#include "llama_cpp/llama_cpp_plugin.h"

// Stub mode (no model required)
themis::llamacpp::LlamaCppPlugin plugin;
plugin.loadModel("", {});

themis::llm::InferenceRequest req;
req.prompt = "Was ist ThemisDB?";
auto resp = plugin.generate(req);
if (resp.success) std::cout << resp.text << "\n";

// With LoRA
plugin.loadLoRA("/adapters/egov.bin", "egov_adapter", 0.8f);
auto loras = plugin.listLoRAs(); // [{id="egov_adapter", scale=0.8}]

// Dynamic loading
auto* p = themis_llm_create();
// ... use p as ILLMPlugin* ...
themis_llm_destroy(p);
```

## Architecture Overview

```
┌────────────────────────────────────────┐
│         ILLMPlugin                     │  (include/llm/llm_plugin_interface.h)
└──────────────────┬─────────────────────┘
                   │ implements
       ┌───────────▼───────────────┐
       │     LlamaCppPlugin        │
       │  ┌───────────────────┐    │
       │  │  LoRA registry    │    │  vector<LoRAEntry>
       │  └───────────────────┘    │
       │  ┌───────────────────┐    │
       │  │  std::mutex       │    │  thread-safe load/unload
       │  └───────────────────┘    │
       └───────────────────────────┘
```

## Build

```cmake
cmake -B build && cmake --build build --target test_llama_cpp_plugin
```

## Test Suite

| Suite | Count | Labels |
|---|---|---|
| `LlamaCppPluginFocusedTests` | 30 | `plugins;llama_cpp;llm;v2.0.0` |

```bash
ctest -R LlamaCppPluginFocusedTests --output-on-failure
```

## Dependencies

| Dependency | Required | Purpose |
|---|---|---|
| `nlohmann_json` | ✅ | config / stats |
| `llama.cpp` | ❌ | linked via existing LlamaWrapper |

## Runtime Configuration Surfaces

`loadModel(model_path, config)` and registrar helpers consume JSON configuration:

| Key | Type | Behavior |
|---|---|---|
| `model_path` | string | Non-empty path triggers `LlamaWrapper` initialization when LLM backend is compiled in |
| `context_length` | number | Context window override (fallback key) |
| `n_ctx` | number | Preferred context window key forwarded to wrapper config |
| `n_gpu_layers` | number | GPU layer offload hint for llama.cpp |
| `n_batch` | number | Batch-size hint forwarded to wrapper |
| `n_threads` | number | CPU thread hint forwarded to wrapper |

## Runtime Behavior, Failure Modes, and Limits

- `loadModel()` always marks the plugin as loaded; with empty or invalid model path it falls back to stub-capable operation.
- Real inference path is available only when `THEMIS_LLM_ENABLED` is compiled and `LlamaWrapper` loads successfully.
- `generate()` fails closed with `success=false` / `"Model not loaded — call loadModel() before generate()"` when no backend is available (except test builds with `THEMIS_LLAMA_CPP_STUB_MODE`).
- `embed()` returns a 384-dim zero-vector fallback if no backend is available and no injected embedding function is configured.
- `generateBatch()` is currently sequential (ordered, but not parallelized).
- `generateRAG()` assembles context with `RAGContextAssembler` and clamps `max_tokens` to available context budget.

## Dynamic Loading Entry Points

| Symbol | Signature |
|---|---|
| `themis_llm_create` | `ILLMPlugin* ()` |
| `themis_llm_destroy` | `void (ILLMPlugin*)` |

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.

## Usage

The implementation files in this module are compiled into the ThemisDB library.
See [`../../include/llama_cpp/README.md`](../../include/llama_cpp/README.md) for the public API.

## Troubleshooting

- **`generate()` returns "model not loaded"**: call `loadModel()` and provide a valid non-empty model path.
- **Output stays in fallback mode**: verify the build enables `THEMIS_LLM_ENABLED` and that model loading succeeds.
- **Embedding quality is always zero/flat**: ensure real backend embedding path is active (wrapper or injected `EmbedFn`).
- **Registrar hot-reload does not activate model**: pass `config["model_path"]` when using `LlamaCppPluginRegistrar`.
- **Short/trimmed RAG responses**: tune `n_ctx` / `context_length`, `request.max_tokens`, and `rag_context.response_budget_tokens`.

## See Also

- [`ARCHITECTURE.md`](ARCHITECTURE.md)
- [`ROADMAP.md`](ROADMAP.md)
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- [`SECURITY.md`](SECURITY.md)
- [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
- [`../../docs/en/llama_cpp/index.md`](../../docs/en/llama_cpp/index.md)
- [`../../docs/de/llama_cpp/index.md`](../../docs/de/llama_cpp/index.md)
