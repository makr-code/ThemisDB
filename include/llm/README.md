> **Build:** `cmake --preset release && cmake --build build/release`

# LLM Module Headers

<!-- Status: current | validated: 2026-04-09 | Primary: include/llm/ | Secondary: docs/de/llm/ -->
<!-- Links: ../../src/llm/README.md · FUTURE_ENHANCEMENTS.md · ../../docs/de/llm/README.md -->

This directory contains header files (.h, .hpp) for the llm module.

## Purpose

Public interfaces and declarations for LLM inference, model management, and optimization features.

## Key Components

### Inference Engines

- **`inference_handle.h`**: Shared handle for tracking async inference requests (NEW in v1.15.0)
- **`async_inference_engine.h`**: Simple async wrapper for single LLM plugin
- **`inference_engine_enhanced.h`**: Advanced multi-model engine with caching, batching, and load balancing

### Model Management

- **`llama_wrapper.h`**: Core llama.cpp plugin implementing ILLMPlugin interface
- **`llm_plugin_interface.h`**: Base interface for all LLM plugins
- **`llm_plugin_manager.h`**: Registry and lifecycle management for plugins
- **`model_loader.h`**: Model loading and validation

### Optimization Features

- **`llm_prefix_cache.h`**: Context caching for faster inference
- **`paged_kv_cache.h`**: Paged attention KV cache
- **`continuous_batch_scheduler.h`**: Dynamic batching for improved throughput

### LoRA Fine-tuning

- **`multi_lora_manager.h`**: Multi-LoRA adapter management
- **`lora_router.h`**: Automatic adapter routing

## Architecture Note

ThemisDB provides **two independent inference engines** serving different needs:

1. **AsyncInferenceEngine**: Lightweight, single-model, for simple API calls
2. **InferenceEngineEnhanced**: Enterprise-grade, multi-model, with advanced features

See [`../../src/llm/README.md`](../../src/llm/README.md) for detailed architecture documentation.

## Public API by Concern

- **Inference engines:** `async_inference_engine.h`, `inference_engine_enhanced.h`, `inference_handle.h`, `llamacpp_inference_engine.h`
- **Model management:** `model_loader.h`, `model_router.h`, `model_downloader.h`, `model_metadata_cache.h`, `gguf_loader.h`, `lazy_model_loader.h`
- **LoRA adapter management:** `adapter_registry.h`, `adapter_load_balancer.h`, `adapter_deployment_manager.h`, `adapter_compatibility.h`, `lora_router.h`, `multi_lora_manager.h`, `lora_metadata_cache.h`, `lora_security_validator.h`, `lora_certificate_store.h`
- **KV-cache and batching:** `paged_kv_cache.h`, `paged_kv_cache_manager.h`, `paged_block_manager.h`, `block_table.h`, `kv_cache_buffer.h`, `kv_prefix_transfer_manager.h`, `llm_prefix_cache.h`, `llm_response_cache.h`, `continuous_batch_scheduler.h`
- **VRAM / GPU resource management:** `active_vram_allocator.h`, `adaptive_vram_allocator.h`, `gpu_memory_manager.h`, `multi_gpu_memory_coordinator.h`, `vision_resource_monitor.h`, `gpu_safe_fail.h`
- **AI safety, ethics, and auditing:** `constitutional_reasoning_engine.h`, `ethical_guidelines_manager.h`, `ethics_aware_confidence_detector.h`, `moral_analyzer.h`, `ai_decision_auditor.h`
- **Prompt management:** `prompt_manager.h`, `prompt_optimizer.h`, `prompt_evaluator.h`, `prompt_policy.h`, `meta_prompt_generator.h`, `fewshot_optimizer.h`
- **Training and fine-tuning:** `inline_training_engine.h`, `distributed_training_coordinator.h`, `llamacpp_training_backend.h`, `aql_train_parser.h`, `training_data_iterator.h`, `multi_model_training_data.h`
- **Plugin and orchestration layer:** `i_llm_plugin.h`, `llm_plugin_interface.h`, `llm_plugin_manager.h`, `llm_deployment_plugin.h`, `ai_orchestrator.h`, `adapter_registry.h`
- **Grammar, sampling, and speculative decoding:** `grammar.h`, `grammar_cache.h`, `json_schema_converter.h`, `sampling_strategy.h`, `speculative_decoder.h`, `lookup_decoder.h`
- **Streaming, vision, and OpenAI compat:** `streaming_handler.h`, `openai_compat_adapter.h`, `vision_encoder.h`, `vision_config.h`
- **Metrics, security, and utility:** `grafana_metrics.h`, `llm_security_utils.h`, `token_quota_manager.h`, `llm_model_audit_logger.h`, `shared_worker_pool.h`, `context_window_budget.h`

## Runtime Configuration Surfaces

