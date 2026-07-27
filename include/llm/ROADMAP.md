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

## Breaking Change History

None in v1.x. LLM headers maintain backward compatibility within the active major line; inference engine API, KV-cache layout, and LoRA adapter format changes require migration notes and changelog updates.
