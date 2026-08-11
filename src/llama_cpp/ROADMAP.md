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

### Phase 7 — Security & Concurrency Hardening ✅ (v2.4.0 — 2026-08-09)
- [x] `inference_count_` / `error_count_` converted to `std::atomic<uint64_t>` — lock-free reads (A1)
- [x] `stream_retry_count_` added as `std::atomic<uint64_t>` — exposed in `getPerformanceStats()` (A1)
- [x] `generateRAG()` data-race fixed: shared state (`model_loaded_`, `context_length_`) snapshotted under mutex at entry (A2)
- [x] `generateStream()` stream-callback retry: up to 3 attempts for transient exceptions; `bad_alloc` non-retryable; `stream_retry_count_` incremented per transient retry (D1)
- [x] `thread_join_no_timeout` findings triaged as false positives; no helper retained because the module has no owned join sites (D2)
- [x] `importLoRA` GGUF magic-bytes check (`0x47 0x47 0x55 0x46`) + 2 GB size bound — fail-closed (B2)
- [x] `loadModel()` opt-in model-file integrity gate via `verify_model_digest` + `expected_model_digest` config keys (B3)
- [x] `setPolicyFn(PolicyFn)` — pluggable inference policy hook; `generate()` / `generateRAG()` gate on denial (B4)
- [x] `LlamaCppPluginRegistrar::initFromServerConfig(server_config)` — server-startup integration point; reads `config["llm"]["model_path"]` (C1)
- [x] `defaultReloadCallback()` fixed — calls `loadModel()` when path present; returns `true` in stub mode (C2)
- [x] LLCPG-1..4 release gate benchmarks added (TTFT, batch-embed, LoRA-load P99, regression baseline) (E1)
- [x] Tests Groups U (concurrency, 4), V (security, 6), W (registrar integration, 8), X (retry/join, 3) — 21 new tests (Q3 2026)

## Production Readiness Checklist

- [x] Unit tests present (89 tests: groups A–X)
- [x] Stub mode for CI without model file
- [x] Thread-safe LoRA registry
- [x] Capabilities correctly reported
- [x] `context_length` read from config JSON (`n_ctx`/`context_length` keys, fallback 4096)
- [x] `ModelInfo::context_length` populated from config on `loadModel()`
- [x] `generateRAG()` uses `RAGContextAssembler` — no naive document concatenation
- [x] `InferenceRequest::max_tokens` capped by `RAGContextAssembler::computeMaxTokens()`
- [x] `generateStream()` honours callback with 3-attempt transient-exception retry
- [x] `generateBatch()` preserves request order in response vector
- [x] `LlamaCppPluginRegistrar` provides PluginManager hot-plug integration
- [x] `LlamaCppPluginRegistrar::initFromServerConfig()` provides server-startup integration point
- [x] Real llama.cpp inference wired in (`THEMIS_LLM_ENABLED`)
- [x] Real embeddings via `LlamaWrapper::embed()` with L2 normalisation
- [x] `exportLoRA` / `importLoRA` delegated to `LlamaWrapper`; `importLoRA` GGUF-validated before delegation
- [x] Concurrency hardening verified: 8-thread generate(), 4-thread generateBatch(), 6-thread LoRA+generate() race — all pass (P1–P3)
- [x] `inference_count_` / `error_count_` / `stream_retry_count_` are `std::atomic<uint64_t>` — lock-free reads
- [x] `generateRAG()` shared-state data-race eliminated (mutex snapshot at entry)
- [x] `supports_function_call = true`; tool-call stub synthesised in test/stub mode; tools forwarded through bridge path (S1–S3)
- [x] Per-request cancellation token (`InferenceRequest::cancellation_token`); pre-inference check returns `success=false` / `"Request cancelled"` (T1–T2)
- [x] Model file integrity check: opt-in via `"verify_model_digest": true` + `"expected_model_digest"` config keys
- [x] LoRA adapter integrity: GGUF magic bytes + 2 GB size bound validated in `importLoRA`
- [x] Inference policy gate: `setPolicyFn(fn)` pluggable hook; denial returns `success=false`
- [x] LLCPG-1..4 release gate benchmarks present (TTFT, batch-embed throughput, LoRA P99, regression baseline)

## Known Issues & Limitations

- `generateBatch()` is sequential; true parallel batch requires real llama.cpp.
- Stub mode is active when compiled without `THEMIS_LLM_ENABLED` or when the
  model path is empty / the file does not exist.

## Breaking Changes

v2.1.0 — `getCapabilities().plugin_version` changed from `"2.0.0"` to `"2.1.0"`.
`getPluginVersion()` similarly returns `"2.1.0"`.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-08-09 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🧪 NUR_TESTS (implementiert, kein Produktions-Aufrufer)

- `LlamaCppPlugin` – LLM-Plugin-Implementierung für llama.cpp; vollständig implementiert
  (`generate`, `embed`, `generateRAG`, `generateStream`, `generateBatch`, LoRA-Lifecycle,
  Memory/Performance-Stats, Policy-Gate, Security-Validation). 89 Unit-Tests + Benchmark vorhanden.

> **✅ Produktionslücke geschlossen (v2.4.0):** `LlamaCppPluginRegistrar::initFromServerConfig(server_config)`
> ist als sauberer Server-Startup-Integrationspunkt implementiert. Der Server-Startup-Code kann
> `LlamaCppPluginRegistrar::initFromServerConfig(config)` aufrufen, um `LlamaCppPlugin` in den
> `LLMPluginManager` zu registrieren, wenn `config["llm"]["model_path"]` gesetzt ist.
> `defaultReloadCallback()` delegiert nun korrekt an `loadModel()` statt eines Stub-Kommentars.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `llama_cpp`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
