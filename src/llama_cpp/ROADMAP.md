# llama_cpp Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-04-10 | Primary: src/llama_cpp/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

v2.1.0 — Streaming token output, batch inference, and PluginManager hot-plug
registrar added. Full `ILLMPlugin` interface remains implemented in stub mode;
real inference requires linking the existing `LlamaWrapper`.

## Completed ✅

- [x] `THEMIS_LLM_PLUGIN()` export macro
- [x] `LlamaCppPlugin : ILLMPlugin` — full interface (generate, RAG, embed, LoRA, stats)
- [x] `loadModel` / `unloadModel` with stub mode
- [x] Thread-safe LoRA registry (`std::mutex`)
- [x] `getCapabilities()` — `supports_lora`, `supports_embeddings`, `plugin_version`
- [x] `getMemoryStats()` / `getPerformanceStats()` — JSON
- [x] `themis_llm_create` / `themis_llm_destroy` C-linkage entry points
- [x] 50 unit tests (`LlamaCppPluginFocusedTests`, groups A–M)
- [x] Plugin manifest + CMake registration
- [x] Streaming token output via `InferenceRequest::stream_callback` (v2.1.0)
- [x] `generateStream(request, callback)` convenience method (v2.1.0)
- [x] `generateBatch(requests)` batch inference method (v2.1.0)
- [x] `LlamaCppPluginRegistrar` — PluginManager hot-plug integration (v2.1.0)
- [x] `getCapabilities().supports_streaming = true` (v2.1.0)
- [x] `getCapabilities().supports_batching = true` (v2.1.0)

## In Progress

- [~] Real llama.cpp inference via `LlamaWrapper`

## Planned Features

- [ ] Real llama.cpp inference via `LlamaWrapper` (Target: Q3 2026)
- [ ] Real embedding model (non-zero vectors) (Target: Q3 2026)
- [ ] Function/tool calling (Target: Q4 2026)
- [ ] `exportLoRA` / `importLoRA` real implementation (Target: Q4 2026)
- [ ] Per-request cancellation token (Target: Q4 2026)

## Implementation Phases

### Phase 1 — Design / API Contract ✅
- [x] `ILLMPlugin` interface reviewed; all methods implemented

### Phase 2 — Core Implementation ✅
- [x] `LlamaCppPlugin` stub with load/generate/embed/LoRA lifecycle

### Phase 3 — Error Handling & Edge Cases ✅
- [x] `generate()` returns error when model not loaded
- [x] `embed()` returns empty when model not loaded
- [x] Thread-safe LoRA registry (duplicate id replacement)
- [x] `generateStream()` swallows callback exceptions; increments `error_count_`
- [x] `generateBatch()` propagates per-request errors without aborting the batch

### Phase 4 — Tests ✅
- [x] 50 unit tests across groups A–M

### Phase 5 — Performance / Hardening
- [ ] Real llama.cpp inference benchmark (Target: Q3 2026)
- [ ] Concurrency test for `loadModel` / `generate` race (Target: Q3 2026)
- [ ] Concurrency test for `generateBatch` under parallel callers (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

## Production Readiness Checklist

- [x] Unit tests present (50 tests)
- [x] Stub mode for CI without model file
- [x] Thread-safe LoRA registry
- [x] Capabilities correctly reported
- [x] `context_length` read from config JSON (`n_ctx`/`context_length` keys, fallback 4096)
- [x] `ModelInfo::context_length` populated from config on `loadModel()`
- [x] `generateRAG()` uses `RAGContextAssembler` — no naive document concatenation
- [x] `InferenceRequest::max_tokens` capped by `RAGContextAssembler::computeMaxTokens()`
- [x] `generateStream()` honours callback; callback exceptions swallowed
- [x] `generateBatch()` preserves request order in response vector
- [x] `LlamaCppPluginRegistrar` provides PluginManager hot-plug integration
- [ ] Real llama.cpp inference validated end-to-end
- [ ] Real embeddings validated
- [ ] `exportLoRA` / `importLoRA` implemented

## Known Issues & Limitations

- `generate()` returns an echo stub; real inference not wired in v2.1.0.
- `embed()` returns a zero vector; real embeddings require a loaded model.
- `exportLoRA` / `importLoRA` are stubs returning empty/false.
- `generateBatch()` is sequential; true parallel batch requires real llama.cpp.

## Breaking Changes

v2.1.0 — `getCapabilities().plugin_version` changed from `"2.0.0"` to `"2.1.0"`.
`getPluginVersion()` similarly returns `"2.1.0"`.

