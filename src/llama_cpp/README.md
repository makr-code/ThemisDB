> **Build (Linux):** `cmake --preset linux-release && cmake --build --preset linux-release`<br>
> **Build (Windows):** `cmake --preset windows-release && cmake --build --preset windows-release`

<!-- Status: current | validated: 2026-09-28 | Primary: src/llama_cpp/ -->
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

**Maturity:** ✅ Production-hardened module — source-verified against the live
`LlamaCppPlugin` and registrar implementations. The module supports the full
`ILLMPlugin` surface with a real `LlamaWrapper` path behind `THEMIS_LLM_ENABLED`,
a fail-closed no-model path in production builds, and a test-only echo stub gated by
`THEMIS_LLAMA_CPP_STUB_MODE`.

## Quick Start

```cpp
#include "llama_cpp/llama_cpp_plugin.h"

// Production: load a concrete model path and config before generating.
themis::llamacpp::LlamaCppPlugin plugin;
const nlohmann::json cfg = {
    {"model_path", "/models/demo.gguf"},
    {"n_ctx", 4096},
    {"n_gpu_layers", 20}
};
plugin.loadModel("/models/demo.gguf", cfg);

themis::llm::InferenceRequest req;
req.prompt = "Was ist ThemisDB?";
auto resp = plugin.generate(req);
if (resp.success) std::cout << resp.text << "\n";

// Optional policy gate: return true to allow inference and set a reason
// only when the request is denied.
plugin.setPolicyFn([](const themis::llm::InferenceRequest&, std::string& reason) {
    reason.clear();
    return true;
});

// Real or stub LoRA flow: production callers must load a valid GGUF adapter
// payload from disk before invoking importLoRA(). The next two lines are a
// placeholder showing the required call pattern; do not invoke the API with an
// empty byte vector in production.
std::vector<uint8_t> binary_gguf_bytes = {};
// binary_gguf_bytes = read_file_bytes("/adapters/example.gguf");
// plugin.importLoRA("egov_adapter", binary_gguf_bytes);
auto loras = plugin.listLoRAs();

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
| `llama_cpp` focused suite | repo-specific | `plugins;llama_cpp;llm;v2.4.0` |

```bash
ctest --preset community-debug -R 'llama_cpp' --output-on-failure
python scripts/check_module_direct_doxygen.py --module llama_cpp
```

## Dependencies

| Dependency | Required | Purpose |
|---|---|---|
| `nlohmann_json` | ✅ | config, stats, and policy metadata |
| `llama.cpp` | ✅ when `THEMIS_LLM_ENABLED` is on | real inference backend via `LlamaWrapper` |
| `spdlog` | ✅ | operational logging for RAG and inference paths |

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

- `loadModel()` is the explicit entry point; it does not silently load a model behind the caller and it updates `context_length_` from `context_length` or `n_ctx` when present.
- Real inference requires `THEMIS_LLM_ENABLED` and a model that successfully loads through `LlamaWrapper`; non-empty paths are the production path.
- `generate()` and `generateRAG()` fail closed by returning `success=false` when the backend is unavailable or a policy gate denies the request.
- `THEMIS_LLAMA_CPP_STUB_MODE` is a test-only compatibility mode; it keeps the echo/stub contract for unit tests but must not be treated as the default production behavior.
- `embed()` returns an empty vector when no real backend is configured and no injected `EmbedFn` is available; zero-vector fallback remains a non-production fallback path for tests or limited CI use.
- `generateBatch()` is sequential and preserves request order; it is not a parallel batch backend.
- `generateRAG()` assembles context with `RAGContextAssembler`, applies the response budget, and dispatches through `generate()` so the same policy and fail-closed rules apply.
- `importLoRA()` validates GGUF magic bytes and a 2 GB size cap before accepting adapter bytes.

## Dynamic Loading Entry Points

| Symbol | Signature |
|---|---|
| `themis_llm_create` | `ILLMPlugin* ()` |
| `themis_llm_destroy` | `void (ILLMPlugin*)` |

The registrar helper `LlamaCppPluginRegistrar::initFromServerConfig()` is the startup integration point and registers the plugin under the `llama_cpp` name when a non-empty `llm.model_path` is configured.

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
