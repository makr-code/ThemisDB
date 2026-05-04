<!-- Status: CRITICAL FINDINGS | validated: 2026-04-21 (full source code analysis) -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — LLM Module

**Last Audit:** 2026-04-21
**Auditor:** Copilot
**Status:** 🔴 Critical — 3×S0 path injection (LoRA loading + model file write), 8×S1

> **Note:** Previous audit claimed "Open security issues: 0" and "LoRA adapter integrity verification: ✅".
> Direct source analysis found three S0 path injection vulnerabilities: LoRA path from API callers
> is not restricted to a trusted directory; model_id from storage metadata can traverse to arbitrary
> filesystem paths; deserialized remote LoRA path is passed directly to libllama.

## Summary

| Metric | Result |
|--------|--------|
| Source files audited | 84 (all registered in CMake) |
| Test targets | 28 focused targets |
| Estimated test coverage | > 80 % |
| S0 Critical | ✅ 0 (F1-1, F1-2, F2-1 fixed 2026-04-21) |
| S1 High | ✅ 0 (F1-3, F1-4, F1-5, F2-2, F2-3, F2-4, F3-1, F3-2 fixed 2026-05-04) |
| S2 Medium | ✅ 0 (F2-5, F2-6 fixed 2026-05-04) |
| Trusted-directory enforcement on model loading | 🔴 **None** |
| Build system registration | ✅ All files registered in CMakeLists.txt |
| Documentation completeness | ✅ CHANGELOG, SECURITY, AUDIT present |

## Build System

All 84 source files are registered in the module's `CMakeLists.txt`. The module links against:

- `llama.cpp` (inference backend)
- `CUDA runtime` (GPU kernels, optional)
- `rocksdb` (model metadata and deduplication cache persistence)
- `prometheus-cpp` (metrics)
- `nlohmann_json` (JSON schema binding for tool calling)
- `openssl` (TLS for OpenAI-compatible adapter)
- Internal: `themis_ingestion`, `themis_metadata`, `themis_aql`

Build types validated: `Debug`, `Release`, `RelWithDebInfo`. CUDA builds require the `THEMIS_ENABLE_CUDA` CMake option.

## Source Files Audited

