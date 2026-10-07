> **Status:** 2026-10-07 — source-verified against `src/llama_cpp/llama_cpp_plugin.cpp`, `llama_cpp_registrar.cpp`, `include/llama_cpp/llama_cpp_plugin.h`, and the module test targets.

# ThemisDB llama_cpp Plugin — Production Requirements

## Scope and source of truth

This document is the production gate for the `llama_cpp` module. It defines the mandatory runtime behavior and deployment constraints for the dynamic LLM plugin implementation. The canonical implementation remains in:

- `src/llama_cpp/llama_cpp_plugin.cpp`
- `src/llama_cpp/llama_cpp_registrar.cpp`
- `include/llama_cpp/llama_cpp_plugin.h`
- `include/llama_cpp/llama_cpp_registrar.h`
- `src/llama_cpp/tests/*.cpp`

Related governance docs:

- `src/llama_cpp/README.md`
- `src/llama_cpp/ARCHITECTURE.md`
- `src/llama_cpp/ROADMAP.md`
- `src/llama_cpp/SECURITY.md`
- `src/llama_cpp/FUTURE_ENHANCEMENTS.md`

## Mandatory production invariants

- MUST: `loadModel()` is the explicit model-entry point; `generate()`, `embed()`, and `generateRAG()` must not silently load a model behind the caller's back.
- MUST: public state is protected by `std::mutex mutex_` for lifecycle, LoRA registry, and readiness checks.
- MUST: the production path fails closed when no model is available: `generate()` returns `success=false` with a structured error and never falls through to a silent success state.
- MUST: stub behavior remains test-only and controlled by build-time gating (`THEMIS_LLAMA_CPP_STUB_MODE`); it must not be the default production path.
- MUST: model configuration is provided through JSON config and runtime path input; no hard-coded model path is accepted in the production code path.
- MUST: `themis_llm_create()` and `themis_llm_destroy()` expose a valid plugin handle only when the plugin is in a consistent state.
- MUST NOT: disable safety checks, policy enforcement, or explicit error propagation in production builds.

## Security and reliability requirements

- All model-load and LoRA-import workflows must be guarded by the module mutex and validate external inputs before side effects.
- `importLoRA()` must fail closed on malformed or oversized payloads; the current implementation validates GGUF magic bytes and a 2 GB size bound before accepting adapter data.
- `generate()` and `generateRAG()` must evaluate the optional policy gate before dispatching inference. A denial must return `success=false` with the denial reason in the response object.
- `getMemoryStats()` and `getPerformanceStats()` must not expose credentials, prompts, or user-specific content; they are operational metrics only.
- `computeFileDigest()` is an opt-in integrity gate controlled by `verify_model_digest` and `expected_model_digest`; if enabled, mismatches fail closed.

## Operational limits and deployment constraints

- `n_ctx`, `n_gpu_layers`, `n_batch`, and `n_threads` are configuration-driven and must be set per deployment; default values are not a substitute for deployment hardening.
- `generateBatch()` is sequential and preserves request order; it is not a parallelized batch backend.
- `embed()` falls back to a 384-dimensional zero-vector result only when no real backend or injected embedding function is configured; production deployments should prefer a real `LlamaWrapper` or an explicit `EmbedFn` injection.
- Rate limiting is intentionally left to the consuming API layer; the module does not implement an in-process rate limiter.
- External model and adapter loading must honor explicit timeouts and retry requirements managed by the higher-level deployment stack.

## Production readiness checklist

- [x] `LlamaCppPlugin` implements the `ILLMPlugin` contract for generation, embeddings, RAG, LoRA lifecycle, and operational stats.
- [x] `loadModel()` and registrar helpers accept runtime JSON config and respect `model_path`, `n_ctx`, and related keys.
- [x] Production error handling is fail-closed when no model is loaded or inference is denied.
- [x] `THEMIS_LLM_ENABLED` and `THEMIS_LLAMA_CPP_STUB_MODE` are treated as distinct build/runtime modes.
- [x] `importLoRA()` validates GGUF identity and size bounds before accepting adapter bytes.
- [x] `setPolicyFn()` enables policy-driven denial gating for generation and RAG requests.
- [x] `generateStream()` and `generateBatch()` preserve deterministic ordering and callback safety constraints.
- [x] `LlamaCppPluginRegistrar::initFromServerConfig()` provides a server-startup integration point.
- [x] The module exposes focused validation for plugin lifecycle, registrar integration, and stress/security scenarios.

## Required validation evidence

When validating the module, prefer the focused llama_cpp unit and registrar checks in the repo:

- `ctest --preset community-debug -R 'llama_cpp' --output-on-failure`
- `python scripts/check_module_direct_doxygen.py --module llama_cpp`

The live implementation must remain in sync with the module docs above and with the `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, and `FUTURE_ENHANCEMENTS.md` source docs.

## Source review checklist

The following source files are the relevant review scope for operational readiness:

- `src/llama_cpp/llama_cpp_plugin.cpp`
- `src/llama_cpp/llama_cpp_registrar.cpp`
- `src/llama_cpp/llama_cpp_plugin_validation_gates.cpp`
- `include/llama_cpp/llama_cpp_plugin.h`
- `include/llama_cpp/llama_cpp_registrar.h`
- `tests/llama_cpp/*.cpp`

## Related documents

- [`README.md`](README.md)
- [`ARCHITECTURE.md`](ARCHITECTURE.md)
- [`ROADMAP.md`](ROADMAP.md)
- [`SECURITY.md`](SECURITY.md)
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