| Header | Key Configuration Types |
|---|---|
| `async_inference_engine.h` | Plugin registration, thread pool size, timeout, priority queue depth |
| `inference_engine_enhanced.h` | Multi-model backends, batch size, KV-cache size, load-balance policy |
| `model_router.h` | Regex/tag routing rules, fallback model |
| `gguf_loader.h` | Model file path (must be within trusted directory) |
| `adapter_registry.h` | LoRA adapter trusted base path, hot-load callback, certificate store |
| `active_vram_allocator.h` | VRAM capacity, LRU eviction threshold, CPU spill ratio |
| `token_quota_manager.h` | Per-tenant token budget, refill interval |
| `openai_compat_adapter.h` | Model-name mapping, streaming mode, function-calling schema |

## All Headers

| Header | Purpose |
|--------|---------|
| `active_vram_allocator.h` | Active VRAM allocation tracking |
| `adapter_compatibility.h` | Adapter compatibility checks |
| `adapter_deployment_manager.h` | Adapter deployment lifecycle manager |
| `adapter_load_balancer.h` | Load balancing across adapters |
| `adapter_registry.h` | Registry for LLM adapters |
| `adaptive_vram_allocator.h` | Adaptive VRAM allocation strategies |
| `ai_decision_auditor.h` | Audit logging for AI decisions |
| `ai_orchestrator.h` | Multi-agent AI orchestration |
| `aql_train_parser.h` | AQL training data parser |
| `async_inference_engine.h` | Async wrapper for single LLM plugin |
| `batch_generator.h` | Token batch generation utilities |
| `block_table.h` | Block table for paged attention |
| `byzantine_detector.h` | Byzantine fault detection for distributed inference |
| `constitutional_reasoning_engine.h` | Constitutional AI reasoning engine |
| `context_window_budget.h` | Context window budget management |
| `continuous_batch_scheduler.h` | Dynamic batching for improved throughput |
| `decision_record_yaml_processor.h` | YAML processor for decision records |
| `distributed_training_coordinator.h` | Coordinator for distributed training jobs |
| `docs_assistant.h` | Documentation assistant LLM interface |
| `embedded_llm.h` | Embedded LLM runtime |
| `ethical_guidelines_manager.h` | Ethical guidelines policy manager |
| `ethics_aware_confidence_detector.h` | Confidence detection with ethics awareness |
| `explanation_generator.h` | Generates human-readable explanations |
| `feedback_store.h` | Persistent store for inference feedback |
| `fewshot_optimizer.h` | Few-shot prompt optimizer |
| `gguf_loader.h` | GGUF model file loader |
| `gguf_st_adapter.h` | GGUF sentence-transformer adapter |
| `gpu_memory_manager.h` | GPU memory lifecycle manager |
| `gpu_safe_fail.h` | Safe GPU failure handling |
| `grafana_metrics.h` | Grafana metrics integration for LLM |
| `grammar.h` | Grammar-constrained generation |
| `grammar_cache.h` | Cache for compiled grammars |
| `i_feedback_plugin.h` | Feedback plugin interface |
| `i_llm_plugin.h` | Core LLM plugin interface |
| `inference_engine_enhanced.h` | Multi-model engine with caching and load balancing |
| `inference_handle.h` | Shared handle for async inference requests |
| `inline_training_engine.h` | Inline/online training engine |
| `json_schema_converter.h` | JSON schema to grammar converter |
| `kernel_fusion.h` | CPU kernel fusion utilities |
| `kernel_fusion_cuda.h` | CUDA kernel fusion utilities |
| `kv_cache_buffer.h` | KV cache buffer management |
| `lazy_model_loader.h` | Lazy/deferred model loader |
| `llama_resource_manager.h` | llama.cpp resource lifecycle manager |
| `llama_wrapper.h` | llama.cpp plugin implementing ILLMPlugin |
| `llamacpp_inference_engine.h` | llama.cpp-backed inference engine |
| `llamacpp_training_backend.h` | llama.cpp training backend |
| `llm_deployment_plugin.h` | LLM deployment plugin interface |
| `llm_ingestion_bridge.h` | Bridge between LLM and ingestion pipeline |
| `llm_interaction_store.h` | Persistent store for LLM interactions |
| `llm_model_audit_logger.h` | Audit logger for model operations |
| `llm_model_storage.h` | Model storage and retrieval |
| `llm_plugin_interface.h` | Base interface for all LLM plugins |
| `llm_plugin_manager.h` | Registry and lifecycle for plugins |
| `llm_prefix_cache.h` | Context caching for faster inference |
| `llm_response_cache.h` | Response-level cache |
| `llm_security_utils.h` | Security utilities for LLM input/output |
| `lora_certificate_store.h` | Certificate store for LoRA adapters |
| `lora_metadata_cache.h` | Metadata cache for LoRA adapters |
| `lora_router.h` | Automatic LoRA adapter routing |
| `lora_security_validator.h` | Security validator for LoRA adapters |
| `meta_prompt_generator.h` | Meta-prompt generation |
| `mixed_precision_inference.h` | Mixed-precision inference support |
| `ml_model_manager.h` | Generic ML model manager |
| `model_downloader.h` | Model download and verification |
| `model_loader.h` | Model loading and validation |
| `model_metadata_cache.h` | Cache for model metadata |
| `model_quantization_pipeline.h` | Model quantization pipeline |
| `model_router.h` | Multi-model routing logic |
| `moral_analyzer.h` | Moral/ethical content analyzer |
| `multi_gpu_memory_coordinator.h` | Multi-GPU memory coordination |
| `multi_lora_manager.h` | Multi-LoRA adapter management |
| `multi_model_training_data.h` | Training data for multi-model setups |
| `multi_perspective_generator.h` | Multi-perspective response generation |
| `openai_compat_adapter.h` | OpenAI-compatible API adapter |
| `paged_block_manager.h` | Block manager for paged attention |
| `paged_kv_cache.h` | Paged attention KV cache |
| `paged_kv_cache_manager.h` | Manager for paged KV cache instances |
| `production_validator.h` | Production-readiness validator |
| `prompt_evaluator.h` | Prompt quality evaluator |
| `prompt_manager.h` | Prompt template manager |
| `prompt_optimizer.h` | Prompt optimization engine |
| `prompt_policy.h` | Prompt policy enforcement |
| `sampling_strategy.h` | Token sampling strategies |
| `shared_worker_pool.h` | Shared worker thread pool for inference |
| `speculative_decoder.h` | Speculative decoding for faster generation |
| `streaming_handler.h` | Streaming response handler |
| `themis_tool_interface.h` | ThemisDB tool-calling interface for LLMs |
| `token_quota_manager.h` | Per-tenant token quota management |
| `training_data_iterator.h` | Iterator over training datasets |
| `vision_config.h` | Vision model configuration |
| `vision_encoder.h` | Vision encoder interface |
| `vision_resource_monitor.h` | Resource monitor for vision models |

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Implementation