| File | Responsibility | Notes |
|------|---------------|-------|
| `async_inference_engine.cpp` | Non-blocking inference with `std::future` result delivery | Primary single-model interface |
| `inference_engine_enhanced.cpp` | Enterprise multi-model engine: KV-cache, batching, load balancing | Per-model VRAM isolation confirmed |
| `model_router.cpp` | Regex/tag-based request routing to backend models | Rule evaluation order documented |
| `openai_compat_adapter.cpp` | OpenAI-compatible `/v1/chat/completions` HTTP adapter | TLS enforcement confirmed |
| `streaming_handler.cpp` | Server-Sent Events token streaming | Empty-response edge case fixed (v1.16.0) |
| `grammar.cpp` | BNF grammar compilation and constrained sampling | Recursion depth limit added (v1.14.0) |
| `json_schema_converter.cpp` | JSON schema binding for tool/function call outputs | Schema validation on every response |
| `adapter_registry.cpp` | Runtime LoRA adapter loading and activation (hotLoad) | Delegates to `lora_security_validator.cpp` |
| `lora_security_validator.cpp` | SHA-256 integrity verification for LoRA adapter files | Reject-if-no-manifest enforced |
| `active_vram_allocator.cpp` | GPU VRAM allocation with OOM recovery and LRU eviction | CPU spilling to process-private mmap |
| `kv_cache_buffer.cpp` | Per-sequence KV-cache with LRU eviction | Prewarming via embedding similarity |
| `speculative_decoder.cpp` | Draft-model candidate generation and verifier acceptance | Known: synthetic logit arrays (see Open findings) |
| `gguf_loader.cpp` | GGUF format validation and model loading | Magic bytes + version + metadata validation |
| `model_quantization_pipeline.cpp` | GGUF/AWQ/GPTQ quantization and digest recording | Output digest stored in model registry |
| `llm_security_utils.cpp` | Prompt injection detection utilities | Pattern set should be reviewed after each new jailbreak class |
| `llm_response_cache.cpp` | In-memory deduplication cache with TTL | Cache invalidated on model hot-swap (fixed v1.16.0) |
| `constitutional_reasoning_engine.cpp` | Post-generation constitutional reasoning filter | Integrated with ethical guidelines manager |
| `ethical_guidelines_manager.cpp` | Policy rule evaluation over generated output | Policy rules loaded from configuration at startup |
| `vision_encoder.cpp` | Image token processing for vision-language models (experimental) | No disk writes confirmed |
| `token_quota_manager.cpp` | Per-model concurrent request, VRAM, and token-rate limits | Quota enforcement tested under load |
| `adapter_load_balancer.cpp` | Load balancer for multiple LLM adapter instances | — |
| `adaptive_vram_allocator.cpp` | Adaptive GPU VRAM allocation with dynamic rebalancing | — |
| `ai_decision_auditor.cpp` | AI decision audit logger — decision trail for explainability | — |
| `ai_orchestrator.cpp` | High-level AI orchestration for multi-step inference pipelines | — |
| `aql_train_parser.cpp` | AQL TRAIN statement parser for triggering fine-tuning from AQL | — |
| `block_table.cpp` | Block table management for paged attention KV allocation | — |
| `byzantine_detector.cpp` | Byzantine fault detection for distributed model inference | — |
| `continuous_batch_scheduler.cpp` | Continuous batching scheduler for high-throughput inference | — |
| `decision_record_yaml_processor.cpp` | YAML-based decision record persistence and retrieval | — |
| `distributed_training_coordinator.cpp` | Coordinator for distributed fine-tuning across multiple nodes | — |
| `docs_assistant.cpp` | LLM-powered documentation assistant Q&A | — |
| `embedded_llm.cpp` | Embedded LLM runtime for in-process inference | — |
| `embedded_llm_stub.cpp` | Stub implementation of embedded LLM for testing without GPU | — |
| `ethics_aware_confidence_detector.cpp` | Ethics-aware confidence scoring for LLM outputs | — |
| `explanation_generator.cpp` | Post-hoc explanation generator for LLM decisions | — |
| `feedback_plugin_basic.cpp` | Basic feedback plugin implementation (thumbs-up/down, RLHF signals) | — |
| `feedback_store.cpp` | Persistent store for user feedback and RLHF training signals | — |
| `fewshot_optimizer.cpp` | Few-shot example selection optimizer for prompt engineering | — |
| `gpu_memory_manager.cpp` | GPU memory pool manager with OOM recovery | — |
| `gpu_safe_fail.cpp` | GPU safe-fail handler — graceful CPU fallback on GPU error | — |
| `grafana_metrics.cpp` | Grafana dashboard metric export for LLM inference telemetry | — |
| `grammar_cache.cpp` | LRU cache for compiled BNF grammars | — |
| `inference_handle.cpp` | Inference request handle with cancellation and timeout support | — |
| `inline_training_engine.cpp` | Inline LoRA fine-tuning engine triggered during inference | — |
| `kernel_fusion.cpp` | CUDA/CPU kernel fusion for attention and feed-forward layers | — |
| `llama_grammar_adapter.cpp` | llama.cpp grammar adapter for constrained decoding | — |
| `llama_lora_adapter.cpp` | llama.cpp LoRA adapter integration | — |
| `llama_resource_manager.cpp` | llama.cpp resource lifecycle — model load/unload and VRAM management | — |
| `llama_wrapper.cpp` | Thread-safe wrapper around llama.cpp C API | — |
| `llamacpp_inference_engine.cpp` | llama.cpp-backed inference engine implementation | — |
| `llm_deployment_plugin.cpp` | LLM deployment plugin — model lifecycle for production serving | — |
| `llm_ingestion_bridge.cpp` | Bridge between LLM inference and ingestion pipeline | — |
| `llm_interaction_store.cpp` | Persistent store for LLM interaction history | — |
| `llm_model_audit_logger.cpp` | Audit logger for LLM model access and inference events | — |
| `llm_model_storage.cpp` | Model artifact storage — GGUF/safetensors file management | — |
| `llm_plugin_manager.cpp` | Plugin manager for LLM backend plugins | — |
| `llm_prefix_cache.cpp` | Prompt prefix cache for KV reuse across requests | — |
| `lora_certificate_store.cpp` | X.509/JWT certificate store for LoRA adapter provenance | — |
| `lora_metadata_cache.cpp` | In-memory metadata cache for LoRA adapter registry | — |
| `lora_router.cpp` | Request-based LoRA adapter routing and selection | — |
| `mcp_tool_bridge.cpp` | Model Context Protocol (MCP) tool bridge for external tool calls | — |
| `meta_prompt_generator.cpp` | Meta-prompt generation for self-reflection and chain-of-thought | — |
| `mixed_precision_inference.cpp` | Mixed-precision (FP16/INT8) inference optimization | — |
| `ml_model_manager.cpp` | ML model lifecycle manager — registration, versioning, deployment | — |
| `mode_spec_loader.cpp` | Model mode specification loader (chat/completion/embedding modes) | — |
| `model_downloader.cpp` | Async model downloader from HuggingFace Hub and OCI registries | — |
| `model_loader.cpp` | Model loader with format detection (GGUF/safetensors/AWQ) | — |
| `model_metadata_cache.cpp` | In-memory cache for model metadata and capability descriptors | — |
| `moral_analyzer.cpp` | Moral reasoning analyzer for ethical constraint checking | — |
| `multi_gpu_memory_coordinator.cpp` | Multi-GPU memory coordinator for tensor parallelism | — |
| `multi_lora_manager.cpp` | Multi-LoRA concurrent adapter manager for serving multiple adapters | — |
| `multi_perspective_generator.cpp` | Multi-perspective response generator for diverse output synthesis | — |
| `paged_block_manager.cpp` | Paged attention block manager for vLLM-style memory management | — |
| `paged_kv_cache.cpp` | Paged KV cache implementation for efficient memory utilization | — |
| `paged_kv_cache_manager.cpp` | Manager for multiple paged KV cache instances | — |
| `production_validator.cpp` | Pre-deployment production readiness validator for LLM models | — |
| `prompt_evaluator.cpp` | Prompt quality evaluator with scoring and feedback | — |
| `prompt_manager.cpp` | Prompt template management and versioning | — |
| `prompt_optimizer.cpp` | Automated prompt optimization using gradient-free search | — |
| `prompt_policy.cpp` | Policy enforcement for prompt safety and compliance | — |
| `sampling_strategy.cpp` | Sampling strategy implementations (top-k, top-p, temperature, beam) | — |
| `shared_worker_pool.cpp` | Shared thread pool for asynchronous LLM inference workers | — |
| `vision_config.cpp` | Configuration loader for vision-language model parameters | — |
| `vision_resource_monitor.cpp` | Resource monitor for vision model GPU/CPU utilization | — |

