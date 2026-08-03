# LLM Module Contract

**Datum:** 2026-08-03  
**Status:** Active  
**Module:** llm (LLM inference, model switching, context mgmt)  
**Primary:** include/llm/inference_engine.h, src/llm/ROADMAP.md

## Public API Surface

| API | Namespace | Input Contract | Output Contract | Errors | Thread-Safety | Ownership/Lifetime | Notes |
|---|---|---|---|---|---|---|---|
| `createInferenceEngine()` | `themis::llm` | Backend type (ONNX/Llama/HF), model spec | std::unique_ptr<IInferenceEngine> | ModelNotFoundError, InitError if GPU unavailable | 🔒 Single-threaded factory call | Caller owns returned pointer; valid for lifetime of use | P0; Phase 2 complete |
| `embed()` | `themis::llm::IInferenceEngine` | Text (1-4K UTF-8), options (timeout, batch-size) | std::vector<float> (embedding, dim 384/768/1024 per model) | InputValidationError, TimeoutError, OOMError | ✅ Multi-call-thread-safe (one engine per thread recommended) | Embedding owned by return; caller may modify | P0; GATE-LLM-01 ≤100µs |
| `embedBatch()` | `themis::llm::IInferenceEngine` | Vec<string> (1-1K items), batch options | Vec<Embedding> (same order as input), may be cached | TimeoutError (batch-level timeout), InputError | ✅ Thread-safe (serialized via internal queue) | Embeddings borrowed; valid for ~1min or next call | P0; GATE-LLM-02 ≤50µs/item |
| `generateText()` | `themis::llm::IInferenceEngine` | Prompt (UTF-8, ≤8K tokens), generation config | std::string (generated text, at most max_tokens) | GenerationTimeout, ContextLimitExceeded, HallucinationFilter triggered | ✅ Thread-safe (async queue internal) | Text owned by return; immutable copy safe | P1 (slower path); defaults 30s timeout |
| `switchModel()` | `themis::llm::IModelRouter` | Target model name (registered via registry) | void (engine reloads in background, serves old model during reload) | ModelNotRegisteredError | 🔒 Single-writer per router instance | N/A (mutation); no references affected | Internal; P2 (rare admin op) |
| `getModelInfo()` | `themis::llm::IModelRouter` | Model name (or null for current) | ModelMetadata (dim, context_size, supported_tasks) | NotFoundError | ✅ Lock-free (immutable metadata) | Metadata borrowed; valid for app lifetime | P1; utility |
| `modelRegistry()` | `themis::llm` | N/A (accessor) | IModelRegistry* (borrowed, global) | None | ✅ Lock-free accessor | Global registry; append-only after init | Internal API |
| `contextManager()` | `themis::llm::IInferenceEngine` | N/A (accessor) | IContextManager* (borrowed) | None | ✅ Provides ref to context controller | Borrowed ref; valid for engine lifetime | Internal; context is stateful |
| `cancel()` | `themis::llm::IInferenceEngine` | CancellationToken (or call without param) | void (inference aborted, partial result may be available) | None | ✅ Thread-safe (signal-based) | N/A (cancellation only) | P0 (interrupt path) |

## Model Registration & Switching

| Operation | Contract | Notes |
|---|---|---|
| Register model | Done at startup via registry; immutable after | registry.register("gpt-4-turbo", spec) |
| List models | Returns Vec<ModelMetadata>, read-only | registry.listModels() |
| Switch model | Async; old model continues serving until switch ready | router.switchModel("qwen2.5") |
| Query current | Returns name of model currently serving | router.currentModel() |

## Embedding Invariants

| Invariant | Enforcement |
|---|---|
| Same text → same embedding (deterministic) | Verified by test_embedded_llm_batch_contract.cpp |
| Dimension matches model spec | Runtime check; returns ErrorDimMismatch if cached embedding has wrong dim |
| Batch order preserved | Unit test: test_llm_embedding_order_preservation.cpp |
| Batch fallback: embedBatch() → N×embed() | Verified for EmbeddedLLM stub (Phase 2 complete) |

## Concurrency & Threading Model

| Scenario | Behavior | Test |
|---|---|---|
| 8 threads calling embed() concurrently | All serve from same CUDA context via queuing; ≤100µs latency P95 | test_llm_concurrent_inference.cpp |
| generateText() + embed() simultaneous | Serialized (context not multi-callable); embed prioritized | test_llm_mixed_workload.cpp |
| Model switch during inference | In-flight requests finish with old model; new requests use new | integration test: test_llm_model_switch_during_load.cpp |

## Performance Commitments (Release Gates)

| Gate | Latency | Model | Test |
|---|---|---|---|
| GATE-LLM-01 | embed() ≤100 µs | Sentence-BERT 384D | bench_llm_inference_gates.cpp |
| GATE-LLM-02 | embedBatch() ≤50 µs/item | Via GPU batching | bench_llm_batch_inference.cpp |
| GATE-LLM-03 | generateText() ≤30s P95 | GPT-2 small (100 tokens) | bench_llm_generation.cpp |
| GATE-LLM-04 | modelSwitch prep ≤5s | Model load time + CUDA warmup | bench_llm_model_switch.cpp |

## Error Categories

| Error | When | Recovery |
|---|---|---|
| ModelNotFoundError | Model name not in registry | Check registry; register model first |
| TimeoutError | embed() exceeds timeout_ms (default 30s) | Increase timeout or use smaller batch |
| HallucinationFilterTriggered | Generated text violates safety policy | Retry with different seed/temp or different model |
| ContextLimitExceeded | Prompt + generation would exceed max_tokens | Truncate prompt or reduce max_tokens |
| OOMError | GPU/CPU memory exhausted | Reduce batch size or model dim; fallback to CPU if available |

## API Stability

| Item | Status | Stability |
|---|---|---|
| IInferenceEngine interface | Public v1.x | Frozen (breaking change = major version) |
| embed() signature | Public v1.x | Stable; may add optional params with defaults |
| Model registry format | Internal | Fluid; may change between patch versions |
| generateText() | Beta (internal) | Will finalize in Q4 2026 |

---

**Zuletzt geprueft (LLM contracts):** 2026-08-03
