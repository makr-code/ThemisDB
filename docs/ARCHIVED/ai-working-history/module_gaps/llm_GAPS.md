# llm Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: llm
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 2146
- Actionable Findings (Critical + High): 1524
- Affected Files: 143

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 567 |
| High | 957 |
| Medium | 548 |
| Low | 74 |

## Category Summary

| Category | Count |
|---|---:|
| data_race | 330 |
| model_integrity_gap | 123 |
| copy_overhead | 109 |
| pointer_arithmetic_unbounded | 109 |
| resource_leaked_in_exception | 108 |
| db_connection_leak | 104 |
| uninitialized_access | 84 |
| hardcoded_output | 70 |
| no_retry_logic | 64 |
| uncaught_exception | 63 |
| null_dereference | 59 |
| unnecessary_copy | 59 |
| string_concat_loop | 51 |
| missing_resource_limits | 49 |
| primitive_no_volatile | 48 |
| manual_cleanup | 44 |
| unordered_container_iter | 40 |
| unvalidated_llm_output | 39 |
| o_n_squared | 36 |
| legacy_or_compat_path | 35 |
| range_temporary | 26 |
| generic_catch | 22 |
| unchecked_cuda_call | 19 |
| prompt_injection | 18 |
| hardcoded_path | 17 |
| no_timeout | 17 |
| unspecified_consistency | 17 |
| delete_without_nullptr | 16 |
| explicit_delete | 16 |
| missing_latency_metric | 16 |
| lock_contention | 15 |
| size_assumption | 15 |
| missing_move_constructor_defaulted | 14 |
| blocking_no_timeout | 13 |
| repeated_search | 13 |
| shared_state_no_sync | 13 |
| unchecked_malloc | 13 |
| unsanitized_llm_input | 13 |
| delete_no_nullptr | 12 |
| stale_doc_section_reference | 12 |
| deadlock_risk | 11 |
| gpu_memory_leak | 11 |
| thread_join_no_timeout | 11 |
| nested_loop_find | 10 |
| arithmetic_overflow | 9 |
| missing_trace_point | 9 |
| exception_in_destructor | 8 |
| missing_correlation_id | 8 |
| command_injection | 7 |
| memory_order | 7 |
| sql_injection | 7 |
| use_after_free_gpu | 7 |
| fp_exact_comparison | 6 |
| map_vs_unordered_map | 6 |
| repeated_lookup | 6 |
| shift_overflow | 6 |
| unchecked_array_index | 6 |
| unchecked_memcpy | 6 |
| allocation_loop | 5 |
| missing_vector_reserve | 5 |
| smart_ptr_misuse | 5 |
| expensive_inner_op | 4 |
| explicit_lock_unlock | 4 |
| lock_in_loop | 4 |
| uninitialized_member_field | 4 |
| endl_in_loop | 3 |
| manual_cleanup_in_destructor | 3 |
| double_lock | 2 |
| duplicate_qualified_signature | 2 |
| expensive_copy | 2 |
| iterator_invalidation | 2 |
| missing_consensus | 2 |
| missing_sync_threads | 2 |
| missing_version_tracking | 2 |
| pointer_without_null_check | 2 |
| insecure_model_url | 1 |
| layer_dependency_violation | 1 |
| module_doc_linkset_drift | 1 |
| multiplication_overflow | 1 |
| path_traversal | 1 |
| regex_in_loop | 1 |
| stale_read_undocumented | 1 |
| timestamp_sorting_unstable | 1 |
| uninitialized_array | 1 |
| unstructured_log | 1 |
| windows_only_api | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| llm/llama_wrapper.cpp | 129 | 47 | 65 | 16 | 1 |
| llm/vision_config.cpp | 111 | 83 | 1 | 27 | 0 |
| llm/async_inference_engine.cpp | 94 | 13 | 52 | 29 | 0 |
| llm/inference_engine_enhanced.cpp | 89 | 11 | 58 | 20 | 0 |
| llm/gpu_memory_manager.cpp | 84 | 14 | 54 | 16 | 0 |
| llm/lora_framework/vram_allocator.cpp | 82 | 3 | 76 | 3 | 0 |
| llm/lora_framework/kernels/directx_kernels.cpp | 54 | 37 | 10 | 0 | 7 |
| llm/multi_lora_manager.cpp | 54 | 24 | 18 | 12 | 0 |
| llm/model_loader.cpp | 43 | 29 | 13 | 1 | 0 |
| llm/docs_assistant.cpp | 40 | 14 | 18 | 7 | 1 |
| llm/llm_model_storage.cpp | 38 | 24 | 13 | 1 | 0 |
| llm/lora_framework/lora_storage_service_themisdb.cpp | 38 | 19 | 16 | 3 | 0 |
| llm/llm_plugin_manager.cpp | 36 | 14 | 18 | 4 | 0 |
| llm/ml_model_manager.cpp | 36 | 5 | 24 | 7 | 0 |
| llm/lora_framework/lora_training_service.cpp | 33 | 10 | 15 | 7 | 1 |
| llm/distributed_training_coordinator.cpp | 32 | 9 | 15 | 8 | 0 |
| llm/multi_perspective_generator.cpp | 31 | 11 | 3 | 17 | 0 |
| llm/active_vram_allocator.cpp | 29 | 1 | 24 | 4 | 0 |
| llm/constitutional_reasoning_engine.cpp | 29 | 17 | 9 | 3 | 0 |
| llm/production_validator.cpp | 29 | 12 | 7 | 10 | 0 |
| llm/grafana_metrics.cpp | 27 | 7 | 12 | 8 | 0 |
| llm/lora_framework/kernels/hip_kernels.cpp | 27 | 3 | 8 | 12 | 4 |
| llm/vision_encoder.cpp | 27 | 15 | 4 | 8 | 0 |
| llm/json_schema_converter.cpp | 25 | 0 | 7 | 18 | 0 |
| llm/meta_prompt_generator.cpp | 25 | 0 | 2 | 23 | 0 |
| llm/moral_analyzer.cpp | 24 | 4 | 4 | 16 | 0 |
| llm/llm_deployment_plugin.cpp | 23 | 8 | 11 | 4 | 0 |
| llm/lora_framework/kernels/hip_fused_kernels.cpp | 23 | 1 | 5 | 17 | 0 |
| llm/lora_framework/multi_gpu_trainer.cpp | 23 | 1 | 1 | 5 | 16 |
| llm/ethics_aware_confidence_detector.cpp | 22 | 8 | 9 | 5 | 0 |
| llm/lora_framework/kernels/vulkan_kernels.cpp | 22 | 1 | 1 | 13 | 7 |
| llm/lora_framework/gpu_training_loop.cpp | 21 | 4 | 12 | 1 | 4 |
| llm/lora_framework/multi_gpu_lora_layer.cpp | 20 | 0 | 1 | 4 | 15 |
| llm/gguf_loader.cpp | 19 | 3 | 9 | 7 | 0 |
| llm/lora_framework/gpu_tensor.cpp | 19 | 2 | 12 | 5 | 0 |
| llm/lora_router.cpp | 19 | 14 | 4 | 1 | 0 |
| llm/lora_framework/base_model_adapter.cpp | 18 | 6 | 5 | 6 | 1 |
| llm/lora_security_validator.cpp | 18 | 3 | 6 | 9 | 0 |
| llm/lora_framework/lora_layers.cpp | 17 | 6 | 11 | 0 | 0 |
| llm/lora_framework/lora_training_config.cpp | 17 | 1 | 1 | 15 | 0 |
| llm/vision_resource_monitor.cpp | 16 | 5 | 7 | 4 | 0 |
| llm/applications/themis_help_lora.cpp | 15 | 5 | 7 | 3 | 0 |
| llm/continuous_batch_scheduler.cpp | 15 | 3 | 9 | 3 | 0 |
| llm/lora_framework/paged_memory_manager.cpp | 15 | 0 | 15 | 0 | 0 |
| llm/lora_framework/paged_optimizer.cpp | 15 | 0 | 14 | 1 | 0 |
| llm/embedded_llm.cpp | 13 | 2 | 6 | 5 | 0 |
| llm/llm_response_cache.cpp | 13 | 2 | 8 | 3 | 0 |
| llm/attention/kv_cache_manager.cpp | 12 | 0 | 12 | 0 | 0 |
| llm/lora_framework/adapter_sync_manager.cpp | 12 | 3 | 3 | 6 | 0 |
| llm/lora_framework/kernels/cpu_fused_kernels.cpp | 12 | 0 | 12 | 0 | 0 |
| llm/lora_framework/lora_storage_service.cpp | 12 | 1 | 5 | 6 | 0 |
| llm/lora_framework/vulkan_pipeline.cpp | 12 | 6 | 6 | 0 | 0 |
| llm/fewshot_optimizer.cpp | 11 | 0 | 2 | 6 | 3 |
| llm/paged_kv_cache_manager.cpp | 11 | 1 | 8 | 2 | 0 |
| llm/prompt_evaluator.cpp | 11 | 0 | 2 | 4 | 5 |
| llm/ethical_guidelines_manager.cpp | 10 | 0 | 5 | 5 | 0 |
| llm/llama_resource_manager.cpp | 10 | 0 | 4 | 6 | 0 |
| llm/lora_framework/gpu_data_loader.cpp | 10 | 5 | 4 | 1 | 0 |
| llm/sampling_strategy.cpp | 10 | 0 | 6 | 4 | 0 |
| llm/embedded_llm_stub.cpp | 9 | 0 | 3 | 6 | 0 |
| llm/inline_training_engine.cpp | 9 | 2 | 3 | 4 | 0 |
| llm/lora_framework/gguf_converter.cpp | 9 | 1 | 6 | 2 | 0 |
| llm/model_quantization_pipeline.cpp | 9 | 0 | 5 | 4 | 0 |
| llm/shared_worker_pool.cpp | 9 | 1 | 7 | 1 | 0 |
| llm/streaming_handler.cpp | 9 | 0 | 2 | 7 | 0 |
| llm/adapter_registry.cpp | 8 | 1 | 5 | 2 | 0 |
| llm/decision_record_yaml_processor.cpp | 8 | 5 | 1 | 2 | 0 |
| llm/lora_certificate_store.cpp | 8 | 0 | 5 | 3 | 0 |
| llm/lora_framework/distributed_dataloader.cpp | 8 | 1 | 4 | 3 | 0 |
| llm/lora_framework/gpu_embedding_layer.cpp | 8 | 5 | 2 | 1 | 0 |
| llm/lora_framework/gpu_memory.cpp | 8 | 0 | 3 | 5 | 0 |
| llm/lora_framework/lora_checkpoint_manager.cpp | 8 | 0 | 5 | 3 | 0 |
| llm/lora_framework/lora_orchestrator.cpp | 8 | 4 | 4 | 0 | 0 |
| llm/model_downloader.cpp | 8 | 2 | 4 | 2 | 0 |
| llm/adapter_load_balancer.cpp | 7 | 1 | 6 | 0 | 0 |
| llm/aql_train_parser.cpp | 7 | 1 | 4 | 2 | 0 |
| llm/llama_lora_adapter.cpp | 7 | 0 | 6 | 1 | 0 |
| llm/lora_framework/distributed_trainer.cpp | 7 | 0 | 1 | 6 | 0 |
| llm/lora_framework/gradient_utils.cpp | 7 | 1 | 6 | 0 | 0 |
| llm/ai_orchestrator.cpp | 6 | 3 | 2 | 1 | 0 |
| llm/explanation_generator.cpp | 6 | 0 | 2 | 4 | 0 |
| llm/feedback_store.cpp | 6 | 1 | 3 | 2 | 0 |
| llm/lora_framework/gpu_lora_layers.cpp | 6 | 2 | 4 | 0 | 0 |
| llm/lora_framework/resource_profiler.cpp | 6 | 1 | 3 | 2 | 0 |
| llm/mode_spec_loader.cpp | 6 | 0 | 3 | 3 | 0 |
| llm/safety/monitoring.cpp | 6 | 0 | 0 | 6 | 0 |
| llm/ai_decision_auditor.cpp | 5 | 0 | 4 | 1 | 0 |
| llm/byzantine_detector.cpp | 5 | 0 | 1 | 4 | 0 |
| llm/lora_framework/adapter_consistency_checker.cpp | 5 | 0 | 2 | 3 | 0 |
| llm/lora_framework/flash_lora.cpp | 5 | 0 | 5 | 0 | 0 |
| llm/lora_framework/vulkan_buffer.cpp | 5 | 4 | 1 | 0 | 0 |
| llm/paged_kv_cache.cpp | 5 | 0 | 5 | 0 | 0 |
| llm/prompt_manager.cpp | 5 | 0 | 1 | 4 | 0 |
| llm/block_table.cpp | 4 | 0 | 4 | 0 | 0 |
| llm/llm_ingestion_bridge.cpp | 4 | 0 | 1 | 3 | 0 |
| llm/llm_interaction_store.cpp | 4 | 0 | 4 | 0 | 0 |
| llm/lookup_decoder.cpp | 4 | 0 | 3 | 1 | 0 |
| llm/lora_framework/directx_context.cpp | 4 | 2 | 0 | 1 | 1 |
| llm/lora_framework/directx_pipeline.cpp | 4 | 1 | 0 | 0 | 3 |
| llm/lora_framework/llama_tokenizer.cpp | 4 | 2 | 1 | 1 | 0 |
| llm/lora_framework/lora_feedback_storage.cpp | 4 | 3 | 1 | 0 | 0 |
| llm/lora_framework/quantized_model.cpp | 4 | 2 | 0 | 2 | 0 |
| llm/lora_framework/vulkan_context.cpp | 4 | 0 | 3 | 0 | 1 |
| llm/prompt_optimizer.cpp | 4 | 0 | 4 | 0 | 0 |
| llm/security/signature_verifier.cpp | 4 | 0 | 1 | 3 | 0 |
| llm/adaptive_vram_allocator.cpp | 3 | 0 | 3 | 0 | 0 |
| llm/federated_inference_coordinator.cpp | 3 | 0 | 3 | 0 | 0 |
| llm/grammar.cpp | 3 | 0 | 1 | 2 | 0 |
| llm/llamacpp_inference_engine.cpp | 3 | 0 | 3 | 0 | 0 |
| llm/lora_framework/adaptive_batcher.cpp | 3 | 0 | 3 | 0 | 0 |
| llm/lora_framework/custom_allreduce.cpp | 3 | 0 | 2 | 1 | 0 |
| llm/lora_framework/data_loader.cpp | 3 | 0 | 0 | 2 | 1 |
| llm/lora_framework/directx_buffer.cpp | 3 | 1 | 2 | 0 | 0 |
| llm/lora_framework/directx_descriptors.cpp | 3 | 1 | 0 | 2 | 0 |
| llm/lora_framework/feedback_plugin.cpp | 3 | 0 | 0 | 3 | 0 |
| llm/lora_framework/model_compatibility.cpp | 3 | 0 | 2 | 1 | 0 |
| llm/model_router.cpp | 3 | 0 | 2 | 1 | 0 |
| llm/multi_gpu_memory_coordinator.cpp | 3 | 0 | 1 | 2 | 0 |
| llm/openai_compat_adapter.cpp | 3 | 0 | 2 | 1 | 0 |
| llm/paged_block_manager.cpp | 3 | 0 | 3 | 0 | 0 |
| llm/attention/flash_attention.cpp | 2 | 1 | 0 | 0 | 1 |
| llm/gpu_safe_fail.cpp | 2 | 1 | 1 | 0 | 0 |
| llm/kernel_fusion.cpp | 2 | 0 | 2 | 0 | 0 |
| llm/kv_cache_buffer.cpp | 2 | 0 | 2 | 0 | 0 |
| llm/llama_grammar_adapter.cpp | 2 | 0 | 1 | 1 | 0 |
| llm/llm_prefix_cache.cpp | 2 | 0 | 2 | 0 | 0 |
| llm/lora_framework/embedding_provider.cpp | 2 | 0 | 0 | 2 | 0 |
| llm/lora_framework/gpu_utilization_monitor.cpp | 2 | 0 | 0 | 2 | 0 |
| llm/lora_framework/mixed_precision.cpp | 2 | 0 | 2 | 0 | 0 |
| llm/lora_framework/sequence_packer.cpp | 2 | 0 | 2 | 0 | 0 |
| llm/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| llm/feedback_plugin_basic.cpp | 1 | 0 | 1 | 0 | 0 |
| llm/inference_handle.cpp | 1 | 0 | 1 | 0 | 0 |
| llm/llm_model_audit_logger.cpp | 1 | 0 | 0 | 1 | 0 |
| llm/llm_security_utils.cpp | 1 | 0 | 1 | 0 | 0 |
| llm/lora_framework/directx_shader.cpp | 1 | 0 | 0 | 0 | 1 |
| llm/lora_framework/multi_gpu.cpp | 1 | 0 | 1 | 0 | 0 |
| llm/lora_framework/nccl_backend.cpp | 1 | 0 | 1 | 0 | 0 |
| llm/lora_framework/quantization.cpp | 1 | 0 | 0 | 1 | 0 |
| llm/lora_framework/rccl_backend.cpp | 1 | 0 | 1 | 0 | 0 |
| llm/mcp_tool_bridge.cpp | 1 | 0 | 1 | 0 | 0 |
| llm/safety/classifier.cpp | 1 | 0 | 1 | 0 | 0 |
| llm/speculative_decoder.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### llm/llama_wrapper.cpp
Total findings: 129

- Line 397: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool LlamaWrapper::loadModel(
- Line 404: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: transitionToState(WrapperState::LOADING, "loadModel() called for: " + model_path);
- Line 496: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* model = model_loader->getOrLoadModel(
- Line 565: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto metadata_opt = storage->loadModel(model_id);
- Line 583: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // The loadModelBlob() method in LLMModelStorage handles blob retrieval
- Line 584: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto blob_data_opt = storage->loadModelBlob(model_id);
- Line 595: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Step 3: Decryption is already handled by loadModelBlob()
- Line 596: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // The data returned from loadModelBlob() is already decrypted if encryption was enabled
- Line 627: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::error("loadModelFromThemisDB: model_id '{}' produces a path '{}' "
- Line 742: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Step 5: Load model using standard loadModel() method
- Line 745: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool load_success = loadModel(temp_model_path.string(), config);
- Line 773: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: void LlamaWrapper::unloadModel() {
- Line 795: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::warn("LlamaWrapper::unloadModel: model loader is not initialized");
- Line 798: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: model_loader->unloadModel(current_model_id_, true);
- Line 984: severity=CRITICAL; category=double_lock
  Description: Double lock without unlock (potential deadlock)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 991: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (current_model_id_ != reload_model_id && !current_model_id_.empty()) {
- Line 994: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: reload_model_id, current_model_id_);
- Line 998: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::error("Lazy reload failed for model {}", reload_model_path);
- Line 1074: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const std::string cache_key = safe_request.prompt + "|" + safe_request.model_id;
- Line 1116: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* cached = model_loader->getOrLoadModel(
- Line 1139: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "Call loadModel() with a valid model file before attempting inference. "
- Line 1164: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::info("Context changed, rebinding adapter {} to new context", adapter_id);
- Line 1409: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const std::string cache_key = request.prompt + "|" + request.model_id;
- Line 1486: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* cached = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
- Line 1577: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string formatted_prompt = formatPromptForRAG(rag_context, request);
- Line 1606: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* cached = model_loader->getOrLoadModel(
- Line 1626: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "Call loadModel() with a valid model file before computing embeddings."
- Line 2348: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: draft_model_ = llama_load_model_from_file(draft_path.c_str(), draft_params);
- Line 2496: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto* cached = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
- Line 2496: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* cached = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
- Line 2501: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: void* target_model_handle = cached->model_handle;
- Line 2502: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: void* target_context_handle = cached->context_handle;
- Line 2768: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto* cached = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
- Line 2768: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* cached = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
- Line 2773: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: void* model_handle = cached->model_handle;
- Line 2774: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: void* context_handle = cached->context_handle;
- Line 2785: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "Call loadModel() with a valid model file before attempting inference. "
- Line 3196: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cached = grammar_cache->get(grammar_key);
- Line 3290: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: prompt += request.image_token + "\n";
- Line 3295: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: prompt += "USER: " + request.text_prompt + "\nASSISTANT:";
- Line 3313: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response.error_message = "No model loaded. Call loadModel() first";
- Line 3342: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto embeddings = vision_encoder->encodeImage(img_path);
- Line 3354: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string prompt = buildVisionPrompt(vision_request);
- Line 3373: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto* cached_m = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
- Line 3373: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* cached_m = model_loader->getOrLoadModel(current_model_id_, current_model_path_);
- Line 3375: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: void* context_handle = cached_m->context_handle;
- Line 3376: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: void* model_handle = cached_m->model_handle;
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5402 W1-L04 follow-up 3: Anchor ... (2026-05-27) | #5205 fix(llm): harden Lo
- Line 285: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // ─── llama.cpp runtime version compatibility check ───────────────────────
- Line 822: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
- Line 923: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: InferenceResponse LlamaWrapper::generate(const InferenceRequest& request) {
- Line 939: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: current_model_id_.empty() ? "unknown" : current_model_id_,

                    "prompt_blocked:" + blocked_rule);

            }

            throw std::invalid_argument(

                "Inference prompt blocked by policy rule '" + blocked_rule + "': " + blocked_reason);

        }

        if (request.system_prompt) {
- Line 953: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: current_model_id_.empty() ? "unknown" : current_model_id_,

                        "system_prompt_blocked:" + blocked_rule);

                }

                throw std::invalid_argument(

                    "System prompt blocked by policy rule '" + blocked_rule + "': " + blocked_reason);

            }

            effective_request.system_prompt = std::move(sanitized_system_prompt);
- Line 956: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw std::invalid_argument(

                    "System prompt blocked by policy rule '" + blocked_rule + "': " + blocked_reason);

            }

            effective_request.system_prompt = std::move(sanitized_system_prompt);

        }

    }

    const InferenceRequest& safe_request = effective_request;
