> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# LLM Module — Architecture Guide

<!-- Status: current | validated: 2026-04-06 | Primary: src/llm/ | Secondary: docs/de/llm/ -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../docs/de/llm/README.md -->

**Version:** 1.2
**Last Updated:** 2026-04-06
**Module Path:** `src/llm/`

---

## 1. Overview

The LLM module is ThemisDB's AI inference and language model management subsystem. It
provides two distinct inference engines (lightweight async wrapper and enterprise
multi-model orchestrator), a GGUF model loader, paged KV-cache management, LoRA adapter
lifecycle management, grammar-constrained generation, vision input support, ethics and
constitutional reasoning, AI decision auditing, and full Prometheus/Grafana observability.

---

## 2. Design Principles

- **Two-Engine Architecture** – `AsyncInferenceEngine` (single model, simple API path)
  and `InferenceEngineEnhanced` (multi-model, RAG path) coexist because they serve
  structurally different workloads.
- **Paged KV-Cache** – vLLM-inspired paged attention: KV-cache is partitioned into fixed
  pages, enabling efficient memory reuse across requests.
- **LoRA-First** – all inference paths support per-request LoRA adapter selection; the
  LoRA router selects the appropriate adapter based on domain and tenant.
- **Safety by Default** – ethical guidelines, constitutional reasoning, and AI decision
  auditing are enabled in the inference pipeline, not optional add-ons.
- **Plugin Architecture** – LLM backends are loaded as plugins via `llm_plugin_manager.cpp`;
  the in-process llama.cpp wrapper is one such backend.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `async_inference_engine.cpp` | Lightweight async wrapper for single LLM plugin |
| `inference_engine_enhanced.cpp` | Enterprise multi-model engine with KV-cache, batching, load balancing |
| `inference_handle.cpp` | Shared async request handle (get/ready/cancel) |
| `embedded_llm.cpp` | In-process embedded LLM server |
| `llamacpp_inference_engine.cpp` | llama.cpp backend integration |
| `llama_wrapper.cpp` | Direct llama.cpp C API wrapper |
| `gguf_loader.cpp` | GGUF model file loading and validation |
| `model_loader.cpp` / `model_downloader.cpp` | Model lifecycle: load, cache, download |
| `paged_kv_cache.cpp` / `paged_kv_cache_manager.cpp` | Paged KV-cache (vLLM-inspired) |
| `paged_block_manager.cpp` | KV-cache block allocation/deallocation |
| `kv_cache_buffer.cpp` | Raw KV-cache buffer management |
| `llm_prefix_cache.cpp` | Prompt prefix caching (prompt reuse) |
| `llm_response_cache.cpp` | Response-level caching |
| `continuous_batch_scheduler.cpp` | Continuous batching for throughput |
| `block_table.cpp` | Page table for KV-cache blocks |
| `lora_framework/` | LoRA adapter loading and hot-swapping |
| `lora_router.cpp` | Per-request LoRA adapter selection |
| `multi_lora_manager.cpp` | Concurrent multi-LoRA management |
| `lora_metadata_cache.cpp` | Cached LoRA adapter metadata |
| `lora_security_validator.cpp` | LoRA adapter safety validation |
| `llama_lora_adapter.cpp` | llama.cpp LoRA integration |
| `grammar.cpp` / `grammar_cache.cpp` | Grammar-constrained generation (GBNF) |
| `llama_grammar_adapter.cpp` | llama.cpp grammar API bridge |
| `attention/` | Flash Attention, paged attention kernels |
| `kernel_fusion.cpp` / `kernel_fusion.cu` | GPU kernel fusion for inference |
| `mixed_precision_inference.cpp` | FP16/BF16/INT8/INT4 inference |
| `adaptive_vram_allocator.cpp` | Dynamic VRAM allocation across requests |
| `active_vram_allocator.cpp` | GPU VRAM allocation with OOM recovery (LRU eviction, CPU spilling) |
| `gpu_memory_manager.cpp` | GPU memory lifecycle for LLM inference |
| `gpu_safe_fail.cpp` | LLM-specific GPU safe-fail wrapper |
| `multi_gpu_memory_coordinator.cpp` | Multi-GPU memory coordination |
| `vision_encoder.cpp` | Vision input encoding (images) |
| `vision_config.cpp` | Vision model configuration |
| `vision_resource_monitor.cpp` | Vision inference resource monitoring |
| `ai_decision_auditor.cpp` | Records and audits AI decisions for compliance |
| `constitutional_reasoning_engine.cpp` | Constitutional AI reasoning (safety constraints) |
| `ethical_guidelines_manager.cpp` | Ethics rules and enforcement |
| `ethics_aware_confidence_detector.cpp` | Confidence scoring with ethics integration |
| `moral_analyzer.cpp` | Moral analysis of LLM outputs |
| `ai_orchestrator.cpp` | Multi-model orchestration and routing |
| `adapter_registry.cpp` | Registry of available LLM adapters |
| `adapter_load_balancer.cpp` | Load balancing across adapter instances |
| `llm_plugin_manager.cpp` | Dynamic LLM backend plugin loading |
| `shared_worker_pool.cpp` | Shared thread pool for inference workers |
| `token_quota_manager.cpp` | Per-tenant token quota enforcement |
| `streaming_handler.cpp` | Token streaming (SSE / WebSocket) |
| `sampling_strategy.cpp` | Sampling: temperature, top-p, top-k, beam search |
| `prompt_manager.cpp` / `prompt_optimizer.cpp` | Prompt management and optimization |
| `prompt_evaluator.cpp` / `prompt_policy.cpp` | Prompt policy enforcement |
| `explanation_generator.cpp` | Natural language explanation of AI decisions |
| `feedback_store.cpp` / `feedback_plugin_basic.cpp` | User feedback collection |
| `fewshot_optimizer.cpp` | Few-shot example selection |
| `meta_prompt_generator.cpp` | Meta-prompt generation |
| `byzantine_detector.cpp` | Byzantine fault detection in distributed inference |
| `ml_model_manager.cpp` | General ML model management (non-LLM) |
| `llm_interaction_store.cpp` | Persist LLM interactions for audit/analysis |
| `llm_model_audit_logger.cpp` | Structured audit logging for model usage |
| `llm_model_storage.cpp` | Model weight storage integration |
| `model_metadata_cache.cpp` | Cached model metadata |
| `grafana_metrics.cpp` | Grafana/Prometheus metrics for LLM operations |
| `mcp_tool_bridge.cpp` | MCP (Model Context Protocol) tool bridge |
| `model_router.cpp` | Regex- and tag-based request routing to backend models |
| `speculative_decoder.cpp` | Speculative decoding: draft-model token generation and verifier acceptance |
| `openai_compat_adapter.cpp` | OpenAI-compatible `/v1/chat/completions` REST adapter |
| `model_quantization_pipeline.cpp` | Model quantization pipeline (GGUF, AWQ, GPTQ) |
| `distributed_training_coordinator.cpp` | Distributed training coordination |
| `aql_train_parser.cpp` | AQL training data parser |
| `docs_assistant.cpp` | Documentation-aware assistant |
| `mode_spec_loader.cpp` | Mode specification loader |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│              Callers (server API, rag, aql modules)              │
│   engine.submit(prompt, options) → InferenceHandle               │
└──────────────────────────┬──────────────────────────────────────┘
                           │
         ┌─────────────────┴────────────────────────┐
         │                                          │
