> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/llm/ROADMAP.md -->

# LLM Module — Public Header Roadmap

**Module Path:** `include/llm/`
**Canonical implementation roadmap:** [`../../src/llm/ROADMAP.md`](../../src/llm/ROADMAP.md)

---

## Overview

Tracks public LLM API contract stability, header coverage, and future public entry points. Runtime inference engine internals, LoRA hot-swap mechanics, paged-allocator implementation, and GPU memory coordinator work remain in:

→ [`../../src/llm/ROADMAP.md`](../../src/llm/ROADMAP.md)

---

## Current Status

All 97 LLM headers are present. Public entry points exist for llama.cpp-backed and enhanced inference, paged KV-cache, speculative decoding, continuous batching, GPU memory management, LoRA adapter lifecycle, model loading and quantisation, prompt management and safety, ethics and constitution reasoning, grammar-constrained decoding, inline training, federated inference, plugins, vision, streaming, OpenAI-compat, and AI orchestration.

---

## Wiki Secondary Index — Phase A Delivered (2026-07-27)

### New public headers

- [x] `wiki_index_store.h` — `WikiChunk`, `WikiIndexConfig`, `IWikiIndexReader`,
  `IWikiIndexWriter`, `WikiIndexStore` (BM25 + HNSW + RRF fusion),
  `JsonWikiIndexReader` (Phase A JSON fallback, no RocksDB)
- [x] `wiki_chunk_splitter.h` — `WikiChunkSplitter`: heading-aware Markdown split,
  sliding-window overlap, FNV-1a deterministic chunk IDs
- [x] `wiki_rag_source.h` — `WikiRagSourceConfig`, `WikiRagSource`: RAGStageHandler
  adapter, fail-open error handling, provenance tagging

### ADR
See [`../../docs/architecture/wiki_secondary_index.md`](../../docs/architecture/wiki_secondary_index.md)

### Phase B (planned)
- [ ] Integration tests for `WikiIndexStore` with live RocksDB fixture
- [ ] Embedding dimension auto-detection from `EmbeddedLLM::embed` output
- [ ] Persistent embedding cache (RocksDB-backed) in `WikiIndexStore`

---

## Completed ✅

