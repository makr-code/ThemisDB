---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources: "include/llama_cpp/llama_cpp_plugin.h; include/llama_cpp/llama_cpp_registrar.h; src/llama_cpp/llama_cpp_plugin.cpp; src/llama_cpp/CMakeLists.txt"
Related_Issue: "#7356"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

> Exact model backend not verifiable from final document text alone.

> **Build (Linux):** `cmake --preset linux-release && cmake --build --preset linux-release`<br>
> **Build (Windows):** `cmake --preset windows-release && cmake --build --preset windows-release`

# llama_cpp Module Headers

<!-- Status: current | validated: 2026-10-31 | Primary: include/llama_cpp/ | Secondary: docs/de/llama_cpp/ -->
<!-- Links: ../../src/llama_cpp/README.md · ../../src/llama_cpp/ROADMAP.md · ../../src/llama_cpp/FUTURE_ENHANCEMENTS.md -->

Public headers for the standalone llama.cpp-backed plugin integration.

## Headers

| Header | Purpose |
|---|---|
| `llama_cpp_plugin.h` | `LlamaCppPlugin` implementation of `llm::ILLMPlugin` plus streaming/batch convenience APIs |
| `llama_cpp_registrar.h` | Factory and registration helpers for `LLMPluginManager` / plugin adapter integration |

## Public API Surface

- `LlamaCppPlugin::loadModel(model_path, config)` / `unloadModel()`
- `generate(...)`, `generateRAG(...)`, `generateStream(...)`, `generateBatch(...)`
- `embed(text)`
- LoRA lifecycle: `loadLoRA(...)`, `unloadLoRA(...)`, `listLoRAs()`, `exportLoRA(...)`, `importLoRA(...)`
- Operational metadata: `getCapabilities()`, `getMemoryStats()`, `getPerformanceStats()`, `getModelInfo()`
- Dynamic loading entry points in implementation: `themis_llm_create`, `themis_llm_destroy`

## Runtime Configuration Keys

The `config` JSON passed to `loadModel()` / registrar helpers accepts:

| Key | Type | Behavior |
|---|---|---|
| `model_path` | string | Non-empty path attempts `LlamaWrapper` initialization when `THEMIS_LLM_ENABLED` is compiled in |
| `context_length` | number | Context override; used for the plugin's local value first, but the wrapper prefers `n_ctx` when both are supplied |
| `n_ctx` | number | Preferred wrapper context setting; supplies the plugin's local value when `context_length` is absent |
| `n_gpu_layers` | number | Forwarded to `LlamaWrapper::Config` when LLM support is compiled in |
| `n_batch` | number | Forwarded batch size hint |
| `n_threads` | number | Forwarded CPU thread count hint |

## Runtime Behavior, Errors, and Limits

- Build/runtime gating: real llama.cpp execution requires `THEMIS_LLM_ENABLED` and a loadable model path.
- An empty model path does not load a model. Synthetic generation is available only in builds defining `THEMIS_LLAMA_CPP_STUB_MODE`; otherwise generation without a backend returns an error.
- `embed()` falls back to a 384-dim zero vector when no real backend (wrapper/injected function) is available.
- `generateBatch()` preserves request order but executes sequentially.
- `supports_function_call` is advertised as `true` in static capabilities; this is not proof that a real backend is loaded or ready.

## Usage

```cpp
#include "llama_cpp/llama_cpp_plugin.h"
#include <nlohmann/json.hpp>

themis::llamacpp::LlamaCppPlugin plugin;
nlohmann::json cfg = {
    {"n_ctx", 4096},
    {"n_threads", 8}
};
plugin.loadModel("/models/model.gguf", cfg);

themis::llm::InferenceRequest req;
req.prompt = "Summarize this text in one sentence.";
auto resp = plugin.generate(req);
```

```cpp
#include "llama_cpp/llama_cpp_registrar.h"
#include "llm/llm_plugin_manager.h"

auto& mgr = themis::llm::LLMPluginManager::instance();
themis::llamacpp::LlamaCppPluginRegistrar::registerWithLLMManager(
    mgr, "llama_cpp", {{"model_path", "/models/model.gguf"}});
```

## Installation

This module ships with ThemisDB. Consumers only need the public include directory:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Troubleshooting

- **`generate()` returns "Model not loaded"**: call `loadModel()` first and ensure `model_path` exists.
- **Stub-like output/empty embedding**: for generation, check `THEMIS_LLAMA_CPP_STUB_MODE` and real-wrapper availability; for embeddings, verify the wrapper or injected callback and reject the zero-vector fallback.
- **No hot-reload initialization**: pass a non-empty `model_path` in registrar config.
- **Unexpected token budget in RAG calls**: check `n_ctx` / `context_length` values in load config.

## See Also

- [`../../src/llama_cpp/README.md`](../../src/llama_cpp/README.md) — implementation overview
- [`../../src/llama_cpp/ARCHITECTURE.md`](../../src/llama_cpp/ARCHITECTURE.md) — component architecture
- [`../../src/llama_cpp/ROADMAP.md`](../../src/llama_cpp/ROADMAP.md) — implementation status and roadmap
- [`../../src/llama_cpp/FUTURE_ENHANCEMENTS.md`](../../src/llama_cpp/FUTURE_ENHANCEMENTS.md) — planned enhancements
- [`../../src/llama_cpp/SECURITY.md`](../../src/llama_cpp/SECURITY.md) — module security notes
- [`../../src/llama_cpp/PERFORMANCE_EXPECTATIONS.md`](../../src/llama_cpp/PERFORMANCE_EXPECTATIONS.md) — benchmark expectations
- [`../../docs/en/llama_cpp/index.md`](../../docs/en/llama_cpp/index.md) — secondary module overview (EN)
- [`../../docs/de/llama_cpp/index.md`](../../docs/de/llama_cpp/index.md) — Sekundärübersicht (DE)
