---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

> Exact model backend not verifiable from final document text alone.

# llama_cpp Plugin Roadmap

The roadmap records module work against the current implementation in
`src/llama_cpp/llama_cpp_plugin.cpp` and `llama_cpp_registrar.cpp`. The plugin
reports version `2.1.0`; its advertised capabilities and loaded flag do not prove
that a real model backend is usable. Production gates are in
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md).

## Current Status

The module implements the `ILLMPlugin` interface, with optional real inference
through `LlamaWrapper` when the parent build enables `THEMIS_ENABLE_LLM`
(`THEMIS_LLM_ENABLED` in the plugin). The module CMake target is a static C++17
library. `THEMIS_LLAMA_CPP_STUB_MODE` is used by focused test targets; it is not a
production backend.

`loadModel()` can return success when wrapper initialization or model loading
failed, leaving the wrapper unavailable while the plugin's loaded flag is true.
Consequently, a real inference probe is required for deployment readiness.

## In Progress

None currently tracked.

## Planned Features

None currently approved. Record new implementation work only after its interfaces,
failure behavior, validation evidence, and measurable performance target (where
applicable) are defined.

## Implementation Phases

### Phase 1 — Design / API Contract

- [x] Implement the `ILLMPlugin` contract in `include/llama_cpp/llama_cpp_plugin.h`.
- [x] Define registrar creation and server-config integration in
  `include/llama_cpp/llama_cpp_registrar.h`.

### Phase 2 — Core Implementation

- [x] Implement optional wrapper-backed generation and embeddings behind
  `THEMIS_LLM_ENABLED`.
- [x] Implement RAG context assembly through `RAGContextAssembler`.
- [x] Implement plugin registration, generation callbacks, sequential batching,
  capability reporting, and performance counters.

### Phase 3 — Error Handling & Edge Cases

- [x] Return structured generation failures for policy denial, pre-dispatch
  cancellation, unloaded state, and unavailable backends in non-stub builds.
- [x] Bound LoRA import size and check GGUF magic before wrapper delegation.
- [x] Keep embedding fallback behavior and wrapper-load limitations explicit in
  the module contract.

### Phase 4 — Tests

- [x] Provide focused lifecycle, inference-contract, registrar, and validation
  tests under `src/llama_cpp/tests/`.
- [x] Provide focused model-loading, inference, and stress tests under
  `tests/llama_cpp/`.
- [x] Register llama_cpp CTest targets in `tests/llama_cpp/CMakeLists.txt`.

### Phase 5 — Performance / Hardening

- [x] Expose inference, error, and stream-retry counters.
- [x] Add the module inference benchmark at
  `benchmarks/llama_cpp/bench_llama_cpp_inference.cpp`.
- [x] Keep batch execution sequential and document that it is not parallel
  batching; validate production performance separately with a real model.

### Phase 6 — Documentation & Acceptance

- [x] Align the README, architecture, roadmap, and production requirements with
  the live implementation.
- [x] Record test-fixture, fallback, validator, and deployment-readiness limits.
- [x] Provide focused CTest and direct Doxygen validation commands.

## Production Readiness Checklist

- [x] Implement model lifecycle and runtime configuration in `LlamaCppPlugin`.
- [x] Provide wrapper-backed generation and embedding when the LLM backend is
  enabled and a model successfully loads.
- [x] Apply an installed `PolicyFn` to generation and RAG dispatch.
- [x] Bound and format-check imported LoRA bytes before optional wrapper
  delegation.
- [x] Provide focused module tests and a module inference benchmark.
- [ ] Verify a real-backend inference and embedding probe for each deployed model
  and configuration before enabling production traffic.
- [ ] Validate model provenance with deployment-controlled cryptographic
  evidence; the plugin's FNV-64 check is not an authenticity control.

The checked implementation items do not constitute production sign-off. See
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) for deployment gates.

## Known Issues & Limitations

- `loadModel()` may return `true` after a wrapper load failure; `model_loaded_`,
  `getModelInfo()`, and static capability flags are not readiness signals.
- Without a live wrapper or injected `GenerateFn`, generation fails in normal
  builds. The synthetic generation response is limited to builds defining
  `THEMIS_LLAMA_CPP_STUB_MODE`.
- `embed()` may return a 384-element zero vector after wrapper or callback
  failure; this is not a semantic embedding.
- The LoRA registry stores metadata. In a no-wrapper path, successful payload
  checks do not load adapter weights.
- `generateBatch()` processes requests sequentially and preserves their order.
- The opt-in model digest uses FNV-64, not a cryptographic digest. The separate
  validation helpers are not automatically invoked by `loadModel()`.
- Focused test fixtures and the integration soak do not establish production
  model quality, throughput, or load reliability.

## Breaking Changes

No public API change is introduced by this documentation update. The plugin
currently reports version `2.1.0` through its capability and version methods.
