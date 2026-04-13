# LLM Module - Future Header Enhancements

<!-- Status: current | validated: 2026-04-09 | Primary: include/llm/ | Secondary: docs/de/llm/ -->
<!-- Links: ../../src/llm/FUTURE_ENHANCEMENTS.md · ../../src/llm/README.md · ../../src/llm/ROADMAP.md -->

## Scope

The following interface enhancements were planned in this document. Most have been implemented in v1.15.0–v1.16.0:

- `IInferenceEngine::submitStreaming()` — **implemented** in `async_inference_engine.h` / `inference_engine_enhanced.h` (`StreamingCallback` = `std::function<void(std::string_view, bool)>`)
- OpenAI-compatible chat completions adapter — **implemented** in `include/llm/openai_compat_adapter.h` + `src/llm/openai_compat_adapter.cpp`
- LoRA adapter hot-swap — **implemented** via `InferenceEngineEnhanced::loadLoRAAdapter()` / `unloadLoRAAdapter()` (`include/llm/inference_engine_enhanced.h`)
- Shared worker pool — **implemented** in `include/llm/shared_worker_pool.h` + `src/llm/shared_worker_pool.cpp`
- Adapter load balancer — **implemented** in `include/llm/adapter_load_balancer.h`

Remaining planned work: see `src/llm/FUTURE_ENHANCEMENTS.md` (federated inference, Issue #1928).

## Design Constraints

- [x] Streaming interface uses a callback (`StreamingCallback`); pull-based `IStreamingTokenIterator` remains an alternative design considered but not adopted
- [x] OpenAI compatibility layer is opt-in via `THEMIS_ENABLE_OPENAI_COMPAT`; default builds do not expose it
- [x] LoRA hot-swap is atomic: the swap completes between generation steps and no mid-generation adapter change is permitted
- [x] Adapter registry is thread-safe for concurrent reads; writes are serialized via internal locking
- [x] Worker pool configuration is injected at engine construction time; reconfiguration requires a new engine instance
- [x] `AdapterLoadBalancer` selection policy is pluggable

## Required Interfaces

| Interface | Consumer | Status |
|---|---|---|
| `IInferenceEngine::submitStreaming()` | HTTP SSE writer, OpenAI compat adapter | ✅ Implemented (`async_inference_engine.h`, `inference_engine_enhanced.h`) |
| `OpenAICompatAdapter` | OpenAI-compatible REST endpoint | ✅ Implemented (`openai_compat_adapter.h`) |
| `InferenceEngineEnhanced::loadLoRAAdapter()` | Fine-tune deployment, A/B testing | ✅ Implemented (`inference_engine_enhanced.h`) |
| `AdapterLoadBalancer` | Multi-instance inference dispatcher | ✅ Implemented (`adapter_load_balancer.h`) |
| `SharedWorkerPool` | Inference thread pool | ✅ Implemented (`shared_worker_pool.h`) |

## Implemented Features

### Streaming Token Output Interface ✅

- [x] `IInferenceEngine::submitStreaming(request, TokenCallback)` defined in both engines
- [x] `TokenCallback` is `std::function<void(std::string_view token, bool is_final)>`
- [x] `cancel()` is non-blocking via shared atomic cancel token; generation stops at the next token boundary
- [x] Returns an `InferenceHandle` from `submitStreaming()` for cancellation

### OpenAI-Compatible Adapter API ✅

- [x] `OpenAICompatAdapter` implemented (`openai_compat_adapter.h` / `.cpp`)
- [x] Maps `POST /v1/chat/completions` JSON body including `messages`, `temperature`, `max_tokens`, `stream`, `stop`, `tools`
- [x] `stream=true` routes to `submitStreaming()` and emits SSE chunks
- [x] Guarded by `THEMIS_ENABLE_OPENAI_COMPAT`

### LoRA Adapter Hot-Swap Interface ✅

- [x] `InferenceEngineEnhanced::loadLoRAAdapter(adapter_id, weights_path, metadata)` implemented
- [x] `unloadLoRAAdapter(adapter_id, wait_for_completion)` implemented
- [x] `getLoadedLoRAAdapters()` returns metadata for all registered adapters
- [x] Swap is atomic between generation steps

### Shared Worker Pool ✅

- [x] `SharedWorkerPool` implemented in `shared_worker_pool.h` / `.cpp`
- [x] Work-stealing thread pool shared between `AsyncInferenceEngine` and `InferenceEngineEnhanced`

### Speculative Decoding Hook ✅

- [x] `SpeculativeDecoder` implemented in `speculative_decoder.h` / `.cpp`

## Test Strategy

- Unit-test streaming with a mock adapter: verify token ordering, `is_final=true` termination, and `cancel()` stopping generation (tests/llm/test_streaming_handler.cpp)
- Test `OpenAICompatAdapter` request/response mapping (tests/llm/test_openai_compat_adapter.cpp)
- Integration-test LoRA hot-loading: load adapter, begin generation, unload (tests/llm/test_lora_hot_loading.cpp)
- Compile-flag test: build with and without `THEMIS_ENABLE_OPENAI_COMPAT`

## Performance Targets

- Token streaming first-token latency ≤ 200 ms on a 7B-parameter model on A10G
- LoRA adapter load (`loadLoRAAdapter()`) ≤ 5 s for rank-64 adapter
- Adapter selection (`AdapterLoadBalancer`) ≤ 100 µs
- Worker pool dispatch overhead ≤ 50 µs p99

## Security / Reliability

- LLM input prompts are sanitized for prompt-injection patterns via `llm_security_utils.h` before dispatch
- Model weights are never exposed through any public interface method
- `loadLoRAAdapter()` accepts only paths within the configured `adapter_load_dir`
- Inference output is filtered for PII before tokens are yielded by streaming callbacks; filtering is configurable but enabled by default
- `AdapterLoadBalancer` enforces a maximum queue depth per adapter; requests exceeding the limit return an error rather than blocking indefinitely
