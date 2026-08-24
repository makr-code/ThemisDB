# Gap Scanner v3 Preflight Summary

Generated: 2026-06-02T12:40:50.366203

## Headline

- Actionable (CRITICAL+HIGH): 2000
- High-confidence CRITICAL (>= 0.85): 1878
- Net-new high-confidence CRITICAL vs previous snapshot: 1878

## Top Categories

- llm_ai_safety: 1332
- distributed_consistency: 432
- audit_logging: 92
- gpu_memory_safety: 66
- performance: 24
- observability: 21
- query: 6
- llm: 6
- security: 4
- determinism: 4
- utils: 2
- auth: 2
- deprecated_apis: 1
- themis: 1
- storage: 1

## Top Files

- src\replication\replication_manager.cpp: 134
- src\llm\lora_framework\kernels\vulkan_kernels.cpp: 81
- src\rag\rag_judge.cpp: 77
- src\llm\lora_framework\gpu_lora_layers.cpp: 75
- src\training\multi_task_lora.cpp: 57
- src\rag\batch_evaluator.cpp: 50
- src\llm\lora_framework\flash_lora.cpp: 48
- src\rag\adversarial_tester.cpp: 46
- src\acceleration\ai_hardware_dispatcher.cpp: 39
- src\llm\lora_framework\kernels\directx_kernels.cpp: 38
- src\llm\lora_framework\lora_training_service.cpp: 34
- src\training\incremental_lora_trainer.cpp: 31
- src\llm\lora_framework\kernels\hip_fused_kernels.cpp: 31
- src\llm\llama_wrapper.cpp: 31
- src\sharding\shard_router.cpp: 30
- src\aql\llm_aql_handler.cpp: 27
- src\analytics\distributed_analytics.cpp: 27
- src\llm\lora_framework\kernels\cpu_fused_kernels.cpp: 26
- src\replication\conflict_resolution.cpp: 25
- src\distributed_knowledge\federated_rag_merger.cpp: 25
- src\training\lora_data_selection.cpp: 24
- src\llm\lora_framework\quantization.cpp: 24
- src\rag\dpr_vectorizer.cpp: 23
- src\llm\lora_framework\lora_layers.cpp: 22
- src\llm\kernel_fusion.cpp: 22

## Top Actionable Items (Top 25)

- [CRITICAL] audit_logging | src\voice\voice_authenticator.cpp:335 | conf=0.99 | Security function "authenticate" without audit log
- [CRITICAL] llm_ai_safety | src\voice\voice_assistant_llm.cpp:88 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\voice\voice_assistant_llm.cpp:87 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\voice\voice_assistant_llm.cpp:85 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] audit_logging | src\voice\voice_assistant.cpp:659 | conf=0.99 | Security function "authenticate" without audit log
- [CRITICAL] audit_logging | src\voice\voice_assistant.cpp:264 | conf=0.99 | Security function "authenticate" without audit log
- [CRITICAL] audit_logging | src\voice\voice_assistant.cpp:144 | conf=0.99 | Security function "authenticate" without audit log
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:1041 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:1030 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:899 | conf=0.99 | Concurrent update without version vector or causal ordering
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:781 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:682 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:679 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:333 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:279 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:268 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:248 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:242 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:125 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:51 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] llm_ai_safety | src\training\training_pipeline.cpp:361 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\training_pipeline.cpp:323 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\training_pipeline.cpp:232 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\training_pipeline.cpp:229 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\training_pipeline.cpp:182 | conf=0.99 | User input in prompt without sanitization (injection risk)
