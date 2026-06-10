> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# llama_cpp Plugin Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-04-12 | Primary: src/llama_cpp/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

v2.2.0 — Real `LlamaWrapper` inference wired in behind `THEMIS_LLM_ENABLED`.
`generate()`, `embed()`, `exportLoRA`, and `importLoRA` all delegate to
`LlamaWrapper` when a non-empty model path is provided and the macro is set.
Stub mode (empty path / CI without model) is preserved as a transparent fallback.

## Completed ✅

- [x] `THEMIS_LLM_PLUGIN()` export macro
- [x] `LlamaCppPlugin : ILLMPlugin` — full interface (generate, RAG, embed, LoRA, stats)
- [x] `loadModel` / `unloadModel` with stub mode and real `LlamaWrapper` initialisation
- [x] Thread-safe LoRA registry (`std::mutex`)
- [x] `getCapabilities()` — `supports_lora`, `supports_embeddings`, `plugin_version`
- [x] `getMemoryStats()` / `getPerformanceStats()` — JSON
- [x] `themis_llm_create` / `themis_llm_destroy` C-linkage entry points
- [x] 50 unit tests (`LlamaCppPluginFocusedTests`, groups A–N)
- [x] Plugin manifest + CMake registration
- [x] Streaming token output via `InferenceRequest::stream_callback` (v2.1.0)
- [x] `generateStream(request, callback)` convenience method (v2.1.0)
- [x] `generateBatch(requests)` batch inference method (v2.1.0)
- [x] `LlamaCppPluginRegistrar` — PluginManager hot-plug integration (v2.1.0)
- [x] `getCapabilities().supports_streaming = true` (v2.1.0)
- [x] `getCapabilities().supports_batching = true` (v2.1.0)
- [x] Real llama.cpp inference via `LlamaWrapper` behind `THEMIS_LLM_ENABLED` (v2.2.0)
- [x] Real embedding vectors via `LlamaWrapper::embed()` (v2.2.0)
- [x] `exportLoRA` / `importLoRA` delegated to `LlamaWrapper` (v2.2.0)
- [x] `tests/CMakeLists.txt` updated — registrar + deps added for N1–N6 (v2.2.0)

## In Progress

*(none — all previously in-progress items are now complete)*

## Planned Features

- [x] Function/tool calling (Target: Q4 2026)
- [x] Per-request cancellation token (Target: Q4 2026)

## Implementation Phases

### Phase 1 — Design / API Contract ✅
- [x] `ILLMPlugin` interface reviewed; all methods implemented

### Phase 2 — Core Implementation ✅
- [x] `LlamaCppPlugin` stub with load/generate/embed/LoRA lifecycle
- [x] Real `LlamaWrapper` delegation behind `THEMIS_LLM_ENABLED` (v2.2.0)

### Phase 3 — Error Handling & Edge Cases ✅
- [x] `generate()` returns error when model not loaded
- [x] `embed()` returns empty when model not loaded
- [x] Thread-safe LoRA registry (duplicate id replacement)
- [x] `generateStream()` swallows callback exceptions; increments `error_count_`
- [x] `generateBatch()` propagates per-request errors without aborting the batch
- [x] Stub fallback when `LlamaWrapper::loadModel()` fails (file not found, etc.) (v2.2.0)

### Phase 4 — Tests ✅
- [x] 50 unit tests across groups A–N
- [x] 3 group-O structured-error tests (O1–O3): generate() without model loaded
- [x] Registrar link fixed in `tests/CMakeLists.txt` (v2.2.0)

### Phase 5 — Performance / Hardening ✅
- [x] Real llama.cpp inference benchmark (`benchmarks/bench_llama_cpp_inference.cpp`; stub path exercised in CI; 6 benchmark scenarios) (Target: Q3 2026)
- [x] Concurrency test P1: 8 threads × 10 `generate()` calls — no race, no deadlock (Target: Q3 2026)
- [x] Concurrency test P2: 4 threads concurrent `generateBatch(5)` — correct response count (Target: Q3 2026)
- [x] Concurrency test P3: interleaved `loadLoRA()` + `generate()` from 6 threads — all succeed (Target: Q3 2026)

### Phase 6 — Documentation & Acceptance ✅
- [x] README, CHANGELOG, ROADMAP, ARCHITECTURE, FUTURE_ENHANCEMENTS, AUDIT, SECURITY

## Production Readiness Checklist

- [x] Unit tests present (68 tests: groups A–T)
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
- [x] Real llama.cpp inference wired in (`THEMIS_LLM_ENABLED`)
- [x] Real embeddings via `LlamaWrapper::embed()` with L2 normalisation
- [x] `exportLoRA` / `importLoRA` delegated to `LlamaWrapper`
- [x] Concurrency hardening verified: 8-thread generate(), 4-thread generateBatch(), 6-thread LoRA+generate() race — all pass (P1–P3)
- [x] `supports_function_call = true`; tool-call stub synthesised in test/stub mode; tools forwarded through bridge path (S1–S3)
- [x] Per-request cancellation token (`InferenceRequest::cancellation_token`); pre-inference check returns `success=false` / `"Request cancelled"` (T1–T2)

## Known Issues & Limitations

- `generateBatch()` is sequential; true parallel batch requires real llama.cpp.
- Stub mode is active when compiled without `THEMIS_LLM_ENABLED` or when the
  model path is empty / the file does not exist.

## Breaking Changes

v2.1.0 — `getCapabilities().plugin_version` changed from `"2.0.0"` to `"2.1.0"`.
`getPluginVersion()` similarly returns `"2.1.0"`.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🧪 NUR_TESTS (implementiert, kein Produktions-Aufrufer)

- `LlamaCppPlugin` – LLM-Plugin-Implementierung für llama.cpp; vollständig implementiert
  (`generate`, `embed`, `generateRAG`, `generateStream`, `generateBatch`, LoRA-Lifecycle,
  Memory/Performance-Stats). 50 Unit-Tests + Benchmark vorhanden.

> **Produktionslücke:** `LlamaCppPluginRegistrar::registerWithLLMManager()` existiert und ist
> vollständig implementiert, wird aber **nicht** vom Server-Startup aufgerufen. Der
> `LLMPluginManager` wird in `http_server.cpp` (Zeile 3416) für `/api/v1/llm/*`-Endpunkte
> genutzt — aber kein Startup-Code registriert `LlamaCppPlugin` in diesem Manager.
>
> **STUB-Hinweis in Registrar:** `defaultReloadCallback()` enthält den Kommentar
> "Stub mode — no model to load; treat as success" für den Fall ohne Modelpfad.
>
> **Aktion:** In `HttpServer::init()` oder einer dedizierten Plugin-Konfigurationsdatei
> `LlamaCppPluginRegistrar::registerWithLLMManager(plugin_mgr, ...)` aufrufen, sobald
> Modelpfad aus Server-Konfiguration gelesen wird (Target: Q3 2026).

