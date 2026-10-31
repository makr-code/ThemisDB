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
Canonical_Sources: "src/llama_cpp/llama_cpp_plugin.cpp; src/llm/llama_wrapper.cpp; include/llm/llm_plugin_interface.h; focused llama_cpp tests"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

# llama_cpp Research Notes

This note is for maintainers evaluating what the `llama_cpp` plugin actually does and how to measure it. It records an evaluation method, not model-quality findings: no model evaluation or production benchmark result is claimed here.

> Exact model backend not verifiable from final document text alone.

## Research question and boundaries

The module adapts `llm::ILLMPlugin` to an optional `llm::LlamaWrapper` backend. With `THEMIS_LLM_ENABLED`, a non-empty path is passed to the wrapper; without a usable wrapper, generation can use an injected `GenerateFn`, a test-only stub path, or return an error. This module does not train models or identify a preferred checkpoint. The plugin's configured runtime path is not evidence that a specific model was successfully loaded.

The relevant implementation is [`llama_cpp_plugin.cpp`](llama_cpp_plugin.cpp), with the wrapper implementation in [`../llm/llama_wrapper.cpp`](../llm/llama_wrapper.cpp). Public request/response contracts are defined in [`../../include/llm/llm_plugin_interface.h`](../../include/llm/llm_plugin_interface.h).

## Method and assumptions

1. Establish the exact build mode (`THEMIS_LLM_ENABLED`, and whether `THEMIS_LLAMA_CPP_STUB_MODE` is enabled), model artifact identity, runtime configuration, and hardware. Keep these fixed for comparisons.
2. Verify a real model load independently: inspect `isModelLoaded()`, `getModelInfo()`, and whether the wrapper exists through real inference behavior. `loadModel()` returning true alone is insufficient because wrapper-load exceptions are caught and can leave the plugin in fallback mode.
3. Use a fixed prompt set and generation parameters. Record request errors, output, latency and token counts from `InferenceResponse`; report repeated-run distributions (including p50/p95/p99) and the model/configuration alongside each result.
4. Evaluate embeddings separately: record output dimension, finite-value checks, L2 norm, and task-level retrieval quality on a documented corpus/query set. Do not count the zero-vector fallback as a model embedding.
5. Separate stub, injected-backend, and real-wrapper runs. They answer different questions and are not comparable as model evaluations.

These are recommended evaluation controls, not evidence that a model has been evaluated. The focused contract tests use stub fixtures; the llama_cpp soak test explicitly simulates queue, loader and acceleration behavior rather than running model inference.

## Available evidence and measurable paths

| Question | Existing path | What it establishes / does not establish |
|---|---|---|
| Request, error, and RAG contract | [`tests/llama_cpp/test_inference_execution.cpp`](../../tests/llama_cpp/test_inference_execution.cpp), [`src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp`](tests/test_llama_cpp_inference_contract_focused.cpp) | Exercises response shape and stub paths; not model quality or real-backend throughput. |
| Load/unload and model metadata | [`tests/llama_cpp/test_model_loading.cpp`](../../tests/llama_cpp/test_model_loading.cpp), [`src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp`](tests/test_llama_cpp_plugin_lifecycle_focused.cpp) | Lifecycle behavior in tests; fixture paths do not prove that a GGUF model was loaded. |
| Validation helper behavior | [`src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp`](tests/test_llama_cpp_validation_gates_focused.cpp) | Focused smoke assertions; several cases explicitly assert no crash rather than a numerical outcome. |
| Sustained stub traffic | [`tests/integration/test_llama_cpp_soak.cpp`](../../tests/integration/test_llama_cpp_soak.cpp) | Simulated queue/load/acceleration acceptance checks; its ops/s result is not tokens/s or real inference throughput. |
| Runtime counters | `getPerformanceStats()` in [`llama_cpp_plugin.cpp`](llama_cpp_plugin.cpp) | Exposes inference, error and stream-retry counts, not latency histograms or quality metrics. |

For baseline expectations and their named benchmark mappings, see [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md). Verify that a benchmark uses the real backend and representative hardware before treating its measurements as model evidence.

## Limits and interpretation

- The plugin provides no built-in dataset, model-quality score, or reproducible evaluation corpus.
- Its `getCapabilities()` values describe the advertised interface, not proof that the current build has a real inference or embedding backend.
- `generateBatch()` is a sequential loop. Stub output, injected callbacks and live wrapper output must be reported separately.
- The standalone validation-gate export is not shown as a call from `generate()` or `loadModel()` in this module's implementation. Do not assume its checks run on every inference or model load.
- `ai_context/developer_llm_wiki` is a 2026-10-26 snapshot and is not current implementation evidence.

## Related module documents

[`MODELING.md`](MODELING.md) · [`EMBEDDING.md`](EMBEDDING.md) · [`IMPLEMENTATION.md`](IMPLEMENTATION.md) · [`API.md`](API.md) · [`MODULE_GUIDE.md`](MODULE_GUIDE.md) · [`RUNBOOK.md`](RUNBOOK.md) · [`OPERATIONS.md`](OPERATIONS.md) · [`TRUST.md`](TRUST.md)

Also see [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`ROADMAP.md`](ROADMAP.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), [`SECURITY.md`](SECURITY.md), and [`../../include/llama_cpp/README.md`](../../include/llama_cpp/README.md).
