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
