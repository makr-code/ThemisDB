<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · ../../src/llm/ -->

# LLM Module — Public Header Architecture
**Version:** 1.16.0
**Module Path:** `include/llm/`
**Implementation:** `../../src/llm/`

---

## Overview

The LLM module is the largest public header surface in ThemisDB, providing interfaces for multi-model inference, LoRA adapter management, KV-cache, paged memory, speculative decoding, continuous batching, VRAM management, constitutional reasoning, ethical guidelines, multi-perspective generation, and LLM deployment plugins.

## Design Principles

- **Plugin Architecture** — `ILlmPlugin` / `LlmPluginManager` enable runtime-swappable LLM backends.
- **Paged KV-Cache** — `PagedKvCache` / `PagedKvCacheManager` implement vLLM-style paged attention memory.
- **Multi-LoRA** — `MultiLoraManager` manages concurrent LoRA adapters per model.
- **Safety-First** — `ConstitutionalReasoningEngine`, `EthicsAwareConfidenceDetector`, and `EthicalGuidelinesManager` enforce alignment constraints.
- **GPU-Native** — `GpuMemoryManager`, `MultiGpuMemoryCoordinator`, `ActiveVramAllocator` provide fine-grained VRAM lifecycle management.

## Interface Inventory (Key Headers)

| Header | Purpose |
|--------|---------|
| `i_llm_plugin.h` / `llm_plugin_interface.h` | LLM plugin interface contracts |
| `llm_plugin_manager.h` | Plugin registry and lifecycle |
| `llm_deployment_plugin.h` | Deployment plugin interface |
| `llamacpp_inference_engine.h` | llama.cpp inference engine |
| `async_inference_engine.h` | Async inference with request queuing |
| `inference_engine_enhanced.h` | Enhanced inference (speculative, streaming) |
| `inference_handle.h` | Inference handle / request token |
| `continuous_batch_scheduler.h` | Continuous batching scheduler |
| `speculative_decoder.h` | Draft-model speculative decoding |
| `paged_kv_cache.h` / `paged_kv_cache_manager.h` | vLLM-style paged KV-cache |
| `kv_cache_buffer.h` | KV-cache buffer management |
| `block_table.h` | Block table for paged attention |
| `paged_block_manager.h` | Paged block allocator |
| `multi_lora_manager.h` | Concurrent LoRA adapter manager |
| `lora_router.h` | Per-request LoRA routing |
| `lora_metadata_cache.h` | LoRA metadata caching |
| `lora_certificate_store.h` | LoRA provenance certificates |
| `lora_security_validator.h` | LoRA security validation |
| `adapter_registry.h` | Adapter registration |
| `adapter_deployment_manager.h` | Adapter deployment lifecycle |
| `adapter_load_balancer.h` | Load balancing across adapters |
| `adapter_compatibility.h` | Adapter compatibility checks |
| `gpu_memory_manager.h` | VRAM lifecycle management |
| `multi_gpu_memory_coordinator.h` | Multi-GPU VRAM coordination |
| `active_vram_allocator.h` / `adaptive_vram_allocator.h` | Active/adaptive VRAM allocation |
| `gpu_safe_fail.h` | GPU safe-fail / OOM handling |
| `model_loader.h` | Model file loading |
| `model_router.h` | Regex/tag-based model routing |
| `model_metadata_cache.h` | Model metadata caching |
| `model_downloader.h` | Model download management |
| `model_quantization_pipeline.h` | Quantisation pipeline |
| `lazy_model_loader.h` | Lazy load for large models |
| `ml_model_manager.h` | ML model lifecycle manager |
| `llama_wrapper.h` | llama.cpp C++ wrapper |
| `llama_resource_manager.h` | llama.cpp resource management |
| `llamacpp_training_backend.h` | llama.cpp fine-tuning backend |
| `gguf_loader.h` / `gguf_st_adapter.h` | GGUF model format loading |
| `inline_training_engine.h` | Inline fine-tuning engine |
| `distributed_training_coordinator.h` | Distributed training coordination |
| `multi_model_training_data.h` | Multi-model training data management |
| `constitutional_reasoning_engine.h` | Constitutional AI reasoning |
| `ethical_guidelines_manager.h` | Ethical guidelines enforcement |
| `ethics_aware_confidence_detector.h` | Ethics-aware confidence scoring |
| `moral_analyzer.h` | Moral analysis of outputs |
| `byzantine_detector.h` | Byzantine fault detection in distributed inference |
| `openai_compat_adapter.h` | OpenAI `/v1/chat/completions` adapter |
| `llm_response_cache.h` | Response-level cache |
| `llm_prefix_cache.h` | Prefix KV-cache reuse |
| `grammar.h` / `grammar_cache.h` | Constrained generation grammar |
| `prompt_manager.h` / `prompt_optimizer.h` / `prompt_policy.h` | Prompt lifecycle |
| `meta_prompt_generator.h` | Meta-prompt generation |
| `fewshot_optimizer.h` | Few-shot example optimisation |
| `sampling_strategy.h` | Token sampling strategies |
| `streaming_handler.h` | SSE streaming response handler |
| `token_quota_manager.h` | Per-tenant token quota |
| `ai_decision_auditor.h` | AI decision audit logging (RocksDB + PKI) |
| `decision_record_yaml_processor.h` | Async YAML decision traceability (see §Decision Traceability) |
| `llm_model_audit_logger.h` | Model-level audit logging |
| `llm_model_storage.h` | Model storage interface |
| `llm_interaction_store.h` | Interaction history store |
| `llm_security_utils.h` | Security utilities (sanitisation) |
| `production_validator.h` | Production readiness validator |
| `explanation_generator.h` | Explanation / rationale generation |
| `multi_perspective_generator.h` | Multi-perspective output generation |
| `feedback_store.h` / `i_feedback_plugin.h` | Feedback collection |
| `shared_worker_pool.h` | Shared thread pool for inference workers |
| `mixed_precision_inference.h` | Mixed-precision (FP16/BF16/INT8) inference |
| `kernel_fusion.h` / `kernel_fusion_cuda.h` | CUDA kernel fusion optimisations |
| `grafana_metrics.h` | Grafana metrics export |
| `docs_assistant.h` | Documentation assistant LLM interface |
| `ai_orchestrator.h` | Multi-agent AI orchestration |
| `vision_config.h` / `vision_encoder.h` / `vision_resource_monitor.h` | Vision model support |
| `json_schema_converter.h` | JSON schema ↔ grammar conversion |

