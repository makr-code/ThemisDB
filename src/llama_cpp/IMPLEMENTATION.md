---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources: "src/llama_cpp/llama_cpp_plugin.cpp; src/llama_cpp/llama_cpp_registrar.cpp; src/llama_cpp/llama_cpp_plugin_validation_gates.cpp; include/llama_cpp/*.h; tests/llama_cpp"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

# llama_cpp Implementation Notes

For developers changing or integrating the `src/llama_cpp` plugin. The implementation is the authority for runtime behavior; this note highlights concrete flows, contracts and gaps.

> Exact model backend not verifiable from final document text alone.

## Components and build modes

- [`llama_cpp_plugin.cpp`](llama_cpp_plugin.cpp) implements lifecycle, generation, RAG prompt assembly, embedding, LoRA metadata/import-export, stats and dynamic entry points.
- [`llama_cpp_registrar.cpp`](llama_cpp_registrar.cpp) creates a plugin/adapter and registers it with `LLMPluginManager`.
- [`llama_cpp_plugin_validation_gates.cpp`](llama_cpp_plugin_validation_gates.cpp) contains separate validation helpers and C-linkage validator exports.
- Public contracts: [`../../include/llama_cpp/llama_cpp_plugin.h`](../../include/llama_cpp/llama_cpp_plugin.h), [`../../include/llama_cpp/llama_cpp_registrar.h`](../../include/llama_cpp/llama_cpp_registrar.h), [`../../include/llm/llm_plugin_interface.h`](../../include/llm/llm_plugin_interface.h), and [`../../include/llm/llama_wrapper.h`](../../include/llm/llama_wrapper.h).

The module CMake target is static C++17. The optional wrapper path is enabled by the parent build's `THEMIS_ENABLE_LLM`, which defines `THEMIS_LLM_ENABLED`; see [`CMakeLists.txt`](CMakeLists.txt). Focused tests define `THEMIS_LLAMA_CPP_STUB_MODE`.

## State and inference invariants

- `loadModel()` updates path/id/context under `mutex_`. Context comes from numeric `context_length`, else numeric `n_ctx`, else the default. The wrapper receives selected config keys; response cache and continuous batching are disabled by this plugin.
- A wrapper load exception or false load result leaves `wrapper_` empty, but the function later sets `model_loaded_ = true` and returns true unless the optional digest comparison explicitly fails. Therefore `isModelLoaded()` alone does not prove a usable backend.
- `unloadModel()` clears loaded state, identifiers, path, LoRA registry and wrapper. `getModelInfo()` returns no value only when `model_loaded_` is false; fallback metadata can therefore exist without a live wrapper.
- `generate()` checks the optional `PolicyFn` and cancellation before dispatch. It calls a live wrapper, then injected generator, then test-only echo/tool behavior where enabled; otherwise it returns a failure response. Inference runs outside the plugin mutex after state snapshot.
- `generateBatch()` calls `generate()` serially and preserves request order; it is not concurrent batching.
- `generateRAG()` snapshots state/context, assembles and budgets chunks, then dispatches via `generate()`. The request policy callback is evaluated inside that final dispatch.

## Input validation and non-invariants

The validator source defines a 128–131072 context range, checks non-empty model paths for existence, a 100 MiB minimum size and GGUF magic, and applies a coarse 1.5× model-size GPU estimate with an 8 GiB limit for requested GPU layers. CUDA availability is cached and its compiled-in branch is a placeholder rather than actual device enumeration. The exported validator's use must be confirmed by its caller; `loadModel()` does not call it in this implementation.

`validateTokenLimits()` warns on some sampling/token inputs but is not called from `generate()` here. Do not describe those checks as guaranteed runtime sanitization.

`loadLoRA()` only replaces/adds an in-memory `{id,path,scale}` record; it does not load adapter weights. `importLoRA()` checks minimum/maximum byte size and GGUF magic before delegating to a live wrapper, but with no wrapper returns true after the byte checks. See [`TRUST.md`](TRUST.md) before treating these methods as an adapter authenticity or activation control.

## Test and implementation workflow

The canonical focused targets are collected in [`../../tests/llama_cpp/CMakeLists.txt`](../../tests/llama_cpp/CMakeLists.txt); focused source tests are in [`tests/`](tests/). Useful coverage includes lifecycle, registrar, inference contract and validation smoke tests, plus [`../../tests/llama_cpp/test_model_loading.cpp`](../../tests/llama_cpp/test_model_loading.cpp) and [`../../tests/llama_cpp/test_inference_execution.cpp`](../../tests/llama_cpp/test_inference_execution.cpp). Tests using stub paths do not validate actual GGUF loading or model output.

Run the repository's configured focused CTest selection, for example `ctest --preset community-debug -R 'llama_cpp' --output-on-failure` when that preset/build exists. For real-backend changes, separately configure an LLM-enabled build and exercise a known model artifact; record the exact artifact and build flags.

## Related module documents

[`RESEARCH.md`](RESEARCH.md) · [`MODELING.md`](MODELING.md) · [`EMBEDDING.md`](EMBEDDING.md) · [`API.md`](API.md) · [`MODULE_GUIDE.md`](MODULE_GUIDE.md) · [`RUNBOOK.md`](RUNBOOK.md) · [`OPERATIONS.md`](OPERATIONS.md) · [`TRUST.md`](TRUST.md)

Also see [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`ROADMAP.md`](ROADMAP.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), [`SECURITY.md`](SECURITY.md), and [`../../include/llama_cpp/README.md`](../../include/llama_cpp/README.md).