┌────────▼──────────────────┐        ┌──────────────▼──────────────┐
│  AsyncInferenceEngine      │        │ InferenceEngineEnhanced      │
│  (single model, simple)    │        │ (multi-model, RAG)          │
│  priority queue            │        │ KV-cache, batch, LB         │
└────────┬──────────────────┘        └──────────────┬──────────────┘
         │                                          │
         └─────────────────┬────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                   LLM Plugin (llamacpp_inference_engine)         │
│   LlamaWrapper → llama.cpp C API → GGUF model on disk/GPU       │
└─────────────────────────────────────────────────────────────────┘
         │
┌────────┴────────────────────────────────────────────────────────┐
│  Paged KV-Cache Manager                                          │
│  LoRA Router → MultiLoraManager → LlamaLoraAdapter              │
│  Grammar → GrammarCache → LlamaGrammarAdapter                   │
│  Vision Encoder (optional)                                       │
│  Constitutional Reasoning / Ethics / AI Decision Auditor         │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Standard Inference

```
engine.submit(InferenceRequest {prompt, model, lora, grammar, sampling})
    │
    ├─ token_quota_manager.cpp: check per-tenant token budget
    │
    ├─ prompt_policy.cpp: policy check (content filter)
    │
    ├─ lora_router.cpp: select LoRA adapter (domain, tenant)
    │
    ├─ paged_kv_cache_manager: allocate pages for this request
    │
    ├─ continuous_batch_scheduler: queue request, batch with others
    │
    ├─ LlamaWrapper::infer(): run llama.cpp inference
    │       ├─ grammar-constrained: sample within GBNF grammar
    │       └─ standard: temperature/top-p/top-k sampling
    │
    ├─ constitutional_reasoning_engine: safety check output
    │
    ├─ ai_decision_auditor: log decision + context
    │
    └─ streaming_handler: stream tokens to client (SSE/WebSocket)
```

