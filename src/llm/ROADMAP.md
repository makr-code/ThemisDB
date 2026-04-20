> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# LLM Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Roadmap-Status: current | validated: 2026-04-09 | Primary: src/llm/ | Secondary: docs/de/llm/ -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../docs/de/llm/README.md -->

## Current Status
v1.19.0 – Full-featured production LLM module. v1.18.0 added LookupDecoder (n-gram lookup, LKD-01..18),
KV-budget guard in ContinuousBatchScheduler, and RAID sharding hints in EnhancedInferenceRequest.
v1.19.0 adds LLM+RAID integration bridge:

- `ContinuousBatchScheduler::getLLMStats()` – returns `ShardStats{pending_requests, avg_queue_ms}` for real-time shard telemetry
- `AdaptiveShardRouter::updateShardLLMLoad()` + LEAST_LOADED tie-breaking in `routeByDomain()`
- `SpeculativeDecoder::Config::remote_draft_shard_id` – cross-shard speculative decoding shard hint

Key additions since v1.15.0:
- Function/tool calling (JSON schema binding) (Issue: #1922)
- Model hot-swap without engine restart (Issue: #1923)
- Request deduplication cache (Issue: #1924)
- Per-model resource quotas (Issue: #1925)
- Multi-modal input/vision support, experimental (Issue: #1927)
- LoRA adapter hot-loading at inference time (Issue: #1929)
- Model quantization pipeline: GGUF, AWQ, GPTQ (Issue: #2412)

## Completed ✅
- [x] AsyncInferenceEngine – lightweight async wrapper for single-model inference
- [x] InferenceEngineEnhanced – enterprise multi-model engine with KV-cache, batching, and load balancing
- [x] InferenceHandle extracted to `include/llm/inference_handle.h` (v1.15.0 refactor)
- [x] Priority queue and worker thread pool for request scheduling
- [x] Dynamic batching for improved throughput
- [x] Context caching (KV-cache reuse)
- [x] Multi-model load balancing
- [x] Backpressure handling
- [x] LLM interaction storage and prompt/response tracking
- [x] Chain-of-thought storage
- [x] Conversation history management
- [x] Grammar-constrained generation with runtime API detection
- [x] Streaming token output (SSE / chunked response) (Target: Q2 2026) (Issue: #1918)
- [x] Shared worker pool (work-stealing) between AsyncInferenceEngine and InferenceEngineEnhanced (Issue: #1945)
- [x] Per-request timeout and cancellation propagation (Target: Q2 2026) (Issue: #2411)
- [x] Unified metrics dashboard for both engines (Target: Q3 2026) (Issue: #1932)
- [x] Speculative decoding for latency reduction (Issue: #1934)
- [x] Function / tool calling support (JSON schema binding) (Issue: #1922)
- [x] Model hot-swap without engine restart (Issue: #1923)
- [x] Request deduplication cache (same prompt → cached response) (Issue: #1924)
- [x] Per-model resource quotas (memory, concurrency) (Issue: #1925)
- [x] Multi-modal input support (image + text, experimental) (Issue: #1927)
- [x] LoRA adapter hot-loading at inference time (Issue: #1929)
- [x] Model quantization pipeline integration (GGUF, AWQ, GPTQ) (Issue: #2412)
- [x] ActiveVRAMAllocator: GPU VRAM allocation, OOM recovery (LRU eviction, defragmentation, CPU spilling), VRAM waste tracking (LLM-MISSING-001, 2026-03-11)

## In Progress 🚧
- [~] Inference optimizations: adaptive batching, n-gram lookup decoding, KV-budget guards, RAID-sharding hints (Issue: #LLM-INFER-OPT)

## Planned Features 📋

### Remaining
- [I] Federated inference across distributed nodes (Issue: #1928)
- [ ] Real draft-model logits for speculative decoding (Target: v1.18.0)
- [ ] Persistent disk-backed KV-cache (Target: v1.18.0)
- [ ] Hard cancellation for in-flight requests (Target: v1.18.0)
- [ ] `DecisionRecordYamlProcessor` integration: `LoraRouter`, `AdapterLoadBalancer`, `LoraOrchestrator` (Target: v1.9.0)

### Completed (formerly planned)
- [x] Function / tool calling support (JSON schema binding) (Issue: #1922)
- [x] Model hot-swap without engine restart (Issue: #1923)
- [x] Request deduplication cache (same prompt → cached response) (Issue: #1924)
- [x] Per-model resource quotas (memory, concurrency) (Issue: #1925)
- [x] Multi-modal input support (image + text, experimental) (Issue: #1927)
- [x] LoRA adapter hot-loading at inference time (Issue: #1929)
- [x] Model quantization pipeline integration (GGUF, AWQ, GPTQ) (Issue: #2412)

## Implementation Phases

### Phase 1: Dual-Engine Architecture (Status: Completed ✅)
- [x] AsyncInferenceEngine – lightweight async wrapper for single-model inference
- [x] InferenceEngineEnhanced – enterprise multi-model engine with KV-cache and load balancing
- [x] InferenceHandle extracted to `include/llm/inference_handle.h` (circular-dependency fix)
- [x] Priority queue and worker thread pool for request scheduling
- [x] Dynamic batching for improved throughput
- [x] Context caching (KV-cache reuse across requests)
- [x] Multi-model load balancing with backpressure handling
- [x] Grammar-constrained generation with runtime API detection

### Phase 2: Streaming & Shared Worker Pool (Status: Completed ✅)
- [x] Streaming token output via SSE / chunked responses (`llm/streaming_handler.cpp`) (Target: Q2 2026) (Issue: #1944)
- [x] Shared worker pool between AsyncInferenceEngine and InferenceEngineEnhanced (Target: Q2 2026) (Issue: #1945)
- [x] Per-request timeout and cancellation propagation (Target: Q2 2026)
- [x] Unified metrics dashboard for both engines (Target: Q3 2026)

### Phase 3: Ecosystem & Performance (Status: Completed ✅)
- [x] OpenAI-compatible `/v1/chat/completions` REST adapter (Issue: #1933, PR: #3068)
- [x] Speculative decoding for latency reduction (Issue: #1934)
- [x] LoRA adapter hot-loading at inference time (`llm/inference_engine_enhanced.cpp`: `loadLoRAAdapter` / `unloadLoRAAdapter`) (Issue: #1935)
- [x] Multi-model routing based on prompt content or metadata tags (Issue: #1936)
  - Implemented `ModelRouter` in `include/llm/model_router.h` + `src/llm/model_router.cpp`
  - Supports ECMAScript-regex prompt matching and metadata-tag matching with ANY/ALL modes
  - Rules are priority-sorted; integrated into `InferenceEngineEnhanced::selectModel()`
  - Public API: `addRoutingRule`, `removeRoutingRule`, `getRoutingRules`, `clearRoutingRules`
  - 22 unit and integration tests in `tests/test_model_router.cpp`
- [x] Model quantization pipeline integration (GGUF, AWQ, GPTQ) (`src/llm/model_quantization_pipeline.cpp`, `tests/test_model_quantization_pipeline.cpp`)

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Issue: #1938)
- [x] Integration tests (single-model and multi-model scenarios) (Issue: #1939)
- [x] Performance benchmarks (tokens/sec, latency p99) (Issue: #1940)
- [x] Security audit (prompt injection mitigation, API key handling) (Issue: #1941)
- [x] Documentation complete (Issue: #1942)
- [x] API stability guaranteed (Issue: #1943)
- [x] Build system audit: 16 previously-unregistered source files added to cmake/CMakeLists.txt and cmake/ModularBuild.cmake; all 151 src/llm/ files now registered (March 2026)
- [x] Test registration: all 28 tests/llm/ focused test targets added to tests/CMakeLists.txt via add_llm_focused_test macro (March 2026)
- [x] ActiveVRAMAllocator implemented (LLM-MISSING-001): real GPU allocation, OOM recovery (eviction/defrag/spill), 36 tests, benchmark (2026-03-11)
- [x] KV-cache prewarming with embedding-based lookup (LLM-MISSING-002): `prewarmCache()` stores real embeddings via `ILLMPlugin::embed()`, `checkCache()` / `updateCache()` use prompt-keyed HNSW similarity search, `PrefixCacheEntry::generated_text` returns actual cached response (2026-03-11)
- [x] InlineTrainingEngine implemented (`src/llm/inline_training_engine.cpp`): on-the-fly LoRA fine-tuning from RocksDB data, AdamW/Adam/SGD/Adagrad/RMSProp optimizers, Cosine/Linear/Polynomial LR schedulers, gradient accumulation, gradient clipping, JSON checkpointing with pruning, progress/checkpoint callbacks (2026-04-13)

## Known Issues & Limitations
- Cancellation is best-effort only; in-flight inference cannot be interrupted at llama.cpp level.
- Grammar-constrained generation depends on runtime API availability.

## Breaking Changes
- `InferenceHandle` header path changed in v1.15.0 (from `async_inference_engine.h` include to `inference_handle.h`).
- No further breaking changes planned for v1.x series.

### Phase 4: LLM+RAID Integration Tests (Status: Completed ✅)

- [x] Integration-Test: 3-Shard-Cluster (legal/medical/general) domain routing verification (LRIR-01)
- [x] Integration-Test: Batch-64 fan-out, 4 domains, result order preserved (LRIR-02)
- [x] Integration-Test: Circuit breaker OPEN during batch → throws (LRIR-03)
- [x] Integration-Test: Remote-Draft-Shard SpeculativeDecoder accept-rate telemetry (LRIR-04)
- [x] Integration-Test: Embedding locality — ShardingManager::GetShardForKey deterministic routing (LRIR-05)
- [x] Registered `test_llm_raid_integration_focused` (LRIR-01..05) in tests/CMakeLists.txt

### Phase 5: KV-Prefix Cross-Shard Transfer (Status: Completed ✅)

- [x] `RemoteExecutor::postBinary()` — binary payload transfer via base64-encoded JSON body
- [x] `IKVStateSerializer` + `NullKVStateSerializer` abstraction over llama_state_seq_save/load_file
- [x] `KVPrefixTransferManager` — orchestrates best-effort KV prefix transfer to target shard
- [x] `LLMAQLHandler::setKVPrefixTransferManager()` — dependency injection point
- [x] Auto-transfer in `executeInfer()` when domain routing selects remote shard + system_prompt ≥ 256 tokens
- [x] Registered `kv_prefix_transfer_manager.cpp` in cmake/CMakeLists.txt and cmake/ModularBuild.cmake
- [x] Thread-safe atomics for attempt/success counters
- Note: `NullKVStateSerializer` active by default (no TTFT savings until `KVStateSerializerLlama` links against llama.cpp — Target: Q2 2027)

### Phase 6: Documentation & Benchmarks (Status: Completed ✅)

- [x] `bench_llm_raid_pipeline.cpp` registered in benchmarks/CMakeLists.txt
- [x] `BM_DomainRouting_OverheadPerRequest` — routing decision latency benchmark (target ≤ 5 µs p99)
- [x] `BM_BatchFanOut_LatencyScaling` — batch sizes 1/8/16/32/64 fan-out benchmark
- [x] `docs/de/llm/CROSS_SHARD_INFERENCE_RUNBOOK.md` — operational runbook for cross-shard debugging
- [x] `docs/de/llm/AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md` — Phase 5 KV-Prefix + Phase 6 benchmark data added

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### 🧪 NUR_TESTS (implementiert, kein Produktions-Aufrufer)

- `ActiveVRAMAllocator` – Verwaltet aktive VRAM-Allokationen pro Inference-Session; Tests + Bench vorhanden
  > **Aktion:** ROADMAP-Ticket für Produktions-Integration ergänzen oder als CANDIDATE_FOR_REMOVAL markieren.

