> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-10 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — llama_cpp Plugin

All notable changes to the llama_cpp LLM backend plugin are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- True parallel `generateBatch()` when real llama.cpp multi-sequence batching is available
- SHA-256 model digest (replace FNV-64 placeholder with OpenSSL `EVP_DigestFinal`)

## [2.4.0] — 2026-08-09

### Added
- **`std::atomic<uint64_t>` counters**: `inference_count_`, `error_count_`, `stream_retry_count_`
  are now lock-free atomics; `getPerformanceStats()` exposes all three including `stream_retry_count`.
- **Stream-callback retry**: `generate()` wraps every `stream_callback` invocation in
  `invokeStreamCallback()` with up to 3 transient-exception retries; `std::bad_alloc` is
  non-retryable; `stream_retry_count_` tracks retries for observability.
- **`joinWithTimeout(thread, 5000ms)`**: anonymous-namespace helper that detaches and logs
  a `spdlog::warn` if a monitored thread does not complete within the deadline.
- **`importLoRA` security hardening**: GGUF magic-bytes validation (`0x47 0x47 0x55 0x46`)
  and 2 GB size bound enforced before any delegation to `LlamaWrapper`; fail-closed.
- **`loadModel` integrity gate**: opt-in FNV-64 digest check via `"verify_model_digest": true`
  + `"expected_model_digest"` config keys; fails closed on mismatch. Upgrade path to SHA-256
  documented in header.
- **`setPolicyFn(PolicyFn)`**: pluggable inference policy hook; `generate()` and `generateRAG()`
  gate on the functor result before dispatching inference; denial returns `success=false` with
  the caller-supplied reason.
- **`LlamaCppPluginRegistrar::initFromServerConfig(server_config)`**: server-startup integration
  point; reads `config["llm"]["model_path"]` and delegates to `registerWithLLMManager()`.
- **`defaultReloadCallback()` fix**: now calls `plugin.loadModel(path, config)` when
  `model_path` is present; returns `true` (stub mode) for empty path.
- **generateRAG() data-race fix**: shared state (`model_loaded_`, `context_length_`) is
  snapshotted under `mutex_` at entry; subsequent work is lock-free.
- **LLCPG-1..4 release gate benchmarks**: TTFT stub baseline, 100-doc batch-embedding
  throughput, LoRA-load P99, and regression-baseline snapshot (all with `UseRealTime()`).
- **21 new tests** — Groups U (concurrency ×4), V (security ×6), W (registrar integration ×8),
  X (retry/join ×3).

### Fixed
- `generateRAG()`: CRITICAL data-race on `model_loaded_` / `context_length_` reads outside mutex.
- `defaultReloadCallback()`: previously contained a stub comment that prevented real model reload.
- `getPerformanceStats()`: now uses explicit `.load()` for all atomic reads.

### Security
- `importLoRA`: GGUF magic + 2 GB size-bound prevent heap-exhaustion and deserialization attacks.
- `loadModel`: opt-in model-file digest gate prevents tampered GGUF models from being loaded.
- `setPolicyFn`: inference can be gated by an external governance policy without modifying the plugin.

## [2.3.0] — 2026-06-10

### Added
- **Function / tool calling**: `LlamaCppPlugin::getCapabilities().supports_function_call`
  is now `true`.  When `InferenceRequest::tools` is non-empty:
  - With `THEMIS_LLM_ENABLED` + a loaded `LlamaWrapper`: tool-call grammar
    constraint and `JsonSchemaConverter::parseToolCall()` are already handled
    inside `LlamaWrapper::generate()`.
  - With an injected `generate_fn_` bridge: tools are forwarded unchanged in
    the request; bridge-produced `tool_calls` pass through untouched.
  - In stub / test mode (`THEMIS_LLAMA_CPP_STUB_MODE`): a minimal JSON tool-call
    for the first tool definition is synthesised, parsed, and stored in
    `InferenceResponse::tool_calls` so callers and tests can exercise the path
    without a real model.
- **Per-request cancellation token**: `InferenceRequest` gains a
  `std::shared_ptr<std::atomic<bool>> cancellation_token` field (default:
  `nullptr` = no cancellation).  `LlamaCppPlugin::generate()` checks the token
  immediately after state acquisition; if already set to `true` the call returns
  `success=false` + `error_message="Request cancelled"` without starting inference.
- 5 new unit tests (groups S1–S3, T1–T2) covering tool calling and cancellation.

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