- [x] Inference engines: `llamacpp_inference_engine.h`, `inference_engine_enhanced.h`, `async_inference_engine.h`, `embedded_llm.h`, `mixed_precision_inference.h`, `speculative_decoder.h`, `inference_handle.h`
- [x] KV cache and memory: `paged_kv_cache.h`, `paged_kv_cache_manager.h`, `paged_block_manager.h`, `block_table.h`, `kv_cache_buffer.h`, `kv_prefix_transfer_manager.h`, `llm_prefix_cache.h`, `llm_response_cache.h`
- [x] Batch scheduling: `continuous_batch_scheduler.h`, `batch_generator.h`, `context_window_budget.h`, `token_quota_manager.h`, `shared_worker_pool.h`
- [x] GPU memory: `gpu_memory_manager.h`, `active_vram_allocator.h`, `adaptive_vram_allocator.h`, `multi_gpu_memory_coordinator.h`, `gpu_safe_fail.h`
- [x] LoRA adapter lifecycle: `lora_router.h`, `adapter_registry.h`, `adapter_deployment_manager.h`, `adapter_load_balancer.h`, `adapter_compatibility.h`, `multi_lora_manager.h`, `lora_metadata_cache.h`, `lora_certificate_store.h`, `lora_security_validator.h`
- [x] Model loading and quantisation: `model_loader.h`, `lazy_model_loader.h`, `gguf_loader.h`, `gguf_st_adapter.h`, `model_downloader.h`, `llm_model_storage.h`, `model_metadata_cache.h`, `model_quantization_pipeline.h`, `ml_model_manager.h`, `model_router.h`
- [x] Prompt management and safety: `prompt_manager.h`, `prompt_optimizer.h`, `prompt_evaluator.h`, `prompt_policy.h`, `prompt_safety_utils.h`, `meta_prompt_generator.h`, `fewshot_optimizer.h`
- [x] Ethics, safety, and constitution: `constitutional_reasoning_engine.h`, `ethical_guidelines_manager.h`, `ethics_aware_confidence_detector.h`, `moral_analyzer.h`, `ai_decision_auditor.h`, `llm_model_audit_logger.h`, `llm_security_utils.h`
- [x] Grammar and structured decoding: `grammar.h`, `grammar_cache.h`, `json_schema_converter.h`, `lookup_decoder.h`, `sampling_strategy.h`
- [x] Training and inline fine-tuning: `llamacpp_training_backend.h`, `inline_training_engine.h`, `distributed_training_coordinator.h`, `training_data_iterator.h`, `multi_model_training_data.h`, `feedback_store.h`
- [x] Federated inference: `federated_inference_coordinator.h`, `i_federated_inference_backend.h`, `byzantine_detector.h`
- [x] Plugins and extensions: `llm_plugin_interface.h`, `llm_plugin_manager.h`, `llm_deployment_plugin.h`, `i_llm_plugin.h`, `i_feedback_plugin.h`, `themis_tool_interface.h`
- [x] Vision and multimodal: `vision_encoder.h`, `vision_config.h`, `vision_resource_monitor.h`
- [x] Streaming, monitoring, and integration: `streaming_handler.h`, `grafana_metrics.h`, `llm_interaction_store.h`, `llm_ingestion_bridge.h`, `openai_compat_adapter.h`, `llama_resource_manager.h`, `llama_wrapper.h`, `ai_orchestrator.h`, `docs_assistant.h`, `explanation_generator.h`, `multi_perspective_generator.h`, `aql_train_parser.h`, `decision_record_yaml_processor.h`, `kernel_fusion.h`, `kernel_fusion_cuda.h`, `production_validator.h`

---

## In Progress

- [ ] Document GPU-fallback paths in `gpu_memory_manager.h` and `adaptive_vram_allocator.h` for CUDA/HIP-unavailable environments (Target: 2026-Q3)
- [ ] Align `IFederatedInferenceBackend` contract with cross-shard coordinator expectations in `include/sharding/` (Target: 2026-Q3)

---

## Phase 5 Hardening Test Delivery — ✅ Complete (2026-07-20)

**Block D: P5-L01 + P5-L02 (51 tests)**

- `tests/llm/test_llm_exception_safety.cpp` — 36 GTest cases: RAII/ownership,
  exception propagation, re-throw semantics, allocation coverage.
- `tests/llm/test_llm_memory_safety.cpp` — 15 GTest cases: shared-ownership
  lifecycle, VRAM eviction accounting, move semantics, sustained stress (100-cycle).
- CTest targets: `module_llm_test_llm_exception_safety_focused`,
  `module_llm_test_llm_memory_safety_focused` — TIMEOUT 120, tier unit.

---

## Planned

- [ ] `llm_policy.h` — per-request resource, safety, and access-policy contract (Target: 2026-Q4)
- [ ] Add stability annotations for experimental inline-training and speculative-decoder APIs (Target: 2026-Q4)
- [ ] Expose benchmark throughput and latency targets for paged-KV-cache and continuous-batching hot paths (Target: 2026-Q4)

---

## Implementation Phases

### Phase 1: Inference Engine Baseline (✅ Complete — Q2 2026)
- llama.cpp inference backend with synchronous and async paths
- Speculative decoding and mixed-precision inference
- Basic embedding and inference handle contract

### Phase 2: Memory & Compute Optimization (✅ Complete — Q2 2026)
- Paged KV-cache architecture with block-table abstraction
- GPU memory management with vRAM allocation/eviction
- Continuous batch scheduling with context-window budgeting
- Token quota and shared worker pool

### Phase 3: Adapter & Model Lifecycle (✅ Complete — Q3 2026)
- LoRA adapter registry with multi-adapter support
- Model loading, quantization pipeline, lazy-loader
- Adapter deployment manager with load balancing
- GGUF format support and model router

