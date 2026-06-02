# llm Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: llm
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 2661
- Actionable Findings (Critical + High): 2114
- Affected Files: 147

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 694 |
| High | 1420 |
| Medium | 546 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| llm_ai_safety | 2034 |
| performance_patterns | 457 |
| concurrency | 367 |
| container | 360 |
| reliability | 229 |
| memory | 168 |
| raii | 153 |
| exception_safety | 130 |
| performance | 77 |
| audit_logging | 71 |
| security | 66 |
| determinism | 47 |
| platform | 42 |
| gpu_memory_safety | 39 |
| legacy_duplication | 37 |
| observability | 34 |
| distributed_consistency | 22 |
| type_conversion | 16 |
| input_validation | 12 |
| uninitialized | 7 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/llm/lora_framework/kernels/vulkan_kernels.cpp | 162 | 81 | 81 | 0 | 0 |
| src/llm/lora_framework/gpu_lora_layers.cpp | 159 | 75 | 83 | 1 | 0 |
| src/llm/llama_wrapper.cpp | 141 | 31 | 97 | 13 | 0 |
| src/llm/inference_engine_enhanced.cpp | 131 | 0 | 111 | 20 | 0 |
| src/llm/lora_framework/flash_lora.cpp | 111 | 48 | 63 | 0 | 0 |
| src/llm/async_inference_engine.cpp | 109 | 1 | 104 | 4 | 0 |
| src/llm/lora_framework/kernels/directx_kernels.cpp | 80 | 38 | 42 | 0 | 0 |
| src/llm/lora_framework/lora_training_service.cpp | 77 | 34 | 35 | 8 | 0 |
| src/llm/grafana_metrics.cpp | 68 | 1 | 62 | 5 | 0 |
| src/llm/multi_lora_manager.cpp | 67 | 3 | 38 | 26 | 0 |
| src/llm/lora_framework/kernels/hip_fused_kernels.cpp | 61 | 31 | 30 | 0 | 0 |
| src/llm/ml_model_manager.cpp | 59 | 10 | 40 | 9 | 0 |
| src/llm/lora_framework/kernels/cpu_fused_kernels.cpp | 52 | 26 | 26 | 0 | 0 |
| src/llm/lora_framework/data_loader.cpp | 51 | 21 | 22 | 8 | 0 |
| src/llm/lora_framework/kernels/hip_kernels.cpp | 46 | 21 | 22 | 3 | 0 |
| src/llm/lora_framework/multi_gpu_lora_layer.cpp | 46 | 8 | 28 | 10 | 0 |
| src/llm/aql_train_parser.cpp | 45 | 20 | 20 | 5 | 0 |
| src/llm/production_validator.cpp | 45 | 4 | 34 | 7 | 0 |
| src/llm/vision_config.cpp | 45 | 1 | 11 | 33 | 0 |
| src/llm/lora_framework/lora_layers.cpp | 44 | 22 | 22 | 0 | 0 |
| src/llm/lora_framework/quantization.cpp | 43 | 24 | 19 | 0 | 0 |
| src/llm/kernel_fusion.cpp | 41 | 22 | 19 | 0 | 0 |
| src/llm/gpu_memory_manager.cpp | 40 | 11 | 10 | 19 | 0 |
| src/llm/lora_framework/quantized_model.cpp | 36 | 17 | 16 | 3 | 0 |
| src/llm/lora_framework/multi_gpu_trainer.cpp | 33 | 5 | 20 | 8 | 0 |
| src/llm/fewshot_optimizer.cpp | 32 | 10 | 13 | 9 | 0 |
| src/llm/distributed_training_coordinator.cpp | 26 | 2 | 6 | 18 | 0 |
| src/llm/llm_plugin_manager.cpp | 26 | 12 | 9 | 5 | 0 |
| src/llm/model_loader.cpp | 26 | 20 | 3 | 3 | 0 |
| src/llm/lora_security_validator.cpp | 25 | 10 | 4 | 11 | 0 |
| src/llm/vision_resource_monitor.cpp | 25 | 0 | 24 | 1 | 0 |
| src/llm/lora_framework/lora_audit_logger.cpp | 24 | 0 | 23 | 1 | 0 |
| src/llm/docs_assistant.cpp | 23 | 0 | 10 | 12 | 1 |
| src/llm/federated_inference_coordinator.cpp | 22 | 0 | 19 | 3 | 0 |
| src/llm/lora_framework/base_model_adapter.cpp | 22 | 6 | 8 | 8 | 0 |
| src/llm/lora_framework/gpu_data_loader.cpp | 22 | 9 | 9 | 4 | 0 |
| src/llm/ethical_guidelines_manager.cpp | 21 | 0 | 4 | 17 | 0 |
| src/llm/mixed_precision_inference.cpp | 20 | 0 | 20 | 0 | 0 |
| src/llm/multi_perspective_generator.cpp | 20 | 0 | 0 | 20 | 0 |
| src/llm/applications/themis_help_lora.cpp | 19 | 5 | 10 | 4 | 0 |
| src/llm/llm_model_storage.cpp | 19 | 11 | 7 | 1 | 0 |
| src/llm/byzantine_detector.cpp | 18 | 0 | 0 | 18 | 0 |
| src/llm/llm_model_audit_logger.cpp | 17 | 0 | 16 | 1 | 0 |
| src/llm/embedded_llm.cpp | 16 | 2 | 11 | 3 | 0 |
| src/llm/safety/classifier.cpp | 16 | 0 | 15 | 1 | 0 |
| src/llm/moral_analyzer.cpp | 15 | 0 | 2 | 13 | 0 |
| src/llm/multi_gpu_memory_coordinator.cpp | 15 | 0 | 2 | 13 | 0 |
| src/llm/lora_framework/lora_training_config.cpp | 14 | 0 | 0 | 14 | 0 |
| src/llm/active_vram_allocator.cpp | 13 | 1 | 10 | 2 | 0 |
| src/llm/adapter_registry.cpp | 13 | 1 | 2 | 10 | 0 |
| src/llm/ai_orchestrator.cpp | 13 | 1 | 9 | 3 | 0 |
| src/llm/llm_deployment_plugin.cpp | 12 | 8 | 0 | 4 | 0 |
| src/llm/explanation_generator.cpp | 11 | 4 | 4 | 3 | 0 |
| src/llm/json_schema_converter.cpp | 11 | 0 | 0 | 11 | 0 |
| src/llm/lora_framework/distributed_dataloader.cpp | 10 | 1 | 2 | 7 | 0 |
| src/llm/lora_framework/lora_storage_service_themisdb.cpp | 10 | 5 | 0 | 5 | 0 |
| src/llm/lora_framework/mixed_precision.cpp | 10 | 5 | 5 | 0 | 0 |
| src/llm/prompt_evaluator.cpp | 10 | 0 | 5 | 5 | 0 |
| src/llm/continuous_batch_scheduler.cpp | 9 | 0 | 8 | 1 | 0 |
| src/llm/embedded_llm_stub.cpp | 9 | 0 | 4 | 5 | 0 |
| src/llm/ethics_aware_confidence_detector.cpp | 9 | 0 | 2 | 7 | 0 |
| src/llm/adapter_load_balancer.cpp | 8 | 0 | 1 | 7 | 0 |
| src/llm/lora_framework/adapter_sync_manager.cpp | 8 | 0 | 1 | 7 | 0 |
| src/llm/lora_framework/adaptive_batcher.cpp | 8 | 4 | 4 | 0 | 0 |
| src/llm/lora_framework/gradient_checkpointing.cpp | 8 | 4 | 4 | 0 | 0 |
| src/llm/lora_framework/lora_provenance.cpp | 8 | 0 | 8 | 0 | 0 |
| src/llm/openai_compat_adapter.cpp | 8 | 0 | 4 | 4 | 0 |
| src/llm/lora_framework/vram_allocator.cpp | 7 | 3 | 4 | 0 | 0 |
| src/llm/mode_spec_loader.cpp | 7 | 0 | 1 | 6 | 0 |
| src/llm/model_quantization_pipeline.cpp | 7 | 0 | 0 | 7 | 0 |
| src/llm/constitutional_reasoning_engine.cpp | 6 | 0 | 3 | 3 | 0 |
| src/llm/llama_resource_manager.cpp | 6 | 0 | 1 | 5 | 0 |
| src/llm/llm_response_cache.cpp | 6 | 0 | 4 | 2 | 0 |
| src/llm/lora_router.cpp | 6 | 2 | 2 | 2 | 0 |
| src/llm/vision_encoder.cpp | 6 | 0 | 6 | 0 | 0 |
| src/llm/llama_lora_adapter.cpp | 5 | 0 | 5 | 0 | 0 |
| src/llm/llm_ingestion_bridge.cpp | 5 | 0 | 2 | 3 | 0 |
| src/llm/lora_framework/lora_storage_service.cpp | 5 | 1 | 0 | 4 | 0 |
| src/llm/lora_framework/sequence_packer.cpp | 5 | 0 | 0 | 5 | 0 |
| src/llm/sampling_strategy.cpp | 5 | 0 | 2 | 3 | 0 |
| src/llm/inference_handle.cpp | 4 | 0 | 4 | 0 | 0 |
| src/llm/llamacpp_inference_engine.cpp | 4 | 0 | 2 | 2 | 0 |
| src/llm/lora_framework/directx_pipeline.cpp | 4 | 1 | 3 | 0 | 0 |
| src/llm/lora_framework/gpu_training_loop.cpp | 4 | 0 | 4 | 0 | 0 |
| src/llm/meta_prompt_generator.cpp | 4 | 2 | 2 | 0 | 0 |
| src/llm/paged_kv_cache_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/llm/prompt_manager.cpp | 4 | 0 | 0 | 4 | 0 |
| src/llm/safety/guardian.cpp | 4 | 0 | 0 | 4 | 0 |
| src/llm/attention/kv_cache_manager.cpp | 3 | 0 | 0 | 3 | 0 |
| src/llm/llm_prefix_cache.cpp | 3 | 0 | 3 | 0 | 0 |
| src/llm/lora_framework/distributed_trainer.cpp | 3 | 1 | 1 | 1 | 0 |
| src/llm/lora_framework/embedding_provider.cpp | 3 | 0 | 1 | 2 | 0 |
| src/llm/lora_framework/feedback_plugin.cpp | 3 | 0 | 0 | 3 | 0 |
| src/llm/lora_framework/gpu_memory.cpp | 3 | 0 | 0 | 3 | 0 |
| src/llm/lora_framework/gradient_utils.cpp | 3 | 0 | 0 | 3 | 0 |
| src/llm/lora_framework/lora_orchestrator.cpp | 3 | 0 | 0 | 3 | 0 |
| src/llm/lora_framework/model_compatibility.cpp | 3 | 0 | 2 | 1 | 0 |
| src/llm/lora_framework/paged_optimizer.cpp | 3 | 0 | 0 | 3 | 0 |
| src/llm/lora_framework/vulkan_context.cpp | 3 | 1 | 2 | 0 | 0 |
| src/llm/lora_framework/vulkan_pipeline.cpp | 3 | 1 | 1 | 1 | 0 |
| src/llm/paged_block_manager.cpp | 3 | 0 | 2 | 1 | 0 |
| src/llm/shared_worker_pool.cpp | 3 | 0 | 2 | 1 | 0 |
| src/llm/attention/flash_attention.cpp | 2 | 0 | 2 | 0 | 0 |
| src/llm/block_table.cpp | 2 | 0 | 0 | 2 | 0 |
| src/llm/feedback_store.cpp | 2 | 1 | 1 | 0 | 0 |
| src/llm/inline_training_engine.cpp | 2 | 0 | 0 | 2 | 0 |
| src/llm/lora_framework/directx_descriptors.cpp | 2 | 0 | 0 | 2 | 0 |
| src/llm/lora_framework/gguf_converter.cpp | 2 | 0 | 0 | 2 | 0 |
| src/llm/lora_framework/llama_tokenizer.cpp | 2 | 2 | 0 | 0 | 0 |
| src/llm/lora_framework/paged_memory_manager.cpp | 2 | 0 | 2 | 0 | 0 |
| src/llm/lora_framework/vulkan_buffer.cpp | 2 | 1 | 1 | 0 | 0 |
| src/llm/mcp_tool_bridge.cpp | 2 | 1 | 1 | 0 | 0 |
| src/llm/model_downloader.cpp | 2 | 1 | 0 | 1 | 0 |
| src/llm/model_router.cpp | 2 | 0 | 0 | 2 | 0 |
| src/llm/safety/monitoring.cpp | 2 | 0 | 0 | 2 | 0 |
| src/llm/security/signature_verifier.cpp | 2 | 0 | 2 | 0 | 0 |
| src/llm/speculative_decoder.cpp | 2 | 0 | 2 | 0 | 0 |
| src/llm/adaptive_vram_allocator.cpp | 1 | 0 | 1 | 0 | 0 |
| src/llm/ai_decision_auditor.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/decision_record_yaml_processor.cpp | 1 | 0 | 1 | 0 | 0 |
| src/llm/feedback_plugin_basic.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/gguf_loader.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/grammar.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/kv_cache_buffer.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/kv_prefix_transfer_manager.cpp | 1 | 0 | 1 | 0 | 0 |
| src/llm/llama_grammar_adapter.cpp | 1 | 0 | 1 | 0 | 0 |
| src/llm/lookup_decoder.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/lora_framework/adapter_consistency_checker.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/lora_framework/custom_allreduce.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/lora_framework/gpu_embedding_layer.cpp | 1 | 0 | 1 | 0 | 0 |
| src/llm/lora_framework/lora_checkpoint_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/lora_framework/multi_gpu.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/lora_framework/resource_profiler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/llm/prompt_optimizer.cpp | 1 | 0 | 1 | 0 | 0 |
| src/llm/streaming_handler.cpp | 1 | 0 | 1 | 0 | 0 |
| src/llm/gpu_safe_fail.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/llm_interaction_store.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/llm_security_utils.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/lora_certificate_store.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/lora_framework/directx_buffer.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/lora_framework/directx_context.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/lora_framework/directx_shader.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/lora_framework/gpu_tensor.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/lora_framework/lora_feedback_storage.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/lora_framework/nccl_backend.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/lora_framework/rccl_backend.cpp | 0 | 0 | 0 | 0 | 0 |
| src/llm/paged_kv_cache.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/llm/lora_framework/kernels/vulkan_kernels.cpp
Total findings: 162

- Line 95: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t size_input = 0;
  Confidence: band=very_high; score=0.99
- Line 101: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::unique_ptr<VulkanBuffer> buf_input;
  Confidence: band=very_high; score=0.99
- Line 109: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_size,
  Confidence: band=very_high; score=0.99
- Line 117: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: (size_input == input_size) &&
  Confidence: band=very_high; score=0.99