## Decision Traceability Subsystem

### Overview

`DecisionRecordYamlProcessor` provides lightweight, **async, human-readable**
traceability for every autonomous decision made by the LLM/LoRA stack.  It
runs as an independent background thread and writes one YAML file per decision
to `logs/decisions/YYYY-MM-DD/`.

### Design

```
LLM/LoRA Component
    │  submit(DecisionRecord)   [non-blocking, O(1)]
    ▼
DecisionRecordYamlProcessor
    ├── std::queue<DecisionRecord>  [bounded, max_queue_depth=4096]
    ├── std::thread writer_thread_  [independent of inference threads]
    └── logs/decisions/YYYY-MM-DD/<ts>_<type>_<id>.yaml
```

### Injection Pattern

All components that produce decisions accept the processor via:

```cpp
component.setDecisionRecordProcessor(shared_ptr<DecisionRecordYamlProcessor>);
```

Passing `nullptr` disables tracing with zero overhead.

### Well-Known Decision Types

| `decision_type` | Produced By |
|-----------------|-------------|
| `FEDERATED_ROUND` | `LoRAFederationCoordinator` |
| `FEDERATED_FEEDBACK` | `CrossShardFeedbackSync` |
| `LORA_ADAPTER_SELECTION` | `LoraRouter` (planned) |
| `LORA_RANK_ADJUSTMENT` | `AdapterLoadBalancer` (planned) |
| `LOOP_TRIGGER` | `LoraOrchestrator` (planned) |
| `THRESHOLD_UPDATE` | `EthicsAwareConfidenceDetector` (planned) |
| `CIRCUIT_BREAKER_OPEN` | `LoRAFederationCoordinator` (planned) |
| `GDPR_ERASE` | `GdprSubjectRightsManager` (planned) |

### vs. `AIDecisionAuditor`

| Aspect | `AIDecisionAuditor` | `DecisionRecordYamlProcessor` |
|--------|---------------------|-------------------------------|
| Storage | RocksDB | YAML files |
| Signing | Ed25519 PKI | None |
| Blocking | Yes (hot-path sync) | No (async queue) |
| Use case | Compliance audit | Developer traceability |
| Dependency | RocksDB + OpenSSL | yaml-cpp |

### References

- `docs/decisions/ADR-001-decision-record-yaml-processor.md`
- `docs/issues/llm/DR-001-decision-record-yaml-integration.md`
- `tests/test_decision_record_yaml_processor.cpp`

---

## References

- Implementation: `../../src/llm/`
- Deployment plugin guide: `../../docs/en/llm/llm_deployment_plugin.md`
