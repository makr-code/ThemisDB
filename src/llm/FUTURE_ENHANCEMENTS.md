<!-- Status: current | validated: 2026-04-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/llm/ -->

# LLM Module - Future Enhancements
<!-- Links: README.md · ROADMAP.md · ARCHITECTURE.md · ../../include/llm/FUTURE_ENHANCEMENTS.md -->

## Scope

This document covers planned enhancements to the LLM module beyond what is tracked in `ROADMAP.md`. It focuses on `async_inference_engine.cpp`, `inference_engine_enhanced.cpp`, `inference_handle.cpp`, and the surrounding components (`adapter_registry.cpp`, `continuous_batch_scheduler.cpp`, `kv_cache_buffer.cpp`, `grammar.cpp`, `adaptive_vram_allocator.cpp`). The following features from the initial enhancement list are now **complete**: streaming token output (SSE/chunked response), OpenAI-compatible API passthrough, speculative decoding, shared thread pool unification between both engines. The remaining planned work covers **federated inference across distributed nodes** (Issue: #1928), which requires multi-node coordination beyond the current single-node multi-GPU implementation.

## Design Constraints

- `AsyncInferenceEngine` and `InferenceEngineEnhanced` must remain independent implementations with distinct use cases; shared infrastructure (thread pool, `InferenceHandle`, metrics) is extracted to shared headers/translation units rather than merged into a single class.
- `InferenceHandle` (defined in `include/llm/inference_handle.h`) is the stable ABI between callers and both engines; its `get()`, `ready()`, and `cancel()` semantics must not change in a breaking way.
- Grammar-constrained generation (`grammar.cpp`, `llama_grammar_adapter.cpp`) must degrade gracefully when the llama.cpp grammar API is absent; no enhancement may introduce a hard dependency on grammar APIs in the non-grammar code path.
- All inference requests must pass through `PolicyEngine::checkInferencePermission()` before being queued; no engine enhancement may introduce a bypass path.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `IInferenceEngine::submitStreaming(request, token_callback)` | OpenAI-compatible API handler, SSE endpoint | New interface; both engines must implement it to support streaming output |
| `SharedWorkerPool::submit(task, priority)` | `AsyncInferenceEngine`, `InferenceEngineEnhanced` | New shared thread pool replacing per-engine `std::thread` workers |
| `AdapterRegistry::hotSwap(model_id, new_weights_path)` | Admin API, `InferenceEngineEnhanced` | Extends `adapter_registry.cpp`; swap LoRA adapter without engine restart |
| `KvCacheBuffer::evict(policy)` | `InferenceEngineEnhanced`, `continuous_batch_scheduler.cpp` | Extend `kv_cache_buffer.cpp` with LRU and prefix-aware eviction policies |
| `SpeculativeDecoder::verify(draft_tokens, target_logits)` | `InferenceEngineEnhanced` draft-model path | New class; implements speculative decoding acceptance/rejection loop |

## Planned Features

### RAID-Sharding Interlock for Cross-Instance (Batch-)Inference
**Priority:** High  
**Target Version:** v1.18.0  
**Status:** [~] In progress (API contract + metadata wiring)

#### Scope
- Coordinate inference and continuous batch inference across multiple ThemisDB shard instances.
- Preserve shard routing context (`routing_key`, `target_instance_ids`) end-to-end from API layer to model plugin call.
- Add explicit request hints for cross-instance batch orchestration without changing single-node default behavior.

#### Design Constraints
- Keep `InferenceEngineEnhanced` backward compatible for local/single-instance inference.
- Treat shard hints as optional metadata (no hard dependency on RAID coordinator availability).
- Never bypass existing policy checks or per-request cancellation semantics.

#### Required Interfaces
| Interface | Consumer | Notes |
|---|---|---|
| `EnhancedInferenceRequest::shard_routing_key` | RAID shard router / coordinator | Stable hint for deterministic shard placement |
| `EnhancedInferenceRequest::target_instance_ids` | Federated inference coordinator | Explicit shard subset for fan-out/fan-in |
| `EnhancedInferenceRequest::allow_cross_instance_batching` | Distributed batch scheduler | Opt-in guard for multi-instance co-batching |
| `InferenceRequest::metadata["raid_sharding"]` | Plugins / transport layer | Canonical handoff envelope for downstream orchestration |

#### Implementation Notes
- [x] Added shard-routing and cross-instance batching hints to `EnhancedInferenceRequest`.
- [x] Forwarded RAID hint envelope into `InferenceRequest::metadata["raid_sharding"]` before plugin generation call.
- [ ] Add coordinator-side fan-out/fan-in execution path using these hints (Issue: #1928).
- [ ] Add per-shard partial-failure handling + adaptive retry gates for distributed batches.

#### Test Strategy
- [x] Unit test validating RAID hint forwarding (`tests/test_inference_engine_enhanced.cpp`: `RaidShardingHintsAreForwardedToRequestMetadata`).
- [ ] Integration test with 1/4/8 shard instances validating deterministic routing and aggregated responses.
- [ ] Failure-injection tests for partial shard outages during cross-instance batch execution.

#### Performance Targets
- Cross-instance batch scheduling overhead ≤ 5 ms p95 per coordinator cycle.
- Throughput improvement ≥ 20% at 8 concurrent requests compared with non-batched distributed baseline.

#### Security / Reliability
- Routing hints are metadata-only and do not override access-control or tenant isolation checks.
- Partial shard failures must degrade to bounded retries and explicit error signaling (no silent data loss).

### `LoraSecurityValidator`: Certificate Store Integration
**Priority:** High
**Target Version:** v1.8.0
**Status:** ✅ Implemented

`lora_security_validator.cpp` had 2 critical TODOs for certificate retrieval (lines 249 and 365).

**Implementation Notes:**
- `[x]` Implemented `LoRACertificateStore` class backed by a filesystem path (`config/security/lora_certs/`).
- `[x]` In `verifySignature()`, look up the certificate by fingerprint from `LoRACertificateStore`; fail closed if the certificate is not found (returns `SIGNATURE_UNVERIFIABLE` error, not success).
- `[x]` In `verifyEmbeddedSignature()`, look up the certificate from the store if not embedded in metadata; fail closed if not found.
- `[x]` Wired integration with the system certificate store (`/etc/ssl/certs` on Linux) as a fallback after the local `LoRACertificateStore`.
- `[x]` Added unit tests: missing cert → verification fails; valid cert + valid sig → passes; valid cert + tampered sig → fails.

---

### `LLMDeploymentPlugin`: RocksDB Model Storage
**Priority:** Medium
**Target Version:** v1.8.0

`llm_deployment_plugin.cpp` line 273 has: "Store in BaseEntity storage (RocksDB) - TODO: Uncomment when `llm_model_storage.cpp` exists". The plugin currently operates in "Filesystem-only mode" (line 136). Model metadata is not persisted to the database, breaking admin query ("list all deployed models") across restarts.

**Implementation Notes:**
- `[x]` Implement `llm_model_storage.cpp` providing `LLMModelStorage::save(model_id, metadata)` and `load(model_id)` using the existing `StorageEngine` API with key prefix `llm_model::`.
- `[x]` Uncomment the RocksDB persistence block at line 273; inject `StorageEngine*` into `LLMDeploymentPlugin` constructor.
- `[x]` Implement `TODO(enhancement)` at line 916: check `model_id` existence before deployment to surface clear errors for unknown model IDs.
- `[x]` Implement `TODO(feature)` at line 175: propagate authenticated user context from the request JWT into `audit.user` instead of hardcoding `"system"`.

---

### `AIOrchestrator`: Tool Call Parsing
**Priority:** Medium
**Target Version:** v1.8.0

`ai_orchestrator.cpp` line 494 has: "TODO(extensible): parse tool calls from `result.text` using `react_agent`". Without tool call parsing, the orchestrator cannot dispatch function calls returned by the LLM, making the ReAct loop incomplete.

**Implementation Notes:**
- `[x]` Add tool call extraction to `AIOrchestrator::runAgentic()`: parse the structured JSON block `{"name":"<tool>","arguments":{...}}` from `result.text` using `nlohmann::json`; dispatch to the registered tool via `ToolRegistry::invokeTool()`.
- `[x]` Handle malformed tool call JSON gracefully: log a warning and continue with the raw text result rather than crashing.

---

### Streaming Token Output (SSE / Chunked Response)
**Priority:** High  
**Target Version:** v1.7.0  
#### Scope
- Deliver OpenAI-style streaming for LLM responses via SSE framing and HTTP chunked responses.
- Expose token-level callbacks through `InferenceRequest::stream_callback` for both engines while keeping engines output-format agnostic.
- Provide reusable formatting helpers in `llm::StreamingHandler` for SSE events, `[DONE]` sentinel, and chunked-transfer frames.

#### Design Constraints
- SSE payloads must be valid JSON per RFC 8259 with control-character escaping; framing must end with `\n\n`.
- On normal completion, emit the canonical `data: [DONE]\n\n` sentinel; chunked responses end with the zero-length chunk `0\r\n\r\n`.
- If cancellation interrupts the callback before completion or a network/client disconnect prevents the final frame, producers cannot send the terminal marker.
- Consumers/clients must tolerate its absence (see Phase 3: consumer-tolerance task):
  - Treat end-of-stream without the marker as an incomplete stream.
  - Surface a retriable `stream_incomplete` warning/error within 500 ms of detecting EOF without the marker.
  - Avoid serving or caching partial responses.
- Streaming callbacks run on worker threads and must respect cancellation/deadlines before emitting tokens.
- Deduplication caching must be skipped for streaming requests to avoid serving partial cached content.

#### Required Interfaces
| Interface | Consumer | Notes |
|-----------|----------|-------|
| `InferenceRequest::stream_callback` | `AsyncInferenceEngine`, `InferenceEngineEnhanced`, HTTP SSE writers | Serial invocation on the producing worker thread; sink must be thread-safe when sharing state. |
| `llm::StreamingHandler::{formatSseEvent, formatDoneEvent, formatChunkedData, makeStreamCallback}` | HTTP layer (SSE endpoints, OpenAI compat adapter) | Static, reentrant helpers; atomic index for single-producer streams. |
| `AsyncInferenceEngine::submitStreaming(InferenceRequest, TokenCallback)` | HTTP SSE layer, interactive chat endpoints | Returns `InferenceHandle`; `TokenCallback` is `std::function<void(std::string_view, bool)>`. |
| `InferenceEngineEnhanced::submitStreaming(EnhancedInferenceRequest, TokenCallback)` | HTTP SSE layer, batch coordinator | Same `TokenCallback` contract; integrates with batch scheduler. |

#### Implementation Phases
- **Phase 1 — Design / API Contract**
  - [x] Expose `InferenceRequest::stream_callback` (`include/llm/llm_plugin_interface.h`) as `std::function<void(const std::string&)>`, invoked serially on the worker thread; sinks must be thread-safe when sharing state and must handle abrupt stop (no further callbacks, possibly without a terminal marker) without throwing.
  - [x] Define SSE/chunked framing surface via `StreamingHandler` (JSON escaping, `[DONE]` sentinel, zero-length terminal chunk) to keep engines output-format agnostic.
  - [x] Add `TokenCallback = std::function<void(std::string_view token, bool is_final)>` to both engine classes; add `submitStreaming()` declarations.
- **Phase 2 — Core Implementation**
  - [x] Wrap `stream_callback` in `AsyncInferenceEngine::processRequest()` (see `async_inference_engine.cpp`) using the shared `cancel_token` and deadline guard before forwarding tokens; partial sequences are dropped once the guard trips.
  - [x] Provide `StreamingHandler::{formatSseEvent, formatChunkedData, makeStreamCallback}` in `src/llm/streaming_handler.cpp`; `makeStreamCallback()` returns an atomic-indexed lambda for single-producer streams, verified to keep indices monotonic and to tolerate empty token strings without emitting invalid SSE frames.
  - [x] Implement `AsyncInferenceEngine::submitStreaming()` — wraps `TokenCallback` into the existing `stream_callback` + completion-callback pattern; fires `is_final=true` exactly once (normal or cancellation path).
  - [x] Implement `InferenceEngineEnhanced::submitStreaming()` — same pattern integrated with the batch scheduler; cancel propagation via shared `cancel_token`.
- **Phase 3 — Error Handling & Edge Cases**
  - [x] Drop token emission when cancellation/deadlines trigger. On graceful completion producers still emit well-formed terminal `[DONE]`/zero-length chunk markers.
  - [ ] Ensure consumers tolerate missing markers when the transport aborts mid-stream (e.g., client disconnect, network failure, or server-side cancellation during write).
    - Scope: HTTP SSE writer/reader in the OpenAI-compatible adapter and chunked-response parsers used by SDK clients.
    - Expected behavior:
      - Detect EOF without a terminal marker (per design constraint).
      - Flag a retriable `stream_incomplete` error.
      - Drop partial responses from dedup caches.
      - Log a warning with request_id for observability.
    - Verification: `tests/llm/test_streaming_handler.cpp` and `tests/test_llm_timeout_cancellation.cpp` assert detection and error surfacing under injected disconnects once the RocksDB dependency is resolved in the CI/sandbox build environment.
  - [x] JSON-escape control characters in SSE payloads to prevent malformed event streams.
- **Phase 4 — Tests**
  - [x] `tests/test_inference_engine_enhanced.cpp` — `SubmitStreaming_TokensDelivered` and `SubmitStreaming_CancelFiresFinalSentinel` for `InferenceEngineEnhanced`; `AsyncInferenceEngineStreamingTest.SubmitStreaming_TokensAndFinalSentinel` and `AsyncInferenceEngineStreamingTest.SubmitStreaming_CancelFiresFinalSentinel` for `AsyncInferenceEngine`.
  - [I] `tests/llm/test_streaming_handler.cpp` validates SSE framing, JSON escaping, chunked frames, and callback index sequencing (Blocked: themis_tests build currently fails in unrelated `llm_deployment_plugin.cpp` incomplete type error).
  - [I] `tests/test_llm_timeout_cancellation.cpp` exercises streaming cancellation/deadline paths on mock plugins (Blocked: same build failure prevents running suite).
- **Phase 5 — Performance / Hardening**
  - [x] Bypass `DeduplicationCache` for streaming requests in `async_inference_engine.cpp` by checking `effective_request.stream_callback` before cache lookups to keep TTFT ≤ 200 ms p99 for ≤ 512-token prompts and to avoid stale partial responses.
  - [x] Pre-reserve SSE payload buffers and reuse atomic counters in `StreamingHandler` to keep streaming overhead ≤ 2% tokens/sec regression versus non-streaming.
- **Phase 6 — Documentation & Acceptance**
  - [x] Document SSE/chunked streaming behavior and roadmap status here; align ROADMAP anchor `streaming-token-output-sse--chunked-response`.
  - [x] Ensure OpenAI-compatible adapter and HTTP SSE surfaces consume `StreamingHandler` helpers for consistent wire format.

#### Test Strategy
- [x] `tests/test_inference_engine_enhanced.cpp` covers `submitStreaming()` for both `InferenceEngineEnhanced` (2 tests) and `AsyncInferenceEngine` (2 tests): token delivery and cancel-fires-final-sentinel.
- [I] `tests/llm/test_streaming_handler.cpp` exists and exercises SSE formatting, JSON escaping, chunked frames, and callback index sequencing (execution blocked by current themis_tests build failure in `llm_deployment_plugin.cpp`).
- [I] `tests/test_llm_timeout_cancellation.cpp` exists and covers streaming callbacks under cancellation/deadline pressure (execution blocked by same build failure).
- [x] OpenAI-compatible adapter streaming paths rely on the same SSE helpers; streaming fixture tests exercise the shared framing surface.

#### Performance Targets
- Time-to-first-token (TTFT) ≤ 200 ms p99 for prompt lengths ≤ 512 tokens on a single A10G GPU.
- Streaming overhead (vs non-streaming) ≤ 2 % of total tokens/sec throughput.

#### Security / Reliability
- Streamed tokens are JSON-escaped to prevent response-body injection in SSE consumers.
- Cancellation/deadline guards prevent runaway streaming after client disconnects; terminal markers are emitted on graceful completion and treated as best-effort when transports abort mid-stream.
- Prompt-policy enforcement still runs before streaming; blocked prompts return policy errors without invoking callbacks.

#### Known Issues & Limitations
- [I] Consumer tolerance verification (Phase 3) is blocked in the current CI/sandbox build environment because the LLM test suite cannot build without the RocksDB dependency.

### OpenAI-Compatible `/v1/chat/completions` Adapter
**Priority:** High
**Target Version:** v1.7.0
**Status:** ✅ Implemented (v1.7.0)

`openai_compat_adapter.cpp` / `openai_compat_adapter.h` implement the full adapter. `PolicyEngine::checkInferencePermission()` (added to `include/governance/policy_engine.h` + `src/governance/policy_engine.cpp`) provides API key validation. `LLMApiHandler::setPolicyEngine()` (added to `include/server/llm_api_handler.h` + `src/server/llm_api_handler.cpp`) wires the policy check into `handleOpenAIChatCompletions()`. OpenAI-compatible routes (`/v1/chat/completions`, `/v1/models`) are dispatched **before** the JWT gate so that OpenAI SDK clients (which send plain API keys, not JWTs) are not rejected by `validateBearerToken()`.

**Implementation Notes:**
- ✅ `openai_compat_adapter.cpp` in `src/llm/`; parses `POST /v1/chat/completions` JSON body into `InferenceRequest` including `messages`, `temperature`, `max_tokens`, `stream`, `stop`, and `tools` fields.
- ✅ Maps the `messages` array to the internal prompt format; supports `system`, `user`, and `assistant` roles.
- ✅ `stream: true` routes to `IInferenceEngine::submitStreaming()` and emits SSE chunks in the `data: {"choices":[{"delta":{"content":"..."}}]}` format.
- ✅ Function/tool calling: tools serialized to `InferenceRequest::tools`; grammar constraints applied via `JsonSchemaConverter::toolsToEbnf()` in `llama_wrapper.cpp`; `tool_calls` returned in response.
- ✅ `PolicyEngine::checkInferencePermission()` added and wired into `handleOpenAIChatCompletions()` via `LLMApiHandler::setPolicyEngine()`. HTTP 401 for missing/malformed `Authorization: Bearer` header; HTTP 403 when `ann_allowed=false` (strict classification). OpenAI-compat routes bypass JWT so that plain API keys work.
- ✅ Adapter round-trip benchmark (`BM_OpenAICompatAdapter_RoundTrip`) added to `benchmarks/llm_bench.cpp` with p99 counter asserting ≤ 2 ms overhead.

**Performance Targets:**
- Non-streaming request overhead (adapter serialization/deserialization) ≤ 2 ms vs direct `submitRequest()` call. ✅ Verified via `BM_OpenAICompatAdapter_RoundTrip` benchmark.
- Compatible with OpenAI SDK smoke tests: `openai.ChatCompletion.create()` with `stream=False` and `stream=True` must both succeed against a local ThemisDB instance. ✅ Wire format validated in `StreamChunkTest` and `BuildResponseTest`.

---

### Shared Worker Thread Pool
**Priority:** Medium
**Target Version:** v1.8.0

Replace the independent `std::thread` worker arrays in `async_inference_engine.cpp` and `inference_engine_enhanced.cpp` with a shared `SharedWorkerPool` that both engines submit tasks to. Currently both engines spin up separate thread pools; on a machine running both engines simultaneously (e.g., simple API requests via `AsyncInferenceEngine` and RAG requests via `InferenceEngineEnhanced`), threads compete for CPU cores.

**Implementation Notes:**
- Add `shared_worker_pool.cpp` in `src/llm/`; implement a work-stealing thread pool based on `std::deque<Task>` per thread with lock-free stealing.
- Thread count defaults to `std::thread::hardware_concurrency()`; configurable via `LlmConfig::worker_threads`.
- Both engines submit `Task` objects (priority, callable, `InferenceHandle*` for completion signaling) to `SharedWorkerPool::submit()`.
- Priority queue within `SharedWorkerPool` preserves the existing priority semantics of `InferenceEngineEnhanced` while giving `AsyncInferenceEngine` tasks a default medium priority.
- Add `llm_worker_pool_queue_depth` Prometheus gauge and `llm_worker_pool_tasks_completed_total` counter in `grafana_metrics.cpp`.

**Performance Targets:**
- CPU utilization improvement: ≥ 10 % higher GPU utilization on mixed `AsyncInferenceEngine` + `InferenceEngineEnhanced` workloads (measured with `nvidia-smi dmon`).
- Work-stealing pool task dispatch latency ≤ 50 µs p99 from `submit()` to worker thread pickup.

---

### Speculative Decoding for Latency Reduction
**Priority:** Medium
**Target Version:** v1.9.0

Implement speculative decoding in `InferenceEngineEnhanced` to reduce latency for long-response requests. A small draft model generates candidate tokens speculatively; the target model verifies them in a single forward pass. On acceptance, multiple tokens advance per step; on rejection, the target model's token is used.

**Implementation Notes:**
- Add `speculative_decoder.cpp` with `SpeculativeDecoder::verify(draft_tokens, target_logits)` implementing the acceptance criterion from the Leviathan et al. 2023 paper.
- Draft model is registered in `adapter_registry.cpp` as a `DRAFT` role adapter; `InferenceEngineEnhanced` selects a draft model based on the target model's family.
- `adaptive_vram_allocator.cpp` must be updated to reserve VRAM for both the target and draft model simultaneously; the draft model is quantized to INT4 by default to minimize VRAM footprint.
- Add a `speculative_k` config parameter (number of draft tokens per step, default 4); expose via `LlmConfig::speculative_draft_tokens`.
- Disable speculative decoding automatically if grammar constraints are active (grammar state cannot be efficiently speculated); log a debug-level notice when this occurs.

**Performance Targets:**
- ≥ 2× tokens/sec improvement for long responses (≥ 200 tokens) on text-generation tasks with a 7B target model + 0.5B draft model on an A10G.
- Speculative decoding overhead (rejected tokens) ≤ 15 % of accepted token latency on typical chat prompts.

---

### LoRA Adapter Hot-Loading at Inference Time
**Priority:** Medium
**Target Version:** v1.8.0

Extend `adapter_registry.cpp` and `AdapterLoadBalancer` (`adapter_load_balancer.cpp`) to support loading new LoRA adapters into a running `InferenceEngineEnhanced` without engine restart. Currently adapter sets are fixed at startup; adding a new fine-tuned adapter requires a rolling restart.

**Implementation Notes:**
- [x] Add `AdapterRegistry::hotLoad(adapter_id, weights_path, metadata)` which loads adapter weights into a pre-allocated VRAM slot managed by `adaptive_vram_allocator.cpp`.
- [x] Use a read-write lock on the adapter registry: hot-load acquires write lock briefly to register the new adapter; inference requests hold read locks and proceed without interruption.
- [x] `AdapterLoadBalancer` must handle the case where `hot_load` is in progress and temporarily routes requests for the loading adapter to a fallback (base model or another adapter variant).
- [x] Add admin API endpoint `POST /llm/adapters/{id}/load` that triggers hot-load; returns a `202 Accepted` with a job ID; status queryable via `GET /llm/adapters/{id}/load-status`.

**Performance Targets:**
- [x] Hot-load of a 7B-parameter LoRA adapter (16-bit weights, rank 64) ≤ 5 s wall-clock from API call to adapter available for inference.
- [x] Zero inference requests dropped during hot-load (all requests served via fallback or existing adapters).

---

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | `SpeculativeDecoder::verify()` tested with synthetic logit arrays; `OpenAICompatAdapter` tested with recorded OpenAI SDK request/response fixtures; `SharedWorkerPool` tested with concurrent task submission under saturation |
| Integration | Both engines end-to-end | Streaming test: submit request with `submitStreaming()`, assert tokens arrive incrementally before `is_final`; OpenAI adapter test: run OpenAI Python SDK `ChatCompletion.create()` against a local test server backed by `InferenceEngineEnhanced` |
| Performance | Tokens/sec regression < 5% | `benchmarks/llm_bench.cpp` runs on every PR touching `async_inference_engine.cpp` or `inference_engine_enhanced.cpp`; speculative decoding benchmark added alongside implementation |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| Time-to-first-token (512-token prompt, A10G) | ~350 ms (estimate) | ≤ 200 ms | `benchmarks/llm_bench.cpp` streaming path |
| Tokens/sec, 7B model, non-streaming (A10G) | ~40 tok/s (estimate) | ≥ 80 tok/s with speculative decoding | `benchmarks/llm_bench.cpp`, 200-token response |
| LoRA adapter hot-load time (rank-64) | Restart required | ≤ 5 s | Timed in integration test with admin API |
| Worker pool task dispatch latency | N/A (per-engine threads) | ≤ 50 µs p99 | Micro-benchmark in `tests/llm/bench_worker_pool.cpp` |

## Security / Reliability

- All inference requests must be checked against `PolicyEngine::checkInferencePermission()` before being queued; the `OpenAICompatAdapter` must propagate the requester identity from the API key header through to the policy check.
- Streaming token callbacks are invoked from worker threads; the HTTP layer must use a thread-safe SSE write queue to prevent data races on the response stream.
- LoRA adapter hot-load accepts a file path from the admin API; the path must be validated against a configurable allowlist directory (`LlmConfig::adapter_load_dir`) to prevent loading arbitrary files from the filesystem.
- Speculative decoding's draft model shares the GPU memory space with the target model; `adaptive_vram_allocator.cpp` must enforce a hard cap to prevent the draft model from evicting KV cache entries needed by in-flight target-model requests.
- `grammar.cpp` EBNF compilation is bounded by a configurable max grammar size (default 64 KB) to prevent CPU exhaustion from adversarial grammar inputs submitted via the OpenAI tools API.

---

## Scientific References

The following IEEE-formatted references support the research basis for features described in this document. References cover speculative decoding, federated/distributed inference, streaming inference, LoRA and adapter methods, and grammar-constrained generation.

### Speculative Decoding

[1] Y. Leviathan, M. Kalman, and Y. Matias, "Fast Inference from Transformers via Speculative Decoding," in *Proc. 40th Int. Conf. Machine Learning (ICML)*, PMLR, vol. 202, pp. 19274–19286, 2023. https://arxiv.org/abs/2211.17192

[2] C. Chen, S. Borgeaud, G. Irving, J.-B. Lespiau, L. Sifre, and J. Jumper, "Accelerating Large Language Model Decoding with Speculative Sampling," *arXiv preprint arXiv:2302.01318*, 2023. https://arxiv.org/abs/2302.01318

[3] X. Miao et al., "SpecInfer: Accelerating Large Language Model Serving with Tree-based Speculative Inference and Verification," in *Proc. 29th ACM Int. Conf. Architectural Support for Programming Languages and Operating Systems (ASPLOS)*, 2024. https://arxiv.org/abs/2305.09781

### Federated / Distributed Inference

[4] A. Diskin et al., "Distributed Deep Learning in Open Collaborations," in *Proc. 35th Conf. Neural Information Processing Systems (NeurIPS)*, 2021. https://arxiv.org/abs/2106.10207

[5] S. Kim et al., "Biscotti: A Blockchain System for Private and Secure Federated Learning," *IEEE Trans. Parallel Distrib. Syst.*, vol. 32, no. 7, pp. 1513–1525, Jul. 2021. https://doi.org/10.1109/TPDS.2020.3044223

[6] M. Shoeybi, M. Patwary, R. Puri, P. LeGresley, J. Casper, and B. Catanzaro, "Megatron-LM: Training Multi-Billion Parameter Language Models Using Model Parallelism," *arXiv preprint arXiv:1909.08053*, 2019. https://arxiv.org/abs/1909.08053

### Streaming Inference & Continuous Batching

[7] W. Kwon et al., "Efficient Memory Management for Large Language Model Serving with PagedAttention," in *Proc. 29th ACM Symp. Operating Systems Principles (SOSP)*, 2023, pp. 611–626. https://arxiv.org/abs/2309.06180

[8] A. Agrawal et al., "SARATHI: Efficient LLM Inference by Piggybacking Decodes with Chunked Prefills," *arXiv preprint arXiv:2308.16369*, 2023. https://arxiv.org/abs/2308.16369

### LoRA and Adapter Methods

[9] E. J. Hu et al., "LoRA: Low-Rank Adaptation of Large Language Models," in *Proc. 10th Int. Conf. Learning Representations (ICLR)*, 2022. https://arxiv.org/abs/2106.09685

[10] S. Sheng et al., "S-LoRA: Serving Thousands of Concurrent LoRA Adapters," *arXiv preprint arXiv:2311.03285*, 2023. https://arxiv.org/abs/2311.03285

[11] T. Dettmers, A. Pagnoni, A. Holtzman, and L. Zettlemoyer, "QLoRA: Efficient Finetuning of Quantized LLMs," in *Proc. 37th Conf. Neural Information Processing Systems (NeurIPS)*, 2023. https://arxiv.org/abs/2305.14314

### Grammar-Constrained Generation

[12] B. Scholak, N. Schucher, and D. Bahdanau, "PICARD: Parsing Incrementally for Constrained Auto-Regressive Decoding from Language Models," in *Proc. 2021 Conf. Empirical Methods in Natural Language Processing (EMNLP)*, 2021, pp. 9895–9901. https://arxiv.org/abs/2109.05093

[13] N. Geng et al., "Grammar-Constrained Decoding for Structured NLP Tasks without Finetuning," in *Proc. 2023 Conf. Empirical Methods in Natural Language Processing (EMNLP)*, 2023. https://arxiv.org/abs/2305.13971