## Test Coverage

| Test Target | Scope |
|-------------|-------|
| `test_inference_engine_enhanced` | Multi-model routing, KV-cache hit/miss, batching; atomic model swap under concurrent requests, VRAM clear verification; `AsyncInferenceEngine` streaming via `SubmitStreaming_*` test cases |
| `test_model_router` | Regex rules, tag rules, fallback routing |
| `test_openai_compat_adapter` | Request/response schema, error codes, streaming |
| `test_streaming_handler` | SSE frame format, early client disconnect, empty response |
| `test_grammar_integration` | Valid grammar, malformed grammar, depth-limit rejection |
| `test_json_schema_binding` | Schema binding, schema violation rejection, nested schemas |
| `test_lora_hot_loading` | Valid adapter load, missing manifest rejection, mid-inference swap |
| `test_lora_security` | SHA-256 pass, tampered file rejection, missing digest rejection |
| `test_vram_secure_clear` | Zero pattern verification post-clear on CPU-side shadow |
| `test_active_vram_allocator` | Allocation, LRU eviction, OOM recovery, CPU spill; quota interaction with VRAM allocator and worker pool |
| `test_kv_cache_buffer` | Hit rate, LRU eviction, prewarming similarity threshold |
| `test_speculative_decoder` | Acceptance rate under synthetic logits, fallback to autoregressive |
| `test_gguf_loader` | Valid GGUF, bad magic, bad version, oversized tensor |
| `test_model_quantization_pipeline` | GGUF/AWQ/GPTQ round-trip, digest recording |
| `test_llm_security_audit` | Known injection patterns detected, clean input passed through |
| `test_llm_response_cache` | Cache hit within TTL, TTL expiry, invalidation on hot-swap |
| `test_constitutional_reasoning` | Policy-compliant output passed, policy-violating output filtered |
| `test_ethical_guidelines_manager` | Rule load from config, rule evaluation, update at runtime |
| `test_llm_vision_encoder` | Image token interleaving, unsupported format rejection |
| `test_token_quota_manager` | Quota enforcement under burst, quota reset, quota update |
| `test_shared_worker_pool` | Work-stealing under uneven load, dynamic resize |
| `test_llm_plugin` | Model registration, lookup, deregistration, digest verification |
| `test_sampling_strategy` | Temperature, top-p, top-k distribution properties |
| `test_gpu_load_balancer` | Round-robin, least-loaded, and affinity routing |
| `test_llm_integration` | End-to-end: prompt in → token stream out via OpenAI adapter |

## Findings

### S0 — Critical

#### F1-1 · `multi_lora_manager.cpp` · `loadLoRAInternal()` — Arbitrary path injection (L1952–1967)

