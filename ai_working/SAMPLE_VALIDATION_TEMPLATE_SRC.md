# Representative Validation Sample (SRC Scan)

- Generated: 2026-06-03T20:57:56
- Source: ai_working/gap_scan_results_src.json
- Population: 23297
- Sample size: 120
- Stratification: (severity, category) proportional + fill
- Seed: 20260603

## Composition

### Severity

- HIGH: 63
- MEDIUM: 37
- CRITICAL: 20

### Top Scopes

- llm: 15
- server: 13
- sharding: 9
- content: 8
- query: 6
- security: 5
- transaction: 5
- index: 5
- auth: 5
- replication: 4

### Top Categories

- pointer_arithmetic_unbounded: 20
- missing_vector_reserve: 19
- uncaught_exception: 10
- resource_leaked_in_exception: 7
- generic_catch: 5
- no_retry_logic: 3
- hardcoded_output: 3
- delete_without_nullptr: 1
- new_without_raii: 1
- array_bounds_violation: 1
- shared_ptr_cycle: 1
- unchecked_malloc: 1

## Review Checklist

- TP = echtes Problem
- FP = false positive
- ? = unklar / Follow-up

| # | File | Line | Severity | Type | Scope | TP | FP | ? | Notes |
|---:|---|---:|---|---|---|---|---|---|---|
| 1 | plugins/plugin_hot_plug_monitor.cpp | 261 | HIGH | pointer_arithmetic_unbounded | plugins | [ ] | [ ] | [ ] |  |
| 2 | content/office_processor.cpp | 162 | HIGH | pointer_arithmetic_unbounded | content | [ ] | [ ] | [ ] |  |
| 3 | llm/model_loader.cpp | 958 | HIGH | pointer_arithmetic_unbounded | llm | [ ] | [ ] | [ ] |  |
| 4 | sharding/secure_transport_client.cpp | 180 | HIGH | pointer_arithmetic_unbounded | sharding | [ ] | [ ] | [ ] |  |
| 5 | content/language_detector.cpp | 202 | HIGH | pointer_arithmetic_unbounded | content | [ ] | [ ] | [ ] |  |
| 6 | server/postgres_session.cpp | 1365 | HIGH | pointer_arithmetic_unbounded | server | [ ] | [ ] | [ ] |  |
| 7 | llm/lora_framework/vulkan_buffer.cpp | 229 | HIGH | pointer_arithmetic_unbounded | llm | [ ] | [ ] | [ ] |  |
| 8 | content/geo_processor.cpp | 486 | HIGH | pointer_arithmetic_unbounded | content | [ ] | [ ] | [ ] |  |
| 9 | security/hsm_key_provider_adapter.cpp | 316 | HIGH | pointer_arithmetic_unbounded | security | [ ] | [ ] | [ ] |  |
| 10 | transaction/distributed_saga.cpp | 397 | HIGH | pointer_arithmetic_unbounded | transaction | [ ] | [ ] | [ ] |  |
| 11 | process/process_graph_rag.cpp | 244 | HIGH | pointer_arithmetic_unbounded | process | [ ] | [ ] | [ ] |  |
| 12 | index/cuda_hnsw_graph_traversal.cpp | 558 | HIGH | pointer_arithmetic_unbounded | index | [ ] | [ ] | [ ] |  |
| 13 | llm/gguf_loader.cpp | 614 | HIGH | pointer_arithmetic_unbounded | llm | [ ] | [ ] | [ ] |  |
| 14 | sharding/transaction_wal.cpp | 348 | HIGH | pointer_arithmetic_unbounded | sharding | [ ] | [ ] | [ ] |  |
| 15 | content/geo_processor.cpp | 825 | HIGH | pointer_arithmetic_unbounded | content | [ ] | [ ] | [ ] |  |
| 16 | utils/memory/pool_allocator.cpp | 831 | HIGH | pointer_arithmetic_unbounded | utils | [ ] | [ ] | [ ] |  |
| 17 | sharding/signed_request.cpp | 108 | HIGH | pointer_arithmetic_unbounded | sharding | [ ] | [ ] | [ ] |  |
| 18 | llm/lora_framework/paged_memory_manager.cpp | 191 | HIGH | pointer_arithmetic_unbounded | llm | [ ] | [ ] | [ ] |  |
| 19 | llm/lora_framework/vulkan_pipeline.cpp | 107 | HIGH | pointer_arithmetic_unbounded | llm | [ ] | [ ] | [ ] |  |
| 20 | acceleration/vec_knn.cpp | 265 | HIGH | pointer_arithmetic_unbounded | acceleration | [ ] | [ ] | [ ] |  |
| 21 | llm/feedback_store.cpp | 453 | HIGH | delete_without_nullptr | llm | [ ] | [ ] | [ ] |  |
| 22 | server/task_scheduler_api_handler.cpp | 602 | CRITICAL | new_without_raii | server | [ ] | [ ] | [ ] |  |
| 23 | storage/wal_storage.cpp | 192 | CRITICAL | array_bounds_violation | storage | [ ] | [ ] | [ ] |  |
| 24 | api/graphql.cpp | 789 | MEDIUM | shared_ptr_cycle | api | [ ] | [ ] | [ ] |  |
| 25 | llm/gpu_memory_manager.cpp | 1188 | HIGH | unchecked_malloc | llm | [ ] | [ ] | [ ] |  |
| 26 | server/llm_grpc_service.cpp | 180 | HIGH | no_retry_logic | server | [ ] | [ ] | [ ] |  |
| 27 | llm/async_inference_engine.cpp | 274 | HIGH | no_retry_logic | llm | [ ] | [ ] | [ ] |  |
| 28 | server/websocket_session.cpp | 341 | HIGH | no_retry_logic | server | [ ] | [ ] | [ ] |  |
| 29 | geo/gpu_backend_production.cpp | 83 | CRITICAL | blocking_no_timeout | geo | [ ] | [ ] | [ ] |  |
| 30 | tensor/hiss_structural_search.cpp | 443 | HIGH | uncaught_exception | tensor | [ ] | [ ] | [ ] |  |
| 31 | llm/llama_wrapper.cpp | 3043 | HIGH | uncaught_exception | llm | [ ] | [ ] | [ ] |  |
| 32 | timeseries/encrypted_chunk_store.cpp | 190 | HIGH | uncaught_exception | timeseries | [ ] | [ ] | [ ] |  |
| 33 | auth/webauthn_authenticator.cpp | 270 | HIGH | uncaught_exception | auth | [ ] | [ ] | [ ] |  |
| 34 | security/vcc_pki_client.cpp | 260 | HIGH | uncaught_exception | security | [ ] | [ ] | [ ] |  |
| 35 | llm/inline_training_engine.cpp | 955 | HIGH | uncaught_exception | llm | [ ] | [ ] | [ ] |  |
| 36 | query/adaptive_join.cpp | 235 | HIGH | uncaught_exception | query | [ ] | [ ] | [ ] |  |
| 37 | observability/metric_aggregator.cpp | 192 | HIGH | uncaught_exception | observability | [ ] | [ ] | [ ] |  |
| 38 | query/let_evaluator.cpp | 1076 | HIGH | uncaught_exception | query | [ ] | [ ] | [ ] |  |
| 39 | training/ada_lora_adapter.cpp | 323 | HIGH | uncaught_exception | training | [ ] | [ ] | [ ] |  |
| 40 | replication/replication_manager.cpp | 5242 | MEDIUM | generic_catch | replication | [ ] | [ ] | [ ] |  |
| 41 | index/advanced_vector_index.cpp | 456 | MEDIUM | generic_catch | index | [ ] | [ ] | [ ] |  |
| 42 | server/monitoring_api_handler.cpp | 720 | MEDIUM | generic_catch | server | [ ] | [ ] | [ ] |  |
| 43 | server/llm_api_handler.cpp | 1193 | MEDIUM | generic_catch | server | [ ] | [ ] | [ ] |  |
| 44 | transaction/branch_manager.cpp | 734 | MEDIUM | generic_catch | transaction | [ ] | [ ] | [ ] |  |
| 45 | llm/shared_worker_pool.cpp | 132 | MEDIUM | primitive_no_volatile | llm | [ ] | [ ] | [ ] |  |
| 46 | llm/model_loader.cpp | 212 | CRITICAL | double_lock | llm | [ ] | [ ] | [ ] |  |
| 47 | cache/semantic_cache.cpp | 80 | CRITICAL | thread_join_no_timeout | cache | [ ] | [ ] | [ ] |  |
| 48 | security/embedded_user_registration_plugin.cpp | 367 | HIGH | shared_state_no_sync | security | [ ] | [ ] | [ ] |  |
| 49 | analytics/expert_system_engine.cpp | 336 | HIGH | explicit_lock_unlock | analytics | [ ] | [ ] | [ ] |  |
| 50 | server/rpc/rpc_service_impl.cpp | 714 | HIGH | explicit_delete | server | [ ] | [ ] | [ ] |  |
| 51 | acceleration/faiss_gpu_backend.cpp | 248 | CRITICAL | unwrapped_resource | acceleration | [ ] | [ ] | [ ] |  |
| 52 | temporal/temporal_cdc.cpp | 314 | HIGH | manual_cleanup_in_destructor | temporal | [ ] | [ ] | [ ] |  |
| 53 | utils/lz4_codec.cpp | 43 | MEDIUM | cast_to_smaller_type | utils | [ ] | [ ] | [ ] |  |
| 54 | query/functions/fulltext_functions.cpp | 284 | HIGH | arithmetic_overflow | query | [ ] | [ ] | [ ] |  |
| 55 | content/pipeline/multimodal_chunker.cpp | 150 | CRITICAL | multiplication_overflow | content | [ ] | [ ] | [ ] |  |
| 56 | llm/distributed_training_coordinator.cpp | 228 | MEDIUM | shift_overflow | llm | [ ] | [ ] | [ ] |  |
| 57 | query/functions/fulltext_functions.cpp | 784 | CRITICAL | function_return_truncation | query | [ ] | [ ] | [ ] |  |
| 58 | query/statistical_aggregator.cpp | 58 | HIGH | unchecked_array_index | query | [ ] | [ ] | [ ] |  |
| 59 | themis/wire_protocol_server.cpp | 767 | CRITICAL | unchecked_memcpy | themis | [ ] | [ ] | [ ] |  |
| 60 | index/graph_analytics.cpp | 655 | HIGH | user_controlled_size | index | [ ] | [ ] | [ ] |  |
| 61 | content/content_manager.cpp | 1158 | HIGH | resource_leaked_in_exception | content | [ ] | [ ] | [ ] |  |
| 62 | core/concerns/concerns_context.cpp | 464 | HIGH | resource_leaked_in_exception | core | [ ] | [ ] | [ ] |  |
| 63 | cdc/tenant_buffer_manager.cpp | 344 | HIGH | resource_leaked_in_exception | cdc | [ ] | [ ] | [ ] |  |
| 64 | plugins/plugin_manager.cpp | 1129 | HIGH | resource_leaked_in_exception | plugins | [ ] | [ ] | [ ] |  |
| 65 | stable_diffusion/tests/test_sd_plugin.cpp | 419 | HIGH | resource_leaked_in_exception | stable_diffusion | [ ] | [ ] | [ ] |  |
| 66 | security/hsm_provider_pkcs11.cpp | 174 | HIGH | resource_leaked_in_exception | security | [ ] | [ ] | [ ] |  |
| 67 | auth/jwks_security.cpp | 36 | HIGH | resource_leaked_in_exception | auth | [ ] | [ ] | [ ] |  |
| 68 | content/stt_processor.cpp | 337 | MEDIUM | missing_move_constructor_defaulted | content | [ ] | [ ] | [ ] |  |
| 69 | cdc/outbox.cpp | 225 | CRITICAL | exception_in_destructor | cdc | [ ] | [ ] | [ ] |  |
| 70 | auth/jwt_validator.cpp | 159 | HIGH | unsafe_move_assignment | auth | [ ] | [ ] | [ ] |  |
| 71 | utils/audit_logger.cpp | 47 | CRITICAL | broken_raii_in_assignment | utils | [ ] | [ ] | [ ] |  |
| 72 | aql/aql_model_router.cpp | 142 | MEDIUM | uninitialized_member_field | aql | [ ] | [ ] | [ ] |  |
| 73 | analytics/streaming_join.cpp | 65 | HIGH | uninitialized_array | analytics | [ ] | [ ] | [ ] |  |
| 74 | sharding/signed_request.cpp | 105 | CRITICAL | uninitialized_pointer | sharding | [ ] | [ ] | [ ] |  |
| 75 | auth/jwks_security.cpp | 46 | HIGH | pointer_without_null_check | auth | [ ] | [ ] | [ ] |  |
| 76 | server/query_api_handler.cpp | 2578 | HIGH | conditional_initialization_use | server | [ ] | [ ] | [ ] |  |
| 77 | analytics/automl.cpp | 952 | MEDIUM | missing_override_keyword | analytics | [ ] | [ ] | [ ] |  |
| 78 | sharding/cloud_backup.cpp | 69 | MEDIUM | pure_virtual_unimplemented | sharding | [ ] | [ ] | [ ] |  |
| 79 | toolbox/toolbox_builder.cpp | 246 | HIGH | virtual_call_in_ctor_dtor | toolbox | [ ] | [ ] | [ ] |  |
| 80 | rag/batch_evaluator.cpp | 420 | HIGH | hardcoded_output | rag | [ ] | [ ] | [ ] |  |
| 81 | demo_encryption.cpp | 131 | HIGH | hardcoded_output | demo_encryption.cpp | [ ] | [ ] | [ ] |  |
| 82 | llm/lora_framework/multi_gpu_trainer.cpp | 147 | HIGH | hardcoded_output | llm | [ ] | [ ] | [ ] |  |
| 83 | demo_encryption.cpp | 186 | CRITICAL | sensitive_data_logging | demo_encryption.cpp | [ ] | [ ] | [ ] |  |
| 84 | server/cache_admin_api_handler.cpp | 167 | CRITICAL | missing_audit_log | server | [ ] | [ ] | [ ] |  |
| 85 | storage/rocksdb_wrapper.cpp | 1515 | MEDIUM | getsnapshot\(\) | storage | [ ] | [ ] | [ ] |  |
| 86 | training/lora_data_selection.cpp | 929 | MEDIUM | string_concat_loop | training | [ ] | [ ] | [ ] |  |
| 87 | process/process_community_detector.cpp | 60 | MEDIUM | missing_vector_reserve | process | [ ] | [ ] | [ ] |  |
| 88 | projects/project_template.cpp | 255 | MEDIUM | missing_vector_reserve | projects | [ ] | [ ] | [ ] |  |
| 89 | importers/federated_learning.cpp | 48 | MEDIUM | missing_vector_reserve | importers | [ ] | [ ] | [ ] |  |
| 90 | server/monitoring_api_handler.cpp | 1388 | MEDIUM | missing_vector_reserve | server | [ ] | [ ] | [ ] |  |
| 91 | metadata/schema_consistency_checker.cpp | 173 | MEDIUM | missing_vector_reserve | metadata | [ ] | [ ] | [ ] |  |
| 92 | transaction/distributed_saga.cpp | 962 | MEDIUM | missing_vector_reserve | transaction | [ ] | [ ] | [ ] |  |
| 93 | sharding/gpu_erasure_coder_opencl.cpp | 500 | MEDIUM | missing_vector_reserve | sharding | [ ] | [ ] | [ ] |  |
| 94 | updates/delta_update_engine.cpp | 254 | MEDIUM | missing_vector_reserve | updates | [ ] | [ ] | [ ] |  |
| 95 | sharding/distributed_transaction.cpp | 732 | MEDIUM | missing_vector_reserve | sharding | [ ] | [ ] | [ ] |  |
| 96 | server/wal_api_handler.cpp | 203 | MEDIUM | missing_vector_reserve | server | [ ] | [ ] | [ ] |  |
| 97 | updates/update_history_logger.cpp | 123 | MEDIUM | missing_vector_reserve | updates | [ ] | [ ] | [ ] |  |
| 98 | security/rbac.cpp | 598 | MEDIUM | missing_vector_reserve | security | [ ] | [ ] | [ ] |  |
| 99 | ingestion/steps/ner_step.cpp | 195 | MEDIUM | missing_vector_reserve | ingestion | [ ] | [ ] | [ ] |  |
| 100 | prompt_engineering/self_improvement_orchestrator.cpp | 145 | MEDIUM | missing_vector_reserve | prompt_engineering | [ ] | [ ] | [ ] |  |
| 101 | query/cte_subquery.cpp | 578 | MEDIUM | missing_vector_reserve | query | [ ] | [ ] | [ ] |  |
| 102 | auth/jwt_key_rotation_manager.cpp | 207 | MEDIUM | missing_vector_reserve | auth | [ ] | [ ] | [ ] |  |
| 103 | content/adapters/format_extractor_factory.cpp | 60 | MEDIUM | missing_vector_reserve | content | [ ] | [ ] | [ ] |  |
| 104 | importers/postgres_importer.cpp | 564 | MEDIUM | missing_vector_reserve | importers | [ ] | [ ] | [ ] |  |
| 105 | training/lora_data_selection.cpp | 370 | MEDIUM | missing_vector_reserve | training | [ ] | [ ] | [ ] |  |
| 106 | server/review_scheduling_api_handler.cpp | 220 | MEDIUM | unnecessary_copy | server | [ ] | [ ] | [ ] |  |
| 107 | transaction/deadlock_predictor.cpp | 326 | HIGH | nested_loop_find | transaction | [ ] | [ ] | [ ] |  |
| 108 | index/graph_analytics.cpp | 224 | MEDIUM | map_vs_unordered_map | index | [ ] | [ ] | [ ] |  |
| 109 | transaction/distributed_transaction_manager.cpp | 641 | HIGH | lock_in_loop | transaction | [ ] | [ ] | [ ] |  |
| 110 | process/vcc_vpb_importer.cpp | 265 | HIGH | regex_in_loop | process | [ ] | [ ] | [ ] |  |
| 111 | index/gpu_memory_oversubscription.cpp | 42 | HIGH | unchecked_cuda_call | index | [ ] | [ ] | [ ] |  |
| 112 | geo/gpu_backend_hip.cpp | 91 | CRITICAL | use_after_free_gpu | geo | [ ] | [ ] | [ ] |  |
| 113 | gpu/unified_memory.cpp | 13 | CRITICAL | gpu_memory_leak | gpu | [ ] | [ ] | [ ] |  |
| 114 | llm/lora_framework/kernels/hip_fused_kernels.cpp | 222 | CRITICAL | missing_sync_threads | llm | [ ] | [ ] | [ ] |  |
| 115 | sharding/metadata_shard.cpp | 149 | HIGH | unspecified_consistency | sharding | [ ] | [ ] | [ ] |  |
| 116 | replication/replication_manager.cpp | 3425 | CRITICAL | missing_version_tracking | replication | [ ] | [ ] | [ ] |  |
| 117 | replication/conflict_resolution.cpp | 597 | HIGH | undefined_conflict_resolution | replication | [ ] | [ ] | [ ] |  |
| 118 | replication/replication_manager.cpp | 2958 | CRITICAL | missing_consensus | replication | [ ] | [ ] | [ ] |  |
| 119 | sharding/paxos_consensus.cpp | 491 | MEDIUM | stale_read_undocumented | sharding | [ ] | [ ] | [ ] |  |
| 120 | server/llm_api_handler.cpp | 898 | CRITICAL | model_integrity_gap | server | [ ] | [ ] | [ ] |  |
