<!-- Status: current | validated: 2026-04-10 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — llama_cpp Plugin

All notable changes to the llama_cpp LLM backend plugin are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Integration with real llama.cpp via the existing `LlamaWrapper`
- Real embedding model support (currently returns fixed 384-dim zero vector)
- Function/tool calling support

## [2.1.0] — 2026-04-10

### Added
- Streaming token output: `LlamaCppPlugin::generate()` now calls
  `InferenceRequest::stream_callback` (when set) with the generated text.
  In stub mode the full response text is delivered as a single callback
  invocation so callers always receive at least one token event.
- `LlamaCppPlugin::generateStream(request, callback)` convenience method that
  injects the callback into `InferenceRequest::stream_callback` and delegates
  to `generate()`.
- `LlamaCppPlugin::generateBatch(requests)` batch inference method that
  processes requests sequentially and returns a same-size response vector.
  Per-request errors are propagated individually without aborting the batch.
- `LlamaCppPluginRegistrar` (`include/llama_cpp/llama_cpp_registrar.h` +
  `src/llama_cpp/llama_cpp_registrar.cpp`): factory and registration helper
  for PluginManager hot-plug integration.  Provides `createPlugin()`,
  `createAdapter()`, `registerWithLLMManager()`, and `defaultReloadCallback()`.
- 20 new unit tests (groups K–M) covering streaming, batch inference,
  capabilities v2.1.0, and PluginManager registration.
- `getCapabilities().supports_streaming` set to `true`.
- `getCapabilities().supports_batching` set to `true`.
- `getCapabilities().plugin_version` bumped to `"2.1.0"`.
- `plugins/llama_cpp/plugin.json.in`: `supports_streaming` and
  `supports_batching` updated to `true`.
- `InferenceResponse::tokens_generated` now set from stub text length.
- `InferenceResponse::trace_id` / `span_id` echoed from request.

### Changed
- `getPluginVersion()` returns `"2.1.0"`.
- `src/llama_cpp/CMakeLists.txt`: version bumped to 2.1.0; registrar source added.

## [2.0.0] — 2026-04-07

### Added
- `LlamaCppPlugin : ILLMPlugin` — full interface implementation for dynamic loading
- `loadModel` / `unloadModel` with stub mode (no model file required for tests)
- `getModelInfo()` returning `std::optional<ModelInfo>` (nullopt when not loaded)
- `generate(InferenceRequest)` — stub returns echo response; error when not loaded
- `generateRAG(InferenceRequest, context_docs)` — prepends context docs and delegates
  to `generate()`
- `embed(text)` — returns 384-dim zero vector when loaded, empty when not loaded
- `loadLoRA` / `unloadLoRA` / `listLoRAs` — full LoRA registry with duplicate-id
  replacement and thread-safe access via `std::mutex`
- `getCapabilities()` — `supports_lora=true`, `supports_embeddings=true`,
  `plugin_version="2.0.0"`
- `getMemoryStats()` / `getPerformanceStats()` — JSON stats with `inference_count`,
  `error_count`, `model_loaded`, `lora_count`
- `exportLoRA` / `importLoRA` — stub implementations (return empty / false)
- `THEMIS_LLM_PLUGIN()` export macro in `include/llm/llm_plugin_interface.h`
- `themis_llm_create` / `themis_llm_destroy` C-linkage entry points
- 30 unit tests (`LlamaCppPluginFocusedTests`, groups A–J)
- `plugins/llama_cpp/plugin.json.in` — plugin manifest
- `src/llama_cpp/CMakeLists.txt` — build target
- `tests/CMakeLists.txt` — `LlamaCppPluginFocusedTests` registered
- `plugins/CMakeLists.txt` — `THEMIS_PLUGIN_LLAMA_CPP` option added