- Line 984: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: // W1-L04: Mutex lock without timeout (CWE-833) — lock.lock() can block indefinitely
- Line 988: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 1021: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    

    spdlog::debug("Generating response for prompt: {} (max_tokens={})",

                  safe_request.prompt.substr(0, std::min(safe_request.prompt.size(), size_t(50))), safe_request.max_tokens);

    

    // Check if speculative decoding is available and enabled

    if (config_.use_speculative_decoding && draft_model_ && draft_context_) {
- Line 1040: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // allowing the nested generate() call to acquire it normally.

    // W1-L04 fix: prompt injection guards — use sanitized prompt (safe_request.prompt)

    // which passed through sanitizePromptText() above; image paths validated in generateVision()

    if (!safe_request.image_paths.empty() && vision_enabled_) {

        try {

            VisionRequest vision_req;

            vision_req.text_prompt = safe_request.prompt;  // Already sanitized
- Line 1075: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto cached_response = response_cache_ptr->get(cache_key);
- Line 1108: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto start_time = std::chrono::high_resolution_clock::now();

    

    spdlog::debug("Generating response for prompt: {} (max_tokens={})",

                  request.prompt.substr(0, std::min(request.prompt.size(), size_t(50))), request.max_tokens);

    

    // Ensure model is loaded (lazy loading trigger)

    auto* const model_loader = model_loader_.get();
- Line 1154: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string prev_adapter;

        bool context_changed = (last_context_ptr_ != lctx);

        

        if (request.lora_adapter_id && !request.lora_adapter_id->empty()) {

            const std::string& adapter_id = *request.lora_adapter_id;

            spdlog::info("Auto-binding LoRA adapter: {}", adapter_id);
- Line 1234: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // 3. Evaluate prompt (populate KV cache)

        if (llama_decode(lctx, batch) != 0) {

            throw std::runtime_error("Failed to evaluate prompt");

        }

        

        // 4. Generate tokens
- Line 1315: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                    // Detokenize this single token for streaming

                    std::string token_text = detokenizeInternal(lctx, {next_token});

                    request.stream_callback(token_text);

                } catch (const std::exception& e) {

                    spdlog::warn("Streaming callback error: {}", e.what());

                    // Continue generation even if streaming fails
- Line 1334: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response.tokens_generated = static_cast<int>(generated_tokens.size());
- Line 1384: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Add validation errors to response metadata

                // We don't throw here - instead we let the caller decide how to handle

                // invalid output. This enables graceful degradation in production.

                response.metadata["validation_errors"] = validation.errors;

                response.metadata["validation_valid"] = false;

            }
- Line 1385: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // We don't throw here - instead we let the caller decide how to handle

                // invalid output. This enables graceful degradation in production.

                response.metadata["validation_errors"] = validation.errors;

                response.metadata["validation_valid"] = false;

            }

            

            // Log warnings
- Line 1394: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

            

            // Add validation metrics to response

            response.metadata["validation_metrics"] = {

                {"token_count", validation.metrics.token_count},

                {"word_count", validation.metrics.word_count},

                {"is_truncated", validation.metrics.is_truncated},
- Line 1403: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: };

            

            if (!validation.warnings.empty()) {

                response.metadata["validation_warnings"] = validation.warnings;

            }

        }
- Line 1520: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    const int32_t n_vocab = llama_vocab_n_tokens(vocab);

    if (n_vocab <= 0) {

        throw std::runtime_error("Model returned non-positive vocabulary size for draft generation");

    }

    const llama_token eos_token = llama_vocab_eos(vocab);

    const size_t produced_vocab_size = static_cast<size_t>(n_vocab);
- Line 1541: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: for (size_t i = 0; i < k; ++i) {

        float* logits_ptr = llama_get_logits_ith(lctx, -1);

        if (!logits_ptr) {

            throw std::runtime_error("llama_get_logits_ith returned null");

        }



        std::vector<float> logit_row(produced_vocab_size, 0.0f);
- Line 1546: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<float> logit_row(produced_vocab_size, 0.0f);

        for (size_t j = 0; j < produced_vocab_size; ++j) {

            logit_row[j] = logits_ptr[j];

        }



        const llama_token next_token = sampleTokenInternal(
- Line 1558: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: llama_token next_token_batch = next_token;

        llama_batch next_batch = llama_batch_get_one(&next_token_batch, 1);

        if (llama_decode(lctx, next_batch) != 0) {

            throw std::runtime_error("Failed to decode draft token");

        }



        if (next_token == eos_token) {
- Line 1583: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = generate(rag_request);
- Line 1583: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = generate(rag_request);
- Line 1586: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: auto response = generate(rag_request);



    // Add RAG metadata to response

    response.metadata["rag_enabled"] = true;

    response.metadata["num_documents"] = rag_context.documents.size();

    

    return response;
- Line 1587: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Add RAG metadata to response

    response.metadata["rag_enabled"] = true;

    response.metadata["num_documents"] = rag_context.documents.size();

    

    return response;

}
- Line 2144: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (temperature > 0.0f && temperature != 1.0f) {
- Line 2179: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '        // Truncate to top-p', '        candidates_p.size = last_idx + 1;', '    }', '']
- Line 2561: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // 2. Evaluate prompt in both models

        llama_batch batch = llama_batch_get_one(prompt_tokens.data(), static_cast<int32_t>(prompt_tokens.size()));

        if (llama_decode(target_context, batch) != 0) {

            throw std::runtime_error("Failed to evaluate prompt in target model");

        }

        if (llama_decode(draft_context_, batch) != 0) {

            throw std::runtime_error("Failed to evaluate prompt in draft model");
- Line 2564: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw std::runtime_error("Failed to evaluate prompt in target model");

        }

        if (llama_decode(draft_context_, batch) != 0) {

            throw std::runtime_error("Failed to evaluate prompt in draft model");

        }

        

        // 3. Speculative generation loop
- Line 2571: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::vector<llama_token> generated_tokens;

        const llama_vocab* vocab = llama_model_get_vocab(target_model);

        if (!vocab) {

            throw std::runtime_error("Failed to get target model vocabulary");

        }

        int32_t n_vocab = llama_vocab_n_tokens(vocab);

        if (n_vocab <= 0) {
- Line 2575: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        int32_t n_vocab = llama_vocab_n_tokens(vocab);

        if (n_vocab <= 0) {

            throw std::runtime_error("Target model returned non-positive vocabulary size");

        }

        llama_token eos_token = llama_vocab_eos(vocab);
- Line 2684: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (request.stream_callback) {

                        try {

                            std::string token_text = detokenizeInternal(target_context, {corrected_token});

                            request.stream_callback(token_text);

                        } catch (const std::exception& e) {

                            spdlog::warn("Streaming callback error: {}", e.what());

                        }
- Line 2716: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response.tokens_generated = static_cast<int>(generated_tokens.size());
- Line 2730: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: : 0.0f;

        

        // Add speculative decoding metadata

        response.metadata["speculative_decoding"] = true;

        response.metadata["acceptance_rate"] = (total_speculations > 0) 

            ? static_cast<double>(total_accepted) / total_speculations : 0.0;

        response.metadata["draft_model"] = draft_model_id_;
- Line 2731: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Add speculative decoding metadata

        response.metadata["speculative_decoding"] = true;

        response.metadata["acceptance_rate"] = (total_speculations > 0) 

            ? static_cast<double>(total_accepted) / total_speculations : 0.0;

        response.metadata["draft_model"] = draft_model_id_;
- Line 2733: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: response.metadata["speculative_decoding"] = true;

        response.metadata["acceptance_rate"] = (total_speculations > 0) 

            ? static_cast<double>(total_accepted) / total_speculations : 0.0;

        response.metadata["draft_model"] = draft_model_id_;

        

        updateStatistics(response);
- Line 2837: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // when called multiple times (e.g., consecutive RAG queries).

        // Validate lctx is still valid before attempting to access it

        if (!lctx) {

            throw std::runtime_error("Context handle became null before inference");

        }

        llama_memory_t mem = llama_get_memory(lctx);

        if (mem) {
- Line 2847: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: llama_batch batch = llama_batch_get_one(prompt_tokens.data(), static_cast<int32_t>(prompt_tokens.size()));

        

        if (llama_decode(lctx, batch) != 0) {

            throw std::runtime_error("Failed to evaluate prompt");

        }

        

        std::vector<llama_token> generated_tokens;
- Line 2862: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const llama_vocab* vocab = llama_model_get_vocab(lmodel);

        if (!vocab) {

            throw std::runtime_error("Failed to get model vocabulary");

        }

        int32_t n_vocab = llama_vocab_n_tokens(vocab);

        if (n_vocab <= 0) {
- Line 2866: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        int32_t n_vocab = llama_vocab_n_tokens(vocab);

        if (n_vocab <= 0) {

            throw std::runtime_error("Model returned non-positive vocabulary size");

        }

        llama_token eos_token = llama_vocab_eos(vocab);
- Line 2897: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (request.stream_callback) {

                try {

                    std::string token_text = detokenizeInternal(lctx, {next_token});

                    request.stream_callback(token_text);

                } catch (const std::exception& e) {

                    spdlog::warn("Streaming callback error: {}", e.what());

                }
- Line 2911: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response.tokens_generated = static_cast<int>(generated_tokens.size());
- Line 3043: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::lock_guard<std::mutex> lock(mutex_);

    

    if (!batch_mode_active_) {

        throw std::runtime_error("Batch mode not active. Call startBatchMode() first.");

    }

    

    auto* const batch_scheduler = batch_scheduler_.get();
- Line 3118: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::shared_ptr<Grammar> LlamaWrapper::getOrCreateGrammar(const InferenceRequest& request) {

    // Check if any grammar source is requested

    const bool has_explicit_grammar =

        request.grammar_type.has_value() || request.grammar_ebnf.has_value();

    const bool has_schema_binding =

        request.json_schema.has_value() || !request.tools.empty();
- Line 3120: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const bool has_explicit_grammar =

        request.grammar_type.has_value() || request.grammar_ebnf.has_value();

    const bool has_schema_binding =

        request.json_schema.has_value() || !request.tools.empty();



    if (!has_explicit_grammar && !has_schema_binding) {

        return nullptr;
- Line 3135: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string ebnf_text;

    

    // Custom EBNF grammar takes precedence

    if (request.grammar_ebnf.has_value()) {

        ebnf_text = request.grammar_ebnf.value();

        // Use hash with length to reduce collision risk

        // In production, consider SHA256 or storing full text as key
- Line 3136: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Custom EBNF grammar takes precedence

    if (request.grammar_ebnf.has_value()) {

        ebnf_text = request.grammar_ebnf.value();

        // Use hash with length to reduce collision risk

        // In production, consider SHA256 or storing full text as key

        size_t hash = std::hash<std::string>{}(ebnf_text);
- Line 3144: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: grammar_key = "custom_" + std::to_string(hash) + "_" + std::to_string(len);

    }

    // Built-in grammar

    else if (request.grammar_type.has_value()) {

        std::string grammar_name = request.grammar_type.value();

        grammar_key = grammar_name;
- Line 3145: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    // Built-in grammar

    else if (request.grammar_type.has_value()) {

        std::string grammar_name = request.grammar_type.value();

        grammar_key = grammar_name;

        

        // Check if built-in grammar exists
- Line 3158: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    }

    // JSON schema binding: convert schema to EBNF (Issue #1922)

    else if (request.json_schema.has_value()) {

        ebnf_text = JsonSchemaConverter::schemaToEbnf(request.json_schema.value());

        if (ebnf_text.empty()) {

            spdlog::warn("getOrCreateGrammar: json_schema conversion produced empty EBNF, "
- Line 3159: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    // JSON schema binding: convert schema to EBNF (Issue #1922)

    else if (request.json_schema.has_value()) {

        ebnf_text = JsonSchemaConverter::schemaToEbnf(request.json_schema.value());

        if (ebnf_text.empty()) {

            spdlog::warn("getOrCreateGrammar: json_schema conversion produced empty EBNF, "

                         "falling back to built-in json grammar");
- Line 3177: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    }

    // Tool calling: generate tool call grammar (Issue #1922)

    else if (!request.tools.empty()) {

        ebnf_text = JsonSchemaConverter::toolsToEbnf(request.tools);

        if (ebnf_text.empty()) {

            spdlog::warn("getOrCreateGrammar: toolsToEbnf produced empty EBNF");
- Line 3203: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3322: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

        // Collect image paths

        std::vector<std::string> image_paths;

        if (!vision_request.image_path.empty()) {

            image_paths.push_back(vision_request.image_path);

        } else if (!vision_request.image_paths.empty()) {

            image_paths = vision_request.image_paths;
- Line 3324: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::vector<std::string> image_paths;

        if (!vision_request.image_path.empty()) {

            image_paths.push_back(vision_request.image_path);

        } else if (!vision_request.image_paths.empty()) {

            image_paths = vision_request.image_paths;

        } else {

            throw std::runtime_error("No images provided in vision request");
- Line 3448: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto inference_response = generate(inference_request);
- Line 3448: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto inference_response = generate(inference_request);
- Line 3454: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response.tokens_generated = inference_response.tokens_generated;
- Line 3502: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3507: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3517: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 40: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void llama_lora_adapter_free(void* adapter);
- Line 55: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 62: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool llava_eval_image_embed(
- Line 70: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void llava_image_embed_free(struct llava_image_embed* embed);
- Line 923: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: InferenceResponse LlamaWrapper::generate(const InferenceRequest& request) {
- Line 1583: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = generate(rag_request);
- Line 1592: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<float> LlamaWrapper::embed(const std::string& text) {
- Line 1923: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "<s>[INST] <<SYS>>\n" << system_msg << "\n<</SYS>>\n\n";
- Line 2412: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: llama_free(draft_context_);
- Line 2759: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // This is the existing generate() implementation extracted
- Line 3038: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3289: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: prompt += request.image_token + "\n";
- Line 3323: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: image_paths.push_back(vision_request.image_path);
- Line 3415: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (!llava_eval_image_embed(lctx, &embed_data, n_batch_size, &n_past)) {
- Line 3426: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // We use the raw generate() path which re-evaluates the full prompt,
- Line 3448: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto inference_response = generate(inference_request);
- Line 1034: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Route to vision pipeline when image inputs are provided.

### llm/vision_config.cpp
Total findings: 111

- Line 118: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->resource_limits_.max_queue_size = limits["max_queue_size"].as<size_t>(100);
- Line 154: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->resource_quota_.reset_period = quotas["reset_period"].as<std::string>("monthly");
- Line 160: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.enabled = monitoring["enabled"].as<bool>(true);
- Line 164: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.track_latency = metrics["track_latency"].as<bool>(true);
- Line 165: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.track_throughput = metrics["track_throughput"].as<bool>(true);
- Line 166: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.track_error_rate = metrics["track_error_rate"].as<bool>(true);
- Line 167: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.track_resource_usage = metrics["track_resource_usage"].as<bool>(tr
- Line 168: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.track_model_usage = metrics["track_model_usage"].as<bool>(true);
- Line 169: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.collect_interval = std::chrono::seconds(metrics["collect_interval_
- Line 173: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.prometheus.enabled = prometheus["enabled"].as<bool>(true);
- Line 174: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.prometheus.port = prometheus["port"].as<int>(9092);
- Line 175: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.prometheus.path = prometheus["path"].as<std::string>("/metrics");
- Line 176: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.prometheus.namespace_prefix = prometheus["namespace"].as<std::stri
- Line 182: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.audit.enabled = audit["enabled"].as<bool>(true);
- Line 192: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.audit.storage_type = storage["type"].as<std::string>("database");
- Line 193: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.audit.retention_days = storage["retention_days"].as<int>(90);
- Line 196: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.audit.compliance_mode = audit["compliance_mode"].as<std::string>("
- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.audit.include_pii = audit["include_pii"].as<bool>(false);
- Line 208: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.validation.enabled = validation["enabled"].as<bool>(true);
- Line 209: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.validation.max_image_size_mb = validation["max_image_size_mb"].as<si
- Line 213: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.validation.max_image_resolution = {
- Line 225: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.validation.validate_image_integrity = validation["validate_image_int
- Line 226: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.validation.scan_for_malware = validation["scan_for_malware"].as<bool
- Line 227: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.validation.max_prompt_length = validation["max_prompt_length"].as<si
- Line 228: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.validation.sanitize_prompts = validation["sanitize_prompts"].as<bool
- Line 229: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.validation.block_injection_attempts = validation["block_injection_at
- Line 235: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.enabled = sandboxing["enabled"].as<bool>(false);
- Line 236: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.type = sandboxing["type"].as<std::string>("container");
- Line 237: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.isolate_memory = sandboxing["isolate_memory"].as<bool>(tr
- Line 238: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.isolate_network = sandboxing["isolate_network"].as<bool>(
- Line 239: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.isolate_filesystem = sandboxing["isolate_filesystem"].as<
- Line 240: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.allow_file_read = sandboxing["allow_file_read"].as<bool>(
- Line 241: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.allow_file_write = sandboxing["allow_file_write"].as<bool
- Line 242: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.allow_network = sandboxing["allow_network"].as<bool>(fals
- Line 243: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.sandbox_memory_mb = sandboxing["sandbox_memory_mb"].as<si
- Line 244: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.sandbox_cpu_cores = sandboxing["sandbox_cpu_cores"].as<in
- Line 245: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.sandboxing.sandbox_timeout = std::chrono::seconds(sandboxing["sandbo
- Line 251: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.model_verification.enabled = model_verification["enabled"].as<bool>(
- Line 252: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.model_verification.verify_signatures = model_verification["verify_si
- Line 260: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.model_verification.verify_checksums = model_verification["verify_che
- Line 261: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.model_verification.checksum_algorithm = model_verification["checksum
- Line 262: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.model_verification.scan_models = model_verification["scan_models"].a
- Line 268: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.access_control.enabled = access_control["enabled"].as<bool>(true);
- Line 269: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.access_control.require_authentication = access_control["require_auth
- Line 270: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.access_control.role_based_access = access_control["role_based_access
- Line 278: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.access_control.require_api_key = access_control["require_api_key"].a
- Line 279: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->security_config_.access_control.api_key_header = access_control["api_key_header"].as<
- Line 286: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.stability = pipeline["stability"].as<std::string>("production");
- Line 291: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.error_handling.strategy = error_handling["strategy"].as<std::string>
- Line 295: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.error_handling.retry_enabled = retry["enabled"].as<bool>(true);
- Line 296: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.error_handling.max_retry_attempts = retry["max_attempts"].as<int>(3)
- Line 297: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.error_handling.backoff_strategy = retry["backoff_strategy"].as<std::
- Line 298: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.error_handling.initial_delay = std::chrono::milliseconds(retry["init
- Line 299: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.error_handling.max_delay = std::chrono::milliseconds(retry["max_dela
- Line 304: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.error_handling.use_cpu_fallback = fallback["use_cpu_fallback"].as<bo
- Line 305: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.error_handling.use_smaller_model = fallback["use_smaller_model"].as<
- Line 306: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.error_handling.return_error_response = fallback["return_error_respon
- Line 313: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.preprocessing.enabled = preprocessing["enabled"].as<bool>(true);
- Line 314: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.preprocessing.resize_strategy = preprocessing["resize_strategy"].as<
- Line 315: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.preprocessing.normalize = preprocessing["normalize"].as<bool>(true);
- Line 316: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.preprocessing.augmentation = preprocessing["augmentation"].as<bool>(
- Line 317: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.preprocessing.cache_preprocessed = preprocessing["cache_preprocessed
- Line 318: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.preprocessing.cache_ttl = std::chrono::seconds(preprocessing["cache_
- Line 324: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.postprocessing.enabled = postprocessing["enabled"].as<bool>(true);
- Line 325: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.postprocessing.format = postprocessing["format"].as<std::string>("js
- Line 326: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.postprocessing.include_metadata = postprocessing["include_metadata"]
- Line 327: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.postprocessing.include_timings = postprocessing["include_timings"].a
- Line 328: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.postprocessing.include_confidence_scores = postprocessing["include_c
- Line 329: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.postprocessing.min_confidence_threshold = postprocessing["min_confid
- Line 330: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->pipeline_config_.postprocessing.max_results = postprocessing["max_results"].as<int>(1
- Line 470: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vision_config->monitoring_config_.audit.retention_days =
- Line 554: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->resource_limits_.max_queue_size = 100;
- Line 568: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->monitoring_config_.enabled = true;
- Line 569: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->monitoring_config_.track_latency = true;
- Line 570: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->monitoring_config_.track_throughput = true;
- Line 571: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->monitoring_config_.track_error_rate = true;
- Line 572: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->monitoring_config_.track_resource_usage = true;
- Line 573: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->monitoring_config_.track_model_usage = true;
- Line 576: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->security_config_.validation.enabled = true;
- Line 577: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->security_config_.validation.max_image_size_mb = 25;
- Line 578: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->security_config_.validation.max_image_resolution = {4096, 4096};
- Line 579: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->security_config_.validation.allowed_formats = {"JPEG", "PNG", "BMP", "WEBP"};
- Line 580: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: config->security_config_.sandboxing.enabled = false;
- Line 24: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Compatibility matrix - can be expanded
- Line 75: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto api = config["vision"]["api"];
- Line 99: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto licensing = config["vision"]["licensing"];
- Line 111: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto limits = config["vision"]["resources"]["limits"];
- Line 126: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto rate_limiting = config["vision"]["resources"]["rate_limiting"];
- Line 130: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto global = rate_limiting["global"];
- Line 142: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto quotas = config["vision"]["resources"]["quotas"];
- Line 147: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto defaults = quotas["default"];
- Line 159: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto monitoring = config["vision"]["monitoring"];
- Line 163: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto metrics = monitoring["metrics"];
- Line 172: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto prometheus = metrics["prometheus"];
- Line 181: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto audit = monitoring["audit"];
- Line 191: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto storage = audit["storage"];
- Line 203: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto security = config["vision"]["security"];
- Line 207: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto validation = security["validation"];
- Line 212: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto res = validation["max_image_resolution"];
- Line 234: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto sandboxing = security["sandboxing"];
- Line 250: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto model_verification = security["model_verification"];
- Line 267: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto access_control = security["access_control"];
- Line 285: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto pipeline = config["vision"]["pipeline"];
- Line 290: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto error_handling = pipeline["error_handling"];
- Line 294: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto retry = error_handling["retry"];
- Line 303: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto fallback = error_handling["fallback"];
- Line 312: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto preprocessing = pipeline["preprocessing"];
- Line 323: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto postprocessing = pipeline["postprocessing"];
- Line 336: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto features = config["vision"]["features"];
- Line 343: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto experimental = features["experimental"];
- Line 654: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: model_ids.push_back(pair.first);

### llm/async_inference_engine.cpp
Total findings: 94

- Line 673: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Use condition variable for efficient waiting instead of polling with sleep

    std::unique_lock<std::mutex> lock(queue_mutex_);

    queue_cv_.wait(lock, [this] { return request_queue_.empty(); });

    

    spdlog::info("All inference requests completed");

}
- Line 673: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: queue_cv_.wait(lock, [this] { return request_queue_.empty(); });
- Line 692: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: worker.join();
- Line 719: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

            std::unique_lock<std::mutex> lock(queue_mutex_);

            

            queue_cv_.wait(lock, [this] {

                return !request_queue_.empty() || !running_.load();

            });
- Line 719: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: queue_cv_.wait(lock, [this] {
- Line 871: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cached->metadata["async"] = true;
- Line 872: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cached->metadata["queue_time_ms"] = queue_time;
- Line 873: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cached->metadata["request_id"] = request.request_id;
- Line 874: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cached->metadata["priority"] = request.priority;
- Line 881: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Take a local snapshot of the policy pointer so that a concurrent
- Line 907: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // concurrent swapPlugin() call does not race with the generate() invocation.
- Line 1000: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: switch (config_.backpressure) {

        case Config::BackpressurePolicy::BLOCK:

            // Wait for space (releases lock while waiting)

            queue_cv_.wait(lock, [this] {

                return request_queue_.size() < config_.max_queue_size ||

                       !running_.load();

            });
- Line 1000: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: queue_cv_.wait(lock, [this] {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4332 Implement AIOrchestrator to... (2026-03-19) | #3284 [llm] Implement pro
- Line 48: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
- Line 64: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ) : config_(config), plugin_(plugin.get()), owned_plugin_(std::move(plugin)) {
- Line 77: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
- Line 109: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
- Line 124: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: ) : config_(config), plugin_(plugin.get()), owned_plugin_(std::move(plugin)),
- Line 135: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: timeout_thread_ = std::thread(&AsyncInferenceEngine::timeoutMonitorLoop, this);
- Line 159: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info(

        "AsyncInferenceEngine::submit start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",

        request.model_id.empty() ? std::string{"default"} : request.model_id,

        request.prompt.size(),

        priority,

        timeout.count(),
- Line 160: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info(

        "AsyncInferenceEngine::submit start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",

        request.model_id.empty() ? std::string{"default"} : request.model_id,

        request.prompt.size(),

        priority,

        timeout.count(),

        (shared_pool_ != nullptr));
- Line 193: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (async_req->cancel_token->load(std::memory_order_acquire)) {
- Line 231: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 239: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: stats_.total_rejected++;

                std::lock_guard<std::mutex> tl(tracking_mutex_);

                active_requests_.erase(async_req->request_id);

                throw std::runtime_error("Request queue full, request rejected");

            }

        }
- Line 274: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info(

        "AsyncInferenceEngine::submitAsync start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",

        request.model_id.empty() ? std::string{"default"} : request.model_id,

        request.prompt.size(),

        priority,

        timeout.count(),
- Line 275: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info(

        "AsyncInferenceEngine::submitAsync start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",

        request.model_id.empty() ? std::string{"default"} : request.model_id,

        request.prompt.size(),

        priority,

        timeout.count(),

        (shared_pool_ != nullptr));
- Line 302: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (async_req->cancel_token->load(std::memory_order_acquire)) {
- Line 327: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: stats_.total_rejected++;

            std::lock_guard<std::mutex> lock(tracking_mutex_);

            active_requests_.erase(async_req->request_id);

            throw std::runtime_error("SharedWorkerPool queue full");

        }

        stats_.total_submitted++;

    } else {
- Line 335: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 342: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: stats_.total_rejected++;

                std::lock_guard<std::mutex> tl(tracking_mutex_);

                active_requests_.erase(async_req->request_id);

                throw std::runtime_error("Request queue full");

            }

        }
- Line 377: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info(

        "AsyncInferenceEngine::submitStreaming start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",

        request.model_id.empty() ? std::string{"default"} : request.model_id,

        request.prompt.size(),

        priority,

        timeout.count(),
- Line 378: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info(

        "AsyncInferenceEngine::submitStreaming start: model='{}' prompt_len={} priority={} timeout_ms={} via_pool={}",

        request.model_id.empty() ? std::string{"default"} : request.model_id,

        request.prompt.size(),

        priority,

        timeout.count(),

        (shared_pool_ != nullptr));
- Line 402: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (cancel_token->load(std::memory_order_acquire)) {
- Line 440: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (async_req->cancel_token->load(std::memory_order_acquire)) {
- Line 478: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 484: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: stats_.total_rejected++;

                std::lock_guard<std::mutex> tl(tracking_mutex_);

                active_requests_.erase(async_req->request_id);

                throw std::runtime_error("Request queue full, streaming request rejected");

            }

        }
- Line 531: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: rag_priority);

    

    // Store RAG context in metadata for worker to use

    rag_request.metadata["rag_enabled"] = true;

    rag_request.metadata["num_documents"] = rag_context.documents.size();



    // Build structured prompt: use context_template when provided, otherwise
- Line 532: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Store RAG context in metadata for worker to use

    rag_request.metadata["rag_enabled"] = true;

    rag_request.metadata["num_documents"] = rag_context.documents.size();



    // Build structured prompt: use context_template when provided, otherwise

    // fall back to XML-tag format that most instruction-tuned models handle well.
- Line 532: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Store RAG context in metadata for worker to use

    rag_request.metadata["rag_enabled"] = true;

    rag_request.metadata["num_documents"] = rag_context.documents.size();



    // Build structured prompt: use context_template when provided, otherwise

    // fall back to XML-tag format that most instruction-tuned models handle well.
- Line 586: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: oss << tmpl;

    }



    rag_request.prompt = oss.str();

    

    auto handle = submit(rag_request, rag_priority);

    spdlog::info(
- Line 717: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 740: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (item.request->cancel_token->load(std::memory_order_acquire)) {
- Line 841: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info(

            "AsyncInferenceEngine::processRequest streaming start: request_id={} prompt_len={} priority={}",

            request.request_id,

            effective_request.prompt.size(),

            request.priority);

    }
- Line 866: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: auto cached = dedup_cache_->get(effective_request.prompt);
- Line 871: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: stats_.total_dedup_cache_hits.fetch_add(1, std::memory_order_relaxed);

            spdlog::debug("Dedup cache hit for request {}", request.request_id);

            cached->cache_hit = true;

            cached->metadata["async"] = true;

            cached->metadata["queue_time_ms"] = queue_time;

            cached->metadata["request_id"] = request.request_id;

            cached->metadata["priority"] = request.priority;
- Line 872: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: spdlog::debug("Dedup cache hit for request {}", request.request_id);

            cached->cache_hit = true;

            cached->metadata["async"] = true;

            cached->metadata["queue_time_ms"] = queue_time;

            cached->metadata["request_id"] = request.request_id;

            cached->metadata["priority"] = request.priority;

            return *cached;
- Line 873: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: cached->cache_hit = true;

            cached->metadata["async"] = true;

            cached->metadata["queue_time_ms"] = queue_time;

            cached->metadata["request_id"] = request.request_id;

            cached->metadata["priority"] = request.priority;

            return *cached;

        }
- Line 874: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: cached->metadata["async"] = true;

            cached->metadata["queue_time_ms"] = queue_time;

            cached->metadata["request_id"] = request.request_id;

            cached->metadata["priority"] = request.priority;

            return *cached;

        }

        stats_.total_dedup_cache_misses.fetch_add(1, std::memory_order_relaxed);
- Line 894: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: request.request_id, policy_result.rule_name);

            InferenceResponse blocked;

            blocked.request_id = request.request_id;

            blocked.metadata["async"] = true;

            blocked.metadata["blocked"] = true;

            blocked.metadata["blocked_rule"] = policy_result.rule_name;

            blocked.metadata["blocked_reason"] = policy_result.reason;
- Line 895: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: InferenceResponse blocked;

            blocked.request_id = request.request_id;

            blocked.metadata["async"] = true;

            blocked.metadata["blocked"] = true;

            blocked.metadata["blocked_rule"] = policy_result.rule_name;

            blocked.metadata["blocked_reason"] = policy_result.reason;

            blocked.metadata["queue_time_ms"] = queue_time;
- Line 896: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: blocked.request_id = request.request_id;

            blocked.metadata["async"] = true;

            blocked.metadata["blocked"] = true;

            blocked.metadata["blocked_rule"] = policy_result.rule_name;

            blocked.metadata["blocked_reason"] = policy_result.reason;

            blocked.metadata["queue_time_ms"] = queue_time;

            blocked.metadata["request_id"] = request.request_id;
- Line 897: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: blocked.metadata["async"] = true;

            blocked.metadata["blocked"] = true;

            blocked.metadata["blocked_rule"] = policy_result.rule_name;

            blocked.metadata["blocked_reason"] = policy_result.reason;

            blocked.metadata["queue_time_ms"] = queue_time;

            blocked.metadata["request_id"] = request.request_id;

            return blocked;
- Line 898: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: blocked.metadata["blocked"] = true;

            blocked.metadata["blocked_rule"] = policy_result.rule_name;

            blocked.metadata["blocked_reason"] = policy_result.reason;

            blocked.metadata["queue_time_ms"] = queue_time;

            blocked.metadata["request_id"] = request.request_id;

            return blocked;

        }
- Line 899: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: blocked.metadata["blocked_rule"] = policy_result.rule_name;

            blocked.metadata["blocked_reason"] = policy_result.reason;

            blocked.metadata["queue_time_ms"] = queue_time;

            blocked.metadata["request_id"] = request.request_id;

            return blocked;

        }

        // Apply any redactions to the effective prompt
- Line 915: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: InferenceResponse response = plugin_snapshot->generate(effective_request);
- Line 915: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: InferenceResponse response = plugin_snapshot->generate(effective_request);
- Line 919: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "AsyncInferenceEngine::processRequest streaming complete: request_id={} tokens_generated={} inference_time_ms={:.2f}",
- Line 931: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    // Add metadata

    response.metadata["async"] = true;

    response.metadata["queue_time_ms"] = queue_time;

    response.metadata["request_id"] = request.request_id;

    response.metadata["priority"] = request.priority;
- Line 932: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Add metadata

    response.metadata["async"] = true;

    response.metadata["queue_time_ms"] = queue_time;

    response.metadata["request_id"] = request.request_id;

    response.metadata["priority"] = request.priority;
- Line 965: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 969: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 970: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: plugin_ = owned_plugin_.get();
- Line 1072: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 1094: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: // Resolve the future immediately so handle.get() unblocks without
- Line 208: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (async_req->callback) {

                        async_req->callback(response);

                    }

                    try { promise->set_value(response); } catch (...) { /* Promise already satisfied; ignore. */ }

                } catch (...) {

                    try { promise->set_exception(std::current_exception()); } catch (...) { /* Promise already satisfied; ignore. */ }

                }
- Line 208: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { promise->set_value(response); } catch (...) { /* Promise already satisfied; ignore. */ }
- Line 209: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: async_req->callback(response);

                    }

                    try { promise->set_value(response); } catch (...) { /* Promise already satisfied; ignore. */ }

                } catch (...) {

                    try { promise->set_exception(std::current_exception()); } catch (...) { /* Promise already satisfied; ignore. */ }

                }

                std::lock_guard<std::mutex> lock(tracking_mutex_);
- Line 209: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 210: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                    try { promise->set_value(response); } catch (...) { /* Promise already satisfied; ignore. */ }

                } catch (...) {

                    try { promise->set_exception(std::current_exception()); } catch (...) { /* Promise already satisfied; ignore. */ }

                }

                std::lock_guard<std::mutex> lock(tracking_mutex_);

                active_requests_.erase(async_req->request_id);
- Line 210: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { promise->set_exception(std::current_exception()); } catch (...) { /* Promise already satisfied
- Line 266: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 404: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = false;
- Line 418: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = false;
- Line 456: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto response = processRequest(*async_req, submit_time);

                    stats_.total_completed++;

                    if (async_req->callback) async_req->callback(response);

                    try { promise->set_value(response); } catch (...) {}

                } catch (...) {

                    try { promise->set_exception(std::current_exception()); } catch (...) {}

                }
- Line 456: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { promise->set_value(response); } catch (...) {}
- Line 457: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: stats_.total_completed++;

                    if (async_req->callback) async_req->callback(response);

                    try { promise->set_value(response); } catch (...) {}

                } catch (...) {

                    try { promise->set_exception(std::current_exception()); } catch (...) {}

                }

                std::lock_guard<std::mutex> lock(tracking_mutex_);
- Line 457: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 458: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (async_req->callback) async_req->callback(response);

                    try { promise->set_value(response); } catch (...) {}

                } catch (...) {

                    try { promise->set_exception(std::current_exception()); } catch (...) {}

                }

                std::lock_guard<std::mutex> lock(tracking_mutex_);

                active_requests_.erase(async_req->request_id);
- Line 458: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { promise->set_exception(std::current_exception()); } catch (...) {}
- Line 544: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "</system>\n\n";
- Line 555: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << ">\n" << doc.content << "\n</document>\n";
- Line 557: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "</context>\n\n";
- Line 558: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "<question>" << rag_context.query << "</question>\n\n";
- Line 758: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: item.promise->set_exception(

                        std::make_exception_ptr(std::runtime_error("Request cancelled"))

                    );

                } catch (...) { /* Promise already satisfied; ignore. */ }

            }

            

            // Remove from tracking
- Line 758: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) { /* Promise already satisfied; ignore. */ }
- Line 804: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Set exception in promise — guard against double-resolve.

            if (item.promise) {

                try { item.promise->set_exception(std::current_exception()); } catch (...) { /* Promise already satisfied; ignore. */ }

            }

        }
- Line 804: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { item.promise->set_exception(std::current_exception()); } catch (...) { /* Promise already sati
- Line 907: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // concurrent swapPlugin() call does not race with the generate() invocation.
- Line 915: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: InferenceResponse response = plugin_snapshot->generate(effective_request);
- Line 1031: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::make_exception_ptr(

                                std::runtime_error("Request dropped: queue full")));

                    }

                } catch (...) {

                    // Promise may already be satisfied; ignore.

                }
- Line 1031: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1101: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: req->shared_promise->set_exception(

                        std::make_exception_ptr(

                            std::runtime_error("Request timed out")));

                } catch (...) {

                    // Promise already satisfied; ignore.

                }

            }
- Line 1101: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### llm/inference_engine_enhanced.cpp
Total findings: 89

- Line 31: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != cfg_json.end() && it->is_number_unsigned()) {
- Line 33: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: } else if (it != cfg_json.end() && it->is_number_integer()) {
- Line 43: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != cfg_json.end() && it->is_number()) {
- Line 857: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread.join();
- Line 1265: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: self_rag.query = self_rag_it->value("query", effective_request.prompt);
- Line 1315: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (max_context_tokens_it != self_rag_it->end() &&
- Line 1322: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (response_budget_tokens_it != self_rag_it->end() &&
- Line 1396: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const auto& prompt = effective_request.prompt;
- Line 1594: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cached = cache->get(request.prompt, embedding);
- Line 1604: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response.text = cached->generated_text;
- Line 1605: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: response.tokens_prompt = static_cast<int>(cached->token_ids.size());
- Line 247: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 278: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 286: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 402: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // (see llm_plugin_interface.h) — populate all three for compatibility.
- Line 453: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 496: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 503: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::lock_guard<std::mutex> stats_lock(stats_mutex_);

                stats_.rejected_requests++;

            }

            throw std::runtime_error("Request queue full");

        }

        

        request_queue_.push(tracked);
- Line 544: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (cancel_token->load(std::memory_order_acquire)) {
- Line 569: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 576: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::lock_guard<std::mutex> stats_lock(stats_mutex_);

                stats_.rejected_requests++;

            }

            throw std::runtime_error("Request queue full");

        }



        request_queue_.push(tracked);
- Line 626: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 636: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 885: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 885: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 908: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> stats_lock(stats_mutex_);
- Line 932: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 989: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 1012: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& id : timed_out) {
- Line 1015: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(requests_mutex_);
- Line 1057: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (tracked->cancel_token->load(std::memory_order_acquire)) {
- Line 1072: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(requests_mutex_);
- Line 1126: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: });

            

            if (!plugin) {

                throw std::runtime_error("No available model for request");

            }



            // Validate requested LoRA adapter (if any) is registered.
- Line 1133: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // The adapter was pre-loaded on the plugin via loadLoRAAdapter(); if

            // it is missing here the request will still execute — the plugin will

            // use its default behaviour when lora_adapter_id is set but unknown.

            if (req.base_request.lora_adapter_id.has_value() &&

                !req.base_request.lora_adapter_id->empty()) {

                const auto& aid = *req.base_request.lora_adapter_id;

                std::lock_guard<std::mutex> lock(lora_adapters_mutex_);
- Line 1134: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // it is missing here the request will still execute — the plugin will

            // use its default behaviour when lora_adapter_id is set but unknown.

            if (req.base_request.lora_adapter_id.has_value() &&

                !req.base_request.lora_adapter_id->empty()) {

                const auto& aid = *req.base_request.lora_adapter_id;

                std::lock_guard<std::mutex> lock(lora_adapters_mutex_);

                if (lora_adapters_.find(aid) == lora_adapters_.end()) {
- Line 1147: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Build an effective request that wraps the stream_callback so

            // cancellation is propagated at every token boundary.

            InferenceRequest effective_request = req.base_request;

            auto raid_sharding = effective_request.metadata.value("raid_sharding", json::object());

            if (!req.shard_routing_key.empty()) {

                raid_sharding["routing_key"] = req.shard_routing_key;

            }
- Line 1157: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Keep this boolean always present so downstream coordinators can

            // distinguish "explicitly disabled" from "field omitted".

            raid_sharding["allow_cross_instance_batching"] = req.allow_cross_instance_batching;

            effective_request.metadata["raid_sharding"] = std::move(raid_sharding);

            auto cancel_token = tracked->cancel_token;

            auto deadline = tracked->deadline;

            if (effective_request.stream_callback) {
- Line 1157: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Keep this boolean always present so downstream coordinators can

            // distinguish "explicitly disabled" from "field omitted".

            raid_sharding["allow_cross_instance_batching"] = req.allow_cross_instance_batching;

            effective_request.metadata["raid_sharding"] = std::move(raid_sharding);

            auto cancel_token = tracked->cancel_token;

            auto deadline = tracked->deadline;

            if (effective_request.stream_callback) {
- Line 1191: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: bool used_speculative = false;



            const bool grammar_active =

                req.base_request.grammar_type.has_value() ||

                req.base_request.grammar_ebnf.has_value() ||

                req.base_request.json_schema.has_value() ||

                !req.base_request.tools.empty();
- Line 1192: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const bool grammar_active =

                req.base_request.grammar_type.has_value() ||

                req.base_request.grammar_ebnf.has_value() ||

                req.base_request.json_schema.has_value() ||

                !req.base_request.tools.empty();
- Line 1193: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const bool grammar_active =

                req.base_request.grammar_type.has_value() ||

                req.base_request.grammar_ebnf.has_value() ||

                req.base_request.json_schema.has_value() ||

                !req.base_request.tools.empty();



            // ── RAID fan-out: delegate to federated backend when requested ──
- Line 1194: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: req.base_request.grammar_type.has_value() ||

                req.base_request.grammar_ebnf.has_value() ||

                req.base_request.json_schema.has_value() ||

                !req.base_request.tools.empty();



            // ── RAID fan-out: delegate to federated backend when requested ──

            // If a federated backend is attached and the request lists specific
- Line 1220: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (const auto& fr : fan_results) {

                        if (fr.success) {

                            response = fr.response;

                            response.metadata["fan_out_instance"] = fr.instance_id;

                            response.metadata["fan_out_total"]    = fan_results.size();

                            merged = true;

                            break;
- Line 1221: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (fr.success) {

                            response = fr.response;

                            response.metadata["fan_out_instance"] = fr.instance_id;

                            response.metadata["fan_out_total"]    = fan_results.size();

                            merged = true;

                            break;

                        }
- Line 1246: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

            }



            if (effective_request.metadata.is_object()) {

                const auto self_rag_it = effective_request.metadata.find("self_rag");

                if (self_rag_it != effective_request.metadata.end() &&

                    self_rag_it->is_object() &&
- Line 1247: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



            if (effective_request.metadata.is_object()) {

                const auto self_rag_it = effective_request.metadata.find("self_rag");

                if (self_rag_it != effective_request.metadata.end() &&

                    self_rag_it->is_object() &&

                    self_rag_it->value("enabled", false)) {
- Line 1248: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (effective_request.metadata.is_object()) {

                const auto self_rag_it = effective_request.metadata.find("self_rag");

                if (self_rag_it != effective_request.metadata.end() &&

                    self_rag_it->is_object() &&

                    self_rag_it->value("enabled", false)) {

                    SelfRAGRetrievalCallback retrieval_cb;
- Line 1260: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



                    if (!retrieval_cb) {

                        throw std::runtime_error(

                            "InferenceEngineEnhanced: self_rag enabled but no retrieval callback set");

                    }
- Line 1338: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: doc.relevance_score =

                                    static_cast<float>(rated.critic_score);

                                doc.metadata = makeSelfRAGMetadataObject();

                                doc.metadata["self_rag_verdict"] = verdict;

                                doc.metadata["critic_score"] = rated.critic_score;

                                doc.metadata["retrieval_score"] = rated.document.score;

                                rag_context.documents.push_back(std::move(doc));
- Line 1339: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: static_cast<float>(rated.critic_score);

                                doc.metadata = makeSelfRAGMetadataObject();

                                doc.metadata["self_rag_verdict"] = verdict;

                                doc.metadata["critic_score"] = rated.critic_score;

                                doc.metadata["retrieval_score"] = rated.document.score;

                                rag_context.documents.push_back(std::move(doc));

                            }
- Line 1340: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: doc.metadata = makeSelfRAGMetadataObject();

                                doc.metadata["self_rag_verdict"] = verdict;

                                doc.metadata["critic_score"] = rated.critic_score;

                                doc.metadata["retrieval_score"] = rated.document.score;

                                rag_context.documents.push_back(std::move(doc));

                            }

                        };
- Line 1351: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!rag_context.documents.empty()) {

                            self_rag.rag_context = std::move(rag_context);

                            self_rag.used_rag_context = true;

                            effective_request.metadata["rag_enabled"] = true;

                        }

                    }

                }
- Line 1420: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // The plugin may or may not use them; standard generation

                    // is used as fallback regardless.

                    auto lookup_decoding =

                        effective_request.metadata.value("lookup_decoding", json::object());

                    lookup_decoding["draft_tokens"] = drafts;

                    lookup_decoding["ngram_hit"] = true;

                    effective_request.metadata["lookup_decoding"] = std::move(lookup_decoding);
- Line 1423: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: effective_request.metadata.value("lookup_decoding", json::object());

                    lookup_decoding["draft_tokens"] = drafts;

                    lookup_decoding["ngram_hit"] = true;

                    effective_request.metadata["lookup_decoding"] = std::move(lookup_decoding);

                }

            }
- Line 1423: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: effective_request.metadata.value("lookup_decoding", json::object());

                    lookup_decoding["draft_tokens"] = drafts;

                    lookup_decoding["ngram_hit"] = true;

                    effective_request.metadata["lookup_decoding"] = std::move(lookup_decoding);

                }

            }
- Line 1429: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response = plugin->generateRAG(*self_rag.rag_context, effective_request);
- Line 1431: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response = plugin->generate(effective_request);
- Line 1446: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: self_rag_meta["relevant_docs"] = self_rag.result.relevant_docs.size();

                self_rag_meta["partial_docs"] = self_rag.result.partial_docs.size();

                self_rag_meta["rounds_used"] = self_rag.result.total_rounds_used;

                response.metadata["self_rag"] = std::move(self_rag_meta);

            }



            // After the (uninterruptible) plugin call returns, re-check whether
- Line 1453: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (tracked->cancel_token->load(std::memory_order_acquire)) {
- Line 1727: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // ── Step 2: Explicit caller preference ──────────────────────────────────

    // If specific model requested and available, use it (honour concurrency quota)

    if (!request.preferred_model_id.empty()) {

        auto it = models_.find(request.preferred_model_id);

        if (it != models_.end() && it->second.is_available) {

            const auto& quota = it->second.quota;
- Line 1757: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t index = round_robin_index_.fetch_add(1, std::memory_order_relaxed) % available.size();
- Line 1808: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (info.avg_response_time_ms == 0.0) {
- Line 1839: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (stats_.avg_batch_size == 0.0) {
- Line 1875: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (model_latency == 0.0) {
- Line 2021: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: draft_result = draft_plugin->generateDraftTokens(
- Line 2131: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response = target_plugin->generate(request);
- Line 2132: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // built during draft verification; here we generate from scratch.

    try {

        response = target_plugin->generate(request);

        response.metadata["speculative_accepted"] =

            static_cast<uint64_t>(verify_result.num_accepted);

        response.metadata["speculative_all_accepted"] = verify_result.all_accepted;

        response.metadata["speculative_acceptance_rate"] = verify_result.acceptance_rate;
- Line 2134: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: response = target_plugin->generate(request);

        response.metadata["speculative_accepted"] =

            static_cast<uint64_t>(verify_result.num_accepted);

        response.metadata["speculative_all_accepted"] = verify_result.all_accepted;

        response.metadata["speculative_acceptance_rate"] = verify_result.acceptance_rate;

    } catch (const std::exception& e) {

        spdlog::warn("Target model generation failed in speculative path: {}", e.what());
- Line 2135: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: response.metadata["speculative_accepted"] =

            static_cast<uint64_t>(verify_result.num_accepted);

        response.metadata["speculative_all_accepted"] = verify_result.all_accepted;

        response.metadata["speculative_acceptance_rate"] = verify_result.acceptance_rate;

    } catch (const std::exception& e) {

        spdlog::warn("Target model generation failed in speculative path: {}", e.what());

        return false;
- Line 488: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1031: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                

                it->second->promise.set_value(timeout_response);

            } catch (...) {

                // Promise already satisfied

            }
- Line 1031: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1230: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: agg += "[" + fr.instance_id + ": " + fr.error + "] ";
- Line 1231: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: agg += "[" + fr.instance_id + ": " + fr.error + "] ";
- Line 1431: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response = plugin->generate(effective_request);
- Line 1478: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

            try {

                tracked->promise.set_value(response);

            } catch (...) {

                // Promise already resolved (rare race with timeout monitor) — ignore.

            }
- Line 1478: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1503: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                tracked->promise.set_value(error_response);

            } catch (...) {

                // Promise already satisfied

            }

        }
- Line 1503: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1653: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // The lock is released before embed() to avoid holding models_mutex_ during
- Line 1671: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return plugin->embed(text);
- Line 1762: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto least_loaded = available[0];
- Line 1776: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto fastest = available[0];
- Line 2015: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // generateDraftTokens() calls generate() internally and maps text to token
- Line 2087: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const auto tgt_resp = target_plugin->generate(one_tok_req);
- Line 2093: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: static_cast<int>(static_cast<unsigned char>(tgt_resp.text[0])) %

                        vocab_size_int;

                }

            } catch (...) {

                // Non-fatal: keep target_pred_token = 0.

            }

        }
- Line 2093: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 2131: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response = target_plugin->generate(request);
- Line 2242: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // in this engine instance (so the engine can call plugin->generate()).

### llm/gpu_memory_manager.cpp
Total findings: 84

- Line 127: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: ptr_
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
- Line 142: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: ptr_
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
- Line 571: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t err = cudaMalloc(&ptr, bytes);
- Line 573: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::error("cudaMalloc failed: {}", cudaGetErrorString(err));
- Line 641: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t err = cudaMallocHost(&ptr, bytes);
- Line 643: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::warn("cudaMallocHost failed: {}, falling back to regular malloc",
- Line 1082: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: new_ptr
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: new_ptr = std::malloc(total_vram);
- Line 1183: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t err = cudaMallocHost(&new_ptr, total_ram);
- Line 1428: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t err = cudaMalloc(&ptr, bytes);
- Line 1430: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::error("cudaMalloc failed on GPU {}: {}", gpu_device_id, cudaGetErrorString(err));
- Line 2227: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        }



        lock.lock();

        if (!isTrackedGpuNoLock(available_gpus_, gpu_device_id) ||

            !isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id)) {

            spdlog::warn("updateGPUHealth: dropping writeback for untracked GPU {}", gpu_device_id);
- Line 2227: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 2269: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    }



    lock.lock();

    if (!isTrackedGpuNoLock(available_gpus_, gpu_device_id) ||

        !isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id)) {

        spdlog::warn("updateGPUHealth: dropping writeback for untracked GPU {}", gpu_device_id);
- Line 2269: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: lock.lock();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3802 [LLM] AdaptiveVRAMA
- Line 553: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* GPUMemoryManager::allocateGPU(const std::string& model_id, size_t bytes) {
- Line 573: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::error("cudaMalloc failed: {}", cudaGetErrorString(err));
- Line 578: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: ptr = std::malloc(bytes);
- Line 587: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: ptr = std::malloc(bytes);
- Line 609: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("allocateGPU metadata bookkeeping failed for model {}: {}", model_id, e.what());
- Line 613: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("allocateGPU metadata bookkeeping failed for model {}: unknown exception", model_id);
- Line 627: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* GPUMemoryManager::allocateCPU(const std::string& model_id, size_t bytes, bool pinned) {
- Line 631: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("Cannot allocate {} bytes RAM for model {}: insufficient memory",
- Line 643: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::warn("cudaMallocHost failed: {}, falling back to regular malloc",
- Line 646: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc() — missing null pointer check
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: spdlog::warn("cudaMallocHost failed: {}, falling back to regular malloc", 

                        cudaGetErrorString(err));

            pinned = false;

            ptr = std::malloc(bytes);

        }

    } else {

        ptr = std::malloc(bytes);
- Line 649: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc() — missing null pointer check
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ptr = std::malloc(bytes);

        }

    } else {

        ptr = std::malloc(bytes);

        pinned = false;

    }

#else
- Line 654: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc() — missing null pointer check
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

#else

    // Simulation mode: always use regular malloc

    ptr = std::malloc(bytes);

    pinned = false;

#endif
- Line 659: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("Failed to allocate {} bytes RAM for model {}", bytes, model_id);
- Line 1054: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::warn("Failed to allocate consolidated GPU memory for model {} on device {}", model_id, devic
- Line 1061: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t copy_err = cudaMemcpy(static_cast<char*>(new_ptr) + offset,
- Line 1066: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::warn("Defrag: cudaMemcpy failed for model {} on GPU {}: {}",
- Line 1074: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // REL-66: check cudaFree return value in defragment cleanup path
- Line 1077: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::warn("Defrag: cudaFree of scratch buffer failed: {}",
- Line 1083: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: new_ptr = std::malloc(total_vram);
- Line 1084: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1089: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1094: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: new_ptr = std::malloc(total_vram);
- Line 1095: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1100: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1132: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1136: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1185: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::warn("Failed to allocate consolidated pinned memory for model {}: {}",
- Line 1188: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc() — missing null pointer check
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: spdlog::warn("Failed to allocate consolidated pinned memory for model {}: {}", 

                           model_id, cudaGetErrorString(err));

                // Fall back to regular malloc

                new_ptr = std::malloc(total_ram);

            }

        } else {

            new_ptr = std::malloc(total_ram);
- Line 1191: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc() — missing null pointer check
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: new_ptr = std::malloc(total_ram);

            }

        } else {

            new_ptr = std::malloc(total_ram);

        }

#else

        new_ptr = std::malloc(total_ram);
- Line 1194: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: new_ptr = std::malloc(total_ram);
- Line 1197: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1232: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1236: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1256: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: void* new_ptr = std::malloc(total_ram);
- Line 1257: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1292: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1296: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1402: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* GPUMemoryManager::allocateGPU(const std::string& model_id, size_t bytes, int gpu_device_id) {
- Line 1430: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::error("cudaMalloc failed on GPU {}: {}", gpu_device_id, cudaGetErrorString(err));
- Line 1435: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: ptr = std::malloc(bytes);
- Line 1444: severity=HIGH; category=unchecked_malloc
  Description: Unchecked malloc — no null check before use
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: ptr = std::malloc(bytes);
- Line 1466: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("allocateGPU(gpu_device_id={}) metadata bookkeeping failed for model {}: {}",
- Line 1471: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("allocateGPU(gpu_device_id={}) metadata bookkeeping failed for model {}: unknown excep
- Line 1478: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("GPUMemoryManager::allocateGPU: global VRAM accounting overflow for model '{}'; clampi
- Line 1784: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (model_id.find("adapter_") != std::string::npos) {
- Line 1830: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = per_gpu_vram_used_.find(gpu_id);
- Line 1846: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (model_id.find("adapter_") != std::string::npos) {
- Line 1930: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = gpu_health_data_.find(gpu_id);
- Line 1935: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto health_it = gpu_health_status_.find(gpu_id);
- Line 1938: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto temp_it = gpu_temperatures_.find(gpu_id);
- Line 1941: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto util_it = gpu_utilizations_.find(gpu_id);
- Line 2060: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = gpu_health_status_.find(gpu_id);
- Line 2150: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 106: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 129: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::free(ptr_);
- Line 133: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::free(ptr_);
- Line 144: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::free(ptr_);
- Line 148: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::free(ptr_);
- Line 154: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::free(ptr_);
- Line 1025: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int, std::vector<MemoryAllocation>> per_device_allocs;
- Line 1127: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<void*> ptrs_to_erase;
- Line 1226: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<void*> ptrs_to_erase;
- Line 1286: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<void*> ptrs_to_erase;
- Line 1895: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: gpu_device_id
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = gpu_health_data_.find(gpu_device_id);
- Line 1901: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: gpu_device_id
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto health_it = gpu_health_status_.find(gpu_device_id);
- Line 1904: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: gpu_device_id
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto temp_it = gpu_temperatures_.find(gpu_device_id);
- Line 1931: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: gpu_id
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = gpu_health_data_.find(gpu_id);
- Line 1936: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: gpu_id
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto health_it = gpu_health_status_.find(gpu_id);
- Line 1939: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: gpu_id
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto temp_it = gpu_temperatures_.find(gpu_id);

### llm/lora_framework/vram_allocator.cpp
Total findings: 82

- Line 717: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t err = cudaMalloc(&ptr, size_bytes);
- Line 814: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: ptr
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: security::VRAMSecureClear::secureClearHIP(ptr, block_size);
- Line 829: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: ptr
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: if (backend_context_ && ptr) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4678 feat: replace production st... (2026-04-15) | #998 C++ Audit: Eliminate
- Line 101: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (vkCreateInstance(&inst_ci, nullptr, &ctx->instance) != VK_SUCCESS) {
- Line 109: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: VkResult enum_result = vkEnumeratePhysicalDevices(ctx->instance, &dev_count, nullptr);
- Line 113: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkDestroyInstance(ctx->instance, nullptr);
- Line 114: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->instance = VK_NULL_HANDLE;
- Line 120: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkDestroyInstance(ctx->instance, nullptr);
- Line 121: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->instance = VK_NULL_HANDLE;
- Line 126: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: VkResult enum_result = vkEnumeratePhysicalDevices(ctx->instance, &dev_count, devs.data());
- Line 130: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkDestroyInstance(ctx->instance, nullptr);
- Line 131: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->instance = VK_NULL_HANDLE;
- Line 136: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->physical_device = devs[0];                     // fallback
- Line 141: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->physical_device = pd;
- Line 148: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &qf_count, nullptr);
- Line 150: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &qf_count,
- Line 162: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkDestroyInstance(ctx->instance, nullptr);
- Line 163: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->instance = VK_NULL_HANDLE;
- Line 180: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (vkCreateDevice(ctx->physical_device, &dev_ci, nullptr,
- Line 181: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: &ctx->device) != VK_SUCCESS) {
- Line 183: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkDestroyInstance(ctx->instance, nullptr);
- Line 184: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->instance = VK_NULL_HANDLE;
- Line 190: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkGetPhysicalDeviceMemoryProperties(ctx->physical_device, &mem_props);
- Line 221: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (vkCreateBuffer(ctx->device, &buf_ci, nullptr, &buffer) != VK_SUCCESS) {
- Line 228: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkGetBufferMemoryRequirements(ctx->device, buffer, &mem_req);
- Line 234: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->findMemoryType(mem_req.memoryTypeBits, kHostProps);
- Line 237: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkDestroyBuffer(ctx->device, buffer, nullptr);
- Line 247: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (vkAllocateMemory(ctx->device, &alloc_info, nullptr, &memory) != VK_SUCCESS) {
- Line 249: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkDestroyBuffer(ctx->device, buffer, nullptr);
- Line 253: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (vkBindBufferMemory(ctx->device, buffer, memory, 0) != VK_SUCCESS) {
- Line 255: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkFreeMemory(ctx->device, memory, nullptr);
- Line 256: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkDestroyBuffer(ctx->device, buffer, nullptr);
- Line 261: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (vkMapMemory(ctx->device, memory, 0, mem_req.size, 0, &mapped) != VK_SUCCESS) {
- Line 263: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkFreeMemory(ctx->device, memory, nullptr);
- Line 264: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkDestroyBuffer(ctx->device, buffer, nullptr);
- Line 268: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->entries.push_back({buffer, memory, mapped,
- Line 274: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (auto it = ctx->entries.begin(); it != ctx->entries.end(); ++it) {
- Line 276: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkUnmapMemory(ctx->device, it->memory);
- Line 277: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkFreeMemory(ctx->device, it->memory, nullptr);
- Line 278: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: vkDestroyBuffer(ctx->device, it->buffer, nullptr);
- Line 279: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->entries.erase(it);
- Line 288: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (auto& e : ctx->entries) {
- Line 289: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (e.mapped) vkUnmapMemory(ctx->device, e.memory);
- Line 290: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (e.memory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, e.memory, nullptr);
- Line 291: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (e.buffer != VK_NULL_HANDLE) vkDestroyBuffer(ctx->device, e.buffer, nullptr);
- Line 293: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->entries.clear();
- Line 294: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ctx->device   != VK_NULL_HANDLE) vkDestroyDevice(ctx->device, nullptr);
- Line 295: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (ctx->instance != VK_NULL_HANDLE) vkDestroyInstance(ctx->instance, nullptr);
- Line 296: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->device   = VK_NULL_HANDLE;
- Line 297: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: ctx->instance = VK_NULL_HANDLE;
- Line 355: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: , allocated_bytes_(other.allocated_bytes_)
- Line 362: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: other.allocated_bytes_ = 0;
- Line 374: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocated_bytes_ = other.allocated_bytes_;
- Line 381: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: other.allocated_bytes_ = 0;
- Line 458: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: case acceleration::BackendType::VULKAN:

#ifdef THEMIS_ENABLE_VULKAN

        {

            // REL-48: use RAII unique_ptr instead of raw new/delete for Vulkan context

            auto vk_ctx_owner = std::make_unique<VulkanAllocContext>();

            if (!vk_init(vk_ctx_owner.get(), pool_size_bytes_)) {

                return false;
- Line 458: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // REL-48: use RAII unique_ptr instead of raw new/delete for Vulkan context
- Line 495: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete vk_ctx;
- Line 495: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (backend_ == acceleration::BackendType::VULKAN && backend_context_) {

        auto* vk_ctx = static_cast<VulkanAllocContext*>(backend_context_);

        vk_shutdown(vk_ctx);

        delete vk_ctx;

    }

#endif

    backend_context_ = nullptr;
- Line 495: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete vk_ctx;
- Line 501: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void* VRAMAllocator::allocate(size_t size_bytes, size_t alignment) {
- Line 525: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 532: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 533: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 534: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 535: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 536: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 545: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void VRAMAllocator::deallocate(void* ptr) {
- Line 571: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: deallocate_to_backend(ptr);
- Line 582: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t err = cudaMemcpy(dst, src, size_bytes, cudaMemcpyHostToDevice);
- Line 629: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaError_t err = cudaMemcpy(dst, src, size_bytes, cudaMemcpyDeviceToHost);
- Line 708: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocated_bytes_ = 0;
- Line 711: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* VRAMAllocator::allocate_from_backend(size_t size_bytes, size_t alignment) {
- Line 801: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // REL-64: check cudaFree return value in release_backend_ptr_
- Line 805: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::error("VRAMAllocator::release_backend_ptr_: cudaFree failed: {}",
- Line 906: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: deallocate_to_backend(it->ptr);
- Line 922: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ptr_ = allocator_->allocate(size_bytes);
- Line 928: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocator_->deallocate(ptr_);
- Line 945: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocator_->deallocate(ptr_);
- Line 836: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: vk_free(vk_ctx, ptr);
- Line 851: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: _aligned_free(ptr);
- Line 853: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: free(ptr);

### llm/lora_framework/kernels/directx_kernels.cpp
Total findings: 54

- Line 297: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_C = g_directx_state.descriptors->create_uav(
- Line 299: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_A = g_directx_state.descriptors->create_srv(
- Line 301: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_B = g_directx_state.descriptors->create_srv(
- Line 321: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
- Line 377: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_C = g_directx_state.descriptors->create_uav(
- Line 379: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_A = g_directx_state.descriptors->create_srv(
- Line 381: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_B = g_directx_state.descriptors->create_srv(
- Line 403: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
- Line 453: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_C = g_directx_state.descriptors->create_uav(
- Line 455: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_A = g_directx_state.descriptors->create_srv(
- Line 457: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_B = g_directx_state.descriptors->create_srv(
- Line 474: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
- Line 523: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_C = g_directx_state.descriptors->create_uav(
- Line 525: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_A = g_directx_state.descriptors->create_srv(
- Line 527: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_B = g_directx_state.descriptors->create_srv(
- Line 544: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
- Line 590: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_C = g_directx_state.descriptors->create_uav(
- Line 592: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_A = g_directx_state.descriptors->create_srv(
- Line 594: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_B = g_directx_state.descriptors->create_srv(
- Line 612: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
- Line 678: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_grad_A = g_directx_state.descriptors->create_uav(
- Line 680: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_grad_B = g_directx_state.descriptors->create_uav(
- Line 682: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_grad_input = g_directx_state.descriptors->create_uav(
- Line 685: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_input = g_directx_state.descriptors->create_srv(
- Line 687: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_B = g_directx_state.descriptors->create_srv(
- Line 689: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_A = g_directx_state.descriptors->create_srv(
- Line 691: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_grad_output = g_directx_state.descriptors->create_srv(
- Line 716: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
- Line 779: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_grad_A = g_directx_state.descriptors->create_uav(
- Line 784: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_grad_B = g_directx_state.descriptors->create_uav(
- Line 786: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t uav_grad_input = g_directx_state.descriptors->create_uav(
- Line 789: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_input = g_directx_state.descriptors->create_srv(
- Line 791: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_B = g_directx_state.descriptors->create_srv(
- Line 793: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_A = g_directx_state.descriptors->create_srv(
- Line 795: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t srv_grad_output = g_directx_state.descriptors->create_srv(
- Line 820: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: ID3D12DescriptorHeap* heaps[] = {g_directx_state.descriptors->heap()};
- Line 874: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: buffer_embedding_weights.upload(embedding_weights, embedding_bytes);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #572 Complete DirectX 12 Compute... (2026-03-11) | #570 [LoRA Phase 10] Add r
- Line 336: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buffer_C.download(C, size_C);

    }

    catch (const std::exception& e) {

        throw std::runtime_error(std::string("launch_matmul_shader failed: ") + e.what());

    }

}
- Line 417: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buffer_C.download(C, byte_size);

    }

    catch (const std::exception& e) {

        throw std::runtime_error(std::string("launch_add_shader failed: ") + e.what());

    }

}
- Line 484: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buffer_C.download(C, byte_size);

    }

    catch (const std::exception& e) {

        throw std::runtime_error(std::string("launch_multiply_shader failed: ") + e.what());

    }

}
- Line 554: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buffer_B.download(B, byte_size);

    }

    catch (const std::exception& e) {

        throw std::runtime_error(std::string("launch_scalar_multiply_shader failed: ") + e.what());

    }

}
- Line 622: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buffer_output.download(output, byte_size);

    }

    catch (const std::exception& e) {

        throw std::runtime_error(std::string("launch_transpose_shader failed: ") + e.what());

    }

}
- Line 731: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buffer_grad_A.download(grad_A, size_grad_A);

    }

    catch (const std::exception& e) {

        throw std::runtime_error(std::string("launch_lora_grad_A_shader failed: ") + e.what());

    }

}
- Line 835: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buffer_grad_B.download(grad_B, size_grad_B);

    }

    catch (const std::exception& e) {

        throw std::runtime_error(std::string("launch_lora_grad_B_shader failed: ") + e.what());

    }

}
- Line 916: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buffer_output.download(output, output_bytes);

    }

    catch (const std::exception& e) {

        throw std::runtime_error(std::string("launch_embedding_lookup_shader failed: ") + e.what());

    }

}
- Line 993: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: buffer_output.download(output, output_bytes);

    }

    catch (const std::exception& e) {

        throw std::runtime_error(std::string("launch_sequence_mean_shader failed: ") + e.what());

    }

}
- Line 200: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "DirectX 12 LoRA backend initialized successfully\n";
- Line 201: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "GPU: " << g_directx_state.context->get_gpu_description() << "\n";
- Line 232: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "DirectX 12 LoRA backend cleaned up\n";
- Line 272: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: 2   // num_srvs (inputs A, B)
- Line 357: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: 2   // num_srvs (inputs A, B)
- Line 661: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Dummy buffers for unused outputs
- Line 765: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Dummy buffers for unused outputs

### llm/multi_lora_manager.cpp
Total findings: 54

- Line 922: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_rank += source_loras[i]->rank * normalized_weights[i];
- Line 923: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_alpha += source_loras[i]->alpha * normalized_weights[i];
- Line 931: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: fused_lora->rank = static_cast<size_t>(std::round(avg_rank));
- Line 932: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: fused_lora->alpha = static_cast<size_t>(std::round(avg_alpha));
- Line 1261: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize LoRA adapter
- Line 1300: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // F1-2 fix: reject deserialized paths that escape the trusted base directory.
- Line 1392: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Load actual LoRA weights from the GGUF file for quantization.
- Line 1507: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t num_channels = config_.quantization.per_channel ? lora->rank : 1;
- Line 1545: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: lora->quantized_weights[offset + i] = static_cast<uint8_t>(quantized + INT8_ZERO_POINT);
- Line 2044: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: lora->assigned_gpus = config_.multi_gpu.devices;
- Line 2045: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: lora->primary_gpu = config_.multi_gpu.devices[0];
- Line 2061: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t chunk_size = lora->vram_bytes / config_.multi_gpu.devices.size();
- Line 2062: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: lora->assigned_gpus = config_.multi_gpu.devices;
- Line 2063: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: lora->primary_gpu = config_.multi_gpu.devices[0];
- Line 2286: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: lora->rank = static_cast<size_t>(parsed_rank);
- Line 2291: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: try { lora->alpha = static_cast<size_t>(std::stoull(it_alpha->second)); } catch (...) {}
- Line 2355: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: vram_charge = lora->vram_bytes * config_.multi_gpu.devices.size();
- Line 3177: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_rank += source_loras[i]->rank * normalized_weights[i];
- Line 3178: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: avg_alpha += source_loras[i]->alpha * normalized_weights[i];
- Line 3182: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: fused_lora->rank = static_cast<size_t>(avg_rank);
- Line 3183: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: fused_lora->alpha = static_cast<size_t>(avg_alpha);
- Line 3670: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: it->second.avg_inference_time_ms = fusion_time_ms;
- Line 3683: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: double prev_avg = it->second.avg_inference_time_ms;
- Line 3686: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: it->second.avg_inference_time_ms =
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4678 feat: replace production st... (2026-04-15) | #723 Fix SCHEDULED fusion
- Line 755: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 807: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response.tokens_generated = static_cast<int>(generated.size());
- Line 864: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = loras_.find(lora_ids[i]);
- Line 939: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1227: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("LoRA serialization buffer underallocated for {}", lora_id);
- Line 1552: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: lora->vram_bytes = num_weights + lora->scale_factors.size() * sizeof(float);  // INT8 weights + scal
- Line 1702: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: const size_t kMaxAlloc = 1024 * 1024 / sizeof(float);  // cap at 1 MB
- Line 2173: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // No base directory configured — skip the check (legacy mode).
- Line 2278: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "is outside allowed bounds [{}, {}]; clamping",
- Line 2418: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 2437: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 3126: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = loras_.find(lora_id);
- Line 3140: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Validate compatibility
- Line 3225: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 3306: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // For backward compatibility, prefer custom schedule function if provided,
- Line 3365: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Backward compatibility: use a_weight and b_weight
- Line 3576: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = loras_.find(lora_id);
- Line 32: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void llama_lora_adapter_free(void* adapter);
- Line 291: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: llama_lora_adapter_free(lora->adapter_handle);
- Line 407: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float scale = 0.0f;
- Line 409: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool has_adapter = false;
- Line 903: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: normalized_weights.push_back(w / weight_sum);
- Line 1806: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<int, size_t> MultiLoRAManager::getPerGPUMemoryUsage() const {
- Line 2012: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // once the base llama_model* is available (called from LlamaWrapper::generate()).
- Line 2140: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int max_threads = 0;
- Line 2515: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 2559: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: candidates.push_back({id, lora.get(), score});
- Line 2767: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: llama_lora_adapter_free(lora->adapter_handle);
- Line 3159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: normalized_weights.push_back(w / weight_sum);

### llm/model_loader.cpp
Total findings: 43

- Line 59: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: while ((pos = state->pending_line.find('\n')) != std::string::npos) {
- Line 60: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const std::string line = state->pending_line.substr(0, pos);
- Line 156: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: CachedModel* LazyModelLoader::getOrLoadModel(
- Line 212: severity=CRITICAL; category=double_lock
  Description: Double lock without unlock (potential deadlock)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: lock.lock();
- Line 235: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto result = loadModelInternal(model_id, model_path, load_config);
- Line 243: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool LazyModelLoader::preloadModel(
- Line 248: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // W1-L01: Model preloading with integrity validation via loadModelInternal.
- Line 249: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Reviewed: integrity checks delegated to loadModelInternal; no FP.
- Line 280: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto result = loadModelInternal(model_id, model_path, load_config);
- Line 316: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Check if already being loaded via preloadModel
- Line 318: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::warn("Model {} is already being loaded via preloadModel(). "
- Line 320: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "Consider using preloadModel() for async loading without progress tracking.",
- Line 371: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Note: loadModelInternal is the heavy operation that does the real work
- Line 373: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto result = loadModelInternal(model_id, model_path, load_config);
- Line 387: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (cancel_token.is_cancelled()) {

                spdlog::info("Model load cancelled after loading: {}", model_id);

                // Model was loaded but user cancelled, so unload it

                load_lock.lock();

                unloadModel(model_id, true);

                load_lock.unlock();

                return nullptr;
- Line 387: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: load_lock.lock();
- Line 388: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: unloadModel(model_id, true);
- Line 427: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool LazyModelLoader::unloadModel(const std::string& model_id, bool force) {
- Line 578: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: unloadModel(id, true);
- Line 644: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Model format validation delegated to llama.cpp library (llama_load_model_from_file validates GGUF header).
- Line 740: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto loadModelWithGpuFallback = [&](const char* stage) -> llama_model* {
- Line 762: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::info("{}: trying llama_load_model_from_file with n_gpu_layers={}", stage, layers);
- Line 763: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* loaded = llama_load_model_from_file(model_path.c_str(), load_params);
- Line 802: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: lmodel = loadModelWithGpuFallback("Custom GGUF validation path");
- Line 820: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::info("Falling back to native llama_load_model_from_file()");
- Line 821: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: lmodel = loadModelWithGpuFallback("Native fallback path");
- Line 957: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: model->info.metadata["runtime_llama_assigned_cpu_tensors"] = log_capture.state().assigned_cpu;
- Line 958: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: model->info.metadata["runtime_llama_assigned_non_cpu_tensors"] = log_capture.state().assigned_non_cp
- Line 959: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: model->info.metadata["runtime_llama_backend_cpu_only_hint"] = log_capture.state().backend_cpu_only_h
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #998 C++ Audit: Eliminate raw me... (2026-03-11) | #751 Phase 4 Error Handlin
- Line 190: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto status = future_model.wait_for(std::chrono::seconds(300)); // 5 minute timeout
- Line 387: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: load_lock.lock();
- Line 607: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t hits = cache_hits_.load(std::memory_order_relaxed);
- Line 608: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t misses = cache_misses_.load(std::memory_order_relaxed);
- Line 712: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Compatibility shim for Gemma GGUF variants that omit
- Line 953: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const bool gpu_offload_requested = requested_gpu_layers > 0;

    const bool gpu_offload_effective = gpu_offload_requested && log_capture.state().assigned_non_cpu;



    model->info.metadata["runtime_gpu_offload_requested"] = gpu_offload_requested;

    model->info.metadata["runtime_gpu_offload_effective"] = gpu_offload_effective;

    model->info.metadata["runtime_gpu_layers_requested"] = requested_gpu_layers;

    model->info.metadata["runtime_gpu_layers_applied"] = applied_gpu_layers;
- Line 954: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const bool gpu_offload_effective = gpu_offload_requested && log_capture.state().assigned_non_cpu;



    model->info.metadata["runtime_gpu_offload_requested"] = gpu_offload_requested;

    model->info.metadata["runtime_gpu_offload_effective"] = gpu_offload_effective;

    model->info.metadata["runtime_gpu_layers_requested"] = requested_gpu_layers;

    model->info.metadata["runtime_gpu_layers_applied"] = applied_gpu_layers;

    model->info.metadata["runtime_llama_assigned_cpu_tensors"] = log_capture.state().assigned_cpu;
- Line 955: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: model->info.metadata["runtime_gpu_offload_requested"] = gpu_offload_requested;

    model->info.metadata["runtime_gpu_offload_effective"] = gpu_offload_effective;

    model->info.metadata["runtime_gpu_layers_requested"] = requested_gpu_layers;

    model->info.metadata["runtime_gpu_layers_applied"] = applied_gpu_layers;

    model->info.metadata["runtime_llama_assigned_cpu_tensors"] = log_capture.state().assigned_cpu;

    model->info.metadata["runtime_llama_assigned_non_cpu_tensors"] = log_capture.state().assigned_non_cpu;
- Line 956: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: model->info.metadata["runtime_gpu_offload_requested"] = gpu_offload_requested;

    model->info.metadata["runtime_gpu_offload_effective"] = gpu_offload_effective;

    model->info.metadata["runtime_gpu_layers_requested"] = requested_gpu_layers;

    model->info.metadata["runtime_gpu_layers_applied"] = applied_gpu_layers;

    model->info.metadata["runtime_llama_assigned_cpu_tensors"] = log_capture.state().assigned_cpu;

    model->info.metadata["runtime_llama_assigned_non_cpu_tensors"] = log_capture.state().assigned_non_cpu;

    model->info.metadata["runtime_llama_backend_cpu_only_hint"] = log_capture.state().backend_cpu_only_hint;
- Line 957: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: model->info.metadata["runtime_gpu_offload_effective"] = gpu_offload_effective;

    model->info.metadata["runtime_gpu_layers_requested"] = requested_gpu_layers;

    model->info.metadata["runtime_gpu_layers_applied"] = applied_gpu_layers;

    model->info.metadata["runtime_llama_assigned_cpu_tensors"] = log_capture.state().assigned_cpu;

    model->info.metadata["runtime_llama_assigned_non_cpu_tensors"] = log_capture.state().assigned_non_cpu;

    model->info.metadata["runtime_llama_backend_cpu_only_hint"] = log_capture.state().backend_cpu_only_hint;
- Line 958: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: model->info.metadata["runtime_gpu_layers_requested"] = requested_gpu_layers;

    model->info.metadata["runtime_gpu_layers_applied"] = applied_gpu_layers;

    model->info.metadata["runtime_llama_assigned_cpu_tensors"] = log_capture.state().assigned_cpu;

    model->info.metadata["runtime_llama_assigned_non_cpu_tensors"] = log_capture.state().assigned_non_cpu;

    model->info.metadata["runtime_llama_backend_cpu_only_hint"] = log_capture.state().backend_cpu_only_hint;



    auto* result = model.get();
- Line 959: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: model->info.metadata["runtime_gpu_layers_applied"] = applied_gpu_layers;

    model->info.metadata["runtime_llama_assigned_cpu_tensors"] = log_capture.state().assigned_cpu;

    model->info.metadata["runtime_llama_assigned_non_cpu_tensors"] = log_capture.state().assigned_non_cpu;

    model->info.metadata["runtime_llama_backend_cpu_only_hint"] = log_capture.state().backend_cpu_only_hint;



    auto* result = model.get();

    models_[model_id] = std::move(model);
- Line 118: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: llama_backend_free();

### llm/docs_assistant.cpp
Total findings: 40

- Line 223: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->semantic_embedding_compatible = false;
- Line 224: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->embedding_dimension = 0;
- Line 228: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->embedding_dimension = emb["dimension"].get<int>();
- Line 233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->semantic_embedding_compatible = (backend.find("hash-fallback") != std::string::npos);
- Line 421: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: query_vec = hashEmbedQuery(query, impl_->embedding_dimension);
- Line 508: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: rag_doc.content = rag_doc.content.substr(0, impl_->config.context_preview_length);
- Line 518: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: rag_request.model_id = impl_->config.llm_model_id;
- Line 630: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto cache_it = impl_->cache.find(query);
- Line 631: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (cache_it != impl_->cache.end()) {
- Line 644: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: result.total_docs_searched = saturating_to_int(impl_->documents.size());
- Line 671: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->cache[query] = result;
- Line 678: severity=CRITICAL; category=sql_injection
  Description: string_concat_sql: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string query = "How do I configure " + topic + " in ThemisDB? What are the configuration option
- Line 683: severity=CRITICAL; category=sql_injection
  Description: string_concat_sql: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string query = "I'm experiencing this issue with ThemisDB: " + error_description + ". How can I
- Line 691: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats["cache_size"] = impl_->cache.size();
- Line 276: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: };



                if (chunk_json.contains("embedding")) {

                    doc.themis_metadata["vector"]["embedding"] = chunk_json["embedding"];

                    if (chunk_json["embedding"].is_array()) {

                        for (const auto& x : chunk_json["embedding"]) {

                            doc.embedding.push_back(x.get<float>());
- Line 293: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: doc.has_embedding = !doc.embedding_q.empty();

                    doc.is_quantized_embedding = true;

                }

                doc.themis_metadata["vector"]["text_content"] = doc.text_content;

                doc.themis_metadata["vector"]["content_length"] = doc.content_length;



                impl_->documents.push_back(std::move(doc));
- Line 294: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: doc.is_quantized_embedding = true;

                }

                doc.themis_metadata["vector"]["text_content"] = doc.text_content;

                doc.themis_metadata["vector"]["content_length"] = doc.content_length;



                impl_->documents.push_back(std::move(doc));

            }
- Line 303: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy format fallback: extract documents
- Line 333: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: doc.themis_metadata = doc_json["themis_metadata"];

                

                if (doc.themis_metadata.contains("vector") && 

                    doc.themis_metadata["vector"].contains("text_content")) {

                    doc.text_content = doc.themis_metadata["vector"]["text_content"].get<std::string>();

                    

                    if (doc.themis_metadata["vector"].contains("content_length")) {
- Line 334: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (doc.themis_metadata.contains("vector") && 

                    doc.themis_metadata["vector"].contains("text_content")) {

                    doc.text_content = doc.themis_metadata["vector"]["text_content"].get<std::string>();

                    

                    if (doc.themis_metadata["vector"].contains("content_length")) {

                        doc.content_length = doc.themis_metadata["vector"]["content_length"].get<int>();
- Line 336: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: doc.themis_metadata["vector"].contains("text_content")) {

                    doc.text_content = doc.themis_metadata["vector"]["text_content"].get<std::string>();

                    

                    if (doc.themis_metadata["vector"].contains("content_length")) {

                        doc.content_length = doc.themis_metadata["vector"]["content_length"].get<int>();

                    }

                }
- Line 337: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: doc.text_content = doc.themis_metadata["vector"]["text_content"].get<std::string>();

                    

                    if (doc.themis_metadata["vector"].contains("content_length")) {

                        doc.content_length = doc.themis_metadata["vector"]["content_length"].get<int>();

                    }

                }

            }
- Line 528: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info(

                "DocsAssistant::generateAnswer plugin-rag dispatch: docs={} rag_mode='{}' tensor_slots={} tensor_slot_chars={} max_context_tokens={} response_budget_tokens={}",

                rag_context.documents.size(),

                rag_request.metadata.value("rag_mode", std::string{"text"}),

                rag_request.metadata.value("rag_tensor_slots", 0),

                rag_request.metadata.value("rag_tensor_slot_chars", 0),

                rag_context.max_context_tokens,
- Line 529: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "DocsAssistant::generateAnswer plugin-rag dispatch: docs={} rag_mode='{}' tensor_slots={} tensor_slot_chars={} max_context_tokens={} response_budget_tokens={}",

                rag_context.documents.size(),

                rag_request.metadata.value("rag_mode", std::string{"text"}),

                rag_request.metadata.value("rag_tensor_slots", 0),

                rag_request.metadata.value("rag_tensor_slot_chars", 0),

                rag_context.max_context_tokens,

                rag_context.response_budget_tokens);
- Line 530: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: rag_context.documents.size(),

                rag_request.metadata.value("rag_mode", std::string{"text"}),

                rag_request.metadata.value("rag_tensor_slots", 0),

                rag_request.metadata.value("rag_tensor_slot_chars", 0),

                rag_context.max_context_tokens,

                rag_context.response_budget_tokens);
- Line 534: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto rag_response = LLMPluginManager::instance().generateRAG(rag_context, rag_request);
- Line 537: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "DocsAssistant::generateAnswer plugin-rag complete: success=1 answer_chars={} tokens_generated={} inference_time_ms={:.2f}",
- Line 546: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "DocsAssistant::generateAnswer plugin-rag failed: error_len={} tokens_generated={} inference_time_ms={:.2f}",
- Line 605: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = LLMPluginManager::instance().generate(req);
- Line 620: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: DocsQueryResult DocsAssistant::query(const std::string& query) {
- Line 678: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string query = "How do I configure " + topic + " in ThemisDB? What are the configuration option
- Line 683: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string query = "I'm experiencing this issue with ThemisDB: " + error_description + ". How can I
- Line 45: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, DocsQueryResult> cache;
- Line 113: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::vector<float> hashEmbedQuery(const std::string& text, int dim) {
- Line 120: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> freqs;
- Line 237: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> doc_id_to_path;
- Line 561: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return THEMIS_LLM_GENERATE(safe_prompt);
- Line 588: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return THEMIS_LLM_GENERATE(safe_prompt);
- Line 605: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = LLMPluginManager::instance().generate(req);
- Line 131: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: const float weight = 1.0f + std::log(static_cast<float>(std::max(1, tf)));

### llm/llm_model_storage.cpp
Total findings: 38

- Line 145: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto blob_ref = config_.blob_manager->put(metadata.model_id, *model_data);
- Line 181: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto encrypted = encryption_->encrypt(serialized, config_.encryption_key_id);
- Line 190: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool success = config_.db->put(key, data_to_store);
- Line 205: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::optional<LLMModelMetadata> loadModel(const std::string& model_id) {
- Line 214: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto data = config_.db->get(key);
- Line 350: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::optional<std::vector<uint8_t>> loadModelBlob(const std::string& model_id) {
- Line 359: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto data = config_.db->get(key);
- Line 381: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize BaseEntity
- Line 382: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: BaseEntity entity = BaseEntity::deserialize(model_id, decrypted_data);
- Line 424: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto blob_data_opt = config_.blob_manager->get(blob_ref);
- Line 480: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto metadata_opt = loadModel(model_id);
- Line 484: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto data = config_.db->get(key);
- Line 544: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto data = config_.db->get(key);
- Line 578: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto metadata_opt = loadModel(model_id);
- Line 611: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto data = config_.db->get(key);
- Line 677: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::optional<LLMModelMetadata> LLMModelStorage::loadModel(const std::string& model_id) {
- Line 678: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return impl_->loadModel(model_id);
- Line 681: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::optional<std::vector<uint8_t>> LLMModelStorage::loadModelBlob(const std::string& model_id) {
- Line 682: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return impl_->loadModelBlob(model_id);
- Line 728: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool success = config_.db->put(edge_key, edge_bytes);
- Line 730: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: spdlog::info("Added edge: {} -> {} (type={})", from_id, to_id, static_cast<int>(edge_type));
- Line 768: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto edge_data = config_.db->get(key);
- Line 829: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool success = config_.db->put(embedding_key, embedding_bytes);
- Line 855: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto query_data = config_.db->get(embedding_key);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #4308 fix(llm): merge dev
- Line 308: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: metadata.total_tokens_generated = entity.getFieldAsInt("total_tokens_generated").value_or(0);
- Line 482: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Load metadata to get blob reference

            auto metadata_opt = loadModel(model_id);

            if (metadata_opt && config_.blob_manager) {

                // Try to delete blob if it exists

                std::string key = config_.key_prefix + model_id;

                auto data = config_.db->get(key);

                if (data) {
- Line 482: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Try to delete blob if it exists
- Line 513: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::warn("Failed to delete blob for model {}: {}", model_id, e.what());
- Line 513: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: spdlog::info("Deleted blob for model {}", model_id);

                        }

                    } catch (const std::exception& e) {

                        spdlog::warn("Failed to delete blob for model {}: {}", model_id, e.what());

                    }

                }

            }
- Line 513: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::warn("Failed to delete blob for model {}: {}", model_id, e.what());
- Line 528: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::error("Failed to delete model {}", model_id);
- Line 528: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (success) {

                spdlog::info("Model {} deleted successfully", model_id);

            } else {

                spdlog::error("Failed to delete model {}", model_id);

            }

            

            return success;
- Line 528: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::error("Failed to delete model {}", model_id);
- Line 533: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::error("Failed to delete model {}: {}", model_id, e.what());
- Line 533: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return success;

        } catch (const std::exception& e) {

            spdlog::error("Failed to delete model {}: {}", model_id, e.what());

            return false;

        }

    }
- Line 533: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::error("Failed to delete model {}: {}", model_id, e.what());
- Line 528: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("Failed to delete model {}", model_id);

### llm/lora_framework/lora_storage_service_themisdb.cpp
Total findings: 38

- Line 134: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto data = config_.db->get(key);
- Line 155: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool blob_deleted = config_.blob_manager->remove(ref);
- Line 168: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::warn("Failed to deserialize or delete blob for adapter {}: {}",
- Line 175: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool success = config_.db->del(key);
- Line 215: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto it_result = config_.db->newIterator();
- Line 283: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto data = config_.db->get(versioned_key);
- Line 291: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool ok = config_.db->put(current_key, *data);
- Line 348: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto it_result = config_.db->newIterator();
- Line 397: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto data = config_.db->get(key);
- Line 403: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize entity
- Line 404: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: BaseEntity entity = BaseEntity::deserialize(adapter_id, *data);
- Line 667: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto blob_ref = config_.blob_manager->put(adapter_id, weights.data);
- Line 679: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto encrypted = encryption_->encrypt(data_to_store, config_.encryption_key_id);
- Line 692: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool success = config_.db->put(key, data_to_store);
- Line 718: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto data = config_.db->get(key);
- Line 729: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize the EncryptedBlob from base64 string
- Line 763: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto blob_data = config_.blob_manager->get(ref);
- Line 789: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto data = config_.db->get(key);
- Line 795: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: BaseEntity entity = BaseEntity::deserialize(adapter_id, *data);
- Line 157: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::warn("Failed to delete blob {} for adapter {}",
- Line 157: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!ref.uri.empty()) {

                                    bool blob_deleted = config_.blob_manager->remove(ref);

                                    if (!blob_deleted) {

                                        spdlog::warn("Failed to delete blob {} for adapter {}", 

                                                    ref.uri, adapter_id);

                                        // Continue with metadata deletion for idempotency

                                    } else {
- Line 157: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::warn("Failed to delete blob {} for adapter {}",
- Line 168: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::warn("Failed to deserialize or delete blob for adapter {}: {}",
- Line 168: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

                        }

                    } catch (const std::exception& e) {

                        spdlog::warn("Failed to deserialize or delete blob for adapter {}: {}", 

                                    adapter_id, e.what());

                        // Continue with metadata deletion

                    }
- Line 168: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::warn("Failed to deserialize or delete blob for adapter {}: {}",
- Line 180: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::warn("Failed to delete metadata for adapter: {}", adapter_id);
- Line 180: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (success) {

                    spdlog::info("Deleted adapter: {}", adapter_id);

                } else {

                    spdlog::warn("Failed to delete metadata for adapter: {}", adapter_id);

                }

                

                return success;
- Line 180: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::warn("Failed to delete metadata for adapter: {}", adapter_id);
- Line 194: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::error("Failed to delete adapter {}: {}", adapter_id, e.what());
- Line 194: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return false;

            }

        } catch (const std::exception& e) {

            spdlog::error("Failed to delete adapter {}: {}", adapter_id, e.what());

            return false;

        }

    }
- Line 194: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::error("Failed to delete adapter {}: {}", adapter_id, e.what());
- Line 234: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(adapters.begin(), adapters.end(), adapter_id) == adapters.end()) {
- Line 240: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(config_.filesystem_path)) {
- Line 270: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 374: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(adapter_dir)) {
- Line 10: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_storage_service.h"
- Line 180: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::warn("Failed to delete metadata for adapter: {}", adapter_id);
- Line 865: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: weights_file.close();

### llm/llm_plugin_manager.cpp
Total findings: 36

- Line 72: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::info("Set '{}' as new default LLM plugin", plugin_name);
- Line 254: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto response = plugin->generateRAG(rag_context, request);
- Line 275: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool LLMPluginManager::loadModel(const std::string& model_id, const std::string& path) {
- Line 278: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::error("LLMPluginManager::loadModel: model_id or path is empty");
- Line 317: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::error("LLMPluginManager::loadModel: path '{}' is outside "
- Line 322: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::warn("LLMPluginManager::loadModel: THEMIS_MODEL_ROOT '{}' cannot "
- Line 335: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::warn("LLMPluginManager::loadModel: no default LLM plugin available; model '{}' not loaded",
- Line 339: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const bool ok = plugin->loadModel(path);
- Line 356: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: void LLMPluginManager::unloadModel(const std::string& model_id) {
- Line 374: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: plugin->unloadModel();
- Line 494: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return loadModel(model_id, model_id);
- Line 663: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (!plugin->loadModel(model_path, config)) {
- Line 712: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const bool ok = loadModel(model_id, path);
- Line 717: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: {"message", "loadModel() returned false for model_id: " + model_id}}.dump();
- Line 72: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: spdlog::info("Set '{}' as new default LLM plugin", plugin_name);
- Line 72: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 226: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: InferenceResponse LLMPluginManager::generate(const InferenceRequest& request) {
- Line 229: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: InferenceResponse LLMPluginManager::generate(const InferenceRequest& request) {

    auto* plugin = getDefaultPlugin();

    if (!plugin) {

        throw std::runtime_error("No default LLM plugin available");

    }

    

    return plugin->generate(request);
- Line 232: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return plugin->generate(request);
- Line 241: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ) {

    spdlog::info(

        "LLMPluginManager::generateRAG start: model='{}' docs={} top_k={} max_context_tokens={} response_budget_tokens={} request_max_tokens={}",

        request.model_id.empty() ? std::string{"default"} : request.model_id,

        rag_context.documents.size(),

        rag_context.top_k,

        rag_context.max_context_tokens,
- Line 251: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto* plugin = getDefaultPlugin();

    if (!plugin) {

        spdlog::warn("LLMPluginManager::generateRAG failed: no default plugin available");

        throw std::runtime_error("No default LLM plugin available");

    }



    auto response = plugin->generateRAG(rag_context, request);
- Line 254: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = plugin->generateRAG(rag_context, request);
- Line 256: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: "LLMPluginManager::generateRAG complete: success={} tokens_generated={} inference_time_ms={:.2f} cache_hit={} error_len={}",
- Line 269: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::vector<float> LLMPluginManager::embed(const std::string& text) {

    auto* plugin = getDefaultPlugin();

    if (!plugin) {

        throw std::runtime_error("No default LLM plugin available");

    }

    

    return plugin->embed(text);
- Line 339: severity=HIGH; category=pointer_without_null_check
  Description: Potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer dereference without null check
- Line 372: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: plugin = getDefaultPluginLocked();

    }

    if (!plugin) {

        throw std::runtime_error("No default LLM plugin available");

    }

    plugin->unloadModel();

}
- Line 374: severity=HIGH; category=pointer_without_null_check
  Description: Potential null-pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Pointer dereference without null check
- Line 479: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::vector<std::string> LLMPluginManager::generateStream(const InferenceRequest& request) {

    auto* plugin = getDefaultPlugin();

    if (!plugin) {

        throw std::runtime_error("No default LLM plugin available");

    }

    // If backend lacks streaming, degrade to single generate and split tokens

    auto response = plugin->generate(request);
- Line 482: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = plugin->generate(request);
- Line 482: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = plugin->generate(request);
- Line 753: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        });



    // ── Session-delete callback ────────────────────────────────────────────────

    // `resource_id` is the session/request ID extracted from the DELETE path by

    // MetricsServer (e.g. DELETE /admin/sessions/req-42 → "req-42").

    // The actual cancellation is delegated to cancel_session_cb_, which must be
- Line 753: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // ── Session-delete callback ────────────────────────────────────────────────
- Line 226: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: InferenceResponse LLMPluginManager::generate(const InferenceRequest& request) {
- Line 266: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<float> LLMPluginManager::embed(const std::string& text) {
- Line 272: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return plugin->embed(text);
- Line 482: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = plugin->generate(request);

### llm/ml_model_manager.cpp
Total findings: 36

- Line 442: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: eng_req.base_request.prompt = request.input_data.get<std::string>();
- Line 829: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: CachedModel* cached = config_.model_loader->getOrLoadModel(
- Line 829: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: CachedModel* cached = config_.model_loader->getOrLoadModel(
- Line 861: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Only unload model weights from LazyModelLoader when this is the
- Line 864: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: config_.model_loader->unloadModel(model_id);
- Line 134: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 140: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 146: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 151: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 169: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 201: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(models_mutex_);
- Line 224: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 385: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: Result<MLInferenceResponse> MLModelManager::infer(const MLInferenceRequest& request) {
- Line 430: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: InferenceEngineEnhanced::EnhancedInferenceRequest eng_req;

        eng_req.base_request.model_id   = request.model_id;

        eng_req.base_request.max_tokens =

            request.inference_params.value("max_tokens", 512);

        eng_req.base_request.temperature =

            static_cast<float>(request.inference_params.value("temperature", 0.7));

        if (request.input_data.is_object()) {
- Line 432: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: eng_req.base_request.max_tokens =

            request.inference_params.value("max_tokens", 512);

        eng_req.base_request.temperature =

            static_cast<float>(request.inference_params.value("temperature", 0.7));

        if (request.input_data.is_object()) {

            if (request.input_data.contains("prompt")) {

                eng_req.base_request.prompt =
- Line 433: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.inference_params.value("max_tokens", 512);

        eng_req.base_request.temperature =

            static_cast<float>(request.inference_params.value("temperature", 0.7));

        if (request.input_data.is_object()) {

            if (request.input_data.contains("prompt")) {

                eng_req.base_request.prompt =

                    request.input_data["prompt"].get<std::string>();
- Line 434: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: eng_req.base_request.temperature =

            static_cast<float>(request.inference_params.value("temperature", 0.7));

        if (request.input_data.is_object()) {

            if (request.input_data.contains("prompt")) {

                eng_req.base_request.prompt =

                    request.input_data["prompt"].get<std::string>();

            } else if (request.input_data.contains("text")) {
- Line 436: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (request.input_data.is_object()) {

            if (request.input_data.contains("prompt")) {

                eng_req.base_request.prompt =

                    request.input_data["prompt"].get<std::string>();

            } else if (request.input_data.contains("text")) {

                eng_req.base_request.prompt =

                    request.input_data["text"].get<std::string>();
- Line 437: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (request.input_data.contains("prompt")) {

                eng_req.base_request.prompt =

                    request.input_data["prompt"].get<std::string>();

            } else if (request.input_data.contains("text")) {

                eng_req.base_request.prompt =

                    request.input_data["text"].get<std::string>();

            }
- Line 439: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: request.input_data["prompt"].get<std::string>();

            } else if (request.input_data.contains("text")) {

                eng_req.base_request.prompt =

                    request.input_data["text"].get<std::string>();

            }

        } else if (request.input_data.is_string()) {

            eng_req.base_request.prompt = request.input_data.get<std::string>();
- Line 441: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: eng_req.base_request.prompt =

                    request.input_data["text"].get<std::string>();

            }

        } else if (request.input_data.is_string()) {

            eng_req.base_request.prompt = request.input_data.get<std::string>();

        }

        eng_req.priority           = request.priority;
- Line 442: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.input_data["text"].get<std::string>();

            }

        } else if (request.input_data.is_string()) {

            eng_req.base_request.prompt = request.input_data.get<std::string>();

        }

        eng_req.priority           = request.priority;

        eng_req.timeout            = std::chrono::milliseconds(request.timeout_ms);
- Line 502: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto result = this->infer(request);
- Line 502: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto result = this->infer(request);
- Line 616: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Deploy new instance
- Line 616: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 728: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(models_mutex_);
- Line 748: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(models_mutex_);
- Line 853: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find_if(instances.begin(), instances.end(),
- Line 107: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: instance_ids.push_back(result.value());
- Line 197: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool drained = false;
- Line 332: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(*inst);
- Line 385: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: Result<MLInferenceResponse> MLModelManager::infer(const MLInferenceRequest& request) {
- Line 496: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 502: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto result = this->infer(request);
- Line 888: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Load Balancing").' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md §"Load Balancing").

### llm/lora_framework/lora_training_service.cpp
Total findings: 33

- Line 130: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize from JSON
- Line 277: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: inst_sample.instruction = sample.input;
- Line 754: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: float current_lr = lr_scheduler->get_lr(global_step);
- Line 1101: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize checkpoint
- Line 1481: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: inst_sample.instruction = sample.input;
- Line 1794: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: quantized_model->embedding_dim = emb_dim;
- Line 1840: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Load weights for each layer
- Line 1841: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // In production, this would load actual quantized weights
- Line 1843: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint32_t emb_dim = quantized_model->embedding_dim > 0 ?
- Line 2097: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool initialized = coord->initialize(adapter_id, training_config);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #745 Integrate ShardRouter and S... (2026-03-23) | #757 [WIP] Implement real
- Line 517: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (local_config.lr_scheduler.type != SchedulerType::CONSTANT || local_config.lr_scheduler.base_lr != 1e-4f) {
- Line 524: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Fall back to params-based scheduler for backward compatibility
- Line 582: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (stop_requested_.load(std::memory_order_acquire)) {
- Line 594: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Reset data loader for new epoch
- Line 594: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 600: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (step % 10 == 0 && stop_requested_.load(std::memory_order_acquire)) {
- Line 802: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: Tensor grad_output = compute_mse_gradient(predictions, batch_target);
- Line 888: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (stop_requested_.load(std::memory_order_acquire)) {
- Line 898: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (stop_requested_.load(std::memory_order_acquire)) {
- Line 1011: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: while (is_training_.load(std::memory_order_acquire)) {
- Line 1016: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 1145: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @return Flattened embedding tensor [batch_size * hidden_dim]
- Line 1304: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Step 1: Model Compatibility Check
- Line 1703: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: return read_exact(reinterpret_cast<char*>(&value), static_cast<std::streamsize>(sizeof(uint64_t)));
- Line 2: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * ThemisDB | File: lora_training_service.cpp | Version: 0.0.47 | Last Modified: 2026-06-01 04:22:09
- Line 10: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_training_service.h"
- Line 11: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_storage_service.h"
- Line 1314: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: error += "  - " + err + "\n";
- Line 1315: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: error += "  - " + err + "\n";
- Line 1807: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: layer_names.push_back("blk." + std::to_string(j) + ".attn.wq");
- Line 1808: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: layer_names.push_back("blk." + std::to_string(j) + ".attn.wv");
- Line 769: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: //   3. Apply LoRA adapter on top of base model outputs

### llm/distributed_training_coordinator.cpp
Total findings: 32

- Line 426: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Deserialize loss metrics
- Line 910: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string rpc_query = "collect_gradients:" + request.dump();
- Line 910: severity=CRITICAL; category=sql_injection
  Description: string_concat_sql: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string rpc_query = "collect_gradients:" + request.dump();
- Line 1137: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string rpc_query = "apply_gradients:" + request.dump();
- Line 1137: severity=CRITICAL; category=sql_injection
  Description: string_concat_sql: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string rpc_query = "apply_gradients:" + request.dump();
- Line 1212: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string rpc_query = "health_check:" + request.dump();
- Line 1212: severity=CRITICAL; category=sql_injection
  Description: string_concat_sql: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string rpc_query = "health_check:" + request.dump();
- Line 1535: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: available_shard_ids.insert(shard_info.shard_id);
- Line 1555: severity=CRITICAL; category=sql_injection
  Description: string_concat_sql: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string rpc_query = "ping:" + ping_request.dump();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4833 Continue Phase-6 tensorgrap... (2026-05-07) | #745 Integrate ShardRoute
- Line 617: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = shard_weights_.find(grad.source_shard);
- Line 910: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string rpc_query = "collect_gradients:" + request.dump();
- Line 911: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: json response = shard_router_->executeQuery(rpc_query);
- Line 1001: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: break;

                        

                    case ByzantineAction::SHUTDOWN:

                        throw std::runtime_error(

                            fmt::format("Byzantine shards detected ({}), shutting down training",

                                      fmt::join(detection_result.suspected_shards, ", ")));

                }
- Line 1137: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string rpc_query = "apply_gradients:" + request.dump();
- Line 1137: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

                

                // Send RPC request to apply gradients

                std::string rpc_query = "apply_gradients:" + request.dump();

                json response = shard_router_->executeQuery(rpc_query);

                

                // Check if successful
- Line 1138: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: json response = shard_router_->executeQuery(rpc_query);
- Line 1160: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: bool success = future.get();
- Line 1212: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string rpc_query = "health_check:" + request.dump();
- Line 1212: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: };

            

            // Send RPC request

            std::string rpc_query = "health_check:" + request.dump();

            json response = shard_router_->executeQuery(rpc_query);

            

            // Parse health response
- Line 1213: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: json response = shard_router_->executeQuery(rpc_query);
- Line 1555: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::string rpc_query = "ping:" + ping_request.dump();
- Line 1555: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: ).count()}

                };

                

                std::string rpc_query = "ping:" + ping_request.dump();

                json response = shard_router_->executeQuery(rpc_query);

                

                if (!response.contains("success") || !response["success"].get<bool>()) {
- Line 1556: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: json response = shard_router_->executeQuery(rpc_query);
- Line 217: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '            uint32_t count = static_cast<uint32_t>(k);', '            compressed.push_back((count >> 24) & 0xFF);', '            compressed.push_back((count >> 16) & 0xFF);', '            compressed.push_back((count >> 8) & 0xFF);']
- Line 218: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            uint32_t count = static_cast<uint32_t>(k);', '            compressed.push_back((count >> 24) & 0xFF);', '            compressed.push_back((count >> 16) & 0xFF);', '            compressed.push_back((count >> 8) & 0xFF);', '            compressed.push_back(count & 0xFF);']
- Line 219: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            compressed.push_back((count >> 24) & 0xFF);', '            compressed.push_back((count >> 16) & 0xFF);', '            compressed.push_back((count >> 8) & 0xFF);', '            compressed.push_back(count & 0xFF);', '']
- Line 228: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['                memcpy(&val_bits, &val, sizeof(float));', '', '                compressed.push_back((idx >> 24) & 0xFF);', '                compressed.push_back((idx >> 16) & 0xFF);', '                compressed.push_back((idx >> 8) & 0xFF);']
- Line 229: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '                compressed.push_back((idx >> 24) & 0xFF);', '                compressed.push_back((idx >> 16) & 0xFF);', '                compressed.push_back((idx >> 8) & 0xFF);', '                compressed.push_back(idx & 0xFF);']
- Line 230: severity=MEDIUM; category=shift_overflow
  Description: Shift operation detected (verify shift count < bitwidth)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['                compressed.push_back((idx >> 24) & 0xFF);', '                compressed.push_back((idx >> 16) & 0xFF);', '                compressed.push_back((idx >> 8) & 0xFF);', '                compressed.push_back(idx & 0xFF);', '                compressed.push_back((val_bits >> 24) & 0xFF);']
- Line 1342: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto stats_json = checkpoint["stats"];
- Line 1466: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, std::vector<GradientTensor>>& shard_gradients,

### llm/multi_perspective_generator.cpp
Total findings: 31

- Line 207: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (impl_->cache.size() >= impl_->config.max_cache_size) {
- Line 211: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->cache[query] = result;
- Line 395: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (selected.size() >= static_cast<size_t>(impl_->config.max_perspectives)) {
- Line 874: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_diversity_score = result.perspective_diversity_score;
- Line 875: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_perspectives_per_query =
- Line 877: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_generation_time = result.generation_time;
- Line 879: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: float n = static_cast<float>(impl_->stats.total_generations);
- Line 880: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_diversity_score =
- Line 883: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_perspectives_per_query =
- Line 887: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto avg_ms = impl_->stats.avg_generation_time.count() * (n - 1) +
- Line 889: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_generation_time = std::chrono::milliseconds(
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12)
- Line 360: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find_if(
- Line 388: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find_if(
- Line 97: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 366: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: selected.push_back(*it);
- Line 394: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: selected.push_back(*it);
- Line 587: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> words_i = extractWords(perspectives[i].response);
- Line 588: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> words_j = extractWords(perspectives[j].response);
- Line 591: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> intersection;
- Line 592: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> union_set;
- Line 633: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> theme_counts;
- Line 648: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: common_themes.push_back(theme);
- Line 686: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> traditions;
- Line 693: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: disagreements.push_back("Consequentialist outcomes versus adherence to universal principles");
- Line 698: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: disagreements.push_back("Focus on character development versus specific action evaluation");
- Line 704: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: disagreements.push_back("Contextual care versus impartial rule application");
- Line 709: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: disagreements.push_back("Different emphases on key ethical considerations");
- Line 754: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 802: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool MultiPerspectiveGenerator::detectEthicalQuery(const std::string& query) {
- Line 865: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: key_points.push_back(perspective.key_principles[0]);

### llm/active_vram_allocator.cpp
Total findings: 29

- Line 15: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: * - Real GPU memory allocation via GPUMemoryManager (cudaMalloc / fallback)
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4370 [WIP] Update llm documentat... (2026-03-21) | #3802 [LLM] AdaptiveVRAMA
- Line 15: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: * - Real GPU memory allocation via GPUMemoryManager (cudaMalloc / fallback)
- Line 65: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: * Uses `cudaMemcpy(DeviceToHost)` / `cudaMemcpy(HostToDevice)` when CUDA is
- Line 85: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: cudaMemcpyKind kind = device_to_host
- Line 86: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: ? cudaMemcpyDeviceToHost
- Line 87: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: : cudaMemcpyHostToDevice;
- Line 90: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: spdlog::warn("[ActiveVRAMAllocator] cudaMemcpy failed: {}",
- Line 155: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::optional<AllocationHandle> allocate(
- Line 161: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::warn("[ActiveVRAMAllocator] Attempted to allocate 0 bytes for '{}'",
- Line 186: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void* gpu_ptr = gpu_mgr_->allocateGPU(
- Line 201: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: h.allocated_bytes = aligned;
- Line 206: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: h.allocated_at_ms = nowMs();
- Line 207: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: h.last_used_at_ms = h.allocated_at_ms;
- Line 314: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = allocations_.find(id);
- Line 386: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // Copy data from CPU back to GPU (uses cudaMemcpy when CUDA is available)
- Line 465: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: h.allocated_bytes = bytes;   // no alignment padding for external allocs
- Line 471: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: h.allocated_at_ms = nowMs();
- Line 472: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: h.last_used_at_ms = h.allocated_at_ms;
- Line 493: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool allocateWithFragmentation(size_t bytes, void** ptr) {
- Line 496: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto handle = allocateOrRecover(bytes, "__bridge__", -1);
- Line 694: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: handle.allocated_bytes, stats_.used_vram_bytes);
- Line 777: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: ActiveVRAMAllocator::allocate(size_t bytes, const std::string& owner_id, int gpu_device_id)
- Line 865: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool ActiveVRAMAllocator::allocateWithFragmentation(size_t bytes, void** ptr)
- Line 867: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return impl_->allocateWithFragmentation(bytes, ptr);
- Line 735: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void notifyOOM(const OOMEvent& ev) {

        if (oom_cb_) {

            try { oom_cb_(ev); } catch (...) {}

        }

    }
- Line 735: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: try { oom_cb_(ev); } catch (...) {}
- Line 788: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool ActiveVRAMAllocator::free(AllocationHandle& handle)
- Line 790: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return impl_->free(handle);

### llm/constitutional_reasoning_engine.cpp
Total findings: 29

- Line 234: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto it = impl_->critique_cache.find(cache_key);
- Line 235: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != impl_->critique_cache.end() && !it->second.empty()) {
- Line 248: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->critique_cache[cache_key] = {llm_critique};
- Line 286: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->critique_cache[cache_key] = {critique};
- Line 694: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_original_score = result.original_score;
- Line 695: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_revised_score = result.revised_score;
- Line 696: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_improvement = result.improvement;
- Line 697: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_critique_time = result.critique_time;
- Line 698: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_revision_time = result.revision_time;
- Line 700: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: float n = static_cast<float>(impl_->stats.total_reasonings);
- Line 701: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_original_score =
- Line 703: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_revised_score =
- Line 705: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_improvement =
- Line 708: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto critique_ms = impl_->stats.avg_critique_time.count() * (n - 1) +
- Line 710: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_critique_time = std::chrono::milliseconds(
- Line 714: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto revision_ms = impl_->stats.avg_revision_time.count() * (n - 1) +
- Line 716: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_revision_time = std::chrono::milliseconds(
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12)
- Line 115: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = std::find_if(
- Line 115: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find_if(
- Line 171: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 203: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find(
- Line 755: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (lower.find(word) != std::string::npos) {
- Line 767: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (lower.find(phrase) != std::string::npos) {
- Line 786: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (lower.find(pattern) != std::string::npos) {
- Line 808: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "all [group] are", "typical [group]", "[group] always",
- Line 49: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 348: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: violations.push_back(principle.id);
- Line 626: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### llm/production_validator.cpp
Total findings: 29

- Line 95: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto handle   = inference_engine_->submit(eng_req);
- Line 336: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto handle = inference_engine_->submit(eng_req);
- Line 469: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (auto& t : threads) t.join();
- Line 1048: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* model = loader.getOrLoadModel("__nonexistent__", "/tmp/__no_such_model.gguf");
- Line 1079: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: req_lo.prompt = "low priority request";
- Line 1081: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: req_hi.prompt = "high priority request";
- Line 1229: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* m1 = loader.getOrLoadModel("switch_model_1", "/tmp/__switch_m1.gguf");
- Line 1230: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* m2 = loader.getOrLoadModel("switch_model_2", "/tmp/__switch_m2.gguf");
- Line 1316: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto* model = loader.getOrLoadModel("fail_test", "/nonexistent/path/model.gguf");
- Line 1434: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: req.prompt = "concurrency test request";
- Line 1445: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (auto& th : threads) th.join();
- Line 1477: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: req.prompt     = "Long-running request test";
- Line 90: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (inference_engine_) {

                InferenceEngineEnhanced::EnhancedInferenceRequest eng_req;

                eng_req.base_request.prompt     = prompt;

                eng_req.base_request.model_id   = model_id.empty() ? "default" : model_id;

                eng_req.base_request.max_tokens = 128;

                eng_req.timeout                 = std::chrono::milliseconds(30000);

                eng_req.preferred_model_id      = model_id;
- Line 331: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: bool success = true;

        if (inference_engine_) {

            InferenceEngineEnhanced::EnhancedInferenceRequest eng_req;

            eng_req.base_request.prompt     = "stress test iteration " + std::to_string(iteration);

            eng_req.base_request.model_id   = "default";

            eng_req.base_request.max_tokens = 32;

            eng_req.timeout                 = std::chrono::milliseconds(10000);
- Line 450: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::microseconds(500));
- Line 744: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(50));
- Line 1432: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (int i = 0; i < kPerThread; ++i) {
- Line 1439: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(id_mutex);
- Line 1653: severity=HIGH; category=windows_only_api
  Description: Windows-only API GetCurrentProcess without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
- Line 540: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: msg += "\n  - " + r;
- Line 541: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: msg += "\n  - " + r;
- Line 591: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool passed = false;
- Line 618: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool passed = false;
- Line 738: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < 4; ++i) {
- Line 1423: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kThreads  = 8;
- Line 1424: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kPerThread = 4;
- Line 1430: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int t = 0; t < kThreads; ++t) {
- Line 1432: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < kPerThread; ++i) {
- Line 1698: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread_local int reasoning_count = 0;

### llm/grafana_metrics.cpp
Total findings: 27

- Line 99: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = metrics_.find(key);
- Line 770: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (delta == 0.0) break;  // nothing new to report
- Line 1303: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: res.set_content(exporter_->handleMetricsRequest(), "text/plain; version=0.0.4");
- Line 1421: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: impl_->svr.stop();

    if (impl_->thread.joinable()) {

        impl_->thread.join();

    }



    running_ = false;
- Line 1421: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: impl_->thread.join();
- Line 1421: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: impl_->thread.join();
- Line 1517: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: R"({"status":"not_implemented","message":"No reload callback registered. Wire setReloadCallback() to LlamaWrapper::loadModel()."})";
- Line 5: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: * PR History (last 5): #3036 [llm] Unified metrics dashb... (2026-03-12) | #1295 Remove legacy query_parser.... (2026-03-11) | #689 Stabilize Extended Context ... (2026-03-11) | #215 Implement P1 LLM Inference ... (2026-03-11) | #214 Integrate Prometheus metric... (2026-03-11)
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3036 [llm] Unified metrics dashb... (2026-03-12) | #1295 Remove legacy query
- Line 751: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 756: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 762: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Counter reset detected: treat total_completed as the new delta.
- Line 764: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1400: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(10));
- Line 1449: severity=HIGH; category=insecure_model_url
  Description: Model downloaded over insecure HTTP
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return "http://" + config_.host + ":" + std::to_string(config_.port) + config_.models_path;
- Line 1475: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response = GrafanaDashboardGenerator(dcfg).generateUnifiedDashboard();
- Line 1546: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: R"({"status":"not_implemented","message":"No session-delete callback registered. Wire setSessionDele
- Line 1546: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const std::string& resource_id,

                                 std::string& response) {

    static constexpr const char* k_sessions_delete_not_impl =

        R"({"status":"not_implemented","message":"No session-delete callback registered. Wire setSessionDeleteCallback() to ContinuousBatchScheduler::cancelRequest()."})";



    if (path == config_.admin_sessions_path) {

        // DELETE /admin/sessions/{id} — cancel or remove the named session.
- Line 1546: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: R"({"status":"not_implemented","message":"No session-delete callback registered. Wire setSessionDeleteCallback() to ContinuousBatchScheduler::cancelRequest()."})";
- Line 121: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, MetricType> metric_types;
- Line 182: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto p50 = sorted[idx_p50];
- Line 183: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto p95 = sorted[idx_p95];
- Line 184: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto p99 = sorted[idx_p99];
- Line 217: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& labels) const {
- Line 1184: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int y = 0;
- Line 1399: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < 50 && !impl_->svr.is_running(); ++i) {
- Line 1546: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: R"({"status":"not_implemented","message":"No session-delete callback registered. Wire setSessionDele

### llm/lora_framework/kernels/hip_kernels.cpp
Total findings: 27

- Line 40: severity=CRITICAL; category=missing_sync_threads
  Description: Shared memory access in CUDA kernel without __syncthreads()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: __global__ void multiply_kernel(const float* A, const float* B, float* C, size_t size) {
- Line 447: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_overflow
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: hipLaunchKernelGGL(check_inf_nan_kernel, gridSize, blockSize, 0, 0, data, size, d_overflow);
- Line 462: severity=CRITICAL; category=use_after_free_gpu
  Description: Use of freed GPU memory: d_overflow
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: err = hipMemcpy(&h_overflow, d_overflow, sizeof(int), hipMemcpyDeviceToHost);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #575 [LoRA Phase 10.4] Implement... (2026-03-11) | #570 [LoRA Phase 10] Add r
- Line 350: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: [') {', '    int blockSize = 256;', '    int gridSize = (size + blockSize - 1) / blockSize;', '', '    if (stream != nullptr) {']
- Line 369: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: [') {', '    int blockSize = 256;', '    int gridSize = (size + blockSize - 1) / blockSize;', '', '    if (stream != nullptr) {']
- Line 388: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: [') {', '    int blockSize = 256;', '    int gridSize = (size + blockSize - 1) / blockSize;', '', '    if (stream != nullptr) {']
- Line 406: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: [') {', '    int blockSize = 256;', '    int gridSize = (size + blockSize - 1) / blockSize;', '', '    if (stream != nullptr) {']
- Line 447: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    // Launch kernel', '    int blockSize = 256;', '    int gridSize = (size + blockSize - 1) / blockSize;', '    hipLaunchKernelGGL(check_inf_nan_kernel, gridSize, blockSize, 0, 0, data, size, d_overflow);', '']
- Line 753: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * Each thread processes one element in the output [batch_size, hidden_dim]
- Line 777: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['                              seq_idx * hidden_dim +', '                              hidden_idx;', '            sum += input[input_idx];', '        }', '']
- Line 129: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float sum = 0.0f;
- Line 154: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int k = 0; k < 16; k++) {
- Line 187: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float sum = 0.0f;
- Line 223: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float sum = 0.0f;
- Line 256: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float local_sum = 0.0f;
- Line 603: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: RocblasHandle::RocblasHandle() {
- Line 613: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: RocblasHandle::~RocblasHandle() {
- Line 619: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: RocblasHandle::RocblasHandle(RocblasHandle&& other) noexcept
- Line 720: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: const int threads_per_block = 256;
- Line 772: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float sum = 0.0f;
- Line 795: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: const int threads_per_block = 256;
- Line 852: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: const int threads_per_block = 256;
- Line 765: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: size_t total_outputs = batch_size * hidden_dim;
- Line 767: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (idx < total_outputs) {
- Line 792: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: size_t total_outputs = batch_size * hidden_dim;
- Line 796: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const int num_blocks = (total_outputs + threads_per_block - 1) / threads_per_block;

### llm/vision_encoder.cpp
Total findings: 27

- Line 75: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (config_->getAPIStability() == VisionAPIStability::EXPERIMENTAL) {
- Line 77: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: } else if (config_->getAPIStability() == VisionAPIStability::DEPRECATED) {
- Line 83: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto license = config_->getModelLicense(model_id_);
- Line 100: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& limits = config_->getResourceLimits();
- Line 108: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& mv = config_->getSecurityConfig().model_verification;
- Line 173: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto license = config_->getModelLicense(model_id_);
- Line 187: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::debug("Using legacy VisionEncoder constructor - consider upgrading to new API");
- Line 190: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 286: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: int n_threads = config_->getResourceLimits().cpu_inference_threads;
- Line 413: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: info += "\n  API Version: " + config_->getAPIVersion();
- Line 414: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto license = config_->getModelLicense(model_id_);
- Line 435: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& validation = config_->getSecurityConfig().validation;
- Line 472: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& validation = config_->getSecurityConfig().validation;
- Line 489: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& validation = config_->getSecurityConfig().validation;
- Line 514: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& validation = config_->getSecurityConfig().validation;
- Line 154: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: spdlog::info("Loading CLIP model from {}", clip_model_path);

    clip_ctx_ = clip_model_load(clip_model_path.c_str(), verbosity);

    if (!clip_ctx_) {

        throw std::runtime_error("Failed to load CLIP model: " + clip_model_path);

    }

    

    initialized_ = true;
- Line 183: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy constructor for backward compatibility
- Line 190: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: VisionEncoder::~VisionEncoder() {
- Line 323: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: throw; // Re-throw

    }

#else

    throw std::runtime_error("VisionEncoder: LLM support not enabled (THEMIS_ENABLE_LLM=OFF)");

#endif

}
- Line 26: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 31: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 39: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void clip_free(clip_ctx* ctx);
- Line 42: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void clip_image_u8_free(clip_image_u8* img);
- Line 45: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void clip_image_f32_free(clip_image_f32* img);
- Line 219: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: clip_free(clip_ctx_);
- Line 577: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: clip_image_u8_free(img_u8);
- Line 585: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: clip_image_f32_free(img_f32);

### llm/json_schema_converter.cpp
Total findings: 25

- Line 128: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 133: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 188: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 235: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 241: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 285: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 307: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 24: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: string ::= "\"" ([^"\\] | "\\" (["\\/bfnrt] | "u" [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F] [0-9a-fA-F]))*
- Line 46: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"':  result += "\\\""; break;
- Line 47: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  result += "\\\""; break;
- Line 48: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': result += "\\\\"; break;
- Line 49: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': result += "\\n";  break;
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': result += "\\r";  break;
- Line 51: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': result += "\\t";  break;
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += '_';
- Line 65: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += '_';
- Line 89: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!first) body += " | ";
- Line 90: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!first) body += " | ";
- Line 93: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += "\"\\\"" + escapeGbnfString(elem.get<std::string>()) + "\\\"\"";
- Line 97: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += "\"null\"";
- Line 100: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += "\"" + elem.dump() + "\"";
- Line 102: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: body += "value";
- Line 149: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> required_set;
- Line 166: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = schema["properties"].begin(); it != schema["properties"].end(); ++it) {
- Line 195: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!required_part.empty()) required_part += " ws \",\" ws ";

### llm/meta_prompt_generator.cpp
Total findings: 25

- Line 111: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    result.metadata["score"] = score;

    result.metadata["strategy"] = config_.improvement_strategy;

    result.metadata["original_length"] = original_prompt.length();

    

    THEMIS_DEBUG("Generated meta-prompt of length {}", result.meta_prompt.length());
- Line 112: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["score"] = score;

    result.metadata["strategy"] = config_.improvement_strategy;

    result.metadata["original_length"] = original_prompt.length();

    

    THEMIS_DEBUG("Generated meta-prompt of length {}", result.meta_prompt.length());
- Line 55: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: meta_prompt << "Current Score: " << score << " / 1.0\n";
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Break down instructions into numbered steps");
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Use clear, simple language");
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Define any technical terms");
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Add 2-3 concrete examples");
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Show both simple and complex cases");
- Line 167: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Highlight key patterns in examples");
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Specify exact output format");
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Provide output template or schema");
- Line 174: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Include formatting requirements");
- Line 179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("List explicit constraints");
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Define boundary conditions");
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Specify error handling requirements");
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Review and simplify language");
- Line 187: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Add structure with headers");
- Line 188: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back("Include validation criteria");
- Line 237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("High-performing prompts include concrete examples");
- Line 241: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Successful prompts use step-by-step instructions");
- Line 245: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Clear output format specifications improve performance");
- Line 249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Explicit constraints help guide the model");
- Line 253: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Structure prompts with clear sections (task, examples, output)");
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Use precise, unambiguous language");
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: patterns.push_back("Include both positive and negative examples when relevant");

### llm/moral_analyzer.cpp
Total findings: 24

- Line 266: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto status = graph_manager_->addNode(arg_entity, graph_id);
- Line 279: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto edge_status = graph_manager_->addEdge(arg_edge, graph_id);
- Line 344: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: decision.reasoning_path = best_it->second;
- Line 807: severity=CRITICAL; category=sql_injection
  Description: string_concat_sql: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: audit.query = "Ethical scenario: " + decision.scenario_id;
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #966 Implement philosophy
- Line 807: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: audit.query = "Ethical scenario: " + decision.scenario_id;
- Line 1222: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(recommendations.begin(), recommendations.end(), rec) ==
- Line 1241: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(unique_recs.begin(), unique_recs.end(), rec) == unique_recs.end()) {
- Line 886: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: static const std::unordered_set<std::string> stopwords = {
- Line 923: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (decision.metrics.consistency > 0.8) keywords.push_back("high_consistency");
- Line 924: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (decision.metrics.fairness > 0.8) keywords.push_back("high_fairness");
- Line 925: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (decision.metrics.feasibility > 0.8) keywords.push_back("highly_feasible");
- Line 926: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (decision.metrics.long_term_impact > 0.8) keywords.push_back("high_impact");
- Line 1053: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> MoralAnalyzer::calculateStakeholderImpacts(
- Line 1057: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> impacts;
- Line 1158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: recommendations.push_back("kant");
- Line 1166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: recommendations.push_back("utilitarian");
- Line 1173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: recommendations.push_back("virtue");
- Line 1180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: recommendations.push_back("care_ethics");
- Line 1333: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fallback_philosophies.push_back("kant");
- Line 1337: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fallback_philosophies.push_back("utilitarian");
- Line 1340: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fallback_philosophies.push_back("virtue");
- Line 1343: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fallback_philosophies.push_back("care_ethics");
- Line 1347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: fallback_philosophies.push_back("rawls");

### llm/llm_deployment_plugin.cpp
Total findings: 23

- Line 173: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: loadModelRegistry();
- Line 334: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: ModelDownloadResult LLMDeploymentPlugin::downloadModel(const std::string& model_id,
- Line 416: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool LLMDeploymentPlugin::loadModel(const std::string& model_id, ILLMPlugin* llm_plugin) {
- Line 433: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (llm_plugin->loadModel(status->model_path, config)) {
- Line 866: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: void LLMDeploymentPlugin::loadModelRegistry() {
- Line 1101: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Only load model weights when explicitly configured to do so (store_weights_in_rocksdb).
- Line 1137: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::optional<LLMModelMetadata> LLMDeploymentPlugin::loadModelFromStorage(const std::string& model_id) {
- Line 1143: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return model_storage_->loadModel(model_id);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4308 fix(llm): merge develop, re... (2026-03-19) | #4304 [LLM-DEP-123] Imple
- Line 1068: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract metadata from custom metadata field

        if (status.metadata.contains("architecture")) {

            metadata.architecture = status.metadata["architecture"];

        }

        if (status.metadata.contains("quantization")) {

            metadata.quantization = status.metadata["quantization"];
- Line 1071: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata.architecture = status.metadata["architecture"];

        }

        if (status.metadata.contains("quantization")) {

            metadata.quantization = status.metadata["quantization"];

        }

        if (status.metadata.contains("parameter_count")) {

            metadata.parameter_count = status.metadata["parameter_count"];
- Line 1074: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata.quantization = status.metadata["quantization"];

        }

        if (status.metadata.contains("parameter_count")) {

            metadata.parameter_count = status.metadata["parameter_count"];

        }

        if (status.metadata.contains("context_length")) {

            metadata.context_length = status.metadata["context_length"];
- Line 1077: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata.parameter_count = status.metadata["parameter_count"];

        }

        if (status.metadata.contains("context_length")) {

            metadata.context_length = status.metadata["context_length"];

        }

        if (status.metadata.contains("capabilities")) {

            metadata.capabilities = status.metadata["capabilities"].get<std::vector<std::string>>();
- Line 1080: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata.context_length = status.metadata["context_length"];

        }

        if (status.metadata.contains("capabilities")) {

            metadata.capabilities = status.metadata["capabilities"].get<std::vector<std::string>>();

        }

        if (status.metadata.contains("languages")) {

            metadata.languages = status.metadata["languages"].get<std::vector<std::string>>();
- Line 1083: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata.capabilities = status.metadata["capabilities"].get<std::vector<std::string>>();

        }

        if (status.metadata.contains("languages")) {

            metadata.languages = status.metadata["languages"].get<std::vector<std::string>>();

        }

        if (status.metadata.contains("tags")) {

            metadata.tags = status.metadata["tags"].get<std::vector<std::string>>();
- Line 1086: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata.languages = status.metadata["languages"].get<std::vector<std::string>>();

        }

        if (status.metadata.contains("tags")) {

            metadata.tags = status.metadata["tags"].get<std::vector<std::string>>();

        }

        if (status.metadata.contains("source")) {

            metadata.source = status.metadata["source"];
- Line 1089: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata.tags = status.metadata["tags"].get<std::vector<std::string>>();

        }

        if (status.metadata.contains("source")) {

            metadata.source = status.metadata["source"];

        }

        if (status.metadata.contains("source_url")) {

            metadata.source_url = status.metadata["source_url"];
- Line 1092: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata.source = status.metadata["source"];

        }

        if (status.metadata.contains("source_url")) {

            metadata.source_url = status.metadata["source_url"];

        }

        if (status.metadata.contains("license")) {

            metadata.license = status.metadata["license"];
- Line 1095: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata.source_url = status.metadata["source_url"];

        }

        if (status.metadata.contains("license")) {

            metadata.license = status.metadata["license"];

        }

        

        // Store custom metadata
- Line 104: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool is_set = false;
- Line 611: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: to_remove.push_back(status.model_id);
- Line 954: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: src_path += ".gguf";
- Line 955: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: src_path += ".gguf";

### llm/lora_framework/kernels/hip_fused_kernels.cpp
Total findings: 23

- Line 222: severity=CRITICAL; category=missing_sync_threads
  Description: Shared memory access in CUDA kernel without __syncthreads()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: ) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #573 Implement kernel fus
- Line 218: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: * 

 * Fuses: gradient + weight_decay + momentum + update

 * With momentum: p = p - lr * ((1-momentum) * g + weight_decay * p + momentum * v)

 * Without momentum: p = p - lr * (g + weight_decay * p)

 * 

 * HIP/AMD optimized version

 */
- Line 246: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: p = p - learning_rate * (v + weight_decay * p);

    } else {

        // Without momentum

        p = p - learning_rate * (g + weight_decay * p);

    }

    

    params[idx] = p;
- Line 429: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '    int blockSize = 256;', '    int gridSize = (size + blockSize - 1) / blockSize;', '', '    if (stream != nullptr) {']
- Line 451: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (grad_output == nullptr || partial_loss == nullptr || predictions == nullptr || targets == nullptr) {
- Line 54: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: const int TILE_SIZE = 16;
- Line 78: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float result = 0.0f;
- Line 79: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int r = 0; r < rank; r++) {
- Line 86: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int tile_start = 0; tile_start < rank; tile_start += TILE_SIZE) {
- Line 90: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float sum = 0.0f;
- Line 91: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < in_dim; i++) {
- Line 100: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float partial_result = 0.0f;
- Line 102: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i < tile_size; i++) {
- Line 158: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float sum = 0.0f;
- Line 161: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float h_val = 0.0f;
- Line 177: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float sum = 0.0f;
- Line 180: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float temp = 0.0f;
- Line 196: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float sum = 0.0f;
- Line 199: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float temp = 0.0f;
- Line 277: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float local_loss = 0.0f;
- Line 278: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float scale = 2.0f / n;
- Line 458: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int threads = 256;

### llm/lora_framework/multi_gpu_trainer.cpp
Total findings: 23

- Line 277: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // In real implementation, would deserialize from file
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #578 [LoRA Phase 10.5] Implement... (2026-03-11) | #1108 Implement Multi-GPU
- Line 71: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto output_data = outputs[i].cpu_data();
- Line 72: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto target_data = targets[i].cpu_data();
- Line 240: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto shape = tensors[0].shape();
- Line 420: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto param_data = params[j]->cpu_data();
- Line 421: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto grad_data = grads[j]->cpu_data();
- Line 50: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<GPUTensor>& inputs,
- Line 56: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto outputs = layer.forward(inputs);
- Line 60: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::vector<GPUTensor> grad_outputs;
- Line 61: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: grad_outputs.reserve(outputs.size());
- Line 63: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (size_t i = 0; i < outputs.size(); ++i) {
- Line 64: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: float local_loss = compute_loss(outputs[i], targets[i]);
- Line 71: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto output_data = outputs[i].cpu_data();
- Line 81: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: GPUTensor grad(outputs[i].shape(), outputs[i].device());
- Line 83: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: grad_outputs.push_back(std::move(grad));
- Line 86: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: float avg_loss = total_loss / static_cast<float>(outputs.size());
- Line 89: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: layer.backward(grad_outputs);
- Line 143: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<GPUTensor>& inputs,
- Line 147: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto outputs = layer.forward(inputs);
- Line 151: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (size_t i = 0; i < outputs.size(); ++i) {
- Line 152: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: total_loss += compute_loss(outputs[i], targets[i]);
- Line 155: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return total_loss / static_cast<float>(outputs.size());

### llm/ethics_aware_confidence_detector.cpp
Total findings: 22

- Line 204: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_technical_confidence =
- Line 207: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_autonomy_respect =
- Line 210: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_transparency =
- Line 213: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->stats.avg_combined_confidence =
- Line 602: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto it = impl_->cache.find(key);
- Line 603: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (it != impl_->cache.end()) {
- Line 615: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (impl_->cache.size() >= impl_->config.max_cache_size) {
- Line 619: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->cache[key] = result;
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12)
- Line 358: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text_lower.find(pattern) != std::string::npos) {
- Line 365: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text_lower.find(pattern) != std::string::npos) {
- Line 381: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text_lower.find(pattern) != std::string::npos) {
- Line 388: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text_lower.find(pattern) != std::string::npos) {
- Line 404: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text_lower.find(word) != std::string::npos) {
- Line 411: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text_lower.find(word) != std::string::npos) {
- Line 492: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text_lower.find(pattern) != std::string::npos) {
- Line 506: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text_lower.find(pattern) != std::string::npos) {
- Line 359: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: detected.push_back(pattern);
- Line 366: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: detected.push_back(pattern);
- Line 382: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: detected.push_back(pattern);
- Line 389: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: detected.push_back(pattern);
- Line 418: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> unique(detected.begin(), detected.end());

### llm/lora_framework/kernels/vulkan_kernels.cpp
Total findings: 22

- Line 886: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: buf_embedding_weights.upload(embedding_weights, checked_mul_size(embedding_elems, sizeof(float), "launch_embedding_lookup_shader"));
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4828 Potential fix for code scan... (2026-05-04) | #571 Implement Vulkan com
- Line 33: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool initialized = false;
- Line 274: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: search_paths.push_back("./shaders/lora/");  // Current directory
- Line 275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: search_paths.push_back("../shaders/lora/"); // Parent directory
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: search_paths.push_back("../../shaders/lora/"); // Grandparent directory
- Line 277: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: search_paths.push_back("shaders/lora/"); // Relative to binary root
- Line 279: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: search_paths.push_back("build/windows-bench-release/shaders/lora/");
- Line 280: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: search_paths.push_back("build/windows-release/shaders/lora/");
- Line 292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: search_paths.push_back("src/acceleration/vulkan/shaders/lora/");
- Line 293: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: search_paths.push_back("../src/acceleration/vulkan/shaders/lora/");
- Line 294: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: search_paths.push_back("../../src/acceleration/vulkan/shaders/lora/");
- Line 301: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cout << "Found shader: " << full_path << std::endl;
- Line 311: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cout << "Found shader: " << full_path << std::endl;
- Line 322: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cerr << "  " << path << shader_name << ".comp.spv" << std::endl;
- Line 301: severity=LOW; category=endl_in_loop
  Description: std::endl in loop (causes unnecessary flush, use '\n')
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::cout << "Found shader: " << full_path << std::endl;
- Line 301: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Found shader: " << full_path << std::endl;
- Line 311: severity=LOW; category=endl_in_loop
  Description: std::endl in loop (causes unnecessary flush, use '\n')
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::cout << "Found shader: " << full_path << std::endl;
- Line 311: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Found shader: " << full_path << std::endl;
- Line 322: severity=LOW; category=endl_in_loop
  Description: std::endl in loop (causes unnecessary flush, use '\n')
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::cerr << "  " << path << shader_name << ".comp.spv" << std::endl;
- Line 353: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Vulkan LoRA backend initialized successfully" << std::endl;
- Line 354: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Device: " << g_vulkan_state.context->device_properties().deviceName << std::endl;

### llm/lora_framework/gpu_training_loop.cpp
Total findings: 21

- Line 130: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const float* embedding_matrix = base_model_->getEmbeddingMatrix();
- Line 388: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t estimated_savings = checkpointer_->estimateMemorySavings(avg_activation_size);
- Line 504: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t hidden_dim = gpu_embedding_layer_ ? gpu_embedding_layer_->hidden_dim() : DEFAULT_HIDDEN_DIM;
- Line 692: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: GPUTensor embeddings_3d = embedding_layer->forward(token_ids);
- Line 253: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 550: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: GPUTensor grad_output = computeMSEGradientGPU(predictions, target_embeddings);
- Line 616: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t used_vram = stats.allocated_bytes;
- Line 659: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: float usage_pct = 100.0f * stats.allocated_bytes / std::max(stats.total_bytes, size_t(1));
- Line 664: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats.allocated_bytes / (1024.0 * 1024.0 * 1024.0),
- Line 759: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t i = 0; i < batch_size; ++i) {

            for (size_t k = 0; k < seq_len; ++k) {

                for (size_t j = 0; j < hidden_dim; ++j) {

                    averaged_data[i * hidden_dim + j] += 

                        embeddings_data[i * seq_len * hidden_dim + k * hidden_dim + j];

                }

            }
- Line 760: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t k = 0; k < seq_len; ++k) {

                for (size_t j = 0; j < hidden_dim; ++j) {

                    averaged_data[i * hidden_dim + j] += 

                        embeddings_data[i * seq_len * hidden_dim + k * hidden_dim + j];

                }

            }

            // Normalize by sequence length
- Line 765: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

            // Normalize by sequence length

            for (size_t j = 0; j < hidden_dim; ++j) {

                averaged_data[i * hidden_dim + j] /= seq_len;

            }

        }
- Line 785: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t i = 0; i < batch_size; ++i) {

        for (size_t j = 0; j < hidden_dim; ++j) {

            size_t token_idx = j % seq_len;

            int token_id = static_cast<int>(token_data[i * seq_len + token_idx]);

            embedding_data[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;

        }

    }
- Line 786: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t j = 0; j < hidden_dim; ++j) {

            size_t token_idx = j % seq_len;

            int token_id = static_cast<int>(token_data[i * seq_len + token_idx]);

            embedding_data[i * hidden_dim + j] = static_cast<float>(token_id % 100) / 100.0f;

        }

    }
- Line 974: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: * @param grad_output Output gradient tensor (will be allocated)
- Line 987: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: grad_output = GPUTensor(predictions.shape(), predictions.device());
- Line 996: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: int threads = 256;
- Line 554: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::vector<GPUTensor> grad_outputs;
- Line 555: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: grad_outputs.reserve(1);
- Line 556: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: grad_outputs.push_back(std::move(grad_output));
- Line 557: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: multi_gpu_layer_->backward(grad_outputs);

### llm/lora_framework/multi_gpu_lora_layer.cpp
Total findings: 20

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #578 [LoRA Phase 10.5] Implement... (2026-03-11)
- Line 215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: param_grads.push_back(all_gradients[gpu_idx][param_idx]);
- Line 248: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto master_params = layers_[0]->parameters();
- Line 252: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto target_params = layers_[i]->parameters();
- Line 261: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto master_data = master_params[j]->cpu_data();
- Line 115: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::vector<GPUTensor> MultiGPULoRALayer::forward(const std::vector<GPUTensor>& inputs) {
- Line 116: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (inputs.size() != static_cast<size_t>(ctx_.num_gpus())) {
- Line 123: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::vector<GPUTensor> outputs;
- Line 124: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: outputs.reserve(inputs.size());
- Line 131: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (inputs[i].device().device_id != expected_device.device_id ||
- Line 132: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: inputs[i].device().type != expected_device.type) {
- Line 137: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: outputs.push_back(layers_[i]->forward(inputs[i]));
- Line 146: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return outputs;
- Line 150: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<GPUTensor>& grad_outputs) {
- Line 152: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (grad_outputs.size() != static_cast<size_t>(ctx_.num_gpus())) {
- Line 159: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::vector<GPUTensor> grad_inputs;
- Line 160: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: grad_inputs.reserve(grad_outputs.size());
- Line 163: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (size_t i = 0; i < grad_outputs.size(); ++i) {
- Line 164: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: grad_inputs.push_back(layers_[i]->backward(grad_outputs[i]));
- Line 173: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: return grad_inputs;

### llm/gguf_loader.cpp
Total findings: 19

- Line 92: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: fd_ = open(filepath.c_str(), O_RDONLY);
- Line 193: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    const char* data = static_cast<const char*>(mmap_base_);', '    uint64_t len;', '    std::memcpy(&len, data + offset, sizeof(uint64_t));', '    offset += 8;', '']
- Line 636: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        if (!data.empty()) {', '            const auto* src = static_cast<const uint8_t*>(mmap_base_) + tensor_start;', '            std::memcpy(data.data(), src, tensor->size);', '        }', '        return data;']
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5122 docs(llm): synchronize GGUF... (2026-05-13) | #998 C++ Audit: Eliminate
- Line 92: severity=HIGH; category=path_traversal
  Description: path_traversal_concat_path: Path construction — validate user input
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: fd_ = open(filepath.c_str(), O_RDONLY);
- Line 167: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&version, data + 4, sizeof(uint32_t));
- Line 177: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&tensor_count, data + 8, sizeof(uint64_t));
- Line 178: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&kv_count, data + 16, sizeof(uint64_t));
- Line 193: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&len, data + offset, sizeof(uint64_t));
- Line 282: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&kv_count, data + 16, sizeof(uint64_t));
- Line 325: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&kv_count, data + 16, sizeof(uint64_t));
- Line 345: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::memcpy(&tensor_count, data + 8, sizeof(uint64_t));
- Line 450: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"': escaped += "\\\""; break;
- Line 451: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': escaped += "\\\\"; break;
- Line 452: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\b': escaped += "\\b"; break;
- Line 453: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\f': escaped += "\\f"; break;
- Line 454: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': escaped += "\\n"; break;
- Line 455: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': escaped += "\\r"; break;
- Line 456: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': escaped += "\\t"; break;

### llm/lora_framework/gpu_tensor.cpp
Total findings: 19

- Line 179: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '    if (is_cpu()) {', '        std::memcpy(cpu_data_.data(), data, count * sizeof(float));', '    } else {', '        if (!allocator_->upload(gpu_data_, data, count * sizeof(float))) {']
- Line 197: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '    if (is_cpu()) {', '        std::memcpy(data, cpu_data_.data(), count * sizeof(float));', '    } else {', '        if (!allocator_->download(data, gpu_data_, count * sizeof(float))) {']
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #575 [LoRA Phase 10.4] Implement... (2026-03-11) | #573 Implement kernel fusi
- Line 127: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 475: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocator_->deallocate(gpu_data_);
- Line 524: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<float> c_data(size());

        

        for (size_t i = 0; i < size(); i++) {

            c_data[i] = a_data[i] + b_data[i];

        }

        

        result.upload(c_data);
- Line 565: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<float> c_data(size());

        

        for (size_t i = 0; i < size(); i++) {

            c_data[i] = a_data[i] - b_data[i];

        }

        

        result.upload(c_data);
- Line 613: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<float> c_data(size());

        

        for (size_t i = 0; i < size(); i++) {

            c_data[i] = a_data[i] * scalar;

        }

        

        result.upload(c_data);
- Line 662: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<float> c_data(size());

        

        for (size_t i = 0; i < size(); i++) {

            c_data[i] = a_data[i] * b_data[i];

        }

        

        result.upload(c_data);
- Line 688: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t j = 0; j < N; j++) {

                float sum = 0.0f;

                for (size_t k = 0; k < K; k++) {

                    sum += a_data[i * K + k] * b_data[k * N + j];

                }

                c_data[i * N + j] = sum;

            }
- Line 690: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t k = 0; k < K; k++) {

                    sum += a_data[i * K + k] * b_data[k * N + j];

                }

                c_data[i * N + j] = sum;

            }

        }

    } else {
- Line 763: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t j = 0; j < N; j++) {

                float sum = 0.0f;

                for (size_t k = 0; k < K; k++) {

                    sum += a_data[i * K + k] * b_data[k * N + j];

                }

                c_data[i * N + j] = sum;

            }
- Line 765: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t k = 0; k < K; k++) {

                    sum += a_data[i * K + k] * b_data[k * N + j];

                }

                c_data[i * N + j] = sum;

            }

        }
- Line 820: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t i = 0; i < rows; i++) {

            for (size_t j = 0; j < cols; j++) {

                c_data[j * rows + i] = a_data[i * cols + j];

            }

        }
- Line 370: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'CUDA dtype kernels").' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md §"CUDA dtype kernels").
- Line 381: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto converted_data = fn(download(), dtype_, target_dtype);

                result.upload(converted_data);

                return result;

            } catch (...) {

                // fall through to CPU round-trip

            }

        }
- Line 381: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 414: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto converted_data = fn(download(), dtype_, target_dtype);

                result.upload(converted_data);

                return result;

            } catch (...) {

                // fall through to CPU round-trip

            }

        }
- Line 414: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### llm/lora_router.cpp
Total findings: 19

- Line 423: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto adapter_meta = adapter_registry_->getAdapter(adapter_id);
- Line 463: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto adapter_meta = adapter_registry_->getAdapter(adapter_id);
- Line 492: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto adapter_meta = adapter_registry_->getAdapter(best_adapter);
- Line 529: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: for (size_t i = 0; i < ab_test_config_->adapter_ids.size(); ++i) {
- Line 530: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cumulative += ab_test_config_->traffic_splits[i];
- Line 532: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: selected_adapter = ab_test_config_->adapter_ids[i];
- Line 538: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: selected_adapter = ab_test_config_->adapter_ids.back();
- Line 550: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto adapter_meta = adapter_registry_->getAdapter(selected_adapter);
- Line 558: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: decision.reason = "Selected by A/B test: " + ab_test_config_->experiment_id;
- Line 582: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: selected_adapter = rollout_config_->new_adapter_id;
- Line 584: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: selected_adapter = rollout_config_->baseline_adapter_id;
- Line 596: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto adapter_meta = adapter_registry_->getAdapter(selected_adapter);
- Line 626: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto adapter_meta = adapter_registry_->getAdapter(fallback_config_.default_adapter_id);
- Line 759: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: return now >= ab_test_config_->start_time && now <= ab_test_config_->end_time;
- Line 252: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 271: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 274: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 575: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 526: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float cumulative = 0.0f;

### llm/lora_framework/base_model_adapter.cpp
Total findings: 18

- Line 31: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool BaseModelAdapter::loadModel(const std::string& model_path) {
- Line 430: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: void* tensor_ptr = gguf_loader_->mmapTensor(embedding_tensor_name_);
- Line 435: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto tensor_data = gguf_loader_->getTensorData(embedding_tensor_name_);
- Line 520: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: void* tensor_ptr = gguf_loader_->mmapTensor(embedding_tensor_name_);
- Line 573: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (!base_model_->loadModel(config_.base_model_path)) {
- Line 579: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: active_layers_ = base_model_->getLayersByTargetModules(config_.target_modules);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #548 Integrate LoRA Training wit... (2026-03-11) | #593 [LoRA] Implement Real
- Line 277: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::string prefix = target_pattern.substr(0, target_pattern.find('*'));
- Line 278: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return layer_name.find(prefix) == 0;
- Line 389: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (tensor.name == name || tensor.name.find(name) != std::string::npos) {
- Line 545: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // This helps with cross-model compatibility
- Line 46: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: size_t last_slash = model_path.find_last_of("/\\");
- Line 583: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i > 0) modules_str += ", ";
- Line 584: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) modules_str += ", ";
- Line 698: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::pair<Tensor, Tensor>>
- Line 700: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::pair<Tensor, Tensor>> weights;
- Line 710: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::pair<Tensor, Tensor>>& weights) {
- Line 373: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // For LoRA training, we need raw token embeddings as inputs to layers,

### llm/lora_security_validator.cpp
Total findings: 18

- Line 714: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<float> LoRASecurityValidator::loadWeightsFromLoRAFile(
- Line 758: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['', '    // Validate header size', '    if (header_size > data.size() - 8 || header_size > 100*1024*1024) {', '        spdlog::warn("Invalid header size in LoRa binary format: {} bytes", header_size);', '        return weights;']
- Line 850: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::info("Loaded {} sampled weights from binary LoRa file", weights.size());
- Line 315: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return result;

    }

    

    std::string signature_b64 = metadata["signature"];

    std::string signer = metadata["signer"];

    

    // Check if signer is trusted
- Line 316: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    std::string signature_b64 = metadata["signature"];

    std::string signer = metadata["signer"];

    

    // Check if signer is trusted

    if (!isTrustedSigner(signer)) {
- Line 360: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Check if certificate is embedded in metadata

    if (metadata.contains("certificate")) {

        cert_pem = metadata["certificate"];

        spdlog::debug("Using embedded certificate for verification");

    }
- Line 501: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Validate base model

    if (!config_.allowed_base_models.empty()) {

        std::string base_model = metadata["base_model"];

        if (config_.allowed_base_models.find(base_model) == 

            config_.allowed_base_models.end()) {

            spdlog::error("LoRa base model not allowed: {}", base_model);
- Line 690: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // ── Attempt 2: Pure JSON format (legacy / lightweight adapters) ───────
- Line 1049: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (char c : prompt) {
- Line 541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: anomalies.push_back("Unusual weight distribution detected");
- Line 557: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: anomalies.push_back("Suspiciously high number of zero weights");
- Line 560: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: anomalies.push_back("Suspiciously high number of large weights");
- Line 734: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: weights.push_back(w.get<float>());
- Line 780: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto dtype = tensor_info["dtype"].get<std::string>();
- Line 781: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto offsets = tensor_info["data_offsets"].get<std::vector<uint64_t>>();
- Line 924: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: PromptInjectionDetector::initializePatterns()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void PromptInjectionDetector::initializePatterns() {
- Line 937: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::regex(R"(<\s*\|.*?\|\s*>)",  // Special tokens
- Line 1007: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: pos += 10;  // Length of "[REDACTED]"

### llm/lora_framework/lora_layers.cpp
Total findings: 17

- Line 241: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cached_BA_ = std::make_unique<Tensor>(B_->matmul(*A_));
- Line 263: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: Tensor input_T = cached_input_->transpose();
- Line 534: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: param->data()[i] -= learning_rate_ * momentum_buffer[i];
- Line 543: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: param->data()[i] -= learning_rate_ * grad_with_decay;
- Line 631: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: param->data()[i] -= learning_rate_ * m_hat / (std::sqrt(v_hat) + epsilon_);
- Line 714: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: float weight_decay_update = learning_rate_ * weight_decay_ * param->data()[i];
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #998 C++ Audit: Eliminate raw me... (2026-03-11) | #547 Implement Adam/AdamW
- Line 533: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (weight_decay_ > 0.0f) {

                    grad_with_decay += weight_decay_ * param->data()[i];

                }

                momentum_buffer[i] = momentum_ * momentum_buffer[i] + grad_with_decay;

                param->data()[i] -= learning_rate_ * momentum_buffer[i];

            }

        } else {
- Line 534: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: grad_with_decay += weight_decay_ * param->data()[i];

                }

                momentum_buffer[i] = momentum_ * momentum_buffer[i] + grad_with_decay;

                param->data()[i] -= learning_rate_ * momentum_buffer[i];

            }

        } else {

            // Standard SGD: param = param - lr * grad (with optional weight decay)
- Line 614: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: // m_t = β1 * m_{t-1} + (1 - β1) * g_t
- Line 622: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: // m̂_t = m_t / (1 - β1^t)
- Line 623: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float m_hat = m[i] / bias_correction1;
- Line 626: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: // v̂_t = v_t / (1 - β2^t)
- Line 696: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: // m_t = β1 * m_{t-1} + (1 - β1) * g_t
- Line 704: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: // m̂_t = m_t / (1 - β1^t)
- Line 705: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float m_hat = m[i] / bias_correction1;
- Line 708: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: // v̂_t = v_t / (1 - β2^t)

### llm/lora_framework/lora_training_config.cpp
Total findings: 17

- Line 129: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: plugin_config.min_avg_rating = adapter_config->triggers.min_avg_rating;
- Line 204: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (node["training_data"]) {

        for (const auto& data : node["training_data"]) {

            std::string source_name = data.first.as<std::string>();

            config.training_data[source_name] = 

                parseTrainingDataSource(data.second);

        }

    }
- Line 51: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto defaults = root["training_defaults"];
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back("Adapter " + id + ": base_model_name is empty");
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back("Adapter " + id + ": base_model_path is empty");
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back("Adapter " + id + ": invalid rank");
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back("Adapter " + id + ": invalid learning_rate");
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back("Adapter " + id + ": invalid direct_response_weight");
- Line 189: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto base = node["base_model"];
- Line 211: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto feedback = node["training_data"]["feedback"];
- Line 230: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto exec = node["pipeline"]["execution"];
- Line 310: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto automatic = node["automatic"];
- Line 315: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto batch = automatic["batch_size"];
- Line 323: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto time = automatic["time"];
- Line 331: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto quality = automatic["quality"];
- Line 348: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ab = node["ab_testing"];
- Line 359: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto rollback = node["auto_rollback"];

### llm/vision_resource_monitor.cpp
Total findings: 16

- Line 307: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: metrics_thread_.join();
- Line 311: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: quota_reset_thread_.join();
- Line 319: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& limits = config_->getResourceLimits();
- Line 672: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& limits = config_->getResourceLimits();
- Line 699: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto& quota = config_->getResourceQuota();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #690 Production-grade Vis
- Line 78: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 79: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 119: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 120: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 372: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: info.memory_allocated_mb = 0;
- Line 688: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::hours(1));
- Line 103: severity=MEDIUM; category=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: RateLimiter::reset()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: void RateLimiter::reset() {
- Line 589: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: ss << "themisdb_vision_inference_duration_seconds{quantile=\"0.5\"} " << (usage.avg_inference_time_m
- Line 590: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: ss << "themisdb_vision_inference_duration_seconds{quantile=\"0.9\"} " << (usage.max_inference_time_m
- Line 606: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: entries.push_back(q.front());

### llm/applications/themis_help_lora.cpp
Total findings: 15

- Line 187: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool loaded = llama_wrapper->loadModel(model_path);
- Line 434: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Reload with new weights
- Line 482: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: doc_data.dataset_name = "documentation_corpus_" + impl_->config.adapter_id;
- Line 485: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: TrainingResult train_result = impl_->training_service->trainOnTheFly(
- Line 517: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Reload with new weights
- Line 220: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.max_tokens = 500;

                request.temperature = 0.7f;

                request.top_p = 0.9f;

                request.request_id = themis::llm::applications::generateModelRequestId();

                

                // Add LoRA adapter if loaded

                if (orchestrator->isLoaded(config.adapter_id)) {
- Line 228: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto llm_response = llama_wrapper->generate(request);
- Line 228: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto llm_response = llama_wrapper->generate(request);
- Line 236: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: response = generatePlaceholderResponse(question);
- Line 316: severity=HIGH; category=missing_trace_point
  Description: Critical function query without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string ThemisHelpLoRA::query(const std::string& question, const std::string& user_id) {
- Line 434: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 517: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 14: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_training_service.h"
- Line 228: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto llm_response = llama_wrapper->generate(request);
- Line 316: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string ThemisHelpLoRA::query(const std::string& question, const std::string& user_id) {

### llm/continuous_batch_scheduler.cpp
Total findings: 15

- Line 580: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto block_table = kv_cache_->getBlockTable(request->sequence_id);
- Line 587: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: block_table = kv_cache_->getBlockTable(request->sequence_id);
- Line 667: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto kv_stats = kv_cache_->getStats();
- Line 86: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (quota_manager_) {

        const std::string& user_id  = request.request_id;  // best available key at ingestion

        const std::string& model_id = request.model_id;

        const size_t estimated = request.prompt.length() / CHARS_PER_TOKEN_ESTIMATE + request.max_tokens;

        auto qr = quota_manager_->check(user_id, model_id, estimated);

        if (!qr.allowed) {

            stats_.rejected_requests++;
- Line 259: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocateKVCacheBlocks(req.get());
- Line 269: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: reserved_blocks_in_batch += req->allocated_blocks.size();
- Line 441: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = all_requests_.find(req_id);
- Line 478: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find_if(
- Line 563: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void ContinuousBatchScheduler::allocateKVCacheBlocks(ScheduledRequest* request) {
- Line 570: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!request->allocated_blocks.empty()) {
- Line 616: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: request->allocated_blocks.size(), request->request_id,
- Line 619: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: request->allocated_blocks.clear();
- Line 60: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 244: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: batch.push_back(req.get());
- Line 267: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: batch.push_back(req.get());

### llm/lora_framework/paged_memory_manager.cpp
Total findings: 15

- Line 49: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: PagedMemoryManager::~PagedMemoryManager() {
- Line 55: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: cpu_allocator_->deallocate(buffer.cpu_ptr);
- Line 58: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_allocator_->deallocate(buffer.gpu_ptr);
- Line 79: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: PagedBuffer PagedMemoryManager::allocate(size_t size, const Device& device) {
- Line 79: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: PagedBuffer PagedMemoryManager::allocate(size_t size, const Device& device) {
- Line 87: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: buffer.cpu_ptr = cpu_allocator_->allocate(size);
- Line 100: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: buffer.gpu_ptr = gpu_allocator_->allocate(size);
- Line 124: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void PagedMemoryManager::deallocate(PagedBuffer& buffer) {
- Line 124: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void PagedMemoryManager::deallocate(PagedBuffer& buffer) {
- Line 132: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: cpu_allocator_->deallocate(buffer.cpu_ptr);
- Line 137: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_allocator_->deallocate(buffer.gpu_ptr);
- Line 176: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: buffer.gpu_ptr = gpu_allocator_->allocate(buffer.size_bytes);
- Line 181: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: buffer.gpu_ptr = gpu_allocator_->allocate(buffer.size_bytes);
- Line 230: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: gpu_allocator_->deallocate(buffer.gpu_ptr);
- Line 253: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = pages_.find(page_id);

### llm/lora_framework/paged_optimizer.cpp
Total findings: 15

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #549 Implement QLoRA (Qua
- Line 54: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: state.momentum = memory_manager_->allocate(param_size, Device::cpu());
- Line 55: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: state.variance = memory_manager_->allocate(param_size, Device::cpu());
- Line 105: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float* m_data = nullptr;
- Line 109: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: m_data = static_cast<float*>(state.momentum.cpu_ptr);
- Line 117: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: m_data = m_temp.data();
- Line 130: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // AdamW update rule

    for (size_t i = 0; i < size; ++i) {

        float grad = grad_data[i];

        

        // Update biased first moment estimate

        m_data[i] = beta1_ * m_data[i] + (1.0f - beta1_) * grad;
- Line 133: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: float grad = grad_data[i];

        

        // Update biased first moment estimate

        m_data[i] = beta1_ * m_data[i] + (1.0f - beta1_) * grad;

        

        // Update biased second raw moment estimate

        v_data[i] = beta2_ * v_data[i] + (1.0f - beta2_) * (grad * grad);
- Line 136: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: m_data[i] = beta1_ * m_data[i] + (1.0f - beta1_) * grad;

        

        // Update biased second raw moment estimate

        v_data[i] = beta2_ * v_data[i] + (1.0f - beta2_) * (grad * grad);

        

        // Compute bias-corrected first moment estimate

        float m_hat = m_data[i] / bias_correction1;
- Line 139: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: v_data[i] = beta2_ * v_data[i] + (1.0f - beta2_) * (grad * grad);

        

        // Compute bias-corrected first moment estimate

        float m_hat = m_data[i] / bias_correction1;

        

        // Compute bias-corrected second raw moment estimate

        float v_hat = v_data[i] / bias_correction2;
- Line 139: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float m_hat = m_data[i] / bias_correction1;
- Line 142: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: float m_hat = m_data[i] / bias_correction1;

        

        // Compute bias-corrected second raw moment estimate

        float v_hat = v_data[i] / bias_correction2;

        

        // Update parameters with AdamW weight decay (decoupled)

        // AdamW applies weight decay directly to parameters, not through gradients
- Line 146: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Update parameters with AdamW weight decay (decoupled)

        // AdamW applies weight decay directly to parameters, not through gradients

        param_data[i] = param_data[i] * (1.0f - learning_rate_ * weight_decay_) 

                        - learning_rate_ * m_hat / (std::sqrt(v_hat) + epsilon_);

    }

}
- Line 288: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(num_to_evict, evictable.size()); ++i) {
- Line 261: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<Tensor*, PagedOptimizerState>& states,

### llm/embedded_llm.cpp
Total findings: 13

- Line 51: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (!wrapper_->loadModel(config.model_path)) {
- Line 61: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: wrapper_->unloadModel();
- Line 84: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = generateFull(request);
- Line 105: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = generateFull(request);
- Line 196: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = generateFull(request);
- Line 221: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = generateFull(request);
- Line 227: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = generateFull(request);
- Line 238: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return wrapper_->generate(request);
- Line 128: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return generate(formatted_prompt);
- Line 147: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<float> EmbeddedLLM::embed(const std::string& text) {
- Line 166: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto embedding = wrapper_->embed(text);
- Line 179: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: embeddings.push_back(embed(text));  // reuse cached embed()
- Line 238: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return wrapper_->generate(request);

### llm/llm_response_cache.cpp
Total findings: 13

- Line 52: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (!db->open()) {
- Line 73: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: if (!db->open()) {
- Line 246: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t total_ops = stats_.hits.load(std::memory_order_relaxed) +
- Line 247: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats_.misses.load(std::memory_order_relaxed);
- Line 248: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 309: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(entry.embedding.size(), query_embedding.size()); ++i) {
- Line 349: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: size_t total_ops = stats_.hits.load(std::memory_order_relaxed) +
- Line 350: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stats_.misses.load(std::memory_order_relaxed);
- Line 351: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 457: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: auto embedding = config_.llm_ptr->embed(prompt);
- Line 303: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: static const std::unordered_set<std::string> stopwords = {
- Line 457: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto embedding = config_.llm_ptr->embed(prompt);
- Line 503: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> words;

### llm/attention/kv_cache_manager.cpp
Total findings: 12

- Line 51: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: BlockTable KVCacheManager::allocateSequence(uint64_t seq_id, int expected_tokens) {
- Line 116: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 124: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: int new_block = allocateBlock();
- Line 124: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 158: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 164: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 165: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 166: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 167: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 168: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 169: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 180: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### llm/lora_framework/adapter_sync_manager.cpp
Total findings: 12

- Line 121: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: sync_thread_.join();
- Line 241: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

                        mutex_.unlock();

                        success = syncAdapter(adapter_id);

                        mutex_.lock();

                    }

                    

                    if (success) {
- Line 241: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: mutex_.lock();
- Line 103: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: sync_thread_ = std::thread([this]() { syncLoop(); });
- Line 241: severity=HIGH; category=explicit_lock_unlock
  Description: Explicit lock/unlock without RAII guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: mutex_.lock();
- Line 351: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(mutex_);
- Line 11: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_storage_service.h"
- Line 324: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 324: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: void onSyncComplete(std::function<void(const SyncJobResult&)> callback) {
- Line 634: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 634: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: void AdapterSyncManager::onSyncComplete(std::function<void(const SyncJobResult&)> callback) {
- Line 635: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: impl_->onSyncComplete(callback);

### llm/lora_framework/kernels/cpu_fused_kernels.cpp
Total findings: 12

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #607 Complete implementat
- Line 42: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param input Input tensor [batch_size, in_dim]
- Line 43: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param B LoRA B matrix [in_dim, rank]
- Line 44: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param A LoRA A matrix [rank, out_dim]
- Line 45: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param output Output tensor [batch_size, out_dim]
- Line 99: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param input Input tensor [batch_size, in_dim] (from forward)
- Line 100: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param B LoRA B matrix [in_dim, rank]
- Line 101: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param A LoRA A matrix [rank, out_dim]
- Line 102: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param grad_output Gradient w.r.t output [batch_size, out_dim]
- Line 103: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param grad_A Gradient w.r.t A [rank, out_dim]
- Line 104: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param grad_B Gradient w.r.t B [in_dim, rank]
- Line 105: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param grad_input Gradient w.r.t input [batch_size, in_dim]

### llm/lora_framework/lora_storage_service.cpp
Total findings: 12

- Line 322: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Load weights
- Line 115: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: spdlog::error("Failed to delete adapter {}: {}", adapter_id, e.what());
- Line 115: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: backendToString(config_.backend));

            return false;

        } catch (const std::exception& e) {

            spdlog::error("Failed to delete adapter {}: {}", adapter_id, e.what());

            return false;

        }

    }
- Line 115: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: spdlog::error("Failed to delete adapter {}: {}", adapter_id, e.what());
- Line 133: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(config_.filesystem_path)) {
- Line 210: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(adapter_dir)) {
- Line 2: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: * ThemisDB | File: lora_storage_service.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
- Line 10: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_storage_service.h"
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: adapters.push_back(entry.path().filename().string());
- Line 159: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                    int num = std::stoi(v.substr(1));

                    max_version = std::max(max_version, num);

                } catch (...) {}

            }

        }
- Line 159: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 335: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: weights_file.close();

### llm/lora_framework/vulkan_pipeline.cpp
Total findings: 12

- Line 41: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 326: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: VkResult result = vkCreateDescriptorPool(context_->device(), &pool_info,
- Line 370: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    }', '', '    std::memcpy(push_constant_data_.data() + offset, data, size);', '}', '']
- Line 412: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void VulkanComputePipeline::dispatch(uint32_t group_x, uint32_t group_y, uint32_t group_z) {

    // Wait for previous dispatch to complete

    wait();

    

    // Update descriptor sets if needed

    update_descriptor_sets();
- Line 412: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: wait();
- Line 463: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: VkResult submit_result = vkQueueSubmit(context_->compute_queue(), 1, &submit_info, fence_);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3629 [MODULE] llm â€“ bu
- Line 187: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!allocate_descriptor_sets()) {
- Line 192: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: command_buffer_ = context_->allocate_command_buffer();
- Line 337: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool VulkanComputePipeline::allocate_descriptor_sets() {
- Line 348: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::cerr << "Failed to allocate descriptor sets: " << result << std::endl;
- Line 367: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: throw std::runtime_error("Push constant size exceeds allocated size");

### llm/fewshot_optimizer.cpp
Total findings: 11

- Line 74: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: config_.diversity_weight * result.avg_diversity;

    }

    

    result.metadata["num_candidates"] = candidate_examples.size();

    result.metadata["num_selected"] = result.selected_examples.size();

    

    THEMIS_INFO("Selected {} examples with avg_relevance={:.4f}, avg_diversity={:.4f}",
- Line 75: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    result.metadata["num_candidates"] = candidate_examples.size();

    result.metadata["num_selected"] = result.selected_examples.size();

    

    THEMIS_INFO("Selected {} examples with avg_relevance={:.4f}, avg_diversity={:.4f}",

                result.selected_examples.size(), result.avg_relevance, result.avg_diversity);
- Line 128: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back(cache_[scored_examples[i].second]);
- Line 163: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> query_set(query_tokens.begin(), query_tokens.end());
- Line 164: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> example_set(example_tokens.begin(), example_tokens.end());
- Line 258: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: selected.push_back(remaining[0]);
- Line 290: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: selected.push_back(remaining[best_idx]);
- Line 303: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> tokens;
- Line 301: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Combine similarity of inputs and outputs
- Line 318: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Jaccard similarity for inputs
- Line 328: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Jaccard similarity for outputs

### llm/paged_kv_cache_manager.cpp
Total findings: 11

- Line 28: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 89: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 133: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::vector<int> block_ids = allocateBlocks(num_blocks_needed);
- Line 175: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t allocated_blocks = 0;
- Line 178: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocated_blocks += table.block_ids.size();
- Line 184: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats.fragmentation_rate = static_cast<double>(allocated_blocks - theoretical_blocks) /
- Line 293: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t total_allocated = total_blocks_allocated_.load();
- Line 296: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (total_allocated == 0) return 0.0;
- Line 298: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return (static_cast<double>(shared) / total_allocated) * 100.0;
- Line 240: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::unordered_set<int> known_free(free_block_ids_.begin(), free_block_ids_.end());
- Line 240: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<int> known_free(free_block_ids_.begin(), free_block_ids_.end());

### llm/prompt_evaluator.cpp
Total findings: 11

- Line 225: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 237: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 75: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: similarities.push_back(metrics.semantic_similarity);
- Line 145: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> set1(tokens1.begin(), tokens1.end());
- Line 146: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> set2(tokens2.begin(), tokens2.end());
- Line 208: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> output_set(tokens_output.begin(), tokens_output.end());
- Line 53: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: const std::vector<std::string>& outputs,
- Line 58: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (outputs.size() != expected.size()) {
- Line 63: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: if (outputs.empty()) {
- Line 71: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: for (size_t i = 0; i < outputs.size(); ++i) {
- Line 72: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: auto metrics = evaluateSingle(outputs[i], expected[i]);

### llm/ethical_guidelines_manager.cpp
Total findings: 10

- Line 22: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Map legacy logging calls to project-wide macros
- Line 446: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text.find(word) != std::string::npos) de_score++;
- Line 452: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text.find(word) != std::string::npos) en_score++;
- Line 467: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text_lower.find(keyword_lower) != std::string::npos) {
- Line 468: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (text_lower.find(keyword_lower) != std::string::npos) {
- Line 63: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto cfg = config["config"];
- Line 93: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto cd = config["context_detection"];
- Line 120: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto pa = config["prompt_augmentation"];
- Line 569: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: judge_prompt += R"(
- Line 592: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: resp = llm->generate(req);

### llm/llama_resource_manager.cpp
Total findings: 10

- Line 78: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 217: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocateGPUMemory(gpu_config);
- Line 222: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const std::string backend_name = backend_ptr ? backend_ptr->name() : "Unknown";
- Line 392: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void BackendAwareLlamaModelHandle::allocateGPUMemory(
- Line 43: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: LlamaModelHandle::~LlamaModelHandle() {
- Line 47: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: LlamaModelHandle::LlamaModelHandle(LlamaModelHandle&& other) noexcept
- Line 109: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: LlamaContextHandle::~LlamaContextHandle() {
- Line 113: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: LlamaContextHandle::LlamaContextHandle(LlamaContextHandle&& other) noexcept
- Line 126: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: llama_free(ctx);
- Line 238: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: BackendAwareLlamaModelHandle::~BackendAwareLlamaModelHandle() {

### llm/lora_framework/gpu_data_loader.cpp
Total findings: 10

- Line 124: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (config_.async_loading && prefetch_active_.load()) {

        // Get from prefetch queue

        std::unique_lock<std::mutex> lock(queue_mutex_);

        queue_cv_.wait(lock, [this] { 

            return !prefetch_queue_.empty() || stop_prefetch_.load(); 

        });
- Line 124: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: queue_cv_.wait(lock, [this] {
- Line 206: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: prefetch_thread_.join();
- Line 227: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Check if we have room in the queue

        {

            std::unique_lock<std::mutex> lock(queue_mutex_);

            queue_cv_.wait(lock, [this, &batch_idx] {

                return prefetch_queue_.size() < config_.prefetch_batches || 

                       stop_prefetch_.load();

            });
- Line 227: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: queue_cv_.wait(lock, [this, &batch_idx] {
- Line 175: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats.gpu_memory_bytes = allocator_stats.allocated_bytes;
- Line 226: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 247: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> lock(queue_mutex_);
- Line 338: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 327: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: formatted += "\n\n### Input:\n" + sample.input;

### llm/sampling_strategy.cpp
Total findings: 10

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #526 Implement token samp
- Line 130: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = std::find(indices.begin(), indices.end(), id);
- Line 130: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find(indices.begin(), indices.end(), id);
- Line 132: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        auto it = std::find(indices.begin(), indices.end(), id);', '        size_t pidx = static_cast<size_t>(std::distance(indices.begin(), it));', '        float p = probs[pidx];', '        nuc_probs.push_back(p);', '        nuc_sum += p;']
- Line 201: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (int id : nucleus) { auto it = std::find(indices.begin(), indices.end(), id); size_t pidx = std::distance(indices.begin(), it); float p = probs[pidx]; nuc_probs.push_back(p); nuc_sum += p; }
- Line 201: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['    if (nucleus.empty()) nucleus.push_back(indices[order.front()]);', '    std::vector<float> nuc_probs; nuc_probs.reserve(nucleus.size()); float nuc_sum = 0.0f;', '    for (int id : nucleus) { auto it = std::find(indices.begin(), indices.end(), id); size_t pidx = std::distance(indices.begin(), it); float p = probs[pidx]; nuc_probs.push_back(p); nuc_sum += p; }', '    for (auto& p : nuc_probs) p /= (nuc_sum > 0.f ? nuc_sum : 1.f);', '']
- Line 118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: nucleus.push_back(indices[oi]);
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (nucleus.empty()) nucleus.push_back(indices[order.front()]);
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (int oi : order) { nucleus.push_back(indices[oi]); cum += probs[oi]; if (cum >= top_p) break; }
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (nucleus.empty()) nucleus.push_back(indices[order.front()]);

### llm/embedded_llm_stub.cpp
Total findings: 9

- Line 167: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = generate_full_fn_(request);
- Line 168: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (generate_full_fn_) {

            try {

                auto response = generate_full_fn_(request);

                if (request.stream_callback && !response.text.empty()) {

                    try {

                        request.stream_callback(response.text);

                    } catch (const std::exception& e) {
- Line 170: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto response = generate_full_fn_(request);

                if (request.stream_callback && !response.text.empty()) {

                    try {

                        request.stream_callback(response.text);

                    } catch (const std::exception& e) {

                        spdlog::warn("EmbeddedLLM stream callback failed: {}", e.what());

                    } catch (...) {
- Line 18: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Production Delta: No actual token generation; embed() always returns {}.
- Line 25: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 1: EmbeddedLLM stub → full LlamaWrapper' that was not found in 'src/llm/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llm/ROADMAP.md § "Phase 1: EmbeddedLLM stub → full LlamaWrapper"
- Line 89: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return generate(merged);
- Line 99: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<float> EmbeddedLLM::embed([[maybe_unused]] const std::string& text) {
- Line 132: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto text = generate(prompt, max_tokens);
- Line 158: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto text = generate(prompt, max_tokens);

### llm/inline_training_engine.cpp
Total findings: 9

- Line 261: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->registry      = std::move(registry);
- Line 967: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->config = InlineTrainingConfig::fromJSON(cfg_j);
- Line 629: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(cfg.checkpoint_dir)) {
- Line 631: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto pos = name.find("checkpoint-step-");
- Line 830: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: const float m_hat = impl_->m_adam[i] / bias1;
- Line 330: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Gap 3).' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md §Gap 3).
- Line 637: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

                                    int step_num = std::stoi(name.substr(pos + 16));

                                    ckpts.emplace_back(step_num, entry.path().string());

                                } catch (...) {}

                            }

                        }

                    }
- Line 637: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}
- Line 747: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'InlineTrainingEngine production gradient (v1' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llm/FUTURE_ENHANCEMENTS.md § "InlineTrainingEngine production gradient (v1.8.0)"

### llm/lora_framework/gguf_converter.cpp
Total findings: 9

- Line 146: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: float q = static_cast<float>(block->qs[i]);
- Line 95: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: reinterpret_cast<const gguf_blocks::Q4KBlock*>(src + block_idx * sizeof(gguf_blocks::Q4KBlock));
- Line 135: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: reinterpret_cast<const gguf_blocks::Q8_0Block*>(src + block_idx * sizeof(gguf_blocks::Q8_0Block));
- Line 190: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: reinterpret_cast<const gguf_blocks::Q4KBlock*>(src + gguf_block_idx * sizeof(gguf_blocks::Q4KBlock))
- Line 243: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            size_t block_begin = internal_block_idx * GGUF_CONVERSION_BLOCK_SIZE;', '            size_t block_end = std::min(block_begin + GGUF_CONVERSION_BLOCK_SIZE, num_elements);', '            result.blocks()[internal_block_idx].size = block_end - block_begin;', '        }', '    }']
- Line 294: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: reinterpret_cast<const gguf_blocks::Q8_0Block*>(src + gguf_block_idx * sizeof(gguf_blocks::Q8_0Block
- Line 327: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            size_t block_begin = internal_block_idx * GGUF_CONVERSION_BLOCK_SIZE;', '            size_t block_end = std::min(block_begin + GGUF_CONVERSION_BLOCK_SIZE, num_elements);', '            result.blocks()[internal_block_idx].size = block_end - block_begin;', '        }', '    }']
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: shape.push_back(static_cast<size_t>(dim));
- Line 272: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: shape.push_back(static_cast<size_t>(dim));

### llm/model_quantization_pipeline.cpp
Total findings: 9

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4370 [WIP] Update llm documentat... (2026-03-21) | #3266 feat(llm): GGUF/AWQ
- Line 110: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(dir)) {
- Line 255: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(dir)) {
- Line 338: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::runtime_error("dequantize_awq_layer: bits must be in [1, 8], got " +
- Line 402: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::runtime_error("dequantize_gptq_layer: bits must be in [1, 8], got " +
- Line 101: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (qtype == "awq") return ModelFormat::AWQ;

                    if (qtype == "gptq") return ModelFormat::GPTQ;

                }

            } catch (...) {

                // config.json present but malformed – fall through to heuristics

            }

        }
- Line 101: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 521: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, LayerBuffers> layers;
- Line 657: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, GptqBuffers> layers;

### llm/shared_worker_pool.cpp
Total findings: 9

- Line 109: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: worker.join();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3284 [llm] Implement prompt inje... (2026-03-12) | #3283 [llm] Propagate tim
- Line 73: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> glock(global_queue_mutex_);
- Line 75: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& q : thread_queues_) {
- Line 76: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> tlock(q->mutex);
- Line 146: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::unique_lock<std::mutex> glock(global_queue_mutex_);
- Line 175: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: cv_.wait_for(wlock, std::chrono::milliseconds(10), [this] {
- Line 200: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (size_t i = 1; i < n; ++i) {
- Line 132: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool found = false;

### llm/streaming_handler.cpp
Total findings: 9

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3284 [llm] Implement prompt inje... (2026-03-12) | #3283 [llm] Propagate tim
- Line 106: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::debug("StreamingHandler: emitting [DONE] for request {}", request_id);
- Line 50: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += "\\\\"; break;
- Line 51: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "\\\""; break;
- Line 52: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\b': out += "\\b";  break;
- Line 53: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\f': out += "\\f";  break;
- Line 54: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': out += "\\n";  break;
- Line 55: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': out += "\\r";  break;
- Line 56: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': out += "\\t";  break;

### llm/adapter_registry.cpp
Total findings: 8

- Line 667: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::error("AdapterRegistry::hotLoad: weights_path must not be empty");
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4333 [LORA-123] Implement LoRA a... (2026-03-19) | #3284 [llm] Implement pro
- Line 359: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Compatibility validation
- Line 384: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Check version compatibility (exact or empty means "any")
- Line 420: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 431: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 599: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> by_model;
- Line 600: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> by_domain;

### llm/decision_record_yaml_processor.cpp
Total findings: 8

- Line 45: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: thread_.join();
- Line 76: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void DecisionRecordYamlProcessor::flush() {

    std::unique_lock<std::mutex> lk(mutex_);

    cv_.wait(lk, [this] {

        const size_t submitted = submitted_.load(std::memory_order_relaxed);

        const size_t finalized = written_.load(std::memory_order_relaxed)

                               + errors_.load(std::memory_order_relaxed)
- Line 76: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: cv_.wait(lk, [this] {
- Line 104: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

            std::unique_lock<std::mutex> lk(mutex_);

            cv_.wait(lk, [this] { return !queue_.empty() || stop_.load(); });



            if (queue_.empty()) {

                // stop_ is true and queue is drained — exit.
- Line 104: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: cv_.wait(lk, [this] { return !queue_.empty() || stop_.load(); });
- Line 224: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Replace colons from ISO timestamp in filename (Windows compatibility)
- Line 207: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: char date_buf[11]; // "YYYY-MM-DD\0"
- Line 216: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||

### llm/lora_certificate_store.cpp
Total findings: 8

- Line 189: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 294: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const unsigned char* der_ptr = ctx->pbCertEncoded;
- Line 296: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: static_cast<long>(ctx->cbCertEncoded));
- Line 315: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const unsigned char* der_ptr2 = ctx->pbCertEncoded;
- Line 317: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: static_cast<long>(ctx->cbCertEncoded));
- Line 101: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: if (local_path.back() != '/' && local_path.back() != '\\') {
- Line 327: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (bio) BIO_free(bio);
- Line 328: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_free(match);

### llm/lora_framework/distributed_dataloader.cpp
Total findings: 8

- Line 101: severity=CRITICAL; category=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: batch_shape.insert(batch_shape.end(), sample_shape.begin(), sample_shape.end());
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12)
- Line 73: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: batch_samples.push_back(dataset_.get(sample_idx));
- Line 125: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "expected [{}], got [{}]; padding with zeros",
- Line 181: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: GPUTensor InMemoryDataset::get(size_t index) const {
- Line 97: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto sample_shape = batch_samples[gpu_start].shape();
- Line 120: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: s += (d ? "×" : "") + std::to_string(sh[d]);
- Line 131: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto sample_data = batch_samples[i].cpu_data();

### llm/lora_framework/gpu_embedding_layer.cpp
Total findings: 8

- Line 49: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Upload embedding weights to GPU
- Line 51: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: embedding_weights_.upload(weights_vec);
- Line 114: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Download embedding weights to CPU
- Line 242: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Download token IDs and embedding weights
- Line 290: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // Download token IDs and embedding weights
- Line 54: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: (vocab_size * hidden_dim * sizeof(float)) / (1024.0 * 1024.0));
- Line 127: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Note: Token IDs stored as floats in GPUTensor (architecture limitation - no int32 tensor support yet)

            // Using round() to handle potential floating point imprecision

            // TODO: Add integer tensor support to GPUTensor to avoid this conversion

            int token_id = static_cast<int>(std::round(token_data[token_idx]));

            

            // Bounds check

            if (token_id < 0 || token_id >= static_cast<int>(vocab_size_)) {
- Line 133: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool expected = false;

### llm/lora_framework/gpu_memory.cpp
Total findings: 8

- Line 342: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (enumerate_fill_result == VK_SUCCESS || enumerate_fill_result == VK_INCOMPLETE) {
- Line 465: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return directx_allocator_ ? directx_allocator_->get_stats() : VRAMAllocator::Stats{};
- Line 500: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (!directx_allocator_->is_available()) {
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: devices.push_back(Device::cpu());
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: devices.push_back(Device::cuda());
- Line 175: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: devices.push_back(Device::hip());
- Line 178: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: devices.push_back(Device::vulkan());
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: devices.push_back(Device::directx());

### llm/lora_framework/lora_checkpoint_manager.cpp
Total findings: 8

- Line 73: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 253: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(dir)) {
- Line 383: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entry : fs::directory_iterator(dir)) {
- Line 405: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (int64_t i = 0; i < delete_count; ++i) {

        uint64_t step = checkpoints[static_cast<size_t>(i)].first;

        // Never delete the best checkpoint

        if (config_.keep_best && best_meta.has_value() && best_meta->step == step) continue;

        fs::remove(fs::path(weightPath(adapter_id, step)));

        fs::remove(fs::path(metaPath(adapter_id, step)));
- Line 405: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Never delete the best checkpoint
- Line 77: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(ctx);
- Line 183: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            auto cur = readMeta(best_path.string());

            update = meta.val_loss < cur.val_loss;

        } catch (...) {}

    }

    if (update) {

        writeMeta(best_path.string(), meta);
- Line 183: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {}

### llm/lora_framework/lora_orchestrator.cpp
Total findings: 8

- Line 88: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->storage_service = std::make_shared<LoRAStorageService>(config.storage_config);
- Line 94: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->consistency_checker = std::make_shared<AdapterConsistencyChecker>(checker_config);
- Line 108: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->storage_service = std::make_shared<LoRAStorageService>(default_config.storage_config);
- Line 114: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->consistency_checker = std::make_shared<AdapterConsistencyChecker>(checker_config);
- Line 83: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("Failed to allocate LoRA Orchestrator Impl");
- Line 102: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("Failed to allocate LoRA Orchestrator Impl");
- Line 353: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 354: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### llm/model_downloader.cpp
Total findings: 8

- Line 41: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: stream->write(static_cast<char*>(ptr), total_size);
- Line 514: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::optional<ModelDownloadConfig> loadModelConfigFromYAML(
- Line 145: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {"name", config.model_name},

        {"stream", false}

    };

    std::string request_body = pull_request.dump();

    

    // Set CURL options

    curl_easy_setopt(curl, CURLOPT_URL, pull_url.c_str());
- Line 269: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ["        const auto colon_pos = filename.find(':');", '        if (colon_pos != std::string::npos) {', "            filename[colon_pos] = '-';", '        }', '']
- Line 438: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string show_url = ollama_url + "/api/show";

        json show_request = {{"name", model_name}};

        std::string request_body = show_request.dump();

        

        curl_easy_setopt(curl, CURLOPT_URL, show_url.c_str());

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
- Line 563: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // 2. Legacy map: models: { model_name: { ... } }
- Line 361: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: output_file.close();
- Line 501: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: models.push_back(model["name"]);

### llm/adapter_load_balancer.cpp
Total findings: 7

- Line 484: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = placements_.find(adapter_id);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4333 [LORA-123] Implement LoRA a... (2026-03-19) | #998 C++ Audit: Eliminate
- Line 287: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = placements_.find(adapter_id);
- Line 288: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto it = placements_.find(adapter_id);
- Line 306: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = placements_.find(adapter_id);
- Line 319: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 483: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = placements_.find(adapter_id);

### llm/aql_train_parser.cpp
Total findings: 7

- Line 815: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: stmt->enrichment = parseEnrichment(using_part);
- Line 18: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: *   LIST ADAPTERS [WHERE ...] [ORDER BY ...] [LIMIT n]
- Line 571: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("AQLTrainParser: epochs must be in [1, 100]");
- Line 577: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("AQLTrainParser: lora_rank must be in [1, 256]");
- Line 673: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("AQLTrainParser: threshold must be in [0, 1]");
- Line 918: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (lower.find("signature")         != std::string::npos) stmt->check_signature         = true;
- Line 919: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (lower.find("manifest")          != std::string::npos) stmt->check_manifest          = true;

### llm/llama_lora_adapter.cpp
Total findings: 7

- Line 37: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Compatibility:
- Line 47: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: //   an error and return -1 / nullptr.  Additionally, the legacy
- Line 201: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: * @param adapter_path Path to LoRA adapter file (for compatibility with old signature)
- Line 204: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: * @note The signature matches the legacy stub for backward compatibility.
- Line 228: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Note: This simplified signature is for backward compatibility only
- Line 443: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: * @brief Apply LoRA adapter with integer handle (compatibility overload)
- Line 64: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'LlamaCpp LoRA Adapter Runtime Activation' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llm/FUTURE_ENHANCEMENTS.md §"LlamaCpp LoRA Adapter Runtime Activation"

### llm/lora_framework/distributed_trainer.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #570 [LoRA Phase 10] Add
- Line 194: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: //                   or multi-node training this leads to gradient staleness
- Line 197: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Distributed Trainer Barrier.' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llm/FUTURE_ENHANCEMENTS.md §Distributed Trainer Barrier.
- Line 289: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: backends.push_back(DistributedBackend::NONE);
- Line 293: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: backends.push_back(DistributedBackend::NCCL);
- Line 298: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: backends.push_back(DistributedBackend::GLOO);
- Line 303: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: backends.push_back(DistributedBackend::MPI);

### llm/lora_framework/gradient_utils.cpp
Total findings: 7

- Line 58: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: spdlog::debug("Clipped gradients: norm={:.4f} -> {:.4f}", global_norm, max_norm);
- Line 121: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: accumulated.push_back(grad_ptr->clone());
- Line 129: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 130: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 134: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 139: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 245: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: accumulated_gradients_.push_back(Tensor(grad_ptr->shape(), 0.0f));

### llm/ai_orchestrator.cpp
Total findings: 6

- Line 507: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: json tool_result = impl_->tool_registry.invokeTool(search_tool, args, mode);
- Line 575: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: InferenceResponse resp   = impl_->plugin->generateRAG(rag_ctx, req);
- Line 623: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: json tool_result = impl_->tool_registry.invokeTool(tool_name, tool_args, mode);
- Line 467: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: result.metadata.tokens_generated  = resp.tokens_generated;
- Line 581: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: result.metadata.tokens_generated = resp.tokens_generated;
- Line 461: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: InferenceResponse resp = impl_->plugin->generate(req);

### llm/explanation_generator.cpp
Total findings: 6

- Line 223: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(response_keywords.begin(), response_keywords.end(), qk)
- Line 283: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(alternatives.size(), size_t(3)); i++) {
- Line 124: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: out << "(GDPR Article 22 / EU AI Act Compliance)\n\n";
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("Analyzed the input query to understand intent");
- Line 200: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back(step.str());
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("Generated response based on processed information");

### llm/feedback_store.cpp
Total findings: 6

- Line 485: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: feedback->training_batch_id = batch_id;
- Line 453: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Failed to delete feedback {}: {}", id, s.ToString());
- Line 453: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    if (!s.ok()) {

        THEMIS_ERROR("Failed to delete feedback {}: {}", id, s.ToString());

        return false;

    }
- Line 453: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Failed to delete feedback {}: {}", id, s.ToString());
- Line 598: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::regex repeat_pattern(R"((.)\1{10,})"); // Same character 10+ times
- Line 816: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back(*feedback);

### llm/lora_framework/gpu_lora_layers.cpp
Total findings: 6

- Line 136: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // All GPU tensors are allocated with cudaMalloc which provides proper alignment
- Line 358: severity=CRITICAL; category=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // All GPU tensors (including gradients) are allocated with cudaMalloc
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #998 C++ Audit: Eliminate raw me... (2026-03-11) | #575 [LoRA Phase 10.4] Imp
- Line 109: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // NOTE: For backward pass compatibility with existing code, we still
- Line 114: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // For now, we prioritize API compatibility and correctness.
- Line 239: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Keep backward compatibility with existing checkpoint logic.

### llm/lora_framework/resource_profiler.cpp
Total findings: 6

- Line 93: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const float n = static_cast<float>(impl_->snapshots.size());
- Line 86: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: stats.peak_gpu_memory = std::max(stats.peak_gpu_memory, s.gpu_memory_allocated);
- Line 138: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return a.gpu_memory_allocated < b.gpu_memory_allocated;
- Line 139: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: })->gpu_memory_allocated;
- Line 153: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 154: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### llm/mode_spec_loader.cpp
Total findings: 6

- Line 351: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::to_string(mode.budgets.temperature) + " is outside [0, 2]");
- Line 360: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(valid_strategies.begin(), valid_strategies.end(), strat) == valid_strategies.end()) {
- Line 397: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (seenTools.find(ta) == seenTools.end()) {
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(item.as<std::string>());
- Line 333: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> seenIds;
- Line 382: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, int> seenTools;

### llm/safety/monitoring.cpp
Total findings: 6

- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"': out += "\\\""; break;
- Line 36: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"': out += "\\\""; break;
- Line 37: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += "\\\\"; break;
- Line 38: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': out += "\\n"; break;
- Line 39: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': out += "\\r"; break;
- Line 40: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': out += "\\t"; break;

### llm/ai_decision_auditor.cpp
Total findings: 5

- Line 482: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        

        json export_data;

        export_data["export_timestamp"] = std::chrono::system_clock::to_time_t(

            std::chrono::system_clock::now()

        );

        export_data["total_decisions"] = decisions.size();
- Line 485: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: export_data["export_timestamp"] = std::chrono::system_clock::to_time_t(

            std::chrono::system_clock::now()

        );

        export_data["total_decisions"] = decisions.size();

        export_data["decisions"] = json::array();

        

        for (const auto& decision : decisions) {
- Line 486: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::chrono::system_clock::now()

        );

        export_data["total_decisions"] = decisions.size();

        export_data["decisions"] = json::array();

        

        for (const auto& decision : decisions) {

            export_data["decisions"].push_back(decision.toJson());
- Line 489: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: export_data["decisions"] = json::array();

        

        for (const auto& decision : decisions) {

            export_data["decisions"].push_back(decision.toJson());

        }

        

        out << export_data.dump(2); // Pretty-print with 2-space indent
- Line 493: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: out.close();

### llm/byzantine_detector.cpp
Total findings: 5

- Line 365: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (std::find(selected.begin(), selected.end(), shard_id) == selected.end()) {
- Line 326: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: selected.push_back(scores[i].second);
- Line 422: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: values.push_back(shard_grads[layer_idx].data[coord]);
- Line 492: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: const std::map<std::string, std::vector<GradientTensor>>& shard_gradients
- Line 526: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, float> all_scores = median_result.anomaly_scores;

### llm/lora_framework/adapter_consistency_checker.cpp
Total findings: 5

- Line 80: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 86: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 98: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_MD_CTX_free(mdctx);
- Line 100: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: EVP_PKEY_free(pkey);
- Line 170: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: // If versions are equal, compare timestamps

### llm/lora_framework/flash_lora.cpp
Total findings: 5

- Line 425: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 500: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "Input must be 2D [batch, in_dim] or 3D [batch, seq_len, in_dim]"
- Line 507: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("B must be 2D [rank, in_dim]");
- Line 513: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("A must be 2D [out_dim, rank]");
- Line 516: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Check dimension compatibility

### llm/lora_framework/vulkan_buffer.cpp
Total findings: 5

- Line 35: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 193: severity=CRITICAL; category=unchecked_memcpy
  Description: memcpy size parameter not validated (potential buffer overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['        // For host-visible buffers, map and copy directly', '        void* mapped = map();', '        std::memcpy(static_cast<char*>(mapped) + offset, data, size);', '        unmap();', '    }']
- Line 211: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: VkResult map_result = vkMapMemory(context_->device(), memory_, offset, size, 0, &mapped_memory);
- Line 285: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: VkResult submit_result = vkQueueSubmit(context_->compute_queue(), 1, &submit_info, fence);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3629 [MODULE] llm â€“ bu

### llm/paged_kv_cache.cpp
Total findings: 5

- Line 53: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t blocks_to_allocate = num_blocks_needed - current_blocks.size();
- Line 54: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: block_table->allocateBlocks(blocks_to_allocate);
- Line 92: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto block_it = kv_storage_.find(block_id);
- Line 94: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto layer_it = block_it->second.find(layer_id);
- Line 118: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### llm/prompt_manager.cpp
Total findings: 5

- Line 152: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: return false;

    }

    

    acc->second.metadata["experiment_id"] = experiment_id;



    if (db_) {

        std::string key = std::string(KEY_PREFIX) + id;
- Line 38: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: acc.release(); // Release lock
- Line 218: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: YAML::Emitter emitter;

                    emitter << prompt_node["metadata"];

                    pt.metadata = nlohmann::json::parse(emitter.c_str());

                } catch (...) {

                    pt.metadata = nlohmann::json::object();

                }

            }
- Line 218: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 240: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& context) const {

### llm/block_table.cpp
Total findings: 4

- Line 31: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 39: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 45: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 67: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: block_manager_->deallocate(block_id);

### llm/llm_ingestion_bridge.cpp
Total findings: 4

- Line 41: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = LLMPluginManager::instance().generate(req);
- Line 13: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: * Bridges ingestion::ITextGenerationBackend to LLMPluginManager::generate().
- Line 41: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = LLMPluginManager::instance().generate(req);
- Line 44: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::error("[LlmIngestionBridge] generate() failed: {}", e.what());

### llm/llm_interaction_store.cpp
Total findings: 4

- Line 339: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: THEMIS_ERROR("Failed to delete LLM interaction {}: {}", id, s.ToString());
- Line 339: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    if (!s.ok()) {

        THEMIS_ERROR("Failed to delete LLM interaction {}: {}", id, s.ToString());

        return false;

    }
- Line 339: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: THEMIS_ERROR("Failed to delete LLM interaction {}: {}", id, s.ToString());
- Line 400: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Merge metadata updates

    for (const auto& [key, value] : metadata_updates.items()) {

        interaction.metadata[key] = value;

    }

    

    // Store updated interaction

### llm/lookup_decoder.cpp
Total findings: 4

- Line 57: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 58: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 113: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it = index_.find(key);
- Line 64: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::vector<int>,

### llm/lora_framework/directx_context.cpp
Total findings: 4

- Line 180: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: HRESULT hr = device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_));
- Line 257: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: HRESULT hr = command_queue_->Signal(fence_.Get(), fence_to_wait);
- Line 162: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::cout << "DirectX 12: Using adapter " << adapter_id_
- Line 162: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "DirectX 12: Using adapter " << adapter_id_

### llm/lora_framework/directx_pipeline.cpp
Total findings: 4

- Line 42: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 106: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // UAV descriptor table (outputs)
- Line 125: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // SRV descriptor table (inputs)
- Line 207: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "DirectXPipeline: Created compute pipeline\n";

### llm/lora_framework/llama_tokenizer.cpp
Total findings: 4

- Line 31: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: model_params.vocab_only = true;  // Only load tokenizer, not weights
- Line 33: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: model_ = llama_load_model_from_file(model_path.c_str(), model_params);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #594 [LoRa] Integrate llama.cpp ... (2026-03-11)
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(eos_token_id());

### llm/lora_framework/lora_feedback_storage.cpp
Total findings: 4

- Line 35: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool ok = config_.db->open();
- Line 419: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto status = config_.graph_index->addEdge(edge);
- Line 442: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto status = config_.graph_index->deleteEdge(edge_id);
- Line 182: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### llm/lora_framework/quantized_model.cpp
Total findings: 4

- Line 156: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: cached_BA_ = B_->matmul(*A_);
- Line 413: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::debug("Loaded pre-quantized tensor: {} (using real quantized weights)",
- Line 299: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, Tensor>& model_weights,
- Line 382: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: shape.push_back(static_cast<size_t>(dim));

### llm/lora_framework/vulkan_context.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5205 fix(llm): harden LoRA input... (2026-05-23) | #3629 [MODULE] llm â€“ bu
- Line 332: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Enable validation layers for device (deprecated but still used in some drivers)
- Line 465: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

    VkResult result = vkResetFences(device_, 1, &fence);

    if (result != VK_SUCCESS) {

        throw std::runtime_error("Failed to reset fence");

    }

}
- Line 286: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Selected Vulkan device: " << device_properties_.deviceName << '\n';

### llm/prompt_optimizer.cpp
Total findings: 4

- Line 112: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.final_score = current_score;

    result.converged = result.converged || (current_score >= config_.target_score);

    

    result.metadata["initial_score"] = result.score_history[0];

    result.metadata["improvement"] = current_score - result.score_history[0];

    result.metadata["relative_improvement"] = 

        (result.score_history[0] > 0) ?
- Line 113: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.converged = result.converged || (current_score >= config_.target_score);

    

    result.metadata["initial_score"] = result.score_history[0];

    result.metadata["improvement"] = current_score - result.score_history[0];

    result.metadata["relative_improvement"] = 

        (result.score_history[0] > 0) ? 

        (current_score - result.score_history[0]) / result.score_history[0] : 0.0;
- Line 114: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["initial_score"] = result.score_history[0];

    result.metadata["improvement"] = current_score - result.score_history[0];

    result.metadata["relative_improvement"] = 

        (result.score_history[0] > 0) ? 

        (current_score - result.score_history[0]) / result.score_history[0] : 0.0;
- Line 169: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto meta_result = meta_gen.generateImprovementPrompt(

### llm/security/signature_verifier.cpp
Total findings: 4

- Line 52: severity=HIGH; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Validate inputs
- Line 607: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: X509_CRL_free(crl_cache_.crl);
- Line 662: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: OPENSSL_free(hex);
- Line 664: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: BN_free(bn);

### llm/adaptive_vram_allocator.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4333 [LORA-123] Implement LoRA a... (2026-03-19) | #3802 [LLM] AdaptiveVRAMA
- Line 193: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: bool AdaptiveVRAMAllocator::allocateWithFragmentation(size_t bytes, void** ptr) {
- Line 197: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return impl_->active_allocator_.allocateWithFragmentation(bytes, ptr);

### llm/federated_inference_coordinator.cpp
Total findings: 3

- Line 194: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
- Line 248: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: resp.tokens_generated = data["tokens_generated"].get<int>();
- Line 254: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: resp.inference_time_ms = data["inference_time_ms"].get<float>();

    }

    // Tag the response with the instance that produced it.

    resp.metadata["source_instance_id"] = instance_id;

    return resp;

}

### llm/grammar.cpp
Total findings: 3

- Line 91: severity=HIGH; category=manual_cleanup_in_destructor
  Description: Manual resource cleanup in destructor (should use RAII wrapper)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: Grammar::~Grammar() {
- Line 18: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: void llama_grammar_free(struct llama_grammar* grammar);
- Line 142: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: llama_grammar* Grammar::getHandle() const {

### llm/llamacpp_inference_engine.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #751 Phase 4 Error Handling: Sto... (2026-03-11) | #712 [Error Handling] Phas
- Line 335: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // (new count > 1) this occurrence is a repetition.
- Line 397: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t pattern_len = 5; pattern_len <= std::min(text.length() / 4, size_t(50)); ++pattern_len)

### llm/lora_framework/adaptive_batcher.cpp
Total findings: 3

- Line 98: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 116: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 121: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### llm/lora_framework/custom_allreduce.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #578 [LoRA Phase 10.5] Implement... (2026-03-11)
- Line 184: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (const auto& gpu_data : all_gpu_data) {

        for (size_t i = 0; i < tensor_size; ++i) {

            reduced_data[i] += gpu_data[i];

        }

    }
- Line 153: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'CustomAllReduce NCCL Integration.' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llm/FUTURE_ENHANCEMENTS.md §CustomAllReduce NCCL Integration.

### llm/lora_framework/data_loader.cpp
Total findings: 3

- Line 50: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back(eos_token_id());
- Line 354: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: batch_indices.push_back(indices_[i]);
- Line 328: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // For causal language modeling, labels are the same as inputs shifted by 1

### llm/lora_framework/directx_buffer.cpp
Total findings: 3

- Line 37: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 321: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 330: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### llm/lora_framework/directx_descriptors.cpp
Total findings: 3

- Line 32: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 145: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: D3D12_CPU_DESCRIPTOR_HANDLE DirectXDescriptors::get_cpu_handle(uint32_t index) const {
- Line 151: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: D3D12_GPU_DESCRIPTOR_HANDLE DirectXDescriptors::get_gpu_handle(uint32_t index) const {

### llm/lora_framework/feedback_plugin.cpp
Total findings: 3

- Line 23: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void PrivacyFilterPlugin::process(Feedback& feedback) {
- Line 124: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"(https?://[^\s]+)"
- Line 239: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void CacheAwareWeightingPlugin::process(Feedback& feedback) {

### llm/lora_framework/model_compatibility.cpp
Total findings: 3

- Line 252: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ModelArchitecture ModelCompatibilityChecker::detect_architecture(const json& metadata) {

    if (metadata.contains("model_type")) {

        return ModelMetadata::string_to_architecture(metadata["model_type"]);

    }

    return ModelArchitecture::UNKNOWN;

}
- Line 287: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Step 4: Check quantization compatibility
- Line 236: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto meta = header["__metadata__"];

### llm/model_router.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4370 [WIP] Update llm documentat... (2026-03-21) | #3269 feat(llm): multi-mo
- Line 131: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (t.is_string()) tags.push_back(t.get<std::string>());

### llm/multi_gpu_memory_coordinator.cpp
Total findings: 3

- Line 557: severity=HIGH; category=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::gpu_memory
  Context: // Use cudaMemcpyPeer for direct GPU-to-GPU transfer
- Line 393: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::info("MultiGPUMemoryCoordinator: P2P setup complete ({} success, {} failed)",
- Line 464: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::info("MultiGPUMemoryCoordinator: P2P setup complete ({} success, {} failed)",

### llm/openai_compat_adapter.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4370 [WIP] Update llm documentat... (2026-03-21) | #4187 feat(llm): OpenAI-c
- Line 399: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Unknown roles are silently skipped to allow forward-compatibility
- Line 361: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: content_text += part["text"].get<std::string>();

### llm/paged_block_manager.cpp
Total findings: 3

- Line 24: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Resolve number of blocks (support legacy total_blocks alias)
- Line 79: severity=HIGH; category=missing_trace_point
  Description: Critical function allocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: int PagedBlockManager::allocate() {
- Line 102: severity=HIGH; category=missing_trace_point
  Description: Critical function deallocate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void PagedBlockManager::deallocate(int block_id) {

### llm/attention/flash_attention.cpp
Total findings: 2

- Line 268: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 169: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Validate inputs.

### llm/gpu_safe_fail.cpp
Total findings: 2

- Line 404: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: spdlog::error("Memory pressure CRITICAL - blocking new allocation of {} MB",
- Line 404: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### llm/kernel_fusion.cpp
Total findings: 2

- Line 301: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['                        int attn_idx = ((b * num_heads + h) * seq_len_q + i) * seq_len_kv + j;', '                        int val_idx = ((b * num_heads + h) * seq_len_kv + j) * head_dim + d;', '                        sum += attention_weights[attn_idx] * values[val_idx];', '                    }', '']
- Line 305: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['', '                    int out_idx = ((b * num_heads + h) * seq_len_q + i) * head_dim + d;', '                    output[out_idx] = sum;', '                }', '            }']

### llm/kv_cache_buffer.cpp
Total findings: 2

- Line 189: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 258: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: .acquired_buffers = buffers_.size() - available

### llm/llama_grammar_adapter.cpp
Total findings: 2

- Line 37: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Compatibility:
- Line 60: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'LlamaCpp Grammar API Runtime Activation' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llm/FUTURE_ENHANCEMENTS.md §"LlamaCpp Grammar API Runtime Activation"

### llm/llm_prefix_cache.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3759 feat(llm): implement KV-cac... (2026-03-12) | #239 Replace LLMPrefixCac
- Line 288: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (mag_a == 0.0 || mag_b == 0.0) {

### llm/lora_framework/embedding_provider.cpp
Total findings: 2

- Line 365: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: llama_batch_free(batch);
- Line 383: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: llama_batch_free(batch);

### llm/lora_framework/gpu_utilization_monitor.cpp
Total findings: 2

- Line 355: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'GPUUtilizationMonitor VulkanMetrics.' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llm/FUTURE_ENHANCEMENTS.md §GPUUtilizationMonitor VulkanMetrics.
- Line 400: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'GPUUtilizationMonitor DirectXMetrics.' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llm/FUTURE_ENHANCEMENTS.md §GPUUtilizationMonitor DirectXMetrics.

### llm/lora_framework/mixed_precision.cpp
Total findings: 2

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4678 feat: replace production st... (2026-04-15) | #570 [LoRA Phase 10] Add
- Line 92: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (size_t i = 0; i < grad_ptr->size(); ++i) {

### llm/lora_framework/sequence_packer.cpp
Total findings: 2

- Line 93: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: spdlog::error("Expected 2D packed output [total_tokens, hidden_dim], got {}D",
- Line 127: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (size_t j = 0; j < length; ++j) {

            size_t token_idx = offset + j;

            for (size_t k = 0; k < hidden_dim; ++k) {

                seq_data.push_back(packed_data[token_idx * hidden_dim + k]);

            }

        }

### llm/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### llm/feedback_plugin_basic.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12)

### llm/inference_handle.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #995 Library usage analys

### llm/llm_model_audit_logger.cpp
Total findings: 1

- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: impl->records.push_back({

### llm/llm_security_utils.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3629 [MODULE] llm â€“ build-syst... (2026-03-12) | #3284 [llm] Implement pro

### llm/lora_framework/directx_shader.cpp
Total findings: 1

- Line 67: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "DirectXShader: Loaded shader from " << shader_path_

### llm/lora_framework/multi_gpu.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #578 [LoRA Phase 10.5] Implement... (2026-03-11)

### llm/lora_framework/nccl_backend.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #578 [LoRA Phase 10.5] Implement... (2026-03-11)

### llm/lora_framework/quantization.cpp
Total findings: 1

- Line 33: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'LoRA Quantization Logging' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llm/FUTURE_ENHANCEMENTS.md §"LoRA Quantization Logging"

### llm/lora_framework/rccl_backend.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #578 [LoRA Phase 10.5] Implement... (2026-03-11)

### llm/mcp_tool_bridge.cpp
Total findings: 1

- Line 24: severity=HIGH; category=layer_dependency_violation
  Description: Module 'llm' must not depend on 'server' (layer violation)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_architecture_rules
  Context: #include "server/mcp_server.h"

### llm/safety/classifier.cpp
Total findings: 1

- Line 30: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (text.find(needle) != std::string::npos) {

### llm/speculative_decoder.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4370 [WIP] Update llm documentat... (2026-03-21) | #3177 [llm] Speculative d

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
