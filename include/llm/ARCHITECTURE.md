> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/llm/ARCHITECTURE.md -->

# LLM Module — Public Header Architecture

**Module Path:** `include/llm/`
**Implementation:** `../../src/llm/`
**Canonical architecture doc:** [`../../src/llm/ARCHITECTURE.md`](../../src/llm/ARCHITECTURE.md)

---

## 1. Overview

`include/llm/` defines the **public LLM inference, serving, adapter management, training, safety, and GPU-memory API contract** for ThemisDB. The 97 headers cover llama.cpp-backed and enhanced inference engines, paged KV-cache, speculative decoding, mixed-precision, LoRA adapter lifecycle, multi-GPU memory management, federated inference, prompt management and safety, OpenAI-compatible adapters, model storage, quantisation, fine-tuning, ethics and constitution reasoning, vision, streaming, and AI orchestration.

For runtime composition — inference engine internals, LoRA hot-swap, paged-allocator mechanics, and GPU memory coordinator — see:
→ [`../../src/llm/ARCHITECTURE.md`](../../src/llm/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Inference Engines and Backends

| Header | Public Type | Purpose |
|--------|------------|---------|
| `llamacpp_inference_engine.h` | `LlamaCppInferenceEngine` | llama.cpp-backed inference engine |
| `inference_engine_enhanced.h` | `InferenceEngineEnhanced` | Feature-extended inference engine with plugin support |
| `async_inference_engine.h` | `AsyncInferenceEngine` | Non-blocking async inference with future/callback API |
| `embedded_llm.h` | `EmbeddedLLM` | In-process embedded LLM for low-latency local inference |
| `mixed_precision_inference.h` | `MixedPrecisionInference` | FP16/BF16/INT8 mixed-precision inference |
| `speculative_decoder.h` | `SpeculativeDecoder` | Draft-model speculative decoding for throughput improvement |
| `inference_handle.h` | `InferenceHandle` | Cancellable handle to an in-flight inference request |

### 2.2 KV Cache and Memory

| Header | Public Type | Purpose |
|--------|------------|---------|
| `paged_kv_cache.h` | `PagedKVCache` | vLLM-style paged key-value cache |
| `paged_kv_cache_manager.h` | `PagedKVCacheManager` | Lifecycle and eviction policy for paged KV cache |
| `paged_block_manager.h` | `PagedBlockManager` | Physical block allocation for paged KV cache |
| `block_table.h` | `BlockTable` | Logical-to-physical block mapping |
| `kv_cache_buffer.h` | `KVCacheBuffer` | Fixed-size KV cache buffer |
| `kv_prefix_transfer_manager.h` | `KVPrefixTransferManager` | KV cache prefix sharing across sequences |
| `llm_prefix_cache.h` | `LLMPrefixCache` | Prefix-keyed prompt-cache for repeated context reuse |
| `llm_response_cache.h` | `LLMResponseCache` | Response-level cache for deterministic output reuse |

### 2.3 Batch Scheduling and Continuous Batching

| Header | Public Type | Purpose |
|--------|------------|---------|
| `continuous_batch_scheduler.h` | `ContinuousBatchScheduler` | vLLM-style continuous batching scheduler |
| `batch_generator.h` | `BatchGenerator` | Request batching and padding for inference |
| `context_window_budget.h` | `ContextWindowBudget` | Token-budget tracking per request context |
| `token_quota_manager.h` | `TokenQuotaManager` | Per-tenant token-usage quota enforcement |
| `shared_worker_pool.h` | `SharedWorkerPool` | Shared inference worker thread pool |

### 2.4 GPU Memory Management

| Header | Public Type | Purpose |
|--------|------------|---------|
| `gpu_memory_manager.h` | `GPUMemoryManager` | Multi-GPU VRAM allocation, tracking, and health |
| `active_vram_allocator.h` | `ActiveVRAMAllocator` | Active-set VRAM allocation with eviction |
| `adaptive_vram_allocator.h` | `AdaptiveVRAMAllocator` | Load-adaptive VRAM allocation strategy |
| `multi_gpu_memory_coordinator.h` | `MultiGPUMemoryCoordinator` | Cross-GPU memory coordination and migration |
| `gpu_safe_fail.h` | `GPUSafeFailGuard` | Safe-fail guard for GPU operation errors |

### 2.5 LoRA Adapter Lifecycle

| Header | Public Type | Purpose |
|--------|------------|---------|
| `lora_router.h` | `LoRARouter` | Request-to-adapter routing |
| `adapter_registry.h` | `AdapterRegistry` | Registered adapter inventory and lookup |
| `adapter_deployment_manager.h` | `AdapterDeploymentManager` | Adapter hot-deploy and teardown orchestration |
| `adapter_load_balancer.h` | `AdapterLoadBalancer` | Load balancing across active adapter instances |
| `adapter_compatibility.h` | `AdapterCompatibility` | Base-model / adapter compatibility validation |
| `multi_lora_manager.h` | `MultiLoRAManager` | Concurrent multi-adapter serving |
| `lora_metadata_cache.h` | `LoRAMetadataCache` | Cached adapter metadata for fast routing |
| `lora_certificate_store.h` | `LoRACertificateStore` | Adapter signing-certificate storage |
| `lora_security_validator.h` | `LoRASecurityValidator` | Adapter signature and integrity validation |

### 2.6 Model Loading, Storage, and Quantisation

| Header | Public Type | Purpose |
|--------|------------|---------|
| `model_loader.h` | `ModelLoader` | Base model loading from disk/registry |
| `lazy_model_loader.h` | `LazyModelLoader` | Demand-loaded model for reduced startup latency |
| `gguf_loader.h` | `GGUFLoader` | GGUF-format model file loading |
| `gguf_st_adapter.h` | `GGUFSTAdapter` | SafeTensors ↔ GGUF conversion adapter |
| `model_downloader.h` | `ModelDownloader` | Authenticated model download from registries |
| `llm_model_storage.h` | `LLMModelStorage` | Persistent model storage and versioning |
| `model_metadata_cache.h` | `ModelMetadataCache` | Cached model metadata and capability registry |
| `model_quantization_pipeline.h` | `ModelQuantizationPipeline` | Post-training quantisation pipeline |
| `ml_model_manager.h` | `MLModelManager` | General ML model lifecycle management |
| `model_router.h` | `ModelRouter` | Request-to-model routing |

### 2.7 Prompt Management and Safety

| Header | Public Type | Purpose |
|--------|------------|---------|
| `prompt_manager.h` | `PromptManager` | Prompt template storage and rendering |
| `prompt_optimizer.h` | `PromptOptimizer` | Automated prompt optimisation |
| `prompt_evaluator.h` | `PromptEvaluator` | Prompt quality and safety evaluation |
| `prompt_policy.h` | `PromptPolicy` | Policy gates for allowed prompt patterns |
| `prompt_safety_utils.h` | `PromptSafetyUtils` | Prompt sanitisation and injection prevention |
| `meta_prompt_generator.h` | `MetaPromptGenerator` | Meta-prompting for adaptive generation |
| `fewshot_optimizer.h` | `FewshotOptimizer` | Few-shot example selection and optimisation |

### 2.8 Ethics, Safety, and Constitution

| Header | Public Type | Purpose |
|--------|------------|---------|
| `constitutional_reasoning_engine.h` | `ConstitutionalReasoningEngine` | Principle-guided constitutional AI reasoning |
| `ethical_guidelines_manager.h` | `EthicalGuidelinesManager` | Active ethics-guideline policy management |
| `ethics_aware_confidence_detector.h` | `EthicsAwareConfidenceDetector` | Confidence scoring with ethics-aware filtering |
| `moral_analyzer.h` | `MoralAnalyzer` | Moral-reasoning analysis over generated text |
| `ai_decision_auditor.h` | `AIDecisionAuditor` | Immutable audit log for AI decision events |
| `llm_model_audit_logger.h` | `LLMModelAuditLogger` | Model-level audit logging for compliance |
| `llm_security_utils.h` | `LLMSecurityUtils` | LLM-specific security utilities |

### 2.9 Grammar and Structured Decoding

| Header | Public Type | Purpose |
|--------|------------|---------|
| `grammar.h` | `Grammar` | CFG-constrained structured output grammar |
| `grammar_cache.h` | `GrammarCache` | Pre-compiled grammar cache |
| `json_schema_converter.h` | `JSONSchemaConverter` | JSON Schema to grammar conversion |
| `lookup_decoder.h` | `LookupDecoder` | Lookup-table constrained decoding |
| `sampling_strategy.h` | `SamplingStrategy` | Token-sampling strategy configuration |

### 2.10 Training and Inline Fine-Tuning

| Header | Public Type | Purpose |
|--------|------------|---------|
| `llamacpp_training_backend.h` | `LlamaCppTrainingBackend` | llama.cpp training backend integration |
| `inline_training_engine.h` | `InlineTrainingEngine` | In-process fine-tuning alongside inference |
| `distributed_training_coordinator.h` | `DistributedTrainingCoordinator` | Distributed training coordination |
| `training_data_iterator.h` | `TrainingDataIterator` | Iterator over training data batches |
| `multi_model_training_data.h` | `MultiModelTrainingData` | Shared training data for multi-model runs |
| `feedback_store.h` | `FeedbackStore` | Persistent feedback storage for RLHF |

### 2.11 Federated Inference

| Header | Public Type | Purpose |
|--------|------------|---------|
| `federated_inference_coordinator.h` | `FederatedInferenceCoordinator` | Cross-node federated inference orchestration |
| `i_federated_inference_backend.h` | `IFederatedInferenceBackend` | Backend abstraction for federated inference |
| `byzantine_detector.h` | `ByzantineDetector` | Byzantine-fault detection in federated inference |

### 2.12 Plugin and Extension

| Header | Public Type | Purpose |
|--------|------------|---------|
| `llm_plugin_interface.h` | `ILLMPlugin` | LLM plugin extension interface |
| `llm_plugin_manager.h` | `LLMPluginManager` | Plugin lifecycle and dispatch |
| `llm_deployment_plugin.h` | `LLMDeploymentPlugin` | Deployment-specific plugin hooks |
| `i_llm_plugin.h` | `ILLMPlugin` | Alternate plugin interface alias |
| `i_feedback_plugin.h` | `IFeedbackPlugin` | Feedback collection plugin interface |
| `themis_tool_interface.h` | `ThemisToolInterface` | Tool-call interface for agent/tool use |

### 2.13 Vision and Multimodal

| Header | Public Type | Purpose |
|--------|------------|---------|
| `vision_encoder.h` | `VisionEncoder` | Image-to-embedding encoder for vision-LLM |
| `vision_config.h` | `VisionConfig` | Vision-model configuration |
| `vision_resource_monitor.h` | `VisionResourceMonitor` | VRAM and compute monitoring for vision models |

### 2.14 Streaming, Monitoring, and Integration

| Header | Public Type | Purpose |
|--------|------------|---------|
| `streaming_handler.h` | `StreamingHandler` | Server-sent-events / websocket streaming handler |
| `grafana_metrics.h` | `GrafanaMetrics` | Grafana-compatible metrics emission |
| `llm_interaction_store.h` | `LLMInteractionStore` | Persistent interaction log for observability |
| `llm_ingestion_bridge.h` | `LLMIngestionBridge` | Bridge for LLM-augmented data ingestion |
| `openai_compat_adapter.h` | `OpenAICompatAdapter` | OpenAI-compatible REST API adapter |
| `llama_resource_manager.h` | `LlamaResourceManager` | llama.cpp context and resource lifecycle |
| `llama_wrapper.h` | `LlamaWrapper` | Thin wrapper over the llama.cpp C API |
| `ai_orchestrator.h` | `AIOrchestrator` | Top-level AI workflow orchestration |
| `docs_assistant.h` | `DocsAssistant` | Documentation assistant integration |
| `explanation_generator.h` | `ExplanationGenerator` | Human-readable explanation generation |
| `multi_perspective_generator.h` | `MultiPerspectiveGenerator` | Multi-viewpoint response generation |
| `aql_train_parser.h` | `AQLTrainParser` | AQL training-query parser for fine-tuning data |
| `decision_record_yaml_processor.h` | `DecisionRecordYAMLProcessor` | Decision-record YAML ingestion and processing |
| `kernel_fusion.h` / `kernel_fusion_cuda.h` | `KernelFusion`, `KernelFusionCUDA` | Operator kernel fusion for inference throughput |
| `production_validator.h` | `ProductionValidator` | Pre-deployment production-readiness validation |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::llm` | Core inference, serving, adapter, and memory types |
| `themis::llm::ethics` | Ethics, safety, and constitution types |
| `themis::llm::vision` | Vision-model and multimodal types |

---

## 4. Public Contract Notes

- Inference engine headers define stable request/response contracts; engine implementation and GGUF internals are opaque.
- Paged KV-cache headers expose allocation and eviction contracts; physical block layout remains internal.
- LoRA adapter headers define stable lifecycle and security contracts; hot-swap mechanics are implementation-internal.
- `IFederatedInferenceBackend` and `ILLMPlugin` define the public extension points for custom backends and plugins.
- Ethics and safety headers must enforce fail-closed behaviour for unsupported or policy-violating inputs.
- Grammar and structured-decoding headers define token-constraint contracts; CFG internals are opaque.
- OpenAI-compat adapter provides a stable wire-compatible contract mapped to internal inference APIs.