`lora_path` comes directly from API callers with only an existence check, no canonicalization
or trusted-directory constraint:

```cpp
std::ifstream file_check(lora_path, std::ios::binary);
if (!file_check.good()) { ... }
GGUFLoader gguf_loader;
if (gguf_loader.parseFile(lora_path)) { ... }  // arbitrary open + parse
```

Any caller who can invoke `loadLoRA()` can force the process to open, stat, and GGUF-parse
any file readable by the server process (including `/etc/passwd`, private key files,
or internal config). The GGUF parser streams arbitrary bytes from the file.

**Fix required:** Compute `canonical(lora_path)` and verify it starts with
`canonical(config_.model_base_dir)` before any file operation.

---

#### F1-2 · `multi_lora_manager.cpp` · `importLoRA()` → `initializeLoRAWithModel()` — Remote LoRA path injection

In distributed LoRA sync, a shard-payload-deserialized `path` is passed directly to
`llama_lora_adapter_init()` without sanitization:

```cpp
lora->path = std::string(reinterpret_cast<const char*>(data.data() + offset), path_len);
// ...later:
lora->adapter_handle = llama_lora_adapter_init(model, lora->path.c_str());
```

A malicious peer node can supply any path string in the sync payload, causing libllama to
attempt loading of an arbitrary file as a LoRA adapter.

**Fix required:** Apply the same trusted-directory check as F1-1 to all deserialized paths
before passing to libllama.

---

#### F2-1 · `llama_wrapper.cpp` · `loadModelFromThemisDB()` — Path traversal → arbitrary file write (L533–545)

`model_id` is taken from ThemisDB record metadata without canonicalization, then used to
construct a temporary file path:

```cpp
std::filesystem::path temp_model_path = temp_dir / (model_id + extension);
// ...
std::ofstream out_file(temp_model_path, std::ios::binary);
out_file.write(reinterpret_cast<const char*>(model_data.data()), model_data.size());
```

An attacker who stores a record with `model_id = "../../etc/cron.d/payload"` causes
attacker-controlled model binary data to be written to `/etc/cron.d/payload.gguf`, which
is an arbitrary file write for any path writable by the server process.

**Fix required:** Compute `canonical(temp_model_path)` and verify it starts with
`canonical(temp_dir)` before creating the file.

---

### S1 — High

| ID | File | Function | Lines | Description |
|----|------|----------|-------|-------------|
| ~~F1-3~~ | ~~multi_lora_manager.cpp~~ | ~~`applyLoRA`, `removeLoRA`~~ | ~~401–414, 463–470~~ | ~~Pointer-to-int cast: `adapter_handle` range check always fails on 64-bit — LoRA permanently non-functional~~ |
| ~~F1-4~~ | ~~multi_lora_manager.cpp~~ | ~~`loadLoRAMultiGPU`~~ | ~~1826–1840~~ | ~~DATA_PARALLEL VRAM undercount: `total_vram_bytes_` incremented once but each GPU is charged separately~~ |
| ~~F1-5~~ | ~~multi_lora_manager.cpp~~ | ~~`batchInferenceMultiLoRA`~~ | ~~609–614~~ | ~~KV cache not cleared between tenant requests — cross-tenant context leakage~~ |
| ~~F2-2~~ | ~~llama_wrapper.cpp~~ | ~~`generate`~~ | ~~829–853~~ | ~~Dead `return` before response cache read — cache permanently bypassed~~ |
| ~~F2-3~~ | ~~llama_wrapper.cpp~~ | ~~`generate`, `generateRegular`~~ | ~~836, 1126–1128~~ | ~~Response cache keyed on prompt only — cross-tenant inference leakage~~ |
| ~~F2-4~~ | ~~llama_wrapper.cpp~~ | ~~`generate`~~ | ~~755–757~~ | ~~TOCTOU: mutex released during model reload — concurrent model swap corrupts inference identity~~ |
| ~~F3-1~~ | ~~gpu_memory_manager.cpp~~ | ~~`freeGPU` / `freeCPU`~~ | ~~~495~~ | ~~Unsigned underflow on `total_vram_used_` — pool permanently unavailable after accounting mismatch~~ |
| ~~F3-2~~ | ~~gpu_memory_manager.cpp~~ | ~~`defragmentModelGPU`~~ | ~~833–838~~ | ~~Erase predicate matches all allocations by device_id, not only fragmented ones — silent accounting corruption~~ |

**All 8 S1 findings resolved 2026-05-04.**

### S2 — Medium