See [`../../src/llm/`](../../src/llm/) for the implementation code.

## Usage

### C++: async inference with InferenceHandle

```cpp
#include "llm/async_inference_engine.h"

AsyncInferenceEngine engine;
engine.registerPlugin(my_llm_plugin);

auto handle = engine.submitRequest({
    .prompt = "Summarize the document.",
    .max_tokens = 512
});

auto result = handle.get();   // blocking wait
```

### C++: grammar-constrained output

```cpp
#include "llm/grammar.h"

Grammar grammar(R"(root ::= "yes" | "no")", "root");
if (!grammar.isValid()) {
    // llama.cpp grammar APIs unavailable — unconstrained fallback
}
```

## Runtime Behavior, Errors, and Limits

- LoRA adapter paths and model IDs **must** be validated against a trusted directory before use; unchecked paths cause path-injection vulnerabilities (see AUDIT.md F1-1/F2-1).
- Grammar-constrained generation silently falls back to unconstrained sampling when the llama.cpp grammar API is absent.
- VRAM OOM is handled by LRU eviction and CPU spilling in `ActiveVRAMAllocator`; callers receive `nullptr` when spill capacity is exhausted.
- Speculative decoding requires draft and target models to share vocabulary.

## Troubleshooting

- **LoRA hot-load fails**: validate the adapter path is inside the configured trusted directory.
- **`grammar.isValid()` returns false**: rebuild llama.cpp with grammar API support.
- **VRAM OOM**: reduce batch size or context window; check `ActiveVRAMAllocator` limits.
- **OpenAI adapter 500**: verify `ModelRouter` routing rules cover the requested model name.

## See Also

- [`../../src/llm/README.md`](../../src/llm/README.md) — implementation details and usage
- [`../../src/llm/ARCHITECTURE.md`](../../src/llm/ARCHITECTURE.md) — engine design and component diagram
- [`../../src/llm/SECURITY.md`](../../src/llm/SECURITY.md) — threat model and path-injection mitigations
- [`../../src/llm/AUDIT.md`](../../src/llm/AUDIT.md) — S0/S1/S2 findings and resolution status
- [`../../src/llm/CHANGELOG.md`](../../src/llm/CHANGELOG.md) — module history
- [`../../src/llm/PERFORMANCE_EXPECTATIONS.md`](../../src/llm/PERFORMANCE_EXPECTATIONS.md) — benchmark targets
- [`../../src/llm/ROADMAP.md`](../../src/llm/ROADMAP.md) — implementation status and planned work
- [`../../src/llm/FUTURE_ENHANCEMENTS.md`](../../src/llm/FUTURE_ENHANCEMENTS.md) — long-term backlog
- [`../../docs/en/llm/PRIMARY_SOURCES.md`](../../docs/en/llm/PRIMARY_SOURCES.md) — canonical source index (EN)
- [`../../docs/de/llm/PRIMARY_SOURCES.md`](../../docs/de/llm/PRIMARY_SOURCES.md) — Quellenindex (DE)
