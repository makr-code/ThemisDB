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
Canonical_Sources: "src/llama_cpp/llama_cpp_plugin.cpp; src/llama_cpp/llama_cpp_plugin_validation_gates.cpp; include/llm/llm_plugin_interface.h; include/llm/llama_wrapper.h"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

# llama_cpp Modeling and Inference

For maintainers integrating and evaluating the model-inference path. This document describes the implemented wrapper boundary; it does not prescribe a model family or assert measured model quality.

> Exact model backend not verifiable from final document text alone.

## Model path and assumptions

`LlamaCppPlugin` implements `llm::ILLMPlugin`. A build compiled with `THEMIS_LLM_ENABLED` can construct an `llm::LlamaWrapper` for a non-empty model path. The plugin forwards selected settings (`n_gpu_layers`, `n_ctx` or `context_length`, `n_batch`, `n_threads`) into `LlamaWrapper::Config`; it disables the wrapper response cache and continuous batching for this plugin instance. The exact model architecture, tokenizer, quantization and supported context are artifact-dependent; the module does not establish these from the path string.

The wrapper executes inference against its loaded model/context. The plugin's request contract carries prompt, sampling parameters, optional system prompt, constraints, tools, stop sequences, cancellation, trace identifiers and callback fields; see [`../../include/llm/llm_plugin_interface.h`](../../include/llm/llm_plugin_interface.h). A field being present in the interface does not by itself prove every backend mode applies it.

## Implemented inference routes

`generate(request)` snapshots loaded state, evaluates an optional policy callback, rejects an unloaded plugin, and checks cancellation before backend dispatch. It delegates to `LlamaWrapper::generate()` when the real wrapper exists. Otherwise it can call an injected `GenerateFn`; only builds with `THEMIS_LLAMA_CPP_STUB_MODE` have the deterministic test response path. Ordinary no-backend production calls return an error response.

`generateRAG(context, request)` converts supplied documents to `RetrievedChunk` values and uses `RAGContextAssembler` with a context budget based on the configured model context (or a caller override) and response budget. It can build ordinary concatenated context or compact, relevance-sorted `tensor_hybrid` / `tensor_prefix` memory slots, then delegates through `generate()`. These are prompt-assembly modes, not a separately trained tensor model. No retrieval ranking or answer-quality metric is produced by this plugin.

The separate validator in [`llama_cpp_plugin_validation_gates.cpp`](llama_cpp_plugin_validation_gates.cpp) defines model-file, memory and CUDA checks and exports a validator pointer. Its presence does not establish that the normal `loadModel()` or inference route invokes that validator.

## Evaluation plan (no results asserted)

To compare model/configuration choices, hold the model artifact, build flags, hardware, prompt set, context size, and generation parameters constant. Capture:

- load outcome and actual `ModelInfo` where available;
- request success/error rate and latency distribution from response fields;
- generated token count and throughput calculated from measured elapsed time;
- output quality using an explicitly versioned task set and human or task-specific scoring;
- for RAG, retrieval inputs, context assembled/truncated state where available, answer grounding, and response-budget behavior;
- for embeddings, vector dimension/norm and retrieval metrics, as specified in [`EMBEDDING.md`](EMBEDDING.md).

Run real-backend tests and stub tests separately. The focused test suites establish interface/lifecycle behavior; [`../../tests/integration/test_llama_cpp_soak.cpp`](../../tests/integration/test_llama_cpp_soak.cpp) uses in-process simulations and is not a real-model benchmark. The current module's performance mapping is in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md); re-check the actual benchmark source and execution mode before applying its targets.

## Limits

- `loadModel()` catches wrapper construction/load exceptions and falls through to the plugin's fallback state; a true return is not sufficient proof of usable model inference.
- Plugin configuration does not expose a selectable model method or training/evaluation pipeline.
- GPU capability detection in the validation helper is a compile-time/macro-based placeholder in this source; it is not evidence of device enumeration or measured acceleration.
- `validateTokenLimits()` is defined as a helper but has no call site in the plugin implementation. Do not assume it clamps every live request.

## Related module documents

[`RESEARCH.md`](RESEARCH.md) · [`EMBEDDING.md`](EMBEDDING.md) · [`IMPLEMENTATION.md`](IMPLEMENTATION.md) · [`API.md`](API.md) · [`MODULE_GUIDE.md`](MODULE_GUIDE.md) · [`RUNBOOK.md`](RUNBOOK.md) · [`OPERATIONS.md`](OPERATIONS.md) · [`TRUST.md`](TRUST.md)

Also see [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`ROADMAP.md`](ROADMAP.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), [`SECURITY.md`](SECURITY.md), and [`../../include/llama_cpp/README.md`](../../include/llama_cpp/README.md).