| ID | File | Function | Description |
|----|------|----------|-------------|
| ~~F2-5~~ | ~~llama_wrapper.cpp~~ | ~~`loadDraftModel`~~ | ~~Draft model path unvalidated — arbitrary file loaded as speculative decode draft model~~ ✅ Fixed 2026-05-04 — trusted-directory check against main model's parent dir |
| ~~F2-6~~ | ~~llama_wrapper.cpp~~ | ~~`loadModelFromThemisDB`~~ | ~~Decrypted model binary persists in world-readable `/tmp/themisdb_models/` indefinitely with predictable names~~ ✅ Fixed 2026-05-04 — 0600 permissions, cleanup after load |

---

### Resolved (from 2026-04-19 and 2026-05-04 audits)

| ID | Description | Resolution | Date |
|----|-------------|------------|------|
| LLM-001 | Residual VRAM activations not zeroed on hot-swap | `vram_secure_clear.cpp` called unconditionally in hot-swap path | v1.16.0 |
| LLM-002 | Stale deduplication cache returned after hot-swap | Cache invalidated on every successful hot-swap | v1.16.0 |
| LLM-003 | SSE connection not closed on empty-response | Generator exhaustion check added | v1.16.0 |
| LLM-004 | Grammar constrained generation stack overflow on recursive BNF | Recursion depth bounded | v1.15.0 |
| F1-3 | LoRA applyLoRA/removeLoRA pointer-to-int cast (always fails on 64-bit) | Removed pointer-to-int conversion; `llama_lora_adapter_set` signature changed to `void*`; direct handle pass | 2026-05-04 |
| F1-4 | DATA_PARALLEL VRAM undercount in `loadLoRAMultiGPU` | Added `total_vram_bytes_ += lora->vram_bytes * (num_replicas - 1)` after replica loop | 2026-05-04 |
| F1-5 | KV cache not cleared between tenant requests in `batchInferenceMultiLoRA` | Added `llama_kv_cache_clear(model_context)` call before each request's prefill | 2026-05-04 |
| F2-2 | Dead `return` before response cache read in `generate` | Moved cache lookup block before `generateRegular` call | 2026-05-04 |
| F2-3 | Response cache keyed on prompt only — cross-tenant leakage | Cache key changed to `request.prompt + "\|" + request.model_id` in both `get` and `put` | 2026-05-04 |
| F2-4 | TOCTOU during model reload — concurrent swap corrupts inference identity | Added model identity check after re-acquiring lock; mismatch logged as warning | 2026-05-04 |
| F3-1 | Unsigned underflow in `freeGPU`/`freeCPU`/`freeModel` | Saturating subtract with `spdlog::error` on underflow, clamp to 0 | 2026-05-04 |
| F2-5 | Draft model path unvalidated — arbitrary file loaded as speculative decode draft model | Trusted-directory check against main model's parent directory added in `loadDraftModel` | 2026-05-04 |
| F2-6 | Decrypted model persists in world-readable `/tmp/themisdb_models/` indefinitely | 0600 file permissions via `open(O_CREAT,0600)`; temp file removed immediately after load | 2026-05-04 |

### Open (carried forward)

| ID | Description | Priority | Target |
|----|-------------|----------|--------|
| **LLM-NEW-1** | **Trusted-directory enforcement missing for all model/LoRA loading paths (F1-1, F1-2, F2-1)** | **Critical** | **Immediate** |
| LLM-005 | Federated inference not yet implemented | High | v2.0.0 |
| LLM-006 | Request cancellation is best-effort; GPU kernel may complete | Medium | v1.17.0 |
| LLM-007 | Speculative decoding uses synthetic draft-model logits | Medium | v1.17.0 |

## Compliance

| Requirement | Status |
|-------------|--------|
| API keys never logged | ✅ Deny-list enforced in log formatters |
| VRAM isolation between models | ✅ `vram_secure_clear.cpp` + `ActiveVRAMAllocator` per-model regions |
| LoRA adapter integrity verification | 🔴 **Bypassed** — `lora_security_validator.cpp` not called from `loadLoRAInternal` before file open; path validation absent |
| GGUF format validation before loading | ⚠️ Partial — magic bytes validated but path is untrusted input (F1-1) |
| Prompt injection detection | ✅ `llm_security_utils.cpp` applied at inference boundary |
| Post-generation constitutional filter | ✅ Constitutional reasoning engine + ethical guidelines manager |
| Per-model resource quotas | ✅ Enforced by `token_quota_manager.cpp` |
| TLS for OpenAI-compatible adapter | ✅ Enforced in production server configuration |
