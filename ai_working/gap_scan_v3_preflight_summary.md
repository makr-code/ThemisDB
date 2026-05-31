# Gap Scanner v3 Preflight Summary

Generated: 2026-05-31T20:07:38.755147

## Headline

- Actionable (CRITICAL+HIGH): 2000
- High-confidence CRITICAL (>= 0.85): 1906
- Net-new high-confidence CRITICAL vs previous snapshot: 398

## Top Categories

- llm_ai_safety: 1278
- distributed_consistency: 435
- audit_logging: 103
- gpu_memory_safety: 71
- performance: 69
- observability: 14
- query: 6
- llm: 6
- security: 4
- utils: 2
- auth: 2
- deprecated_apis: 1
- themis: 1
- storage: 1
- stable_diffusion: 1

## Top Files

- src\replication\replication_manager.cpp: 132
- src\llm\lora_framework\kernels\vulkan_kernels.cpp: 82
- src\rag\rag_judge.cpp: 77
- src\llm\lora_framework\gpu_lora_layers.cpp: 75
- src\rag\batch_evaluator.cpp: 50
- src\llm\lora_framework\flash_lora.cpp: 48
- src\rag\adversarial_tester.cpp: 46
- src\llm\lora_framework\lora_training_service.cpp: 40
- src\llm\lora_framework\kernels\directx_kernels.cpp: 39
- src\acceleration\ai_hardware_dispatcher.cpp: 39
- src\replication\conflict_resolution.cpp: 35
- src\llm\llama_wrapper.cpp: 33
- src\sharding\shard_router.cpp: 31
- src\llm\lora_framework\kernels\hip_fused_kernels.cpp: 31
- src\training\incremental_lora_trainer.cpp: 30
- src\analytics\distributed_analytics.cpp: 27
- src\llm\lora_framework\kernels\cpu_fused_kernels.cpp: 26
- src\distributed_knowledge\federated_rag_merger.cpp: 25
- src\aql\llm_aql_handler.cpp: 25
- src\llm\lora_framework\quantization.cpp: 24
- src\rag\dpr_vectorizer.cpp: 23
- src\llm\lora_framework\lora_layers.cpp: 22
- src\llm\kernel_fusion.cpp: 22
- src\llm\lora_framework\kernels\hip_kernels.cpp: 21
- src\llm\lora_framework\data_loader.cpp: 21

## Top Actionable Items (Top 25)

- [CRITICAL] audit_logging | src\voice\voice_authenticator.cpp:335 | conf=0.99 | Security function "authenticate" without audit log
- [CRITICAL] llm_ai_safety | src\voice\voice_assistant_llm.cpp:44 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] audit_logging | src\voice\voice_assistant.cpp:659 | conf=0.99 | Security function "authenticate" without audit log
- [CRITICAL] audit_logging | src\voice\voice_assistant.cpp:264 | conf=0.99 | Security function "authenticate" without audit log
- [CRITICAL] audit_logging | src\voice\voice_assistant.cpp:144 | conf=0.99 | Security function "authenticate" without audit log
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:900 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:889 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:758 | conf=0.99 | Concurrent update without version vector or causal ordering
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:608 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:605 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:311 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:257 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:246 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:226 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:220 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] distributed_consistency | src\transaction\distributed_saga.cpp:51 | conf=0.99 | Write without consensus/replication acknowledgment
- [CRITICAL] llm_ai_safety | src\training\training_pipeline.cpp:305 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\training_pipeline.cpp:275 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\training_pipeline.cpp:202 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\training_pipeline.cpp:199 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\training_pipeline.cpp:152 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\modality_parser.cpp:435 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\modality_parser.cpp:350 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\modality_parser.cpp:316 | conf=0.99 | User input in prompt without sanitization (injection risk)
- [CRITICAL] llm_ai_safety | src\training\modality_parser.cpp:272 | conf=0.99 | User input in prompt without sanitization (injection risk)
