# LLM Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.15.0 – Production-ready dual-engine architecture. AsyncInferenceEngine and InferenceEngineEnhanced serve distinct use cases; shared InferenceHandle extracted to eliminate circular dependencies.
Per-request timeout and cancellation propagation is fully implemented across both engines via shared atomic cancel tokens and dedicated timeout monitor threads.

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
- [x] Per-request timeout and cancellation propagation (Issue: #2411)

## In Progress 🚧
- [I] Streaming token output (SSE / chunked response) (Target: Q2 2026) (Issue: #1918)
- [I] Unified metrics dashboard for both engines (Target: Q3 2026) (Issue: #1932)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] OpenAI-compatible `/v1/chat/completions` passthrough adapter (Issue: #1921)
- [I] Function / tool calling support (JSON schema binding) (Issue: #1922)
- [I] Model hot-swap without engine restart (Issue: #1923)
- [I] Request deduplication cache (same prompt → cached response) (Issue: #1924)
- [I] Per-model resource quotas (memory, concurrency) (Issue: #1925)

### Long-term (6-12 months)
- [I] Speculative decoding for latency reduction (Issue: #1934)
- [I] Multi-modal input support (image + text) (Issue: #1927)
- [I] Federated inference across distributed nodes (Issue: #1928)
- [I] LoRA adapter hot-loading at inference time (Issue: #1929)
- [!] Model quantization pipeline integration (GGUF, AWQ, GPTQ) (Issue: #2412)

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

### Phase 2: Streaming & Shared Worker Pool (Status: In Progress 🚧)
- [I] Streaming token output via SSE / chunked responses (`llm/streaming_handler.cpp`) (Target: Q2 2026) (Issue: #1944)
- [I] Shared worker pool between AsyncInferenceEngine and InferenceEngineEnhanced (Target: Q2 2026) (Issue: #1945)
- [x] Per-request timeout and cancellation propagation (Target: Q2 2026)
- [ ] Unified metrics dashboard for both engines (Target: Q3 2026)

### Phase 3: Ecosystem & Performance (Status: Planned 📋)
- [I] OpenAI-compatible `/v1/chat/completions` REST adapter (Issue: #1933)
- [ ] Speculative decoding for latency reduction
- [I] LoRA adapter hot-loading at inference time (`llm/adapter_registry.cpp`) (Issue: #1935)
- [I] Multi-model routing based on prompt content or metadata tags (Issue: #1936)
- [ ] Model quantization pipeline integration (GGUF, AWQ, GPTQ)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1938)
- [I] Integration tests (single-model and multi-model scenarios) (Issue: #1939)
- [I] Performance benchmarks (tokens/sec, latency p99) (Issue: #1940)
- [I] Security audit (prompt injection mitigation, API key handling) (Issue: #1941)
- [I] Documentation complete (Issue: #1942)
- [I] API stability guaranteed (Issue: #1943)

## Known Issues & Limitations
- Both engines maintain independent worker threads; no shared thread pool yet.
- Cancellation is best-effort only; in-flight inference cannot be interrupted at llama.cpp level.
- Grammar-constrained generation depends on runtime API availability.

## Breaking Changes
- `InferenceHandle` header path changed in v1.15.0 (from `async_inference_engine.h` include to `inference_handle.h`).
- No further breaking changes planned for v1.x series.