- Line 123: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (same_context && same_sizes && buf_input && buf_B && buf_A && buf_h && buf_output) {
  Confidence: band=very_high; score=0.99
- Line 128: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_input = input_size;
  Confidence: band=very_high; score=0.99
- Line 134: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buf_input = std::make_unique<VulkanBuffer>(ctx, size_input, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.99
- Line 145: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t size_input = 0;
  Confidence: band=very_high; score=0.99
- Line 153: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t size_input_t = 0;
  Confidence: band=very_high; score=0.99
- Line 157: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t size_grad_input = 0;
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::unique_ptr<VulkanBuffer> buf_input;
  Confidence: band=very_high; score=0.99
- Line 167: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::unique_ptr<VulkanBuffer> buf_input_t;
  Confidence: band=very_high; score=0.99
- Line 171: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::unique_ptr<VulkanBuffer> buf_grad_input;
  Confidence: band=very_high; score=0.99
- Line 175: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_size,
  Confidence: band=very_high; score=0.99
- Line 183: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_t_size,
  Confidence: band=very_high; score=0.99
- Line 187: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t grad_input_size) {
  Confidence: band=very_high; score=0.99
- Line 191: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: (size_input == input_size) &&
  Confidence: band=very_high; score=0.99
- Line 199: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: (size_input_t == input_t_size) &&
  Confidence: band=very_high; score=0.99
- Line 203: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: (size_grad_input == grad_input_size);
  Confidence: band=very_high; score=0.99
- Line 206: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buf_input && buf_B && buf_A && buf_grad_output &&
  Confidence: band=very_high; score=0.99
- Line 207: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buf_h && buf_grad_h && buf_a_t && buf_b_t && buf_input_t && buf_h_t &&
  Confidence: band=very_high; score=0.99
- Line 208: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buf_grad_A && buf_grad_B && buf_grad_input) {
  Confidence: band=very_high; score=0.99
- Line 213: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_input = input_size;
  Confidence: band=very_high; score=0.99
- Line 221: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_input_t = input_t_size;
  Confidence: band=very_high; score=0.99
- Line 225: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_grad_input = grad_input_size;
  Confidence: band=very_high; score=0.99
- Line 227: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buf_input = std::make_unique<VulkanBuffer>(ctx, size_input, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.99
- Line 235: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buf_input_t = std::make_unique<VulkanBuffer>(ctx, size_input_t, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.99
- Line 239: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buf_grad_input = std::make_unique<VulkanBuffer>(ctx, size_grad_input, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.99
- Line 454: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const VulkanBuffer& buf_input,
  Confidence: band=very_high; score=0.99
- Line 466: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_buffer(0, buf_input);
  Confidence: band=very_high; score=0.99
- Line 467: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Layout requires binding 1 for elementwise pipeline; reuse input as dummy.
  Confidence: band=very_high; score=0.99
- Line 468: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_buffer(1, buf_input);
  Confidence: band=very_high; score=0.99
- Line 669: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
  Confidence: band=very_high; score=0.99
- Line 672: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!input || !output) {
  Confidence: band=very_high; score=0.99
- Line 698: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: VulkanBuffer buf_input(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.99
- Line 701: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buf_input.upload(input, byte_size);
  Confidence: band=very_high; score=0.99
- Line 703: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_buffer(0, buf_input);
  Confidence: band=very_high; score=0.99
- Line 705: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_buffer(1, buf_input);
  Confidence: band=very_high; score=0.99
- Line 762: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Gradient shader bindings: input(0), B(1), A(2), grad_output(3), grad_A(4), grad_B(5), grad_input(6)
  Confidence: band=very_high; score=0.99
- Line 763: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // For grad_A computation we need: input(h), grad_output, grad_A
  Confidence: band=very_high; score=0.99
- Line 764: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_buffer(0, buf_h); // h serves as input
  Confidence: band=very_high; score=0.99
- Line 770: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_buffer(6, buf_grad_A); // dummy grad_input
  Confidence: band=very_high; score=0.99
- Line 784: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input, const float* grad_h, float* grad_B,
  Confidence: band=very_high; score=0.99
- Line 788: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!input || !grad_h || !grad_B) {
  Confidence: band=very_high; score=0.99
- Line 814: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t size_input = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(D), "launch_lora_grad_B_shader");
  Confidence: band=very_high; score=0.99
- Line 818: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: VulkanBuffer buf_input(&context, size_input, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.99
- Line 822: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buf_input.upload(input, size_input);
  Confidence: band=very_high; score=0.99
- Line 825: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // For grad_B computation we need: input, grad_h (as grad_output), grad_B
  Confidence: band=very_high; score=0.99
- Line 826: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_buffer(0, buf_input);
  Confidence: band=very_high; score=0.99
- Line 832: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_buffer(6, buf_grad_B); // dummy grad_input
  Confidence: band=very_high; score=0.99
- Line 902: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 908: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!output || !input) {
  Confidence: band=very_high; score=0.99
- Line 927: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t input_elems = checked_mul_size(
  Confidence: band=very_high; score=0.99
- Line 936: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: VulkanBuffer buf_input(&context, checked_mul_size(input_elems, sizeof(float), "launch_sequence_mean_shader"), VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.99
- Line 939: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buf_input.upload(input, checked_mul_size(input_elems, sizeof(float), "launch_sequence_mean_shader"));
  Confidence: band=very_high; score=0.99
- Line 940: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_buffer(0, buf_input);
  Confidence: band=very_high; score=0.99
- Line 952: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 963: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!input || !B || !A || !output) {
  Confidence: band=very_high; score=0.99
- Line 984: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t size_input = checked_float_bytes_2d(batch_size, in_dim, "launch_fused_lora_forward");
  Confidence: band=very_high; score=0.99
- Line 995: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_input,
  Confidence: band=very_high; score=0.99
- Line 1002: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cache.buf_input->upload(input, size_input);
  Confidence: band=very_high; score=0.99
- Line 1006: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: dispatch_matmul_device(pipeline, *cache.buf_input, *cache.buf_B, *cache.buf_h, batch_u, rank_u, in_u, 1.0f);
  Confidence: band=very_high; score=0.99
- Line 1014: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 1020: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float* grad_input,
  Confidence: band=very_high; score=0.99
- Line 1028: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!input || !B || !A || !grad_output || !grad_A || !grad_B || !grad_input) {
  Confidence: band=very_high; score=0.99
- Line 1049: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t size_input = checked_float_bytes_2d(batch_size, in_dim, "launch_fused_lora_backward");
  Confidence: band=very_high; score=0.99
- Line 1057: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t size_input_t = checked_float_bytes_2d(in_dim, batch_size, "launch_fused_lora_backward");
  Confidence: band=very_high; score=0.99
- Line 1061: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t size_grad_input = checked_float_bytes_2d(batch_size, in_dim, "launch_fused_lora_backward");
  Confidence: band=very_high; score=0.99
- Line 1069: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_input,
  Confidence: band=very_high; score=0.99
- Line 1077: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_input_t,
  Confidence: band=very_high; score=0.99
- Line 1081: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_grad_input);
  Confidence: band=very_high; score=0.99
- Line 1084: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cache.buf_input->upload(input, size_input);
  Confidence: band=very_high; score=0.99
- Line 1089: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // h = input @ B
  Confidence: band=very_high; score=0.99
- Line 1090: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: dispatch_matmul_device(matmul_pipeline, *cache.buf_input, *cache.buf_B, *cache.buf_h, batch_u, rank_u, in_u, 1.0f);
  Confidence: band=very_high; score=0.99
- Line 1096: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // input^T and B^T
  Confidence: band=very_high; score=0.99
- Line 1097: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: dispatch_transpose_device(elementwise_pipeline, *cache.buf_input, *cache.buf_input_t, batch_u, in_u);
  Confidence: band=very_high; score=0.99
- Line 1105: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_B = input^T @ grad_h
  Confidence: band=very_high; score=0.99
- Line 1106: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: dispatch_matmul_device(matmul_pipeline, *cache.buf_input_t, *cache.buf_grad_h, *cache.buf_grad_B, in_u, rank_u, batch_u, 1.0f);
  Confidence: band=very_high; score=0.99
- Line 1107: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_input = grad_h @ B^T
  Confidence: band=very_high; score=0.99
- Line 1108: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: dispatch_matmul_device(matmul_pipeline, *cache.buf_grad_h, *cache.buf_b_t, *cache.buf_grad_input, batch_u, in_u, rank_u, 1.0f);
  Confidence: band=very_high; score=0.99
- Line 1113: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cache.buf_grad_input->download(grad_input, size_grad_input);
  Confidence: band=very_high; score=0.99
- Line 95: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t size_input = 0;
  Confidence: band=very_high; score=0.9
- Line 101: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::unique_ptr<VulkanBuffer> buf_input;
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_size,
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: (size_input == input_size) &&
  Confidence: band=very_high; score=0.9
- Line 123: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (same_context && same_sizes && buf_input && buf_B && buf_A && buf_h && buf_output) {
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_input = input_size;
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buf_input = std::make_unique<VulkanBuffer>(ctx, size_input, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.9
- Line 145: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t size_input = 0;
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t size_input_t = 0;
  Confidence: band=very_high; score=0.9
- Line 157: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t size_grad_input = 0;
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::unique_ptr<VulkanBuffer> buf_input;
  Confidence: band=very_high; score=0.9
- Line 167: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::unique_ptr<VulkanBuffer> buf_input_t;
  Confidence: band=very_high; score=0.9
- Line 171: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::unique_ptr<VulkanBuffer> buf_grad_input;
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_size,
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_t_size,
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t grad_input_size) {
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: (size_input == input_size) &&
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: (size_input_t == input_t_size) &&
  Confidence: band=very_high; score=0.9
- Line 203: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: (size_grad_input == grad_input_size);
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buf_input && buf_B && buf_A && buf_grad_output &&
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buf_h && buf_grad_h && buf_a_t && buf_b_t && buf_input_t && buf_h_t &&
  Confidence: band=very_high; score=0.9
- Line 208: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buf_grad_A && buf_grad_B && buf_grad_input) {
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_input = input_size;
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_input_t = input_t_size;
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_grad_input = grad_input_size;
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buf_input = std::make_unique<VulkanBuffer>(ctx, size_input, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buf_input_t = std::make_unique<VulkanBuffer>(ctx, size_input_t, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.9
- Line 239: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buf_grad_input = std::make_unique<VulkanBuffer>(ctx, size_grad_input, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.9
- Line 454: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const VulkanBuffer& buf_input,
  Confidence: band=very_high; score=0.9
- Line 466: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_buffer(0, buf_input);
  Confidence: band=very_high; score=0.9
- Line 467: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Layout requires binding 1 for elementwise pipeline; reuse input as dummy.
  Confidence: band=very_high; score=0.9
- Line 468: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_buffer(1, buf_input);
  Confidence: band=very_high; score=0.9
- Line 669: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
  Confidence: band=very_high; score=0.9
- Line 672: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!input || !output) {
  Confidence: band=very_high; score=0.9
- Line 698: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: VulkanBuffer buf_input(&context, byte_size, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.9
- Line 701: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buf_input.upload(input, byte_size);
  Confidence: band=very_high; score=0.9
- Line 703: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_buffer(0, buf_input);
  Confidence: band=very_high; score=0.9
- Line 705: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_buffer(1, buf_input);
  Confidence: band=very_high; score=0.9
- Line 762: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Gradient shader bindings: input(0), B(1), A(2), grad_output(3), grad_A(4), grad_B(5), grad_input(6)
  Confidence: band=very_high; score=0.9
- Line 763: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // For grad_A computation we need: input(h), grad_output, grad_A
  Confidence: band=very_high; score=0.9
- Line 764: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_buffer(0, buf_h); // h serves as input
  Confidence: band=very_high; score=0.9
- Line 770: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_buffer(6, buf_grad_A); // dummy grad_input
  Confidence: band=very_high; score=0.9
- Line 784: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input, const float* grad_h, float* grad_B,
  Confidence: band=very_high; score=0.9
- Line 788: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!input || !grad_h || !grad_B) {
  Confidence: band=very_high; score=0.9
- Line 814: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t size_input = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(D), "launch_lora_grad_B_shader");
  Confidence: band=very_high; score=0.9
- Line 818: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: VulkanBuffer buf_input(&context, size_input, VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.9
- Line 822: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buf_input.upload(input, size_input);
  Confidence: band=very_high; score=0.9
- Line 825: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // For grad_B computation we need: input, grad_h (as grad_output), grad_B
  Confidence: band=very_high; score=0.9
- Line 826: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_buffer(0, buf_input);
  Confidence: band=very_high; score=0.9
- Line 832: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_buffer(6, buf_grad_B); // dummy grad_input
  Confidence: band=very_high; score=0.9
- Line 902: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 908: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!output || !input) {
  Confidence: band=very_high; score=0.9
- Line 927: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t input_elems = checked_mul_size(
  Confidence: band=very_high; score=0.9
- Line 936: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: VulkanBuffer buf_input(&context, checked_mul_size(input_elems, sizeof(float), "launch_sequence_mean_shader"), VulkanBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.9
- Line 939: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buf_input.upload(input, checked_mul_size(input_elems, sizeof(float), "launch_sequence_mean_shader"));
  Confidence: band=very_high; score=0.9
- Line 940: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_buffer(0, buf_input);
  Confidence: band=very_high; score=0.9
- Line 952: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 963: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!input || !B || !A || !output) {
  Confidence: band=very_high; score=0.9
- Line 984: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t size_input = checked_float_bytes_2d(batch_size, in_dim, "launch_fused_lora_forward");
  Confidence: band=very_high; score=0.9
- Line 995: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_input,
  Confidence: band=very_high; score=0.9
- Line 1002: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cache.buf_input->upload(input, size_input);
  Confidence: band=very_high; score=0.9
- Line 1006: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: dispatch_matmul_device(pipeline, *cache.buf_input, *cache.buf_B, *cache.buf_h, batch_u, rank_u, in_u, 1.0f);
  Confidence: band=very_high; score=0.9
- Line 1014: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 1020: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float* grad_input,
  Confidence: band=very_high; score=0.9
- Line 1028: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!input || !B || !A || !grad_output || !grad_A || !grad_B || !grad_input) {
  Confidence: band=very_high; score=0.9
- Line 1049: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t size_input = checked_float_bytes_2d(batch_size, in_dim, "launch_fused_lora_backward");
  Confidence: band=very_high; score=0.9
- Line 1057: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t size_input_t = checked_float_bytes_2d(in_dim, batch_size, "launch_fused_lora_backward");
  Confidence: band=very_high; score=0.9
- Line 1061: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t size_grad_input = checked_float_bytes_2d(batch_size, in_dim, "launch_fused_lora_backward");
  Confidence: band=very_high; score=0.9
- Line 1069: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_input,
  Confidence: band=very_high; score=0.9
- Line 1077: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_input_t,
  Confidence: band=very_high; score=0.9
- Line 1081: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_grad_input);
  Confidence: band=very_high; score=0.9
- Line 1084: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cache.buf_input->upload(input, size_input);
  Confidence: band=very_high; score=0.9
- Line 1089: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // h = input @ B
  Confidence: band=very_high; score=0.9
- Line 1090: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: dispatch_matmul_device(matmul_pipeline, *cache.buf_input, *cache.buf_B, *cache.buf_h, batch_u, rank_u, in_u, 1.0f);
  Confidence: band=very_high; score=0.9
- Line 1096: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // input^T and B^T
  Confidence: band=very_high; score=0.9
- Line 1097: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: dispatch_transpose_device(elementwise_pipeline, *cache.buf_input, *cache.buf_input_t, batch_u, in_u);
  Confidence: band=very_high; score=0.9
- Line 1105: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_B = input^T @ grad_h
  Confidence: band=very_high; score=0.9
- Line 1106: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: dispatch_matmul_device(matmul_pipeline, *cache.buf_input_t, *cache.buf_grad_h, *cache.buf_grad_B, in_u, rank_u, batch_u, 1.0f);
  Confidence: band=very_high; score=0.9
- Line 1107: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_input = grad_h @ B^T
  Confidence: band=very_high; score=0.9
- Line 1108: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: dispatch_matmul_device(matmul_pipeline, *cache.buf_grad_h, *cache.buf_b_t, *cache.buf_grad_input, batch_u, in_u, rank_u, 1.0f);
  Confidence: band=very_high; score=0.9
- Line 1113: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cache.buf_grad_input->download(grad_input, size_grad_input);
  Confidence: band=very_high; score=0.9

### src/llm/lora_framework/gpu_lora_layers.cpp
Total findings: 159

- Line 79: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: GPUTensor GPULoRALayer::forward(const GPUTensor& input) {
  Confidence: band=very_high; score=0.99
- Line 91: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Gradient checkpointing: Only cache input if NOT checkpointing
  Confidence: band=very_high; score=0.99
- Line 94: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cached_input_ = input.clone();
  Confidence: band=very_high; score=0.99
- Line 105: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto output = FlashLoRA::forward(input, B_T, A_T, scaling_);
  Confidence: band=very_high; score=0.99
- Line 110: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // need to cache intermediate h = input @ B. This partially defeats
  Confidence: band=very_high; score=0.99
- Line 115: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cached_h_ = input.matmul(*B_);
  Confidence: band=very_high; score=0.99
- Line 131: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto batch_size = input.shape()[0];
  Confidence: band=very_high; score=0.99
- Line 136: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // All GPU tensors are allocated with cudaMalloc which provides proper alignment
  Confidence: band=very_high; score=0.99
- Line 137: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: assert(performance::is_aligned<alignof(float)>(input.data()) &&
  Confidence: band=very_high; score=0.99
- Line 138: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.99
- Line 146: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input_ptr = reinterpret_cast<const float*>(input.data());
  Confidence: band=very_high; score=0.99
- Line 152: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_ptr, B_ptr, A_ptr, output_ptr,
  Confidence: band=very_high; score=0.99
- Line 158: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cached_h_ = input.matmul(*B_);
  Confidence: band=very_high; score=0.99
- Line 169: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto batch_size = input.shape()[0];
  Confidence: band=very_high; score=0.99
- Line 175: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: assert(performance::is_aligned<alignof(float)>(input.data()) &&
  Confidence: band=very_high; score=0.99
- Line 176: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input_ptr = reinterpret_cast<const float*>(input.data());
  Confidence: band=very_high; score=0.99
- Line 190: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_ptr, B_ptr, A_ptr, output_ptr,
  Confidence: band=very_high; score=0.99
- Line 196: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cached_h_ = input.matmul(*B_);
  Confidence: band=very_high; score=0.99
- Line 206: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto batch_size = input.shape()[0];
  Confidence: band=very_high; score=0.99
- Line 220: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto input_host = input.download();
  Confidence: band=very_high; score=0.99
- Line 227: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_host.data(),
  Confidence: band=very_high; score=0.99
- Line 240: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cached_h_ = input.matmul(*B_);
  Confidence: band=very_high; score=0.99
- Line 252: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Forward: output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.99
- Line 253: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Step 1: h = input @ B
  Confidence: band=very_high; score=0.99
- Line 254: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: GPUTensor h = input.matmul(*B_);
  Confidence: band=very_high; score=0.99
- Line 283: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (cached_input_.shape().empty()) {
  Confidence: band=very_high; score=0.99
- Line 292: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (cached_input_.shape().empty()) {
  Confidence: band=very_high; score=0.99
- Line 293: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: throw std::runtime_error("No cached input for Vulkan fused backward pass");
  Confidence: band=very_high; score=0.99
- Line 299: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: GPUTensor grad_input({batch_size, in_dim_}, device_);
  Confidence: band=very_high; score=0.99
- Line 312: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto input_host = cached_input_.download();
  Confidence: band=very_high; score=0.99
- Line 319: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> grad_input_host(batch_size * in_dim_);
  Confidence: band=very_high; score=0.99
- Line 323: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_host.data(),
  Confidence: band=very_high; score=0.99
- Line 329: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_input_host.data(),
  Confidence: band=very_high; score=0.99
- Line 338: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_input.upload(grad_input_host);
  Confidence: band=very_high; score=0.99
- Line 339: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return grad_input;
  Confidence: band=very_high; score=0.99
- Line 354: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: GPUTensor grad_input({batch_size, in_dim_}, device_);
  Confidence: band=very_high; score=0.99
- Line 358: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // All GPU tensors (including gradients) are allocated with cudaMalloc
  Confidence: band=very_high; score=0.99
- Line 359: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: assert(performance::is_aligned<alignof(float)>(input_for_backward.data()) &&
  Confidence: band=very_high; score=0.99
- Line 360: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.99
- Line 371: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: assert(performance::is_aligned<alignof(float)>(grad_input.data()) &&
  Confidence: band=very_high; score=0.99
- Line 372: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Grad input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.99
- Line 374: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input_ptr = reinterpret_cast<const float*>(input_for_backward.data());
  Confidence: band=very_high; score=0.99
- Line 380: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float* grad_input_ptr = reinterpret_cast<float*>(grad_input.data());
  Confidence: band=very_high; score=0.99
- Line 383: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_ptr, B_ptr, A_ptr, grad_output_ptr,
  Confidence: band=very_high; score=0.99
- Line 384: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_A_ptr, grad_B_ptr, grad_input_ptr,
  Confidence: band=very_high; score=0.99
- Line 388: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return grad_input;
  Confidence: band=very_high; score=0.99
- Line 402: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: GPUTensor grad_input({batch_size, in_dim_}, device_);
  Confidence: band=very_high; score=0.99
- Line 407: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: assert(performance::is_aligned<alignof(float)>(input_for_backward.data()) &&
  Confidence: band=very_high; score=0.99
- Line 408: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.99
- Line 419: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: assert(performance::is_aligned<alignof(float)>(grad_input.data()) &&
  Confidence: band=very_high; score=0.99
- Line 420: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Grad input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.99
- Line 422: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input_ptr = reinterpret_cast<const float*>(input_for_backward.data());
  Confidence: band=very_high; score=0.99
- Line 428: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float* grad_input_ptr = reinterpret_cast<float*>(grad_input.data());
  Confidence: band=very_high; score=0.99
- Line 431: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_ptr, B_ptr, A_ptr, grad_output_ptr,
  Confidence: band=very_high; score=0.99
- Line 432: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_A_ptr, grad_B_ptr, grad_input_ptr,
  Confidence: band=very_high; score=0.99
- Line 436: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return grad_input;
  Confidence: band=very_high; score=0.99
- Line 445: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: GPUTensor input_for_backward;
  Confidence: band=very_high; score=0.99
- Line 452: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Note: In a full implementation, the input should be saved by the
  Confidence: band=very_high; score=0.99
- Line 453: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // checkpointer. For now, we check if cached_input_ has data.
  Confidence: band=very_high; score=0.99
- Line 454: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (cached_input_.shape().empty()) {
  Confidence: band=very_high; score=0.99
- Line 456: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Checkpointing enabled but no input saved. "
  Confidence: band=very_high; score=0.99
- Line 461: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_for_backward = cached_input_.clone();
  Confidence: band=very_high; score=0.99
- Line 462: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: h_for_backward = input_for_backward.matmul(*B_);
  Confidence: band=very_high; score=0.99
- Line 465: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_for_backward = cached_input_.clone();
  Confidence: band=very_high; score=0.99
- Line 484: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_B = input^T @ (scaled_grad @ A^T)
  Confidence: band=very_high; score=0.99
- Line 487: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto input_t = input_for_backward.transpose();
  Confidence: band=very_high; score=0.99
- Line 489: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: *(B_->grad) = input_t.matmul(grad_h);
  Confidence: band=very_high; score=0.99
- Line 491: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_input = (scaled_grad @ A^T) @ B^T
  Confidence: band=very_high; score=0.99
- Line 493: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto grad_input = grad_h.matmul(B_t);
  Confidence: band=very_high; score=0.99
- Line 495: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return grad_input;
  Confidence: band=very_high; score=0.99
- Line 693: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float GPULoRATrainer::train_step(const GPUTensor& input, const GPUTensor& target) {
  Confidence: band=very_high; score=0.99
- Line 698: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto output = layer_->forward(input);
  Confidence: band=very_high; score=0.99
- Line 715: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float GPULoRATrainer::eval_step(const GPUTensor& input, const GPUTensor& target) {
  Confidence: band=very_high; score=0.99
- Line 717: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto output = layer_->forward(input);
  Confidence: band=very_high; score=0.99
- Line 79: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: GPUTensor GPULoRALayer::forward(const GPUTensor& input) {
  Confidence: band=very_high; score=0.9
- Line 80: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Verify input is on the same device
  Confidence: band=very_high; score=0.9
- Line 81: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.device() != device_) {
  Confidence: band=very_high; score=0.9
- Line 82: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: throw std::runtime_error("Input device mismatch in GPULoRALayer::forward");
  Confidence: band=very_high; score=0.9
- Line 84: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.shape().size() != 2) {
  Confidence: band=very_high; score=0.9
- Line 85: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: throw std::invalid_argument("GPULoRALayer::forward expects a 2D input tensor");
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.shape()[1] != in_dim_) {
  Confidence: band=very_high; score=0.9
- Line 88: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: throw std::invalid_argument("GPULoRALayer::forward input feature dimension mismatch");
  Confidence: band=very_high; score=0.9
- Line 91: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Gradient checkpointing: Only cache input if NOT checkpointing
  Confidence: band=very_high; score=0.9
- Line 94: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cached_input_ = input.clone();
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto output = FlashLoRA::forward(input, B_T, A_T, scaling_);
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // NOTE: For backward pass compatibility with existing code, we still
  Confidence: band=high; score=0.8
- Line 110: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // need to cache intermediate h = input @ B. This partially defeats
  Confidence: band=very_high; score=0.9
- Line 114: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // For now, we prioritize API compatibility and correctness.
  Confidence: band=high; score=0.8
- Line 115: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cached_h_ = input.matmul(*B_);
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto batch_size = input.shape()[0];
  Confidence: band=very_high; score=0.9
- Line 137: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: assert(performance::is_aligned<alignof(float)>(input.data()) &&
  Confidence: band=very_high; score=0.9
- Line 138: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input_ptr = reinterpret_cast<const float*>(input.data());
  Confidence: band=very_high; score=0.9
- Line 152: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_ptr, B_ptr, A_ptr, output_ptr,
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cached_h_ = input.matmul(*B_);
  Confidence: band=very_high; score=0.9
- Line 169: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto batch_size = input.shape()[0];
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: assert(performance::is_aligned<alignof(float)>(input.data()) &&
  Confidence: band=very_high; score=0.9
- Line 176: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input_ptr = reinterpret_cast<const float*>(input.data());
  Confidence: band=very_high; score=0.9
- Line 190: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_ptr, B_ptr, A_ptr, output_ptr,
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cached_h_ = input.matmul(*B_);
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto batch_size = input.shape()[0];
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input_host = input.download();
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_host.data(),
  Confidence: band=very_high; score=0.9
- Line 239: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Keep backward compatibility with existing checkpoint logic.
  Confidence: band=high; score=0.8
- Line 240: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cached_h_ = input.matmul(*B_);
  Confidence: band=very_high; score=0.9
- Line 252: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Forward: output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.9
- Line 253: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Step 1: h = input @ B
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: GPUTensor h = input.matmul(*B_);
  Confidence: band=very_high; score=0.9
- Line 283: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (cached_input_.shape().empty()) {
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (cached_input_.shape().empty()) {
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: throw std::runtime_error("No cached input for Vulkan fused backward pass");
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: GPUTensor grad_input({batch_size, in_dim_}, device_);
  Confidence: band=very_high; score=0.9
- Line 312: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input_host = cached_input_.download();
  Confidence: band=very_high; score=0.9
- Line 319: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> grad_input_host(batch_size * in_dim_);
  Confidence: band=very_high; score=0.9
- Line 323: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_host.data(),
  Confidence: band=very_high; score=0.9
- Line 329: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_input_host.data(),
  Confidence: band=very_high; score=0.9
- Line 338: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_input.upload(grad_input_host);
  Confidence: band=very_high; score=0.9
- Line 339: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return grad_input;
  Confidence: band=very_high; score=0.9
- Line 354: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: GPUTensor grad_input({batch_size, in_dim_}, device_);
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: assert(performance::is_aligned<alignof(float)>(input_for_backward.data()) &&
  Confidence: band=very_high; score=0.9
- Line 360: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.9
- Line 371: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: assert(performance::is_aligned<alignof(float)>(grad_input.data()) &&
  Confidence: band=very_high; score=0.9
- Line 372: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Grad input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.9
- Line 374: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input_ptr = reinterpret_cast<const float*>(input_for_backward.data());
  Confidence: band=very_high; score=0.9
- Line 380: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float* grad_input_ptr = reinterpret_cast<float*>(grad_input.data());
  Confidence: band=very_high; score=0.9
- Line 383: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_ptr, B_ptr, A_ptr, grad_output_ptr,
  Confidence: band=very_high; score=0.9
- Line 384: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_A_ptr, grad_B_ptr, grad_input_ptr,
  Confidence: band=very_high; score=0.9
- Line 388: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return grad_input;
  Confidence: band=very_high; score=0.9
- Line 402: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: GPUTensor grad_input({batch_size, in_dim_}, device_);
  Confidence: band=very_high; score=0.9
- Line 407: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: assert(performance::is_aligned<alignof(float)>(input_for_backward.data()) &&
  Confidence: band=very_high; score=0.9
- Line 408: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: assert(performance::is_aligned<alignof(float)>(grad_input.data()) &&
  Confidence: band=very_high; score=0.9
- Line 420: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Grad input tensor must be float-aligned for GPU operations");
  Confidence: band=very_high; score=0.9
- Line 422: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input_ptr = reinterpret_cast<const float*>(input_for_backward.data());
  Confidence: band=very_high; score=0.9
- Line 428: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float* grad_input_ptr = reinterpret_cast<float*>(grad_input.data());
  Confidence: band=very_high; score=0.9
- Line 431: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_ptr, B_ptr, A_ptr, grad_output_ptr,
  Confidence: band=very_high; score=0.9
- Line 432: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_A_ptr, grad_B_ptr, grad_input_ptr,
  Confidence: band=very_high; score=0.9
- Line 436: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return grad_input;
  Confidence: band=very_high; score=0.9
- Line 445: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: GPUTensor input_for_backward;
  Confidence: band=very_high; score=0.9
- Line 452: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Note: In a full implementation, the input should be saved by the
  Confidence: band=very_high; score=0.9
- Line 453: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // checkpointer. For now, we check if cached_input_ has data.
  Confidence: band=very_high; score=0.9
- Line 454: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (cached_input_.shape().empty()) {
  Confidence: band=very_high; score=0.9
- Line 456: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Checkpointing enabled but no input saved. "
  Confidence: band=very_high; score=0.9
- Line 461: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_for_backward = cached_input_.clone();
  Confidence: band=very_high; score=0.9
- Line 462: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: h_for_backward = input_for_backward.matmul(*B_);
  Confidence: band=very_high; score=0.9
- Line 465: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_for_backward = cached_input_.clone();
  Confidence: band=very_high; score=0.9
- Line 484: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_B = input^T @ (scaled_grad @ A^T)
  Confidence: band=very_high; score=0.9
- Line 487: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input_t = input_for_backward.transpose();
  Confidence: band=very_high; score=0.9
- Line 489: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: *(B_->grad) = input_t.matmul(grad_h);
  Confidence: band=very_high; score=0.9
- Line 491: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_input = (scaled_grad @ A^T) @ B^T
  Confidence: band=very_high; score=0.9
- Line 493: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto grad_input = grad_h.matmul(B_t);
  Confidence: band=very_high; score=0.9
- Line 495: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return grad_input;
  Confidence: band=very_high; score=0.9
- Line 693: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float GPULoRATrainer::train_step(const GPUTensor& input, const GPUTensor& target) {
  Confidence: band=very_high; score=0.9
- Line 698: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto output = layer_->forward(input);
  Confidence: band=very_high; score=0.9
- Line 715: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float GPULoRATrainer::eval_step(const GPUTensor& input, const GPUTensor& target) {
  Confidence: band=very_high; score=0.9
- Line 717: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto output = layer_->forward(input);
  Confidence: band=very_high; score=0.9
- Line 582: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: momentum_buffers_.push_back(std::move(buffer));
  Confidence: band=high; score=0.74

### src/llm/llama_wrapper.cpp
Total findings: 141

- Line 5: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * PR History (last 5): #5402 W1-L04 follow-up 3: Anchor ... (2026-05-27) | #5205 fix(llm): harden LoRA input... (2026-05-23) | #2965 [llm] Implement multi-modal... (2026-03-12) | #2962 feat(llm): Implement JSON s... (2026-03-12) | #655 [RAG-GAP-P2] Implement LLM-... (2026-03-11)
  Confidence: band=very_high; score=0.99
- Line 344: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: unloadModel();
  Confidence: band=very_high; score=0.99
- Line 351: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool LlamaWrapper::loadModel(
  Confidence: band=very_high; score=0.99
- Line 358: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: transitionToState(WrapperState::LOADING, "loadModel() called for: " + model_path);
  Confidence: band=very_high; score=0.99
- Line 450: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* model = model_loader->getOrLoadModel(
  Confidence: band=very_high; score=0.99
- Line 519: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto metadata_opt = storage->loadModel(model_id);
  Confidence: band=very_high; score=0.99
- Line 537: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // The loadModelBlob() method in LLMModelStorage handles blob retrieval
  Confidence: band=very_high; score=0.99
- Line 538: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto blob_data_opt = storage->loadModelBlob(model_id);
  Confidence: band=very_high; score=0.99
- Line 549: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Step 3: Decryption is already handled by loadModelBlob()
  Confidence: band=very_high; score=0.99
- Line 550: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // The data returned from loadModelBlob() is already decrypted if encryption was enabled
  Confidence: band=very_high; score=0.99
- Line 581: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::error("loadModelFromThemisDB: model_id '{}' produces a path '{}' "
  Confidence: band=very_high; score=0.99
- Line 663: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Step 5: Load model using standard loadModel() method
  Confidence: band=very_high; score=0.99
- Line 666: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool load_success = loadModel(temp_model_path.string(), config);
  Confidence: band=very_high; score=0.99
- Line 694: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: void LlamaWrapper::unloadModel() {
  Confidence: band=very_high; score=0.99
- Line 719: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: model_loader->unloadModel(current_model_id_, true);
  Confidence: band=very_high; score=0.99
- Line 908: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (current_model_id_ != reload_model_id && !current_model_id_.empty()) {
  Confidence: band=very_high; score=0.99
- Line 911: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: reload_model_id, current_model_id_);
  Confidence: band=very_high; score=0.99
- Line 915: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::error("Lazy reload failed for model {}", reload_model_path);
  Confidence: band=very_high; score=0.99
- Line 951: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Route to vision pipeline when image inputs are provided.
  Confidence: band=very_high; score=0.99
- Line 1028: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* cached = model_loader->getOrLoadModel(
  Confidence: band=very_high; score=0.99
- Line 1051: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: "Call loadModel() with a valid model file before attempting inference. "
  Confidence: band=very_high; score=0.99
- Line 1398: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* cached = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
  Confidence: band=very_high; score=0.99
- Line 1518: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* cached = model_loader->getOrLoadModel(
  Confidence: band=very_high; score=0.99
- Line 1543: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // 1. Tokenize input text
  Confidence: band=very_high; score=0.99
- Line 2260: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: draft_model_ = llama_load_model_from_file(draft_path.c_str(), draft_params);
  Confidence: band=very_high; score=0.99
- Line 2408: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* cached = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
  Confidence: band=very_high; score=0.99
- Line 2680: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* cached = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
  Confidence: band=very_high; score=0.99
- Line 2697: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: "Call loadModel() with a valid model file before attempting inference. "
  Confidence: band=very_high; score=0.99
- Line 3207: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt += "USER: " + request.text_prompt + "\nASSISTANT:";
  Confidence: band=very_high; score=0.99
- Line 3225: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: response.error_message = "No model loaded. Call loadModel() first";
  Confidence: band=very_high; score=0.99
- Line 3285: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* cached_m = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * PR History (last 5): #5402 W1-L04 follow-up 3: Anchor ... (2026-05-27) | #5205 fix(llm): harden LoRA input... (2026-05-23) | #2965 [llm] Implement multi-modal... (2026-03-12) | #2962 feat(llm): Implement JSON s... (2026-03-12) | #655 [RAG-GAP-P2] Implement LLM-... (2026-03-11)
  Confidence: band=very_high; score=0.87
- Line 14: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/llamacpp_inference_engine.h"
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ─── llama.cpp runtime version compatibility check ───────────────────────
  Confidence: band=high; score=0.8
- Line 371: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Model loads on-demand during first inference
  Confidence: band=very_high; score=0.9
- Line 841: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Inference
  Confidence: band=very_high; score=0.9
- Line 844: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse LlamaWrapper::generate(const InferenceRequest& request) {
  Confidence: band=very_high; score=0.9
- Line 847: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest effective_request = request;
  Confidence: band=very_high; score=0.9
- Line 856: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics_collector_->recordInferenceFailure(
  Confidence: band=very_high; score=0.9
- Line 861: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Inference prompt blocked by policy rule '" + blocked_rule + "': " + blocked_reason);
  Confidence: band=very_high; score=0.9
- Line 870: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics_collector_->recordInferenceFailure(
  Confidence: band=very_high; score=0.9
- Line 889: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Check state before attempting inference
  Confidence: band=very_high; score=0.9
- Line 944: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = generateSpeculative(safe_request);
  Confidence: band=very_high; score=0.9
- Line 951: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Route to vision pipeline when image inputs are provided.
  Confidence: band=very_high; score=0.9
- Line 951: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Route to vision pipeline when image inputs are provided.
  Confidence: band=very_high; score=0.9
- Line 964: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: VisionResponse vision_resp = generateVision(vision_req);
  Confidence: band=very_high; score=0.9
- Line 968: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ? "Vision inference failed"
  Confidence: band=very_high; score=0.9
- Line 995: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Record cache hit in inference metrics too
  Confidence: band=very_high; score=0.9
- Line 997: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics_collector_->recordInferenceRequest(current_model_id_);
  Confidence: band=very_high; score=0.9
- Line 998: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics_collector_->recordInferenceSuccess(current_model_id_, 1.0); // Cached responses are ~1ms
  Confidence: band=very_high; score=0.9
- Line 1008: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("Regular inference error: {}", e.what());
  Confidence: band=very_high; score=0.9
- Line 1012: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Record inference request
  Confidence: band=very_high; score=0.9
- Line 1014: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics_collector_->recordInferenceRequest(current_model_id_);
  Confidence: band=very_high; score=0.9
- Line 1034: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics_collector_->recordInferenceFailure(current_model_id_, "model_load_failed");
  Confidence: band=very_high; score=0.9
- Line 1047: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Model and context must be loaded before inference
  Confidence: band=very_high; score=0.9
- Line 1051: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Call loadModel() with a valid model file before attempting inference. "
  Confidence: band=very_high; score=0.9
- Line 1056: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Real llama.cpp inference implementation
  Confidence: band=very_high; score=0.9
- Line 1130: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse response;
  Confidence: band=very_high; score=0.9
- Line 1245: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.text = detokenizeInternal(lctx, generated_tokens);
  Confidence: band=very_high; score=0.9
- Line 1246: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.tokens_generated = static_cast<int>(generated_tokens.size());
  Confidence: band=very_high; score=0.9
- Line 1253: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
  Confidence: band=very_high; score=0.9
- Line 1254: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.latency_ms = static_cast<int64_t>(response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 1256: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.tokens_per_second = (response.inference_time_ms > 0)
  Confidence: band=very_high; score=0.9
- Line 1257: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ? response.tokens_generated / (response.inference_time_ms / 1000.0f)
  Confidence: band=very_high; score=0.9
- Line 1354: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("Inference error: {}", e.what());
  Confidence: band=very_high; score=0.9
- Line 1364: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics_collector_->recordInferenceFailure(current_model_id_, "inference_exception");
  Confidence: band=very_high; score=0.9
- Line 1373: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest& request,
  Confidence: band=very_high; score=0.9
- Line 1481: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse LlamaWrapper::generateRAG(
  Confidence: band=very_high; score=0.9
- Line 1483: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest& request
  Confidence: band=very_high; score=0.9
- Line 1492: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest rag_request = request;
  Confidence: band=very_high; score=0.9
- Line 1495: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = generate(rag_request);
  Confidence: band=very_high; score=0.9
- Line 1543: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // 1. Tokenize input text
  Confidence: band=very_high; score=0.9
- Line 1665: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats["total_inferences"] = stats_.total_inferences;
  Confidence: band=very_high; score=0.9
- Line 1666: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats["total_tokens_generated"] = stats_.total_tokens_generated;
  Confidence: band=very_high; score=0.9
- Line 1668: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (stats_.total_inferences > 0) {
  Confidence: band=very_high; score=0.9
- Line 1669: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats["avg_inference_time_ms"] =
  Confidence: band=very_high; score=0.9
- Line 1670: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats_.total_inference_time_ms / stats_.total_inferences;
  Confidence: band=very_high; score=0.9
- Line 1671: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats["avg_tokens_per_inference"] =
  Confidence: band=very_high; score=0.9
- Line 1672: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: static_cast<double>(stats_.total_tokens_generated) / stats_.total_inferences;
  Confidence: band=very_high; score=0.9
- Line 1734: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest& request
  Confidence: band=very_high; score=0.9
- Line 1771: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LlamaWrapper::updateStatistics(const InferenceResponse& response) {
  Confidence: band=very_high; score=0.9
- Line 1772: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats_.total_inferences++;
  Confidence: band=very_high; score=0.9
- Line 1773: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats_.total_tokens_generated += response.tokens_generated;
  Confidence: band=very_high; score=0.9
- Line 1774: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats_.total_inference_time_ms += response.inference_time_ms;
  Confidence: band=very_high; score=0.9
- Line 2056: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (temperature > 0.0f && temperature != 1.0f) {
  Confidence: band=very_high; score=0.9
- Line 2118: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: json LlamaWrapper::formatAsMCPResponse(const InferenceResponse& response) {
  Confidence: band=very_high; score=0.9
- Line 2144: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string LlamaWrapper::formatAsSSE(const InferenceResponse& response) {
  Confidence: band=very_high; score=0.9
- Line 2150: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: json LlamaWrapper::formatAsJsonMarkdown(const InferenceResponse& response) {
  Confidence: band=very_high; score=0.9
- Line 2400: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse LlamaWrapper::generateSpeculative(const InferenceRequest& request) {
  Confidence: band=very_high; score=0.9
- Line 2459: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse response;
  Confidence: band=very_high; score=0.9
- Line 2627: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.text = detokenizeInternal(target_context, generated_tokens);
  Confidence: band=very_high; score=0.9
- Line 2628: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.tokens_generated = static_cast<int>(generated_tokens.size());
  Confidence: band=very_high; score=0.9
- Line 2634: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
  Confidence: band=very_high; score=0.9
- Line 2635: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.latency_ms = static_cast<int64_t>(response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 2637: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.tokens_per_second = (response.inference_time_ms > 0)
  Confidence: band=very_high; score=0.9
- Line 2638: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ? response.tokens_generated / (response.inference_time_ms / 1000.0f)
  Confidence: band=very_high; score=0.9
- Line 2670: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse LlamaWrapper::generateRegular(const InferenceRequest& request) {
  Confidence: band=very_high; score=0.9
- Line 2693: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Model and context must be loaded before inference
  Confidence: band=very_high; score=0.9
- Line 2697: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Call loadModel() with a valid model file before attempting inference. "
  Confidence: band=very_high; score=0.9
- Line 2702: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Real llama.cpp inference implementation
  Confidence: band=very_high; score=0.9
- Line 2749: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: throw std::runtime_error("Context handle became null before inference");
  Confidence: band=very_high; score=0.9
- Line 2822: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.text = detokenizeInternal(lctx, generated_tokens);
  Confidence: band=very_high; score=0.9
- Line 2823: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.tokens_generated = static_cast<int>(generated_tokens.size());
  Confidence: band=very_high; score=0.9
- Line 2829: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.inference_time_ms = std::chrono::duration<float, std::milli>(end_time - start_time).count();
  Confidence: band=very_high; score=0.9
- Line 2830: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.latency_ms = static_cast<int64_t>(response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 2832: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.tokens_per_second = (response.inference_time_ms > 0)
  Confidence: band=very_high; score=0.9
- Line 2833: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ? response.tokens_generated / (response.inference_time_ms / 1000.0f)
  Confidence: band=very_high; score=0.9
- Line 2846: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("Inference error: {}", e.what());
  Confidence: band=very_high; score=0.9
- Line 2948: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest& request,
  Confidence: band=very_high; score=0.9
- Line 2950: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::function<void(const InferenceResponse&)> callback
  Confidence: band=very_high; score=0.9
- Line 3268: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Create inference request for the text part of the prompt.
  Confidence: band=very_high; score=0.9
- Line 3269: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest inference_request;
  Confidence: band=very_high; score=0.9
- Line 3270: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_request.prompt = prompt;
  Confidence: band=very_high; score=0.9
- Line 3271: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_request.max_tokens = vision_request.max_tokens;
  Confidence: band=very_high; score=0.9
- Line 3272: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_request.temperature = vision_request.temperature;
  Confidence: band=very_high; score=0.9
- Line 3273: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_request.top_p = vision_request.top_p;
  Confidence: band=very_high; score=0.9
- Line 3274: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_request.top_k = vision_request.top_k;
  Confidence: band=very_high; score=0.9
- Line 3277: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // so that the model actually "sees" the visual content during inference.
  Confidence: band=very_high; score=0.9
- Line 3348: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "falling back to text-only inference with vision-formatted prompt");
  Confidence: band=very_high; score=0.9
- Line 3360: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto inference_response = generate(inference_request);
  Confidence: band=very_high; score=0.9
- Line 3365: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.text = inference_response.text;
  Confidence: band=very_high; score=0.9
- Line 3366: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.tokens_generated = inference_response.tokens_generated;
  Confidence: band=very_high; score=0.9
- Line 3367: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.inference_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
  Confidence: band=very_high; score=0.9
- Line 3370: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.model_name = inference_response.model_id;
  Confidence: band=very_high; score=0.9
- Line 3373: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Vision inference completed: {} tokens in {}ms ({}ms image encoding)",
  Confidence: band=very_high; score=0.9
- Line 3375: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.inference_time_ms,
  Confidence: band=very_high; score=0.9
- Line 3380: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.error_message = std::string("Vision inference failed: ") + e.what();
  Confidence: band=very_high; score=0.9
- Line 3381: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("Vision inference error: {}", e.what());
  Confidence: band=very_high; score=0.9
- Line 844: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: InferenceResponse LlamaWrapper::generate(const InferenceRequest& request) {
  Confidence: band=high; score=0.74
- Line 1463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.tokens.push_back(static_cast<int>(next_token));
  Confidence: band=high; score=0.74
- Line 1463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.tokens.push_back(static_cast<int>(next_token));
  Confidence: band=high; score=0.74
- Line 1495: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto response = generate(rag_request);
  Confidence: band=high; score=0.74
- Line 2030: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back({token_id, logits[token_id], 0.0f});
  Confidence: band=high; score=0.74
- Line 2520: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: draft_tokens.push_back(draft_token);
  Confidence: band=high; score=0.74
- Line 2561: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: token_probabilities.push_back(target_prob);
  Confidence: band=high; score=0.74
- Line 2671: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: // This is the existing generate() implementation extracted
  Confidence: band=high; score=0.74
- Line 2801: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: token_probabilities.push_back(token_prob);
  Confidence: band=high; score=0.74
- Line 3201: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: prompt += request.image_token + "\n";
  Confidence: band=high; score=0.74
- Line 3254: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: image_embeddings.push_back(std::move(embeddings));
  Confidence: band=high; score=0.74
- Line 3338: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: // We use the raw generate() path which re-evaluates the full prompt,
  Confidence: band=high; score=0.74
- Line 3360: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto inference_response = generate(inference_request);
  Confidence: band=high; score=0.74

### src/llm/inference_engine_enhanced.cpp
Total findings: 131

- Line 2: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * ThemisDB | File: inference_engine_enhanced.cpp | Version: 0.0.47 | Last Modified: 2026-06-01 13:17:04
  Confidence: band=very_high; score=0.9
- Line 10: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced::InferenceEngineEnhanced(const Config& config)
  Confidence: band=very_high; score=0.9
- Line 61: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Initializing Enhanced Inference Engine");
  Confidence: band=very_high; score=0.9
- Line 75: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: prefix_cache_ = std::make_unique<LLMPrefixCache>("inference_cache", cache_config);
  Confidence: band=very_high; score=0.9
- Line 106: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Enhanced Inference Engine initialized successfully");
  Confidence: band=very_high; score=0.9
- Line 143: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced::InferenceEngineEnhanced(
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ) : InferenceEngineEnhanced(config) {
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Enhanced Inference Engine will use shared worker pool ({} threads)",
  Confidence: band=very_high; score=0.9
- Line 155: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced::~InferenceEngineEnhanced() {
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::setRemoteExecutor(
  Confidence: band=very_high; score=0.9
- Line 167: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("InferenceEngineEnhanced: remote draft executor set "
  Confidence: band=very_high; score=0.9
- Line 171: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("InferenceEngineEnhanced: remote draft executor detached");
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::setFederatedBackend(
  Confidence: band=very_high; score=0.9
- Line 176: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<IFederatedInferenceBackend> backend)
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("InferenceEngineEnhanced: federated inference backend attached "
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("InferenceEngineEnhanced: federated inference backend detached");
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::setSelfRAGRetrievalCallback(SelfRAGRetrievalCallback cb) {
  Confidence: band=very_high; score=0.9
- Line 193: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::setSelfRAGCriticCallback(SelfRAGCriticCallback cb) {
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::setTargetLogitsFn(TargetLogitsFn fn) {
  Confidence: band=very_high; score=0.9
- Line 208: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::registerModel(
  Confidence: band=very_high; score=0.9
- Line 247: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::unregisterModel(const std::string& model_id) {
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<std::string> InferenceEngineEnhanced::getAvailableModels() const {
  Confidence: band=very_high; score=0.9
- Line 268: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::swapModel(
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::setAdapterRegistry(
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("InferenceEngineEnhanced: adapter registry attached for DRAFT model discovery");
  Confidence: band=very_high; score=0.9
- Line 296: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::loadLoRAAdapter(
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool InferenceEngineEnhanced::unloadLoRAAdapter(
  Confidence: band=very_high; score=0.9
- Line 388: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<LoRAInfo> InferenceEngineEnhanced::getLoadedLoRAAdapters() const {
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // (see llm_plugin_interface.h) — populate all three for compatibility.
  Confidence: band=high; score=0.8
- Line 409: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::setModelQuota(
  Confidence: band=very_high; score=0.9
- Line 423: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced::ModelResourceQuota InferenceEngineEnhanced::getModelQuota(
  Confidence: band=very_high; score=0.9
- Line 475: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Share the cancel token with the handle so InferenceHandle::cancel()
  Confidence: band=very_high; score=0.9
- Line 477: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return InferenceHandle(request.request_id, future, tracked->cancel_token);
  Confidence: band=very_high; score=0.9
- Line 480: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string InferenceEngineEnhanced::submitAsync(
  Confidence: band=very_high; score=0.9
- Line 481: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const EnhancedInferenceRequest& request,
  Confidence: band=very_high; score=0.9
- Line 482: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::function<void(const InferenceResponse&)> callback
  Confidence: band=very_high; score=0.9
- Line 519: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceHandle InferenceEngineEnhanced::submitStreaming(
  Confidence: band=very_high; score=0.9
- Line 520: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const EnhancedInferenceRequest& request,
  Confidence: band=very_high; score=0.9
- Line 552: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: [cb, fired_final](const InferenceResponse&) {
  Confidence: band=very_high; score=0.9
- Line 589: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Share the cancel token with the handle so InferenceHandle::cancel()
  Confidence: band=very_high; score=0.9
- Line 591: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return InferenceHandle(request.request_id, future, tracked->cancel_token);
  Confidence: band=very_high; score=0.9
- Line 594: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool InferenceEngineEnhanced::cancel(const std::string& request_id) {
  Confidence: band=very_high; score=0.9
- Line 620: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool InferenceEngineEnhanced::reprioritize(const std::string& request_id, int new_priority) {
  Confidence: band=very_high; score=0.9
- Line 638: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::clearCache() {
  Confidence: band=very_high; score=0.9
- Line 644: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Cleared inference cache");
  Confidence: band=very_high; score=0.9
- Line 647: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::prewarmCache(const std::vector<std::string>& common_prompts) {
  Confidence: band=very_high; score=0.9
- Line 679: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced::Statistics InferenceEngineEnhanced::getStatistics() const {
  Confidence: band=very_high; score=0.9
- Line 733: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: json InferenceEngineEnhanced::getDetailedMetrics() const {
  Confidence: band=very_high; score=0.9
- Line 805: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::start() {
  Confidence: band=very_high; score=0.9
- Line 815: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // processBatch() tasks to the shared pool.  Actual inference is
  Confidence: band=very_high; score=0.9
- Line 817: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // AsyncInferenceEngine.
  Confidence: band=very_high; score=0.9
- Line 819: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: &InferenceEngineEnhanced::batchCoordinatorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 820: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Enhanced Inference Engine started with shared worker pool "
  Confidence: band=very_high; score=0.9
- Line 827: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: &InferenceEngineEnhanced::workerLoop, this, i);
  Confidence: band=very_high; score=0.9
- Line 829: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Enhanced Inference Engine started with {} private workers",
  Confidence: band=very_high; score=0.9
- Line 835: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: &InferenceEngineEnhanced::timeoutMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 838: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::shutdown() {
  Confidence: band=very_high; score=0.9
- Line 843: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Shutting down Enhanced Inference Engine...");
  Confidence: band=very_high; score=0.9
- Line 861: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Enhanced Inference Engine shutdown complete");
  Confidence: band=very_high; score=0.9
- Line 864: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool InferenceEngineEnhanced::isRunning() const {
  Confidence: band=very_high; score=0.9
- Line 872: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::workerLoop(size_t worker_id) {
  Confidence: band=very_high; score=0.9
- Line 919: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::batchCoordinatorLoop() {
  Confidence: band=very_high; score=0.9
- Line 920: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("InferenceEngineEnhanced batch coordinator started");
  Confidence: band=very_high; score=0.9
- Line 989: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::checkAndHandleTimeouts() {
  Confidence: band=very_high; score=0.9
- Line 1006: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& id : timed_out) {
  Confidence: band=very_high; score=0.9
- Line 1016: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse timeout_response;
  Confidence: band=very_high; score=0.9
- Line 1039: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::processBatch(
  Confidence: band=very_high; score=0.9
- Line 1140: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest effective_request = req.base_request;
  Confidence: band=very_high; score=0.9
- Line 1168: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Execute inference — use speculative decoding when:
  Confidence: band=very_high; score=0.9
- Line 1181: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse response;
  Confidence: band=very_high; score=0.9
- Line 1195: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<IFederatedInferenceBackend> fed_backend;
  Confidence: band=very_high; score=0.9
- Line 1202: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("InferenceEngineEnhanced: delegating request '{}' "
  Confidence: band=very_high; score=0.9
- Line 1227: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("InferenceEngineEnhanced: all {} fan-out instances "
  Confidence: band=very_high; score=0.9
- Line 1234: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Skip local inference for fan-out requests.
  Confidence: band=very_high; score=0.9
- Line 1255: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "InferenceEngineEnhanced: self_rag enabled but no retrieval callback set");
  Confidence: band=very_high; score=0.9
- Line 1423: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response = plugin->generateRAG(*self_rag.rag_context, effective_request);
  Confidence: band=very_high; score=0.9
- Line 1425: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response = plugin->generate(effective_request);
  Confidence: band=very_high; score=0.9
- Line 1452: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // detected after inference completes.
  Confidence: band=very_high; score=0.9
- Line 1454: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: try { tracked->callback(InferenceResponse{}); } catch (...) {}
  Confidence: band=very_high; score=0.9
- Line 1487: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse error_response;
  Confidence: band=very_high; score=0.9
- Line 1593: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // but must not short-circuit model inference; fall through to normal generation.
  Confidence: band=very_high; score=0.9
- Line 1597: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse response;
  Confidence: band=very_high; score=0.9
- Line 1598: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.text = cached->generated_text;
  Confidence: band=very_high; score=0.9
- Line 1673: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<int> InferenceEngineEnhanced::estimateTokenSequence(const std::string& text) {
  Confidence: band=very_high; score=0.9
- Line 1689: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string InferenceEngineEnhanced::selectModel(const EnhancedInferenceRequest& request) {
  Confidence: band=very_high; score=0.9
- Line 1707: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("InferenceEngineEnhanced: content-routing rule '{}' selected model '{}'",
  Confidence: band=very_high; score=0.9
- Line 1713: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("InferenceEngineEnhanced: content-routing rule '{}' target '{}' "
  Confidence: band=very_high; score=0.9
- Line 1787: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::updateModelStats(
  Confidence: band=very_high; score=0.9
- Line 1802: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (info.avg_response_time_ms == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 1816: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::recordCacheHit(size_t tokens_saved) {
  Confidence: band=very_high; score=0.9
- Line 1822: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::recordCacheMiss() {
  Confidence: band=very_high; score=0.9
- Line 1827: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::recordBatchCompletion(size_t batch_size) {
  Confidence: band=very_high; score=0.9
- Line 1834: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (stats_.avg_batch_size == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 1846: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::recordRequestCompletion(
  Confidence: band=very_high; score=0.9
- Line 1854: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats_.total_tokens_generated += tokens_generated;
  Confidence: band=very_high; score=0.9
- Line 1870: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (model_latency == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 1905: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool InferenceEngineEnhanced::trySpeculativeGeneration(
  Confidence: band=very_high; score=0.9
- Line 1906: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest&     request,
  Confidence: band=very_high; score=0.9
- Line 1909: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse&          response
  Confidence: band=very_high; score=0.9
- Line 2013: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest draft_request = request;
  Confidence: band=very_high; score=0.9
- Line 2016: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: draft_result = draft_plugin->generateDraftTokens(
  Confidence: band=very_high; score=0.9
- Line 2078: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest one_tok_req = request;
  Confidence: band=very_high; score=0.9
- Line 2145: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string InferenceEngineEnhanced::generateRequestId() {
  Confidence: band=very_high; score=0.9
- Line 2160: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::addRoutingRule(const RoutingRule& rule) {
  Confidence: band=very_high; score=0.9
- Line 2164: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool InferenceEngineEnhanced::removeRoutingRule(const std::string& rule_id) {
  Confidence: band=very_high; score=0.9
- Line 2168: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<RoutingRule> InferenceEngineEnhanced::getRoutingRules() const {
  Confidence: band=very_high; score=0.9
- Line 2172: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceEngineEnhanced::clearRoutingRules() {
  Confidence: band=very_high; score=0.9
- Line 2180: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string InferenceEngineEnhanced::resolveDraftModelId(
  Confidence: band=very_high; score=0.9
- Line 2242: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "InferenceEngineEnhanced: auto-selected DRAFT model '{}' for target '{}'",
  Confidence: band=very_high; score=0.9
- Line 2249: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "InferenceEngineEnhanced: DRAFT adapter '{}' found in registry but not registered "
  Confidence: band=very_high; score=0.9
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_load.emplace_back(aid, entry.path, entry.scale);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available.push_back(id);
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.emplace_back(id, info.plugin);
  Confidence: band=high; score=0.74
- Line 367: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.emplace_back(id, info.plugin);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 825: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: worker_threads_.emplace_back(
  Confidence: band=high; score=0.74
- Line 999: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: timed_out.push_back(id);
  Confidence: band=high; score=0.74
- Line 999: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: timed_out.push_back(id);
  Confidence: band=high; score=0.74
- Line 1224: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: agg += "[" + fr.instance_id + ": " + fr.error + "] ";
  Confidence: band=high; score=0.74
- Line 1224: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: agg += "[" + fr.instance_id + ": " + fr.error + "] ";
  Confidence: band=high; score=0.74
- Line 1334: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rag_context.documents.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 1425: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: response = plugin->generate(effective_request);
  Confidence: band=high; score=0.74
- Line 1739: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available.push_back(id);
  Confidence: band=high; score=0.74
- Line 1756: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto least_loaded = available[0];
  Confidence: band=high; score=0.74
- Line 1770: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto fastest = available[0];
  Confidence: band=high; score=0.74
- Line 2000: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: draft_result.tokens.push_back(tid);
  Confidence: band=high; score=0.74
- Line 2010: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: // generateDraftTokens() calls generate() internally and maps text to token
  Confidence: band=high; score=0.74
- Line 2082: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: const auto tgt_resp = target_plugin->generate(one_tok_req);
  Confidence: band=high; score=0.74
- Line 2126: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: response = target_plugin->generate(request);
  Confidence: band=high; score=0.74
- Line 2237: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: // in this engine instance (so the engine can call plugin->generate()).
  Confidence: band=high; score=0.74

### src/llm/lora_framework/flash_lora.cpp
Total findings: 111

- Line 78: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const GPUTensor& input,
  Confidence: band=very_high; score=0.99
- Line 83: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return forward(input, B, A, scaling, Config{});
  Confidence: band=very_high; score=0.99
- Line 87: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const GPUTensor& input,
  Confidence: band=very_high; score=0.99
- Line 105: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto input_shape = input.shape();
  Confidence: band=very_high; score=0.99
- Line 108: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input_shape.size() == 2) {
  Confidence: band=very_high; score=0.99
- Line 110: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch_size = input_shape[0];
  Confidence: band=very_high; score=0.99
- Line 112: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: in_dim = input_shape[1];
  Confidence: band=very_high; score=0.99
- Line 113: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: } else if (input_shape.size() == 3) {
  Confidence: band=very_high; score=0.99
- Line 115: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch_size = input_shape[0];
  Confidence: band=very_high; score=0.99
- Line 116: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: seq_len = input_shape[1];
  Confidence: band=very_high; score=0.99
- Line 117: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: in_dim = input_shape[2];
  Confidence: band=very_high; score=0.99
- Line 119: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: throw std::invalid_argument("Input must be 2D or 3D tensor");
  Confidence: band=very_high; score=0.99
- Line 127: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input_shape.size() == 2) {
  Confidence: band=very_high; score=0.99
- Line 128: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: output = GPUTensor({batch_size, out_dim}, input.device());
  Confidence: band=very_high; score=0.99
- Line 130: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: output = GPUTensor({batch_size, seq_len, out_dim}, input.device());
  Confidence: band=very_high; score=0.99
- Line 134: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.device().type == DeviceType::CUDA) {
  Confidence: band=very_high; score=0.99
- Line 140: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 150: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 160: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 170: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 180: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 223: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const GPUTensor& input,
  Confidence: band=very_high; score=0.99
- Line 228: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return backward(grad_output, input, B, A, scaling, Config{});
  Confidence: band=very_high; score=0.99
- Line 233: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const GPUTensor& input,
  Confidence: band=very_high; score=0.99
- Line 253: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch_size = input_shape[0];
  Confidence: band=very_high; score=0.99
- Line 254: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: seq_len = input_shape[1];
  Confidence: band=very_high; score=0.99
- Line 255: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: in_dim = input_shape[2];
  Confidence: band=very_high; score=0.99
- Line 273: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: GPUTensor grad_input;
  Confidence: band=very_high; score=0.99
- Line 274: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input_shape.size() == 2) {
  Confidence: band=very_high; score=0.99
- Line 275: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_input = GPUTensor({batch_size, in_dim}, input.device());
  Confidence: band=very_high; score=0.99
- Line 277: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_input = GPUTensor({batch_size, seq_len, in_dim}, input.device());
  Confidence: band=very_high; score=0.99
- Line 280: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: GPUTensor grad_B({rank, in_dim}, input.device());
  Confidence: band=very_high; score=0.99
- Line 281: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: GPUTensor grad_A({out_dim, rank}, input.device());
  Confidence: band=very_high; score=0.99
- Line 288: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.device().type == DeviceType::CUDA) {
  Confidence: band=very_high; score=0.99
- Line 295: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 305: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 324: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 334: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 347: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Compute grad_input
  Confidence: band=very_high; score=0.99
- Line 349: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: err = cuda::flash::launch_flash_lora_backward_input_kernel<128, 64, 8>(
  Confidence: band=very_high; score=0.99
- Line 353: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<float*>(grad_input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 359: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: err = cuda::flash::launch_flash_lora_backward_input_kernel<128, 64, 16>(
  Confidence: band=very_high; score=0.99
- Line 363: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<float*>(grad_input.gpu_ptr()),
  Confidence: band=very_high; score=0.99
- Line 371: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: throw std::runtime_error("FlashLoRA backward input kernel failed");
  Confidence: band=very_high; score=0.99
- Line 387: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return std::make_tuple(std::move(grad_input), std::move(grad_B), std::move(grad_A));
  Confidence: band=very_high; score=0.99
- Line 507: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Input in_dim (" + std::to_string(in_dim) +
  Confidence: band=very_high; score=0.99
- Line 520: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.device().type != B.device().type ||
  Confidence: band=very_high; score=0.99
- Line 521: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.device().type != A.device().type) {
  Confidence: band=very_high; score=0.99
- Line 78: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const GPUTensor& input,
  Confidence: band=very_high; score=0.9
- Line 83: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return forward(input, B, A, scaling, Config{});
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const GPUTensor& input,
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: validate_shapes(input, B, A);
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!is_available(input.device())) {
  Confidence: band=very_high; score=0.9
- Line 104: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Get input shape
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input_shape = input.shape();
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input_shape.size() == 2) {
  Confidence: band=very_high; score=0.9
- Line 110: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch_size = input_shape[0];
  Confidence: band=very_high; score=0.9
- Line 112: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: in_dim = input_shape[1];
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: } else if (input_shape.size() == 3) {
  Confidence: band=very_high; score=0.9
- Line 115: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch_size = input_shape[0];
  Confidence: band=very_high; score=0.9
- Line 116: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: seq_len = input_shape[1];
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: in_dim = input_shape[2];
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: throw std::invalid_argument("Input must be 2D or 3D tensor");
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input_shape.size() == 2) {
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: output = GPUTensor({batch_size, out_dim}, input.device());
  Confidence: band=very_high; score=0.9
- Line 130: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: output = GPUTensor({batch_size, seq_len, out_dim}, input.device());
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.device().type == DeviceType::CUDA) {
  Confidence: band=very_high; score=0.9
- Line 140: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 150: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 170: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const GPUTensor& input,
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return backward(grad_output, input, B, A, scaling, Config{});
  Confidence: band=very_high; score=0.9
- Line 233: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const GPUTensor& input,
  Confidence: band=very_high; score=0.9
- Line 242: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: validate_shapes(input, B, A);
  Confidence: band=very_high; score=0.9
- Line 245: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input_shape = input.shape();
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input_shape.size() == 2) {
  Confidence: band=very_high; score=0.9
- Line 249: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch_size = input_shape[0];
  Confidence: band=very_high; score=0.9
- Line 251: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: in_dim = input_shape[1];
  Confidence: band=very_high; score=0.9
- Line 253: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch_size = input_shape[0];
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: seq_len = input_shape[1];
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: in_dim = input_shape[2];
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: GPUTensor grad_input;
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input_shape.size() == 2) {
  Confidence: band=very_high; score=0.9
- Line 275: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_input = GPUTensor({batch_size, in_dim}, input.device());
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_input = GPUTensor({batch_size, seq_len, in_dim}, input.device());
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: GPUTensor grad_B({rank, in_dim}, input.device());
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: GPUTensor grad_A({out_dim, rank}, input.device());
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.device().type == DeviceType::CUDA) {
  Confidence: band=very_high; score=0.9
- Line 295: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 324: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<const float*>(input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Compute grad_input
  Confidence: band=very_high; score=0.9
- Line 349: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: err = cuda::flash::launch_flash_lora_backward_input_kernel<128, 64, 8>(
  Confidence: band=very_high; score=0.9
- Line 353: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<float*>(grad_input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: err = cuda::flash::launch_flash_lora_backward_input_kernel<128, 64, 16>(
  Confidence: band=very_high; score=0.9
- Line 363: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<float*>(grad_input.gpu_ptr()),
  Confidence: band=very_high; score=0.9
- Line 371: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: throw std::runtime_error("FlashLoRA backward input kernel failed");
  Confidence: band=very_high; score=0.9
- Line 387: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return std::make_tuple(std::move(grad_input), std::move(grad_B), std::move(grad_A));
  Confidence: band=very_high; score=0.9
- Line 475: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const GPUTensor& input,
  Confidence: band=very_high; score=0.9
- Line 479: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Validate input shape
  Confidence: band=very_high; score=0.9
- Line 480: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto input_shape = input.shape();
  Confidence: band=very_high; score=0.9
- Line 481: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input_shape.size() != 2 && input_shape.size() != 3) {
  Confidence: band=very_high; score=0.9
- Line 483: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Input must be 2D [batch, in_dim] or 3D [batch, seq_len, in_dim]"
  Confidence: band=very_high; score=0.9
- Line 499: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check dimension compatibility
  Confidence: band=high; score=0.8
- Line 500: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t in_dim = input_shape.back();
  Confidence: band=very_high; score=0.9
- Line 507: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Input in_dim (" + std::to_string(in_dim) +
  Confidence: band=very_high; score=0.9
- Line 520: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.device().type != B.device().type ||
  Confidence: band=very_high; score=0.9
- Line 521: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.device().type != A.device().type) {
  Confidence: band=very_high; score=0.9

### src/llm/async_inference_engine.cpp
Total findings: 109

- Line 907: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: // concurrent swapPlugin() call does not race with the generate() invocation.
  Confidence: band=very_high; score=0.99
- Line 2: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * ThemisDB | File: async_inference_engine.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 17:16:09
  Confidence: band=very_high; score=0.9
- Line 10: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/async_inference_engine.h"
  Confidence: band=very_high; score=0.9
- Line 24: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // AsyncInferenceEngine Implementation
  Confidence: band=very_high; score=0.9
- Line 27: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AsyncInferenceEngine::AsyncInferenceEngine(
  Confidence: band=very_high; score=0.9
- Line 38: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine starting with {} worker threads",
  Confidence: band=very_high; score=0.9
- Line 44: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: workers_.emplace_back(&AsyncInferenceEngine::workerLoop, this, i);
  Confidence: band=very_high; score=0.9
- Line 48: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 48: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 54: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine: deduplication cache enabled (dir={})",
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine started - inference runs independently from DB operations");
  Confidence: band=very_high; score=0.9
- Line 61: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AsyncInferenceEngine::AsyncInferenceEngine(
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ) : config_(config), plugin_(plugin.get()), owned_plugin_(std::move(plugin)) {
  Confidence: band=very_high; score=0.9
- Line 69: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine starting with {} worker threads",
  Confidence: band=very_high; score=0.9
- Line 73: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: workers_.emplace_back(&AsyncInferenceEngine::workerLoop, this, i);
  Confidence: band=very_high; score=0.9
- Line 77: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 77: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 83: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine: deduplication cache enabled (dir={})",
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine started - inference runs independently from DB operations");
  Confidence: band=very_high; score=0.9
- Line 92: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AsyncInferenceEngine::AsyncInferenceEngine(
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine started with shared worker pool ({} threads)",
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 115: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine: deduplication cache enabled (dir={})",
  Confidence: band=very_high; score=0.9
- Line 120: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AsyncInferenceEngine::AsyncInferenceEngine(
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: ) : config_(config), plugin_(plugin.get()), owned_plugin_(std::move(plugin)),
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine started with shared worker pool ({} threads)",
  Confidence: band=very_high; score=0.9
- Line 135: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 135: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
  Confidence: band=very_high; score=0.9
- Line 141: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine: deduplication cache enabled (dir={})",
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AsyncInferenceEngine::~AsyncInferenceEngine() {
  Confidence: band=very_high; score=0.9
- Line 150: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceHandle AsyncInferenceEngine::submit(
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest& request,
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "AsyncInferenceEngine::submit start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto async_req = std::make_shared<AsyncInferenceRequest>();
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_future<InferenceResponse> future;
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto promise = std::make_shared<std::promise<InferenceResponse>>();
  Confidence: band=very_high; score=0.9
- Line 252: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("Submitted inference request {} (priority={}, via_pool={})",
  Confidence: band=very_high; score=0.9
- Line 256: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "AsyncInferenceEngine::submit queued: request_id={} priority={} via_pool={}",
  Confidence: band=very_high; score=0.9
- Line 261: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return InferenceHandle(async_req->request_id, future, async_req->cancel_token);
  Confidence: band=very_high; score=0.9
- Line 264: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string AsyncInferenceEngine::submitAsync(
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest& request,
  Confidence: band=very_high; score=0.9
- Line 266: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::function<void(const InferenceResponse&)> callback,
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "AsyncInferenceEngine::submitAsync start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto async_req = std::make_shared<AsyncInferenceRequest>();
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto promise = std::make_shared<std::promise<InferenceResponse>>();
  Confidence: band=very_high; score=0.9
- Line 355: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("Submitted async inference request {} (callback mode, via_pool={})",
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "AsyncInferenceEngine::submitAsync queued: request_id={} priority={} via_pool={}",
  Confidence: band=very_high; score=0.9
- Line 367: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceHandle AsyncInferenceEngine::submitStreaming(
  Confidence: band=very_high; score=0.9
- Line 368: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest&   request,
  Confidence: band=very_high; score=0.9
- Line 376: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "AsyncInferenceEngine::submitStreaming start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",
  Confidence: band=very_high; score=0.9
- Line 383: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto async_req          = std::make_shared<AsyncInferenceRequest>();
  Confidence: band=very_high; score=0.9
- Line 417: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: [cb, fired_final](const InferenceResponse&) {
  Confidence: band=very_high; score=0.9
- Line 431: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_future<InferenceResponse> future;
  Confidence: band=very_high; score=0.9
- Line 434: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto promise          = std::make_shared<std::promise<InferenceResponse>>();
  Confidence: band=very_high; score=0.9
- Line 442: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: async_req->callback(InferenceResponse{});
  Confidence: band=very_high; score=0.9
- Line 474: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto local_promise        = std::make_shared<std::promise<InferenceResponse>>();
  Confidence: band=very_high; score=0.9
- Line 497: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("Submitted streaming inference request {} (priority={}, via_pool={})",
  Confidence: band=very_high; score=0.9
- Line 501: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "AsyncInferenceEngine::submitStreaming queued: request_id={} priority={} via_pool={}",
  Confidence: band=very_high; score=0.9
- Line 506: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return InferenceHandle(async_req->request_id, future, async_req->cancel_token);
  Confidence: band=very_high; score=0.9
- Line 509: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceHandle AsyncInferenceEngine::submitRAG(
  Confidence: band=very_high; score=0.9
- Line 511: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest& request,
  Confidence: band=very_high; score=0.9
- Line 515: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest rag_request = request;
  Confidence: band=very_high; score=0.9
- Line 521: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "AsyncInferenceEngine::submitRAG start: docs={} top_k={} max_context_tokens={} response_budget_tokens={} request_max_tokens={} priority={} rag_priority={}",
  Confidence: band=very_high; score=0.9
- Line 608: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Cancelled inference request: {}", request_id);
  Confidence: band=very_high; score=0.9
- Line 612: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: json AsyncInferenceEngine::getQueueStats() const {
  Confidence: band=very_high; score=0.9
- Line 624: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: json AsyncInferenceEngine::getWorkerStats() const {
  Confidence: band=very_high; score=0.9
- Line 635: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats["avg_inference_time_ms"] =
  Confidence: band=very_high; score=0.9
- Line 636: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats_.total_inference_time_ms.load() / completed;
  Confidence: band=very_high; score=0.9
- Line 645: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats["total_tokens_generated"] = stats_.total_tokens_generated.load();
  Confidence: band=very_high; score=0.9
- Line 668: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void AsyncInferenceEngine::waitForCompletion() {
  Confidence: band=very_high; score=0.9
- Line 669: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Waiting for all pending inference requests to complete...");
  Confidence: band=very_high; score=0.9
- Line 675: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("All inference requests completed");
  Confidence: band=very_high; score=0.9
- Line 678: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void AsyncInferenceEngine::shutdown() {
  Confidence: band=very_high; score=0.9
- Line 683: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Shutting down AsyncInferenceEngine...");
  Confidence: band=very_high; score=0.9
- Line 702: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine shutdown complete");
  Confidence: band=very_high; score=0.9
- Line 709: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void AsyncInferenceEngine::workerLoop(size_t worker_id) {
  Confidence: band=very_high; score=0.9
- Line 710: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Inference worker {} started", worker_id);
  Confidence: band=very_high; score=0.9
- Line 748: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: try { item.request->callback(InferenceResponse{}); } catch (...) {}
  Confidence: band=very_high; score=0.9
- Line 777: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Actual inference (blocking call to plugin)
  Confidence: band=very_high; score=0.9
- Line 796: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 815: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Inference worker {} stopped", worker_id);
  Confidence: band=very_high; score=0.9
- Line 832: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest effective_request = request.request;
  Confidence: band=very_high; score=0.9
- Line 839: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "AsyncInferenceEngine::processRequest streaming start: request_id={} prompt_len={} priority={}",
  Confidence: band=very_high; score=0.9
- Line 866: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: auto cached = dedup_cache_->get(effective_request.prompt);
  Confidence: band=very_high; score=0.9
- Line 890: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "AsyncInferenceEngine: request {} blocked by prompt policy rule '{}'",
  Confidence: band=very_high; score=0.9
- Line 892: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse blocked;
  Confidence: band=very_high; score=0.9
- Line 914: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Call plugin (blocking inference)
  Confidence: band=very_high; score=0.9
- Line 915: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse response = plugin_snapshot->generate(effective_request);
  Confidence: band=very_high; score=0.9
- Line 919: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "AsyncInferenceEngine::processRequest streaming complete: request_id={} tokens_generated={} inference_time_ms={:.2f}",
  Confidence: band=very_high; score=0.9
- Line 922: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 937: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats_.total_inference_time_ms.fetch_add(response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 942: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (response.inference_time_ms > 0.0) {
  Confidence: band=very_high; score=0.9
- Line 944: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: latency_samples_.push_back(response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 953: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void AsyncInferenceEngine::setDedupCache(std::shared_ptr<LLMResponseCache> cache) {
  Confidence: band=very_high; score=0.9
- Line 957: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: LLMResponseCache::CacheStatistics AsyncInferenceEngine::getDedupCacheStats() const {
  Confidence: band=very_high; score=0.9
- Line 964: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void AsyncInferenceEngine::swapPlugin(std::shared_ptr<ILLMPlugin> new_plugin) {
  Confidence: band=very_high; score=0.9
- Line 970: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: plugin_ = owned_plugin_.get();
  Confidence: band=very_high; score=0.9
- Line 972: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("AsyncInferenceEngine: plugin hot-swapped");
  Confidence: band=very_high; score=0.9
- Line 975: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void AsyncInferenceEngine::setPromptPolicy(std::shared_ptr<PromptPolicy> policy) {
  Confidence: band=very_high; score=0.9
- Line 979: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string AsyncInferenceEngine::generateRequestId() {
  Confidence: band=very_high; score=0.9
- Line 994: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool AsyncInferenceEngine::handleBackpressure(std::unique_lock<std::mutex>& lock) {
  Confidence: band=very_high; score=0.9
- Line 1020: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Signal cancel token so any InferenceHandle for this request
  Confidence: band=very_high; score=0.9
- Line 1078: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void AsyncInferenceEngine::checkAndHandleTimeouts() {
  Confidence: band=very_high; score=0.9
- Line 1094: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: // Resolve the future immediately so handle.get() unblocks without
  Confidence: band=very_high; score=0.9
- Line 43: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&AsyncInferenceEngine::workerLoop, this, i);
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&AsyncInferenceEngine::workerLoop, this, i);
  Confidence: band=high; score=0.74
- Line 907: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: // concurrent swapPlugin() call does not race with the generate() invocation.
  Confidence: band=high; score=0.74
- Line 915: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: InferenceResponse response = plugin_snapshot->generate(effective_request);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/kernels/directx_kernels.cpp
Total findings: 80

- Line 272: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: 2   // num_srvs (inputs A, B)
  Confidence: band=very_high; score=0.99
- Line 284: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Upload input data
  Confidence: band=very_high; score=0.99
- Line 357: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: 2   // num_srvs (inputs A, B)
  Confidence: band=very_high; score=0.99
- Line 367: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Upload input data
  Confidence: band=very_high; score=0.99
- Line 558: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
  Confidence: band=very_high; score=0.99
- Line 561: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!input || !output) {
  Confidence: band=very_high; score=0.99
- Line 579: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: DirectXBuffer buffer_input(g_directx_state.context.get(), byte_size);
  Confidence: band=very_high; score=0.99
- Line 583: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buffer_input.upload(input, byte_size);
  Confidence: band=very_high; score=0.99
- Line 593: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buffer_input.resource(), size_u32, sizeof(float));
  Confidence: band=very_high; score=0.99
- Line 647: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: 3,  // num_uavs (grad_A, grad_B, grad_input)
  Confidence: band=very_high; score=0.99
- Line 648: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: 4   // num_srvs (input, B, A, grad_output)
  Confidence: band=very_high; score=0.99
- Line 666: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Upload input data
  Confidence: band=very_high; score=0.99
- Line 682: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: uint32_t uav_grad_input = g_directx_state.descriptors->create_uav(
  Confidence: band=very_high; score=0.99
- Line 685: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: uint32_t srv_input = g_directx_state.descriptors->create_srv(
  Confidence: band=very_high; score=0.99
- Line 713: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_input));
  Confidence: band=very_high; score=0.99
- Line 736: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input, const float* grad_h, float* grad_B,
  Confidence: band=very_high; score=0.99
- Line 740: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!input || !grad_h || !grad_B) {
  Confidence: band=very_high; score=0.99
- Line 756: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // For grad_B: input is (M, D), grad_h is (M, K), output grad_B is (D, K)
  Confidence: band=very_high; score=0.99
- Line 757: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t size_input = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(D), "launch_lora_grad_B_shader");
  Confidence: band=very_high; score=0.99
- Line 761: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: DirectXBuffer buffer_input(g_directx_state.context.get(), size_input);
  Confidence: band=very_high; score=0.99
- Line 770: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Upload input data
  Confidence: band=very_high; score=0.99
- Line 771: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buffer_input.upload(input, size_input);
  Confidence: band=very_high; score=0.99
- Line 782: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t elems_input = checked_mul_size(static_cast<size_t>(M), static_cast<size_t>(D), "launch_lora_grad_B_shader");
  Confidence: band=very_high; score=0.99
- Line 786: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: uint32_t uav_grad_input = g_directx_state.descriptors->create_uav(
  Confidence: band=very_high; score=0.99
- Line 789: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: uint32_t srv_input = g_directx_state.descriptors->create_srv(
  Confidence: band=very_high; score=0.99
- Line 790: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buffer_input.resource(), checked_u32_size(elems_input, "launch_lora_grad_B_shader"), sizeof(float));
  Confidence: band=very_high; score=0.99
- Line 817: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_input));
  Confidence: band=very_high; score=0.99
- Line 922: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 928: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!output || !input) {
  Confidence: band=very_high; score=0.99
- Line 937: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_size = checked_mul_size(
  Confidence: band=very_high; score=0.99
- Line 944: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t input_bytes = checked_mul_size(input_size, sizeof(float), "launch_sequence_mean_shader");
  Confidence: band=very_high; score=0.99
- Line 948: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: DirectXBuffer buffer_input(g_directx_state.context.get(), input_bytes, DirectXBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.99
- Line 952: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: buffer_input.upload(input, input_bytes);
  Confidence: band=very_high; score=0.99
- Line 960: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: 1   // 1 SRV (input)
  Confidence: band=very_high; score=0.99
- Line 980: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pipeline->bind_srv(0, buffer_input);  // Input
  Confidence: band=very_high; score=0.99
- Line 1037: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
  Confidence: band=very_high; score=0.99
- Line 1048: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input, const float* grad_h, float* grad_B,
  Confidence: band=very_high; score=0.99
- Line 1066: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 272: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: 2   // num_srvs (inputs A, B)
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: 2   // num_srvs (inputs A, B)
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Upload input data
  Confidence: band=very_high; score=0.9
- Line 357: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: 2   // num_srvs (inputs A, B)
  Confidence: band=very_high; score=0.9
- Line 357: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: 2   // num_srvs (inputs A, B)
  Confidence: band=very_high; score=0.9
- Line 367: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Upload input data
  Confidence: band=very_high; score=0.9
- Line 558: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
  Confidence: band=very_high; score=0.9
- Line 561: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!input || !output) {
  Confidence: band=very_high; score=0.9
- Line 579: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: DirectXBuffer buffer_input(g_directx_state.context.get(), byte_size);
  Confidence: band=very_high; score=0.9
- Line 583: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buffer_input.upload(input, byte_size);
  Confidence: band=very_high; score=0.9
- Line 593: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buffer_input.resource(), size_u32, sizeof(float));
  Confidence: band=very_high; score=0.9
- Line 647: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: 3,  // num_uavs (grad_A, grad_B, grad_input)
  Confidence: band=very_high; score=0.9
- Line 648: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: 4   // num_srvs (input, B, A, grad_output)
  Confidence: band=very_high; score=0.9
- Line 661: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Dummy buffers for unused outputs
  Confidence: band=very_high; score=0.9
- Line 666: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Upload input data
  Confidence: band=very_high; score=0.9
- Line 682: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: uint32_t uav_grad_input = g_directx_state.descriptors->create_uav(
  Confidence: band=very_high; score=0.9
- Line 685: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: uint32_t srv_input = g_directx_state.descriptors->create_srv(
  Confidence: band=very_high; score=0.9
- Line 713: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_input));
  Confidence: band=very_high; score=0.9
- Line 736: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input, const float* grad_h, float* grad_B,
  Confidence: band=very_high; score=0.9
- Line 740: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!input || !grad_h || !grad_B) {
  Confidence: band=very_high; score=0.9
- Line 756: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // For grad_B: input is (M, D), grad_h is (M, K), output grad_B is (D, K)
  Confidence: band=very_high; score=0.9
- Line 757: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t size_input = checked_float_bytes_2d(static_cast<size_t>(M), static_cast<size_t>(D), "launch_lora_grad_B_shader");
  Confidence: band=very_high; score=0.9
- Line 761: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: DirectXBuffer buffer_input(g_directx_state.context.get(), size_input);
  Confidence: band=very_high; score=0.9
- Line 765: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Dummy buffers for unused outputs
  Confidence: band=very_high; score=0.9
- Line 770: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Upload input data
  Confidence: band=very_high; score=0.9
- Line 771: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buffer_input.upload(input, size_input);
  Confidence: band=very_high; score=0.9
- Line 782: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t elems_input = checked_mul_size(static_cast<size_t>(M), static_cast<size_t>(D), "launch_lora_grad_B_shader");
  Confidence: band=very_high; score=0.9
- Line 786: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: uint32_t uav_grad_input = g_directx_state.descriptors->create_uav(
  Confidence: band=very_high; score=0.9
- Line 789: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: uint32_t srv_input = g_directx_state.descriptors->create_srv(
  Confidence: band=very_high; score=0.9
- Line 790: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buffer_input.resource(), checked_u32_size(elems_input, "launch_lora_grad_B_shader"), sizeof(float));
  Confidence: band=very_high; score=0.9
- Line 817: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_srv_table(0, g_directx_state.descriptors->get_gpu_handle(srv_input));
  Confidence: band=very_high; score=0.9
- Line 922: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 928: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!output || !input) {
  Confidence: band=very_high; score=0.9
- Line 937: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_size = checked_mul_size(
  Confidence: band=very_high; score=0.9
- Line 944: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t input_bytes = checked_mul_size(input_size, sizeof(float), "launch_sequence_mean_shader");
  Confidence: band=very_high; score=0.9
- Line 948: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: DirectXBuffer buffer_input(g_directx_state.context.get(), input_bytes, DirectXBuffer::Usage::DeviceLocal);
  Confidence: band=very_high; score=0.9
- Line 952: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: buffer_input.upload(input, input_bytes);
  Confidence: band=very_high; score=0.9
- Line 960: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: 1   // 1 SRV (input)
  Confidence: band=very_high; score=0.9
- Line 980: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pipeline->bind_srv(0, buffer_input);  // Input
  Confidence: band=very_high; score=0.9
- Line 1037: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
  Confidence: band=very_high; score=0.9
- Line 1048: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input, const float* grad_h, float* grad_B,
  Confidence: band=very_high; score=0.9
- Line 1066: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9

### src/llm/lora_framework/lora_training_service.cpp
Total findings: 77

- Line 130: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize from JSON
  Confidence: band=very_high; score=0.99
- Line 277: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inst_sample.instruction = sample.input;
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inst_sample.input = "";  // No separate input field in original format
  Confidence: band=very_high; score=0.99
- Line 615: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t batch_size = batch.input_ids.size();
  Confidence: band=very_high; score=0.99
- Line 616: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (batch_size == 0 || batch.input_ids.front().empty()) {
  Confidence: band=very_high; score=0.99
- Line 622: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Skipping malformed training batch at step {}: input/label row count mismatch ({} vs {})",
  Confidence: band=very_high; score=0.99
- Line 630: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor batch_input({batch_size, hidden_dim});
  Confidence: band=very_high; score=0.99
- Line 648: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t row_seq = batch.input_ids[i].size();
  Confidence: band=very_high; score=0.99
- Line 651: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: int token_id = (row_seq > 0) ? batch.input_ids[i][token_idx] : 0;
  Confidence: band=very_high; score=0.99
- Line 652: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch_input[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;
  Confidence: band=very_high; score=0.99
- Line 656: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // In production, this would be the actual transformer input
  Confidence: band=very_high; score=0.99
- Line 658: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t row_input_seq = batch.input_ids[i].size();
  Confidence: band=very_high; score=0.99
- Line 659: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t eff_input_seq = std::min(row_input_seq, emb_depth);
  Confidence: band=very_high; score=0.99
- Line 662: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t tok_idx = 0; tok_idx < eff_input_seq; ++tok_idx) {
  Confidence: band=very_high; score=0.99
- Line 665: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch_input[i * hidden_dim + j] = (eff_input_seq > 0) ? sum / static_cast<float>(eff_input_seq) : 0.0f;
  Confidence: band=very_high; score=0.99
- Line 700: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t ri = batch.input_ids[i].size();
  Confidence: band=very_high; score=0.99
- Line 704: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: int token_id = (ri > 0) ? batch.input_ids[i][token_idx] : 0;
  Confidence: band=very_high; score=0.99
- Line 705: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch_input[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;
  Confidence: band=very_high; score=0.99
- Line 718: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t ri = batch.input_ids[i].size();
  Confidence: band=very_high; score=0.99
- Line 722: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: int token_id = (ri > 0) ? batch.input_ids[i][token_idx] : 0;
  Confidence: band=very_high; score=0.99
- Line 723: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch_input[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;
  Confidence: band=very_high; score=0.99
- Line 734: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t ri = batch.input_ids[i].size();
  Confidence: band=very_high; score=0.99
- Line 738: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: int token_id = (ri > 0) ? batch.input_ids[i][token_idx] : 0;
  Confidence: band=very_high; score=0.99
- Line 739: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch_input[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;
  Confidence: band=very_high; score=0.99
- Line 741: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Target is shifted input (next token prediction)
  Confidence: band=very_high; score=0.99
- Line 767: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: //   1. Tokenize input text using llama.cpp tokenizer
  Confidence: band=very_high; score=0.99
- Line 775: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: predictions = enhanced_model->forward(batch_input, 0);
  Confidence: band=very_high; score=0.99
- Line 778: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: predictions = lora_layer->forward(batch_input);
  Confidence: band=very_high; score=0.99
- Line 1101: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize checkpoint
  Confidence: band=very_high; score=0.99
- Line 1440: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: spdlog::info("  Input dim: {}", hidden_dim);
  Confidence: band=very_high; score=0.99
- Line 1481: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inst_sample.instruction = sample.input;
  Confidence: band=very_high; score=0.99
- Line 1482: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inst_sample.input = "";
  Confidence: band=very_high; score=0.99
- Line 1840: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Load weights for each layer
  Confidence: band=very_high; score=0.99
- Line 1841: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // In production, this would load actual quantized weights
  Confidence: band=very_high; score=0.99
- Line 277: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inst_sample.instruction = sample.input;
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inst_sample.input = "";  // No separate input field in original format
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "SimpleTokenizer causes train/inference mismatch.";
  Confidence: band=very_high; score=0.9
- Line 517: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (local_config.lr_scheduler.type != SchedulerType::CONSTANT || local_config.lr_scheduler.base_lr != 1e-4f) {
  Confidence: band=very_high; score=0.9
- Line 524: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Fall back to params-based scheduler for backward compatibility
  Confidence: band=high; score=0.8
- Line 615: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t batch_size = batch.input_ids.size();
  Confidence: band=very_high; score=0.9
- Line 616: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (batch_size == 0 || batch.input_ids.front().empty()) {
  Confidence: band=very_high; score=0.9
- Line 622: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Skipping malformed training batch at step {}: input/label row count mismatch ({} vs {})",
  Confidence: band=very_high; score=0.9
- Line 630: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor batch_input({batch_size, hidden_dim});
  Confidence: band=very_high; score=0.9
- Line 648: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t row_seq = batch.input_ids[i].size();
  Confidence: band=very_high; score=0.9
- Line 651: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: int token_id = (row_seq > 0) ? batch.input_ids[i][token_idx] : 0;
  Confidence: band=very_high; score=0.9
- Line 652: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch_input[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;
  Confidence: band=very_high; score=0.9
- Line 656: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // In production, this would be the actual transformer input
  Confidence: band=very_high; score=0.9
- Line 658: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t row_input_seq = batch.input_ids[i].size();
  Confidence: band=very_high; score=0.9
- Line 659: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t eff_input_seq = std::min(row_input_seq, emb_depth);
  Confidence: band=very_high; score=0.9
- Line 662: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t tok_idx = 0; tok_idx < eff_input_seq; ++tok_idx) {
  Confidence: band=very_high; score=0.9
- Line 665: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch_input[i * hidden_dim + j] = (eff_input_seq > 0) ? sum / static_cast<float>(eff_input_seq) : 0.0f;
  Confidence: band=very_high; score=0.9
- Line 700: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t ri = batch.input_ids[i].size();
  Confidence: band=very_high; score=0.9
- Line 704: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: int token_id = (ri > 0) ? batch.input_ids[i][token_idx] : 0;
  Confidence: band=very_high; score=0.9
- Line 705: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch_input[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;
  Confidence: band=very_high; score=0.9
- Line 718: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t ri = batch.input_ids[i].size();
  Confidence: band=very_high; score=0.9
- Line 722: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: int token_id = (ri > 0) ? batch.input_ids[i][token_idx] : 0;
  Confidence: band=very_high; score=0.9
- Line 723: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch_input[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;
  Confidence: band=very_high; score=0.9
- Line 734: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t ri = batch.input_ids[i].size();
  Confidence: band=very_high; score=0.9
- Line 738: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: int token_id = (ri > 0) ? batch.input_ids[i][token_idx] : 0;
  Confidence: band=very_high; score=0.9
- Line 739: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch_input[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;
  Confidence: band=very_high; score=0.9
- Line 741: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Target is shifted input (next token prediction)
  Confidence: band=very_high; score=0.9
- Line 767: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //   1. Tokenize input text using llama.cpp tokenizer
  Confidence: band=very_high; score=0.9
- Line 769: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //   3. Apply LoRA adapter on top of base model outputs
  Confidence: band=very_high; score=0.9
- Line 775: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: predictions = enhanced_model->forward(batch_input, 0);
  Confidence: band=very_high; score=0.9
- Line 778: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: predictions = lora_layer->forward(batch_input);
  Confidence: band=very_high; score=0.9
- Line 1304: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Step 1: Model Compatibility Check
  Confidence: band=high; score=0.8
- Line 1440: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: spdlog::info("  Input dim: {}", hidden_dim);
  Confidence: band=very_high; score=0.9
- Line 1481: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inst_sample.instruction = sample.input;
  Confidence: band=very_high; score=0.9
- Line 1482: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inst_sample.input = "";
  Confidence: band=very_high; score=0.9
- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: lora_training_service.cpp | Version: 0.0.47 | Last Modified: 2026-06-01 04:22:09
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_training_service.h"
  Confidence: band=high; score=0.74
- Line 11: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_storage_service.h"
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: instruction_samples.push_back(inst_sample);
  Confidence: band=high; score=0.74
- Line 1314: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: error += "  - " + err + "\n";
  Confidence: band=high; score=0.74
- Line 1483: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: instruction_samples.push_back(inst_sample);
  Confidence: band=high; score=0.74
- Line 1642: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: layers.push_back(std::move(layer));
  Confidence: band=high; score=0.74
- Line 1806: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: layer_names.push_back("blk." + std::to_string(j) + ".attn.wq");
  Confidence: band=high; score=0.74

### src/llm/grafana_metrics.cpp
Total findings: 68

- Line 1517: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: R"({"status":"not_implemented","message":"No reload callback registered. Wire setReloadCallback() to LlamaWrapper::loadModel()."})";
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * PR History (last 5): #3036 [llm] Unified metrics dashb... (2026-03-12) | #1295 Remove legacy query_parser.... (2026-03-11) | #689 Stabilize Extended Context ... (2026-03-11) | #215 Implement P1 LLM Inference ... (2026-03-11) | #214 Integrate Prometheus metric... (2026-03-11)
  Confidence: band=very_high; score=0.87
- Line 261: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Inference metrics
  Confidence: band=very_high; score=0.9
- Line 263: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "llm_inference_requests_total",
  Confidence: band=very_high; score=0.9
- Line 264: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Total number of LLM inference requests",
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "llm_inference_success_total",
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Total number of successful LLM inference requests",
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "llm_inference_failures_total",
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Total number of failed LLM inference requests",
  Confidence: band=very_high; score=0.9
- Line 300: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "End-to-end inference latency in milliseconds",
  Confidence: band=very_high; score=0.9
- Line 306: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "llm_inference_duration_ms",
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Total inference duration in milliseconds",
  Confidence: band=very_high; score=0.9
- Line 456: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Total inference requests rejected due to queue depth limit (backpressure)",
  Confidence: band=very_high; score=0.9
- Line 477: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // engine_type label: "async" = AsyncInferenceEngine,
  Confidence: band=very_high; score=0.9
- Line 478: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: //                    "enhanced" = InferenceEngineEnhanced
  Confidence: band=very_high; score=0.9
- Line 480: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "llm_engine_inference_requests_total",
  Confidence: band=very_high; score=0.9
- Line 481: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Total inference requests per engine type",
  Confidence: band=very_high; score=0.9
- Line 487: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "llm_engine_inference_success_total",
  Confidence: band=very_high; score=0.9
- Line 488: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Total successful inference requests per engine type",
  Confidence: band=very_high; score=0.9
- Line 494: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "llm_engine_inference_failures_total",
  Confidence: band=very_high; score=0.9
- Line 495: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Total failed inference requests per engine type",
  Confidence: band=very_high; score=0.9
- Line 501: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "llm_engine_inference_duration_ms",
  Confidence: band=very_high; score=0.9
- Line 502: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Inference duration histogram per engine type",
  Confidence: band=very_high; score=0.9
- Line 525: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMMetricsCollector::recordInferenceRequest(const std::string& model_id) {
  Confidence: band=very_high; score=0.9
- Line 526: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: exporter_->incrementCounter("llm_inference_requests_total", {{"model_id", model_id}});
  Confidence: band=very_high; score=0.9
- Line 529: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMMetricsCollector::recordInferenceSuccess(const std::string& model_id, double duration_ms) {
  Confidence: band=very_high; score=0.9
- Line 530: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: exporter_->incrementCounter("llm_inference_success_total", {{"model_id", model_id}});
  Confidence: band=very_high; score=0.9
- Line 531: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: exporter_->observeHistogram("llm_inference_duration_ms", duration_ms, {{"model_id", model_id}});
  Confidence: band=very_high; score=0.9
- Line 534: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMMetricsCollector::recordInferenceFailure(const std::string& model_id, const std::string& error) {
  Confidence: band=very_high; score=0.9
- Line 535: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: exporter_->incrementCounter("llm_inference_failures_total", {{"model_id", model_id}, {"error", error}});
  Confidence: band=very_high; score=0.9
- Line 786: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMMetricsCollector::recordEngineInferenceRequest(const std::string& model_id,
  Confidence: band=very_high; score=0.9
- Line 788: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: exporter_->incrementCounter("llm_engine_inference_requests_total",
  Confidence: band=very_high; score=0.9
- Line 792: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMMetricsCollector::recordEngineInferenceSuccess(const std::string& model_id,
  Confidence: band=very_high; score=0.9
- Line 795: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: exporter_->incrementCounter("llm_engine_inference_success_total",
  Confidence: band=very_high; score=0.9
- Line 797: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: exporter_->observeHistogram("llm_engine_inference_duration_ms", duration_ms,
  Confidence: band=very_high; score=0.9
- Line 801: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMMetricsCollector::recordEngineInferenceFailure(const std::string& model_id,
  Confidence: band=very_high; score=0.9
- Line 804: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: exporter_->incrementCounter("llm_engine_inference_failures_total",
  Confidence: band=very_high; score=0.9
- Line 1048: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Inference panel
  Confidence: band=very_high; score=0.9
- Line 1049: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Inference Requests",
  Confidence: band=very_high; score=0.9
- Line 1050: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "rate(llm_inference_requests_total[5m])",
  Confidence: band=very_high; score=0.9
- Line 1057: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "histogram_quantile(0.95, rate(llm_inference_duration_ms_bucket[5m]))",
  Confidence: band=very_high; score=0.9
- Line 1122: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string GrafanaDashboardGenerator::generateInferencePanel() const {
  Confidence: band=very_high; score=0.9
- Line 1123: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return createPanel("Inference Requests Rate",
  Confidence: band=very_high; score=0.9
- Line 1124: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "rate(llm_inference_requests_total[5m])",
  Confidence: band=very_high; score=0.9
- Line 1187: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Requests/sec — AsyncInferenceEngine",
  Confidence: band=very_high; score=0.9
- Line 1188: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "rate(llm_engine_inference_requests_total{engine_type=\"async\"}[5m])",
  Confidence: band=very_high; score=0.9
- Line 1191: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Requests/sec — InferenceEngineEnhanced",
  Confidence: band=very_high; score=0.9
- Line 1192: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "rate(llm_engine_inference_requests_total{engine_type=\"enhanced\"}[5m])",
  Confidence: band=very_high; score=0.9
- Line 1198: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Latency p95 (ms) — AsyncInferenceEngine",
  Confidence: band=very_high; score=0.9
- Line 1199: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "histogram_quantile(0.95, rate(llm_engine_inference_duration_ms_bucket{engine_type=\"async\"}[5m]))",
  Confidence: band=very_high; score=0.9
- Line 1202: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Latency p95 (ms) — InferenceEngineEnhanced",
  Confidence: band=very_high; score=0.9
- Line 1203: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "histogram_quantile(0.95, rate(llm_engine_inference_duration_ms_bucket{engine_type=\"enhanced\"}[5m]))",
  Confidence: band=very_high; score=0.9
- Line 1209: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Tokens/sec — AsyncInferenceEngine",
  Confidence: band=very_high; score=0.9
- Line 1213: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Tokens/sec — InferenceEngineEnhanced",
  Confidence: band=very_high; score=0.9
- Line 1220: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Queue Depth — AsyncInferenceEngine",
  Confidence: band=very_high; score=0.9
- Line 1224: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Queue Depth — InferenceEngineEnhanced",
  Confidence: band=very_high; score=0.9
- Line 1235: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Error Rate — AsyncInferenceEngine",
  Confidence: band=very_high; score=0.9
- Line 1236: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "rate(llm_engine_inference_failures_total{engine_type=\"async\"}[5m])",
  Confidence: band=very_high; score=0.9
- Line 1239: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << createPanel("Error Rate — InferenceEngineEnhanced",
  Confidence: band=very_high; score=0.9
- Line 1240: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "rate(llm_engine_inference_failures_total{engine_type=\"enhanced\"}[5m])",
  Confidence: band=very_high; score=0.9
- Line 1357: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // GET /admin/sessions — list active inference sessions
  Confidence: band=very_high; score=0.9
- Line 1449: severity=HIGH; category=llm_ai_safety; pattern=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Context: return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.models_path;
  Confidence: band=very_high; score=0.9
- Line 1475: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response = GrafanaDashboardGenerator(dcfg).generateUnifiedDashboard();
  Confidence: band=very_high; score=0.9
- Line 121: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, MetricType> metric_types;
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p50 = sorted[idx_p50];
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p95 = sorted[idx_p95];
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto p99 = sorted[idx_p99];
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& labels) const {
  Confidence: band=medium; score=0.66

### src/llm/multi_lora_manager.cpp
Total findings: 67

- Line 1261: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize LoRA adapter
  Confidence: band=very_high; score=0.99
- Line 1300: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // F1-2 fix: reject deserialized paths that escape the trusted base directory.
  Confidence: band=very_high; score=0.99
- Line 1392: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Load actual LoRA weights from the GGUF file for quantization.
  Confidence: band=very_high; score=0.99
- Line 609: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<InferenceResponse> MultiLoRAManager::batchInferenceMultiLoRA(
  Confidence: band=very_high; score=0.9
- Line 610: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::vector<std::pair<InferenceRequest, std::string>>& requests,
  Confidence: band=very_high; score=0.9
- Line 613: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Multi-LoRA batch inference: {} requests", requests.size());
  Confidence: band=very_high; score=0.9
- Line 620: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // A valid llama_context is required for inference.
  Confidence: band=very_high; score=0.9
- Line 622: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("batchInferenceMultiLoRA: null model_context — cannot run inference");
  Confidence: band=very_high; score=0.9
- Line 623: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<InferenceResponse> error_responses(requests.size());
  Confidence: band=very_high; score=0.9
- Line 634: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("batchInferenceMultiLoRA: llama_get_model returned null");
  Confidence: band=very_high; score=0.9
- Line 635: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<InferenceResponse> error_responses(requests.size());
  Confidence: band=very_high; score=0.9
- Line 642: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("batchInferenceMultiLoRA: llama_model_get_vocab returned null");
  Confidence: band=very_high; score=0.9
- Line 643: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<InferenceResponse> error_responses(requests.size());
  Confidence: band=very_high; score=0.9
- Line 650: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("batchInferenceMultiLoRA: invalid vocabulary size {}", n_vocab);
  Confidence: band=very_high; score=0.9
- Line 651: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<InferenceResponse> error_responses(requests.size());
  Confidence: band=very_high; score=0.9
- Line 693: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Run inference for each request in this LoRA group
  Confidence: band=very_high; score=0.9
- Line 695: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest& request = requests[idx].first;
  Confidence: band=very_high; score=0.9
- Line 698: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse response;
  Confidence: band=very_high; score=0.9
- Line 740: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Without this reset, tokens from the preceding inference persist in
  Confidence: band=very_high; score=0.9
- Line 806: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.text             = std::move(generated_text);
  Confidence: band=very_high; score=0.9
- Line 807: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.tokens_generated = static_cast<int>(generated.size());
  Confidence: band=very_high; score=0.9
- Line 809: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.inference_time_ms = latency_ms;
  Confidence: band=very_high; score=0.9
- Line 827: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Multi-LoRA batch inference completed: {} responses", responses.size());
  Confidence: band=very_high; score=0.9
- Line 3140: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Validate compatibility
  Confidence: band=high; score=0.8
- Line 3306: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // For backward compatibility, prefer custom schedule function if provided,
  Confidence: band=high; score=0.8
- Line 3365: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: use a_weight and b_weight
  Confidence: band=high; score=0.8
- Line 3532: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: double total_inference_time = 0.0;
  Confidence: band=very_high; score=0.9
- Line 3533: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: size_t total_inference_count = 0;
  Confidence: band=very_high; score=0.9
- Line 3536: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (entry.inference_count > 0) {
  Confidence: band=very_high; score=0.9
- Line 3537: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: total_inference_time += entry.avg_inference_time_ms * entry.inference_count;
  Confidence: band=very_high; score=0.9
- Line 3538: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: total_inference_count += entry.inference_count;
  Confidence: band=very_high; score=0.9
- Line 3545: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (total_inference_count > 0) {
  Confidence: band=very_high; score=0.9
- Line 3546: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.avg_inference_time_ms = total_inference_time / total_inference_count;
  Confidence: band=very_high; score=0.9
- Line 3670: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: it->second.avg_inference_time_ms = fusion_time_ms;
  Confidence: band=very_high; score=0.9
- Line 3674: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void MultiLoRAManager::updateInferenceMetrics(
  Confidence: band=very_high; score=0.9
- Line 3676: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: double inference_time_ms
  Confidence: band=very_high; score=0.9
- Line 3682: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: size_t count = it->second.inference_count;
  Confidence: band=very_high; score=0.9
- Line 3683: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: double prev_avg = it->second.avg_inference_time_ms;
  Confidence: band=very_high; score=0.9
- Line 3686: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: it->second.avg_inference_time_ms =
  Confidence: band=very_high; score=0.9
- Line 3687: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: (prev_avg * count + inference_time_ms) / (count + 1);
  Confidence: band=very_high; score=0.9
- Line 3688: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: it->second.inference_count++;
  Confidence: band=very_high; score=0.9
- Line 663: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lora_to_requests[requests[i].second].push_back(i);
  Confidence: band=high; score=0.74
- Line 663: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lora_to_requests[requests[i].second].push_back(i);
  Confidence: band=high; score=0.74
- Line 663: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lora_to_requests[requests[i].second].push_back(i);
  Confidence: band=high; score=0.74
- Line 778: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: generated.push_back(next_token);
  Confidence: band=high; score=0.74
- Line 778: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: generated.push_back(next_token);
  Confidence: band=high; score=0.74
- Line 875: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: source_loras.push_back(lora);
  Confidence: band=high; score=0.74
- Line 902: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized_weights.push_back(w / weight_sum);
  Confidence: band=high; score=0.74
- Line 902: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized_weights.push_back(w / weight_sum);
  Confidence: band=high; score=0.74
- Line 1018: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 1042: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(info);
  Confidence: band=high; score=0.74
- Line 1133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_evict.push_back(id);
  Confidence: band=high; score=0.74
- Line 1806: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, size_t> MultiLoRAManager::getPerGPUMemoryUsage() const {
  Confidence: band=medium; score=0.66
- Line 1836: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overloaded_gpus.push_back(gpu_id);
  Confidence: band=high; score=0.74
- Line 1836: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overloaded_gpus.push_back(gpu_id);
  Confidence: band=high; score=0.74
- Line 2012: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: // once the base llama_model* is available (called from LlamaWrapper::generate()).
  Confidence: band=high; score=0.74
- Line 2162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available.push_back(gpu_id);
  Confidence: band=high; score=0.74
- Line 2643: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gpu_scores.push_back(gpu_eval);
  Confidence: band=high; score=0.74
- Line 2827: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unhealthy_gpus.push_back(gpu_id);
  Confidence: band=high; score=0.74
- Line 2855: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loras_to_migrate.push_back(id);
  Confidence: band=high; score=0.74
- Line 2855: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loras_to_migrate.push_back(id);
  Confidence: band=high; score=0.74
- Line 2956: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: log.push_back(entry);
  Confidence: band=high; score=0.74
- Line 3136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: source_loras.push_back(slot);
  Confidence: band=high; score=0.74
- Line 3158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized_weights.push_back(w / weight_sum);
  Confidence: band=high; score=0.74
- Line 3158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized_weights.push_back(w / weight_sum);
  Confidence: band=high; score=0.74
- Line 3558: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back(entry);
  Confidence: band=high; score=0.74
- Line 3586: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: source_loras.push_back(slot);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/kernels/hip_fused_kernels.cpp
Total findings: 61

- Line 28: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @brief Fused LoRA forward pass: output = (input @ B) @ A * scaling
  Confidence: band=very_high; score=0.99
- Line 31: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * 1. h = input @ B
  Confidence: band=very_high; score=0.99
- Line 41: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,   // [batch_size, in_dim]
  Confidence: band=very_high; score=0.99
- Line 51: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Shared memory for intermediate results (h = input @ B)
  Confidence: band=very_high; score=0.99
- Line 63: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Compute h = input @ B for this batch sample
  Confidence: band=very_high; score=0.99
- Line 64: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // h[rank] = input[in_dim] @ B[in_dim, rank]
  Confidence: band=very_high; score=0.99
- Line 71: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum += input[batch_idx * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.99
- Line 92: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum += input[batch_idx * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.99
- Line 126: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * - grad_B = input^T @ (grad_output @ A^T * scaling)
  Confidence: band=very_high; score=0.99
- Line 127: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * - grad_input = (grad_output @ A^T) @ B^T * scaling
  Confidence: band=very_high; score=0.99
- Line 133: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,        // [batch_size, in_dim]
  Confidence: band=very_high; score=0.99
- Line 139: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float* grad_input,         // [batch_size, in_dim]
  Confidence: band=very_high; score=0.99
- Line 147: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_type: 0 = grad_A, 1 = grad_B, 2 = grad_input
  Confidence: band=very_high; score=0.99
- Line 160: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Compute h[r] = input[b] @ B[:, r]
  Confidence: band=very_high; score=0.99
- Line 163: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: h_val += input[b * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.99
- Line 170: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Compute grad_B = input^T @ (grad_output @ A^T * scaling)
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum += input[b * in_dim + i] * temp * scaling;
  Confidence: band=very_high; score=0.99
- Line 189: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Compute grad_input = (grad_output @ A^T) @ B^T * scaling
  Confidence: band=very_high; score=0.99
- Line 190: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_input[batch_size, in_dim]
  Confidence: band=very_high; score=0.99
- Line 205: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_input[b * in_dim + i] = sum * scaling;
  Confidence: band=very_high; score=0.99
- Line 222: severity=CRITICAL; category=gpu_memory_safety; pattern=missing_sync_threads
  Description: Shared memory access in CUDA kernel without __syncthreads()
  Context: ) {
  Confidence: band=very_high; score=0.99
- Line 325: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 336: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input == nullptr || B == nullptr || A == nullptr || output == nullptr) {
  Confidence: band=very_high; score=0.99
- Line 353: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input, B, A, output, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.99
- Line 356: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input, B, A, output, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.99
- Line 363: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 369: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float* grad_input,
  Confidence: band=very_high; score=0.99
- Line 377: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input == nullptr || B == nullptr || A == nullptr || grad_output == nullptr ||
  Confidence: band=very_high; score=0.99
- Line 378: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_A == nullptr || grad_B == nullptr || grad_input == nullptr) {
  Confidence: band=very_high; score=0.99
- Line 400: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input, B, A, grad_output, grad_A, grad_B, grad_input,
  Confidence: band=very_high; score=0.99
- Line 404: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input, B, A, grad_output, grad_A, grad_B, grad_input,
  Confidence: band=very_high; score=0.99
- Line 28: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @brief Fused LoRA forward pass: output = (input @ B) @ A * scaling
  Confidence: band=very_high; score=0.9
- Line 31: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * 1. h = input @ B
  Confidence: band=very_high; score=0.9
- Line 41: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,   // [batch_size, in_dim]
  Confidence: band=very_high; score=0.9
- Line 51: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Shared memory for intermediate results (h = input @ B)
  Confidence: band=very_high; score=0.9
- Line 63: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Compute h = input @ B for this batch sample
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // h[rank] = input[in_dim] @ B[in_dim, rank]
  Confidence: band=very_high; score=0.9
- Line 71: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum += input[batch_idx * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.9
- Line 92: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum += input[batch_idx * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * - grad_B = input^T @ (grad_output @ A^T * scaling)
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * - grad_input = (grad_output @ A^T) @ B^T * scaling
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,        // [batch_size, in_dim]
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float* grad_input,         // [batch_size, in_dim]
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_type: 0 = grad_A, 1 = grad_B, 2 = grad_input
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Compute h[r] = input[b] @ B[:, r]
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: h_val += input[b * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.9
- Line 170: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Compute grad_B = input^T @ (grad_output @ A^T * scaling)
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum += input[b * in_dim + i] * temp * scaling;
  Confidence: band=very_high; score=0.9
- Line 189: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Compute grad_input = (grad_output @ A^T) @ B^T * scaling
  Confidence: band=very_high; score=0.9
- Line 190: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_input[batch_size, in_dim]
  Confidence: band=very_high; score=0.9
- Line 205: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_input[b * in_dim + i] = sum * scaling;
  Confidence: band=very_high; score=0.9
- Line 325: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input == nullptr || B == nullptr || A == nullptr || output == nullptr) {
  Confidence: band=very_high; score=0.9
- Line 353: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input, B, A, output, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.9
- Line 356: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input, B, A, output, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.9
- Line 363: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 369: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float* grad_input,
  Confidence: band=very_high; score=0.9
- Line 377: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input == nullptr || B == nullptr || A == nullptr || grad_output == nullptr ||
  Confidence: band=very_high; score=0.9
- Line 378: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_A == nullptr || grad_B == nullptr || grad_input == nullptr) {
  Confidence: band=very_high; score=0.9
- Line 400: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input, B, A, grad_output, grad_A, grad_B, grad_input,
  Confidence: band=very_high; score=0.9
- Line 404: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input, B, A, grad_output, grad_A, grad_B, grad_input,
  Confidence: band=very_high; score=0.9

### src/llm/ml_model_manager.cpp
Total findings: 59

- Line 433: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (request.input_data.is_object()) {
  Confidence: band=very_high; score=0.99
- Line 434: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (request.input_data.contains("prompt")) {
  Confidence: band=very_high; score=0.99
- Line 436: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: request.input_data["prompt"].get<std::string>();
  Confidence: band=very_high; score=0.99
- Line 437: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: } else if (request.input_data.contains("text")) {
  Confidence: band=very_high; score=0.99
- Line 439: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: request.input_data["text"].get<std::string>();
  Confidence: band=very_high; score=0.99
- Line 441: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: } else if (request.input_data.is_string()) {
  Confidence: band=very_high; score=0.99
- Line 442: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: eng_req.base_request.prompt = request.input_data.get<std::string>();
  Confidence: band=very_high; score=0.99
- Line 829: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: CachedModel* cached = config_.model_loader->getOrLoadModel(
  Confidence: band=very_high; score=0.99
- Line 861: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Only unload model weights from LazyModelLoader when this is the
  Confidence: band=very_high; score=0.99
- Line 864: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: config_.model_loader->unloadModel(model_id);
  Confidence: band=very_high; score=0.99
- Line 11: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 382: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Inference Operations
  Confidence: band=very_high; score=0.9
- Line 385: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: Result<MLInferenceResponse> MLModelManager::infer(const MLInferenceRequest& request) {
  Confidence: band=very_high; score=0.9
- Line 391: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: MLInferenceResponse response;
  Confidence: band=very_high; score=0.9
- Line 403: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: MLInferenceResponse response;
  Confidence: band=very_high; score=0.9
- Line 408: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto infer_start = std::chrono::steady_clock::now();
  Confidence: band=very_high; score=0.9
- Line 410: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceDispatchFn dispatch_fn;
  Confidence: band=very_high; score=0.9
- Line 413: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: dispatch_fn = inference_dispatch_fn_;
  Confidence: band=very_high; score=0.9
- Line 422: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.error_message = std::string("Inference dispatch error: ") + ex.what();
  Confidence: band=very_high; score=0.9
- Line 424: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: } else if (config_.inference_engine) {
  Confidence: band=very_high; score=0.9
- Line 426: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Route via the configured InferenceEngineEnhanced.
  Confidence: band=very_high; score=0.9
- Line 427: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced::EnhancedInferenceRequest eng_req;
  Confidence: band=very_high; score=0.9
- Line 430: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: request.inference_params.value("max_tokens", 512);
  Confidence: band=very_high; score=0.9
- Line 432: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: static_cast<float>(request.inference_params.value("temperature", 0.7));
  Confidence: band=very_high; score=0.9
- Line 433: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (request.input_data.is_object()) {
  Confidence: band=very_high; score=0.9
- Line 434: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (request.input_data.contains("prompt")) {
  Confidence: band=very_high; score=0.9
- Line 436: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: request.input_data["prompt"].get<std::string>();
  Confidence: band=very_high; score=0.9
- Line 437: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: } else if (request.input_data.contains("text")) {
  Confidence: band=very_high; score=0.9
- Line 439: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: request.input_data["text"].get<std::string>();
  Confidence: band=very_high; score=0.9
- Line 441: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: } else if (request.input_data.is_string()) {
  Confidence: band=very_high; score=0.9
- Line 442: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: eng_req.base_request.prompt = request.input_data.get<std::string>();
  Confidence: band=very_high; score=0.9
- Line 448: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto handle   = config_.inference_engine->submit(eng_req);
  Confidence: band=very_high; score=0.9
- Line 459: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.error_message = std::string("InferenceEngine error: ") + ex.what();
  Confidence: band=very_high; score=0.9
- Line 463: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.error_message = "No InferenceEngineEnhanced configured in MLModelManager::Config. "
  Confidence: band=very_high; score=0.9
- Line 464: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Set config.inference_engine to enable real inference.";
  Confidence: band=very_high; score=0.9
- Line 467: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto infer_end = std::chrono::steady_clock::now();
  Confidence: band=very_high; score=0.9
- Line 468: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto inference_time = std::chrono::duration_cast<std::chrono::milliseconds>(
  Confidence: band=very_high; score=0.9
- Line 469: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: infer_end - infer_start
  Confidence: band=very_high; score=0.9
- Line 472: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.inference_time_ms = static_cast<float>(inference_time);
  Confidence: band=very_high; score=0.9
- Line 473: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.total_time_ms = response.queue_time_ms + response.inference_time_ms;
  Confidence: band=very_high; score=0.9
- Line 489: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void MLModelManager::setInferenceDispatchFn(InferenceDispatchFn fn) {
  Confidence: band=very_high; score=0.9
- Line 491: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_dispatch_fn_ = std::move(fn);
  Confidence: band=very_high; score=0.9
- Line 494: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string MLModelManager::inferAsync(
  Confidence: band=very_high; score=0.9
- Line 495: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const MLInferenceRequest& request,
  Confidence: band=very_high; score=0.9
- Line 496: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::function<void(const MLInferenceResponse&)> callback
  Confidence: band=very_high; score=0.9
- Line 500: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Launch async inference
  Confidence: band=very_high; score=0.9
- Line 502: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto result = this->infer(request);
  Confidence: band=very_high; score=0.9
- Line 504: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: MLInferenceResponse error_response{};
  Confidence: band=very_high; score=0.9
- Line 517: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool MLModelManager::cancelInference(const std::string& request_id) {
  Confidence: band=very_high; score=0.9
- Line 825: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: json load_cfg = config.inference_config;
  Confidence: band=very_high; score=0.9
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: instance_ids.push_back(result.value());
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: instance_ids.push_back(result.value());
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_instance_ids.push_back(result.value());
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_instance_ids.push_back(result.value());
  Confidence: band=high; score=0.74
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(model_id);
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*inst);
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: instances.push_back(inst->toJSON());
  Confidence: band=high; score=0.74
- Line 385: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: Result<MLInferenceResponse> MLModelManager::infer(const MLInferenceRequest& request) {
  Confidence: band=high; score=0.74
- Line 502: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto result = this->infer(request);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/kernels/cpu_fused_kernels.cpp
Total findings: 52

- Line 42: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param input Input tensor [batch_size, in_dim]
  Confidence: band=very_high; score=0.99
- Line 47: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param in_dim Input dimension
  Confidence: band=very_high; score=0.99
- Line 53: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 63: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Allocate intermediate result: h = input @ B^T
  Confidence: band=very_high; score=0.99
- Line 66: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Step 1: Compute h = input @ B^T
  Confidence: band=very_high; score=0.99
- Line 67: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // h[batch_idx, r] = sum_i(input[batch_idx, i] * B[i, r])
  Confidence: band=very_high; score=0.99
- Line 72: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum += input[b * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.99
- Line 95: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * - grad_input = (grad_output @ A) @ B * scaling
  Confidence: band=very_high; score=0.99
- Line 97: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * - grad_B = input^T @ (grad_output @ A) * scaling
  Confidence: band=very_high; score=0.99
- Line 99: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param input Input tensor [batch_size, in_dim] (from forward)
  Confidence: band=very_high; score=0.99
- Line 105: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param grad_input Gradient w.r.t input [batch_size, in_dim]
  Confidence: band=very_high; score=0.99
- Line 107: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param in_dim Input dimension
  Confidence: band=very_high; score=0.99
- Line 113: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 119: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float* grad_input,
  Confidence: band=very_high; score=0.99
- Line 126: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Recompute intermediate from forward pass: h = input @ B^T
  Confidence: band=very_high; score=0.99
- Line 132: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum += input[b * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.99
- Line 141: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::fill(grad_input, grad_input + batch_size * in_dim, 0.0f);
  Confidence: band=very_high; score=0.99
- Line 167: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Compute grad_B = input^T @ grad_intermediate
  Confidence: band=very_high; score=0.99
- Line 168: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_B[i, r] = sum_b(input[b, i] * grad_intermediate[b, r])
  Confidence: band=very_high; score=0.99
- Line 173: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum += input[b * in_dim + i] * grad_intermediate[b * rank + r];
  Confidence: band=very_high; score=0.99
- Line 179: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Compute grad_input = grad_intermediate @ B^T
  Confidence: band=very_high; score=0.99
- Line 180: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_input[b, i] = sum_r(grad_intermediate[b, r] * B[i, r])
  Confidence: band=very_high; score=0.99
- Line 187: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_input[b * in_dim + i] = sum;
  Confidence: band=very_high; score=0.99
- Line 198: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 211: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Step 1: Compute h = input @ B^T (parallelized over batch)
  Confidence: band=very_high; score=0.99
- Line 217: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum += input[b * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.99
- Line 42: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param input Input tensor [batch_size, in_dim]
  Confidence: band=very_high; score=0.9
- Line 47: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param in_dim Input dimension
  Confidence: band=very_high; score=0.9
- Line 53: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 63: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Allocate intermediate result: h = input @ B^T
  Confidence: band=very_high; score=0.9
- Line 66: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Step 1: Compute h = input @ B^T
  Confidence: band=very_high; score=0.9
- Line 67: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // h[batch_idx, r] = sum_i(input[batch_idx, i] * B[i, r])
  Confidence: band=very_high; score=0.9
- Line 72: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum += input[b * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * - grad_input = (grad_output @ A) @ B * scaling
  Confidence: band=very_high; score=0.9
- Line 97: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * - grad_B = input^T @ (grad_output @ A) * scaling
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param input Input tensor [batch_size, in_dim] (from forward)
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param grad_input Gradient w.r.t input [batch_size, in_dim]
  Confidence: band=very_high; score=0.9
- Line 107: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param in_dim Input dimension
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float* grad_input,
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Recompute intermediate from forward pass: h = input @ B^T
  Confidence: band=very_high; score=0.9
- Line 132: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum += input[b * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.9
- Line 141: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::fill(grad_input, grad_input + batch_size * in_dim, 0.0f);
  Confidence: band=very_high; score=0.9
- Line 167: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Compute grad_B = input^T @ grad_intermediate
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_B[i, r] = sum_b(input[b, i] * grad_intermediate[b, r])
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum += input[b * in_dim + i] * grad_intermediate[b * rank + r];
  Confidence: band=very_high; score=0.9
- Line 179: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Compute grad_input = grad_intermediate @ B^T
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_input[b, i] = sum_r(grad_intermediate[b, r] * B[i, r])
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_input[b * in_dim + i] = sum;
  Confidence: band=very_high; score=0.9
- Line 198: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 211: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Step 1: Compute h = input @ B^T (parallelized over batch)
  Confidence: band=very_high; score=0.9
- Line 217: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum += input[b * in_dim + i] * B[i * rank + r];
  Confidence: band=very_high; score=0.9

### src/llm/lora_framework/data_loader.cpp
Total findings: 51

- Line 142: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input = item.value("input", "");
  Confidence: band=very_high; score=0.99
- Line 151: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input = j.value("input", "");
  Confidence: band=very_high; score=0.99
- Line 216: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input = j.value("input", "");
  Confidence: band=very_high; score=0.99
- Line 295: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input = "";
  Confidence: band=very_high; score=0.99
- Line 312: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!sample.input.empty()) {
  Confidence: band=very_high; score=0.99
- Line 313: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: formatted += config_.input_prefix + sample.input;
  Confidence: band=very_high; score=0.99
- Line 326: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input_ids = tokenizer_->encode(formatted_text, true, false);
  Confidence: band=very_high; score=0.99
- Line 328: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // For causal language modeling, labels are the same as inputs shifted by 1
  Confidence: band=very_high; score=0.99
- Line 330: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.label_ids = sample.input_ids;
  Confidence: band=very_high; score=0.99
- Line 334: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input_ids.size() > static_cast<size_t>(config_.max_sequence_length)) {
  Confidence: band=very_high; score=0.99
- Line 335: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input_ids.resize(config_.max_sequence_length);
  Confidence: band=very_high; score=0.99
- Line 370: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch.input_ids.push_back(sample.input_ids);
  Confidence: band=very_high; score=0.99
- Line 372: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch.sequence_lengths.push_back(sample.input_ids.size());
  Confidence: band=very_high; score=0.99
- Line 376: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<int>(sample.input_ids.size())
  Confidence: band=very_high; score=0.99
- Line 392: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t i = 0; i < batch.input_ids.size(); ++i) {
  Confidence: band=very_high; score=0.99
- Line 393: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto& input_seq = batch.input_ids[i];
  Confidence: band=very_high; score=0.99
- Line 397: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: while (input_seq.size() < static_cast<size_t>(target_length)) {
  Confidence: band=very_high; score=0.99
- Line 398: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_seq.push_back(config_.pad_token_id);
  Confidence: band=very_high; score=0.99
- Line 451: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input = item.value("input", "");
  Confidence: band=very_high; score=0.99
- Line 501: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input = "";
  Confidence: band=very_high; score=0.99
- Line 552: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input = "";
  Confidence: band=very_high; score=0.99
- Line 142: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input = item.value("input", "");
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input = j.value("input", "");
  Confidence: band=very_high; score=0.9
- Line 216: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input = j.value("input", "");
  Confidence: band=very_high; score=0.9
- Line 295: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input = "";
  Confidence: band=very_high; score=0.9
- Line 312: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!sample.input.empty()) {
  Confidence: band=very_high; score=0.9
- Line 313: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: formatted += config_.input_prefix + sample.input;
  Confidence: band=very_high; score=0.9
- Line 326: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input_ids = tokenizer_->encode(formatted_text, true, false);
  Confidence: band=very_high; score=0.9
- Line 328: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // For causal language modeling, labels are the same as inputs shifted by 1
  Confidence: band=very_high; score=0.9
- Line 328: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // For causal language modeling, labels are the same as inputs shifted by 1
  Confidence: band=very_high; score=0.9
- Line 330: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.label_ids = sample.input_ids;
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input_ids.size() > static_cast<size_t>(config_.max_sequence_length)) {
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input_ids.resize(config_.max_sequence_length);
  Confidence: band=very_high; score=0.9
- Line 370: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch.input_ids.push_back(sample.input_ids);
  Confidence: band=very_high; score=0.9
- Line 372: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch.sequence_lengths.push_back(sample.input_ids.size());
  Confidence: band=very_high; score=0.9
- Line 376: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<int>(sample.input_ids.size())
  Confidence: band=very_high; score=0.9
- Line 392: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t i = 0; i < batch.input_ids.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 393: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto& input_seq = batch.input_ids[i];
  Confidence: band=very_high; score=0.9
- Line 397: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: while (input_seq.size() < static_cast<size_t>(target_length)) {
  Confidence: band=very_high; score=0.9
- Line 398: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_seq.push_back(config_.pad_token_id);
  Confidence: band=very_high; score=0.9
- Line 451: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input = item.value("input", "");
  Confidence: band=very_high; score=0.9
- Line 501: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input = "";
  Confidence: band=very_high; score=0.9
- Line 552: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input = "";
  Confidence: band=very_high; score=0.9
- Line 44: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(token_id);
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples_.push_back(sample);
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples_.push_back(sample);
  Confidence: band=high; score=0.74
- Line 353: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_indices.push_back(indices_[i]);
  Confidence: band=high; score=0.74
- Line 369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.input_ids.push_back(sample.input_ids);
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: input_seq.push_back(config_.pad_token_id);
  Confidence: band=high; score=0.74
- Line 454: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(sample);
  Confidence: band=high; score=0.74
- Line 502: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(sample);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/kernels/hip_kernels.cpp
Total findings: 46

- Line 40: severity=CRITICAL; category=gpu_memory_safety; pattern=missing_sync_threads
  Description: Shared memory access in CUDA kernel without __syncthreads()
  Context: __global__ void multiply_kernel(const float* A, const float* B, float* C, size_t size) {
  Confidence: band=very_high; score=0.99
- Line 170: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 191: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float input_val = input[b * in_dim + i];
  Confidence: band=very_high; score=0.99
- Line 195: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum += b_val * input_val * grad_val;
  Confidence: band=very_high; score=0.99
- Line 206: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 229: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float input_val = input[b * in_dim + i];
  Confidence: band=very_high; score=0.99
- Line 231: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum += grad_val * a_val * input_val;
  Confidence: band=very_high; score=0.99
- Line 447: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_overflow
  Context: hipLaunchKernelGGL(check_inf_nan_kernel, gridSize, blockSize, 0, 0, data, size, d_overflow);
  Confidence: band=very_high; score=0.99
- Line 462: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: d_overflow
  Context: err = hipMemcpy(&h_overflow, d_overflow, sizeof(int), hipMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.99
- Line 504: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 520: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input, B, grad_output, grad_A, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.99
- Line 523: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input, B, grad_output, grad_A, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.99
- Line 530: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 546: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input, A, grad_output, grad_B, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.99
- Line 549: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input, A, grad_output, grad_B, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.99
- Line 758: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,     // [batch_size, seq_len, hidden_dim]
  Confidence: band=very_high; score=0.99
- Line 774: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_idx = batch_idx * seq_len * hidden_dim +
  Confidence: band=very_high; score=0.99
- Line 777: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum += input[input_idx];
  Confidence: band=very_high; score=0.99
- Line 786: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 802: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: output, input, batch_size, seq_len, hidden_dim);
  Confidence: band=very_high; score=0.99
- Line 806: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: output, input, batch_size, seq_len, hidden_dim);
  Confidence: band=very_high; score=0.99
- Line 170: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float input_val = input[b * in_dim + i];
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum += b_val * input_val * grad_val;
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 229: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float input_val = input[b * in_dim + i];
  Confidence: band=very_high; score=0.9
- Line 231: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum += grad_val * a_val * input_val;
  Confidence: band=very_high; score=0.9
- Line 504: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 520: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input, B, grad_output, grad_A, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.9
- Line 523: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input, B, grad_output, grad_A, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.9
- Line 530: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 546: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input, A, grad_output, grad_B, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.9
- Line 549: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input, A, grad_output, grad_B, batch_size, in_dim, rank, out_dim, scaling);
  Confidence: band=very_high; score=0.9
- Line 758: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,     // [batch_size, seq_len, hidden_dim]
  Confidence: band=very_high; score=0.9
- Line 765: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: size_t total_outputs = batch_size * hidden_dim;
  Confidence: band=very_high; score=0.9
- Line 767: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (idx < total_outputs) {
  Confidence: band=very_high; score=0.9
- Line 774: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_idx = batch_idx * seq_len * hidden_dim +
  Confidence: band=very_high; score=0.9
- Line 777: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum += input[input_idx];
  Confidence: band=very_high; score=0.9
- Line 786: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 792: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: size_t total_outputs = batch_size * hidden_dim;
  Confidence: band=very_high; score=0.9
- Line 796: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const int num_blocks = (total_outputs + threads_per_block - 1) / threads_per_block;
  Confidence: band=very_high; score=0.9
- Line 802: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: output, input, batch_size, seq_len, hidden_dim);
  Confidence: band=very_high; score=0.9
- Line 806: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: output, input, batch_size, seq_len, hidden_dim);
  Confidence: band=very_high; score=0.9
- Line 603: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: RocblasHandle::RocblasHandle() {
  Confidence: band=high; score=0.74
- Line 613: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: RocblasHandle::~RocblasHandle() {
  Confidence: band=high; score=0.74
- Line 619: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: RocblasHandle::RocblasHandle(RocblasHandle&& other) noexcept
  Confidence: band=high; score=0.74

### src/llm/lora_framework/multi_gpu_lora_layer.cpp
Total findings: 46

- Line 115: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<GPUTensor> MultiGPULoRALayer::forward(const std::vector<GPUTensor>& inputs) {
  Confidence: band=very_high; score=0.99
- Line 116: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (inputs.size() != static_cast<size_t>(ctx_.num_gpus())) {
  Confidence: band=very_high; score=0.99
- Line 118: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Number of input tensors must match number of GPUs");
  Confidence: band=very_high; score=0.99
- Line 124: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: outputs.reserve(inputs.size());
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<GPUTensor> grad_inputs;
  Confidence: band=very_high; score=0.99
- Line 160: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_inputs.reserve(grad_outputs.size());
  Confidence: band=very_high; score=0.99
- Line 164: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_inputs.push_back(layers_[i]->backward(grad_outputs[i]));
  Confidence: band=very_high; score=0.99
- Line 173: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return grad_inputs;
  Confidence: band=very_high; score=0.99
- Line 115: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<GPUTensor> MultiGPULoRALayer::forward(const std::vector<GPUTensor>& inputs) {
  Confidence: band=very_high; score=0.9
- Line 115: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<GPUTensor> MultiGPULoRALayer::forward(const std::vector<GPUTensor>& inputs) {
  Confidence: band=very_high; score=0.9
- Line 116: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (inputs.size() != static_cast<size_t>(ctx_.num_gpus())) {
  Confidence: band=very_high; score=0.9
- Line 116: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (inputs.size() != static_cast<size_t>(ctx_.num_gpus())) {
  Confidence: band=very_high; score=0.9
- Line 118: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Number of input tensors must match number of GPUs");
  Confidence: band=very_high; score=0.9
- Line 123: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<GPUTensor> outputs;
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: outputs.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: outputs.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 130: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Verify input is on correct device
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (inputs[i].device().device_id != expected_device.device_id ||
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (inputs[i].device().device_id != expected_device.device_id ||
  Confidence: band=very_high; score=0.9
- Line 132: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: inputs[i].device().type != expected_device.type) {
  Confidence: band=very_high; score=0.9
- Line 132: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inputs[i].device().type != expected_device.type) {
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Input tensor " + std::to_string(i) + " is not on correct device");
  Confidence: band=very_high; score=0.9
- Line 137: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: outputs.push_back(layers_[i]->forward(inputs[i]));
  Confidence: band=very_high; score=0.9
- Line 137: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: outputs.push_back(layers_[i]->forward(inputs[i]));
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: return outputs;
  Confidence: band=very_high; score=0.9
- Line 150: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<GPUTensor>& grad_outputs) {
  Confidence: band=very_high; score=0.9
- Line 152: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (grad_outputs.size() != static_cast<size_t>(ctx_.num_gpus())) {
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<GPUTensor> grad_inputs;
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<GPUTensor> grad_inputs;
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: grad_inputs.reserve(grad_outputs.size());
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_inputs.reserve(grad_outputs.size());
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (size_t i = 0; i < grad_outputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 164: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: grad_inputs.push_back(layers_[i]->backward(grad_outputs[i]));
  Confidence: band=very_high; score=0.9
- Line 164: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_inputs.push_back(layers_[i]->backward(grad_outputs[i]));
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: return grad_inputs;
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return grad_inputs;
  Confidence: band=very_high; score=0.9
- Line 36: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: layers_.push_back(
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outputs.push_back(layers_[i]->forward(inputs[i]));
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grad_inputs.push_back(layers_[i]->backward(grad_outputs[i]));
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_gradients.push_back(layer->gradients());
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: param_grads.push_back(all_gradients[gpu_idx][param_idx]);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: param_grads.push_back(all_gradients[gpu_idx][param_idx]);
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto master_params = layers_[0]->parameters();
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto target_params = layers_[i]->parameters();
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto master_data = master_params[j]->cpu_data();
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(layer.get());
  Confidence: band=high; score=0.74

### src/llm/aql_train_parser.cpp
Total findings: 45

- Line 442: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<std::string> AQLTrainParser::tokenize(const std::string& input) {
  Confidence: band=very_high; score=0.99
- Line 444: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::istringstream iss(input);
  Confidence: band=very_high; score=0.99
- Line 453: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string& input,
  Confidence: band=very_high; score=0.99
- Line 456: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto pos = findKeyword(input, keyword);
  Confidence: band=very_high; score=0.99
- Line 460: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string rest = input.substr(pos + keyword.size());
  Confidence: band=very_high; score=0.99
- Line 479: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string& input
  Confidence: band=very_high; score=0.99
- Line 483: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t n = input.size();
  Confidence: band=very_high; score=0.99
- Line 486: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: while (i < n && std::isspace(static_cast<unsigned char>(input[i]))) {
  Confidence: band=very_high; score=0.99
- Line 493: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (pos < n && (input[pos] == ',' || input[pos] == '{' || input[pos] == '}')) {
  Confidence: band=very_high; score=0.99
- Line 503: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const unsigned char ch = static_cast<unsigned char>(input[pos]);
  Confidence: band=very_high; score=0.99
- Line 504: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (std::isalnum(ch) || input[pos] == '_') {
  Confidence: band=very_high; score=0.99
- Line 515: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string key = toLower(input.substr(keyStart, pos - keyStart));
  Confidence: band=very_high; score=0.99
- Line 517: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (pos >= n || (input[pos] != '=' && input[pos] != ':')) {
  Confidence: band=very_high; score=0.99
- Line 524: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (pos < n && (input[pos] == '\'' || input[pos] == '"')) {
  Confidence: band=very_high; score=0.99
- Line 525: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const char quote = input[pos++];
  Confidence: band=very_high; score=0.99
- Line 527: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: while (pos < n && input[pos] != quote) {
  Confidence: band=very_high; score=0.99
- Line 530: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: value = input.substr(valueStart, pos - valueStart);
  Confidence: band=very_high; score=0.99
- Line 531: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (pos < n && input[pos] == quote) {
  Confidence: band=very_high; score=0.99
- Line 536: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: while (pos < n && input[pos] != ',' && input[pos] != '}') {
  Confidence: band=very_high; score=0.99
- Line 539: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: value = themis::utils::trim(input.substr(valueStart, pos - valueStart));
  Confidence: band=very_high; score=0.99
- Line 442: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<std::string> AQLTrainParser::tokenize(const std::string& input) {
  Confidence: band=very_high; score=0.9
- Line 444: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::istringstream iss(input);
  Confidence: band=very_high; score=0.9
- Line 453: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string& input,
  Confidence: band=very_high; score=0.9
- Line 456: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto pos = findKeyword(input, keyword);
  Confidence: band=very_high; score=0.9
- Line 460: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string rest = input.substr(pos + keyword.size());
  Confidence: band=very_high; score=0.9
- Line 479: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string& input
  Confidence: band=very_high; score=0.9
- Line 483: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t n = input.size();
  Confidence: band=very_high; score=0.9
- Line 486: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: while (i < n && std::isspace(static_cast<unsigned char>(input[i]))) {
  Confidence: band=very_high; score=0.9
- Line 493: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (pos < n && (input[pos] == ',' || input[pos] == '{' || input[pos] == '}')) {
  Confidence: band=very_high; score=0.9
- Line 503: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const unsigned char ch = static_cast<unsigned char>(input[pos]);
  Confidence: band=very_high; score=0.9
- Line 504: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (std::isalnum(ch) || input[pos] == '_') {
  Confidence: band=very_high; score=0.9
- Line 515: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string key = toLower(input.substr(keyStart, pos - keyStart));
  Confidence: band=very_high; score=0.9
- Line 517: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (pos >= n || (input[pos] != '=' && input[pos] != ':')) {
  Confidence: band=very_high; score=0.9
- Line 524: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (pos < n && (input[pos] == '\'' || input[pos] == '"')) {
  Confidence: band=very_high; score=0.9
- Line 525: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const char quote = input[pos++];
  Confidence: band=very_high; score=0.9
- Line 527: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: while (pos < n && input[pos] != quote) {
  Confidence: band=very_high; score=0.9
- Line 530: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: value = input.substr(valueStart, pos - valueStart);
  Confidence: band=very_high; score=0.9
- Line 531: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (pos < n && input[pos] == quote) {
  Confidence: band=very_high; score=0.9
- Line 536: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: while (pos < n && input[pos] != ',' && input[pos] != '}') {
  Confidence: band=very_high; score=0.9
- Line 539: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: value = themis::utils::trim(input.substr(valueStart, pos - valueStart));
  Confidence: band=very_high; score=0.9
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: e.relational_joins.push_back(RelationalJoinConfig::fromJSON(jc));
  Confidence: band=high; score=0.74
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: e.relational_joins.push_back(RelationalJoinConfig::fromJSON(jc));
  Confidence: band=high; score=0.74
- Line 717: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: e.relational_joins.push_back(parseRelationalJoin((*it)[1].str()));
  Confidence: band=high; score=0.74
- Line 750: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cfg.participant_shards.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 877: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stmt->target_shards.push_back((*it)[1].str());
  Confidence: band=high; score=0.74

### src/llm/production_validator.cpp
Total findings: 45

- Line 1048: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* model = loader.getOrLoadModel("__nonexistent__", "/tmp/__no_such_model.gguf");
  Confidence: band=very_high; score=0.99
- Line 1229: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* m1 = loader.getOrLoadModel("switch_model_1", "/tmp/__switch_m1.gguf");
  Confidence: band=very_high; score=0.99
- Line 1230: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* m2 = loader.getOrLoadModel("switch_model_2", "/tmp/__switch_m2.gguf");
  Confidence: band=very_high; score=0.99
- Line 1316: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* model = loader.getOrLoadModel("fail_test", "/nonexistent/path/model.gguf");
  Confidence: band=very_high; score=0.99
- Line 11: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 50: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void ProductionValidator::setInferenceEngine(
  Confidence: band=very_high; score=0.9
- Line 51: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<InferenceEngineEnhanced> engine) {
  Confidence: band=very_high; score=0.9
- Line 52: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_engine_ = std::move(engine);
  Confidence: band=very_high; score=0.9
- Line 55: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ProductionValidator::ProductionMetrics ProductionValidator::benchmarkInference(
  Confidence: band=very_high; score=0.9
- Line 86: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Route through InferenceEngineEnhanced when configured.
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (inference_engine_) {
  Confidence: band=very_high; score=0.9
- Line 88: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced::EnhancedInferenceRequest eng_req;
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto handle   = inference_engine_->submit(eng_req);
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::warn("Benchmark request {} inference failed: {}", i, inner.what());
  Confidence: band=very_high; score=0.9
- Line 106: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::warn("Benchmark skipped: No InferenceEngineEnhanced configured. "
  Confidence: band=very_high; score=0.9
- Line 107: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Call setInferenceEngine() to enable real benchmarking.");
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: all_passed &= testInferencePipeline();
  Confidence: band=very_high; score=0.9
- Line 327: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Run an actual inference request through the engine when available.
  Confidence: band=very_high; score=0.9
- Line 329: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (inference_engine_) {
  Confidence: band=very_high; score=0.9
- Line 330: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced::EnhancedInferenceRequest eng_req;
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto handle = inference_engine_->submit(eng_req);
  Confidence: band=very_high; score=0.9
- Line 449: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Simulate one inference unit (wall-clock latency is what matters here)
  Confidence: band=very_high; score=0.9
- Line 590: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Verify InferenceEngineEnhanced can be built and started with a minimal config.
  Confidence: band=very_high; score=0.9
- Line 593: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced::Config eng_cfg;
  Confidence: band=very_high; score=0.9
- Line 596: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced engine(eng_cfg);
  Confidence: band=very_high; score=0.9
- Line 600: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("  InferenceEngineEnhanced instantiated; {} model(s) available.",
  Confidence: band=very_high; score=0.9
- Line 603: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("  InferenceEngineEnhanced construction failed: {}", e.what());
  Confidence: band=very_high; score=0.9
- Line 734: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 1078: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req_lo, req_hi;
  Confidence: band=very_high; score=0.9
- Line 1112: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool IntegrationTestSuite::testKernelFusionWithInference() {
  Confidence: band=very_high; score=0.9
- Line 1113: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Integration Test: KernelFusion + Inference");
  Confidence: band=very_high; score=0.9
- Line 1166: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 1343: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 1432: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (int i = 0; i < kPerThread; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1433: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 1476: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 1514: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 1555: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"KernelFusion+Inference", [this]() { return testKernelFusionWithInference(); }},
  Confidence: band=very_high; score=0.9
- Line 466: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back(worker);
  Confidence: band=high; score=0.74
- Line 540: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: msg += "\n  - " + r;
  Confidence: band=high; score=0.74
- Line 1393: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!id.empty()) lo_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 1430: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: threads.emplace_back([&]() {
  Confidence: band=high; score=0.74
- Line 1439: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 1524: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 1584: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74

### src/llm/vision_config.cpp
Total findings: 45

- Line 205: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Input validation
  Confidence: band=very_high; score=0.99
- Line 24: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility matrix - can be expanded
  Confidence: band=high; score=0.8
- Line 119: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: vision_config->resource_limits_.max_inference_time = std::chrono::seconds(limits["max_inference_time_seconds"].as<int>(60));
  Confidence: band=very_high; score=0.9
- Line 150: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: vision_config->resource_quota_.total_inference_minutes = defaults["total_inference_time_minutes"].as<size_t>(600);
  Confidence: band=very_high; score=0.9
- Line 205: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Input validation
  Confidence: band=very_high; score=0.9
- Line 338: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: vision_config->feature_flags_["llava_inference"] = features["llava_inference"].as<bool>(true);
  Confidence: band=very_high; score=0.9
- Line 346: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: vision_config->experimental_features_["batch_inference"] = experimental["batch_inference"].as<bool>(true);
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: vision_config->experimental_features_["distributed_inference"] = experimental["distributed_inference"].as<bool>(false);
  Confidence: band=very_high; score=0.9
- Line 429: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (rl.contains("max_inference_time_s") && rl["max_inference_time_s"].is_number()) {
  Confidence: band=very_high; score=0.9
- Line 430: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: vision_config->resource_limits_.max_inference_time =
  Confidence: band=very_high; score=0.9
- Line 431: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::chrono::seconds(rl["max_inference_time_s"].get<int64_t>());
  Confidence: band=very_high; score=0.9
- Line 555: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: config->resource_limits_.max_inference_time = std::chrono::seconds(60);
  Confidence: band=very_high; score=0.9
- Line 75: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto api = config["vision"]["api"];
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto licensing = config["vision"]["licensing"];
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vision_config->allowed_licenses_.push_back(license.as<std::string>());
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto limits = config["vision"]["resources"]["limits"];
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto rate_limiting = config["vision"]["resources"]["rate_limiting"];
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto global = rate_limiting["global"];
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto quotas = config["vision"]["resources"]["quotas"];
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto defaults = quotas["default"];
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto monitoring = config["vision"]["monitoring"];
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto metrics = monitoring["metrics"];
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto prometheus = metrics["prometheus"];
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto audit = monitoring["audit"];
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vision_config->monitoring_config_.audit.events.push_back(event.as<std::string>());
  Confidence: band=high; score=0.74
- Line 191: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto storage = audit["storage"];
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto security = config["vision"]["security"];
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto validation = security["validation"];
  Confidence: band=high; score=0.74
- Line 212: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto res = validation["max_image_resolution"];
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vision_config->security_config_.validation.allowed_formats.push_back(format.as<std::string>());
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sandboxing = security["sandboxing"];
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto model_verification = security["model_verification"];
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vision_config->security_config_.model_verification.trusted_publishers.push_back(publisher.as<std::string>());
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto access_control = security["access_control"];
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vision_config->security_config_.access_control.allowed_roles.push_back(role.as<std::string>());
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto pipeline = config["vision"]["pipeline"];
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto error_handling = pipeline["error_handling"];
  Confidence: band=high; score=0.74
- Line 294: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto retry = error_handling["retry"];
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto fallback = error_handling["fallback"];
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto preprocessing = pipeline["preprocessing"];
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto postprocessing = pipeline["postprocessing"];
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto features = config["vision"]["features"];
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto experimental = features["experimental"];
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vision_config->allowed_licenses_.push_back(lic.get<std::string>());
  Confidence: band=high; score=0.74
- Line 653: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: model_ids.push_back(pair.first);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/lora_layers.cpp
Total findings: 44

- Line 233: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor LoRALayer::forward(const Tensor& input) {
  Confidence: band=very_high; score=0.99
- Line 234: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: spdlog::debug("{}: forward with input shape ({}, {})",
  Confidence: band=very_high; score=0.99
- Line 235: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: name_, input.shape()[0], input.shape().size() > 1 ? input.shape()[1] : 0);
  Confidence: band=very_high; score=0.99
- Line 237: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Cache input for backward pass
  Confidence: band=very_high; score=0.99
- Line 238: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cached_input_ = std::make_unique<Tensor>(input.clone());
  Confidence: band=very_high; score=0.99
- Line 243: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Compute output = input @ BA * scaling
  Confidence: band=very_high; score=0.99
- Line 244: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor output = input.matmul(*cached_BA_);
  Confidence: band=very_high; score=0.99
- Line 257: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // For LoRA: output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.99
- Line 260: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_A = B.T @ input.T @ scaled_grad
  Confidence: band=very_high; score=0.99
- Line 263: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor input_T = cached_input_->transpose();
  Confidence: band=very_high; score=0.99
- Line 264: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor temp_BA = B_T.matmul(input_T);  // (rank, batch)
  Confidence: band=very_high; score=0.99
- Line 268: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_B = input.T @ scaled_grad @ A.T
  Confidence: band=very_high; score=0.99
- Line 272: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto grad_B_result = input_T.matmul(temp_AB);  // (in_dim, rank)
  Confidence: band=very_high; score=0.99
- Line 275: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // grad_input = scaled_grad @ A.T @ B.T
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor grad_input = temp_grad.matmul(B_T);  // (batch, in_dim)
  Confidence: band=very_high; score=0.99
- Line 280: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return grad_input;
  Confidence: band=very_high; score=0.99
- Line 353: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor AttentionLoRA::forward(const Tensor& input) {
  Confidence: band=very_high; score=0.99
- Line 354: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: spdlog::debug("AttentionLoRA: forward with input shape ({}, {})",
  Confidence: band=very_high; score=0.99
- Line 355: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.shape()[0], input.shape().size() > 1 ? input.shape()[1] : 0);
  Confidence: band=very_high; score=0.99
- Line 357: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor output = input.clone();
  Confidence: band=very_high; score=0.99
- Line 444: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor Sequential::forward(const Tensor& input) {
  Confidence: band=very_high; score=0.99
- Line 447: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor output = input.clone();
  Confidence: band=very_high; score=0.99
- Line 233: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor LoRALayer::forward(const Tensor& input) {
  Confidence: band=very_high; score=0.9
- Line 234: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: spdlog::debug("{}: forward with input shape ({}, {})",
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: name_, input.shape()[0], input.shape().size() > 1 ? input.shape()[1] : 0);
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Cache input for backward pass
  Confidence: band=very_high; score=0.9
- Line 238: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cached_input_ = std::make_unique<Tensor>(input.clone());
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Compute output = input @ BA * scaling
  Confidence: band=very_high; score=0.9
- Line 244: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor output = input.matmul(*cached_BA_);
  Confidence: band=very_high; score=0.9
- Line 257: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // For LoRA: output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.9
- Line 260: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_A = B.T @ input.T @ scaled_grad
  Confidence: band=very_high; score=0.9
- Line 263: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor input_T = cached_input_->transpose();
  Confidence: band=very_high; score=0.9
- Line 264: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor temp_BA = B_T.matmul(input_T);  // (rank, batch)
  Confidence: band=very_high; score=0.9
- Line 268: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_B = input.T @ scaled_grad @ A.T
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto grad_B_result = input_T.matmul(temp_AB);  // (in_dim, rank)
  Confidence: band=very_high; score=0.9
- Line 275: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // grad_input = scaled_grad @ A.T @ B.T
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor grad_input = temp_grad.matmul(B_T);  // (batch, in_dim)
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return grad_input;
  Confidence: band=very_high; score=0.9
- Line 353: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor AttentionLoRA::forward(const Tensor& input) {
  Confidence: band=very_high; score=0.9
- Line 354: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: spdlog::debug("AttentionLoRA: forward with input shape ({}, {})",
  Confidence: band=very_high; score=0.9
- Line 355: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.shape()[0], input.shape().size() > 1 ? input.shape()[1] : 0);
  Confidence: band=very_high; score=0.9
- Line 357: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor output = input.clone();
  Confidence: band=very_high; score=0.9
- Line 444: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor Sequential::forward(const Tensor& input) {
  Confidence: band=very_high; score=0.9
- Line 447: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor output = input.clone();
  Confidence: band=very_high; score=0.9

### src/llm/lora_framework/quantization.cpp
Total findings: 43

- Line 146: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: void quantize_nf4(const std::vector<float>& input,
  Confidence: band=very_high; score=0.99
- Line 150: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t total = input.size();
  Confidence: band=very_high; score=0.99
- Line 172: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float min_val = input[start];
  Confidence: band=very_high; score=0.99
- Line 173: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float max_val = input[start];
  Confidence: band=very_high; score=0.99
- Line 175: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: min_val = std::min(min_val, input[i]);
  Confidence: band=very_high; score=0.99
- Line 176: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: max_val = std::max(max_val, input[i]);
  Confidence: band=very_high; score=0.99
- Line 189: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float normalized = (input[i] - zero_point) / scale;
  Confidence: band=very_high; score=0.99
- Line 209: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: void quantize_int8(const std::vector<float>& input,
  Confidence: band=very_high; score=0.99
- Line 213: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t total = input.size();
  Confidence: band=very_high; score=0.99
- Line 237: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: max_abs = std::max(max_abs, std::abs(input[i]));
  Confidence: band=very_high; score=0.99
- Line 248: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float scaled = input[i] / scale;
  Confidence: band=very_high; score=0.99
- Line 263: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: void dequantize(const QuantizedTensor& input, std::vector<float>& output) {
  Confidence: band=very_high; score=0.99
- Line 264: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t total = input.total_elements();
  Confidence: band=very_high; score=0.99
- Line 267: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.type() == QuantizationType::NF4) {
  Confidence: band=very_high; score=0.99
- Line 269: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t block_idx = 0; block_idx < input.num_blocks(); ++block_idx) {
  Confidence: band=very_high; score=0.99
- Line 270: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const auto& block = input.blocks()[block_idx];
  Confidence: band=very_high; score=0.99
- Line 271: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t start = block_idx * input.block_size();
  Confidence: band=very_high; score=0.99
- Line 280: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: bin = input.data()[byte_idx] & 0x0F;
  Confidence: band=very_high; score=0.99
- Line 283: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: bin = (input.data()[byte_idx] >> 4) & 0x0F;
  Confidence: band=very_high; score=0.99
- Line 291: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: } else if (input.type() == QuantizationType::INT8) {
  Confidence: band=very_high; score=0.99
- Line 293: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t block_idx = 0; block_idx < input.num_blocks(); ++block_idx) {
  Confidence: band=very_high; score=0.99
- Line 294: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const auto& block = input.blocks()[block_idx];
  Confidence: band=very_high; score=0.99
- Line 295: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t start = block_idx * input.block_size();
  Confidence: band=very_high; score=0.99
- Line 300: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: int16_t quantized = static_cast<int16_t>(input.data()[i]) - 128;
  Confidence: band=very_high; score=0.99
- Line 146: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: void quantize_nf4(const std::vector<float>& input,
  Confidence: band=very_high; score=0.9
- Line 150: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t total = input.size();
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float min_val = input[start];
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float max_val = input[start];
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: min_val = std::min(min_val, input[i]);
  Confidence: band=very_high; score=0.9
- Line 176: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: max_val = std::max(max_val, input[i]);
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: void quantize_int8(const std::vector<float>& input,
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t total = input.size();
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: max_abs = std::max(max_abs, std::abs(input[i]));
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float scaled = input[i] / scale;
  Confidence: band=very_high; score=0.9
- Line 263: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: void dequantize(const QuantizedTensor& input, std::vector<float>& output) {
  Confidence: band=very_high; score=0.9
- Line 264: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t total = input.total_elements();
  Confidence: band=very_high; score=0.9
- Line 267: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.type() == QuantizationType::NF4) {
  Confidence: band=very_high; score=0.9
- Line 269: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t block_idx = 0; block_idx < input.num_blocks(); ++block_idx) {
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const auto& block = input.blocks()[block_idx];
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t start = block_idx * input.block_size();
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: bin = input.data()[byte_idx] & 0x0F;
  Confidence: band=very_high; score=0.9
- Line 283: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: bin = (input.data()[byte_idx] >> 4) & 0x0F;
  Confidence: band=very_high; score=0.9
- Line 300: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: int16_t quantized = static_cast<int16_t>(input.data()[i]) - 128;
  Confidence: band=very_high; score=0.9

### src/llm/kernel_fusion.cpp
Total findings: 41

- Line 52: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 69: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: output, input, weight, bias,
  Confidence: band=very_high; score=0.99
- Line 97: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input_row = input + i * hidden_dim;
  Confidence: band=very_high; score=0.99
- Line 104: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: mean += input_row[j];
  Confidence: band=very_high; score=0.99
- Line 110: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float diff = input_row[j] - mean;
  Confidence: band=very_high; score=0.99
- Line 119: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float normalized = (input_row[j] - mean) * inv_std;
  Confidence: band=very_high; score=0.99
- Line 138: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 149: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input, qkv_weight, qkv_bias,
  Confidence: band=very_high; score=0.99
- Line 157: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // CPU fallback: project input to Q, K, V simultaneously
  Confidence: band=very_high; score=0.99
- Line 163: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input_row = input + i * hidden_dim;
  Confidence: band=very_high; score=0.99
- Line 167: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: query[i * hidden_dim + j] = input_row[j] * qkv_weight[j] + qkv_bias[j];
  Confidence: band=very_high; score=0.99
- Line 168: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: key[i * hidden_dim + j] = input_row[j] * qkv_weight[hidden_dim + j] + qkv_bias[hidden_dim + j];
  Confidence: band=very_high; score=0.99
- Line 169: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: value[i * hidden_dim + j] = input_row[j] * qkv_weight[2 * hidden_dim + j] + qkv_bias[2 * hidden_dim + j];
  Confidence: band=very_high; score=0.99
- Line 305: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 317: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: output, input,
  Confidence: band=very_high; score=0.99
- Line 330: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input_row = input + i * hidden_dim;
  Confidence: band=very_high; score=0.99
- Line 344: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: gate_out[j] += input_row[k] * gate_weight[k * intermediate_dim + j];
  Confidence: band=very_high; score=0.99
- Line 345: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: up_out[j] += input_row[k] * up_weight[k * intermediate_dim + j];
  Confidence: band=very_high; score=0.99
- Line 370: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input,
  Confidence: band=very_high; score=0.99
- Line 391: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const float* input_row = input + i * hidden_dim;
  Confidence: band=very_high; score=0.99
- Line 397: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sum_squares += input_row[j] * input_row[j];
  Confidence: band=very_high; score=0.99
- Line 403: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float normalized = (input_row[j] / rms) * rms_weight[j];
  Confidence: band=very_high; score=0.99
- Line 52: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 69: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: output, input, weight, bias,
  Confidence: band=very_high; score=0.9
- Line 97: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input_row = input + i * hidden_dim;
  Confidence: band=very_high; score=0.9
- Line 104: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: mean += input_row[j];
  Confidence: band=very_high; score=0.9
- Line 110: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float diff = input_row[j] - mean;
  Confidence: band=very_high; score=0.9
- Line 138: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input, qkv_weight, qkv_bias,
  Confidence: band=very_high; score=0.9
- Line 157: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // CPU fallback: project input to Q, K, V simultaneously
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input_row = input + i * hidden_dim;
  Confidence: band=very_high; score=0.9
- Line 167: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: query[i * hidden_dim + j] = input_row[j] * qkv_weight[j] + qkv_bias[j];
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: key[i * hidden_dim + j] = input_row[j] * qkv_weight[hidden_dim + j] + qkv_bias[hidden_dim + j];
  Confidence: band=very_high; score=0.9
- Line 169: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: value[i * hidden_dim + j] = input_row[j] * qkv_weight[2 * hidden_dim + j] + qkv_bias[2 * hidden_dim + j];
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input,
  Confidence: band=very_high; score=0.9
- Line 317: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: output, input,
  Confidence: band=very_high; score=0.9
- Line 330: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input_row = input + i * hidden_dim;
  Confidence: band=very_high; score=0.9
- Line 344: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: gate_out[j] += input_row[k] * gate_weight[k * intermediate_dim + j];
  Confidence: band=very_high; score=0.9
- Line 345: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: up_out[j] += input_row[k] * up_weight[k * intermediate_dim + j];
  Confidence: band=very_high; score=0.9
- Line 391: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const float* input_row = input + i * hidden_dim;
  Confidence: band=very_high; score=0.9
- Line 397: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sum_squares += input_row[j] * input_row[j];
  Confidence: band=very_high; score=0.9

### src/llm/gpu_memory_manager.cpp
Total findings: 40

- Line 5: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3802 [LLM] AdaptiveVRAMAllocator... (2026-03-12) | #379 Migrate critical error logg... (2026-03-11) | #240 Replace GPU Memory Manager ... (2026-03-11) | #220 Add multi-GPU LoRA adapter ... (2026-03-11)
  Confidence: band=very_high; score=0.99
- Line 127: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: ptr_
  Context: security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
  Confidence: band=very_high; score=0.99
- Line 142: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: ptr_
  Context: security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
  Confidence: band=very_high; score=0.99
- Line 571: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: cudaError_t err = cudaMalloc(&ptr, bytes);
  Confidence: band=very_high; score=0.99
- Line 573: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: spdlog::error("cudaMalloc failed: {}", cudaGetErrorString(err));
  Confidence: band=very_high; score=0.99
- Line 641: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: cudaError_t err = cudaMallocHost(&ptr, bytes);
  Confidence: band=very_high; score=0.99
- Line 643: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: spdlog::warn("cudaMallocHost failed: {}, falling back to regular malloc",
  Confidence: band=very_high; score=0.99
- Line 1082: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: new_ptr
  Context: new_ptr = std::malloc(total_vram);
  Confidence: band=very_high; score=0.99
- Line 1183: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: cudaError_t err = cudaMallocHost(&new_ptr, total_ram);
  Confidence: band=very_high; score=0.99
- Line 1428: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: cudaError_t err = cudaMalloc(&ptr, bytes);
  Confidence: band=very_high; score=0.99
- Line 1430: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: spdlog::error("cudaMalloc failed on GPU {}: {}", gpu_device_id, cudaGetErrorString(err));
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3802 [LLM] AdaptiveVRAMAllocator... (2026-03-12) | #379 Migrate critical error logg... (2026-03-11) | #240 Replace GPU Memory Manager ... (2026-03-11) | #220 Add multi-GPU LoRA adapter ... (2026-03-11)
  Confidence: band=very_high; score=0.87
- Line 573: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("cudaMalloc failed: {}", cudaGetErrorString(err));
  Confidence: band=very_high; score=0.9
- Line 643: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::warn("cudaMallocHost failed: {}, falling back to regular malloc",
  Confidence: band=very_high; score=0.9
- Line 1061: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaError_t copy_err = cudaMemcpy(static_cast<char*>(new_ptr) + offset,
  Confidence: band=very_high; score=0.9
- Line 1066: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: spdlog::warn("Defrag: cudaMemcpy failed for model {} on GPU {}: {}",
  Confidence: band=very_high; score=0.9
- Line 1074: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: // REL-66: check cudaFree return value in defragment cleanup path
  Confidence: band=very_high; score=0.9
- Line 1077: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: spdlog::warn("Defrag: cudaFree of scratch buffer failed: {}",
  Confidence: band=very_high; score=0.9
- Line 1430: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: spdlog::error("cudaMalloc failed on GPU {}: {}", gpu_device_id, cudaGetErrorString(err));
  Confidence: band=very_high; score=0.9
- Line 1784: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (model_id.find("adapter_") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1846: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (model_id.find("adapter_") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sanitized.push_back(gpu_id);
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: available_gpus_.push_back(gpu_device_id_);
  Confidence: band=high; score=0.74
- Line 982: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gpu_allocs.push_back(alloc);
  Confidence: band=high; score=0.74
- Line 982: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gpu_allocs.push_back(alloc);
  Confidence: band=high; score=0.74
- Line 1025: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, std::vector<MemoryAllocation>> per_device_allocs;
  Confidence: band=medium; score=0.66
- Line 1026: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: per_device_allocs[alloc.gpu_device_id].push_back(alloc);
  Confidence: band=high; score=0.74
- Line 1127: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<void*> ptrs_to_erase;
  Confidence: band=medium; score=0.66
- Line 1138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: model_allocs.push_back(std::move(consolidated));
  Confidence: band=high; score=0.74
- Line 1165: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pinned_allocs.push_back(alloc);
  Confidence: band=high; score=0.74
- Line 1226: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<void*> ptrs_to_erase;
  Confidence: band=medium; score=0.66
- Line 1238: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: model_allocs.push_back(std::move(consolidated));
  Confidence: band=high; score=0.74
- Line 1286: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<void*> ptrs_to_erase;
  Confidence: band=medium; score=0.66
- Line 1298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: model_allocs.push_back(std::move(consolidated));
  Confidence: band=high; score=0.74
- Line 1349: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models.push_back(model_id);
  Confidence: band=high; score=0.74
- Line 1784: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.loaded_adapters.push_back(model_id);
  Confidence: band=high; score=0.74
- Line 1784: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.loaded_adapters.push_back(model_id);
  Confidence: band=high; score=0.74
- Line 1846: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.loaded_adapters.push_back(model_id);
  Confidence: band=high; score=0.74
- Line 1846: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.loaded_adapters.push_back(model_id);
  Confidence: band=high; score=0.74
- Line 2062: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: healthy_gpus.push_back(gpu_id);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/quantized_model.cpp
Total findings: 36

- Line 152: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor QLoRALayer::forward(const Tensor& input) {
  Confidence: band=very_high; score=0.99
- Line 153: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cached_input_ = input.clone();
  Confidence: band=very_high; score=0.99
- Line 158: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Apply LoRA to input
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor lora_output = input.matmul(cached_BA_) * scaling_;
  Confidence: band=very_high; score=0.99
- Line 166: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Compute base output: input @ W_base
  Confidence: band=very_high; score=0.99
- Line 167: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cached_base_output_ = input.matmul(base_W);
  Confidence: band=very_high; score=0.99
- Line 182: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Gradient w.r.t. A: B.T @ (input.T @ scaled_grad)
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor input_T = cached_input_.transpose();
  Confidence: band=very_high; score=0.99
- Line 185: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor grad_A = B_T.matmul(input_T.matmul(scaled_grad));
  Confidence: band=very_high; score=0.99
- Line 187: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Gradient w.r.t. B: input.T @ scaled_grad @ A.T
  Confidence: band=very_high; score=0.99
- Line 190: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor grad_B = input_T.matmul(scaled_grad).matmul(A_T);
  Confidence: band=very_high; score=0.99
- Line 196: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Gradient w.r.t. input
  Confidence: band=very_high; score=0.99
- Line 198: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor grad_input = scaled_grad.matmul(BA_T);
  Confidence: band=very_high; score=0.99
- Line 205: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor grad_input_base = grad_output.matmul(base_W_T);
  Confidence: band=very_high; score=0.99
- Line 206: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: grad_input = grad_input + grad_input_base;
  Confidence: band=very_high; score=0.99
- Line 209: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return grad_input;
  Confidence: band=very_high; score=0.99
- Line 413: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::debug("Loaded pre-quantized tensor: {} (using real quantized weights)",
  Confidence: band=very_high; score=0.99
- Line 152: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor QLoRALayer::forward(const Tensor& input) {
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cached_input_ = input.clone();
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Apply LoRA to input
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor lora_output = input.matmul(cached_BA_) * scaling_;
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Compute base output: input @ W_base
  Confidence: band=very_high; score=0.9
- Line 167: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: cached_base_output_ = input.matmul(base_W);
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Gradient w.r.t. A: B.T @ (input.T @ scaled_grad)
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor input_T = cached_input_.transpose();
  Confidence: band=very_high; score=0.9
- Line 185: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor grad_A = B_T.matmul(input_T.matmul(scaled_grad));
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Gradient w.r.t. B: input.T @ scaled_grad @ A.T
  Confidence: band=very_high; score=0.9
- Line 190: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor grad_B = input_T.matmul(scaled_grad).matmul(A_T);
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Gradient w.r.t. input
  Confidence: band=very_high; score=0.9
- Line 198: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor grad_input = scaled_grad.matmul(BA_T);
  Confidence: band=very_high; score=0.9
- Line 205: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor grad_input_base = grad_output.matmul(base_W_T);
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: grad_input = grad_input + grad_input_base;
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return grad_input;
  Confidence: band=very_high; score=0.9
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(pair.first);
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, Tensor>& model_weights,
  Confidence: band=medium; score=0.66
- Line 399: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shape.push_back(static_cast<size_t>(dim));
  Confidence: band=high; score=0.74

### src/llm/lora_framework/multi_gpu_trainer.cpp
Total findings: 33

- Line 50: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<GPUTensor>& inputs,
  Confidence: band=very_high; score=0.99
- Line 56: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto outputs = layer.forward(inputs);
  Confidence: band=very_high; score=0.99
- Line 143: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<GPUTensor>& inputs,
  Confidence: band=very_high; score=0.99
- Line 147: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto outputs = layer.forward(inputs);
  Confidence: band=very_high; score=0.99
- Line 277: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // In real implementation, would deserialize from file
  Confidence: band=very_high; score=0.99
- Line 50: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<GPUTensor>& inputs,
  Confidence: band=very_high; score=0.9
- Line 50: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<GPUTensor>& inputs,
  Confidence: band=very_high; score=0.9
- Line 56: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: auto outputs = layer.forward(inputs);
  Confidence: band=very_high; score=0.9
- Line 56: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto outputs = layer.forward(inputs);
  Confidence: band=very_high; score=0.9
- Line 60: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<GPUTensor> grad_outputs;
  Confidence: band=very_high; score=0.9
- Line 61: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: grad_outputs.reserve(outputs.size());
  Confidence: band=very_high; score=0.9
- Line 63: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (size_t i = 0; i < outputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: float local_loss = compute_loss(outputs[i], targets[i]);
  Confidence: band=very_high; score=0.9
- Line 71: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: auto output_data = outputs[i].cpu_data();
  Confidence: band=very_high; score=0.9
- Line 81: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: GPUTensor grad(outputs[i].shape(), outputs[i].device());
  Confidence: band=very_high; score=0.9
- Line 83: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: grad_outputs.push_back(std::move(grad));
  Confidence: band=very_high; score=0.9
- Line 86: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: float avg_loss = total_loss / static_cast<float>(outputs.size());
  Confidence: band=very_high; score=0.9
- Line 89: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: layer.backward(grad_outputs);
  Confidence: band=very_high; score=0.9
- Line 143: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<GPUTensor>& inputs,
  Confidence: band=very_high; score=0.9
- Line 143: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<GPUTensor>& inputs,
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: auto outputs = layer.forward(inputs);
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto outputs = layer.forward(inputs);
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (size_t i = 0; i < outputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 152: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: total_loss += compute_loss(outputs[i], targets[i]);
  Confidence: band=very_high; score=0.9
- Line 155: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: return total_loss / static_cast<float>(outputs.size());
  Confidence: band=very_high; score=0.9
- Line 71: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto output_data = outputs[i].cpu_data();
  Confidence: band=high; score=0.74
- Line 72: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto target_data = targets[i].cpu_data();
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grad_outputs.push_back(std::move(grad));
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grad_outputs.push_back(std::move(grad));
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.emplace_back(shard_shape, ctx.get_device(i));
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto shape = tensors[0].shape();
  Confidence: band=high; score=0.74
- Line 420: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto param_data = params[j]->cpu_data();
  Confidence: band=high; score=0.74
- Line 421: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto grad_data = grads[j]->cpu_data();
  Confidence: band=high; score=0.74

### src/llm/fewshot_optimizer.cpp
Total findings: 32

- Line 157: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto example_tokens = tokenize(example.input);
  Confidence: band=very_high; score=0.99
- Line 211: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: while ((pos = formatted.find("{input}", pos)) != std::string::npos) {
  Confidence: band=very_high; score=0.99
- Line 212: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: formatted.replace(pos, 7, examples[i].input);
  Confidence: band=very_high; score=0.99
- Line 213: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pos += examples[i].input.length();
  Confidence: band=very_high; score=0.99
- Line 301: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Combine similarity of inputs and outputs
  Confidence: band=very_high; score=0.99
- Line 313: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto tokens1_in = tokenize(ex1.input);
  Confidence: band=very_high; score=0.99
- Line 314: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto tokens2_in = tokenize(ex2.input);
  Confidence: band=very_high; score=0.99
- Line 318: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Jaccard similarity for inputs
  Confidence: band=very_high; score=0.99
- Line 338: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Average of input and output similarity
  Confidence: band=very_high; score=0.99
- Line 347: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::istringstream iss(cache_[i].input);
  Confidence: band=very_high; score=0.99
- Line 157: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto example_tokens = tokenize(example.input);
  Confidence: band=very_high; score=0.9
- Line 211: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: while ((pos = formatted.find("{input}", pos)) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: formatted.replace(pos, 7, examples[i].input);
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pos += examples[i].input.length();
  Confidence: band=very_high; score=0.9
- Line 301: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Combine similarity of inputs and outputs
  Confidence: band=very_high; score=0.9
- Line 301: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Combine similarity of inputs and outputs
  Confidence: band=very_high; score=0.9
- Line 313: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto tokens1_in = tokenize(ex1.input);
  Confidence: band=very_high; score=0.9
- Line 314: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto tokens2_in = tokenize(ex2.input);
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Jaccard similarity for inputs
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Jaccard similarity for inputs
  Confidence: band=very_high; score=0.9
- Line 328: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Jaccard similarity for outputs
  Confidence: band=very_high; score=0.9
- Line 338: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Average of input and output similarity
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::istringstream iss(cache_[i].input);
  Confidence: band=very_high; score=0.9
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cache_.push_back(ex);
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored_examples.push_back({relevance, i});
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(cache_[scored_examples[i].second]);
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> query_set(query_tokens.begin(), query_tokens.end());
  Confidence: band=medium; score=0.66
- Line 164: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> example_set(example_tokens.begin(), example_tokens.end());
  Confidence: band=medium; score=0.66
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(remaining[0]);
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(remaining[best_idx]);
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tokens;
  Confidence: band=medium; score=0.66
- Line 351: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: query_index_[first_word].push_back(i);
  Confidence: band=high; score=0.74

### src/llm/distributed_training_coordinator.cpp
Total findings: 26

- Line 426: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize loss metrics
  Confidence: band=very_high; score=0.99
- Line 1535: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: available_shard_ids.insert(shard_info.shard_id);
  Confidence: band=very_high; score=0.99
- Line 617: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = shard_weights_.find(grad.source_shard);
  Confidence: band=very_high; score=0.9
- Line 911: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: json response = shard_router_->executeQuery(rpc_query);
  Confidence: band=very_high; score=0.9
- Line 1138: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: json response = shard_router_->executeQuery(rpc_query);
  Confidence: band=very_high; score=0.9
- Line 1160: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: bool success = future.get();
  Confidence: band=very_high; score=0.9
- Line 1213: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: json response = shard_router_->executeQuery(rpc_query);
  Confidence: band=very_high; score=0.9
- Line 1556: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: json response = shard_router_->executeQuery(rpc_query);
  Confidence: band=very_high; score=0.9
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: compressed.push_back((idx >> 24) & 0xFF);
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: data.push_back(val1 * scale + min_val);
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["gradients"].push_back(grad.toJSON());
  Confidence: band=high; score=0.74
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: msg.gradients.push_back(GradientTensor::fromJSON(grad_json));
  Confidence: band=high; score=0.74
- Line 561: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated.push_back(std::move(agg_tensor));
  Confidence: band=high; score=0.74
- Line 561: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated.push_back(std::move(agg_tensor));
  Confidence: band=high; score=0.74
- Line 626: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated.push_back(std::move(agg_tensor));
  Confidence: band=high; score=0.74
- Line 626: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aggregated.push_back(std::move(agg_tensor));
  Confidence: band=high; score=0.74
- Line 873: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_grads.push_back(dummy);
  Confidence: band=high; score=0.74
- Line 918: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_grads.push_back(gradient);
  Confidence: band=high; score=0.74
- Line 1016: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: grad_list.push_back(gradients);
  Confidence: band=high; score=0.74
- Line 1043: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_losses_and_counts.push_back({state.current_loss, state.samples_processed});
  Confidence: band=high; score=0.74
- Line 1122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id]() {
  Confidence: band=high; score=0.74
- Line 1122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(std::async(std::launch::async, [&, shard_id]() {
  Confidence: band=high; score=0.74
- Line 1132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: request["gradients"].push_back(grad.toJSON());
  Confidence: band=high; score=0.74
- Line 1342: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stats_json = checkpoint["stats"];
  Confidence: band=high; score=0.74
- Line 1466: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<GradientTensor>>& shard_gradients,
  Confidence: band=high; score=0.74
- Line 1604: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decompressed.push_back(std::move(grad));
  Confidence: band=high; score=0.74

### src/llm/llm_plugin_manager.cpp
Total findings: 26

- Line 275: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool LLMPluginManager::loadModel(const std::string& model_id, const std::string& path) {
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::error("LLMPluginManager::loadModel: model_id or path is empty");
  Confidence: band=very_high; score=0.99
- Line 317: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::error("LLMPluginManager::loadModel: path '{}' is outside "
  Confidence: band=very_high; score=0.99
- Line 322: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::warn("LLMPluginManager::loadModel: THEMIS_MODEL_ROOT '{}' cannot "
  Confidence: band=very_high; score=0.99
- Line 335: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::warn("LLMPluginManager::loadModel: no default LLM plugin available; model '{}' not loaded",
  Confidence: band=very_high; score=0.99
- Line 339: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: const bool ok = plugin->loadModel(path);
  Confidence: band=very_high; score=0.99
- Line 356: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: void LLMPluginManager::unloadModel(const std::string& model_id) {
  Confidence: band=very_high; score=0.99
- Line 374: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: plugin->unloadModel();
  Confidence: band=very_high; score=0.99
- Line 494: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return loadModel(model_id, model_id);
  Confidence: band=very_high; score=0.99
- Line 663: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (!plugin->loadModel(model_path, config)) {
  Confidence: band=very_high; score=0.99
- Line 712: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: const bool ok = loadModel(model_id, path);
  Confidence: band=very_high; score=0.99
- Line 717: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: {"message", "loadModel() returned false for model_id: " + model_id}}.dump();
  Confidence: band=very_high; score=0.99
- Line 226: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse LLMPluginManager::generate(const InferenceRequest& request) {
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse LLMPluginManager::generateRAG(
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest& request
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = plugin->generateRAG(rag_context, request);
  Confidence: band=very_high; score=0.9
- Line 256: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "LLMPluginManager::generateRAG complete: success={} tokens_generated={} inference_time_ms={:.2f} cache_hit={} error_len={}",
  Confidence: band=very_high; score=0.9
- Line 259: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response.inference_time_ms,
  Confidence: band=very_high; score=0.9
- Line 476: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<std::string> LLMPluginManager::generateStream(const InferenceRequest& request) {
  Confidence: band=very_high; score=0.9
- Line 482: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = plugin->generate(request);
  Confidence: band=very_high; score=0.9
- Line 577: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // underlying inference runtime; we log a notice for callers that rely on this.
  Confidence: band=very_high; score=0.9
- Line 129: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(name);
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: InferenceResponse LLMPluginManager::generate(const InferenceRequest& request) {
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models.push_back(info->name);
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loras.push_back(std::move(lora));
  Confidence: band=high; score=0.74
- Line 482: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto response = plugin->generate(request);
  Confidence: band=high; score=0.74

### src/llm/model_loader.cpp
Total findings: 26

- Line 153: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: CachedModel* LazyModelLoader::getOrLoadModel(
  Confidence: band=very_high; score=0.99
- Line 227: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto result = loadModelInternal(model_id, model_path, load_config);
  Confidence: band=very_high; score=0.99
- Line 235: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool LazyModelLoader::preloadModel(
  Confidence: band=very_high; score=0.99
- Line 270: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto result = loadModelInternal(model_id, model_path, load_config);
  Confidence: band=very_high; score=0.99
- Line 306: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Check if already being loaded via preloadModel
  Confidence: band=very_high; score=0.99
- Line 308: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::warn("Model {} is already being loaded via preloadModel(). "
  Confidence: band=very_high; score=0.99
- Line 310: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: "Consider using preloadModel() for async loading without progress tracking.",
  Confidence: band=very_high; score=0.99
- Line 361: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Note: loadModelInternal is the heavy operation that does the real work
  Confidence: band=very_high; score=0.99
- Line 363: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto result = loadModelInternal(model_id, model_path, load_config);
  Confidence: band=very_high; score=0.99
- Line 378: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: unloadModel(model_id, true);
  Confidence: band=very_high; score=0.99
- Line 417: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool LazyModelLoader::unloadModel(const std::string& model_id, bool force) {
  Confidence: band=very_high; score=0.99
- Line 566: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: unloadModel(id, true);
  Confidence: band=very_high; score=0.99
- Line 625: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: Result<CachedModel*> LazyModelLoader::loadModelInternal(
  Confidence: band=very_high; score=0.99
- Line 721: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto loadModelWithGpuFallback = [&](const char* stage) -> llama_model* {
  Confidence: band=very_high; score=0.99
- Line 743: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::info("{}: trying llama_load_model_from_file with n_gpu_layers={}", stage, layers);
  Confidence: band=very_high; score=0.99
- Line 744: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto* loaded = llama_load_model_from_file(model_path.c_str(), load_params);
  Confidence: band=very_high; score=0.99
- Line 783: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: lmodel = loadModelWithGpuFallback("Custom GGUF validation path");
  Confidence: band=very_high; score=0.99
- Line 801: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::info("Falling back to native llama_load_model_from_file()");
  Confidence: band=very_high; score=0.99
- Line 802: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: lmodel = loadModelWithGpuFallback("Native fallback path");
  Confidence: band=very_high; score=0.99
- Line 878: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Dynamic scaling: adapts to input length
  Confidence: band=very_high; score=0.99
- Line 672: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("CPU-only inference configured (n_gpu_layers={})", requested_gpu_layers);
  Confidence: band=very_high; score=0.9
- Line 693: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility shim for Gemma GGUF variants that omit
  Confidence: band=high; score=0.8
- Line 878: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Dynamic scaling: adapts to input length
  Confidence: band=very_high; score=0.9
- Line 493: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(id);
  Confidence: band=high; score=0.74
- Line 559: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_evict.push_back(id);
  Confidence: band=high; score=0.74
- Line 741: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attempted_gpu_layers.push_back(layers);
  Confidence: band=high; score=0.74

### src/llm/lora_security_validator.cpp
Total findings: 25

- Line 39: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param input Base64 encoded string
  Confidence: band=very_high; score=0.99
- Line 43: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static bool base64_decode(const std::string& input, std::vector<uint8_t>& output) {
  Confidence: band=very_high; score=0.99
- Line 48: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Remove any whitespace from input
  Confidence: band=very_high; score=0.99
- Line 49: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string cleaned_input;
  Confidence: band=very_high; score=0.99
- Line 50: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (char c : input) {
  Confidence: band=very_high; score=0.99
- Line 52: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: cleaned_input += c;
  Confidence: band=very_high; score=0.99
- Line 63: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: bio = BIO_new_mem_buf(cleaned_input.data(), static_cast<int>(cleaned_input.size()));
  Confidence: band=very_high; score=0.99
- Line 74: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t max_decode_len = cleaned_input.size();
  Confidence: band=very_high; score=0.99
- Line 714: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::vector<float> LoRASecurityValidator::loadWeightsFromLoRAFile(
  Confidence: band=very_high; score=0.99
- Line 850: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::info("Loaded {} sampled weights from binary LoRa file", weights.size());
  Confidence: band=very_high; score=0.99
- Line 39: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param input Base64 encoded string
  Confidence: band=very_high; score=0.9
- Line 43: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static bool base64_decode(const std::string& input, std::vector<uint8_t>& output) {
  Confidence: band=very_high; score=0.9
- Line 48: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Remove any whitespace from input
  Confidence: band=very_high; score=0.9
- Line 1049: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (char c : prompt) {
  Confidence: band=very_high; score=0.9
- Line 556: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: anomalies.push_back("Suspiciously high number of zero weights");
  Confidence: band=high; score=0.74
- Line 589: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: config_.trusted_signers.push_back(cert_fingerprint);
  Confidence: band=high; score=0.74
- Line 733: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: weights.push_back(w.get<float>());
  Confidence: band=high; score=0.74
- Line 780: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto dtype = tensor_info["dtype"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 781: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto offsets = tensor_info["data_offsets"].get<std::vector<uint64_t>>();
  Confidence: band=high; score=0.74
- Line 841: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: weights.push_back(value);
  Confidence: band=high; score=0.74
- Line 887: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outliers.push_back(i);
  Confidence: band=high; score=0.74
- Line 887: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outliers.push_back(i);
  Confidence: band=high; score=0.74
- Line 924: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: PromptInjectionDetector::initializePatterns()
  Context: void PromptInjectionDetector::initializePatterns() {
  Confidence: band=medium; score=0.56
- Line 1007: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pos += 10;  // Length of "[REDACTED]"
  Confidence: band=high; score=0.74
- Line 1007: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: pos += 10;  // Length of "[REDACTED]"
  Confidence: band=high; score=0.74

### src/llm/vision_resource_monitor.cpp
Total findings: 25

- Line 197: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: quota.inference_minutes_used += std::chrono::duration_cast<std::chrono::minutes>(inference_time).count();
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: .inference_minutes_remaining = quota_config_.total_inference_minutes,
  Confidence: band=very_high; score=0.9
- Line 224: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: .inference_minutes_remaining = quota_config_.total_inference_minutes > quota.inference_minutes_used ?
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: quota_config_.total_inference_minutes - quota.inference_minutes_used : 0,
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: usage_.min_inference_time_ms = std::numeric_limits<double>::max();
  Confidence: band=very_high; score=0.9
- Line 388: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::chrono::milliseconds inference_time,
  Confidence: band=very_high; score=0.9
- Line 417: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const double time_ms = std::chrono::duration<double, std::milli>(inference_time).count();
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: usage_.avg_inference_time_ms = time_ms;
  Confidence: band=very_high; score=0.9
- Line 420: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: usage_.min_inference_time_ms = time_ms;
  Confidence: band=very_high; score=0.9
- Line 421: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: usage_.max_inference_time_ms = time_ms;
  Confidence: band=very_high; score=0.9
- Line 425: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: usage_.avg_inference_time_ms =
  Confidence: band=very_high; score=0.9
- Line 426: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ((usage_.avg_inference_time_ms * (successful_requests - 1.0)) + time_ms) /
  Confidence: band=very_high; score=0.9
- Line 428: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: usage_.min_inference_time_ms = std::min(usage_.min_inference_time_ms, time_ms);
  Confidence: band=very_high; score=0.9
- Line 429: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: usage_.max_inference_time_ms = std::max(usage_.max_inference_time_ms, time_ms);
  Confidence: band=very_high; score=0.9
- Line 435: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: quota_tracker_->consumeQuota(info.user_id, 1, inference_time, memory_used_mb);
  Confidence: band=very_high; score=0.9
- Line 440: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Time: " + std::to_string(inference_time.count()) + "ms", success);
  Confidence: band=very_high; score=0.9
- Line 552: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: .inference_minutes_remaining = 0,
  Confidence: band=very_high; score=0.9
- Line 587: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ss << "# HELP themisdb_vision_inference_duration_seconds Inference duration\n";
  Confidence: band=very_high; score=0.9
- Line 588: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ss << "# TYPE themisdb_vision_inference_duration_seconds summary\n";
  Confidence: band=very_high; score=0.9
- Line 589: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ss << "themisdb_vision_inference_duration_seconds{quantile=\"0.5\"} " << (usage.avg_inference_time_ms / 1000.0) << "\n";
  Confidence: band=very_high; score=0.9
- Line 590: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ss << "themisdb_vision_inference_duration_seconds{quantile=\"0.9\"} " << (usage.max_inference_time_ms / 1000.0) << "\n";
  Confidence: band=very_high; score=0.9
- Line 591: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ss << "themisdb_vision_inference_duration_seconds_sum "
  Confidence: band=very_high; score=0.9
- Line 592: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: << ((usage.avg_inference_time_ms * static_cast<double>(usage.successful_requests)) / 1000.0)
  Confidence: band=very_high; score=0.9
- Line 594: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ss << "themisdb_vision_inference_duration_seconds_count " << usage.successful_requests << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 103: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RateLimiter::reset()
  Context: void RateLimiter::reset() {
  Confidence: band=medium; score=0.56

### src/llm/lora_framework/lora_audit_logger.cpp
Total findings: 24

- Line 49: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void logInference(const LoRAInferenceAudit& audit) {
  Confidence: band=very_high; score=0.9
- Line 56: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: log_entry["event_type"] = "INFERENCE";
  Confidence: band=very_high; score=0.9
- Line 57: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: log_entry["log_type"] = "lora_inference";
  Confidence: band=very_high; score=0.9
- Line 69: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceAuditEntry e;
  Confidence: band=very_high; score=0.9
- Line 92: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_count_++;
  Confidence: band=very_high; score=0.9
- Line 94: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("Logged inference: model={}, adapter={}, request={}",
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("Failed to log inference: {}", e.what());
  Confidence: band=very_high; score=0.9
- Line 249: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<LoRAInferenceAudit> getInferenceHistory(
  Confidence: band=very_high; score=0.9
- Line 253: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<LoRAInferenceAudit> results;
  Confidence: band=very_high; score=0.9
- Line 257: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (log.contains("log_type") && log["log_type"] == "lora_inference") {
  Confidence: band=very_high; score=0.9
- Line 258: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: LoRAInferenceAudit audit;
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: int inferences = 0;
  Confidence: band=very_high; score=0.9
- Line 296: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (log_type == "lora_inference") {
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferences++;
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: stats["inferences"] = inferences;
  Confidence: band=very_high; score=0.9
- Line 348: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: uint64_t inference_count_ = 0;
  Confidence: band=very_high; score=0.9
- Line 367: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: case LoRAAuditEventType::INFERENCE_STARTED: return "INFERENCE_STARTED";
  Confidence: band=very_high; score=0.9
- Line 368: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: case LoRAAuditEventType::INFERENCE_COMPLETED: return "INFERENCE_COMPLETED";
  Confidence: band=very_high; score=0.9
- Line 369: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: case LoRAAuditEventType::INFERENCE_FAILED: return "INFERENCE_FAILED";
  Confidence: band=very_high; score=0.9
- Line 416: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LoRAAuditLogger::logInference(const LoRAInferenceAudit& audit) {
  Confidence: band=very_high; score=0.9
- Line 417: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->logInference(audit);
  Confidence: band=very_high; score=0.9
- Line 478: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<LoRAInferenceAudit> LoRAAuditLogger::getInferenceHistory(
  Confidence: band=very_high; score=0.9
- Line 482: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return impl_->getInferenceHistory(adapter_id, limit);
  Confidence: band=very_high; score=0.9
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(audit);
  Confidence: band=high; score=0.74

### src/llm/docs_assistant.cpp
Total findings: 23

- Line 512: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest rag_request;
  Confidence: band=very_high; score=0.9
- Line 533: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto rag_response = LLMPluginManager::instance().generateRAG(rag_context, rag_request);
  Confidence: band=very_high; score=0.9
- Line 536: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "DocsAssistant::generateAnswer plugin-rag complete: success=1 answer_chars={} tokens_generated={} inference_time_ms={:.2f}",
  Confidence: band=very_high; score=0.9
- Line 539: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: rag_response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 545: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "DocsAssistant::generateAnswer plugin-rag failed: error_len={} tokens_generated={} inference_time_ms={:.2f}",
  Confidence: band=very_high; score=0.9
- Line 548: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: rag_response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 595: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 604: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = LLMPluginManager::instance().generate(req);
  Confidence: band=very_high; score=0.9
- Line 619: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: DocsQueryResult DocsAssistant::query(const std::string& query) {
  Confidence: band=very_high; score=0.9
- Line 657: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.generated_answer = generateAnswer(query, result.relevant_docs);
  Confidence: band=very_high; score=0.9
- Line 44: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, DocsQueryResult> cache;
  Confidence: band=medium; score=0.66
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cur.push_back(static_cast<char>(std::tolower(ch)));
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::vector<float> hashEmbedQuery(const std::string& text, int dim) {
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> freqs;
  Confidence: band=medium; score=0.66
- Line 236: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> doc_id_to_path;
  Confidence: band=medium; score=0.66
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc.embedding.push_back(x.get<float>());
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc.embedding_q.push_back(static_cast<int16_t>(x.get<int>()));
  Confidence: band=high; score=0.74
- Line 438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored_docs.push_back(doc);
  Confidence: band=high; score=0.74
- Line 508: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rag_context.documents.push_back(std::move(rag_doc));
  Confidence: band=high; score=0.74
- Line 560: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return THEMIS_LLM_GENERATE(safe_prompt);
  Confidence: band=high; score=0.74
- Line 587: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return THEMIS_LLM_GENERATE(safe_prompt);
  Confidence: band=high; score=0.74
- Line 604: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto response = LLMPluginManager::instance().generate(req);
  Confidence: band=high; score=0.74
- Line 130: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: const float weight = 1.0f + std::log(static_cast<float>(std::max(1, tf)));
  Confidence: band=medium; score=0.6

### src/llm/federated_inference_coordinator.cpp
Total findings: 22

- Line 2: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * ThemisDB | File: federated_inference_coordinator.cpp | Version: 1.0.0 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=very_high; score=0.9
- Line 10: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/federated_inference_coordinator.h"
  Confidence: band=very_high; score=0.9
- Line 23: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: FederatedInferenceCoordinator::FederatedInferenceCoordinator(
  Confidence: band=very_high; score=0.9
- Line 32: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void FederatedInferenceCoordinator::addStaticShard(const std::string&         instance_id,
  Confidence: band=very_high; score=0.9
- Line 39: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // IFederatedInferenceBackend::execute()
  Confidence: band=very_high; score=0.9
- Line 42: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<FanOutInstanceResult> FederatedInferenceCoordinator::execute(
  Confidence: band=very_high; score=0.9
- Line 44: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest&         request) {
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("FederatedInferenceCoordinator: fan-out complete — {}/{} instances succeeded",
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: FanOutInstanceResult FederatedInferenceCoordinator::dispatchToInstance(
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest&    request) {
  Confidence: band=very_high; score=0.9
- Line 185: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::warn("FederatedInferenceCoordinator: {}", result.error);
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("FederatedInferenceCoordinator: instance '{}' attempt {} failed "
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: FederatedInferenceCoordinator::resolveShard(const std::string& instance_id) const {
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: nlohmann::json FederatedInferenceCoordinator::buildRequestBody(const InferenceRequest& req) {
  Confidence: band=very_high; score=0.9
- Line 234: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse FederatedInferenceCoordinator::parseResponse(const nlohmann::json& data,
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse resp;
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: resp.tokens_generated = data["tokens_generated"].get<int>();
  Confidence: band=very_high; score=0.9
- Line 250: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (data.contains("inference_time_ms") && data["inference_time_ms"].is_number()) {
  Confidence: band=very_high; score=0.9
- Line 251: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: resp.inference_time_ms = data["inference_time_ms"].get<float>();
  Confidence: band=very_high; score=0.9
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(r));
  Confidence: band=high; score=0.74
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(fut.get());
  Confidence: band=high; score=0.74

### src/llm/lora_framework/base_model_adapter.cpp
Total findings: 22

- Line 31: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool BaseModelAdapter::loadModel(const std::string& model_path) {
  Confidence: band=very_high; score=0.99
- Line 573: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (!base_model_->loadModel(config_.base_model_path)) {
  Confidence: band=very_high; score=0.99
- Line 638: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor LoRAEnhancedModel::forward(const Tensor& input, int /*layer_idx*/) {
  Confidence: band=very_high; score=0.99
- Line 653: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return lora_layer->forward(input);
  Confidence: band=very_high; score=0.99
- Line 656: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Fallback: return input (identity)
  Confidence: band=very_high; score=0.99
- Line 657: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return input.clone();
  Confidence: band=very_high; score=0.99
- Line 89: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Try to infer from tensor names
  Confidence: band=very_high; score=0.9
- Line 373: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // For LoRA training, we need raw token embeddings as inputs to layers,
  Confidence: band=very_high; score=0.9
- Line 389: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (tensor.name == name || tensor.name.find(name) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 545: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // This helps with cross-model compatibility
  Confidence: band=high; score=0.8
- Line 638: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor LoRAEnhancedModel::forward(const Tensor& input, int /*layer_idx*/) {
  Confidence: band=very_high; score=0.9
- Line 653: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return lora_layer->forward(input);
  Confidence: band=very_high; score=0.9
- Line 656: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Fallback: return input (identity)
  Confidence: band=very_high; score=0.9
- Line 657: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return input.clone();
  Confidence: band=very_high; score=0.9
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adaptable_layers_.push_back(layer_info);
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: layer_info.shape.push_back(static_cast<size_t>(dim));
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matching_layers.push_back(layer);
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matching_layers.push_back(layer);
  Confidence: band=high; score=0.74
- Line 583: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) modules_str += ", ";
  Confidence: band=high; score=0.74
- Line 698: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::pair<Tensor, Tensor>>
  Confidence: band=medium; score=0.66
- Line 700: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::pair<Tensor, Tensor>> weights;
  Confidence: band=medium; score=0.66
- Line 710: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::pair<Tensor, Tensor>>& weights) {
  Confidence: band=medium; score=0.66

### src/llm/lora_framework/gpu_data_loader.cpp
Total findings: 22

- Line 278: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> input_ids_data;
  Confidence: band=very_high; score=0.99
- Line 283: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_ids_data.reserve(total_size);
  Confidence: band=very_high; score=0.99
- Line 293: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_ids_data.push_back(static_cast<float>(tokens[j]));
  Confidence: band=very_high; score=0.99
- Line 295: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Labels are shifted input for causal LM
  Confidence: band=very_high; score=0.99
- Line 304: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_ids_data.push_back(static_cast<float>(tokenizer_->pad_token_id()));
  Confidence: band=very_high; score=0.99
- Line 312: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch.input_ids = GPUTensor({actual_batch_size, config_.max_sequence_length}, config_.target_device);
  Confidence: band=very_high; score=0.99
- Line 316: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: batch.input_ids.upload(input_ids_data);
  Confidence: band=very_high; score=0.99
- Line 327: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!sample.input.empty()) {
  Confidence: band=very_high; score=0.99
- Line 328: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: formatted += "\n\n### Input:\n" + sample.input;
  Confidence: band=very_high; score=0.99
- Line 278: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> input_ids_data;
  Confidence: band=very_high; score=0.9
- Line 283: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_ids_data.reserve(total_size);
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_ids_data.push_back(static_cast<float>(tokens[j]));
  Confidence: band=very_high; score=0.9
- Line 295: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Labels are shifted input for causal LM
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_ids_data.push_back(static_cast<float>(tokenizer_->pad_token_id()));
  Confidence: band=very_high; score=0.9
- Line 312: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch.input_ids = GPUTensor({actual_batch_size, config_.max_sequence_length}, config_.target_device);
  Confidence: band=very_high; score=0.9
- Line 316: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: batch.input_ids.upload(input_ids_data);
  Confidence: band=very_high; score=0.9
- Line 327: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!sample.input.empty()) {
  Confidence: band=very_high; score=0.9
- Line 328: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: formatted += "\n\n### Input:\n" + sample.input;
  Confidence: band=very_high; score=0.9
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: input_ids_data.push_back(static_cast<float>(tokens[j]));
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: input_ids_data.push_back(static_cast<float>(tokens[j]));
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: input_ids_data.push_back(static_cast<float>(tokenizer_->pad_token_id()));
  Confidence: band=high; score=0.74
- Line 327: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: formatted += "\n\n### Input:\n" + sample.input;
  Confidence: band=high; score=0.74

### src/llm/ethical_guidelines_manager.cpp
Total findings: 21

- Line 468: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (text_lower.find(keyword_lower) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 585: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 590: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse resp;
  Confidence: band=very_high; score=0.9
- Line 594: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: LogWarning("LLM judge inference failed: {}", ex.what());
  Confidence: band=very_high; score=0.9
- Line 63: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto cfg = config["config"];
  Confidence: band=high; score=0.74
- Line 86: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: principles_.push_back(principle);
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto cd = config["context_detection"];
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ethical_keywords_de_.push_back(kw.as<std::string>());
  Confidence: band=high; score=0.74
- Line 104: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ethical_keywords_en_.push_back(kw.as<std::string>());
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: high_autonomy_contexts_.push_back(ctx.as<std::string>());
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto pa = config["prompt_augmentation"];
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: guideline.applies_to.push_back(keyword.as<std::string>());
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: guideline.applies_to.push_back(keyword.as<std::string>());
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.detected_keywords.push_back(keyword);
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.detected_keywords.push_back(keyword);
  Confidence: band=high; score=0.74
- Line 468: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected_domains.push_back(domain_name);
  Confidence: band=high; score=0.74
- Line 468: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected_domains.push_back(domain_name);
  Confidence: band=high; score=0.74
- Line 468: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected_domains.push_back(domain_name);
  Confidence: band=high; score=0.74
- Line 569: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: judge_prompt += R"(
  Confidence: band=high; score=0.74
- Line 592: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: resp = llm->generate(req);
  Confidence: band=high; score=0.74
- Line 691: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: schools.push_back(school_id);
  Confidence: band=high; score=0.74

### src/llm/mixed_precision_inference.cpp
Total findings: 20

- Line 2: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * ThemisDB | File: mixed_precision_inference.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=very_high; score=0.9
- Line 10: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/mixed_precision_inference.h"
  Confidence: band=very_high; score=0.9
- Line 21: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: class MixedPrecisionInference::Impl {
  Confidence: band=very_high; score=0.9
- Line 27: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: MixedPrecisionInference::MixedPrecisionInference()
  Confidence: band=very_high; score=0.9
- Line 30: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: MixedPrecisionInference::~MixedPrecisionInference() = default;
  Confidence: band=very_high; score=0.9
- Line 32: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: PrecisionMode MixedPrecisionInference::selectOptimalPrecision(
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<MixedPrecisionInference::LayerPrecisionConfig>
  Confidence: band=very_high; score=0.9
- Line 59: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: MixedPrecisionInference::getTuningSchedule(
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: size_t MixedPrecisionInference::calculateModelSize(
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: MixedPrecisionInference::PrecisionInfo
  Confidence: band=very_high; score=0.9
- Line 125: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: MixedPrecisionInference::getPrecisionInfo(PrecisionMode precision) {
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<MixedPrecisionInference::PrecisionInfo>
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: MixedPrecisionInference::getAllPrecisions() {
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: float MixedPrecisionInference::calculateExpectedAccuracy(PrecisionMode precision) {
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: float MixedPrecisionInference::calculateMemoryReduction(PrecisionMode precision) {
  Confidence: band=very_high; score=0.9
- Line 203: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: PrecisionMode MixedPrecisionInference::fromString(const std::string& str) {
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string MixedPrecisionInference::toString(PrecisionMode precision) {
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool MixedPrecisionInference::isSupported(PrecisionMode precision) {
  Confidence: band=very_high; score=0.9
- Line 268: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: case PrecisionMode::INT8:     return true;   // CPU INT8 inference (ONNX Runtime etc.)
  Confidence: band=very_high; score=0.9
- Line 269: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: case PrecisionMode::Q4:       return true;   // CPU Q4 inference (llama.cpp style)
  Confidence: band=very_high; score=0.9

### src/llm/multi_perspective_generator.cpp
Total findings: 20

- Line 45: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current.push_back(static_cast<char>(std::tolower(uc)));
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.perspectives.push_back(resp);
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(*it);
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(*it);
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(*it);
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(p);
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(p);
  Confidence: band=high; score=0.74
- Line 587: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> words_i = extractWords(perspectives[i].response);
  Confidence: band=medium; score=0.66
- Line 588: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> words_j = extractWords(perspectives[j].response);
  Confidence: band=medium; score=0.66
- Line 591: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection;
  Confidence: band=medium; score=0.66
- Line 592: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> union_set;
  Confidence: band=medium; score=0.66
- Line 633: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> theme_counts;
  Confidence: band=medium; score=0.66
- Line 648: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: common_themes.push_back(theme);
  Confidence: band=high; score=0.74
- Line 648: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: common_themes.push_back(theme);
  Confidence: band=high; score=0.74
- Line 648: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: common_themes.push_back(theme);
  Confidence: band=high; score=0.74
- Line 660: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: common_themes.push_back(theme);
  Confidence: band=high; score=0.74
- Line 686: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> traditions;
  Confidence: band=medium; score=0.66
- Line 692: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: disagreements.push_back("Consequentialist outcomes versus adherence to universal principles");
  Confidence: band=high; score=0.74
- Line 802: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool MultiPerspectiveGenerator::detectEthicalQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 849: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: key_points.push_back(trimmed);
  Confidence: band=high; score=0.74

### src/llm/applications/themis_help_lora.cpp
Total findings: 19

- Line 5: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #371 Implement ThemisHelpLoRA: D... (2026-03-11) | #370 Integrate themis_help_lora ... (2026-03-11) | #376 Implement Real LLM Integrat... (2026-03-11)
  Confidence: band=very_high; score=0.99
- Line 187: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool loaded = llama_wrapper->loadModel(model_path);
  Confidence: band=very_high; score=0.99
- Line 388: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input = item.question;
  Confidence: band=very_high; score=0.99
- Line 434: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Reload with new weights
  Confidence: band=very_high; score=0.99
- Line 517: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Reload with new weights
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #371 Implement ThemisHelpLoRA: D... (2026-03-11) | #370 Integrate themis_help_lora ... (2026-03-11) | #376 Implement Real LLM Integrat... (2026-03-11)
  Confidence: band=very_high; score=0.87
- Line 96: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Initialize LlamaWrapper for LLM inference
  Confidence: band=very_high; score=0.9
- Line 214: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Create inference request
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto llm_response = llama_wrapper->generate(request);
  Confidence: band=very_high; score=0.9
- Line 231: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("LLM inference completed: {} tokens in {:.2f}ms",
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm_response.tokens_generated, llm_response.inference_time_ms);
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: response = generatePlaceholderResponse(question);
  Confidence: band=very_high; score=0.9
- Line 316: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: std::string ThemisHelpLoRA::query(const std::string& question, const std::string& user_id) {
  Confidence: band=very_high; score=0.9
- Line 388: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input = item.question;
  Confidence: band=very_high; score=0.9
- Line 14: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_training_service.h"
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto llm_response = llama_wrapper->generate(request);
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string ThemisHelpLoRA::query(const std::string& question, const std::string& user_id) {
  Confidence: band=high; score=0.74
- Line 391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: training_data.samples.push_back(std::move(sample));
  Confidence: band=high; score=0.74

### src/llm/llm_model_storage.cpp
Total findings: 19

- Line 5: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #4308 fix(llm): merge develop, re... (2026-03-19) | #4304 [LLM-DEP-123] Implement Roc... (2026-03-17) | #543 Implement LLM Model and LoR... (2026-03-11) | #677 Implement native model load... (2026-03-11)
  Confidence: band=very_high; score=0.99
- Line 205: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<LLMModelMetadata> loadModel(const std::string& model_id) {
  Confidence: band=very_high; score=0.99
- Line 350: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<std::vector<uint8_t>> loadModelBlob(const std::string& model_id) {
  Confidence: band=very_high; score=0.99
- Line 381: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize BaseEntity
  Confidence: band=very_high; score=0.99
- Line 382: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity entity = BaseEntity::deserialize(model_id, decrypted_data);
  Confidence: band=very_high; score=0.99
- Line 480: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto metadata_opt = loadModel(model_id);
  Confidence: band=very_high; score=0.99
- Line 578: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto metadata_opt = loadModel(model_id);
  Confidence: band=very_high; score=0.99
- Line 677: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<LLMModelMetadata> LLMModelStorage::loadModel(const std::string& model_id) {
  Confidence: band=very_high; score=0.99
- Line 678: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return impl_->loadModel(model_id);
  Confidence: band=very_high; score=0.99
- Line 681: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<std::vector<uint8_t>> LLMModelStorage::loadModelBlob(const std::string& model_id) {
  Confidence: band=very_high; score=0.99
- Line 682: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return impl_->loadModelBlob(model_id);
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #4308 fix(llm): merge develop, re... (2026-03-19) | #4304 [LLM-DEP-123] Implement Roc... (2026-03-17) | #543 Implement LLM Model and LoR... (2026-03-11) | #677 Implement native model load... (2026-03-11)
  Confidence: band=very_high; score=0.87
- Line 121: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: entity.setField("total_inferences", Value(metadata.total_inferences));
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (entity.hasField("total_inferences")) {
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metadata.total_inferences = entity.getFieldAsInt("total_inferences").value_or(0);
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metadata.total_tokens_generated = entity.getFieldAsInt("total_tokens_generated").value_or(0);
  Confidence: band=very_high; score=0.9
- Line 584: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metadata.total_inferences++;
  Confidence: band=very_high; score=0.9
- Line 585: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metadata.total_tokens_generated += tokens_generated;
  Confidence: band=very_high; score=0.9
- Line 915: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similar_models.emplace_back(other_model_id, similarity);
  Confidence: band=high; score=0.74

### src/llm/byzantine_detector.cpp
Total findings: 18

- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deviations.push_back(std::abs(val - median));
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.gradient_norms.push_back(norm);
  Confidence: band=high; score=0.74
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.gradient_means.push_back(mean);
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.gradient_variances.push_back(variance);
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.gradient_variances.push_back(variance);
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.suspected_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_ids.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back({score, shard_ids[i]});
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected.push_back(scores[i].second);
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.suspected_shards.push_back(shard_id);
  Confidence: band=high; score=0.74
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(shard_grads[layer_idx].data[coord]);
  Confidence: band=high; score=0.74
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(shard_grads[layer_idx].data[coord]);
  Confidence: band=high; score=0.74
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(shard_grads[layer_idx].data[coord]);
  Confidence: band=high; score=0.74
- Line 492: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
  Confidence: band=high; score=0.74
- Line 501: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected_gradients.push_back(shard_gradients.at(shard_id));
  Confidence: band=high; score=0.74
- Line 526: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, float> all_scores = median_result.anomaly_scores;
  Confidence: band=high; score=0.74

### src/llm/llm_model_audit_logger.cpp
Total findings: 17

- Line 30: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: case LLMModelAuditEventType::INFERENCE_STARTED:    return "INFERENCE_STARTED";
  Confidence: band=very_high; score=0.9
- Line 31: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: case LLMModelAuditEventType::INFERENCE_COMPLETED:  return "INFERENCE_COMPLETED";
  Confidence: band=very_high; score=0.9
- Line 32: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: case LLMModelAuditEventType::INFERENCE_FAILED:     return "INFERENCE_FAILED";
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Core write helper (shared by logInference / logEvent)
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMModelAuditLogger::logInference(const LLMModelInferenceAudit& audit) {
  Confidence: band=very_high; score=0.9
- Line 197: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("LLM inference audit model={} request={}",
  Confidence: band=very_high; score=0.9
- Line 202: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: ? LLMModelAuditEventType::INFERENCE_COMPLETED
  Confidence: band=very_high; score=0.9
- Line 203: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: : LLMModelAuditEventType::INFERENCE_FAILED,
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<LLMModelInferenceAudit> LLMModelAuditLogger::getInferenceHistory(
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("LLM getInferenceHistory model={} limit={}", model_id, limit);
  Confidence: band=very_high; score=0.9
- Line 313: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // struct) to avoid duplicating typed struct state.  For inference history
  Confidence: band=very_high; score=0.9
- Line 316: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // LLMModelInferenceAudit when event_type == INFERENCE_COMPLETED/FAILED.
  Confidence: band=very_high; score=0.9
- Line 374: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: size_t total = 0, inferences = 0, failures = 0, policy_blocks = 0;
  Confidence: band=very_high; score=0.9
- Line 378: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (r.event_type == LLMModelAuditEventType::INFERENCE_COMPLETED) ++inferences;
  Confidence: band=very_high; score=0.9
- Line 379: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (r.event_type == LLMModelAuditEventType::INFERENCE_FAILED)    ++failures;
  Confidence: band=very_high; score=0.9
- Line 385: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"inferences",    inferences},
  Confidence: band=very_high; score=0.9
- Line 301: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(entry));
  Confidence: band=high; score=0.74

### src/llm/embedded_llm.cpp
Total findings: 16

- Line 51: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (!wrapper_->loadModel(config.model_path)) {
  Confidence: band=very_high; score=0.99
- Line 61: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: wrapper_->unloadModel();
  Confidence: band=very_high; score=0.99
- Line 83: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest request = createRequest(final_prompt, max_tokens);
  Confidence: band=very_high; score=0.9
- Line 84: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = generateFull(request);
  Confidence: band=very_high; score=0.9
- Line 104: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest request = createRequest(final_prompt, max_tokens, temperature, top_p);
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = generateFull(request);
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest request = createRequest(prompt, max_tokens);
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = generateFull(request);
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest request = createRequest(prompt, max_tokens);
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = generateFull(request);
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest request = createRequest(prompt, max_tokens);
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = generateFull(request);
  Confidence: band=very_high; score=0.9
- Line 296: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 128: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return generate(formatted_prompt);
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: embeddings.push_back(embed(text));  // reuse cached embed()
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return wrapper_->generate(request);
  Confidence: band=high; score=0.74

### src/llm/safety/classifier.cpp
Total findings: 16

- Line 40: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: SafetyClassifier::SafetyClassifier(InferenceFn inference_fn)
  Confidence: band=very_high; score=0.9
- Line 41: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: : inference_fn_(std::move(inference_fn)) {}
  Confidence: band=very_high; score=0.9
- Line 43: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void SafetyClassifier::setInferenceFn(InferenceFn inference_fn) {
  Confidence: band=very_high; score=0.9
- Line 44: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = std::move(inference_fn);
  Confidence: band=very_high; score=0.9
- Line 47: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: bool SafetyClassifier::hasInferenceFn() const {
  Confidence: band=very_high; score=0.9
- Line 48: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return static_cast<bool>(inference_fn_);
  Confidence: band=very_high; score=0.9
- Line 52: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (inference_fn_) {
  Confidence: band=very_high; score=0.9
- Line 53: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (auto inferred = inference_fn_(text); inferred.has_value()) {
  Confidence: band=very_high; score=0.9
- Line 54: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (inferred->confidence < 0.0) {
  Confidence: band=very_high; score=0.9
- Line 55: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred->confidence = 0.0;
  Confidence: band=very_high; score=0.9
- Line 57: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (inferred->confidence > 1.0) {
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred->confidence = 1.0;
  Confidence: band=very_high; score=0.9
- Line 60: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (inferred->source.empty()) {
  Confidence: band=very_high; score=0.9
- Line 61: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inferred->source = "inference";
  Confidence: band=very_high; score=0.9
- Line 63: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return *inferred;
  Confidence: band=very_high; score=0.9
- Line 89: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.emplace_back(std::async(std::launch::async, [this, &texts, pos]() {
  Confidence: band=high; score=0.74

### src/llm/moral_analyzer.cpp
Total findings: 15

- Line 542: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // - Causal inference models
  Confidence: band=very_high; score=0.9
- Line 1278: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceEngineEnhanced::EnhancedInferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 329: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: action_evaluations.push_back({action, path});
  Confidence: band=high; score=0.74
- Line 377: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_paths.push_back({philosophy, decision.reasoning_path});
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.supporting_principles.push_back(principle);
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.supporting_principles.push_back(virtue);
  Confidence: band=high; score=0.74
- Line 836: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: audit.alternatives.push_back(phil + ": " + perspective);
  Confidence: band=high; score=0.74
- Line 886: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static const std::unordered_set<std::string> stopwords = {
  Confidence: band=medium; score=0.66
- Line 916: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keywords.push_back(word);
  Confidence: band=high; score=0.74
- Line 916: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keywords.push_back(word);
  Confidence: band=high; score=0.74
- Line 1053: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> MoralAnalyzer::calculateStakeholderImpacts(
  Confidence: band=high; score=0.74
- Line 1057: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> impacts;
  Confidence: band=high; score=0.74
- Line 1223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: recommendations.push_back(rec);
  Confidence: band=high; score=0.74
- Line 1241: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unique_recs.push_back(rec);
  Confidence: band=high; score=0.74
- Line 1310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: philosophies.push_back(phil_str);
  Confidence: band=high; score=0.74

### src/llm/multi_gpu_memory_coordinator.cpp
Total findings: 15

- Line 268: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: MultiGPUMemoryCoordinator::balanceInferenceLoad(
  Confidence: band=very_high; score=0.9
- Line 557: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: // Use cudaMemcpyPeer for direct GPU-to-GPU transfer
  Confidence: band=very_high; score=0.9
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->gpus_.push_back(device);
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.shard_sizes.push_back(shard_size);
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.p2p_pairs.emplace_back(gpu_ids[i], gpu_ids[j]);
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.p2p_pairs.emplace_back(gpu_ids[i], gpu_ids[j]);
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gpu_layers.push_back(static_cast<int>(current_layer++));
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gpu_layers.push_back(static_cast<int>(current_layer++));
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.p2p_pairs.emplace_back(gpu_ids[i], gpu_ids[i + 1]);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: utilizations.push_back(gpu.utilization_percent);
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: inverse_util.push_back(inv);
  Confidence: band=high; score=0.74
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: plan.batch_assignments.push_back(static_cast<int>(batch_for_gpu));
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: spdlog::info("MultiGPUMemoryCoordinator: P2P setup complete ({} success, {} failed)",
  Confidence: band=high; score=0.74
- Line 464: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: spdlog::info("MultiGPUMemoryCoordinator: P2P setup complete ({} success, {} failed)",
  Confidence: band=high; score=0.74
- Line 634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: status.emplace_back(gpu.device_id, gpu.is_healthy);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/lora_training_config.cpp
Total findings: 14

- Line 51: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto defaults = root["training_defaults"];
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: configs.push_back(config);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back("Adapter " + id + ": base_model_name is empty");
  Confidence: band=high; score=0.74
- Line 189: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto base = node["base_model"];
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto feedback = node["training_data"]["feedback"];
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto exec = node["pipeline"]["execution"];
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: params.target_modules.push_back(module.as<std::string>());
  Confidence: band=high; score=0.74
- Line 310: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto automatic = node["automatic"];
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto batch = automatic["batch_size"];
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto time = automatic["time"];
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto quality = automatic["quality"];
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto ab = node["ab_testing"];
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto rollback = node["auto_rollback"];
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: quality.rollback_triggers.push_back(trigger.as<std::string>());
  Confidence: band=high; score=0.74

### src/llm/active_vram_allocator.cpp
Total findings: 13

- Line 15: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: * - Real GPU memory allocation via GPUMemoryManager (cudaMalloc / fallback)
  Confidence: band=very_high; score=0.99
- Line 15: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: * - Real GPU memory allocation via GPUMemoryManager (cudaMalloc / fallback)
  Confidence: band=very_high; score=0.9
- Line 65: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: * Uses `cudaMemcpy(DeviceToHost)` / `cudaMemcpy(HostToDevice)` when CUDA is
  Confidence: band=very_high; score=0.9
- Line 85: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyKind kind = device_to_host
  Confidence: band=very_high; score=0.9
- Line 86: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: ? cudaMemcpyDeviceToHost
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: : cudaMemcpyHostToDevice;
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: spdlog::warn("[ActiveVRAMAllocator] cudaMemcpy failed: {}",
  Confidence: band=very_high; score=0.9
- Line 386: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: // Copy data from CPU back to GPU (uses cudaMemcpy when CUDA is available)
  Confidence: band=very_high; score=0.9
- Line 583: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // External allocations are owned by the inference runtime and must not be evicted.
  Confidence: band=very_high; score=0.9
- Line 623: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // External allocations are owned by the inference runtime and cannot be spilled.
  Confidence: band=very_high; score=0.9
- Line 777: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: ActiveVRAMAllocator::allocate(size_t bytes, const std::string& owner_id, int gpu_device_id)
  Confidence: band=very_high; score=0.9
- Line 309: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_evict.push_back(id);
  Confidence: band=high; score=0.74
- Line 443: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(h);
  Confidence: band=high; score=0.74

### src/llm/adapter_registry.cpp
Total findings: 13

- Line 667: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::error("AdapterRegistry::hotLoad: weights_path must not be empty");
  Confidence: band=very_high; score=0.99
- Line 359: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility validation
  Confidence: band=high; score=0.8
- Line 384: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check version compatibility (exact or empty means "any")
  Confidence: band=high; score=0.8
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(meta);
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(meta);
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(meta);
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(meta);
  Confidence: band=high; score=0.74
- Line 556: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(meta);
  Confidence: band=high; score=0.74
- Line 556: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(meta);
  Confidence: band=high; score=0.74
- Line 583: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(meta);
  Confidence: band=high; score=0.74
- Line 599: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> by_model;
  Confidence: band=medium; score=0.66
- Line 600: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> by_domain;
  Confidence: band=medium; score=0.66
- Line 708: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->hot_load_callbacks.push_back(std::move(callback));
  Confidence: band=high; score=0.74

### src/llm/ai_orchestrator.cpp
Total findings: 13

- Line 121: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: spec.args_schema = tool->inputSchema();
  Confidence: band=very_high; score=0.99
- Line 121: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: spec.args_schema = tool->inputSchema();
  Confidence: band=very_high; score=0.9
- Line 368: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest AIOrchestrator::buildRequest(const OrchestratorContext& ctx,
  Confidence: band=very_high; score=0.9
- Line 370: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 457: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req  = buildRequest(ctx, mode);
  Confidence: band=very_high; score=0.9
- Line 461: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse resp = impl_->plugin->generate(req);
  Confidence: band=very_high; score=0.9
- Line 467: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.metadata.tokens_generated  = resp.tokens_generated;
  Confidence: band=very_high; score=0.9
- Line 565: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req = buildRequest(ctx, mode);
  Confidence: band=very_high; score=0.9
- Line 575: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse resp   = impl_->plugin->generateRAG(rag_ctx, req);
  Confidence: band=very_high; score=0.9
- Line 581: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.metadata.tokens_generated = resp.tokens_generated;
  Confidence: band=very_high; score=0.9
- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74
- Line 461: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: InferenceResponse resp = impl_->plugin->generate(req);
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(std::move(doc));
  Confidence: band=high; score=0.74

### src/llm/llm_deployment_plugin.cpp
Total findings: 12

- Line 173: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: loadModelRegistry();
  Confidence: band=very_high; score=0.99
- Line 334: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: ModelDownloadResult LLMDeploymentPlugin::downloadModel(const std::string& model_id,
  Confidence: band=very_high; score=0.99
- Line 416: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool LLMDeploymentPlugin::loadModel(const std::string& model_id, ILLMPlugin* llm_plugin) {
  Confidence: band=very_high; score=0.99
- Line 433: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (llm_plugin->loadModel(status->model_path, config)) {
  Confidence: band=very_high; score=0.99
- Line 866: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: void LLMDeploymentPlugin::loadModelRegistry() {
  Confidence: band=very_high; score=0.99
- Line 1101: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Only load model weights when explicitly configured to do so (store_weights_in_rocksdb).
  Confidence: band=very_high; score=0.99
- Line 1137: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<LLMModelMetadata> LLMDeploymentPlugin::loadModelFromStorage(const std::string& model_id) {
  Confidence: band=very_high; score=0.99
- Line 1143: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return model_storage_->loadModel(model_id);
  Confidence: band=very_high; score=0.99
- Line 610: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_remove.push_back(status.model_id);
  Confidence: band=high; score=0.74
- Line 780: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deployment_config.sources.push_back(source);
  Confidence: band=high; score=0.74
- Line 854: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j.push_back(model_json);
  Confidence: band=high; score=0.74
- Line 954: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: src_path += ".gguf";
  Confidence: band=high; score=0.74

### src/llm/explanation_generator.cpp
Total findings: 11

- Line 88: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: out << "INPUT QUERY:\n" << query << "\n\n";
  Confidence: band=very_high; score=0.99
- Line 127: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: out << "   Input Query: " << query << "\n\n";
  Confidence: band=very_high; score=0.99
- Line 186: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: steps.push_back("Analyzed the input query to understand intent");
  Confidence: band=very_high; score=0.99
- Line 311: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: out << "SECTION 2: INPUT\n";
  Confidence: band=very_high; score=0.99
- Line 88: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: out << "INPUT QUERY:\n" << query << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: out << "   Input Query: " << query << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 186: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: steps.push_back("Analyzed the input query to understand intent");
  Confidence: band=very_high; score=0.9
- Line 311: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: out << "SECTION 2: INPUT\n";
  Confidence: band=very_high; score=0.9
- Line 199: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(step.str());
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: common_keywords.push_back(qk);
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: keywords.push_back(word);
  Confidence: band=high; score=0.74

### src/llm/json_schema_converter.cpp
Total findings: 11

- Line 46: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  result += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 64: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += '_';
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!first) body += " | ";
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> required_set;
  Confidence: band=medium; score=0.66
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_props.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_props.push_back(r.get<std::string>());
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
  Confidence: band=high; score=0.74
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ordered_props.push_back(it.key());
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_rules.emplace_back(val_rule_name, val_body);
  Confidence: band=high; score=0.74
- Line 195: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!required_part.empty()) required_part += " ws \",\" ws ";
  Confidence: band=high; score=0.74
- Line 195: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!required_part.empty()) required_part += " ws \",\" ws ";
  Confidence: band=high; score=0.74

### src/llm/lora_framework/distributed_dataloader.cpp
Total findings: 10

- Line 101: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: batch_shape.insert(batch_shape.end(), sample_shape.begin(), sample_shape.end());
  Confidence: band=very_high; score=0.99
- Line 73: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: batch_samples.push_back(dataset_.get(sample_idx));
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: GPUTensor InMemoryDataset::get(size_t index) const {
  Confidence: band=very_high; score=0.9
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sharded_batch.emplace_back(std::vector<size_t>{0}, ctx_.get_device(gpu_idx));
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sample_shape = batch_samples[gpu_start].shape();
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: s += (d ? "×" : "") + std::to_string(sh[d]);
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: s += (d ? "×" : "") + std::to_string(sh[d]);
  Confidence: band=high; score=0.74
- Line 120: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: s += (d ? "×" : "") + std::to_string(sh[d]);
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto sample_data = batch_samples[i].cpu_data();
  Confidence: band=high; score=0.74
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sharded_batch.push_back(std::move(batched_tensor));
  Confidence: band=high; score=0.74

### src/llm/lora_framework/lora_storage_service_themisdb.cpp
Total findings: 10

- Line 168: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: spdlog::warn("Failed to deserialize or delete blob for adapter {}: {}",
  Confidence: band=very_high; score=0.99
- Line 403: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize entity
  Confidence: band=very_high; score=0.99
- Line 404: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity entity = BaseEntity::deserialize(adapter_id, *data);
  Confidence: band=very_high; score=0.99
- Line 729: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize the EncryptedBlob from base64 string
  Confidence: band=very_high; score=0.99
- Line 795: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity entity = BaseEntity::deserialize(adapter_id, *data);
  Confidence: band=very_high; score=0.99
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_storage_service.h"
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adapters.push_back(adapter_id);
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adapters.push_back(entry.path().filename().string());
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(version);
  Confidence: band=high; score=0.74
- Line 379: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(version);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/mixed_precision.cpp
Total findings: 10

- Line 35: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor MixedPrecisionTrainer::to_lower_precision(const Tensor& input) const {
  Confidence: band=very_high; score=0.99
- Line 37: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return input.clone();
  Confidence: band=very_high; score=0.99
- Line 42: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor output = input.clone();
  Confidence: band=very_high; score=0.99
- Line 65: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Tensor MixedPrecisionTrainer::to_fp32(const Tensor& input) const {
  Confidence: band=very_high; score=0.99
- Line 67: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return input.clone();
  Confidence: band=very_high; score=0.99
- Line 35: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor MixedPrecisionTrainer::to_lower_precision(const Tensor& input) const {
  Confidence: band=very_high; score=0.9
- Line 37: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return input.clone();
  Confidence: band=very_high; score=0.9
- Line 42: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor output = input.clone();
  Confidence: band=very_high; score=0.9
- Line 65: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Tensor MixedPrecisionTrainer::to_fp32(const Tensor& input) const {
  Confidence: band=very_high; score=0.9
- Line 67: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return input.clone();
  Confidence: band=very_high; score=0.9

### src/llm/prompt_evaluator.cpp
Total findings: 10

- Line 53: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<std::string>& outputs,
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (outputs.size() != expected.size()) {
  Confidence: band=very_high; score=0.9
- Line 63: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (outputs.empty()) {
  Confidence: band=very_high; score=0.9
- Line 71: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (size_t i = 0; i < outputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 72: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: auto metrics = evaluateSingle(outputs[i], expected[i]);
  Confidence: band=very_high; score=0.9
- Line 74: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similarities.push_back(metrics.semantic_similarity);
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set1(tokens1.begin(), tokens1.end());
  Confidence: band=medium; score=0.66
- Line 146: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set2(tokens2.begin(), tokens2.end());
  Confidence: band=medium; score=0.66
- Line 208: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> output_set(tokens_output.begin(), tokens_output.end());
  Confidence: band=medium; score=0.66
- Line 264: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(' ');
  Confidence: band=high; score=0.74

### src/llm/continuous_batch_scheduler.cpp
Total findings: 9

- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * PR History (last 5): #4332 Implement AIOrchestrator to... (2026-03-19) | #242 Complete PagedAttention int... (2026-03-11) | #215 Implement P1 LLM Inference ... (2026-03-11) | #1211 Fix null pointer dereferenc... (2026-03-11) | #1215 Fix thread-safety: atomic c... (2026-03-11)
  Confidence: band=very_high; score=0.87
- Line 58: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceRequest& request,
  Confidence: band=very_high; score=0.9
- Line 60: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::function<void(const InferenceResponse&)> callback
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: scheduled->inference_request = request;
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::vector<InferenceResponse>& responses
  Confidence: band=very_high; score=0.9
- Line 550: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: size_t total_tokens = request->total_prompt_tokens + request->inference_request.max_tokens;
  Confidence: band=very_high; score=0.9
- Line 575: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: size_t tokens = request->total_prompt_tokens + request->inference_request.max_tokens;
  Confidence: band=very_high; score=0.9
- Line 650: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: total_tokens_generated += (req->tokens_generated - 1);
  Confidence: band=very_high; score=0.9
- Line 329: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_retry.push_back(req);
  Confidence: band=high; score=0.74

### src/llm/embedded_llm_stub.cpp
Total findings: 9

- Line 17: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: //             which provides full inference via LlamaWrapper.
  Confidence: band=very_high; score=0.9
- Line 167: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = generate_full_fn_(request);
  Confidence: band=very_high; score=0.9
- Line 186: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse resp;
  Confidence: band=very_high; score=0.9
- Line 231: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back('\n');
  Confidence: band=high; score=0.74
- Line 89: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return generate(merged);
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({});
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto text = generate(prompt, max_tokens);
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto text = generate(prompt, max_tokens);
  Confidence: band=high; score=0.74

### src/llm/ethics_aware_confidence_detector.cpp
Total findings: 9

- Line 194: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.reasoning = generateReasoning(result);
  Confidence: band=very_high; score=0.9
- Line 251: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.reasoning = generateReasoning(result);
  Confidence: band=very_high; score=0.9
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected.push_back(pattern);
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected.push_back(pattern);
  Confidence: band=high; score=0.74
- Line 382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected.push_back(pattern);
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected.push_back(pattern);
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected.push_back(word);
  Confidence: band=high; score=0.74
- Line 412: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: detected.push_back(word);
  Confidence: band=high; score=0.74
- Line 418: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> unique(detected.begin(), detected.end());
  Confidence: band=medium; score=0.66

### src/llm/adapter_load_balancer.cpp
Total findings: 8

- Line 288: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = placements_.find(adapter_id);
  Confidence: band=very_high; score=0.9
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overloaded_gpus.push_back(gpu_id);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: overloaded_gpus.push_back(gpu_id);
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(adapter_id);
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back(adapter_id);
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: evicted.push_back(adapter_id);
  Confidence: band=high; score=0.74
- Line 485: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back({adapter_id, it->second});
  Confidence: band=high; score=0.74
- Line 500: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_evict.push_back(adapter_id);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/adapter_sync_manager.cpp
Total findings: 8

- Line 103: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: sync_thread_ = std::thread([this]() { syncLoop(); });
  Confidence: band=very_high; score=0.9
- Line 11: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_storage_service.h"
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Failed to sync " + adapter_id);
  Confidence: band=high; score=0.74
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(status);
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: void onSyncComplete(std::function<void(const SyncJobResult&)> callback) {
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: peer_ids.push_back(shard.shard_id);
  Confidence: band=high; score=0.74
- Line 634: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: void AdapterSyncManager::onSyncComplete(std::function<void(const SyncJobResult&)> callback) {
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: impl_->onSyncComplete(callback);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/adaptive_batcher.cpp
Total findings: 8

- Line 175: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_memory = sequence_length * config_.hidden_dim * 4;
  Confidence: band=very_high; score=0.99
- Line 179: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t base_estimate = input_memory + activation_memory + gradient_memory;
  Confidence: band=very_high; score=0.99
- Line 203: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_memory = sequence_length * config.hidden_dim * 4;
  Confidence: band=very_high; score=0.99
- Line 206: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return input_memory + activation_memory + gradient_memory;
  Confidence: band=very_high; score=0.99
- Line 175: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_memory = sequence_length * config_.hidden_dim * 4;
  Confidence: band=very_high; score=0.9
- Line 179: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t base_estimate = input_memory + activation_memory + gradient_memory;
  Confidence: band=very_high; score=0.9
- Line 203: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_memory = sequence_length * config.hidden_dim * 4;
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return input_memory + activation_memory + gradient_memory;
  Confidence: band=very_high; score=0.9

### src/llm/lora_framework/gradient_checkpointing.cpp
Total findings: 8

- Line 86: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: void GradientCheckpointer::saveCheckpoint(int layer_id, const GPUTensor& input,
  Confidence: band=very_high; score=0.99
- Line 93: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: data.input = input.clone();  // Make a copy of the input
  Confidence: band=very_high; score=0.99
- Line 95: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: data.activation_size_bytes = input.size() * sizeof(float);  // Assuming float32
  Confidence: band=very_high; score=0.99
- Line 118: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: GPUTensor recomputed = checkpoint.forward_fn(checkpoint.input);
  Confidence: band=very_high; score=0.99
- Line 86: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: void GradientCheckpointer::saveCheckpoint(int layer_id, const GPUTensor& input,
  Confidence: band=very_high; score=0.9
- Line 93: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: data.input = input.clone();  // Make a copy of the input
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: data.activation_size_bytes = input.size() * sizeof(float);  // Assuming float32
  Confidence: band=very_high; score=0.9
- Line 118: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: GPUTensor recomputed = checkpoint.forward_fn(checkpoint.input);
  Confidence: band=very_high; score=0.9

### src/llm/lora_framework/lora_provenance.cpp
Total findings: 8

- Line 188: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // InferenceAuditEntry – serialisation & hash computation
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: json InferenceAuditEntry::toJSON() const {
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceAuditEntry InferenceAuditEntry::fromJSON(const json& j) {
  Confidence: band=very_high; score=0.9
- Line 208: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceAuditEntry e;
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string InferenceAuditEntry::computeContentHash() const {
  Confidence: band=very_high; score=0.9
- Line 264: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::unordered_map<std::string, std::vector<InferenceAuditEntry>> audit_logs;
  Confidence: band=very_high; score=0.9
- Line 443: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceAuditEntry LoRAProvenanceManager::appendAuditEntry(
  Confidence: band=very_high; score=0.9
- Line 445: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceAuditEntry entry) {
  Confidence: band=very_high; score=0.9

### src/llm/openai_compat_adapter.cpp
Total findings: 8

- Line 73: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::variant<InferenceRequest, std::string>
  Confidence: band=very_high; score=0.9
- Line 75: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceResponse& response,
  Confidence: band=very_high; score=0.9
- Line 399: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Unknown roles are silently skipped to allow forward-compatibility
  Confidence: band=high; score=0.8
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: req.stop_sequences.push_back(item.get<std::string>());
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tool_calls_arr.push_back({
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: content_text += part["text"].get<std::string>();
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: content_text += part["text"].get<std::string>();
  Confidence: band=high; score=0.74

### src/llm/lora_framework/vram_allocator.cpp
Total findings: 7

- Line 717: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: cudaError_t err = cudaMalloc(&ptr, size_bytes);
  Confidence: band=very_high; score=0.99
- Line 814: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: ptr
  Context: security::VRAMSecureClear::secureClearHIP(ptr, block_size);
  Confidence: band=very_high; score=0.99
- Line 829: severity=CRITICAL; category=gpu_memory_safety; pattern=use_after_free_gpu
  Description: Use of freed GPU memory: ptr
  Context: if (backend_context_ && ptr) {
  Confidence: band=very_high; score=0.99
- Line 501: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: void* VRAMAllocator::allocate(size_t size_bytes, size_t alignment) {
  Confidence: band=very_high; score=0.9
- Line 545: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: void VRAMAllocator::deallocate(void* ptr) {
  Confidence: band=very_high; score=0.9
- Line 801: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: // REL-64: check cudaFree return value in release_backend_ptr_
  Confidence: band=very_high; score=0.9
- Line 805: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: spdlog::error("VRAMAllocator::release_backend_ptr_: cudaFree failed: {}",
  Confidence: band=very_high; score=0.9

### src/llm/mode_spec_loader.cpp
Total findings: 7

- Line 397: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (seenTools.find(ta) == seenTools.end()) {
  Confidence: band=very_high; score=0.9
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(item.as<std::string>());
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pack.models.push_back(parseModelEntry(m));
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pack.tools.push_back(parseToolSpec(t));
  Confidence: band=high; score=0.74
- Line 233: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pack.modes.push_back(parseModeSpec(m));
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> seenIds;
  Confidence: band=medium; score=0.66
- Line 382: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> seenTools;
  Confidence: band=medium; score=0.66

### src/llm/model_quantization_pipeline.cpp
Total findings: 7

- Line 229: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: desc.shape.push_back(dim.get<int64_t>());
  Confidence: band=high; score=0.74
- Line 229: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: desc.shape.push_back(dim.get<int64_t>());
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards.push_back(entry.path().string());
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<float>(v));
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(static_cast<float>(v));
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, LayerBuffers> layers;
  Confidence: band=medium; score=0.66
- Line 657: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, GptqBuffers> layers;
  Confidence: band=medium; score=0.66

### src/llm/constitutional_reasoning_engine.cpp
Total findings: 6

- Line 115: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = std::find_if(
  Confidence: band=very_high; score=0.9
- Line 457: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Avoids exposing, inferring, or encouraging misuse of private data.",
  Confidence: band=very_high; score=0.9
- Line 809: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "inferior", "superior race", "less intelligent"
  Confidence: band=very_high; score=0.9
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: critiques.push_back(critique);
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.applied_principles.push_back(principle.id);
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: violations.push_back(principle.id);
  Confidence: band=high; score=0.74

### src/llm/llama_resource_manager.cpp
Total findings: 6

- Line 281: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Priority list for LLM Inference (Vulkan first)
  Confidence: band=very_high; score=0.9
- Line 43: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: LlamaModelHandle::~LlamaModelHandle() {
  Confidence: band=high; score=0.74
- Line 47: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: LlamaModelHandle::LlamaModelHandle(LlamaModelHandle&& other) noexcept
  Confidence: band=high; score=0.74
- Line 109: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: LlamaContextHandle::~LlamaContextHandle() {
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: LlamaContextHandle::LlamaContextHandle(LlamaContextHandle&& other) noexcept
  Confidence: band=high; score=0.74
- Line 238: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: BackendAwareLlamaModelHandle::~BackendAwareLlamaModelHandle() {
  Confidence: band=high; score=0.74

### src/llm/llm_response_cache.cpp
Total findings: 6

- Line 116: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMResponseCache::put(const std::string& prompt, const InferenceResponse& response) {
  Confidence: band=very_high; score=0.9
- Line 138: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: entity.setField("inference_time_ms", Value{static_cast<double>(response.inference_time_ms)});
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::optional<InferenceResponse> LLMResponseCache::get(const std::string& prompt) {
  Confidence: band=very_high; score=0.9
- Line 300: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceResponse best_response;
  Confidence: band=very_high; score=0.9
- Line 303: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static const std::unordered_set<std::string> stopwords = {
  Confidence: band=medium; score=0.66
- Line 503: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> words;
  Confidence: band=medium; score=0.66

### src/llm/lora_router.cpp
Total findings: 6

- Line 25: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string computeHash(const std::string& input) {
  Confidence: band=very_high; score=0.99
- Line 27: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto hash = hasher(input);
  Confidence: band=very_high; score=0.99
- Line 25: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string computeHash(const std::string& input) {
  Confidence: band=very_high; score=0.9
- Line 27: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto hash = hasher(input);
  Confidence: band=very_high; score=0.9
- Line 196: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: decisions.push_back(routeQuery(query, base_model_id));
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back({adapter.adapter_id, similarity});
  Confidence: band=high; score=0.74

### src/llm/vision_encoder.cpp
Total findings: 6

- Line 286: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: int n_threads = config_->getResourceLimits().cpu_inference_threads;
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Calculate inference time
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto inference_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  Confidence: band=very_high; score=0.9
- Line 303: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: resource_monitor_->completeRequest(request_id, true, inference_time, 0);
  Confidence: band=very_high; score=0.9
- Line 317: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto inference_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: resource_monitor_->completeRequest(request_id, false, inference_time, 0);
  Confidence: band=very_high; score=0.9

### src/llm/llama_lora_adapter.cpp
Total findings: 5

- Line 37: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility:
  Confidence: band=high; score=0.8
- Line 198: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * to inference contexts. It uses the real llama.cpp API when available.
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * @param adapter_path Path to LoRA adapter file (for compatibility with old signature)
  Confidence: band=high; score=0.8
- Line 228: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Note: This simplified signature is for backward compatibility only
  Confidence: band=high; score=0.8
- Line 443: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: * @brief Apply LoRA adapter with integer handle (compatibility overload)
  Confidence: band=high; score=0.8

### src/llm/llm_ingestion_bridge.cpp
Total findings: 5

- Line 29: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 41: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = LLMPluginManager::instance().generate(req);
  Confidence: band=very_high; score=0.9
- Line 13: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: * Bridges ingestion::ITextGenerationBackend to LLMPluginManager::generate().
  Confidence: band=high; score=0.74
- Line 41: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto response = LLMPluginManager::instance().generate(req);
  Confidence: band=high; score=0.74
- Line 44: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: spdlog::error("[LlmIngestionBridge] generate() failed: {}", e.what());
  Confidence: band=high; score=0.74

### src/llm/lora_framework/lora_storage_service.cpp
Total findings: 5

- Line 322: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Load weights
  Confidence: band=very_high; score=0.99
- Line 2: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: * ThemisDB | File: lora_storage_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=high; score=0.74
- Line 10: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_storage_service.h"
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adapters.push_back(entry.path().filename().string());
  Confidence: band=high; score=0.74
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(version);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/sequence_packer.cpp
Total findings: 5

- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packed_data.push_back(static_cast<float>(token));
  Confidence: band=high; score=0.74
- Line 52: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: packed_data.push_back(static_cast<float>(token));
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: seq_data.push_back(packed_data[token_idx * hidden_dim + k]);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: seq_data.push_back(packed_data[token_idx * hidden_dim + k]);
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sorted.push_back(sequences[idx]);
  Confidence: band=high; score=0.74

### src/llm/sampling_strategy.cpp
Total findings: 5

- Line 130: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = std::find(indices.begin(), indices.end(), id);
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: for (int id : nucleus) { auto it = std::find(indices.begin(), indices.end(), id); size_t pidx = std::distance(indices.begin(), it); float p = probs[pidx]; nuc_probs.push_back(p); nuc_sum += p; }
  Confidence: band=very_high; score=0.9
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: probs.push_back(p);
  Confidence: band=high; score=0.74
- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: probs.push_back(p);
  Confidence: band=high; score=0.74
- Line 132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nuc_probs.push_back(p);
  Confidence: band=high; score=0.74

### src/llm/inference_handle.cpp
Total findings: 4

- Line 2: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * ThemisDB | File: inference_handle.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=very_high; score=0.9
- Line 10: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_handle.h"
  Confidence: band=very_high; score=0.9
- Line 16: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void InferenceHandle::cancel() {
  Confidence: band=very_high; score=0.9
- Line 21: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::info("Cancellation requested for inference: {}", request_id_);
  Confidence: band=very_high; score=0.9

### src/llm/llamacpp_inference_engine.cpp
Total findings: 4

- Line 2: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * ThemisDB | File: llamacpp_inference_engine.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
  Confidence: band=very_high; score=0.9
- Line 10: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/llamacpp_inference_engine.h"
  Confidence: band=very_high; score=0.9
- Line 293: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(current.substr(a, b - a + 1));
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(tok));
  Confidence: band=high; score=0.74

### src/llm/lora_framework/directx_pipeline.cpp
Total findings: 4

- Line 125: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // SRV descriptor table (inputs)
  Confidence: band=very_high; score=0.99
- Line 106: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // UAV descriptor table (outputs)
  Confidence: band=very_high; score=0.9
- Line 125: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // SRV descriptor table (inputs)
  Confidence: band=very_high; score=0.9
- Line 125: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // SRV descriptor table (inputs)
  Confidence: band=very_high; score=0.9

### src/llm/lora_framework/gpu_training_loop.cpp
Total findings: 4

- Line 554: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<GPUTensor> grad_outputs;
  Confidence: band=very_high; score=0.9
- Line 555: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: grad_outputs.reserve(1);
  Confidence: band=very_high; score=0.9
- Line 556: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: grad_outputs.push_back(std::move(grad_output));
  Confidence: band=very_high; score=0.9
- Line 557: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: multi_gpu_layer_->backward(grad_outputs);
  Confidence: band=very_high; score=0.9

### src/llm/meta_prompt_generator.cpp
Total findings: 4

- Line 133: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: analysis << "**Input**: " << examples[i].first << "\n";
  Confidence: band=very_high; score=0.99
- Line 139: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: analysis << "2. Assess whether examples cover the input space adequately\n";
  Confidence: band=very_high; score=0.99
- Line 133: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: analysis << "**Input**: " << examples[i].first << "\n";
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: analysis << "2. Assess whether examples cover the input space adequately\n";
  Confidence: band=very_high; score=0.9

### src/llm/paged_kv_cache_manager.cpp
Total findings: 4

- Line 51: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allocated.push_back(block_id);
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: child_table.block_ids.push_back(block_id);
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<int> known_free(free_block_ids_.begin(), free_block_ids_.end());
  Confidence: band=medium; score=0.66
- Line 246: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: free_block_ids_.push_back(block.block_id);
  Confidence: band=high; score=0.74

### src/llm/prompt_manager.cpp
Total findings: 4

- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& context) const {
  Confidence: band=medium; score=0.66
- Line 302: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: caps_array.push_back(cap);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tables_array.push_back(table_info);
  Confidence: band=high; score=0.74

### src/llm/safety/guardian.cpp
Total findings: 4

- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(c);
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_topics.emplace_back(topic);
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_topics.emplace_back(topic);
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matched_topics.emplace_back(topic);
  Confidence: band=high; score=0.74

### src/llm/attention/kv_cache_manager.cpp
Total findings: 3

- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table.block_ids.push_back(block_id);
  Confidence: band=high; score=0.74
- Line 82: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: table.block_ids.push_back(block_id);
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_table.block_ids.push_back(block_id);
  Confidence: band=high; score=0.74

### src/llm/llm_prefix_cache.cpp
Total findings: 3

- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * PR History (last 5): #3759 feat(llm): implement KV-cac... (2026-03-12) | #239 Replace LLMPrefixCache stub... (2026-03-11) | #215 Implement P1 LLM Inference ... (2026-03-11) | #1100 [WIP] Fix missing and stub ... (2026-03-11) | #1126 Add dynamic cache routing, ... (2026-03-11)
  Confidence: band=very_high; score=0.87
- Line 75: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: entry.generated_text = generated_text;
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (mag_a == 0.0 || mag_b == 0.0) {
  Confidence: band=very_high; score=0.9

### src/llm/lora_framework/distributed_trainer.cpp
Total findings: 3

- Line 5: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #570 [LoRA Phase 10] Add readine... (2026-03-11)
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #570 [LoRA Phase 10] Add readine... (2026-03-11)
  Confidence: band=very_high; score=0.9
- Line 194: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: //                   or multi-node training this leads to gradient staleness
  Confidence: band=high; score=0.74

### src/llm/lora_framework/embedding_provider.cpp
Total findings: 3

- Line 347: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Create batch for inference
  Confidence: band=very_high; score=0.9
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cache_out.push_back(entry);
  Confidence: band=high; score=0.74
- Line 395: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entries.push_back({text, entry.cached_at});
  Confidence: band=high; score=0.74

### src/llm/lora_framework/feedback_plugin.cpp
Total findings: 3

- Line 23: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void PrivacyFilterPlugin::process(Feedback& feedback) {
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: patterns.emplace_back(w, std::regex_constants::icase |
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void CacheAwareWeightingPlugin::process(Feedback& feedback) {
  Confidence: band=high; score=0.74

### src/llm/lora_framework/gpu_memory.cpp
Total findings: 3

- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: devices.push_back(Device::cpu());
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: devices.push_back(Device::cuda());
  Confidence: band=high; score=0.74
- Line 378: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backends.push_back(info);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/gradient_utils.cpp
Total findings: 3

- Line 120: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: accumulated.push_back(grad_ptr->clone());
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: accumulated_gradients_.push_back(Tensor(grad_ptr->shape(), 0.0f));
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(&grad);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/lora_orchestrator.cpp
Total findings: 3

- Line 185: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->versions[adapter_id].push_back(version);
  Confidence: band=high; score=0.74
- Line 206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: jobs.push_back(kv.second);
  Confidence: band=high; score=0.74

### src/llm/lora_framework/model_compatibility.cpp
Total findings: 3

- Line 159: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Try to infer from filename
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Step 4: Check quantization compatibility
  Confidence: band=high; score=0.8
- Line 236: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto meta = header["__metadata__"];
  Confidence: band=high; score=0.74

### src/llm/lora_framework/paged_optimizer.cpp
Total findings: 3

- Line 44: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parameters_.push_back(param);
  Confidence: band=high; score=0.74
- Line 261: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<Tensor*, PagedOptimizerState>& states,
  Confidence: band=medium; score=0.66
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: evictable.push_back({state.momentum.last_access_time, &state});
  Confidence: band=high; score=0.74

### src/llm/lora_framework/vulkan_context.cpp
Total findings: 3

- Line 5: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #571 Implement Vulkan compute pi... (2026-03-11)
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #571 Implement Vulkan compute pi... (2026-03-11)
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Enable validation layers for device (deprecated but still used in some drivers)
  Confidence: band=high; score=0.8

### src/llm/lora_framework/vulkan_pipeline.cpp
Total findings: 3

- Line 5: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #571 Implement Vulkan compute pi... (2026-03-11)
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #571 Implement Vulkan compute pi... (2026-03-11)
  Confidence: band=very_high; score=0.9
- Line 388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buffer_infos.push_back(buffer_info);
  Confidence: band=high; score=0.74

### src/llm/paged_block_manager.cpp
Total findings: 3

- Line 79: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: int PagedBlockManager::allocate() {
  Confidence: band=very_high; score=0.9
- Line 102: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: void PagedBlockManager::deallocate(int block_id) {
  Confidence: band=very_high; score=0.9
- Line 63: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allocated_ids.push_back(block_id);
  Confidence: band=high; score=0.74

### src/llm/shared_worker_pool.cpp
Total findings: 3

- Line 75: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& q : thread_queues_) {
  Confidence: band=very_high; score=0.9
- Line 200: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 1; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 43: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&SharedWorkerPool::workerLoop, this, i);
  Confidence: band=high; score=0.74

### src/llm/attention/flash_attention.cpp
Total findings: 2

- Line 169: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs.
  Confidence: band=very_high; score=0.9
- Line 169: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Validate inputs.
  Confidence: band=very_high; score=0.9

### src/llm/block_table.cpp
Total findings: 2

- Line 44: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_blocks.push_back(block_id);
  Confidence: band=high; score=0.74
- Line 44: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_blocks.push_back(block_id);
  Confidence: band=high; score=0.74

### src/llm/feedback_store.cpp
Total findings: 2

- Line 5: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #365 Implement feedback collecti... (2026-03-11) | #1214 Add null-pointer safety uti... (2026-03-11)
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #365 Implement feedback collecti... (2026-03-11) | #1214 Add null-pointer safety uti... (2026-03-11)
  Confidence: band=very_high; score=0.9

### src/llm/inline_training_engine.cpp
Total findings: 2

- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hist.push_back(m.toJSON());
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ckpts.emplace_back(step_num, entry.path().string());
  Confidence: band=high; score=0.74

### src/llm/lora_framework/directx_descriptors.cpp
Total findings: 2

- Line 145: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: D3D12_CPU_DESCRIPTOR_HANDLE DirectXDescriptors::get_cpu_handle(uint32_t index) const {
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: D3D12_GPU_DESCRIPTOR_HANDLE DirectXDescriptors::get_gpu_handle(uint32_t index) const {
  Confidence: band=high; score=0.74

### src/llm/lora_framework/gguf_converter.cpp
Total findings: 2

- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shape.push_back(static_cast<size_t>(dim));
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shape.push_back(static_cast<size_t>(dim));
  Confidence: band=high; score=0.74

### src/llm/lora_framework/llama_tokenizer.cpp
Total findings: 2

- Line 31: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: model_params.vocab_only = true;  // Only load tokenizer, not weights
  Confidence: band=very_high; score=0.99
- Line 33: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: model_ = llama_load_model_from_file(model_path.c_str(), model_params);
  Confidence: band=very_high; score=0.99

### src/llm/lora_framework/paged_memory_manager.cpp
Total findings: 2

- Line 79: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function allocate without trace point
  Context: PagedBuffer PagedMemoryManager::allocate(size_t size, const Device& device) {
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function deallocate without trace point
  Context: void PagedMemoryManager::deallocate(PagedBuffer& buffer) {
  Confidence: band=very_high; score=0.9

### src/llm/lora_framework/vulkan_buffer.cpp
Total findings: 2

- Line 5: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #571 Implement Vulkan compute pi... (2026-03-11)
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #571 Implement Vulkan compute pi... (2026-03-11)
  Confidence: band=very_high; score=0.9

### src/llm/mcp_tool_bridge.cpp
Total findings: 2

- Line 95: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const json        schema      = t.value("inputSchema", json::object());
  Confidence: band=very_high; score=0.99
- Line 95: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const json        schema      = t.value("inputSchema", json::object());
  Confidence: band=very_high; score=0.9

### src/llm/model_downloader.cpp
Total findings: 2

- Line 513: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: std::optional<ModelDownloadConfig> loadModelConfigFromYAML(
  Confidence: band=very_high; score=0.99
- Line 499: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models.push_back(model["name"]);
  Confidence: band=high; score=0.74

### src/llm/model_router.cpp
Total findings: 2

- Line 27: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: compiled.emplace_back(pattern,
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (t.is_string()) tags.push_back(t.get<std::string>());
  Confidence: band=high; score=0.74

### src/llm/safety/monitoring.cpp
Total findings: 2

- Line 35: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"': out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 40: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: default: out.push_back(c); break;
  Confidence: band=high; score=0.74

### src/llm/security/signature_verifier.cpp
Total findings: 2

- Line 52: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs
  Confidence: band=very_high; score=0.9
- Line 52: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Validate inputs
  Confidence: band=very_high; score=0.9

### src/llm/speculative_decoder.cpp
Total findings: 2

- Line 15: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: *   "Fast Inference from Transformers via Speculative Decoding"
  Confidence: band=very_high; score=0.9
- Line 21: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * are handled by InferenceEngineEnhanced.
  Confidence: band=very_high; score=0.9

### src/llm/adaptive_vram_allocator.cpp
Total findings: 1

- Line 281: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceConfig& config
  Confidence: band=very_high; score=0.9

### src/llm/ai_decision_auditor.cpp
Total findings: 1

- Line 488: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: export_data["decisions"].push_back(decision.toJson());
  Confidence: band=high; score=0.74

### src/llm/decision_record_yaml_processor.cpp
Total findings: 1

- Line 224: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Replace colons from ISO timestamp in filename (Windows compatibility)
  Confidence: band=high; score=0.8

### src/llm/feedback_plugin_basic.cpp
Total findings: 1

- Line 24: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: spam_keywords_.push_back(keyword.get<std::string>());
  Confidence: band=high; score=0.74

### src/llm/gguf_loader.cpp
Total findings: 1

- Line 404: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata_.tensors.push_back(tensor);
  Confidence: band=high; score=0.74

### src/llm/grammar.cpp
Total findings: 1

- Line 142: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: llama_grammar* Grammar::getHandle() const {
  Confidence: band=high; score=0.74

### src/llm/kv_cache_buffer.cpp
Total findings: 1

- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: buffers_.emplace_back(std::make_shared<KVCacheBuffer>(config_.buffer_config));
  Confidence: band=high; score=0.74

### src/llm/kv_prefix_transfer_manager.cpp
Total findings: 1

- Line 114: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "[KVPrefix] Transfer to shard={} failed: {}; inference continues cold",
  Confidence: band=very_high; score=0.9

### src/llm/llama_grammar_adapter.cpp
Total findings: 1

- Line 37: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Compatibility:
  Confidence: band=high; score=0.8

### src/llm/lookup_decoder.cpp
Total findings: 1

- Line 64: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::vector<int>,
  Confidence: band=medium; score=0.66

### src/llm/lora_framework/adapter_consistency_checker.cpp
Total findings: 1

- Line 170: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // If versions are equal, compare timestamps
  Confidence: band=high; score=0.74

### src/llm/lora_framework/custom_allreduce.cpp
Total findings: 1

- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_gpu_data.push_back(tensor.cpu_data());
  Confidence: band=high; score=0.74

### src/llm/lora_framework/gpu_embedding_layer.cpp
Total findings: 1

- Line 77: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Validate input
  Confidence: band=very_high; score=0.9

### src/llm/lora_framework/lora_checkpoint_manager.cpp
Total findings: 1

- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: checkpoints.emplace_back(step, entry.path().string());
  Confidence: band=high; score=0.74

### src/llm/lora_framework/multi_gpu.cpp
Total findings: 1

- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: devices_.push_back(Device::cuda(i));
  Confidence: band=high; score=0.74

### src/llm/lora_framework/resource_profiler.cpp
Total findings: 1

- Line 103: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->callbacks.push_back(std::move(callback));
  Confidence: band=high; score=0.74

### src/llm/prompt_optimizer.cpp
Total findings: 1

- Line 169: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto meta_result = meta_gen.generateImprovementPrompt(
  Confidence: band=very_high; score=0.9

### src/llm/streaming_handler.cpp
Total findings: 1

- Line 42: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param s Raw input string.
  Confidence: band=very_high; score=0.9

### src/llm/gpu_safe_fail.cpp
Total findings: 0


### src/llm/llm_interaction_store.cpp
Total findings: 0


### src/llm/llm_security_utils.cpp
Total findings: 0


### src/llm/lora_certificate_store.cpp
Total findings: 0


### src/llm/lora_framework/directx_buffer.cpp
Total findings: 0


### src/llm/lora_framework/directx_context.cpp
Total findings: 0


### src/llm/lora_framework/directx_shader.cpp
Total findings: 0


### src/llm/lora_framework/gpu_tensor.cpp
Total findings: 0


### src/llm/lora_framework/lora_feedback_storage.cpp
Total findings: 0


### src/llm/lora_framework/nccl_backend.cpp
Total findings: 0


### src/llm/lora_framework/rccl_backend.cpp
Total findings: 0


### src/llm/paged_kv_cache.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
