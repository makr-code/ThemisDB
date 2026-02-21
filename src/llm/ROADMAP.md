# LLM Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
v1.15.0 – Production-ready dual-engine architecture. AsyncInferenceEngine and InferenceEngineEnhanced serve distinct use cases; shared InferenceHandle extracted to eliminate circular dependencies.

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

## In Progress 🚧
- [ ] Streaming token output (SSE / chunked response) (Target: Q2 2026)
- [ ] Per-request timeout and cancellation propagation (Target: Q2 2026)
- [ ] Unified metrics dashboard for both engines (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] OpenAI-compatible `/v1/chat/completions` passthrough adapter
- [ ] Function / tool calling support (JSON schema binding)
- [ ] Model hot-swap without engine restart
- [ ] Request deduplication cache (same prompt → cached response)
- [ ] Per-model resource quotas (memory, concurrency)

### Long-term (6-12 months)
- [ ] Speculative decoding for latency reduction
- [ ] Multi-modal input support (image + text)
- [ ] Federated inference across distributed nodes
- [ ] LoRA adapter hot-loading at inference time
- [ ] Model quantization pipeline integration (GGUF, AWQ, GPTQ)

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
- [~] Streaming token output via SSE / chunked responses (`llm/streaming_handler.cpp`) (Target: Q2 2026)
- [~] Shared worker pool between AsyncInferenceEngine and InferenceEngineEnhanced (Target: Q2 2026)
- [ ] Per-request timeout and cancellation propagation (Target: Q2 2026)
- [ ] Unified metrics dashboard for both engines (Target: Q3 2026)

### Phase 3: Ecosystem & Performance (Status: Planned 📋)
- [ ] OpenAI-compatible `/v1/chat/completions` REST adapter
- [ ] Speculative decoding for latency reduction
- [ ] LoRA adapter hot-loading at inference time (`llm/adapter_registry.cpp`)
- [ ] Multi-model routing based on prompt content or metadata tags
- [ ] Model quantization pipeline integration (GGUF, AWQ, GPTQ)

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests (single-model and multi-model scenarios)
- [ ] Performance benchmarks (tokens/sec, latency p99)
- [ ] Security audit (prompt injection mitigation, API key handling)
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- Both engines maintain independent worker threads; no shared thread pool yet.
- Cancellation is best-effort only; in-flight inference cannot be interrupted at llama.cpp level.
- Grammar-constrained generation depends on runtime API availability.

## Breaking Changes
- `InferenceHandle` header path changed in v1.15.0 (from `async_inference_engine.h` include to `inference_handle.h`).
- No further breaking changes planned for v1.x series.