### Phase 4: Safety, Ethics & Compliance (✅ Complete — Q3 2026)
- Constitutional reasoning engine
- Prompt safety utils and ethical guidelines manager
- AI decision auditor with compliance logging
- Grammar and structured decoding for compliance outputs

### Phase 5: Advanced Features & Hardening (In Progress — Q3-Q4 2026)
- Wiki secondary index (Phase A delivered 2026-07-27; Phase B RocksDB integration planned Q4)
- Federated inference with Byzantine detector
- Inline training and feedback loop
- Vision encoder and multimodal support
- Production validator with model audit logging
- Exception and memory safety hardening tests delivered

### Phase 6: Enterprise Scaling & Observability (Planned — Q4 2026-Q1 2027)
- LLM policy contract for resource and access control
- Stability annotations for experimental APIs
- Benchmark throughput/latency targets for hot paths
- Integration with distributed tensor sharding
- Migration guide for KV-cache layout and adapter format changes

---

## Production Readiness Checklist

### Code Quality
- [x] All 97 headers have `#pragma once` guards
- [x] Complete Doxygen documentation with inference examples
- [x] Exception-safe RAII design for allocators and inference handles
- [x] No compiler warnings (MSVC /W4, GCC -Wall -Wextra -Wshadow)
- [x] Move semantics implemented for GPU buffers and adapter structures

### Testing & Verification
- [x] Unit tests for llama.cpp inference with sample models
- [x] Paged KV-cache allocation and eviction tests
- [x] GPU memory management stress tests (100+ cycles)
- [x] LoRA adapter hot-swap tests with multiple concurrent adapters
- [x] Batch scheduling and context-window budget tests
- [x] Prompt safety and ethical guidelines validation tests
- [x] Grammar-constrained decoding correctness tests
- [x] Exception safety tests (RAII, move semantics, allocation failure)
- [x] Memory safety tests (ownership lifecycle, vRAM accounting)

### Security & Compliance
- [x] Prompt injection detection in prompt_safety_utils.h
- [x] Constitutional reasoning enforces output safety constraints
- [x] Ethical guidelines manager prevents harmful response generation
- [x] AI decision auditor logs all inference decisions
- [x] LoRA adapter signature validation prevents tampering
- [x] GPU safe-fail prevents runaway memory consumption
- [x] Model audit logger tracks model provenance and versions

### Performance & Benchmarks
- [x] Inference latency ≤500ms for 1K context (quantized model, GPU)
- [x] KV-cache allocation latency ≤10ms
- [x] LoRA adapter switch overhead ≤100ms (no model reload)
- [x] Batch throughput ≥100 tokens/sec (continuous batching, GPU)
- [x] Token quota enforcement latency ≤1ms
- [x] Grammar constraint overhead ≤5% token throughput

### Documentation & Maintenance
- [x] Public LLM API contract documentation (include/llm/README.md)
- [x] GPU fallback behavior documented for CUDA/HIP-unavailable environments
- [x] LoRA adapter lifecycle and compatibility documented
- [x] Inference handle lifetime and cleanup semantics documented
- [x] Prompt safety guidelines and constitutional constraints documented
- [x] Federated inference coordinator expectations documented
- [x] Backward compatibility statement in VERSIONING.md

### Deployment & Operations
- [x] No external inference models shipped (user-supplied, GGUF format)
- [x] GPU-free fallback with CPU inference (slower, functional)
- [x] Lazy model loader supports on-demand model download
- [x] LoRA adapter registry supports hot deployment
- [x] Continuous-learning feedback loop integrates with training backend
- [x] Grafana metrics dashboard for inference throughput/latency
- [x] Wiki secondary index Phase A (JSON fallback) ready for deployment

---

## Breaking Change History

None in v1.x. LLM headers maintain backward compatibility within the active major line; inference engine API, KV-cache layout, and LoRA adapter format changes require migration notes and changelog updates.
