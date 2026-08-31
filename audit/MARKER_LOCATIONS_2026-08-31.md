# ThemisDB — Marker Fund­orte in `src/`

**Erstellt:** 2026-08-31  
**Commit:** `f159aa34be`  
**Branch:** `develop`  
**Werkzeug:** `grep -rn --include='*.{cpp,cc,c,h,hpp}' -E 'TODO|STUB|MOCK|FIXME' src/`

## Zusammenfassung

| Marker | Anzahl |
|--------|--------|
| TODO | 1538 |
| STUB | 192 |
| MOCK | 6 |
| FIXME | 1 |
| **Gesamt** | **1730** |

---

## Fundorte nach Marker-Typ

### TODO

| Datei | Zeile | Kontext |
|-------|-------|---------|
| `training/database_domain_auto_labeler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `training/examples/database_optimizer_labeler.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=1, M=4, L=0` |
| `training/adapter_serving.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `training/knowledge_graph_enricher.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `training/modality_parser.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0` |
| `training/lora_checkpoint_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `training/auto_labeler.cpp` | 7 | `* @note Gap Summary: total=9; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=2, C=0, H=6, M=23, L=0` |
| `training/multi_task_lora.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=18, H=24, M=3, L=0` |
| `training/adalora_tt_bridge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=3, L=0` |
| `training/provenance_tracker.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=1, Debt=0, C=3, H=1, M=8, L=0` |
| `training/incremental_lora_trainer.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=21, H=29, M=10, L=0` |
| `training/lora_data_selection.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=6, H=11, M=16, L=0` |
| `training/training_pipeline.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=4, H=3, M=4, L=0` |
| `training/lora_adapter.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=2, Debt=0, C=7, H=7, M=0, L=0` |
| `training/lora_adapter_merger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=8, L=0` |
| `training/ada_lora_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=14, M=1, L=0` |
| `llm/llama_wrapper.cpp` | 7 | `* @note Gap Summary: total=5; TODO=2, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=35, H=76, M=13, L=0` |
| `llm/token_quota_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/paged_kv_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `llm/prompt_policy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/byzantine_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0` |
| `llm/gguf_loader.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=0, M=8, L=0` |
| `llm/multi_lora_manager.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=20, H=25, M=25, L=0` |
| `llm/llamacpp_inference_engine.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=1, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `llm/gpu_safe_fail.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0` |
| `llm/llama_resource_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0` |
| `llm/multi_model_training_data.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `llm/production_validator.cpp` | 7 | `* @note Gap Summary: total=14; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=11, Debt=0, C=2, H=25, M=11, L=0` |
| `llm/llm_deployment_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=1, M=6, L=0` |
| `llm/ml_model_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=11, H=48, M=11, L=0` |
| `llm/llm_security_utils.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/mixed_precision_inference.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=13, M=0, L=0` |
| `llm/model_router.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `llm/explanation_generator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=4, M=5, L=0` |
| `llm/feedback_plugin_basic.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/vision_encoder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=10, H=3, M=5, L=0` |
| `llm/lora_framework/adapter_consistency_checker.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `llm/lora_framework/directx_buffer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/lora_framework/lora_storage_service_themisdb.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=4, Sim=0, Debt=0, C=14, H=12, M=7, L=0` |
| `llm/lora_framework/quantization.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=12, H=10, M=1, L=0` |
| `llm/lora_framework/base_model_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=9, L=0` |
| `llm/lora_framework/distributed_trainer.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/lora_framework/lr_scheduler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/lora_framework/gpu_data_loader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=3, L=0` |
| `llm/lora_framework/adapter_sync_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=11, L=0` |
| `llm/lora_framework/mixed_precision.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=3, H=3, M=0, L=0` |
| `llm/lora_framework/directx_descriptors.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `llm/lora_framework/distributed_dataloader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=3, L=0` |
| `llm/lora_framework/directx_context.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=0, L=0` |
| `llm/lora_framework/sequence_packer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `llm/lora_framework/lora_checkpoint_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0` |
| `llm/lora_framework/lora_provenance.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0` |
| `llm/lora_framework/gpu_training_loop.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=8, M=0, L=0` |
| `llm/lora_framework/paged_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0` |
| `llm/lora_framework/gguf_converter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=0, L=0` |
| `llm/lora_framework/gradient_utils.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0` |
| `llm/lora_framework/rccl_backend.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/lora_framework/adaptive_batcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=0, L=0` |
| `llm/lora_framework/gpu_utilization_monitor.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=3, Unimpl=1, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/lora_framework/vulkan_pipeline.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=4, M=1, L=0` |
| `llm/lora_framework/gradient_checkpointing.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=0, L=0` |
| `llm/lora_framework/lora_orchestrator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=3, M=5, L=0` |
| `llm/lora_framework/feedback_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/lora_framework/nccl_backend.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/lora_framework/gpu_embedding_layer.cpp` | 7 | `* @note Gap Summary: total=4; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=0, M=0, L=0` |
| `llm/lora_framework/lora_training_service.cpp` | 7 | `* @note Gap Summary: total=7; TODO=2, Stub=2, Unimpl=0, Mock=1, Sim=2, Debt=0, C=30, H=35, M=7, L=0` |
| `llm/lora_framework/quantized_model.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=4, M=2, L=0` |
| `llm/lora_framework/lora_feedback_storage.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=0, L=0` |
| `llm/lora_framework/vulkan_buffer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=0, L=0` |
| `llm/lora_framework/lora_layers.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=14, H=13, M=0, L=0` |
| `llm/lora_framework/directx_pipeline.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=0, L=0` |
| `llm/lora_framework/paged_memory_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=0, L=0` |
| `llm/lora_framework/directx_shader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `llm/lora_framework/gpu_memory.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=8, L=0` |
| `llm/lora_framework/resource_profiler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=1, L=0` |
| `llm/lora_framework/kernels/vulkan_kernels.cpp` | 7 | `* @note Gap Summary: total=14; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=11, Debt=0, C=66, H=87, M=2, L=0` |
| `llm/lora_framework/kernels/cpu_fused_kernels.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=8, H=19, M=0, L=0` |
| `llm/lora_framework/kernels/hip_kernels.cpp` | 7 | `* @note Gap Summary: total=4; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=13, H=14, M=2, L=0` |
| `llm/lora_framework/kernels/hip_fused_kernels.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=13, H=13, M=0, L=0` |
| `llm/lora_framework/kernels/directx_kernels.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=2, Debt=0, C=56, H=45, M=0, L=0` |
| `llm/lora_framework/data_loader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=9, H=9, M=7, L=0` |
| `llm/lora_framework/custom_allreduce.cpp` | 7 | `* @note Gap Summary: total=11; TODO=1, Stub=3, Unimpl=1, Mock=1, Sim=5, Debt=0, C=0, H=0, M=1, L=0` |
| `llm/lora_framework/multi_gpu_trainer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=13, M=4, L=0` |
| `llm/lora_framework/training_service_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/lora_framework/lora_audit_logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=19, M=1, L=0` |
| `llm/lora_framework/flash_lora.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=24, H=31, M=0, L=0` |
| `llm/lora_framework/lora_training_config.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=15, L=0` |
| `llm/lora_framework/model_compatibility.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `llm/lora_framework/gpu_lora_layers.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=34, H=36, M=0, L=0` |
| `llm/lora_framework/lora_storage_service.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=5, L=0` |
| `llm/lora_framework/llama_tokenizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/lora_framework/vram_allocator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=54, M=3, L=0` |
| `llm/lora_framework/gpu_tensor.cpp` | 7 | `* @note Gap Summary: total=11; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=3, M=2, L=0` |
| `llm/lora_framework/vulkan_context.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `llm/lora_framework/multi_gpu_lora_layer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=19, M=5, L=0` |
| `llm/lora_framework/multi_gpu.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `llm/lora_framework/embedding_provider.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `llm/ethics_aware_confidence_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=9, M=0, L=0` |
| `llm/inference_engine_enhanced.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=76, M=19, L=0` |
| `llm/model_switch_workflow.cpp` | 7 | `* @note Gap Summary: total=3; TODO=0, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=2, L=0` |
| `llm/grammar.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `llm/kernel_fusion_cuda.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `llm/llamacpp_training_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `llm/block_table.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `llm/lora_security_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=1, M=9, L=0` |
| `llm/paged_kv_cache_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=3, L=0` |
| `llm/batch_generator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `llm/feedback_store.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=1, L=0` |
| `llm/active_vram_allocator.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=13, M=5, L=0` |
| `llm/kv_cache_buffer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `llm/adaptive_vram_allocator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=0, L=0` |
| `llm/model_downloader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=7, M=2, L=0` |
| `llm/grammar_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/training_data_iterator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `llm/multi_gpu_memory_coordinator.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=7, Debt=0, C=0, H=1, M=9, L=0` |
| `llm/federated_inference_coordinator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=3, L=0` |
| `llm/llama_grammar_adapter.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/lookup_decoder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `llm/embedded_llm_stub.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=7, M=10, L=0` |
| `llm/embedded_llm.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=6, M=4, L=0` |
| `llm/adapter_load_balancer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `llm/lora_certificate_store.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0` |
| `llm/llama_lora_adapter.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=0, L=0` |
| `llm/openai_compat_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0` |
| `llm/llm_client.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `llm/constitutional_reasoning_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=14, H=7, M=5, L=0` |
| `llm/ssm_state_rocksdb_store.cpp` | 261 | `// TODO: Use protobuf or binary serialization for efficiency` |
| `llm/docs_assistant.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=9, H=19, M=15, L=1` |
| `llm/ai_orchestrator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=6, M=3, L=0` |
| `llm/vision_config.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=63, H=8, M=33, L=0` |
| `llm/mcp_tool_bridge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=0, L=0` |
| `llm/model_quantization_pipeline.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=8, L=0` |
| `llm/llm_plugin_manager.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=11, H=10, M=8, L=0` |
| `llm/fewshot_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=5, M=1, L=0` |
| `llm/grafana_metrics.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=5, Debt=0, C=4, H=32, M=5, L=0` |
| `llm/model_metadata_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/llm_interaction_store.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `llm/ai_decision_auditor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `llm/shared_worker_pool.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0` |
| `llm/streaming_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `llm/sampling_strategy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=3, L=0` |
| `llm/moral_analyzer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=2, M=9, L=0` |
| `llm/json_schema_converter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=17, L=0` |
| `llm/security/signature_verifier.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `llm/meta_prompt_generator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0` |
| `llm/llm_ingestion_bridge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0` |
| `llm/gpu_memory_manager.cpp` | 7 | `* @note Gap Summary: total=28; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=24, Debt=0, C=7, H=28, M=36, L=0` |
| `llm/speculative_decoder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/prompt_manager.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `llm/lora_metadata_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `llm/vision_resource_monitor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=17, M=7, L=0` |
| `llm/paged_block_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `llm/continuous_batch_scheduler.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=13, M=3, L=0` |
| `llm/model_loader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=20, H=11, M=3, L=0` |
| `llm/distributed_training_coordinator.cpp` | 7 | `* @note Gap Summary: total=24; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=21, Debt=0, C=4, H=7, M=18, L=0` |
| `llm/llm_model_audit_logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=9, M=1, L=0` |
| `llm/prompt_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=2, L=0` |
| `llm/infini_attention_cpu.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `llm/inline_training_engine.cpp` | 7 | `* @note Gap Summary: total=9; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=2, Debt=0, C=1, H=0, M=4, L=0` |
| `llm/llm_model_storage.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=17, H=9, M=8, L=0` |
| `llm/kernel_fusion.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=7, M=0, L=0` |
| `llm/ethical_guidelines_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=17, L=0` |
| `llm/applications/themis_help_lora.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=12, M=6, L=0` |
| `llm/adapter_deployment_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `llm/adapter_registry.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=9, L=0` |
| `llm/lora_router.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=13, H=0, M=3, L=0` |
| `llm/llm_response_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=9, M=1, L=0` |
| `llm/inference_handle.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `llm/prompt_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `llm/lazy_model_loader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `llm/kv_prefix_transfer_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `llm/final_layer_orchestrator.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=2, L=0` |
| `llm/safety/monitoring.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `llm/safety/guardian.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `llm/safety/classifier.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=13, M=1, L=0` |
| `llm/multi_perspective_generator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=1, M=18, L=0` |
| `llm/attention/flash_attention.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `llm/attention/kv_cache_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0` |
| `llm/aql_train_parser.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=18, H=20, M=8, L=0` |
| `llm/decision_record_yaml_processor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=1, L=0` |
| `llm/mode_spec_loader.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=12, L=0` |
| `llm/gguf_st_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `llm/llm_prefix_cache.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `main.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `sharding/shard_router.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=23, H=31, M=12, L=0` |
| `sharding/paged_kv_cache.cpp` | 9 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0` |
| `sharding/hardware_migration_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `sharding/gpu_erasure_coder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `sharding/multi_primary_coordinator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=1, M=1, L=0` |
| `sharding/locality_aware_router.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0` |
| `sharding/transaction_wal.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=2, L=0` |
| `sharding/slo_monitor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=9, M=12, L=0` |
| `sharding/partition_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=7, L=0` |
| `sharding/data_migrator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=4, M=2, L=0` |
| `sharding/remote_executor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=5, M=2, L=0` |
| `sharding/cross_shard_speculative_decoder.cpp` | 9 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0` |
| `sharding/shard_topology.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=15, L=0` |
| `sharding/raft_consensus_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `sharding/admin_api.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `sharding/urn.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `sharding/inference_engine_enhanced.cpp` | 9 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0` |
| `sharding/paxos_snapshot.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=9, M=1, L=0` |
| `sharding/consistent_hash.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=4, L=0` |
| `sharding/consensus_factory.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `sharding/distributed_coordinator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `sharding/hot_spare_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=6, M=7, L=0` |
| `sharding/distributed_transaction.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=29, M=28, L=0` |
| `sharding/rebalance_operation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `sharding/raft_log.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=5, M=5, L=0` |
| `sharding/mtls_connection_pool.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=9, M=0, L=0` |
| `sharding/gossip_config_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=7, M=4, L=0` |
| `sharding/metrics_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `sharding/operational_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=3, L=0` |
| `sharding/wal_applier.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=1, M=2, L=0` |
| `sharding/paxos_state_persistence.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `sharding/raid_paxos_consensus.cpp` | 9 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0` |
| `sharding/health_check.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=4, M=8, L=0` |
| `sharding/orphan_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `sharding/raft_consensus.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=4, M=4, L=0` |
| `sharding/epoch_fencing.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=15, M=6, L=0` |
| `sharding/two_phase_commit_participant.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=11, M=1, L=0` |
| `sharding/raft_configuration.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `sharding/gossip_protocol.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=2, M=12, L=0` |
| `sharding/shard_resource_manager.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=2, H=4, M=4, L=0` |
| `sharding/cloud_agent.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=6, M=3, L=0` |
| `sharding/wal_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=7, L=0` |
| `sharding/shard_repair_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=6, L=0` |
| `sharding/distributed_time_coordinator.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `sharding/pki_shard_certificate.cpp` | 7 | `* @note Gap Summary: total=4; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=8, M=6, L=0` |
| `sharding/paxos_consensus.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=10, M=6, L=0` |
| `sharding/health_monitor.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=0, H=7, M=3, L=0` |
| `sharding/capability_matcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=4, L=0` |
| `sharding/auto_rebalancer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=4, M=7, L=0` |
| `sharding/sharding_manager_edition.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `sharding/circuit_breaker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `sharding/raft_wal_integration.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=0, L=0` |
| `sharding/stream_protocol.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=2, Debt=1, C=6, H=15, M=10, L=0` |
| `sharding/raft_state.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0` |
| `sharding/prometheus_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=10, H=3, M=6, L=0` |
| `sharding/urn_resolver.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=3, L=0` |
| `sharding/metadata_snapshot.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `sharding/paxos_wal.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `sharding/secure_transport_client.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `sharding/admin_operations.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `sharding/replication_coordinator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0` |
| `sharding/shard_rpc_server.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=2, L=0` |
| `sharding/continuous_batch_scheduler.cpp` | 9 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0` |
| `sharding/shard_durability.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `sharding/shard_rpc_client.cpp` | 7 | `* @note Gap Summary: total=34; TODO=1, Stub=14, Unimpl=1, Mock=1, Sim=11, Debt=6, C=0, H=11, M=15, L=0` |
| `sharding/cloud_backup.cpp` | 7 | `* @note Gap Summary: total=90; TODO=1, Stub=71, Unimpl=0, Mock=1, Sim=17, Debt=0, C=0, H=14, M=2, L=0` |
| `sharding/shard_load_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=23, L=0` |
| `sharding/replica_topology.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `sharding/predictive_detector.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=7, M=6, L=0` |
| `sharding/redundancy_strategy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=13, H=21, M=62, L=0` |
| `sharding/quorum_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=5, L=0` |
| `sharding/raft_shard_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=4, L=0` |
| `sharding/mtls_client.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=0, L=0` |
| `sharding/two_phase_commit_coordinator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=19, M=1, L=0` |
| `sharding/cross_shard_transaction.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=29, M=40, L=0` |
| `sharding/gpu_erasure_coder_opencl.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=8, M=6, L=1` |
| `sharding/truetime.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=4, L=0` |
| `sharding/replica_consistency.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=11, M=4, L=0` |
| `sharding/dual_consensus_orchestrator.cpp` | 9 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0` |
| `sharding/metadata_wal.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=0, L=0` |
| `sharding/gossip_consensus_adapter.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=5, M=3, L=0` |
| `sharding/adaptive_shard_router.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=10, M=3, L=0` |
| `sharding/metadata_shard.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=6, M=2, L=0` |
| `sharding/transaction_snapshot.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `sharding/wal_shipper.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=4, M=3, L=0` |
| `sharding/signed_request.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=16, M=0, L=0` |
| `stable_diffusion/sd_config.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `stable_diffusion/tests/test_sd_plugin_registrar.cpp` | 7 | `* @note Gap Summary: total=13; TODO=1, Stub=11, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `stable_diffusion/tests/test_sd_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `stable_diffusion/sd_prompt_sanitizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `stable_diffusion/sd_plugin_registrar.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `stable_diffusion/sd_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=4, L=0` |
| `cdc/cross_collection_stream.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `cdc/dead_letter_queue.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=1, L=0` |
| `cdc/cdc_materialized_view.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `cdc/cdc_admin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=15, M=2, L=0` |
| `cdc/ws_transport.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0` |
| `cdc/tenant_buffer_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=7, M=3, L=0` |
| `cdc/kafka_cdc_producer.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=0, L=0` |
| `cdc/outbox.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `cdc/cdc_ws_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `cdc/delivery_tracker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `cdc/changefeed.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=16, M=2, L=0` |
| `cdc/changefeed_buffer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=0, L=0` |
| `ingestion/oauth_token_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0` |
| `ingestion/s3_connector.cpp` | 7 | `* @note Gap Summary: total=11; TODO=1, Stub=4, Unimpl=0, Mock=3, Sim=3, Debt=0, C=1, H=0, M=5, L=0` |
| `ingestion/database_connector.cpp` | 7 | `* @note Gap Summary: total=12; TODO=1, Stub=3, Unimpl=0, Mock=6, Sim=2, Debt=0, C=3, H=2, M=14, L=0` |
| `ingestion/filesystem_ingester.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=7, L=0` |
| `ingestion/llm_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `ingestion/api_connector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0` |
| `ingestion/web_crawler_connector.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=3, Sim=0, Debt=0, C=2, H=2, M=5, L=0` |
| `ingestion/agentic_reference_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0` |
| `ingestion/ingestion_coordinator.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=2, H=3, M=10, L=0` |
| `ingestion/kafka_connector.cpp` | 7 | `* @note Gap Summary: total=12; TODO=1, Stub=3, Unimpl=0, Mock=6, Sim=2, Debt=0, C=0, H=0, M=4, L=0` |
| `ingestion/ingestion_sinks.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=7, M=6, L=0` |
| `ingestion/semantic_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `ingestion/cdc_connector.cpp` | 7 | `* @note Gap Summary: total=11; TODO=1, Stub=3, Unimpl=0, Mock=5, Sim=2, Debt=0, C=2, H=0, M=13, L=0` |
| `ingestion/ingestion_quality_judge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `ingestion/object_storage_connector.cpp` | 7 | `* @note Gap Summary: total=11; TODO=1, Stub=3, Unimpl=0, Mock=5, Sim=2, Debt=0, C=0, H=4, M=4, L=0` |
| `ingestion/entity_assembler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=14, L=0` |
| `ingestion/steps/tensor_core_bridge_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=0, L=0` |
| `ingestion/steps/legal_metadata_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `ingestion/steps/chunk_text_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `ingestion/steps/deontic_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ingestion/steps/llm_extract_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0` |
| `ingestion/steps/decompress_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0` |
| `ingestion/steps/legal_reference_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `ingestion/steps/ner_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `ingestion/steps/chunk_embed_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `ingestion/steps/base_entity_assembler_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=2, L=0` |
| `ingestion/steps/parse_text_step.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `ingestion/steps/format_parse_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `ingestion/steps/chunk_tt_decompose_step.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `ingestion/legal_domain.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=28, L=0` |
| `ingestion/workflow_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=17, L=0` |
| `ingestion/huggingface_connector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=2, L=0` |
| `ingestion/deontic_extractor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0` |
| `ingestion/ingestion_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=3, M=36, L=0` |
| `metadata/schema_audit_log.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `metadata/column_lineage.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=8, L=0` |
| `metadata/schema_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=7, M=24, L=0` |
| `metadata/information_schema.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=8, M=24, L=0` |
| `metadata/distributed_catalog.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=1` |
| `metadata/er_diagram_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=11, L=0` |
| `metadata/catalog_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=3, L=0` |
| `metadata/schema_constraints.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=13, L=0` |
| `metadata/index_recommender.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=14, L=0` |
| `metadata/statistics_collector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=13, L=0` |
| `metadata/schema_consistency_checker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `metadata/schema_version_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=10, L=0` |
| `failover/disaster_recovery_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=0, L=0` |
| `failover/auto_failover_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=11, M=3, L=0` |
| `timeseries/gap_fill.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `timeseries/ts_encrypted_key_rotation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `timeseries/aggregates.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `timeseries/continuous_agg.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=4, L=0` |
| `timeseries/gorilla_simd.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `timeseries/downsampling.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `timeseries/timeseries_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `timeseries/retention.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `timeseries/ts_auto_buffer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=0, L=0` |
| `timeseries/ts_auto_buffer_adaptive.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `timeseries/ts_stream_cursor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=0, L=0` |
| `timeseries/timeseries.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0` |
| `timeseries/compression_selector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `timeseries/gorilla.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `timeseries/aggregate_scheduler_helper.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `timeseries/hypertable.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `timeseries/anomaly_detection.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=5, L=0` |
| `timeseries/aggregate_scheduler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `temporal/snapshot_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=7, M=10, L=0` |
| `temporal/temporal_cold_store.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=15, L=0` |
| `temporal/temporal_compressor.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=7, L=0` |
| `temporal/bitemporal_join.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `temporal/temporal_cdc.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=8, L=0` |
| `temporal/temporal_tier_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=10, L=0` |
| `temporal/retention_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=8, M=20, L=0` |
| `temporal/temporal_conflict_resolver.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0` |
| `temporal/temporal_migrator.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=13, L=0` |
| `temporal/temporal_query_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=20, L=0` |
| `temporal/system_versioned_table.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `temporal/bi_temporal.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=11, L=0` |
| `temporal/temporal_aggregator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=18, L=0` |
| `temporal/temporal_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `temporal/interval_tree_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `graph/distributed_graph.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=7, M=6, L=0` |
| `graph/rotate_completion.cpp` | 15 | `* @note Gap Summary: total=3; TODO=0, Stub=0, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `graph/scheduled_edge_refresh.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=11, L=0` |
| `graph/graph_query_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=11, M=46, L=0` |
| `graph/explain_plan.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `graph/ontology_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0` |
| `graph/parallel_traversal.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=20, L=0` |
| `graph/path_constraints.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0` |
| `graph/gpu_traversal.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=13, L=0` |
| `graph/graph_query_rewriter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=4, L=0` |
| `graph/tensor_deduplication_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=6, M=11, L=0` |
| `graph/graph_error_taxonomy.cpp` | 78 | `{0x07010002, "Generic: feature not implemented (stub/TODO code path)"},` |
| `graph/tensor_fingerprint_graph.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=20, L=0` |
| `graph/knowledge_graph_reasoner.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=8, L=0` |
| `graph/graph_watermark.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `plugins/plugin_hot_plug_monitor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=3, L=0` |
| `plugins/signed_plugin_repository.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0` |
| `plugins/wasm_plugin_loader.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=1, L=0` |
| `plugins/rpc_service_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `plugins/plugin_health_monitor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `plugins/plugin_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `plugins/plugin_system_edition.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `plugins/oci_registry_client.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `plugins/plugin_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `plugins/plugin_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=6, M=25, L=0` |
| `plugins/plugin_manager.cpp` | 2188 | `// TODO(makr-code): Fix capability comparison - PluginCapabilities is a struct with bool fields, not a container` |
| `search/negative_keyword_filter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0` |
| `search/conversational_search.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `search/llm_query_rewriter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=4, L=0` |
| `search/fuzzy_matcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `search/federated_search.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `search/personalized_ranker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `search/multi_field_search.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `search/autocomplete.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0` |
| `search/learning_to_rank.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `search/search_analytics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `search/cross_lingual_search.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=3, L=0` |
| `search/query_expander.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=11, L=0` |
| `search/llm_reranker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=4, L=0` |
| `search/faceted_search.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=8, L=0` |
| `search/search_highlighter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=9, L=0` |
| `search/hybrid_search.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `search/neural_sparse_retrieval.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `search/search_result_stream.cpp` | 7 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=0, H=0, M=0, L=0` |
| `search/multi_modal_search.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=6, L=0` |
| `scheduler/distributed_task_coordinator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `scheduler/event_trigger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=8, L=0` |
| `scheduler/task_result_store.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `scheduler/hybrid_retention_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `scheduler/external_scheduler_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0` |
| `scheduler/task_anomaly_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `scheduler/task_scheduler.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=1, C=4, H=16, M=14, L=0` |
| `scheduler/task_audit_event.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `scheduler/task_audit_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0` |
| `observability/log_search_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `observability/storage_profiler.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0` |
| `observability/tracer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0` |
| `observability/metrics_stream_server.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `observability/alertmanager.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=1, M=6, L=0` |
| `observability/performance_analyzer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=22, L=0` |
| `observability/opentelemetry_tracer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `observability/metric_anomaly_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `observability/ebpf_tracer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `observability/log_aggregator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=7, M=5, L=7` |
| `observability/continuous_profiler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=16, L=0` |
| `observability/query_profiler.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=10, L=0` |
| `observability/tenant_metrics_namespace.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `observability/ml_anomaly_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=9, L=0` |
| `observability/slo_reporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `observability/alerting_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0` |
| `observability/distributed_trace_span.cpp` | 208 | `// TODO: Send span to OTel backend (async)` |
| `observability/root_cause_analyzer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=15, L=0` |
| `observability/distributed_flame_graph.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=13, M=6, L=0` |
| `observability/advanced_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `observability/metric_aggregator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=8, M=18, L=0` |
| `observability/metrics_collector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `observability/tracer_utils.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `version.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `cache/redis_cache_coordinator.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=8, M=6, L=0` |
| `cache/distributed_cache_coordinator.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=2, Debt=0, C=2, H=7, M=8, L=0` |
| `cache/cache_replication.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `cache/semantic_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0` |
| `cache/warmup.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `cache/adaptive_query_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=36, M=3, L=0` |
| `cache/cache_hit_rate_slo_monitor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0` |
| `cache/embedding_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `cache/predictive_prefetcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0` |
| `cache/bounded_lru_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=11, M=0, L=0` |
| `cache/cache_replication_coordinator.cpp` | 7 | `* @note Gap Summary: total=2; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, blocking_no_timeout=0(fixed), C=1, H=5, M` |
| `cache/grpc_remote_cache_peer.cpp` | 10 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `auth/jwt_key_rotation_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `auth/ldap_authenticator.cpp` | 7 | `* @note Gap Summary: total=16; TODO=1, Stub=12, Unimpl=0, Mock=1, Sim=2, Debt=0, C=2, H=8, M=18, L=0` |
| `auth/auth_event_bus.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `auth/totp_replay_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `auth/auth_worker_thread_pool.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `auth/kerberos_security.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `auth/password_policy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=3, L=0` |
| `auth/auth_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `auth/auth_error.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=0, L=0` |
| `auth/principal_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `auth/jwks_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `auth/token_blacklist.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `auth/session_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `auth/rate_limiter_backend.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=5, M=6, L=0` |
| `auth/totp_secret_encryption.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0` |
| `auth/rocksdb_token_blacklist.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `auth/redis_token_blacklist.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=0, L=0` |
| `auth/federated_identity_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=9, M=4, L=0` |
| `auth/saml_authenticator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=9, L=0` |
| `auth/jwks_security.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=4, L=0` |
| `auth/auth_audit_logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `auth/oauth_pkce_flow.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=11, M=6, L=0` |
| `auth/mfa_authenticator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0` |
| `auth/mtls_authenticator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=10, M=9, L=0` |
| `auth/secure_memory.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `auth/api_key_authenticator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=9, M=0, L=0` |
| `auth/webauthn_authenticator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=7, L=0` |
| `auth/oauth_device_flow.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=6, M=6, L=0` |
| `auth/ldap_connection_pool.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0` |
| `auth/auth_rate_limiter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `auth/jwt_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=4, M=4, L=0` |
| `auth/gssapi_authenticator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `auth/zero_trust_auth_verifier.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0` |
| `auth/passkey_authenticator.cpp` | 7 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0` |
| `auth/oidc_provider.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0` |
| `chimera/qdrant_adapter.cpp` | 64 | `// TODO: Actual gRPC client connection` |
| `chimera/qdrant_adapter.cpp` | 134 | `// TODO: Insert vector into Qdrant collection` |
| `chimera/qdrant_adapter.cpp` | 173 | `// TODO: Execute KNN search against Qdrant` |
| `chimera/qdrant_adapter.cpp` | 190 | `// TODO: Create vector index with specified distance metric` |
| `chimera/qdrant_adapter.cpp` | 373 | `info.database_version = "1.0.0";  // TODO: Query actual server version` |
| `chimera/qdrant_adapter.cpp` | 493 | `// TODO: Mask API keys if present` |
| `chimera/neo4j_adapter.cpp` | 64 | `// TODO: Actual neo4j::Driver creation` |
| `chimera/neo4j_adapter.cpp` | 182 | `// TODO: Execute CREATE (node:Label {properties}) via Cypher` |
| `chimera/neo4j_adapter.cpp` | 195 | `// TODO: Execute CREATE RELATIONSHIP (from)-[rel:TYPE]->(to)` |
| `chimera/neo4j_adapter.cpp` | 212 | `// TODO: Execute Cypher shortest path query` |
| `chimera/neo4j_adapter.cpp` | 229 | `// TODO: Execute graph traversal query` |
| `chimera/neo4j_adapter.cpp` | 245 | `// TODO: Execute arbitrary Cypher query` |
| `chimera/neo4j_adapter.cpp` | 265 | `// TODO: Create node with collection label and document properties` |
| `chimera/neo4j_adapter.cpp` | 281 | `// TODO: Batch create nodes` |
| `chimera/neo4j_adapter.cpp` | 297 | `// TODO: Query nodes with label matching filter` |
| `chimera/neo4j_adapter.cpp` | 314 | `// TODO: Update node properties` |
| `chimera/neo4j_adapter.cpp` | 351 | `// TODO: Commit transaction via Neo4j session` |
| `chimera/neo4j_adapter.cpp` | 366 | `// TODO: Rollback transaction via Neo4j session` |
| `chimera/neo4j_adapter.cpp` | 424 | `info.database_version = "5.0.0";  // TODO: Query actual server version` |
| `chimera/neo4j_adapter.cpp` | 473 | `// TODO: Mask password in connection string` |
| `chimera/neo4j_adapter.cpp` | 478 | `// TODO: Convert Scalar to Cypher literal syntax` |
| `chimera/mongodb_adapter.cpp` | 67 | `// TODO: Actual mongocxx client creation` |
| `chimera/mongodb_adapter.cpp` | 101 | `// TODO: Translate AQL to MongoDB query and execute` |
| `chimera/mongodb_adapter.cpp` | 117 | `// TODO: Convert RelationalRow to BSON and insert into collection` |
| `chimera/mongodb_adapter.cpp` | 132 | `// TODO: Batch insert documents into collection` |
| `chimera/mongodb_adapter.cpp` | 197 | `// TODO: Store node as document` |
| `chimera/mongodb_adapter.cpp` | 202 | `// TODO: Store edge as document with references to nodes` |
| `chimera/mongodb_adapter.cpp` | 253 | `// TODO: Insert document into collection` |
| `chimera/mongodb_adapter.cpp` | 269 | `// TODO: Batch insert documents` |
| `chimera/mongodb_adapter.cpp` | 285 | `// TODO: Query documents with filter` |
| `chimera/mongodb_adapter.cpp` | 302 | `// TODO: Update documents matching filter` |
| `chimera/mongodb_adapter.cpp` | 389 | `info.database_version = "5.0.0";  // TODO: Query actual server version` |
| `chimera/mongodb_adapter.cpp` | 395 | `metrics.total_queries = 0;  // TODO: Track actual statistics` |
| `chimera/mongodb_adapter.cpp` | 512 | `// TODO: Implement rollback logic` |
| `chimera/mongodb_adapter.cpp` | 647 | `// TODO: Mask password and API key in connection string` |
| `chimera/mongodb_adapter.cpp` | 652 | `// TODO: Serialize Scalar to BSON` |
| `chimera/mongodb_adapter.cpp` | 657 | `// TODO: Serialize RelationalRow to BSON document` |
| `chimera/mongodb_adapter.cpp` | 664 | `// TODO: Translate AQL to MongoDB aggregation pipeline` |
| `chimera/themisdb_adapter.cpp` | 7 | `* @note Gap Summary: total=19; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=11, Debt=0, C=6, H=18, M=28, L=0` |
| `distributed_knowledge/cross_shard_feedback_sync.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `distributed_knowledge/federated_distillation_coordinator.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=3, M=0, L=0` |
| `distributed_knowledge/federated_rag_merger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=21, M=4, L=0` |
| `distributed_knowledge/lora_federation_coordinator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0` |
| `distributed_knowledge/adapter_capability_announcement.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `document/round_trip_editor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `themis/license_info.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=3, L=0` |
| `themis/module_loader_linux.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `themis/module_hash_verifier.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `themis/module_security.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `themis/wire_protocol_server.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=9, M=19, L=0` |
| `themis/edition_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `themis/module_signature_verifier.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `themis/build_info.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=28, L=0` |
| `themis/module_loader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=8, L=0` |
| `themis/module_loader_win32.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `themis/module_dependency_resolver.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=11, L=0` |
| `onnx_clip/onnx_clip_plugin.h` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `onnx_clip/onnx_clip_plugin.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=4, Debt=0, C=2, H=1, M=5, L=0` |
| `rpc_grpc/bidi_stream_adapter.h` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `rpc_grpc/grpc_plugin.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `rpc_grpc/grpc_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=8, L=0` |
| `prompt_engineering/prompt_performance_tracker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=4, L=0` |
| `prompt_engineering/protegi_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `prompt_engineering/prompt_regression_runner.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `prompt_engineering/dspy_module.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=14, M=4, L=0` |
| `prompt_engineering/prompt_engineering_integration.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `prompt_engineering/llm_reflection_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `prompt_engineering/prompt_library_io.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=11, L=0` |
| `prompt_engineering/prompt_ab_experiment.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `prompt_engineering/tree_of_thoughts.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=17, L=0` |
| `prompt_engineering/prompt_engineering_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `prompt_engineering/system_prompt_manager.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `prompt_engineering/reflection_tuner.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=7, L=0` |
| `prompt_engineering/rag_context_budget_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `prompt_engineering/structured_output.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0` |
| `prompt_engineering/cot_tracer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `prompt_engineering/prompt_version_control.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=18, L=0` |
| `prompt_engineering/prompt_compressor.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0` |
| `prompt_engineering/meta_prompt_generator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0` |
| `prompt_engineering/self_improvement_orchestrator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=7, M=4, L=0` |
| `prompt_engineering/prompt_manager.cpp` | 7 | `* @note Gap Summary: total=9; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0` |
| `prompt_engineering/rag_prompt_builder.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `prompt_engineering/adversarial_prompt_tester.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=1, L=0` |
| `prompt_engineering/chain_of_thought.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `prompt_engineering/prompt_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=2, L=1` |
| `prompt_engineering/markdown_utils.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0` |
| `prompt_engineering/prompt_injection_detector.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=3, M=10, L=0` |
| `prompt_engineering/prompt_template_compiler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=30, M=9, L=0` |
| `prompt_engineering/prompt_template_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `prompt_engineering/context_window_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=6, L=0` |
| `prompt_engineering/prompt_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `prompt_engineering/feedback_collector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=12, L=0` |
| `prompt_engineering/prompt_quality_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `process/cmmn_serializer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=22, L=0` |
| `process/dmn_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=7, L=0` |
| `process/process_community_detector.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0` |
| `process/process_light_retriever.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `process/process_agentic_rag.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=9, L=0` |
| `process/ocel_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=15, L=0` |
| `process/epk_aris_xml_importer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0` |
| `process/process_linker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `process/vcc_vpb_importer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=13, L=0` |
| `process/object_centric_tracer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=8, L=0` |
| `process/process_model_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0` |
| `process/process_model_generator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=15, L=0` |
| `process/bpmn_serializer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=30, L=0` |
| `process/fim_importer.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=11, L=0` |
| `process/llm_process_descriptor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `process/process_graph_rag.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=35, L=0` |
| `process/epk_serializer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `content/content_policy.cpp` | 7 | `* @note Gap Summary: total=15; TODO=2, Stub=2, Unimpl=2, Mock=0, Sim=1, Debt=3, C=2, H=4, M=7, L=0` |
| `content/video_processor.cpp` | 7 | `* @note Gap Summary: total=9; TODO=1, Stub=0, Unimpl=1, Mock=0, Sim=0, Debt=2, C=1, H=2, M=5, L=0` |
| `content/pdf_processor.cpp` | 7 | `* @note Gap Summary: total=5; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=1, M=4, L=0` |
| `content/language_detector.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=1, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=2, M=7, L=0` |
| `content/office_processor.cpp` | 7 | `* @note Gap Summary: total=8; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=3, C=1, H=2, M=5, L=0` |
| `content/content_security.cpp` | 7 | `* @note Gap Summary: total=5; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=1, M=4, L=0` |
| `content/image_processor.cpp` | 7 | `* @note Gap Summary: total=6; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=2, M=4, L=0` |
| `content/content_manager.cpp` | 7 | `* @note Gap Summary: total=8; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=4, C=1, H=2, M=5, L=0` |
| `content/content_fs.cpp` | 7 | `* @note Gap Summary: total=5; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=1, M=4, L=0` |
| `content/geo_processor.cpp` | 7 | `* @note Gap Summary: total=7; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=3, C=1, H=2, M=4, L=0` |
| `content/html_processor.cpp` | 7 | `* @note Gap Summary: total=11; TODO=1, Stub=1, Unimpl=1, Mock=0, Sim=0, Debt=2, C=1, H=3, M=6, L=0` |
| `content/mime_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=1, C=0, H=1, M=2, L=0` |
| `content/audio_processor.cpp` | 7 | `* @note Gap Summary: total=6; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=3, C=0, H=3, M=3, L=0` |
| `content/tts_processor.cpp` | 7 | `* @note Gap Summary: total=14; TODO=1, Stub=1, Unimpl=1, Mock=0, Sim=1, Debt=3, C=1, H=3, M=8, L=0` |
| `content/content_manager_embedding.cpp` | 7 | `* @note Gap Summary: total=12; TODO=2, Stub=1, Unimpl=1, Mock=0, Sim=0, Debt=2, C=1, H=3, M=6, L=0` |
| `content/stt_processor.cpp` | 7 | `* @note Gap Summary: total=7; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=3, C=1, H=1, M=5, L=0` |
| `content/cad_processor.cpp` | 7 | `* @note Gap Summary: total=5; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=1, M=4, L=0` |
| `content/content_validator.cpp` | 7 | `* @note Gap Summary: total=6; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=1, H=1, M=4, L=0` |
| `content/content_type.cpp` | 7 | `* @note Gap Summary: total=4; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=1, C=0, H=1, M=3, L=0` |
| `content/version_manager.cpp` | 7 | `* @note Gap Summary: total=4; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=1, C=0, H=1, M=3, L=0` |
| `content/abuse_detector.cpp` | 7 | `* @note Gap Summary: total=2; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=0, M=2, L=0` |
| `content/content_metrics.cpp` | 7 | `* @note Gap Summary: total=7; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=3, C=0, H=1, M=6, L=0` |
| `content/content_logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=1, C=0, H=1, M=2, L=0` |
| `content/content_manager_llm.cpp` | 7 | `* @note Gap Summary: total=14; TODO=3, Stub=1, Unimpl=2, Mock=0, Sim=0, Debt=2, C=1, H=4, M=7, L=0` |
| `content/content_errors.cpp` | 7 | `* @note Gap Summary: total=2; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=1, C=0, H=0, M=2, L=0` |
| `content/embedding_pipeline.cpp` | 7 | `* @note Gap Summary: total=13; TODO=2, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=2, C=1, H=3, M=7, L=0` |
| `content/adapters/audio_extractor_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `content/adapters/format_extractor_factory.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `content/adapters/text_extractor_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `content/deduplication_checker.cpp` | 7 | `* @note Gap Summary: total=8; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=4, C=1, H=2, M=5, L=0` |
| `content/pipeline/async_bulk_uploader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `content/pipeline/content_chunker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `content/pipeline/zstd_compression.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `content/pipeline/multimodal_chunker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `content/pipeline/bulk_upload_interface.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `content/mock_clip_processor.cpp` | 7 | `* @note Gap Summary: total=28; TODO=5, Stub=8, Unimpl=4, Mock=3, Sim=2, Debt=2, C=3, H=8, M=12, L=2` |
| `content/ocr_processor.cpp` | 7 | `* @note Gap Summary: total=18; TODO=2, Stub=2, Unimpl=2, Mock=1, Sim=1, Debt=3, C=2, H=5, M=8, L=0` |
| `content/ingestion_plugin.cpp` | 7 | `* @note Gap Summary: total=16; TODO=3, Stub=2, Unimpl=2, Mock=1, Sim=0, Debt=2, C=2, H=4, M=8, L=0` |
| `content/text_processor.cpp` | 7 | `* @note Gap Summary: total=7; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=3, C=0, H=2, M=5, L=0` |
| `content/async_ingestion_worker.cpp` | 7 | `* @note Gap Summary: total=8; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=4, C=1, H=3, M=4, L=0` |
| `content/markdown_processor.cpp` | 7 | `* @note Gap Summary: total=6; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=1, M=5, L=0` |
| `content/archive_processor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=1, C=0, H=1, M=2, L=0` |
| `geo/raster_query_interface.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `geo/temporal_spatial_query.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `geo/gpu_backend_stub.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=3, L=0` |
| `geo/gpu_kernel_dispatcher_cpu.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=2, L=0` |
| `geo/gpu_backend_production.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=14, H=30, M=6, L=0` |
| `geo/cpu_backend.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=21, L=0` |
| `geo/geo_clustering.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0` |
| `geo/geo_faiss_knn.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `geo/tile_server.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `geo/raster.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `geo/spatial_join_filter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `geo/rtree_cursor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `geo/geo_json_geometry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=16, L=0` |
| `geo/temporal_spatial_query_builder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `geo/spatial_join.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `geo/phase2_phase3_hardening.cpp` | 7 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `geo/device_detector.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=1, L=0` |
| `geo/geo_backend_dispatch.cpp` | 265 | `// TODO: Integrate Vincenty CUDA kernel with batch dispatch` |
| `geo/geo_rtree.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=9, L=0` |
| `geo/boost_cpu_exact_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0` |
| `geo/geo_phase2_phase3_integration.cpp` | 7 | `* @note Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `geo/gpu_backend_hip.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=1, M=0, L=0` |
| `main_server.cpp` | 7 | `* @note Gap Summary: total=21; TODO=1, Stub=18, Unimpl=0, Mock=1, Sim=1, Debt=0, C=101, H=1578, M=181, L=0` |
| `ethics_ai/discourse_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `ethics_ai/chain_visualizer.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/ethics_profile_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `ethics_ai/llm_cascade_router.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/ethics_aql_queries.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/discourse_engine.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/rag_context_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=6, L=0` |
| `ethics_ai/cross_school_tension_resolver.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `ethics_ai/rag_context_engine.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/ethics_ai_types.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/argument_store.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `ethics_ai/chain_visualizer.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `ethics_ai/ethics_ai_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `ethics_ai/ethics_evaluator.h` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=1, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/convergence_marker_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `ethics_ai/position_abstract_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `ethics_ai/ethics_selection_router.cpp` | 7 | `* @note Gap Summary: total=9; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=2, Debt=0, C=1, H=5, M=16, L=0` |
| `ethics_ai/ethics_profile_registry.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/philosophy_loader.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/argument_store.h` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/ethics_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `ethics_ai/philosophy_loader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=13, L=0` |
| `ethics_ai/prior_round_compressor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=6, L=0` |
| `ethics_ai/tournament_mode_selector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `ethics_ai/synthesis_matrix_builder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=2, L=0` |
| `ethics_ai/discourse_memory_store.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ethics_ai/ethics_base_entity_adapter.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `index/hnsw_parameter_tuner.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `index/graph_auto_buffer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=1, L=0` |
| `index/rotary_embeddings_gpu_cpu.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `index/gpu_vector_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=16, M=15, L=0` |
| `index/rotary_embeddings_hip.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=0, L=0` |
| `index/multi_vector_search.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=9, L=0` |
| `index/hnsw_production_defaults.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `index/matryoshka_truncation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `index/graph_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=4, M=21, L=0` |
| `index/approximate_radius_search.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `index/cuda_hnsw_graph_traversal.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=9, H=22, M=6, L=0` |
| `index/hnsw_layer_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `index/binary_quantizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `index/property_graph.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `index/spatial_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=9, M=17, L=0` |
| `index/learned_quantizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `index/edge_types.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `index/index_compression.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=13, L=1` |
| `index/process_graph.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=0, M=44, L=0` |
| `index/gpu_memory_oversubscription.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0` |
| `index/secondary_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=29, H=17, M=96, L=1` |
| `index/gnn_embeddings.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=8, H=1, M=8, L=0` |
| `index/tiered_index_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `index/rotary_embeddings.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `index/workload_replay.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `index/distributed_vector_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=7, M=5, L=0` |
| `index/vector_auto_buffer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=1, C=1, H=1, M=9, L=0` |
| `index/learnable_rope.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `index/index_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `index/adaptive_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `index/vector_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=26, H=3, M=42, L=0` |
| `index/product_quantizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=4, L=0` |
| `index/lora_rope.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `index/multi_gpu_vector_index.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=5, L=0` |
| `index/inverted_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=20, L=1` |
| `index/gpu_vector_index_vulkan.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=2, H=5, M=10, L=0` |
| `index/residual_quantizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `index/temporal_graph.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `index/advanced_vector_index.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=5, H=4, M=11, L=0` |
| `index/graph_analytics.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=6, L=0` |
| `index/ann_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=14, L=0` |
| `projects/project_diff.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=8, L=0` |
| `projects/collaboration_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `projects/project_versioning.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `projects/project_lifecycle.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `projects/project_template.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `projects/project_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `projects/in_memory_project_audit_log.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `governance/policy_change_manager.cpp` | 647 | `// TODO: Implement actual rollback operation with policy manager` |
| `governance/policy_version_history.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=4, L=0` |
| `governance/iso27001_rules.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=14, L=0` |
| `governance/ccpa_rules.cpp` | 10 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `governance/policy_template.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `governance/policy_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=14, L=0` |
| `governance/audit_batch_writer.cpp` | 454 | `// TODO: Implement proper p95/p99 tracking with histogram` |
| `governance/cross_border_transfer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `governance/compliance_reporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=54, L=0` |
| `governance/policy_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=13, L=0` |
| `governance/policy_file_watcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `governance/review_scheduler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `governance/model_governance.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `governance/data_lineage.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `governance/compliance_reporting.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=62, L=0` |
| `governance/cross_tenant_policy_inheritance.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0` |
| `governance/data_masker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `governance/policy_engine.cpp` | 7 | `* @note Gap Summary: total=14; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=7, Debt=0, C=0, H=0, M=3, L=0` |
| `governance/policy_coordinator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `governance/soc2_controls.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=12, L=0` |
| `governance/opa_adapter.cpp` | 7 | `* @note Gap Summary: total=11; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=2, L=0` |
| `governance/policy_validation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=47, L=0` |
| `governance/pci_dss_rules.cpp` | 10 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `governance/policy_manager_versioned.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=4, L=0` |
| `governance/policy_review.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=1, H=0, M=23, L=0` |
| `governance/gdpr_subject_rights.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=6, L=0` |
| `governance/hipaa_rules.cpp` | 10 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `maintenance/maintenance_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `maintenance/database_maintenance_orchestrator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=5, L=0` |
| `utils/pii_pseudonymizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=0, L=0` |
| `utils/checksum_utils.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `utils/pii_detection_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0` |
| `utils/zstd_codec.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `utils/stemmer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `utils/hkdf_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=0, L=0` |
| `utils/simd_distance.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `utils/consistent_hash.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `utils/saga_logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `utils/sampled_logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=0, L=2` |
| `utils/pii_stream_scanner.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0` |
| `utils/stopwords.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `utils/update_checker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `utils/memory/pool_allocator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=30, M=0, L=0` |
| `utils/pki_client.cpp` | 10 | `* @note Gap Summary: total=8; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=2, Debt=1, C=0, H=6, M=32, L=0` |
| `utils/logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=2` |
| `utils/grpc_channel_pool.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=13, M=0, L=0` |
| `utils/capability_auto_generator.cpp` | 10 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=1` |
| `utils/cursor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `utils/geo/ewkb.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=39, L=0` |
| `utils/normalizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `utils/lz4_codec.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `utils/error_registry.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=6, M=26, L=1` |
| `utils/boost_throw_exception.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `utils/retention_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0` |
| `utils/audit_logger.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=12, L=0` |
| `utils/timestamp_utils.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `utils/regex_detection_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0` |
| `utils/cron_parser.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `utils/self_awareness.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `utils/tracing.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=5, L=0` |
| `utils/http_client_pool.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=13, M=1, L=0` |
| `utils/serialization.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0` |
| `utils/compression_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `utils/ner_detection_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `utils/rate_limiter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=1, L=0` |
| `utils/utils_adapters.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0` |
| `utils/build_info.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=26, L=0` |
| `utils/thread_pool_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=4, M=1, L=0` |
| `utils/input_validator.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=11, L=0` |
| `utils/pii_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0` |
| `utils/runtime_license_gate.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `utils/bloom_filter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `utils/file_utils.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `acceleration/shader_integrity.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `acceleration/faiss_gpu_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=25, H=41, M=8, L=0` |
| `acceleration/cpu_backend_mt.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=1, L=0` |
| `acceleration/rccl_vector_backend.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=5, Debt=0, C=0, H=2, M=4, L=0` |
| `acceleration/vec_knn.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `acceleration/plugin_security.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=25, L=0` |
| `acceleration/graphics_backends.cpp` | 10 | `* @note Gap Summary: total=34; TODO=1, Stub=28, Unimpl=0, Mock=1, Sim=3, Debt=1, C=0, H=28, M=13, L=4` |
| `acceleration/cpu_backend.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `acceleration/vulkan_backend_full.cpp` | 7 | `* @note Gap Summary: total=12; TODO=1, Stub=9, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=3, M=0, L=0` |
| `acceleration/hip_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=12, M=2, L=0` |
| `acceleration/zluda_backend.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=6, M=1, L=0` |
| `acceleration/directx_backend_full.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=3, L=0` |
| `acceleration/nccl_vector_backend.cpp` | 7 | `* @note Gap Summary: total=13; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=6, Debt=0, C=0, H=2, M=4, L=0` |
| `acceleration/vllm_resource_manager.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=2, H=3, M=2, L=0` |
| `acceleration/break_even_validator.cc` | 184 | `// Export metrics (TODO: integrate with Prometheus)` |
| `acceleration/break_even_validator.cc` | 194 | `// TODO: Delegate to CPU reference kernel implementations` |
| `acceleration/break_even_validator.cc` | 213 | `// TODO: Delegate to GPU kernel implementation` |
| `acceleration/tensor_core_matmul.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `acceleration/oneapi_backend.cpp` | 7 | `* @note Gap Summary: total=14; TODO=1, Stub=10, Unimpl=0, Mock=1, Sim=2, Debt=0, C=5, H=6, M=3, L=0` |
| `acceleration/backend_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0` |
| `acceleration/cpu_backend_tbb.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0` |
| `acceleration/device_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0` |
| `acceleration/plugin_loader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=4, L=0` |
| `acceleration/geo_acceleration_bridge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `acceleration/opencl_backend.cpp` | 7 | `* @note Gap Summary: total=9; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=7, M=1, L=0` |
| `acceleration/ai_hardware_dispatcher.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=28, H=49, M=2, L=0` |
| `acceleration/compute_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `acceleration/multi_gpu_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `acceleration/cuda_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=33, M=5, L=0` |
| `security/security_evidence_collector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=8, L=0` |
| `security/timestamp_authority_openssl.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=12, L=0` |
| `security/arrow_user_registration_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `security/timestamp_authority.cpp` | 7 | `* @note Gap Summary: total=45; TODO=1, Stub=35, Unimpl=0, Mock=1, Sim=8, Debt=0, C=0, H=2, M=24, L=0` |
| `security/prompt_injection_pattern_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `security/user_registration_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `security/binary_manifest.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `security/manifest_signer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `security/hsm_provider.cpp` | 7 | `* @note Gap Summary: total=58; TODO=1, Stub=47, Unimpl=0, Mock=1, Sim=7, Debt=2, C=0, H=1, M=7, L=0` |
| `security/ai_operation_guard.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `security/hsm_provider_pkcs11.cpp` | 7 | `* @note Gap Summary: total=23; TODO=1, Stub=20, Unimpl=0, Mock=1, Sim=1, Debt=0, C=2, H=10, M=7, L=0` |
| `security/behavioral_anomaly_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `security/access_control_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=1, M=0, L=0` |
| `security/field_encryption.cpp` | 10 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=8, H=12, M=17, L=0` |
| `security/query_masking_policy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `security/vault_signing_provider.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=5, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `security/usb_volume_hardening.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=2, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `security/intent_classifier.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `security/key_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `security/pii_redaction_policy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `security/hsm_signing.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `security/usb_admin_authenticator.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=5, Unimpl=1, Mock=1, Sim=2, Debt=0, C=0, H=0, M=3, L=0` |
| `security/fips_crypto_mode.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=0, L=0` |
| `security/access_control.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0` |
| `security/encrypted_field.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `security/secret_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0` |
| `security/row_level_security.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `security/vcc_pki_client.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=6, L=0` |
| `security/vram_secure_clear.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `security/confidential_computing.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=1, M=6, L=0` |
| `security/pki_key_provider.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=20, L=0` |
| `security/aql_injection_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=5, L=0` |
| `security/vault_key_provider.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=18, M=14, L=0` |
| `security/hsm_key_provider_adapter.cpp` | 7 | `* @note Gap Summary: total=26; TODO=1, Stub=22, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=11, M=5, L=0` |
| `security/ai_snapshot_cleanup.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `security/keyprovider_signing.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `security/rbac.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=11, L=0` |
| `security/tsa_api.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `security/embedded_user_registration_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=6, L=0` |
| `security/post_quantum_crypto.cpp` | 7 | `* @note Gap Summary: total=24; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=18, Debt=0, C=0, H=5, M=22, L=0` |
| `security/input_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=11, L=0` |
| `security/mock_key_provider.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `security/malware_scanner.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=6, M=9, L=0` |
| `security/cms_signing.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `security/webdav_user_registration_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `security/zero_trust_policy_enforcer.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=0, M=3, L=0` |
| `updates/update_history_logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `updates/preflight_health_check.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `updates/parallel_downloader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=3, M=1, L=0` |
| `updates/canary_rollout.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `updates/dependency_resolver.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=9, L=0` |
| `updates/hot_reload_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=3, M=6, L=0` |
| `updates/coordinated_update_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `updates/schema_migration_tester.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `updates/tenant_update_scheduler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `updates/updates_config.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0` |
| `updates/in_place_schema_migrator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=8, L=0` |
| `updates/notification_webhook.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `updates/release_manifest.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `updates/cluster_update_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `updates/delta_update_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=22, L=0` |
| `updates/update_state_machine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `updates/blue_green_deployment.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `updates/build_verifier.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `updates/schema_migration.cpp` | 7 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=0, H=0, M=0, L=0` |
| `updates/manifest_database.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=4, M=6, L=0` |
| `updates/hardware_telemetry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=5, M=0, L=0` |
| `user_storage_encrypted/key_rotation_scheduler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `user_storage_encrypted/gocryptfs_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=22, L=0` |
| `user_storage_encrypted/multi_level_storage.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=2, H=8, M=18, L=0` |
| `user_storage_encrypted/key_derivation_service.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=2, H=0, M=3, L=0` |
| `rag/learning_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `rag/llm_judge_client.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=19, M=4, L=0` |
| `rag/http_metrics_client.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=1, L=0` |
| `rag/streaming_retriever.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `rag/prompt_templates.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=1, L=0` |
| `rag/targ_retrieval.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=0, L=1` |
| `rag/citation_highlighter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `rag/hallucination_dashboard.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `rag/adaptive_retrieval.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `rag/examples/loop_orchestration_example.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=1, L=0` |
| `rag/distributed_rag_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=11, M=7, L=0` |
| `rag/bias_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `rag/replug_retriever.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `rag/document_splitter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `rag/knowledge_graph_retriever.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=14, L=0` |
| `rag/calibration_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=2, M=6, L=1` |
| `rag/quality_control_pipeline.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=4, L=0` |
| `rag/document_summarizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=7, M=9, L=0` |
| `rag/onnx_model_loader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0` |
| `rag/continuous_learning_client.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=5, L=0` |
| `rag/coherence_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `rag/hybrid_retriever.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0` |
| `rag/ontology_aware_retriever.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `rag/evaluation_report_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=39, L=0` |
| `rag/flare_retrieval.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=0, L=0` |
| `rag/judge_ensemble.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `rag/pairwise_comparator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0` |
| `rag/knowledge_gap_detector.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=22, M=22, L=1` |
| `rag/reranker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=3, M=4, L=0` |
| `rag/continuous_learning_orchestrator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=3, M=11, L=0` |
| `rag/bayesian_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `rag/llm_meta_analyzer.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=1, C=11, H=7, M=1, L=0` |
| `rag/quality_control_factory.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=0, L=0` |
| `rag/batch_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=41, H=56, M=5, L=0` |
| `rag/tensor_rag_pipeline.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `rag/faithfulness_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `rag/dpr_vectorizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=28, H=25, M=3, L=0` |
| `rag/explainability_reason_builder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0` |
| `rag/rag_context_assembler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=1, L=0` |
| `rag/nli_faithfulness_verifier.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `rag/multi_hop_reasoner.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=4, L=0` |
| `rag/lora_enhanced_retriever.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=1, L=0` |
| `rag/rag_judge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=56, H=63, M=12, L=0` |
| `rag/evaluation_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=0, L=0` |
| `rag/completeness_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `rag/self_rag.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `rag/multi_step_rag.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=7, M=10, L=0` |
| `rag/geval_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `rag/judge_config.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `rag/prompt_injection_detector.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=5, M=7, L=0` |
| `rag/multimodal_rag.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=6, L=0` |
| `rag/relevance_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `rag/claim_extractor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=4, L=0` |
| `rag/adversarial_tester.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=39, H=33, M=6, L=0` |
| `rag/ab_testing_framework.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `rag/rag_ingestion_bridge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=4, M=1, L=0` |
| `rag/rubric_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `rag/fairness_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=11, L=0` |
| `rag/agentic_rag.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=2, H=0, M=5, L=0` |
| `rag/rlaif_trainer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=5, L=0` |
| `rag/response_parser.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `rag/llm_integration.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=1, L=0` |
| `rag/cot_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=3, L=0` |
| `rag/llm_judge_integration.cpp` | 7 | `* @note Gap Summary: total=24; TODO=1, Stub=5, Unimpl=0, Mock=16, Sim=1, Debt=1, C=2, H=24, M=2, L=0` |
| `rag/delegate_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=4, L=0` |
| `core/concerns/adapter_signing.cpp` | 7 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0` |
| `core/concerns/concerns_context.cpp` | 7 | `* @note Gap Summary: total=13; TODO=1, Stub=11, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `core/concerns/lockfree_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=4, L=0` |
| `core/concerns/redis_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=19, M=7, L=0` |
| `core/concerns/zero_copy_logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=26, L=5` |
| `core/concerns/prometheus_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `core/concerns/context_propagation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `core/concerns/adapter_registry.cpp` | 7 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0` |
| `core/concerns/i_logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `core/security_initialization.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=5, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `core/index_interface_stubs.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `core/adapters/otel_tracer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `replication/multi_tier_replication.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `replication/schema_cdc.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `replication/event_stream.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=3, L=0` |
| `replication/conflict_resolution.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=14, H=28, M=7, L=0` |
| `replication/logical_replication.cpp` | 9 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=10, M=19, L=4` |
| `replication/replication_manager.cpp` | 11 | `* @note Gap Summary: total=13; TODO=2, Stub=6, Unimpl=0, Mock=1, Sim=4, Debt=0, C=108, H=91, M=171, L=0` |
| `replication/raft_v2.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `replication/replication_slot.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0` |
| `replication/policy.cpp` | 9 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `replication/observability.cpp` | 9 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0` |
| `api/otlp_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `api/graphql_ws_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `api/ws_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `api/themisdb_grpc_service.cpp` | 7 | `* @note Gap Summary: total=13; TODO=1, Stub=3, Unimpl=7, Mock=1, Sim=1, Debt=0, C=3, H=13, M=1, L=0` |
| `api/graphql.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=22, M=12, L=0` |
| `api/grpc_server.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0` |
| `api/geo_index_hooks.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=7, L=0` |
| `api/tracing_middleware.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `api/federation_admin_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `tensor/tensor_butterfly_operator.cpp` | 7 | `* @note Gap Summary: total=27; TODO=1, Stub=20, Unimpl=2, Mock=1, Sim=3, Debt=0, C=0, H=3, M=0, L=0` |
| `tensor/hnsw_tt_bridge.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=4, M=6, L=0` |
| `tensor/tensor_index.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `tensor/tensor_mmap_bridge.cpp` | 7 | `* @note Gap Summary: total=12; TODO=1, Stub=9, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=5, L=0` |
| `tensor/compression_strategy.cpp` | 40 | `// TODO: Wire to actual TensorTrainDecomposer` |
| `tensor/compression_strategy.cpp` | 306 | `// TODO: Implement registration mechanism` |
| `tensor/adapter_repository.cpp` | 7 | `* @note Gap Summary: total=19; TODO=1, Stub=13, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=6, M=3, L=0` |
| `tensor/ht_index.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `tensor/utr_converter.cpp` | 7 | `* @note Gap Summary: total=9; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=0, M=6, L=0` |
| `tensor/tensor_routing_strategy.cpp` | 79 | `float freshness = 1.0f;  // TODO: Parse created_at and compute age` |
| `tensor/tensor_routing_strategy.cpp` | 94 | `float freshness_a = 1.0f;  // TODO: Compute from timestamp` |
| `tensor/tensor_routing_strategy.cpp` | 283 | `// TODO: Implement adaptive learning with metrics tracking` |
| `tensor/tensor_ingestion_bridge.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `tensor/tnsr_task.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `tensor/tensor_fingerprint_graph.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `tensor/hiss_structural_search.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=4, M=4, L=0` |
| `tensor/hyper_index_builder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0` |
| `tensor/tensor_index_manager.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0` |
| `tensor/tensor_core_bridge.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=1, L=0` |
| `gpu/audit_log.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `gpu/gpu_memory_manager_edition.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=24, M=4, L=0` |
| `gpu/profiler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `gpu/rocm_backend.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=4, M=1, L=0` |
| `gpu/tensor_buffer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `gpu/admin_api.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=4, M=6, L=0` |
| `gpu/launcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `gpu/metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `gpu/mig_manager.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=4, L=0` |
| `gpu/alerts.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `gpu/feature_flags.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `gpu/cluster_topology.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `gpu/training_loop.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `gpu/kernel_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `gpu/config.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `gpu/wasm_kernel_sandbox.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=0, M=1, L=0` |
| `gpu/cluster_coordinator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `gpu/query_accelerator.cpp` | 7 | `* @note Gap Summary: total=9; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=6, Debt=0, C=5, H=17, M=14, L=0` |
| `gpu/policy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `gpu/time_slice_scheduler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=1, L=0` |
| `gpu/graph_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `gpu/device_discovery.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `gpu/load_balancer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `gpu/p2p_transfer.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=3, M=0, L=0` |
| `gpu/vulkan_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `gpu/gpu_module.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0` |
| `gpu/stream_manager.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=3, L=0` |
| `gpu/memory_pool.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `gpu/safe_fail.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `gpu/unified_memory.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=2, M=2, L=0` |
| `network/wire_protocol_server_ws.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=13, M=2, L=0` |
| `network/kernel_bypass.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=5, M=6, L=0` |
| `network/wire_protocol_helpers.cpp` | 7 | `* @note Gap Summary: total=11; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=8, Debt=0, C=0, H=1, M=2, L=0` |
| `network/wire_protocol_performance.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=4, L=0` |
| `network/socket_timeout_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=10, M=1, L=0` |
| `network/connection_compression.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `network/qos_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=3, L=0` |
| `network/io_uring_batcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `network/wire_protocol_server.cpp` | 21 | `* @note Gap Summary: total=5; TODO=2, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=108, M=32, L=0` |
| `network/wire_protocol_batch.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `network/envoy_xds.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=15, M=23, L=0` |
| `network/raft_load_balancer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=11, M=5, L=0` |
| `network/wire_protocol_v2.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=7, L=0` |
| `network/adaptive_circuit_breaker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0` |
| `network/geo_topology_router.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0` |
| `network/wire_protocol_connection_pool.cpp` | 18 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=17, M=4, L=0` |
| `network/udp_server.cpp` | 17 | `* @note Gap Summary: total=2; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0` |
| `network/wire_protocol_zero_copy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=3, M=3, L=0` |
| `network/network_audit_log.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=1, L=0` |
| `network/quic_server.cpp` | 20 | `* @note Gap Summary: total=2; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=11, M=11, L=0` |
| `network/udp_fast_path.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0` |
| `network/quic_transport.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=6, L=0` |
| `network/grpc_transport.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `network/service_mesh.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=1, M=5, L=0` |
| `toolbox/text_quality_scorer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `toolbox/toolbox_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `toolbox/language_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `toolbox/text_chunker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `toolbox/content_fingerprinter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `toolbox/content_toolbox_bridge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0` |
| `toolbox/toolbox_composite.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `toolbox/text_normalizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `toolbox/toolbox_builder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `toolbox/ingestion_toolbox.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0` |
| `toolbox/toolbox_streaming.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `base/ab_test_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `base/remote_registry_client.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=10, H=0, M=4, L=0` |
| `base/module_sandbox.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=11, L=0` |
| `base/hot_reload_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `base/plugin_dependency_graph.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=21, L=0` |
| `base/wasm_plugin_sandbox.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=5, L=0` |
| `base/wasm_runtime_injector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `base/module_loader.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=8, M=10, L=0` |
| `transaction/branch_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `transaction/snapshot_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `transaction/transaction_auditor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `transaction/transaction_batcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=0, L=0` |
| `transaction/distributed_transaction_manager.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=28, M=7, L=0` |
| `transaction/lock_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=9, L=0` |
| `transaction/saga_orchestrator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=33, L=0` |
| `transaction/deadlock_predictor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=1, L=0` |
| `transaction/crash_recovery_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0` |
| `transaction/merge_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=19, L=0` |
| `transaction/saga_plugin/saga_orchestrator_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `transaction/global_transaction_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=22, M=0, L=0` |
| `transaction/transaction_semantic_advisor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=2, L=0` |
| `transaction/saga.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `transaction/saga_plugin_bridge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=0, L=0` |
| `transaction/transaction_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=20, M=16, L=0` |
| `transaction/distributed_saga.cpp` | 7 | `* @note Gap Summary: total=5; TODO=2, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=10, H=4, M=21, L=0` |
| `analytics/process_mining.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=5, M=76, L=0` |
| `analytics/diff_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=12, L=0` |
| `analytics/olap.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=6, H=23, M=65, L=0` |
| `analytics/forecasting.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=28, H=9, M=12, L=2` |
| `analytics/incremental_view.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=7, L=0` |
| `analytics/arrow_export.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `analytics/ml_serving.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=14, M=7, L=0` |
| `analytics/columnar_execution.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=20, M=18, L=0` |
| `analytics/distributed_analytics.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=0, C=11, H=24, M=13, L=0` |
| `analytics/knowledge_base.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=4, L=0` |
| `analytics/nlp_text_analyzer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=1, M=12, L=0` |
| `analytics/detail/memory_pool.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `analytics/detail/ring_buffer.h` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `analytics/streaming_window.cpp` | 7 | `* @note Gap Summary: total=20; TODO=18, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=9, H=10, M=17, L=0` |
| `analytics/streaming_window.cpp` | 50 | `* Open TODOs (tracked here per code-review requirements; see also` |
| `analytics/streaming_window.cpp` | 53 | `* TODO(v1.8.0) #1: RESOLVED — idle_timeout background thread added to` |
| `analytics/streaming_window.cpp` | 58 | `* TODO(v1.8.0) #2: RESOLVED — partition_key stored in InternalWindow for both` |
| `analytics/streaming_window.cpp` | 62 | `* TODO(v1.8.0) #3: RESOLVED — SessionWindow::expiryLoop now passes` |
| `analytics/streaming_window.cpp` | 66 | `* TODO(v1.8.0) #4: RESOLVED — StreamingWindowPipeline::Config gains` |
| `analytics/streaming_window.cpp` | 71 | `* TODO(v1.8.0) #5: RESOLVED — O(N) duplicate-detection loop in` |
| `analytics/streaming_window.cpp` | 76 | `* TODO(v1.8.0) #6: RESOLVED — calcPercentile() now accepts a const reference;` |
| `analytics/streaming_window.cpp` | 80 | `* TODO(v1.8.0) #7: RESOLVED — SessionWindow::computeResult() now accepts a` |
| `analytics/streaming_window.cpp` | 84 | `* TODO(v1.8.0) #8: RESOLVED — The double-close guard is already present via` |
| `analytics/streaming_window.cpp` | 172 | `*  fixes TODO(v1.8.0) #6: was taking by value (O(N) copy per call-site).` |
| `analytics/process_pattern_matcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0` |
| `analytics/jit_aggregation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=5, M=1, L=0` |
| `analytics/expert_system_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=13, L=0` |
| `analytics/lora_pattern_classifier.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `analytics/llm_process_analyzer.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=11, L=0` |
| `analytics/automl.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=11, M=34, L=1` |
| `analytics/streaming_join.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=24, L=0` |
| `analytics/cep_engine.cpp` | 7 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=2, H=11, M=65, L=0` |
| `analytics/model_serving.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=9, M=1, L=0` |
| `analytics/analytics_export.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0` |
| `analytics/arrow_flight.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=15, L=0` |
| `analytics/anomaly_detection.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=7, M=18, L=1` |
| `config/config_schema_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=3, L=0` |
| `config/config_audit_log.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `config/config_path_resolver.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=11, M=8, L=0` |
| `config/config_metrics_exporter.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=1, Debt=0, C=3, H=6, M=0, L=0` |
| `config/config_encrypted_store.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=18, M=4, L=0` |
| `config/config_file_watcher.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=5, H=6, M=15, L=0` |
| `chaos/chaos_framework.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `llama_cpp/llama_cpp_registrar.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=1, L=0` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 7 | `* @note Gap Summary: total=22; TODO=1, Stub=20, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `llama_cpp/llama_cpp_plugin.cpp` | 7 | `* @note Gap Summary: total=19; TODO=1, Stub=15, Unimpl=0, Mock=1, Sim=2, Debt=0, C=4, H=11, M=11, L=0` |
| `scraper/scraper_api_client.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0` |
| `scraper/scraper_llm_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0` |
| `scraper/gov_source_catalog.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0` |
| `scraper/scraper_metadata_writer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `scraper/scraper_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=6, L=0` |
| `scraper/scraper_config.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0` |
| `scraper/scraper_js_renderer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=9, L=0` |
| `scraper/scraper_search_engine.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `voice/voice_audio_storage.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `voice/voice_security.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `voice/audio_preprocessing.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `voice/voice_error_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `voice/voice_batch_processor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=8, L=0` |
| `voice/voice_telephony.cpp` | 7 | `* @note Gap Summary: total=12; TODO=1, Stub=7, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=1, M=5, L=0` |
| `voice/voice_tts_customizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=6, L=0` |
| `voice/voice_browser_streaming.cpp` | 7 | `* @note Gap Summary: total=11; TODO=1, Stub=8, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `voice/wake_word_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `voice/voice_assistant.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=11, M=5, L=0` |
| `voice/voice_intent_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=3, L=0` |
| `voice/voice_accessibility.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0` |
| `voice/emotion_analyzer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=1` |
| `voice/voice_assistant_llm.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=7, M=3, L=0` |
| `voice/voice_model_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `voice/voice_meeting_support.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=6, L=0` |
| `voice/voice_authenticator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=1` |
| `voice/voice_session_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0` |
| `voice/voice_macro_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=21, L=0` |
| `performance/phase2_feature_flags.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `performance/workload_predictor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `performance/ligra.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=2, M=10, L=0` |
| `performance/async_metrics_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=0, L=0` |
| `performance/intelligent_prefetcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `performance/adaptive_query_compiler.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=1, H=8, M=29, L=0` |
| `performance/rabitq.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `performance/numa_memory_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=3, L=0` |
| `performance/phase3/bao.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=0, M=1, L=0` |
| `performance/phase3/feature_flags.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `performance/phase3/memory_pressure.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `performance/phase3/per_query_cost_model.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `performance/phase3/splinterdb.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `performance/phase3/bwtree.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=8, M=4, L=0` |
| `performance/phase3/diskann.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=2, M=1, L=0` |
| `performance/phase3/gunrock.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `performance/phase3/adaptive_batch_tuner.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `performance/workload_adaptive_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `performance/numa_topology.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=14, L=0` |
| `performance/prometheus_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=9, H=0, M=1, L=0` |
| `performance/cycle_metrics.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=2, L=0` |
| `performance/advanced_cache_manager.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=3, L=0` |
| `performance/phase4/io_uring_zero_copy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=56, H=2, M=5, L=0` |
| `performance/phase4/pmu_counters.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=6, H=0, M=10, L=0` |
| `performance/phase4/feature_flags.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `performance/phase4/pmem_storage.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=2, M=2, L=0` |
| `performance/cicada.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0` |
| `performance/chimera_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=2, L=0` |
| `performance/dostoevsky.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `performance/wisckey.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=1, M=2, L=0` |
| `performance/hardware_accelerator.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=5, M=8, L=0` |
| `exporters/export_format_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=2, L=0` |
| `exporters/huggingface_hub_client.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=2, M=0, L=0` |
| `exporters/huggingface_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=8, L=0` |
| `exporters/incremental_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `exporters/format_template.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `exporters/parquet_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=7, M=4, L=0` |
| `exporters/stream_writer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=4, M=1, L=0` |
| `exporters/arrow_ipc_exporter.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=10, L=0` |
| `exporters/aql_predicate_filter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0` |
| `exporters/streaming_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `exporters/data_augmentation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=1, L=0` |
| `exporters/jsonl_llm_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=3, M=2, L=0` |
| `exporters/exporter_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `exporters/export_encryption.cpp` | 8 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0,` |
| `exporters/join_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `exporters/pii_detector.cpp` | 7 | `* @note Gap Summary: total=4; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `whisper/tests/test_whisper_plugin_registrar.cpp` | 7 | `* @note Gap Summary: total=13; TODO=1, Stub=11, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `ai/ai_plugin_generator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `ai/cai_ethics_integration.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0` |
| `stubs.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `query/statistical_aggregator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `query/functions/process_mining_functions.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=6, Unimpl=2, Mock=1, Sim=0, Debt=0, C=0, H=1, M=17, L=0` |
| `query/functions/udf_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `query/functions/function_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `query/functions/ethics_functions.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=3, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `query/functions/fulltext_functions.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=8, M=30, L=0` |
| `query/functions/lora_functions.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=8, M=10, L=0` |
| `query/functions/tensor_functions.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=11, M=8, L=0` |
| `query/result_type_annotation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=5, L=0` |
| `query/cypher_parser.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=17, L=0` |
| `query/gremlin_parser.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=25, L=0` |
| `query/query_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=23, H=30, M=73, L=0` |
| `query/optimizer_cost_model.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=3, Debt=0, C=6, H=8, M=0, L=0` |
| `query/sql_parser.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `query/query_federation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0` |
| `query/semantic_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=1, L=0` |
| `query/adaptive_join.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=5, M=15, L=0` |
| `query/query_compiler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0` |
| `query/materialized_view.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=6, L=0` |
| `query/adaptive_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `query/vectorized_execution.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=1, L=0` |
| `query/sparql_parser.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=19, M=14, L=0` |
| `query/let_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=10, L=0` |
| `query/query_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=6, L=0` |
| `query/query_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=3, L=0` |
| `query/query_canceller.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `query/continuous_query_planner.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `query/query_profiler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `query/result_stream.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `query/materialized_cte.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `query/plan_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0` |
| `query/query_cache_manager.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0` |
| `query/aql_parser.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=12, L=0` |
| `query/cq_watermark.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `query/tensor_contraction_engine.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=17, L=0` |
| `query/cte_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `query/aql_safety_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `query/incremental_agg.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `query/continuous_query_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `query/window_evaluator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `query/cross_cluster_federation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `query/runtime_reoptimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `query/parallel_executor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=2, M=10, L=0` |
| `query/approximate_aggregator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `query/cte_subquery.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=1, L=0` |
| `query/query_rewrite_rule.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `query/tensor_aware_query_optimizer.cpp` | 7 | `* @note Gap Summary: total=9; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=2, L=0` |
| `query/aql_parser_json.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `query/aql_runner.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=13, L=0` |
| `query/synopsis_store.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `query/workload_cache_strategy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `query/aql_translator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=54, H=6, M=10, L=0` |
| `query/query_plan_visualizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=10, M=10, L=0` |
| `demo_encryption.cpp` | 7 | `* @note Gap Summary: total=15; TODO=1, Stub=1, Unimpl=0, Mock=11, Sim=2, Debt=0, C=8, H=70, M=0, L=0` |
| `importers/mdm_metrics.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `importers/flatfile_importer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=9, M=19, L=0` |
| `importers/kafka_importer.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=5, Sim=0, Debt=0, C=1, H=8, M=4, L=0` |
| `importers/schema_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `importers/audit_trail.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `importers/huggingface_ingestion_plugin.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=1, M=8, L=0` |
| `importers/schema_inference.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=19, M=6, L=0` |
| `importers/graphql_federation.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `importers/canonical_resolver.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `importers/conflict_resolver.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `importers/mysql_importer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=11, M=25, L=0` |
| `importers/postgres_cdc.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `importers/federated_learning.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `importers/mdm_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=9, M=9, L=0` |
| `importers/crdt_importer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `importers/s3_importer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=10, M=8, L=0` |
| `importers/gui_import_wizard.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=1, L=0` |
| `importers/mongo_importer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=9, M=15, L=0` |
| `importers/entity_linker.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `importers/postgres_importer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=18, M=35, L=0` |
| `importers/data_quality.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `importers/oracle_importer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=8, M=8, L=0` |
| `importers/adaptive_import.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `importers/column_importance.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=3, L=0` |
| `importers/blockchain_integrity.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=1, L=0` |
| `importers/sqlite_importer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=8, M=9, L=0` |
| `importers/temporal_support.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `importers/semantic_matcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `importers/mdm_audit_trail.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `importers/postgres_importer_mdm.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `importers/deterministic_matcher.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=17, L=0` |
| `importers/polyglot_mapper.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `aql/aql_agent.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=10, M=2, L=0` |
| `aql/llm_metrics_collector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=8, H=13, M=0, L=0` |
| `aql/aql_lora_finetuner.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=16, L=0` |
| `aql/docs_assistant_functions.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=3, L=0` |
| `aql/aql_rollback_suggester.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `aql/aql_model_router.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `aql/aql_query_builder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=16, L=0` |
| `aql/llm_aql_embedding_bridge.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `aql/classify_bridge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0` |
| `aql/aql_schema_provider.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `aql/aql_autocomplete.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=19, L=0` |
| `aql/aql_migration_assistant.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0` |
| `aql/aql_conversation_context.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `aql/aql_fewshot_example_library.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `aql/aql_ingestion_bridge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `aql/aql_syntax_highlighter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=16, L=0` |
| `aql/llm_aql_handler.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=2, Unimpl=0, Mock=3, Sim=0, Debt=0, C=25, H=92, M=24, L=0` |
| `aql/aql_query_validator.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0` |
| `aql/aql_query_diff_explainer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `aql/aql_confidence_scorer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=4, L=0` |
| `aql/aql_query_template_library.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=4, L=0` |
| `aql/aql_optimizer_advisor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `storage/transaction_retry_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/index_maintenance.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=1, C=0, H=1, M=8, L=0` |
| `storage/compaction_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/rocksdb_wrapper.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=19, H=7, M=10, L=0` |
| `storage/wom_tree.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=2, L=0` |
| `storage/columnar_format.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=42, M=15, L=0` |
| `storage/nlp_metadata_extractor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `storage/hierarchical_tucker_decomposer.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=11, H=25, M=4, L=0` |
| `storage/tt_quantizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=8, M=18, L=0` |
| `storage/base_entity.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=1, M=3, L=0` |
| `storage/tensor_network_storage_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=4, L=0` |
| `storage/tensor_train_decomposer.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=6, L=0` |
| `storage/compression_strategy.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=2, L=0` |
| `storage/tensor_compaction_filter.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=6, H=1, M=0, L=0` |
| `storage/storage_parquet_exporter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `storage/merge_operators.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/ggml_tensor_bridge.cpp` | 7 | `* @note Gap Summary: total=26; TODO=1, Stub=19, Unimpl=0, Mock=1, Sim=5, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/hamming_coder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=1, L=0` |
| `storage/batch_write_optimizer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/erasure_coder_factory.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=21, L=0` |
| `storage/storage_audit_logger.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=6` |
| `storage/simd_filter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=29, L=0` |
| `storage/nvme_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=21, H=5, M=2, L=0` |
| `storage/distributed_transaction_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=13, M=0, L=0` |
| `storage/pitr_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0` |
| `storage/security_signature.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0` |
| `storage/erasure_coding_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=2, L=0` |
| `storage/tiered_storage.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `storage/history_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=3, L=0` |
| `storage/blob_backend_gcs.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `storage/raft_mvcc_bridge.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `storage/backup_manager.cpp` | 7 | `* @note Gap Summary: total=13; TODO=1, Stub=9, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=10, M=14, L=0` |
| `storage/schema_dead_weight_detector.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/blob_backend_s3.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `storage/disk_space_monitor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `storage/blob_redundancy_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=17, L=0` |
| `storage/zero_copy_blob_transfer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=0, M=2, L=0` |
| `storage/mvcc_store.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `storage/hlc.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/mvcc_chain_pruner.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/index_analyzer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `storage/storage_engine.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=3, Debt=0, C=1, H=2, M=0, L=0` |
| `storage/blob_backend_filesystem.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/security_signature_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `storage/wal_storage.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=2, L=0` |
| `storage/blob_backend_webdav.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `storage/gguf_metadata.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=1, C=0, H=1, M=2, L=0` |
| `storage/blob_backend_azure.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `storage/concurrent_write_controller.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=1, L=0` |
| `storage/vector_index_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/tensor_router.cpp` | 7 | `* @note Gap Summary: total=9; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=6, M=0, L=2` |
| `storage/gpu_compression.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=29, M=8, L=0` |
| `storage/online_schema_migration.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `storage/columnar_cache.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=4, L=0` |
| `storage/streaming_ingest_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `storage/encrypted_blob_backend.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/compressed_storage.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `storage/key_schema.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `storage/storage_layout_advisor.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `storage/database_connection_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=3, L=0` |
| `storage/adaptive_compaction.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0` |
| `server/oauth2_provider.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0` |
| `server/api_security_audit.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `server/mqtt_client_service.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=14, M=17, L=0` |
| `server/error_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `server/admin_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/themis_core_grpc_service.cpp` | 7 | `* @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0` |
| `server/pii_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `server/graphql_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `server/ethics_api_handler.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0` |
| `server/lora_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=17, L=0` |
| `server/mvcc_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/session_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=0, L=0` |
| `server/ranger_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0` |
| `server/pki_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0` |
| `server/voice_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=4, M=69, L=0` |
| `server/tenant_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `server/openapi_route_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0` |
| `server/schema_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=23, L=0` |
| `server/response_transformer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `server/merge_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `server/diff_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/spatial_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `server/retention_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0` |
| `server/buffer_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0` |
| `server/rate_limiter_v2.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=15, M=3, L=0` |
| `server/rope_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=9, M=14, L=0` |
| `server/cache_admin_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0` |
| `server/continuous_query_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `server/distributed_gateway.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=2, M=10, L=0` |
| `server/mqtt_session.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=8, L=0` |
| `server/grpc_web_proxy_handler.cpp` | 7 | `* @note Gap Summary: total=10; TODO=1, Stub=4, Unimpl=3, Mock=1, Sim=1, Debt=0, C=0, H=1, M=2, L=0` |
| `server/api_version.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/prompt_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `server/policy_versioning_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0` |
| `server/buffer_binary_protocol.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=0, L=0` |
| `server/policy_manager_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `server/wasm_handler_registry.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=9, L=0` |
| `server/hsm_provider_global.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/content_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=15, L=0` |
| `server/export_api_handler.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0` |
| `server/policy_template_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `server/cdn_cache_middleware.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=2, L=0` |
| `server/import_api_handler.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=6, L=0` |
| `server/udf_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `server/prompt_engineering_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=2, L=0` |
| `server/transaction_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=10, L=0` |
| `server/bpmn_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=3, L=0` |
| `server/timeseries_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0` |
| `server/timeseries_api_handler.cpp` | 40 | `// TODO(W9-5): Wire setAggregatesProvider() after construction so that` |
| `server/http_type_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `server/pitr_grpc_service.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/http3_datagram.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/cache_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `server/policy_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `server/replication_topology_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=3, M=12, L=0` |
| `server/service_mesh_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/task_scheduler_api_handler.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=0, M=123, L=5` |
| `server/rpc/rpc_service_impl.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=25, L=0` |
| `server/rpc/snapshot_transfer_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0` |
| `server/rpc/differential_update_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=11, L=0` |
| `server/rpc/blob_transfer_handler.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `server/wal_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `server/audit_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0` |
| `server/feedback_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0` |
| `server/load_shedder.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/serverless_function_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `server/saml_auth_provider.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=14, L=0` |
| `server/http3_production_config.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/adaptive_rate_limiter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `server/branch_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `server/http_server.cpp` | 7 | `* @note Gap Summary: total=5; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=48, H=69, M=113, L=0` |
| `server/http_server.cpp` | 114 | `#include "server/http_type_adapter.h"  // TODO: Remove after migration to cpp-httplib (see HTTP_SERVER_REFACTORING_ACTIO` |
| `server/snapshot_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `server/mcp_server.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=2, Unimpl=2, Mock=1, Sim=1, Debt=0, C=1, H=27, M=36, L=0` |
| `server/reports_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `server/compliance_reporting_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0` |
| `server/graph_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=19, L=0` |
| `server/sharding_metrics_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0` |
| `server/update_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0` |
| `server/request_coalescing.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/policy_validation_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `server/rate_limiting_middleware.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `server/changefeed_api_handler.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=10, L=0` |
| `server/api_gateway.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0` |
| `server/geo_topology_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=6, L=0` |
| `server/saga_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=7, L=0` |
| `server/chunked_response_writer.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0` |
| `server/prompt_engineering_grpc_service.cpp` | 7 | `* @note Gap Summary: total=7; TODO=1, Stub=3, Unimpl=1, Mock=1, Sim=1, Debt=0, C=0, H=0, M=1, L=0` |
| `server/cost_based_rate_limiter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/vector_api_handler.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=14, L=0` |
| `server/async_job_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=11, M=8, L=0` |
| `server/smart_routing.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=0, M=2, L=0` |
| `server/llm_grpc_service.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=8, M=4, L=0` |
| `server/maintenance_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=4, L=0` |
| `server/api_auth_config.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/policy_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=27, L=0` |
| `server/hot_reload_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0` |
| `server/shard_repair_api_handler.cpp` | 7 | `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=7, H=3, M=43, L=0` |
| `server/query_api_handler.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=1, C=24, H=19, M=77, L=0` |
| `server/request_validation_middleware.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/wal_grpc_service.cpp` | 10 | `* @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=3, M=1, L=0` |
| `server/sse_connection_manager.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=0, L=0` |
| `server/opa_adapter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/review_scheduling_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0` |
| `server/keys_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/rate_limiter.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0` |
| `server/distributed_txn_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=3, L=0` |
| `server/profiling_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=13, L=0` |
| `server/monitoring_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=3, M=72, L=0` |
| `server/import_wizard_builder.cpp` | 7 | `* @note Gap Summary: total=8; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=170, L=0` |
| `server/health_error_service.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=5, M=0, L=0` |
| `server/index_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=10, L=0` |
| `server/llm_api_handler.cpp` | 7 | `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=2, C=13, H=62, M=46, L=0` |
| `server/classification_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0` |
| `server/api_key_mgmt_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0` |
| `server/websocket_session.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=17, M=7, L=0` |
| `server/http2_session.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=9, L=0` |
| `server/auth_middleware.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=2, M=7, L=0` |
| `server/postgres_session.cpp` | 7 | `* @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=10, M=68, L=0` |
| `server/workload_fingerprint_engine.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a` |
| `server/entity_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=4, M=15, L=0` |
| `server/pitr_api_handler.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0` |
| `server/http3_session.cpp` | 7 | `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=5, M=21, L=0` |

