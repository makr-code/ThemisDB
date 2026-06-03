# Fachliche Erstbewertung der Stichprobe

- Quelle: ai_working/gap_scan_results_src.json
- Stichprobe: 120
- Methode: rule-based fachliche Erstbewertung (konservative TP-Einstufung)

## Ergebnis

- TP: 15 (12.5%)
- FP: 48 (40.0%)
- ?: 57 (47.5%)

## Nach Typ (Top 20)

| Typ | TP | FP | ? |
|---|---:|---:|---:|
| pointer_arithmetic_unbounded | 0 | 7 | 13 |
| missing_vector_reserve | 0 | 19 | 0 |
| uncaught_exception | 0 | 10 | 0 |
| resource_leaked_in_exception | 0 | 0 | 7 |
| generic_catch | 0 | 5 | 0 |
| no_retry_logic | 0 | 0 | 3 |
| hardcoded_output | 0 | 3 | 0 |
| delete_without_nullptr | 0 | 1 | 0 |
| new_without_raii | 1 | 0 | 0 |
| array_bounds_violation | 1 | 0 | 0 |
| shared_ptr_cycle | 0 | 1 | 0 |
| unchecked_malloc | 0 | 1 | 0 |
| blocking_no_timeout | 1 | 0 | 0 |
| primitive_no_volatile | 0 | 1 | 0 |
| double_lock | 1 | 0 | 0 |
| thread_join_no_timeout | 1 | 0 | 0 |
| shared_state_no_sync | 1 | 0 | 0 |
| explicit_lock_unlock | 0 | 0 | 1 |
| explicit_delete | 1 | 0 | 0 |
| unwrapped_resource | 1 | 0 | 0 |

## Einzelbewertung

