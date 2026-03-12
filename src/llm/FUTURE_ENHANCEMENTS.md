<!-- Status: current | validated: 2026-03-12 -->
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

### `LoraSecurityValidator`: Certificate Store Integration
**Priority:** High
**Target Version:** v1.8.0

`lora_security_validator.cpp` has 2 critical TODOs for certificate retrieval (lines 249 and 365):
- Line 249: "TODO: Retrieve from certificate store by fingerprint" — `cert_pem` is left empty; signature verification is effectively **never performed** when no inline certificate is provided in the metadata.
- Line 365: "TODO: Implement certificate store integration for production deployments"

This means LoRA adapter signature validation silently succeeds without verifying the certificate chain.

**Implementation Notes:**
- `[ ]` Implement a `LoRACertificateStore` class backed by `src/security/secret_manager.cpp` or a filesystem path (`config/security/lora_certs/`).
- `[ ]` In `verifyLoRASignature()` at line 248, look up the certificate by fingerprint from `LoRACertificateStore`; fail closed if the certificate is not found (return `SIGNATURE_UNVERIFIABLE` error, not success).
- `[ ]` Wire integration with the system certificate store (`/etc/ssl/certs` on Linux; `HCERTSTORE` on Windows) as a fallback after the local `LoRACertificateStore`.
- `[ ]` Add unit tests: missing cert → verification fails; valid cert + valid sig → passes; valid cert + tampered sig → fails.

---

### `LLMDeploymentPlugin`: RocksDB Model Storage
**Priority:** Medium
**Target Version:** v1.8.0

`llm_deployment_plugin.cpp` line 273 has: "Store in BaseEntity storage (RocksDB) - TODO: Uncomment when `llm_model_storage.cpp` exists". The plugin currently operates in "Filesystem-only mode" (line 136). Model metadata is not persisted to the database, breaking admin query ("list all deployed models") across restarts.

**Implementation Notes:**
- `[ ]` Implement `llm_model_storage.cpp` providing `LLMModelStorage::save(model_id, metadata)` and `load(model_id)` using the existing `StorageEngine` API with key prefix `llm_model::`.
- `[ ]` Uncomment the RocksDB persistence block at line 273; inject `StorageEngine*` into `LLMDeploymentPlugin` constructor.
- `[ ]` Implement `TODO(enhancement)` at line 916: check `model_id` existence before deployment to surface clear errors for unknown model IDs.
- `[ ]` Implement `TODO(feature)` at line 175: propagate authenticated user context from the request JWT into `audit.user` instead of hardcoding `"system"`.

---

### `AIOrchestrator`: Tool Call Parsing
**Priority:** Medium
**Target Version:** v1.8.0

`ai_orchestrator.cpp` line 494 has: "TODO(extensible): parse tool calls from `result.text` using `react_agent`". Without tool call parsing, the orchestrator cannot dispatch function calls returned by the LLM, making the ReAct loop incomplete.

**Implementation Notes:**
- `[ ]` Add tool call extraction to `AIOrchestrator::execute()` at line 494: parse the structured JSON block from `result.text` using `nlohmann::json`; dispatch to the registered tool via `AQLReActAgent::dispatchTool()`.
- `[ ]` Handle malformed tool call JSON gracefully: log a warning and continue with the raw text result rather than crashing.

---


**Priority:** High
**Target Version:** v1.7.0

Add token-streaming support so that callers receive generated tokens incrementally rather than waiting for the full response. This is required by interactive chat applications and the planned OpenAI-compatible API passthrough adapter. Both `AsyncInferenceEngine` and `InferenceEngineEnhanced` must support streaming via a new `submitStreaming()` method.

**Implementation Notes:**
- Add `IInferenceEngine::submitStreaming(InferenceRequest, TokenCallback)` to the engine interface; `TokenCallback` is `std::function<void(std::string_view token, bool is_final)>`.
- In `async_inference_engine.cpp`, invoke the callback from the worker thread after each llama.cpp token decode step; the callback must be thread-safe (called from the worker, consumed by the HTTP layer).
- In `inference_engine_enhanced.cpp`, integrate streaming with `continuous_batch_scheduler.cpp`; each batch step flushes decoded tokens for all in-flight requests to their respective callbacks.
- Return an `InferenceHandle` from `submitStreaming()` so callers can still call `cancel()` to abort mid-stream; on cancellation, the token callback receives a final call with `is_final=true` and an empty token.
- SSE framing (`data: {token}\n\n`) is applied at the HTTP layer, not inside the engine; the engine emits raw token strings.

**Performance Targets:**
- Time-to-first-token (TTFT) ≤ 200 ms p99 for prompt lengths ≤ 512 tokens on a single A10G GPU.
- Streaming overhead (vs non-streaming) ≤ 2 % of total tokens/sec throughput.

---

### OpenAI-Compatible `/v1/chat/completions` Adapter
**Priority:** High
**Target Version:** v1.7.0

Add an `OpenAICompatAdapter` that translates OpenAI Chat Completions API requests to `InferenceRequest` structs and routes them to `InferenceEngineEnhanced`. This allows existing OpenAI API clients (LangChain, LlamaIndex, OpenAI Python SDK) to target ThemisDB's local inference engine without code changes.

**Implementation Notes:**
- Add `openai_compat_adapter.cpp` in `src/llm/`; parse `POST /v1/chat/completions` JSON body into `InferenceRequest` including `messages`, `temperature`, `max_tokens`, `stream`, `stop`, and `tools` fields.
- Map the `messages` array to the internal prompt format using the same chat template logic already used by `InferenceEngineEnhanced`; support `system`, `user`, and `assistant` roles.
- `stream: true` routes to `IInferenceEngine::submitStreaming()` and emits SSE chunks in the `data: {"choices":[{"delta":{"content":"..."}}]}` format.
- Function/tool calling: serialize tool definitions as grammar constraints via `grammar.cpp` to enforce valid JSON output; return `tool_calls` in the response when the model outputs a function-call JSON object.
- Add API key validation via `PolicyEngine::checkInferencePermission()` before request processing; return HTTP 401 on denied requests.

**Performance Targets:**
- Non-streaming request overhead (adapter serialization/deserialization) ≤ 2 ms vs direct `submitRequest()` call.
- Compatible with OpenAI SDK smoke tests: `openai.ChatCompletion.create()` with `stream=False` and `stream=True` must both succeed against a local ThemisDB instance.

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
- Add `AdapterRegistry::hotLoad(adapter_id, weights_path, metadata)` which loads adapter weights into a pre-allocated VRAM slot managed by `adaptive_vram_allocator.cpp`.
- Use a read-write lock on the adapter registry: hot-load acquires write lock briefly to register the new adapter; inference requests hold read locks and proceed without interruption.
- `AdapterLoadBalancer` must handle the case where `hot_load` is in progress and temporarily routes requests for the loading adapter to a fallback (base model or another adapter variant).
- Add admin API endpoint `POST /llm/adapters/{id}/load` that triggers hot-load; returns a `202 Accepted` with a job ID; status queryable via `GET /llm/adapters/{id}/load-status`.

**Performance Targets:**
- Hot-load of a 7B-parameter LoRA adapter (16-bit weights, rank 64) ≤ 5 s wall-clock from API call to adapter available for inference.
- Zero inference requests dropped during hot-load (all requests served via fallback or existing adapters).

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