### 4.2 KV-Cache Reuse (Prefix Cache)

```
New request with prompt "System: You are a helpful assistant. User: ..."
    │
    ├─ llm_prefix_cache.cpp: hash prompt prefix
    ├─ cache hit? → reuse KV-cache pages (no re-computation)
    └─ cache miss → compute → store pages in prefix cache
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Called by** | `src/rag/` | Retrieval-Augmented Generation |
| **Called by** | `src/aql/` | LLM INFER/RAG/EMBED commands |
| **Called by** | `src/server/` | LLM API endpoints |
| **Uses** | `src/gpu/` | VRAM management |
| **Uses** | `src/acceleration/` | GPU compute backends |
| **Uses** | `src/index/` | Vector search for RAG context |
| **Uses** | `src/plugins/` | LLM backend plugin loading |
| **Uses** | `src/training/` | LoRA training coordination |

---

## 6. Threading & Concurrency Model

- `AsyncInferenceEngine` runs a priority queue with a configurable worker thread pool.
- `InferenceEngineEnhanced` uses continuous batching with a dedicated scheduler thread.
- `SharedWorkerPool` shared across both engines to limit total thread count.
- KV-cache page allocation uses a per-device lock.
- LoRA hot-swapping uses a read-write lock per adapter slot.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Paged KV-cache | Pages shared across requests; reduces memory fragmentation |
| Prefix cache | Common prompt prefixes reuse KV-cache without recomputation |
| Continuous batching | Dynamic batching groups requests to maximize GPU utilization |
| Kernel fusion | `kernel_fusion.cu` fuses attention + FFN for fewer GPU kernel launches |
| Mixed precision | FP16/INT8/INT4 inference for memory/speed tradeoffs |
| LoRA adapter | Domain adaptation without full model reload |

---

## 8. Security Considerations

- Constitutional reasoning and ethics guidelines block harmful outputs.
- AI decision auditor logs all inferences for compliance and forensics.
- LoRA security validator (`lora_security_validator.cpp`) checks adapters before loading.
- Token quota manager prevents abuse by limiting per-tenant token consumption.
- Vision inputs are sanitized to prevent adversarial inputs.
- `vram_secure_clear.cpp` (in `src/security/`) performs explicit VRAM zeroing on model unload to prevent cross-model data leakage; called unconditionally on model swap including error paths.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `llm.engine` | "async" | Engine: async / enhanced |
| `llm.model.default` | "" | Default model path |
| `llm.kv_cache.pages` | 1024 | KV-cache page count |
| `llm.kv_cache.page_size` | 16 | Tokens per KV-cache page |
| `llm.batch.max_size` | 32 | Max batch size |
| `llm.prefix_cache.enabled` | true | Enable prompt prefix cache |
| `llm.grammar.enabled` | true | Enable grammar-constrained generation |
| `llm.ethics.enabled` | true | Enable ethical guidelines |
| `llm.quota.tokens_per_minute` | 100000 | Per-tenant token quota |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Model file not found | Return structured error; no crash |
| GPU OOM during inference | `gpu_safe_fail.cpp`: fall back to CPU inference |
| LoRA load failure | Continue without adapter; log warning |
| Constitutional reasoning violation | Block output; return safety explanation |
| Token quota exceeded | Return 429-equivalent error |
| Byzantine node failure (distributed) | `byzantine_detector.cpp`: exclude node; re-route |

---

## 11. Known Limitations & Future Work

- Cancellation is best-effort only; in-flight inference cannot be interrupted at the llama.cpp level once a token generation step has started.
- Grammar-constrained generation requires runtime availability of the llama.cpp grammar API; it degrades gracefully when the API is absent.
- Vision/multi-modal support is experimental; only select model architectures are supported (`vision_encoder.cpp`).
- Multi-node distributed (federated) inference is not yet implemented; single-node multi-GPU is operational. (Issue: #1928)
- Speculative decoding uses synthetic logit arrays until per-token logits are exposed through the plugin interface.

---

## 12. References

- `src/llm/README.md` — module overview and architecture decision
- `src/llm/FUTURE_ENHANCEMENTS.md` — roadmap
- `docs/llm_roadmap.md` — LLM roadmap
- `docs/llm/` — LLM documentation
- `docs/GGUF_SUPPORT.md` — GGUF model format
- `docs/PAGED_ATTENTION_CODE_REVIEW.md` — paged attention review
- `ARCHITECTURE.md` (root) — full system architecture