| # | File | Line | Type | Sev | Bewertung | Konf. | Begründung |
|---:|---|---:|---|---|---|---:|---|
| 1 | plugins/plugin_hot_plug_monitor.cpp | 261 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 2 | content/office_processor.cpp | 162 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 3 | llm/model_loader.cpp | 958 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 4 | sharding/secure_transport_client.cpp | 180 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 5 | content/language_detector.cpp | 202 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 6 | server/postgres_session.cpp | 1365 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 7 | llm/lora_framework/vulkan_buffer.cpp | 229 | pointer_arithmetic_unbounded | HIGH | FP | 0.62 | Heuristik triggert häufig auf kontrollierte Buffer-/Container-Zugriffe in Performance-Code. |
| 8 | content/geo_processor.cpp | 486 | pointer_arithmetic_unbounded | HIGH | FP | 0.62 | Heuristik triggert häufig auf kontrollierte Buffer-/Container-Zugriffe in Performance-Code. |
| 9 | security/hsm_key_provider_adapter.cpp | 316 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 10 | transaction/distributed_saga.cpp | 397 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 11 | process/process_graph_rag.cpp | 244 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 12 | index/cuda_hnsw_graph_traversal.cpp | 558 | pointer_arithmetic_unbounded | HIGH | FP | 0.62 | Heuristik triggert häufig auf kontrollierte Buffer-/Container-Zugriffe in Performance-Code. |
| 13 | llm/gguf_loader.cpp | 614 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 14 | sharding/transaction_wal.cpp | 348 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 15 | content/geo_processor.cpp | 825 | pointer_arithmetic_unbounded | HIGH | FP | 0.62 | Heuristik triggert häufig auf kontrollierte Buffer-/Container-Zugriffe in Performance-Code. |
| 16 | utils/memory/pool_allocator.cpp | 831 | pointer_arithmetic_unbounded | HIGH | FP | 0.62 | Heuristik triggert häufig auf kontrollierte Buffer-/Container-Zugriffe in Performance-Code. |
| 17 | sharding/signed_request.cpp | 108 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 18 | llm/lora_framework/paged_memory_manager.cpp | 191 | pointer_arithmetic_unbounded | HIGH | FP | 0.62 | Heuristik triggert häufig auf kontrollierte Buffer-/Container-Zugriffe in Performance-Code. |
| 19 | llm/lora_framework/vulkan_pipeline.cpp | 107 | pointer_arithmetic_unbounded | HIGH | FP | 0.62 | Heuristik triggert häufig auf kontrollierte Buffer-/Container-Zugriffe in Performance-Code. |
| 20 | acceleration/vec_knn.cpp | 265 | pointer_arithmetic_unbounded | HIGH | ? | 0.5 | Potenziell echt, aber ohne vollständigen Guard-Kontext nicht sicher beurteilbar. |
| 21 | llm/feedback_store.cpp | 453 | delete_without_nullptr | HIGH | FP | 0.6 | delete_without_nullptr: häufiges Heuristik-FP-Muster in bisherigen Reviews. |
| 22 | server/task_scheduler_api_handler.cpp | 602 | new_without_raii | CRITICAL | TP | 0.75 | new_without_raii: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 23 | storage/wal_storage.cpp | 192 | array_bounds_violation | CRITICAL | TP | 0.75 | array_bounds_violation: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 24 | api/graphql.cpp | 789 | shared_ptr_cycle | MEDIUM | FP | 0.6 | shared_ptr_cycle: häufiges Heuristik-FP-Muster in bisherigen Reviews. |
| 25 | llm/gpu_memory_manager.cpp | 1188 | unchecked_malloc | HIGH | FP | 0.6 | unchecked_malloc: häufiges Heuristik-FP-Muster in bisherigen Reviews. |
| 26 | server/llm_grpc_service.cpp | 180 | no_retry_logic | HIGH | ? | 0.55 | Kann TP sein, aber hängt von Upstream-Retry/Idempotenz ab. |
| 27 | llm/async_inference_engine.cpp | 274 | no_retry_logic | HIGH | ? | 0.55 | Kann TP sein, aber hängt von Upstream-Retry/Idempotenz ab. |
| 28 | server/websocket_session.cpp | 341 | no_retry_logic | HIGH | ? | 0.55 | Kann TP sein, aber hängt von Upstream-Retry/Idempotenz ab. |
| 29 | geo/gpu_backend_production.cpp | 83 | blocking_no_timeout | CRITICAL | TP | 0.75 | blocking_no_timeout: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 30 | tensor/hiss_structural_search.cpp | 443 | uncaught_exception | HIGH | FP | 0.68 | Exception-Throw in API/Service-Layern ist häufig bewusst; kein klarer Fehlpfadbeleg im Sample. |
| 31 | llm/llama_wrapper.cpp | 3043 | uncaught_exception | HIGH | FP | 0.68 | Exception-Throw in API/Service-Layern ist häufig bewusst; kein klarer Fehlpfadbeleg im Sample. |
| 32 | timeseries/encrypted_chunk_store.cpp | 190 | uncaught_exception | HIGH | FP | 0.68 | Exception-Throw in API/Service-Layern ist häufig bewusst; kein klarer Fehlpfadbeleg im Sample. |
| 33 | auth/webauthn_authenticator.cpp | 270 | uncaught_exception | HIGH | FP | 0.68 | Exception-Throw in API/Service-Layern ist häufig bewusst; kein klarer Fehlpfadbeleg im Sample. |
| 34 | security/vcc_pki_client.cpp | 260 | uncaught_exception | HIGH | FP | 0.68 | Exception-Throw in API/Service-Layern ist häufig bewusst; kein klarer Fehlpfadbeleg im Sample. |
| 35 | llm/inline_training_engine.cpp | 955 | uncaught_exception | HIGH | FP | 0.68 | Exception-Throw in API/Service-Layern ist häufig bewusst; kein klarer Fehlpfadbeleg im Sample. |
| 36 | query/adaptive_join.cpp | 235 | uncaught_exception | HIGH | FP | 0.68 | Exception-Throw in API/Service-Layern ist häufig bewusst; kein klarer Fehlpfadbeleg im Sample. |
| 37 | observability/metric_aggregator.cpp | 192 | uncaught_exception | HIGH | FP | 0.68 | Exception-Throw in API/Service-Layern ist häufig bewusst; kein klarer Fehlpfadbeleg im Sample. |
| 38 | query/let_evaluator.cpp | 1076 | uncaught_exception | HIGH | FP | 0.68 | Exception-Throw in API/Service-Layern ist häufig bewusst; kein klarer Fehlpfadbeleg im Sample. |
| 39 | training/ada_lora_adapter.cpp | 323 | uncaught_exception | HIGH | FP | 0.68 | Exception-Throw in API/Service-Layern ist häufig bewusst; kein klarer Fehlpfadbeleg im Sample. |
| 40 | replication/replication_manager.cpp | 5242 | generic_catch | MEDIUM | FP | 0.63 | Catch-all ist oft bewusstes Boundary-Handling; ohne Schlucken/Maskieren schwer als TP zu markieren. |
| 41 | index/advanced_vector_index.cpp | 456 | generic_catch | MEDIUM | FP | 0.63 | Catch-all ist oft bewusstes Boundary-Handling; ohne Schlucken/Maskieren schwer als TP zu markieren. |
| 42 | server/monitoring_api_handler.cpp | 720 | generic_catch | MEDIUM | FP | 0.63 | Catch-all ist oft bewusstes Boundary-Handling; ohne Schlucken/Maskieren schwer als TP zu markieren. |
| 43 | server/llm_api_handler.cpp | 1193 | generic_catch | MEDIUM | FP | 0.63 | Catch-all ist oft bewusstes Boundary-Handling; ohne Schlucken/Maskieren schwer als TP zu markieren. |
| 44 | transaction/branch_manager.cpp | 734 | generic_catch | MEDIUM | FP | 0.63 | Catch-all ist oft bewusstes Boundary-Handling; ohne Schlucken/Maskieren schwer als TP zu markieren. |
| 45 | llm/shared_worker_pool.cpp | 132 | primitive_no_volatile | MEDIUM | FP | 0.6 | primitive_no_volatile: häufiges Heuristik-FP-Muster in bisherigen Reviews. |
| 46 | llm/model_loader.cpp | 212 | double_lock | CRITICAL | TP | 0.75 | double_lock: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 47 | cache/semantic_cache.cpp | 80 | thread_join_no_timeout | CRITICAL | TP | 0.75 | thread_join_no_timeout: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 48 | security/embedded_user_registration_plugin.cpp | 367 | shared_state_no_sync | HIGH | TP | 0.75 | shared_state_no_sync: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 49 | analytics/expert_system_engine.cpp | 336 | explicit_lock_unlock | HIGH | ? | 0.55 | explicit_lock_unlock: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 50 | server/rpc/rpc_service_impl.cpp | 714 | explicit_delete | HIGH | TP | 0.75 | explicit_delete: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 51 | acceleration/faiss_gpu_backend.cpp | 248 | unwrapped_resource | CRITICAL | TP | 0.75 | unwrapped_resource: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 52 | temporal/temporal_cdc.cpp | 314 | manual_cleanup_in_destructor | HIGH | TP | 0.75 | manual_cleanup_in_destructor: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 53 | utils/lz4_codec.cpp | 43 | cast_to_smaller_type | MEDIUM | ? | 0.55 | cast_to_smaller_type: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 54 | query/functions/fulltext_functions.cpp | 284 | arithmetic_overflow | HIGH | TP | 0.75 | arithmetic_overflow: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 55 | content/pipeline/multimodal_chunker.cpp | 150 | multiplication_overflow | CRITICAL | TP | 0.75 | multiplication_overflow: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 56 | llm/distributed_training_coordinator.cpp | 228 | shift_overflow | MEDIUM | ? | 0.55 | shift_overflow: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 57 | query/functions/fulltext_functions.cpp | 784 | function_return_truncation | CRITICAL | TP | 0.75 | function_return_truncation: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 58 | query/statistical_aggregator.cpp | 58 | unchecked_array_index | HIGH | TP | 0.75 | unchecked_array_index: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 59 | themis/wire_protocol_server.cpp | 767 | unchecked_memcpy | CRITICAL | TP | 0.75 | unchecked_memcpy: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 60 | index/graph_analytics.cpp | 655 | user_controlled_size | HIGH | TP | 0.75 | user_controlled_size: typischerweise sicherheits-/korrektheitsrelevant ohne zusätzlichen Kontext. |
| 61 | content/content_manager.cpp | 1158 | resource_leaked_in_exception | HIGH | ? | 0.55 | resource_leaked_in_exception: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 62 | core/concerns/concerns_context.cpp | 464 | resource_leaked_in_exception | HIGH | ? | 0.55 | resource_leaked_in_exception: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 63 | cdc/tenant_buffer_manager.cpp | 344 | resource_leaked_in_exception | HIGH | ? | 0.55 | resource_leaked_in_exception: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 64 | plugins/plugin_manager.cpp | 1129 | resource_leaked_in_exception | HIGH | ? | 0.55 | resource_leaked_in_exception: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 65 | stable_diffusion/tests/test_sd_plugin.cpp | 419 | resource_leaked_in_exception | HIGH | ? | 0.55 | resource_leaked_in_exception: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 66 | security/hsm_provider_pkcs11.cpp | 174 | resource_leaked_in_exception | HIGH | ? | 0.55 | resource_leaked_in_exception: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 67 | auth/jwks_security.cpp | 36 | resource_leaked_in_exception | HIGH | ? | 0.55 | resource_leaked_in_exception: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 68 | content/stt_processor.cpp | 337 | missing_move_constructor_defaulted | MEDIUM | ? | 0.55 | missing_move_constructor_defaulted: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 69 | cdc/outbox.cpp | 225 | exception_in_destructor | CRITICAL | ? | 0.55 | exception_in_destructor: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 70 | auth/jwt_validator.cpp | 159 | unsafe_move_assignment | HIGH | ? | 0.55 | unsafe_move_assignment: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 71 | utils/audit_logger.cpp | 47 | broken_raii_in_assignment | CRITICAL | ? | 0.55 | broken_raii_in_assignment: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 72 | aql/aql_model_router.cpp | 142 | uninitialized_member_field | MEDIUM | ? | 0.55 | uninitialized_member_field: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 73 | analytics/streaming_join.cpp | 65 | uninitialized_array | HIGH | ? | 0.55 | uninitialized_array: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 74 | sharding/signed_request.cpp | 105 | uninitialized_pointer | CRITICAL | ? | 0.55 | uninitialized_pointer: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 75 | auth/jwks_security.cpp | 46 | pointer_without_null_check | HIGH | ? | 0.55 | pointer_without_null_check: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 76 | server/query_api_handler.cpp | 2578 | conditional_initialization_use | HIGH | ? | 0.55 | conditional_initialization_use: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 77 | analytics/automl.cpp | 952 | missing_override_keyword | MEDIUM | ? | 0.55 | missing_override_keyword: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 78 | sharding/cloud_backup.cpp | 69 | pure_virtual_unimplemented | MEDIUM | ? | 0.55 | pure_virtual_unimplemented: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 79 | toolbox/toolbox_builder.cpp | 246 | virtual_call_in_ctor_dtor | HIGH | ? | 0.55 | virtual_call_in_ctor_dtor: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 80 | rag/batch_evaluator.cpp | 420 | hardcoded_output | HIGH | FP | 0.6 | hardcoded_output: häufiges Heuristik-FP-Muster in bisherigen Reviews. |
| 81 | demo_encryption.cpp | 131 | hardcoded_output | HIGH | FP | 0.6 | hardcoded_output: häufiges Heuristik-FP-Muster in bisherigen Reviews. |
| 82 | llm/lora_framework/multi_gpu_trainer.cpp | 147 | hardcoded_output | HIGH | FP | 0.6 | hardcoded_output: häufiges Heuristik-FP-Muster in bisherigen Reviews. |
| 83 | demo_encryption.cpp | 186 | sensitive_data_logging | CRITICAL | ? | 0.55 | sensitive_data_logging: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 84 | server/cache_admin_api_handler.cpp | 167 | missing_audit_log | CRITICAL | ? | 0.55 | missing_audit_log: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 85 | storage/rocksdb_wrapper.cpp | 1515 | getsnapshot\(\) | MEDIUM | ? | 0.55 | getsnapshot\(\): keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 86 | training/lora_data_selection.cpp | 929 | string_concat_loop | MEDIUM | ? | 0.55 | string_concat_loop: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 87 | process/process_community_detector.cpp | 60 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 88 | projects/project_template.cpp | 255 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 89 | importers/federated_learning.cpp | 48 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 90 | server/monitoring_api_handler.cpp | 1388 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 91 | metadata/schema_consistency_checker.cpp | 173 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 92 | transaction/distributed_saga.cpp | 962 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 93 | sharding/gpu_erasure_coder_opencl.cpp | 500 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 94 | updates/delta_update_engine.cpp | 254 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 95 | sharding/distributed_transaction.cpp | 732 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 96 | server/wal_api_handler.cpp | 203 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 97 | updates/update_history_logger.cpp | 123 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 98 | security/rbac.cpp | 598 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 99 | ingestion/steps/ner_step.cpp | 195 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 100 | prompt_engineering/self_improvement_orchestrator.cpp | 145 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 101 | query/cte_subquery.cpp | 578 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 102 | auth/jwt_key_rotation_manager.cpp | 207 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 103 | content/adapters/format_extractor_factory.cpp | 60 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 104 | importers/postgres_importer.cpp | 564 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 105 | training/lora_data_selection.cpp | 370 | missing_vector_reserve | MEDIUM | FP | 0.67 | Performance-Hinweis, aber fachlich oft akzeptabel; kein funktionaler Fehler. |
| 106 | server/review_scheduling_api_handler.cpp | 220 | unnecessary_copy | MEDIUM | ? | 0.55 | unnecessary_copy: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 107 | transaction/deadlock_predictor.cpp | 326 | nested_loop_find | HIGH | ? | 0.55 | nested_loop_find: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 108 | index/graph_analytics.cpp | 224 | map_vs_unordered_map | MEDIUM | ? | 0.55 | map_vs_unordered_map: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 109 | transaction/distributed_transaction_manager.cpp | 641 | lock_in_loop | HIGH | ? | 0.55 | lock_in_loop: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 110 | process/vcc_vpb_importer.cpp | 265 | regex_in_loop | HIGH | ? | 0.55 | regex_in_loop: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 111 | index/gpu_memory_oversubscription.cpp | 42 | unchecked_cuda_call | HIGH | ? | 0.55 | unchecked_cuda_call: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 112 | geo/gpu_backend_hip.cpp | 91 | use_after_free_gpu | CRITICAL | ? | 0.55 | use_after_free_gpu: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 113 | gpu/unified_memory.cpp | 13 | gpu_memory_leak | CRITICAL | ? | 0.55 | gpu_memory_leak: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 114 | llm/lora_framework/kernels/hip_fused_kernels.cpp | 222 | missing_sync_threads | CRITICAL | ? | 0.55 | missing_sync_threads: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 115 | sharding/metadata_shard.cpp | 149 | unspecified_consistency | HIGH | ? | 0.55 | unspecified_consistency: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 116 | replication/replication_manager.cpp | 3425 | missing_version_tracking | CRITICAL | ? | 0.55 | missing_version_tracking: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 117 | replication/conflict_resolution.cpp | 597 | undefined_conflict_resolution | HIGH | ? | 0.55 | undefined_conflict_resolution: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 118 | replication/replication_manager.cpp | 2958 | missing_consensus | CRITICAL | ? | 0.55 | missing_consensus: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 119 | sharding/paxos_consensus.cpp | 491 | stale_read_undocumented | MEDIUM | ? | 0.55 | stale_read_undocumented: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
| 120 | server/llm_api_handler.cpp | 898 | model_integrity_gap | CRITICAL | ? | 0.55 | model_integrity_gap: keine robuste Kategoriezuordnung möglich; manuelle Prüfung nötig. |
