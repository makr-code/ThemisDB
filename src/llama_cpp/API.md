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
Canonical_Sources: "include/llama_cpp/llama_cpp_plugin.h; include/llama_cpp/llama_cpp_registrar.h; include/llm/llm_plugin_interface.h; src/llama_cpp/llama_cpp_plugin.cpp; src/llama_cpp/llama_cpp_registrar.cpp"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

# llama_cpp API Reference

For C++ callers of the llama_cpp plugin and registrar. Declarations in the public headers take precedence over this summary.

> Exact model backend not verifiable from final document text alone.

## Plugin lifecycle and request APIs

The concrete class is [`themis::llamacpp::LlamaCppPlugin`](../../include/llama_cpp/llama_cpp_plugin.h), implementing [`themis::llm::ILLMPlugin`](../../include/llm/llm_plugin_interface.h).

| API | Contract and caller notes |
|---|---|
| `bool loadModel(const std::string&, const json&)` | Configures path/id/context and attempts wrapper creation when compiled with `THEMIS_LLM_ENABLED`. `true` is not proof of a live wrapper; inspect backend behavior and model info. |
| `void unloadModel()` | Clears plugin loaded state, metadata and wrapper. |
| `InferenceResponse generate(const InferenceRequest&)` | Policy callback (if installed), loaded-state and pre-inference cancellation checks precede dispatch. Returns structured errors on denial/unavailable backend. |
| `InferenceResponse generateRAG(const RAGContext&, const InferenceRequest&)` | Assembles bounded prompt context and dispatches through `generate()`. |
| `std::vector<float> embed(const std::string&)` | Empty when plugin is not marked loaded; wrapper or injected callback output when available; otherwise a 384-zero fallback. See [`EMBEDDING.md`](EMBEDDING.md). |
| `generateStream(request, callback)` | Sets/overwrites `request.stream_callback` and delegates to `generate()`. Callback behavior depends on selected backend path. |
| `generateBatch(requests)` | Sequential calls to `generate()`; result count/order match inputs. |
| `setPolicyFn(PolicyFn)` | Installs optional request gate; nullptr disables it. It is an integration hook, not built-in authentication or authorization. |
| `setGenerateFn(GenerateFn)`, `setEmbedFn(EmbedFn)` | Install test/integration callbacks. Embedding callback must not throw per API comment; implementation catches exceptions and uses zero fallback. |
| `getCapabilities()`, `getMemoryStats()`, `getPerformanceStats()`, `getModelInfo()` | Interface/counters/runtime metadata. Capability flags are static advertisements, not readiness probes. |

`InferenceRequest` includes `prompt`, `model_id`, token/sampling settings, optional system prompt, grammar/schema/tool fields, stop sequences, stream callback, metadata, trace/span IDs, tenant ID and cancellation token. `InferenceResponse` carries success/error, text, trace identifiers, token/time fields, model metadata, tool calls and optional quality data. Consult the interface header for field definitions and do not assume every mode populates every field.

## Configuration and registration

Recognized plugin configuration includes `model_path`, `context_length` or `n_ctx`, `n_gpu_layers`, `n_batch`, `n_threads`, and the optional `verify_model_digest` / `expected_model_digest` pair. The digest implementation currently computes FNV-64, not a cryptographic digest; see [`TRUST.md`](TRUST.md).

[`LlamaCppPluginRegistrar`](../../include/llama_cpp/llama_cpp_registrar.h) exposes:

- `createPlugin(config)`: creates a plugin and calls `loadModel()` only for a nonempty string `model_path`.
- `createAdapter(config)`: wraps a created plugin in `LLMPluginAdapter`.
- `registerWithLLMManager(manager, name, config)`: creates and transfers plugin ownership; returns false on caught exceptions.
- `initFromServerConfig(server_config)`: no-op success when there is no `llm` section, no model path, or an empty path; otherwise registers as `llama_cpp`.
- `defaultReloadCallback()`: attempts reload for a nonempty path; absent path returns true only in stub mode, false otherwise.

The implementation defines C-linkage dynamic entry points `themis_llm_create` and `themis_llm_destroy`.

## Edge cases and related contracts

- `loadLoRA()` updates only the plugin's metadata registry. It does not activate a wrapper adapter.
- `importLoRA()` enforces 8-byte minimum, 2 GiB maximum and GGUF magic before optional wrapper delegation; a stub can accept validated bytes without loading weights.
- A live wrapper embedding exception is swallowed by `embed()` before callback/zero fallback.
- In the test-only stub generation path, a response is synthetic; it is not a model response.

For implementation flow and validation, see [`IMPLEMENTATION.md`](IMPLEMENTATION.md) and [`MODULE_GUIDE.md`](MODULE_GUIDE.md).

## Related module documents

[`RESEARCH.md`](RESEARCH.md) · [`MODELING.md`](MODELING.md) · [`EMBEDDING.md`](EMBEDDING.md) · [`IMPLEMENTATION.md`](IMPLEMENTATION.md) · [`MODULE_GUIDE.md`](MODULE_GUIDE.md) · [`RUNBOOK.md`](RUNBOOK.md) · [`OPERATIONS.md`](OPERATIONS.md) · [`TRUST.md`](TRUST.md)

Also see [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md), [`ROADMAP.md`](ROADMAP.md), [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md), [`SECURITY.md`](SECURITY.md), and [`../../include/llama_cpp/README.md`](../../include/llama_cpp/README.md).