### STUB

| Datei | Zeile | Kontext |
|-------|-------|---------|
| `training/multi_task_lora.cpp` | 82 | `// STUB/SIMULATION NOTE (MTL-S02 — SGD training loop, no BLAS)` |
| `training/multi_task_lora.cpp` | 126 | `// STUB/SIMULATION NOTE (MTL-S01 — cosine-similarity gating heuristic)` |
| `training/multi_task_lora.cpp` | 151 | `// Training loop (MTL-S02 — see STUB/SIMULATION NOTE above).` |
| `training/multi_task_lora.cpp` | 282 | `// MTL-S01 gating heuristic (cosine similarity to prototype vectors — see STUB/SIMULATION NOTE above).` |
| `llm/lora_framework/gpu_tensor.cpp` | 33 | `// ─── dtype-cast callback bridges (STUB #2 / STUB #3) ───────────────────�` |
| `llm/lora_framework/gpu_tensor.cpp` | 362 | `// STUB/SIMULATION NOTE` |
| `llm/lora_framework/gpu_tensor.cpp` | 399 | `// STUB/SIMULATION NOTE` |
| `llm/inference_engine_enhanced.cpp` | 2015 | `// the local generateDraftTokens() path (see STUB/SIMULATION NOTE below).` |
| `llm/inference_engine_enhanced.cpp` | 2090 | `// Byte-modulo fallback (STUB #263)` |
| `llm/inference_engine_enhanced.cpp` | 2113 | `// STUB #261 — Production Injection Point (wired by` |
| `llm/inference_engine_enhanced.cpp` | 2182 | `spdlog::debug("Local draft (STUB #261 bridge): "` |
| `llm/inference_engine_enhanced.cpp` | 2186 | `spdlog::warn("Local draft (STUB #261 bridge): "` |
| `llm/inference_engine_enhanced.cpp` | 2191 | `spdlog::warn("Local draft (STUB #261 bridge): "` |
| `llm/inference_engine_enhanced.cpp` | 2248 | `// STUB/SIMULATION NOTE` |
| `llm/embedded_llm_stub.cpp` | 326 | `// In THEMIS_LLM_STUB_MODE (test/dev-only builds) return the deterministic` |
| `llm/embedded_llm_stub.cpp` | 333 | `// Activation: compile-time flag THEMIS_LLM_STUB_MODE (never set in release presets).` |
| `llm/embedded_llm_stub.cpp` | 343 | `#ifdef THEMIS_LLM_STUB_MODE` |
| `llm/embedded_llm_stub.cpp` | 380 | `#ifdef THEMIS_LLM_STUB_MODE` |
| `llm/llm_plugin_manager.cpp` | 668 | `#ifdef THEMIS_LLAMA_CPP_STUB_MODE` |
| `llm/ssm_stub_plugin.cpp` | 18 | `SyntheticSSMStub::SyntheticSSMStub() : rng_(STUB_SEED) {` |
| `llm/ssm_stub_plugin.cpp` | 21 | `oss << "stub-v0.1-seed" << STUB_SEED << "-dim" << HIDDEN_DIM;` |
| `cdc/kafka_cdc_producer.cpp` | 32 | `*   via injected callbacks in tests/dev builds (see STUB #98 bridge APIs).` |
| `ingestion/s3_connector.cpp` | 313 | `// STUB/SIMULATION NOTE` |
| `ingestion/s3_connector.cpp` | 481 | `// STUB/SIMULATION NOTE` |
| `ingestion/database_connector.cpp` | 449 | `// STUB/SIMULATION NOTE` |
| `ingestion/kafka_connector.cpp` | 229 | `// STUB/SIMULATION NOTE` |
| `ingestion/cdc_connector.cpp` | 558 | `// STUB/SIMULATION NOTE` |
| `ingestion/object_storage_connector.cpp` | 264 | `// STUB/SIMULATION NOTE` |
| `plugins/wasm_plugin_loader.cpp` | 476 | `// STUB/SIMULATION NOTE` |
| `cache/redis_cache_coordinator.cpp` | 70 | `// STUB #42 — RedisPublishFn static bridge (non-hiredis injection)` |
| `cache/distributed_cache_coordinator.cpp` | 73 | `// STUB #61 — RedisPublishBridgeFn static bridge (non-POSIX injection)` |
| `themis/build_info.cpp` | 57 | `// HSM MODULE STATUS BRIDGE – storage (STUB #95)` |
| `themis/build_info.cpp` | 573 | `// STUB #95: Consult the runtime bridge when available so the server can` |
| `onnx_clip/onnx_clip_plugin.h` | 112 | `// Injectable model-hash bridge (STUB #94)` |
| `onnx_clip/onnx_clip_plugin.cpp` | 44 | `// STUB #94 — ModelHashFn static bridge (non-OpenSSL SHA-256 injection)` |
| `process/process_community_detector.cpp` | 351 | `// planned for Q4 2026 (STUB_INVENTORY #238); once integrated, `llm_endpoint`` |
| `geo/gpu_backend_stub.cpp` | 577 | `// STUB/SIMULATION NOTE` |
| `geo/gpu_kernel_dispatcher_cpu.cpp` | 13 | `// STUB/SIMULATION NOTE` |
| `geo/cpu_backend.cpp` | 34 | `// STUB/SIMULATION NOTE` |
| `geo/device_detector.cpp` | 35 | `/// STUB/SIMULATION NOTE` |
| `main_server.cpp` | 41 | `//      `THEMIS_ALLOW_HSM_STUB=1`. Missing/invalid HSM config and HSM init` |
| `ethics_ai/argument_store.h` | 76 | `* former STUB/SIMULATION NOTE for the vector path in storeArgument().` |
| `ethics_ai/prior_round_compressor.cpp` | 225 | `// than only citation tokens) are retained.  See STUB_INVENTORY.md entry #235.` |
| `index/gpu_vector_index_vulkan.cpp` | 980 | `// STUB/SIMULATION NOTE` |
| `index/advanced_vector_index.cpp` | 34 | `// STUB/SIMULATION NOTE` |
| `governance/opa_adapter.cpp` | 335 | `// STUB/SIMULATION NOTE` |
| `utils/build_info.cpp` | 549 | `// STUB #95 — RESOLVED via HsmModuleStatusFn bridge in canonical` |
| `utils/build_info.cpp` | 553 | `// Original STUB/SIMULATION NOTE` |
| `acceleration/graphics_backends.cpp` | 1037 | `// ── STUB #169 bridge — global GLSL→SPIR-V compiler storage ─────────────────` |
| `acceleration/vulkan_backend_full.cpp` | 41 | `// ── STUB #169 bridge — access storage from graphics_backends.cpp (extern) ───` |
| `acceleration/vulkan_backend_full.cpp` | 173 | `// STUB/SIMULATION NOTE` |
| `acceleration/vulkan_backend_full.cpp` | 191 | `std::cerr << "GLSL to SPIR-V compilation requires shaderc library (STUB)" << std::endl;` |
| `acceleration/vulkan_backend_full.cpp` | 192 | `// Check injected fn first (STUB #169 bridge).` |
| `acceleration/vulkan_backend_full.cpp` | 210 | `std::cerr << "GLSL to SPIR-V compilation requires shaderc library (STUB #169)" << std::endl;` |
| `acceleration/nccl_vector_backend.cpp` | 565 | `// STUB/SIMULATION NOTE` |
| `acceleration/nccl_vector_backend.cpp` | 582 | `// STUB/SIMULATION NOTE (allReduce bridge)` |
| `acceleration/oneapi_backend.cpp` | 236 | `// STUB/SIMULATION NOTE` |
| `acceleration/oneapi_backend.cpp` | 251 | `// STUB/SIMULATION NOTE (computeDistances bridge)` |
| `acceleration/opencl_backend.cpp` | 349 | `// STUB/SIMULATION NOTE` |
| `acceleration/opencl_backend.cpp` | 364 | `// STUB/SIMULATION NOTE (computeDistances bridge)` |
| `acceleration/ai_hardware_dispatcher.cpp` | 741 | `// STUB/SIMULATION NOTE` |
| `security/timestamp_authority_openssl.cpp` | 13 | `// STUB/SIMULATION NOTE` |
| `security/timestamp_authority.cpp` | 21 | `//          Production mode is explicitly blocked unless THEMIS_ALLOW_TSA_STUB=1 is set.` |
| `security/timestamp_authority.cpp` | 74 | `const char* allow_stub = std::getenv("THEMIS_ALLOW_TSA_STUB");` |
| `security/timestamp_authority.cpp` | 85 | `"or set THEMIS_ALLOW_TSA_STUB=1 to explicitly allow the insecure stub.";` |
| `security/timestamp_authority.cpp` | 145 | `// WARNING: This is a STUB implementation for development only` |
| `security/timestamp_authority.cpp` | 148 | `THEMIS_WARN("Using TimestampAuthority STUB - NOT SECURE for production!");` |
| `security/timestamp_authority.cpp` | 198 | `tok.serial_number = "STUB-SERIAL";` |
| `security/timestamp_authority.cpp` | 203 | `tok.tsa_name = "STUB-TSA";` |
| `security/timestamp_authority.cpp` | 204 | `tok.tsa_serial = "STUB-TSA-SERIAL";` |
| `security/timestamp_authority.cpp` | 283 | `std::string("-----BEGIN CERTIFICATE-----\nSTUB-TSA\n-----END CERTIFICATE-----\n");` |
| `security/timestamp_authority.cpp` | 345 | `"or set THEMIS_ALLOW_TSA_STUB=1 for explicit non-production override.");` |
| `security/hsm_provider.cpp` | 24 | `//          Production mode is explicitly blocked unless THEMIS_ALLOW_HSM_STUB=1 env var` |
| `security/hsm_provider.cpp` | 190 | `const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");` |
| `security/hsm_provider.cpp` | 194 | `// This cannot be overridden by THEMIS_ALLOW_HSM_STUB.` |
| `security/hsm_provider.cpp` | 215 | `last_error_ = "HSM stub provider detected production environment but THEMIS_ALLOW_HSM_STUB is not set. "` |
| `security/hsm_provider.cpp` | 216 | `"Set THEMIS_ALLOW_HSM_STUB=1 to explicitly allow insecure stub, or use real HSM.";` |
| `security/hsm_provider.cpp` | 240 | `THEMIS_WARN("║  ⚠️  INSECURE CONFIGURATION: HSM STUB PROVIDER ACTIVE!  ⚠️   ║");` |
| `security/hsm_provider.cpp` | 251 | `THEMIS_WARN("║  - Set THEMIS_ALLOW_HSM_STUB=1 environment variable          ║");` |
| `security/hsm_provider.cpp` | 308 | `THEMIS_WARN("HSMProvider STUB signing - NOT cryptographically secure!");` |
| `security/hsm_provider.cpp` | 313 | `r.cert_serial = "STUB-CERT";` |
| `security/hsm_provider.cpp` | 383 | `THEMIS_WARN("HSMProvider STUB encryptData - NOT hardware-protected, for development only!");` |
| `security/hsm_provider.cpp` | 411 | `THEMIS_WARN("HSMProvider STUB decryptData - NOT hardware-protected, for development only!");` |
| `security/hsm_provider.cpp` | 504 | `const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");` |
| `security/hsm_provider.cpp` | 508 | `"is insecure. Set THEMIS_ALLOW_HSM_STUB=1 for explicit development override, "` |
| `security/hsm_provider.cpp` | 515 | `"(THEMIS_ALLOW_HSM_STUB=1). Not suitable for production.",` |
| `security/hsm_provider.cpp` | 517 | `return std::string("-----BEGIN CERTIFICATE-----\nSTUB\n-----END CERTIFICATE-----\n");` |
| `security/hsm_provider.cpp` | 572 | `std::optional<std::string> HSMPKIClient::getCertSerial() { return std::string("STUB-SERIAL"); }` |
| `security/hsm_provider_pkcs11.cpp` | 82 | `//             fails). Controlled by THEMIS_ALLOW_HSM_STUB env var in production mode.` |
| `security/hsm_provider_pkcs11.cpp` | 445 | `THEMIS_WARN("║  ⚠️  HSM FALLBACK STUB ACTIVE - INSECURE CONFIGURATION  ⚠️   ║");` |
| `security/hsm_provider_pkcs11.cpp` | 669 | `r.cert_serial = "STUB-CERT"; ` |
| `security/hsm_provider_pkcs11.cpp` | 1109 | `return std::string("-----BEGIN CERTIFICATE-----\nSTUB\n-----END CERTIFICATE-----\n");` |
| `security/hsm_key_provider_adapter.cpp` | 28 | `const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");` |
| `security/hsm_key_provider_adapter.cpp` | 34 | `// ── Process-wide injectable DEK bridge (STUB #47 / #48) ───────────────────�` |
| `security/hsm_key_provider_adapter.cpp` | 388 | `// ── Injected bridge (STUB #47) ───────────────────────────�` |
| `security/hsm_key_provider_adapter.cpp` | 415 | `"Configure a real PKCS#11 HSM or set THEMIS_ALLOW_HSM_STUB=1 "` |
| `security/hsm_key_provider_adapter.cpp` | 460 | `// ── Injected bridge (STUB #48) ───────────────────────────�` |
| `security/hsm_key_provider_adapter.cpp` | 487 | `"Configure a real PKCS#11 HSM or set THEMIS_ALLOW_HSM_STUB=1 "` |
| `security/hsm_key_provider_adapter.cpp` | 605 | `// ── Static bridge setters (STUB #47 / #48) ───────────────────────�` |
| `rag/targ_retrieval.cpp` | 24 | `// FullEntropyFn injection bridge (STUB #262)` |
| `rag/targ_retrieval.cpp` | 81 | `// If a full-vocabulary entropy function is injected, use it (STUB #262).` |
| `api/themisdb_grpc_service.cpp` | 1860 | `// STUB/SIMULATION NOTE` |
| `tensor/tensor_butterfly_operator.cpp` | 199 | `// STUB #268 — RADON bridge storage` |
| `tensor/tensor_butterfly_operator.cpp` | 206 | `// STUB #268 — GREENS_FUNCTION bridge storage` |
| `tensor/tensor_butterfly_operator.cpp` | 226 | `// STUB #268 — RADON bridge` |
| `tensor/tensor_butterfly_operator.cpp` | 239 | `// STUB #268 — GREENS_FUNCTION bridge` |
| `tensor/tensor_butterfly_operator.cpp` | 350 | `// (STUB #268): if no fn was set, build() already prevented construction` |
| `tensor/tensor_butterfly_operator.cpp` | 499 | `// Apply injected FOURIER backend when available (STUB #267),` |
| `tensor/tensor_mmap_bridge.cpp` | 38 | `// STUB #270 — SST page-map bridge storage` |
| `tensor/tensor_mmap_bridge.cpp` | 141 | `// Snapshot the SST-page-map bridge fn once (STUB #270).` |
| `tensor/tensor_mmap_bridge.cpp` | 163 | `// STUB #270: try the injected SST page-map fn first (zero-copy path).` |
| `tensor/tensor_mmap_bridge.cpp` | 177 | `// Fallback: MAP_ANONYMOUS + memcpy (STUB #270 — Q1 2027).` |
| `tensor/adapter_repository.cpp` | 216 | `// Delegate to injected mmap-style loader backend when available (STUB #265).` |
| `tensor/adapter_repository.cpp` | 251 | `// STUB/SIMULATION NOTE (AR-01)` |
| `tensor/adapter_repository.cpp` | 364 | `// Delegate to injected exact-similarity backend when available (STUB #266).` |
| `tensor/adapter_repository.cpp` | 398 | `// STUB/SIMULATION NOTE (AR-02 / STUB #266)` |
| `tensor/utr_converter.cpp` | 38 | `// Static bridge slots — STUB #257 (EmbedFn) / STUB #258 (ImageEmbedFn)` |
| `tensor/utr_converter.cpp` | 216 | `// STUB/SIMULATION NOTE` |
| `tensor/utr_converter.cpp` | 483 | `// STUB/SIMULATION NOTE` |
| `tensor/tensor_index_manager.cpp` | 334 | `// mapCores() — Phase 3 mmap-pinned TT-core bridge (TIM-01, STUB #176)` |
| `tensor/tensor_core_bridge.cpp` | 47 | `// STUB #269 — default backend factory bridge` |
| `tensor/tensor_core_bridge.cpp` | 79 | `// STUB #269: try the process-wide factory first (injected by production` |
| `gpu/stream_manager.cpp` | 38 | `// STUB #77 — CudaStreamBackendFn static bridge (non-CUDA injection)` |
| `transaction/distributed_transaction_manager.cpp` | 67 | `// RPC phase-2 bridge (STUB #279)` |
| `transaction/distributed_transaction_manager.cpp` | 69 | `// STUB/SIMULATION NOTE` |
| `transaction/distributed_transaction_manager.cpp` | 97 | `// RPC phase-1 bridge (STUB #279 — Phase-1 PREPARE extension)` |
| `transaction/distributed_transaction_manager.cpp` | 99 | `// STUB/SIMULATION NOTE` |
| `analytics/process_mining.cpp` | 13 | `#if defined(_WIN32) && defined(THEMIS_PROCESS_MINING_WINDOWS_STUB)` |
| `analytics/process_mining.cpp` | 14 | `// STUB/SIMULATION NOTE` |
| `analytics/process_mining.cpp` | 19 | `// Activation: Compiled when both _WIN32 and THEMIS_PROCESS_MINING_WINDOWS_STUB` |
| `analytics/process_mining.cpp` | 28 | `//   the THEMIS_PROCESS_MINING_WINDOWS_STUB CMake option.  Tracking` |
| `analytics/process_mining.cpp` | 42 | `"Windows stub build (THEMIS_PROCESS_MINING_WINDOWS_STUB). "` |
| `analytics/process_mining.cpp` | 2328 | `// RESOLUTION NOTE (clusterVariants — was: naive round-robin, see STUB_INVENTORY.md #212):` |
| `analytics/olap.cpp` | 2188 | `// STUB/SIMULATION NOTE` |
| `analytics/knowledge_base.cpp` | 36 | `// STUB #272 — injectable YAML parser bridge` |
| `llama_cpp/llama_cpp_registrar.cpp` | 69 | `// STUB/SIMULATION NOTE` |
| `llama_cpp/llama_cpp_registrar.cpp` | 85 | `#ifdef THEMIS_LLAMA_CPP_STUB_MODE` |
| `llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp` | 9 | `* All tests run under THEMIS_LLAMA_CPP_STUB_MODE; no real model is required.` |
| `llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp` | 8 | `* All tests run under THEMIS_LLAMA_CPP_STUB_MODE so no real model file is` |
| `llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp` | 74 | `// In THEMIS_LLAMA_CPP_STUB_MODE loadModel() always succeeds, so this` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 474 | `// These tests verify the production contract: generate() without STUB_MODE` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 476 | `// descriptive error_message.  In this test file THEMIS_LLAMA_CPP_STUB_MODE is` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 480 | `// O2: model loaded with empty path, STUB_MODE → still returns success=true  (sanity)` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 493 | `// With THEMIS_LLAMA_CPP_STUB_MODE defined (as it is in this test build),` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 501 | `<< "THEMIS_LLAMA_CPP_STUB_MODE must preserve success=true for test builds";` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 518 | `// All tests operate in STUB_MODE (no real model required) and set a generous` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 753 | `///     THEMIS_LLAMA_CPP_STUB_MODE (the test binary always defines this macro).` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 1075 | `///     Requires THEMIS_LLAMA_CPP_STUB_MODE so the callback is exercised in` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 1078 | `#ifndef THEMIS_LLAMA_CPP_STUB_MODE` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 1079 | `GTEST_SKIP() << "Requires THEMIS_LLAMA_CPP_STUB_MODE for stub callback path";` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 1128 | `#ifndef THEMIS_LLAMA_CPP_STUB_MODE` |
| `llama_cpp/tests/test_llama_cpp_plugin.cpp` | 1129 | `GTEST_SKIP() << "Requires THEMIS_LLAMA_CPP_STUB_MODE for stub path";` |
| `llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp` | 7 | `* LlamaCppPlugin API.  All tests run under THEMIS_LLAMA_CPP_STUB_MODE so` |
| `llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp` | 6 | `* THEMIS_LLAMA_CPP_STUB_MODE` |
| `llama_cpp/llama_cpp_plugin.cpp` | 347 | `// STUB/SIMULATION NOTE` |
| `llama_cpp/llama_cpp_plugin.cpp` | 351 | `//          behaviour should define THEMIS_LLAMA_CPP_STUB_MODE.` |
| `llama_cpp/llama_cpp_plugin.cpp` | 361 | `#ifdef THEMIS_LLAMA_CPP_STUB_MODE` |
| `llama_cpp/llama_cpp_plugin.cpp` | 586 | `// STUB/SIMULATION NOTE` |
| `llama_cpp/llama_cpp_plugin.cpp` | 775 | `// STUB/SIMULATION NOTE` |
| `llama_cpp/llama_cpp_plugin.cpp` | 823 | `// STUB/SIMULATION NOTE` |
| `llama_cpp/llama_cpp_plugin.cpp` | 848 | `// STUB/SIMULATION NOTE` |
| `voice/audio_preprocessing.cpp` | 55 | `// STUB/SIMULATION NOTE` |
| `voice/voice_telephony.cpp` | 491 | `//                   See STUB_INVENTORY entry #173 and` |
| `voice/voice_telephony.cpp` | 709 | `//                   See STUB_INVENTORY entry #174 and` |
| `voice/voice_browser_streaming.cpp` | 132 | `// STUB/SIMULATION NOTE` |
| `performance/cycle_metrics.cpp` | 165 | `// STUB/SIMULATION NOTE` |
| `performance/advanced_cache_manager.cpp` | 81 | `// STUB/SIMULATION NOTE` |
| `performance/phase4/pmu_counters.cpp` | 777 | `// STUB/SIMULATION NOTE` |
| `storage/tensor_compaction_filter.cpp` | 13 | `// STUB/SIMULATION NOTE` |
| `storage/tensor_compaction_filter.cpp` | 55 | `// STUB/SIMULATION NOTE (STUB #264 — RecompressFn injection bridge)` |
| `storage/tensor_compaction_filter.cpp` | 71 | `// RecompressFn injection bridge (STUB #264)` |
| `storage/ggml_tensor_bridge.cpp` | 48 | `// STUB/SIMULATION NOTE (STUB #263a — GgmlAllocFn injection bridge)` |
| `storage/ggml_tensor_bridge.cpp` | 59 | `// GgmlAllocFn injection bridge (STUB #263a)` |
| `storage/ggml_tensor_bridge.cpp` | 81 | `// STUB/SIMULATION NOTE (STUB #263b — PrefetchFn injection bridge)` |
| `storage/ggml_tensor_bridge.cpp` | 91 | `// PrefetchFn injection bridge (STUB #263b)` |
| `storage/ggml_tensor_bridge.cpp` | 113 | `// STUB/SIMULATION NOTE (STUB #263c — TypeRegistrationFn injection bridge)` |
| `storage/ggml_tensor_bridge.cpp` | 124 | `// TypeRegistrationFn injection bridge (STUB #263c)` |
| `storage/ggml_tensor_bridge.cpp` | 221 | `// Return real allocation when GgmlAllocFn was wired (GTB-01 / STUB #263a).` |
| `storage/backup_manager.cpp` | 1613 | `THEMIS_WARN("BackupManager::decompressPath: STUB — files copied without decompression "` |
| `storage/backup_manager.cpp` | 1813 | `THEMIS_WARN("BackupManager::decryptFile: STUB — files will be copied without "` |
| `server/rope_api_handler.cpp` | 845 | `// STUB #307 REMEDIATION: Query real rotation metrics` |
| `server/timeseries_api_handler.cpp` | 415 | `// STUB #301 REMEDIATION: Use real aggregates provider if available` |
| `server/timeseries_api_handler.cpp` | 426 | `// STUB/SIMULATION NOTE` |
| `server/timeseries_api_handler.cpp` | 490 | `// STUB #301 REMEDIATION: Use real retention policies provider if available` |
| `server/mcp_server.cpp` | 2961 | `// STUB/SIMULATION NOTE` |

### MOCK

| Datei | Zeile | Kontext |
|-------|-------|---------|
| `security/field_encryption.cpp` | 323 | `// Set THEMIS_ALLOW_MOCK_KEY_PROVIDER=1 only in test/demo environments.` |
| `security/field_encryption.cpp` | 324 | `const char* allow_env = std::getenv("THEMIS_ALLOW_MOCK_KEY_PROVIDER");` |
| `security/field_encryption.cpp` | 333 | `"To explicitly opt in for testing, set THEMIS_ALLOW_MOCK_KEY_PROVIDER=1.");` |
| `rag/llm_judge_integration.cpp` | 60 | `THEMIS_WARN("LLMJudgeIntegration initialized with nullptr engine in MOCK MODE "` |
| `rag/llm_judge_integration.cpp` | 83 | `THEMIS_WARN("LLMJudgeIntegration initialized in MOCK MODE - evaluations will use stub responses");` |
| `rag/llm_judge_integration.cpp` | 224 | `THEMIS_WARN("LLM evaluation using MOCK MODE - results are not real (warning shown once)");` |

### FIXME

| Datei | Zeile | Kontext |
|-------|-------|---------|
| `network/wire_protocol_server.cpp` | 1253 | `// KNOWN LIMITATION (FIXME): payload_buffer_ is also used by asyncReadPayload` |

