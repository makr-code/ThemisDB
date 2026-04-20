# src Module Function Usage Map

_Generated: 2026-04-20 10:41:39Z (heuristische statische Analyse)_

Diese Dokumentation zeigt für **jedes Modul in `src/`**:
- API-/Kernfunktionen als Einstiegspunkte,
- externe Nutzungspfade (Call-Sites und Include-Consumer),
- Audit-Linsen für fehlende Verknüpfungen, Bottlenecks, Security, Governance und Performance.

> Hinweis: Best-effort aus statischer Symbolsuche. Für Freigaben zusätzlich Build/Test/Profiling und Laufzeit-Telemetrie nutzen.

## Global Audit Workflow (pro Modul)

1. Missing Links/Nutzung über „Consumer Modules“ und „Symbol References“ prüfen.
2. Bottlenecks über stark konsumierte Module/Symbole priorisieren.
3. Security/Governance mit `SECURITY.md` + `GOVERNANCE.md` gegenprüfen.
4. Performance-Pfade mit `PERFORMANCE_EXPECTATIONS.md`/Benchmarks abgleichen.
5. Findings als Tasks in modulnahe `ROADMAP.md`/`FUTURE_ENHANCEMENTS.md` übernehmen.

## Module `acceleration`

- **Docs:** [README](./acceleration/README.md) · [ARCHITECTURE](./acceleration/ARCHITECTURE.md) · [ROADMAP](./acceleration/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./acceleration/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./acceleration/SECURITY.md)
- **Public Header Count:** 34
- **Implementation File Count:** 26
- **Consumer Modules (Include-basiert):** base, index, llm, plugins, tests:acceleration, tests:geo, tests:test_acceleration.cpp, tests:test_acceleration_coverage.cpp, tests:test_acceleration_dispatch.cpp, tests:test_acceleration_metrics.cpp, tests:test_acceleration_regression.cpp, tests:test_backend_api_stability.cpp

### Symbol References (Funktion -> Nutzung)

- `instance` -> src/analytics/arrow_flight.cpp, src/api/graphql.cpp, src/aql/classify_bridge.cpp, src/aql/llm_aql_handler.cpp
- `probeCapabilities` -> tests/acceleration/test_ai_hardware_dispatcher.cpp
- `bestBackend` -> tests/acceleration/test_ai_hardware_dispatcher.cpp
- `bestOnnxEP` -> tests/acceleration/test_ai_hardware_dispatcher.cpp
- `hasAccelerator` -> tests/acceleration/test_ai_hardware_dispatcher.cpp
- `hasNPU` -> tests/acceleration/test_ai_hardware_dispatcher.cpp
- `runOn` -> tests/acceleration/test_ai_hardware_dispatcher.cpp
- `logCapabilities` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `analytics`

- **Docs:** [README](./analytics/README.md) · [ARCHITECTURE](./analytics/ARCHITECTURE.md) · [ROADMAP](./analytics/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./analytics/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./analytics/SECURITY.md)
- **Public Header Count:** 25
- **Implementation File Count:** 21
- **Consumer Modules (Include-basiert):** aql, query, server, tests:analytics, tests:graph, tests:integration, tests:test_behoerden_genehmigungsverfahren_e2e.cpp, tests:test_bimschv_genehmigungsverfahren_e2e.cpp, tests:test_branch_conflict_resolution.cpp, tests:test_cross_module_cache_anomaly.cpp, tests:test_cross_module_index_matryoshka.cpp, tests:test_cross_module_timeseries_forecasting.cpp

### Symbol References (Funktion -> Nutzung)

- `exportToFile` -> src/prompt_engineering/prompt_library_io.cpp, src/security/security_evidence_collector.cpp, src/storage/storage_parquet_exporter.cpp, tests/analytics/test_arrow_export.cpp
- `exportToString` -> tests/analytics/test_arrow_export.cpp
- `exportWithCallback` -> tests/analytics/test_arrow_export.cpp
- `supportsFormat` -> tests/analytics/test_arrow_export.cpp
- `getExporterInfo` -> tests/analytics/test_arrow_export.cpp
- `createExporter` -> src/exporters/export_format_registry.cpp, tests/analytics/test_arrow_export.cpp, tests/exporters/test_export_format_registry.cpp
- `createDefaultExporter` -> tests/analytics/test_arrow_export.cpp
- `numericFeatures` -> tests/analytics/test_anomaly_detection.cpp, tests/test_cross_module_cache_anomaly.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `api`

- **Docs:** [README](./api/README.md) · [ARCHITECTURE](./api/ARCHITECTURE.md) · [ROADMAP](./api/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./api/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./api/SECURITY.md)
- **Public Header Count:** 25
- **Implementation File Count:** 10
- **Consumer Modules (Include-basiert):** observability, server, tests:integration, tests:test_api_grpc_server.cpp, tests:test_api_interfaces.cpp, tests:test_aql_graphql_integration.cpp, tests:test_federation_admin.cpp, tests:test_geo_index_integration.cpp, tests:test_graphql.cpp, tests:test_graphql_cache_security.cpp, tests:test_graphql_error_masking.cpp, tests:test_graphql_introspection.cpp

### Symbol References (Funktion -> Nutzung)

- `hookId` -> _no external call-site detected (or indirect usage)_
- `registerHook` -> _no external call-site detected (or indirect usage)_
- `unregisterHook` -> _no external call-site detected (or indirect usage)_
- `getHooks` -> _no external call-site detected (or indirect usage)_
- `registerVersion` -> src/server/response_transformer.cpp, src/training/incremental_lora_trainer.cpp, src/utils/error_registry.cpp, tests/test_document_store.cpp
- `route` -> src/aql/aql_model_router.cpp, src/llm/inference_engine_enhanced.cpp, src/llm/model_router.cpp, src/server/smart_routing.cpp
- `registeredVersions` -> src/server/response_transformer.cpp, tests/test_document_store.cpp, tests/test_response_transformer.cpp
- `lock` -> src/acceleration/ai_hardware_dispatcher.cpp, src/acceleration/backend_registry.cpp, src/acceleration/cuda_backend.cpp, src/acceleration/device_manager.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `aql`

- **Docs:** [README](./aql/README.md) · [ARCHITECTURE](./aql/ARCHITECTURE.md) · [ROADMAP](./aql/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./aql/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./aql/SECURITY.md)
- **Public Header Count:** 27
- **Implementation File Count:** 21
- **Consumer Modules (Include-basiert):** query, server, tests:test_accurate_token_count_estimation.cpp, tests:test_aql_agent.cpp, tests:test_aql_api_stability.cpp, tests:test_aql_async_backend.cpp, tests:test_aql_autocomplete.cpp, tests:test_aql_confidence_scorer.cpp, tests:test_aql_conversation_context.cpp, tests:test_aql_fewshot_example_library.cpp, tests:test_aql_lora_finetuner.cpp, tests:test_aql_migration_assistant.cpp

### Symbol References (Funktion -> Nutzung)

- `registerTool` -> src/llm/ai_orchestrator.cpp, src/llm/mcp_tool_bridge.cpp, src/server/mcp_server.cpp, tests/llm/test_ai_orchestrator.cpp
- `removeTool` -> tests/test_aql_agent.cpp
- `getTools` -> tests/test_aql_agent.cpp
- `hasTool` -> tests/test_aql_agent.cpp
- `ReActAgent` -> _no external call-site detected (or indirect usage)_
- `setConfig` -> src/acceleration/hip_backend.cpp, src/acceleration/vllm_resource_manager.cpp, src/cdc/changefeed_buffer.cpp, src/content/content_manager.cpp
- `getConfig` -> src/acceleration/hip_backend.cpp, src/content/content_manager.cpp, src/content/content_security.cpp, src/content/content_validator.cpp
- `complete` -> src/auth/webauthn_authenticator.cpp, src/index/binary_quantizer.cpp, src/index/product_quantizer.cpp, src/llm/multi_gpu_memory_coordinator.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `auth`

- **Docs:** [README](./auth/README.md) · [ARCHITECTURE](./auth/ARCHITECTURE.md) · [ROADMAP](./auth/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./auth/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./auth/SECURITY.md)
- **Public Header Count:** 37
- **Implementation File Count:** 31
- **Consumer Modules (Include-basiert):** security, server, tests:security, tests:test_api_key_authenticator.cpp, tests:test_auth_anomaly_detection.cpp, tests:test_auth_audit_logger.cpp, tests:test_auth_error.cpp, tests:test_auth_input_validation.cpp, tests:test_auth_metrics.cpp, tests:test_auth_middleware.cpp, tests:test_auth_rate_limiter.cpp, tests:test_auth_rate_limiter_distributed.cpp

### Symbol References (Funktion -> Nutzung)

- `ApiKeyAuthenticator` -> _no external call-site detected (or indirect usage)_
- `addCredential` -> src/server/auth_middleware.cpp, tests/test_api_key_authenticator.cpp, tests/test_auth_audit_logger.cpp
- `removeCredential` -> src/server/auth_middleware.cpp, tests/test_api_key_authenticator.cpp
- `credentialCount` -> tests/test_api_key_authenticator.cpp
- `authenticate` -> src/security/access_control.cpp, src/security/access_control_manager.cpp, src/server/auth_middleware.cpp, src/voice/voice_assistant.cpp
- `authenticateCombined` -> src/server/auth_middleware.cpp, tests/test_api_key_authenticator.cpp
- `hashSecret` -> tests/test_api_key_authenticator.cpp
- `constantTimeEqual` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `base`

- **Docs:** [README](./base/README.md) · [ARCHITECTURE](./base/ARCHITECTURE.md) · [ROADMAP](./base/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./base/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./base/SECURITY.md)
- **Public Header Count:** 0
- **Implementation File Count:** 8
- **Consumer Modules (Include-basiert):** none detected

### Symbol References (Funktion -> Nutzung)

- `statusToString` -> src/observability/alertmanager.cpp, src/prompt_engineering/prompt_ab_experiment.cpp, src/server/monitoring_api_handler.cpp, src/server/shard_repair_api_handler.cpp
- `statusFromString` -> _no external call-site detected (or indirect usage)_
- `configToJson` -> _no external call-site detected (or indirect usage)_
- `configFromJson` -> _no external call-site detected (or indirect usage)_
- `Impl` -> src/acceleration/multi_gpu_backend.cpp, src/acceleration/nccl_vector_backend.cpp, src/acceleration/rccl_vector_backend.cpp, src/analytics/jit_aggregation.cpp
- `verifyModule` -> src/themis/module_hash_verifier.cpp, src/themis/module_loader.cpp, src/themis/module_security.cpp, tests/test_module_hash_verifier.cpp
- `calculateFileHash` -> src/acceleration/plugin_loader.cpp, src/acceleration/plugin_security.cpp, src/plugins/plugin_manager.cpp, src/themis/module_loader.cpp
- `setRequireSignature` -> src/themis/module_loader.cpp, src/themis/module_security.cpp, tests/test_module_loader.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `cache`

- **Docs:** [README](./cache/README.md) · [ARCHITECTURE](./cache/ARCHITECTURE.md) · [ROADMAP](./cache/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./cache/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./cache/SECURITY.md)
- **Public Header Count:** 26
- **Implementation File Count:** 12
- **Consumer Modules (Include-basiert):** llm, server, tests:test_adaptive_cache_fuzz.cpp, tests:test_adaptive_cache_integration.cpp, tests:test_adaptive_cache_phase1.cpp, tests:test_adaptive_query_cache.cpp, tests:test_aligned_vector_cache.cpp, tests:test_arc_cache.cpp, tests:test_bounded_lru_cache.cpp, tests:test_cache_admin_api_handler.cpp, tests:test_cache_hit_rate_slo_monitor.cpp, tests:test_cache_interfaces.cpp

### Symbol References (Funktion -> Nutzung)

- `validate` -> src/api/ws_handler.cpp, src/aql/aql_query_builder.cpp, src/aql/aql_query_validator.cpp, src/aql/llm_aql_handler.cpp
- `AdaptiveQueryCache` -> _no external call-site detected (or indirect usage)_
- `invalidate` -> src/acceleration/vec_knn.cpp, src/analytics/jit_aggregation.cpp, src/core/concerns/redis_cache.cpp, src/gpu/graph_cache.cpp
- `clearExpired` -> src/query/query_cache.cpp, tests/test_adaptive_cache_fuzz.cpp, tests/test_embedding_cache.cpp, tests/test_query_cache.cpp
- `getStats` -> src/acceleration/vllm_resource_manager.cpp, src/analytics/anomaly_detection.cpp, src/analytics/cep_engine.cpp, src/analytics/streaming_window.cpp
- `getDetailedInfo` -> src/query/query_cache.cpp, src/query/query_cache_manager.cpp, src/server/cache_admin_api_handler.cpp, tests/test_adaptive_cache_phase1.cpp
- `getStatsByTier` -> tests/test_adaptive_cache_phase1.cpp, tests/test_adaptive_query_cache.cpp
- `getHealthStatus` -> src/acceleration/graphics_backends.cpp, src/failover/auto_failover_manager.cpp, src/llm/gpu_safe_fail.cpp, src/llm/llm_plugin_manager.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `cdc`

- **Docs:** [README](./cdc/README.md) · [ARCHITECTURE](./cdc/ARCHITECTURE.md) · [ROADMAP](./cdc/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./cdc/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./cdc/SECURITY.md)
- **Public Header Count:** 27
- **Implementation File Count:** 13
- **Consumer Modules (Include-basiert):** metadata, scheduler, server, storage, tests:graph, tests:test_branch_conflict_resolution.cpp, tests:test_branch_integration.cpp, tests:test_branch_manager.cpp, tests:test_cdc_admin.cpp, tests:test_cdc_backpressure_signal.cpp, tests:test_cdc_batch_commit_coordinator.cpp, tests:test_cdc_change_stream_compressor.cpp

### Symbol References (Funktion -> Nutzung)

- `CDCAdmin` -> _no external call-site detected (or indirect usage)_
- `purgeAll` -> tests/test_cdc_admin.cpp
- `purgeBySequenceRange` -> tests/test_cdc_admin.cpp
- `purgeByTimestamp` -> tests/test_cdc_admin.cpp
- `purgeOlderThan` -> tests/test_cdc_admin.cpp
- `purgeTenant` -> _no external call-site detected (or indirect usage)_
- `compactLog` -> src/server/changefeed_api_handler.cpp, tests/test_cdc_admin.cpp, tests/test_sharding_interfaces.cpp
- `redactByKeyPrefix` -> src/server/changefeed_api_handler.cpp, tests/test_cdc_gdpr_redaction.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `chaos`

- **Docs:** [README](./chaos/README.md) · [ARCHITECTURE](./chaos/ARCHITECTURE.md) · [ROADMAP](./chaos/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./chaos/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./chaos/SECURITY.md)
- **Public Header Count:** 1
- **Implementation File Count:** 1
- **Consumer Modules (Include-basiert):** tests:test_chaos_framework.cpp, tests:test_chaos_stress.cpp

### Symbol References (Funktion -> Nutzung)

- `FaultInjector` -> _no external call-site detected (or indirect usage)_
- `injectFault` -> tests/test_chaos_framework.cpp, tests/test_chaos_stress.cpp
- `recoverFault` -> tests/test_chaos_framework.cpp, tests/test_chaos_stress.cpp
- `isFaultActive` -> tests/test_chaos_framework.cpp, tests/test_chaos_stress.cpp
- `getActiveFaults` -> tests/test_chaos_framework.cpp
- `activeFaultCount` -> tests/test_chaos_framework.cpp, tests/test_chaos_stress.cpp
- `clearAllFaults` -> tests/test_chaos_framework.cpp
- `registerEventCallback` -> src/failover/auto_failover_manager.cpp, src/llm/lora_framework/lora_orchestrator.cpp, src/observability/ebpf_tracer.cpp, src/plugins/plugin_health_monitor.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `chimera`

- **Docs:** [README](./chimera/README.md) · [ARCHITECTURE](./chimera/ARCHITECTURE.md) · [ROADMAP](./chimera/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./chimera/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./chimera/SECURITY.md)
- **Public Header Count:** 2
- **Implementation File Count:** 1
- **Consumer Modules (Include-basiert):** tests:chimera

### Symbol References (Funktion -> Nutzung)

- `has_more` -> tests/chimera/test_chimera_streaming.cpp
- `next_batch` -> tests/chimera/test_chimera_streaming.cpp
- `position` -> src/analytics/nlp_text_analyzer.cpp, src/auth/auth_error.cpp, src/content/html_processor.cpp, src/exporters/pii_detector.cpp
- `total_size` -> src/utils/memory/pool_allocator.cpp, tests/chimera/test_chimera_streaming.cpp
- `set_stream_config` -> tests/chimera/test_chimera_streaming.cpp
- `get_id` -> tests/chimera/test_chimera_prepared_statements.cpp, tests/test_cdc_gdpr_redaction.cpp, tests/test_cdc_operation_filter.cpp, tests/test_index_recommender.cpp
- `get_query` -> tests/chimera/test_chimera_prepared_statements.cpp
- `bind` -> src/network/kernel_bypass.cpp, src/network/service_mesh.cpp, src/network/wire_protocol_server.cpp, src/network/wire_protocol_v2.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `config`

- **Docs:** [README](./config/README.md) · [ARCHITECTURE](./config/ARCHITECTURE.md) · [ROADMAP](./config/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./config/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./config/SECURITY.md)
- **Public Header Count:** 10
- **Implementation File Count:** 6
- **Consumer Modules (Include-basiert):** content, index, main_server.cpp, server, tests:test_config_coverage.cpp, tests:test_config_encrypted_store.cpp, tests:test_config_file_watcher.cpp, tests:test_config_metrics_scrape.cpp, tests:test_config_migration_scanner.cpp, tests:test_config_path_resolver.cpp, tests:test_config_schema_validator.cpp, tests:test_lru_cache.cpp

### Symbol References (Funktion -> Nutzung)

- `enable` -> src/gpu/feature_flags.cpp, src/observability/continuous_profiler.cpp, src/observability/ebpf_tracer.cpp, src/observability/query_profiler.cpp
- `disable` -> src/gpu/feature_flags.cpp, src/observability/continuous_profiler.cpp, src/observability/ebpf_tracer.cpp, src/observability/query_profiler.cpp
- `isEnabled` -> src/content/content_manager.cpp, src/content/content_manager_embedding.cpp, src/content/embedding_pipeline.cpp, src/core/concerns/concerns_context.cpp
- `setMaxEntries` -> tests/test_config_coverage.cpp
- `maxEntries` -> tests/test_config_coverage.cpp
- `record` -> src/analytics/streaming_window.cpp, src/cache/cache_hit_rate_slo_monitor.cpp, src/geo/gpu_backend_stub.cpp, src/gpu/audit_log.cpp
- `getEntries` -> src/sharding/raft_consensus_adapter.cpp, src/sharding/raft_log.cpp, tests/test_config_coverage.cpp
- `toJson` -> src/analytics/diff_engine.cpp, src/api/geo_index_hooks.cpp, src/aql/aql_lora_finetuner.cpp, src/cache/cache_replication.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `content`

- **Docs:** [README](./content/README.md) · [ARCHITECTURE](./content/ARCHITECTURE.md) · [ROADMAP](./content/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./content/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./content/SECURITY.md)
- **Public Header Count:** 42
- **Implementation File Count:** 46
- **Consumer Modules (Include-basiert):** plugins, server, tests:integration, tests:test_archive_processor.cpp, tests:test_async_ingestion_backpressure.cpp, tests:test_async_ingestion_yaml_config.cpp, tests:test_content_audio_processor.cpp, tests:test_content_deduplication.cpp, tests:test_content_embedding_pipeline.cpp, tests:test_content_errors.cpp, tests:test_content_features.cpp, tests:test_content_fs.cpp

### Symbol References (Funktion -> Nutzung)

- `detect` -> src/gpu/cluster_coordinator.cpp, src/gpu/cluster_topology.cpp, src/gpu/p2p_transfer.cpp, src/graph/graph_watermark.cpp
- `detectorType` -> _no external call-site detected (or indirect usage)_
- `PhotoDNAAbuseDetector` -> _no external call-site detected (or indirect usage)_
- `computeHash` -> src/llm/lora_router.cpp, src/security/timestamp_authority.cpp, src/security/timestamp_authority_openssl.cpp, src/voice/voice_audio_storage.cpp
- `hammingDistance` -> src/index/binary_quantizer.cpp, tests/test_binary_quantizer.cpp, tests/test_content_security.cpp
- `TextAbuseDetector` -> _no external call-site detected (or indirect usage)_
- `loadFromYAML` -> src/governance/policy_engine.cpp, src/llm/prompt_manager.cpp, src/prompt_engineering/prompt_manager.cpp, src/rag/continuous_learning_orchestrator.cpp
- `createPdfExtractorAdapter` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `core`

- **Docs:** [README](./core/README.md) · [ARCHITECTURE](./core/ARCHITECTURE.md) · [ROADMAP](./core/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./core/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./core/SECURITY.md)
- **Public Header Count:** 38
- **Implementation File Count:** 10
- **Consumer Modules (Include-basiert):** observability, security, tests:test_aql_explain.cpp, tests:test_circuit_breaker.cpp, tests:test_concerns_context.cpp, tests:test_context_propagation.cpp, tests:test_distributed_cache_integration.cpp, tests:test_eviction_strategies.cpp, tests:test_feature_flags.cpp, tests:test_graceful_shutdown.cpp, tests:test_health_checks.cpp, tests:test_jaeger_tracer_adapter.cpp

### Symbol References (Funktion -> Nutzung)

- `onAccess` -> src/cache/adaptive_query_cache.cpp, tests/test_cache_interfaces.cpp, tests/test_eviction_strategies.cpp
- `onInsert` -> src/cache/adaptive_query_cache.cpp, src/cache/warmup.cpp, src/query/materialized_view.cpp, tests/test_cache_interfaces.cpp
- `onRemove` -> src/cache/adaptive_query_cache.cpp, tests/test_cache_interfaces.cpp, tests/test_eviction_strategies.cpp
- `selectVictim` -> src/cache/adaptive_query_cache.cpp, tests/test_eviction_strategies.cpp
- `getName` -> src/cache/adaptive_query_cache.cpp, src/ethics_ai/ethics_ai_plugin.cpp, src/index/index_manager.cpp, src/ingestion/steps/base_entity_assembler_step.cpp
- `createCustom` -> tests/test_circuit_breaker.cpp, tests/test_concerns_context.cpp, tests/test_distributed_cache_integration.cpp, tests/test_feature_flags.cpp
- `createNoOp` -> tests/test_circuit_breaker.cpp, tests/test_concerns_context.cpp, tests/test_feature_flags.cpp, tests/test_graceful_shutdown.cpp
- `healthCheck` -> src/cdc/cdc_admin.cpp, src/content/audio_processor.cpp, src/content/cad_processor.cpp, src/content/geo_processor.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `distributed_knowledge`

- **Docs:** [README](./distributed_knowledge/README.md) · [ARCHITECTURE](./distributed_knowledge/ARCHITECTURE.md) · [ROADMAP](./distributed_knowledge/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./distributed_knowledge/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./distributed_knowledge/SECURITY.md)
- **Public Header Count:** 4
- **Implementation File Count:** 4
- **Consumer Modules (Include-basiert):** prompt_engineering, rag, sharding, tests:test_clo_loops.cpp, tests:test_continuous_learning_orchestrator.cpp, tests:test_decision_record_e2e.cpp, tests:test_distributed_knowledge.cpp, tests:test_distributed_knowledge_integration.cpp, tests:test_distributed_knowledge_or.cpp, tests:test_federation_admin.cpp, tests:test_feedback_collector.cpp, tests:test_incremental_lora_trainer.cpp

### Symbol References (Funktion -> Nutzung)

- `GossipAdapterPublisher` -> _no external call-site detected (or indirect usage)_
- `announce` -> tests/test_distributed_knowledge.cpp
- `handleInboundMessage` -> tests/test_distributed_knowledge.cpp
- `setAnnouncementCallback` -> tests/test_distributed_knowledge.cpp
- `erase` -> src/acceleration/cuda_backend.cpp, src/acceleration/plugin_loader.cpp, src/acceleration/vec_knn.cpp, src/analytics/anomaly_detection.cpp
- `CrossShardFeedbackSync` -> tests/test_distributed_knowledge.cpp, tests/test_distributed_knowledge_or.cpp
- `publishFeedback` -> src/prompt_engineering/feedback_collector.cpp, tests/test_distributed_knowledge.cpp, tests/test_distributed_knowledge_or.cpp, tests/test_feedback_collector.cpp
- `handleInboundSummary` -> tests/test_distributed_knowledge.cpp, tests/test_distributed_knowledge_integration.cpp, tests/test_distributed_knowledge_or.cpp, tests/test_rag_rlaif_trainer.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `ethics_ai`

- **Docs:** [README](./ethics_ai/README.md) · [ARCHITECTURE](./ethics_ai/ARCHITECTURE.md) · [ROADMAP](./ethics_ai/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./ethics_ai/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./ethics_ai/SECURITY.md)
- **Public Header Count:** 0
- **Implementation File Count:** 8
- **Consumer Modules (Include-basiert):** tests:test_argument_store_standalone.cpp, tests:test_discourse_engine.cpp, tests:test_ethics_ai_benchmark.cpp, tests:test_ethics_ai_chain_visualizer.cpp, tests:test_ethics_ai_integration.cpp, tests:test_rag_context_engine.cpp

### Symbol References (Funktion -> Nutzung)

- `EthicsAIPlugin` -> _no external call-site detected (or indirect usage)_
- `createPlugin` -> src/importers/postgres_importer.cpp, src/llama_cpp/llama_cpp_registrar.cpp, src/llama_cpp/tests/test_llama_cpp_plugin.cpp, src/plugins/plugin_manager.cpp
- `destroyPlugin` -> src/importers/postgres_importer.cpp, src/rpc_grpc/grpc_plugin.cpp, tests/test_ethics_ai_plugin.cpp, tests/test_importer_plugin_api.cpp
- `argumentTypeToString` -> tests/test_ethics_ai_types.cpp
- `stringToArgumentType` -> tests/test_ethics_ai_types.cpp
- `argumentStrengthToString` -> tests/test_ethics_ai_types.cpp
- `stringToArgumentStrength` -> tests/test_ethics_ai_types.cpp
- `strengthToScore` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `exporters`

- **Docs:** [README](./exporters/README.md) · [ARCHITECTURE](./exporters/ARCHITECTURE.md) · [ROADMAP](./exporters/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./exporters/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./exporters/SECURITY.md)
- **Public Header Count:** 18
- **Implementation File Count:** 16
- **Consumer Modules (Include-basiert):** server, tests:exporters, tests:test_export_api_handler.cpp

### Symbol References (Funktion -> Nutzung)

- `AqlPredicateFilter` -> _no external call-site detected (or indirect usage)_
- `evaluate` -> src/analytics/cep_engine.cpp, src/analytics/forecasting.cpp, src/cache/cache_hit_rate_slo_monitor.cpp, src/governance/ccpa_rules.cpp
- `isArrowAvailable` -> tests/exporters/test_arrow_ipc_exporter.cpp, tests/exporters/test_parquet_exporter.cpp
- `resolveColumns` -> src/analytics/streaming_join.cpp
- `exportWithArrow` -> _no external call-site detected (or indirect usage)_
- `exportFallback` -> _no external call-site detected (or indirect usage)_
- `augment` -> tests/exporters/test_data_augmentation.cpp
- `applyStrategy` -> src/governance/data_masker.cpp, tests/exporters/test_data_augmentation.cpp, tests/test_data_masker.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `failover`

- **Docs:** [README](./failover/README.md) · [ARCHITECTURE](./failover/ARCHITECTURE.md) · [ROADMAP](./failover/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./failover/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./failover/SECURITY.md)
- **Public Header Count:** 2
- **Implementation File Count:** 2
- **Consumer Modules (Include-basiert):** tests:test_auto_failover_manager.cpp, tests:test_disaster_recovery_manager.cpp, tests:test_failover_chaos_scenarios.cpp

### Symbol References (Funktion -> Nutzung)

- `AutoFailoverManager` -> tests/test_auto_failover_manager.cpp
- `isRunning` -> src/analytics/arrow_flight.cpp, src/api/grpc_server.cpp, src/chaos/chaos_framework.cpp, src/config/config_path_resolver.cpp
- `triggerManualFailover` -> src/sharding/health_monitor.cpp, tests/test_auto_failover_manager.cpp, tests/test_failover_chaos_scenarios.cpp, tests/test_wal_replication_integration.cpp
- `getState` -> src/aql/llm_aql_handler.cpp, src/cache/adaptive_query_cache.cpp, src/importers/gui_import_wizard.cpp, src/network/adaptive_circuit_breaker.cpp
- `isFailoverInProgress` -> tests/test_auto_failover_manager.cpp
- `getFailingNodes` -> tests/test_auto_failover_manager.cpp, tests/test_failover_chaos_scenarios.cpp
- `getLastFailoverResult` -> tests/test_auto_failover_manager.cpp, tests/test_failover_chaos_scenarios.cpp
- `updateConfig` -> src/auth/auth_rate_limiter.cpp, src/index/hnsw_parameter_tuner.cpp, src/llm/distributed_training_coordinator.cpp, src/llm/llm_deployment_plugin.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `geo`

- **Docs:** [README](./geo/README.md) · [ARCHITECTURE](./geo/ARCHITECTURE.md) · [ROADMAP](./geo/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./geo/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./geo/SECURITY.md)
- **Public Header Count:** 17
- **Implementation File Count:** 19
- **Consumer Modules (Include-basiert):** acceleration, query, server, tests:geo, tests:index, tests:test_cpu_backend_exact.cpp, tests:test_cross_module_geo_spatial.cpp, tests:test_geo_gpu_backend.cpp, tests:test_geo_index_integration.cpp, tests:test_hybrid_queries.cpp, tests:test_rpc_geo_query.cpp, tests:test_spatial_index_atomic.cpp

### Symbol References (Funktion -> Nutzung)

- `Detect` -> tests/geo/test_geo_device_detector.cpp
- `BestDevice` -> tests/geo/test_geo_device_detector.cpp
- `HasSuitableDevice` -> tests/geo/test_geo_device_detector.cpp
- `Assess` -> tests/geo/test_geo_device_detector.cpp
- `ReportJson` -> tests/geo/test_geo_device_detector.cpp
- `GeoFaissKnn` -> _no external call-site detected (or indirect usage)_
- `build` -> src/acceleration/opencl_backend.cpp, src/analytics/analytics_export.cpp, src/analytics/olap.cpp, src/analytics/process_mining.cpp
- `knnSearch` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `governance`

- **Docs:** [README](./governance/README.md) · [ARCHITECTURE](./governance/ARCHITECTURE.md) · [ROADMAP](./governance/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./governance/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./governance/SECURITY.md)
- **Public Header Count:** 25
- **Implementation File Count:** 25
- **Consumer Modules (Include-basiert):** exporters, server, tests:exporters, tests:llm, tests:test_ccpa_rules.cpp, tests:test_compliance_reporting.cpp, tests:test_compliance_security_governance.cpp, tests:test_cross_module_graph_lineage.cpp, tests:test_cross_module_security_governance.cpp, tests:test_cross_module_training_governance.cpp, tests:test_cross_tenant_policy_inheritance.cpp, tests:test_data_lineage.cpp

### Symbol References (Funktion -> Nutzung)

- `framework` -> tests/test_ccpa_rules.cpp, tests/test_hipaa_rules.cpp, tests/test_pci_dss_rules.cpp
- `description` -> src/auth/saml_authenticator.cpp, src/content/office_processor.cpp, src/ingestion/ingestion_quality_judge.cpp, src/ingestion/steps/chunk_embed_step.cpp
- `evaluate` -> src/analytics/cep_engine.cpp, src/analytics/forecasting.cpp, src/cache/cache_hit_rate_slo_monitor.cpp, src/exporters/aql_predicate_filter.cpp
- `toJson` -> src/analytics/diff_engine.cpp, src/api/geo_index_hooks.cpp, src/aql/aql_lora_finetuner.cpp, src/cache/cache_replication.cpp
- `CcpaRuleSet` -> _no external call-site detected (or indirect usage)_
- `addOptOut` -> tests/test_ccpa_rules.cpp
- `removeOptOut` -> tests/test_ccpa_rules.cpp
- `isOptedOut` -> tests/test_ccpa_rules.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `gpu`

- **Docs:** [README](./gpu/README.md) · [ARCHITECTURE](./gpu/ARCHITECTURE.md) · [ROADMAP](./gpu/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./gpu/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./gpu/SECURITY.md)
- **Public Header Count:** 0
- **Implementation File Count:** 30
- **Consumer Modules (Include-basiert):** none detected

### Symbol References (Funktion -> Nutzung)

- `interconnectTypeName` -> tests/test_gpu_cluster_topology.cpp
- `MakeCPUFallback` -> _no external call-site detected (or indirect usage)_
- `EnumerateCUDA` -> _no external call-site detected (or indirect usage)_
- `EnumerateROCm` -> _no external call-site detected (or indirect usage)_
- `GetGPUFallbackStrategy` -> tests/test_gpu_memory_management.cpp
- `migStatusName` -> tests/test_gpu_mig_manager.cpp
- `p2pStatusName` -> tests/test_gpu_p2p_transfer.cpp
- `resolveDevices` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `graph`

- **Docs:** [README](./graph/README.md) · [ARCHITECTURE](./graph/ARCHITECTURE.md) · [ROADMAP](./graph/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./graph/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./graph/SECURITY.md)
- **Public Header Count:** 10
- **Implementation File Count:** 8
- **Consumer Modules (Include-basiert):** server, tests:graph, tests:test_gpu_graph_traversal.cpp, tests:test_graph_advanced_features.cpp, tests:test_graph_distributed.cpp, tests:test_graph_parallel_traversal.cpp, tests:test_graph_query_optimizer.cpp, tests:test_graph_query_rewriter.cpp, tests:test_graph_watermarking.cpp, tests:test_q3_module_interfaces.cpp

### Symbol References (Funktion -> Nutzung)

- `shardId` -> tests/test_graph_distributed.cpp
- `executeBFS` -> src/server/graph_api_handler.cpp, tests/test_gpu_graph_traversal.cpp, tests/test_graph_distributed.cpp, tests/test_graph_query_optimizer.cpp
- `executeDijkstra` -> tests/test_graph_distributed.cpp, tests/test_graph_query_optimizer.cpp
- `LocalShardGraphExecutor` -> _no external call-site detected (or indirect usage)_
- `qualify` -> _no external call-site detected (or indirect usage)_
- `addShard` -> src/analytics/distributed_analytics.cpp, src/main_server.cpp, src/rag/explainability_reason_builder.cpp, src/server/geo_topology_api_handler.cpp
- `removeShard` -> src/analytics/distributed_analytics.cpp, src/server/geo_topology_api_handler.cpp, src/sharding/admin_operations.cpp, src/sharding/consistent_hash.cpp
- `shardIds` -> tests/query/test_query_federation_routing.cpp, tests/test_cross_module_query_sharding.cpp, tests/test_graph_distributed.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `importers`

- **Docs:** [README](./importers/README.md) · [ARCHITECTURE](./importers/ARCHITECTURE.md) · [ROADMAP](./importers/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./importers/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./importers/SECURITY.md)
- **Public Header Count:** 37
- **Implementation File Count:** 31
- **Consumer Modules (Include-basiert):** tests:test_behoerden_genehmigungsverfahren_e2e.cpp, tests:test_bimschv_genehmigungsverfahren_e2e.cpp, tests:test_canonical_resolver.cpp, tests:test_cross_module_german_egov.cpp, tests:test_egov_data_driven.cpp, tests:test_entity_linking.cpp, tests:test_entity_matching.cpp, tests:test_importer_interfaces.cpp, tests:test_importer_plugin_api.cpp, tests:test_kafka_importer.cpp, tests:test_mdm_engine.cpp, tests:test_mdm_integration.cpp

### Symbol References (Funktion -> Nutzung)

- `adaptBatchSize` -> tests/test_postgres_importer_v2.cpp
- `topologicalSort` -> src/scheduler/task_scheduler.cpp, src/themis/module_dependency_resolver.cpp, src/transaction/distributed_saga.cpp, src/transaction/saga_orchestrator.cpp
- `eventTypeToString` -> src/analytics/cep_engine.cpp, src/llm/lora_framework/lora_audit_logger.cpp, tests/test_postgres_importer_v2.cpp
- `recordEvent` -> src/cdc/changefeed.cpp, src/cdc/changefeed_buffer.cpp, src/cdc/dead_letter_queue.cpp, src/cdc/outbox.cpp
- `verifyIntegrity` -> src/base/remote_registry_client.cpp, src/llm/ai_decision_auditor.cpp, src/sharding/data_migrator.cpp, src/sharding/stream_protocol.cpp
- `exportForSIEM` -> tests/test_postgres_importer_v2.cpp
- `events` -> src/cdc/changefeed.cpp, src/cdc/changefeed_buffer.cpp, src/governance/iso27001_rules.cpp, src/index/workload_replay.cpp
- `computeEventHash` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `index`

- **Docs:** [README](./index/README.md) · [ARCHITECTURE](./index/ARCHITECTURE.md) · [ROADMAP](./index/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./index/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./index/SECURITY.md)
- **Public Header Count:** 42
- **Implementation File Count:** 41
- **Consumer Modules (Include-basiert):** acceleration, api, aql, cache, graph, llm, main.cpp, main_server.cpp, metadata, network, process, query

### Symbol References (Funktion -> Nutzung)

- `toJson` -> src/analytics/diff_engine.cpp, src/api/geo_index_hooks.cpp, src/aql/aql_lora_finetuner.cpp, src/cache/cache_replication.cpp
- `fromJson` -> src/analytics/diff_engine.cpp, src/cache/semantic_cache.cpp, src/cdc/changefeed.cpp, src/cdc/consumer_group.cpp
- `QueryPatternTracker` -> _no external call-site detected (or indirect usage)_
- `recordPattern` -> src/server/http_server.cpp, src/server/index_api_handler.cpp, tests/test_adaptive_index.cpp
- `getPatterns` -> src/server/http_server.cpp, src/server/index_api_handler.cpp, src/transaction/deadlock_predictor.cpp, tests/test_adaptive_deadlock_prevention.cpp
- `getTopPatterns` -> tests/test_adaptive_index.cpp
- `makeKey` -> src/acceleration/vec_knn.cpp, src/analytics/streaming_join.cpp, src/auth/rate_limiter_backend.cpp, src/auth/redis_token_blacklist.cpp
- `getCurrentTimeMs` -> src/cache/adaptive_query_cache.cpp, src/cache/warmup.cpp, src/content/async_ingestion_worker.cpp, src/governance/policy_review.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `ingestion`

- **Docs:** [README](./ingestion/README.md) · [ARCHITECTURE](./ingestion/ARCHITECTURE.md) · [ROADMAP](./ingestion/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./ingestion/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./ingestion/SECURITY.md)
- **Public Header Count:** 29
- **Implementation File Count:** 32
- **Consumer Modules (Include-basiert):** rag, tests:test_content_toolbox_bridge.cpp, tests:test_ingestion_assembler_sinks.cpp, tests:test_ingestion_base_entity.cpp, tests:test_ingestion_builder.cpp, tests:test_ingestion_builtin_steps_v14.cpp, tests:test_ingestion_cdc.cpp, tests:test_ingestion_checkpoint.cpp, tests:test_ingestion_coordinator.cpp, tests:test_ingestion_database.cpp, tests:test_ingestion_errors.cpp, tests:test_ingestion_features.cpp

### Symbol References (Funktion -> Nutzung)

- `AgenticReferenceValidator` -> _no external call-site detected (or indirect usage)_
- `validate` -> src/api/ws_handler.cpp, src/aql/aql_query_builder.cpp, src/aql/aql_query_validator.cpp, src/aql/llm_aql_handler.cpp
- `extract` -> src/content/adapters/archive_extractor_adapter.cpp, src/content/adapters/audio_extractor_adapter.cpp, src/content/adapters/image_extractor_adapter.cpp, src/content/adapters/office_extractor_adapter.cpp
- `addKnownLaw` -> tests/test_legal_extraction.cpp
- `addKnownSection` -> tests/test_legal_extraction.cpp
- `clearKnowledgeBase` -> tests/test_legal_extraction.cpp
- `knownLawCount` -> tests/test_legal_extraction.cpp
- `setExtractorFn` -> tests/test_ingestion_llm_adapter.cpp, tests/test_legal_extraction.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `llama_cpp`

- **Docs:** [README](./llama_cpp/README.md) · [ARCHITECTURE](./llama_cpp/ARCHITECTURE.md) · [ROADMAP](./llama_cpp/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./llama_cpp/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./llama_cpp/SECURITY.md)
- **Public Header Count:** 2
- **Implementation File Count:** 3
- **Consumer Modules (Include-basiert):** none detected

### Symbol References (Funktion -> Nutzung)

- `LlamaCppPlugin` -> _no external call-site detected (or indirect usage)_
- `generateStream` -> src/llm/llm_plugin_manager.cpp, src/server/llm_grpc_service.cpp
- `generateBatch` -> src/stable_diffusion/sd_plugin.cpp, src/stable_diffusion/tests/test_sd_plugin.cpp
- `getModelId` -> src/stable_diffusion/sd_plugin.cpp, src/stable_diffusion/tests/test_sd_plugin.cpp, src/whisper/tests/test_whisper_plugin.cpp, src/whisper/whisper_plugin.cpp
- `defaultReloadCallback` -> src/stable_diffusion/sd_plugin_registrar.cpp, src/stable_diffusion/tests/test_sd_plugin_registrar.cpp, src/whisper/tests/test_whisper_plugin_registrar.cpp, src/whisper/whisper_plugin_registrar.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `llm`

- **Docs:** [README](./llm/README.md) · [ARCHITECTURE](./llm/ARCHITECTURE.md) · [ROADMAP](./llm/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./llm/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./llm/SECURITY.md)
- **Public Header Count:** 174
- **Implementation File Count:** 151
- **Consumer Modules (Include-basiert):** aql, content, index, llama_cpp, main_server.cpp, query, rag, server, tests:byzantine_attacks.h, tests:integration, tests:llm, tests:stubs

### Symbol References (Funktion -> Nutzung)

- `ActiveVRAMAllocator` -> _no external call-site detected (or indirect usage)_
- `allocate` -> src/gpu/gpu_module.cpp, src/gpu/rocm_backend.cpp, src/gpu/unified_memory.cpp, src/index/gpu_memory_oversubscription.cpp
- `allocateOrRecover` -> tests/llm/test_active_vram_allocator.cpp
- `free` -> src/acceleration/oneapi_backend.cpp, src/gpu/unified_memory.cpp, src/graph/gpu_traversal.cpp, src/index/gpu_memory_oversubscription.cpp
- `touch` -> tests/llm/test_active_vram_allocator.cpp, tests/test_llm_caching.cpp, tests/test_llm_prefix_cache.cpp
- `handleOOM` -> tests/llm/test_active_vram_allocator.cpp
- `evictLRU` -> src/acceleration/cuda_backend.cpp, src/cache/adaptive_query_cache.cpp, src/cache/warmup.cpp, src/gpu/graph_cache.cpp
- `evictOwner` -> tests/llm/test_active_vram_allocator.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `maintenance`

- **Docs:** [README](./maintenance/README.md) · [ARCHITECTURE](./maintenance/ARCHITECTURE.md) · [ROADMAP](./maintenance/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./maintenance/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./maintenance/SECURITY.md)
- **Public Header Count:** 8
- **Implementation File Count:** 3
- **Consumer Modules (Include-basiert):** server, tests:test_database_maintenance_orchestrator.cpp, tests:test_sharding_repair.cpp

### Symbol References (Funktion -> Nutzung)

- `DatabaseMaintenanceOrchestrator` -> _no external call-site detected (or indirect usage)_
- `isRunning` -> src/analytics/arrow_flight.cpp, src/api/grpc_server.cpp, src/chaos/chaos_framework.cpp, src/config/config_path_resolver.cpp
- `createSchedule` -> src/server/http_server.cpp, src/server/maintenance_api_handler.cpp, tests/test_database_maintenance_orchestrator.cpp
- `getSchedule` -> src/governance/policy_review.cpp, src/server/http_server.cpp, src/server/maintenance_api_handler.cpp, tests/test_database_maintenance_orchestrator.cpp
- `listSchedules` -> src/server/http_server.cpp, src/server/maintenance_api_handler.cpp, tests/test_database_maintenance_orchestrator.cpp
- `updateSchedule` -> src/server/http_server.cpp, src/server/maintenance_api_handler.cpp, tests/test_database_maintenance_orchestrator.cpp
- `patchSchedule` -> src/server/http_server.cpp, src/server/maintenance_api_handler.cpp, tests/test_database_maintenance_orchestrator.cpp
- `deleteSchedule` -> src/server/http_server.cpp, src/server/maintenance_api_handler.cpp, tests/test_database_maintenance_orchestrator.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `metadata`

- **Docs:** [README](./metadata/README.md) · [ARCHITECTURE](./metadata/ARCHITECTURE.md) · [ROADMAP](./metadata/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./metadata/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./metadata/SECURITY.md)
- **Public Header Count:** 19
- **Implementation File Count:** 12
- **Consumer Modules (Include-basiert):** llm, prompt_engineering, query, server, tests:test_catalog_exporter.cpp, tests:test_column_lineage.cpp, tests:test_distributed_catalog.cpp, tests:test_er_diagram_exporter.cpp, tests:test_in_place_schema_migrator.cpp, tests:test_index_recommender.cpp, tests:test_index_workload_replay.cpp, tests:test_information_schema.cpp

### Symbol References (Funktion -> Nutzung)

- `CatalogExporter` -> _no external call-site detected (or indirect usage)_
- `publishSchema` -> tests/test_catalog_exporter.cpp, tests/test_distributed_catalog.cpp
- `publishTable` -> tests/test_catalog_exporter.cpp
- `setHttpPostForTesting` -> src/auth/federated_identity_manager.cpp, src/auth/oauth_device_flow.cpp, src/auth/oauth_pkce_flow.cpp, src/ingestion/api_connector.cpp
- `buildAtlasPayload` -> _no external call-site detected (or indirect usage)_
- `sendToAtlas` -> _no external call-site detected (or indirect usage)_
- `buildDataHubProposals` -> _no external call-site detected (or indirect usage)_
- `sendToDataHub` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `network`

- **Docs:** [README](./network/README.md) · [ARCHITECTURE](./network/ARCHITECTURE.md) · [ROADMAP](./network/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./network/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./network/SECURITY.md)
- **Public Header Count:** 24
- **Implementation File Count:** 24
- **Consumer Modules (Include-basiert):** server, tests:performance, tests:test_bandwidth_management_qos.cpp, tests:test_bpmn_wire_protocol.cpp, tests:test_connection_compression.cpp, tests:test_envoy_xds.cpp, tests:test_fuzz_core.cpp, tests:test_geo_topology_router.cpp, tests:test_grpc_transport.cpp, tests:test_http3_datagram.cpp, tests:test_io_uring_batcher.cpp, tests:test_kernel_bypass.cpp

### Symbol References (Funktion -> Nutzung)

- `AdaptiveCircuitBreaker` -> _no external call-site detected (or indirect usage)_
- `shouldAllow` -> tests/test_network_circuit_breaker.cpp
- `recordSuccess` -> src/aql/llm_aql_handler.cpp, src/cache/adaptive_query_cache.cpp, src/core/concerns/lockfree_metrics.cpp, src/geo/gpu_backend_stub.cpp
- `recordFailure` -> src/aql/llm_aql_handler.cpp, src/base/module_loader.cpp, src/cache/adaptive_query_cache.cpp, src/geo/gpu_backend_stub.cpp
- `getState` -> src/aql/llm_aql_handler.cpp, src/cache/adaptive_query_cache.cpp, src/failover/auto_failover_manager.cpp, src/failover/disaster_recovery_manager.cpp
- `getStats` -> src/acceleration/vllm_resource_manager.cpp, src/analytics/anomaly_detection.cpp, src/analytics/cep_engine.cpp, src/analytics/streaming_window.cpp
- `forceOpen` -> src/sharding/circuit_breaker.cpp, tests/test_circuit_breaker.cpp, tests/test_concerns_context.cpp, tests/test_network_circuit_breaker.cpp
- `setStateChangeCallback` -> tests/test_network_circuit_breaker.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `observability`

- **Docs:** [README](./observability/README.md) · [ARCHITECTURE](./observability/ARCHITECTURE.md) · [ROADMAP](./observability/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./observability/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./observability/SECURITY.md)
- **Public Header Count:** 22
- **Implementation File Count:** 21
- **Consumer Modules (Include-basiert):** base, cache, config, core, governance, maintenance, metadata, query, server, tests:integration, tests:test_ab_test_manager.cpp, tests:test_alert_rules.cpp

### Symbol References (Funktion -> Nutzung)

- `recordSummary` -> tests/test_custom_metric_types.cpp
- `recordExponentialHistogram` -> tests/test_custom_metric_types.cpp
- `getExponentialHistogram` -> tests/test_custom_metric_types.cpp
- `recordCardinality` -> tests/test_custom_metric_types.cpp
- `getCardinalityEstimate` -> tests/test_custom_metric_types.cpp
- `recordTimeWeightedAverage` -> tests/test_custom_metric_types.cpp
- `getTimeWeightedAverage` -> tests/test_custom_metric_types.cpp
- `recordRate` -> tests/test_custom_metric_types.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `onnx_clip`

- **Docs:** [README](./onnx_clip/README.md) · [ARCHITECTURE](./onnx_clip/ARCHITECTURE.md) · [ROADMAP](./onnx_clip/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./onnx_clip/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./onnx_clip/SECURITY.md)
- **Public Header Count:** 0
- **Implementation File Count:** 1
- **Consumer Modules (Include-basiert):** tests:test_onnx_clip_plugin.cpp

### Symbol References (Funktion -> Nutzung)

- `backendToString` -> src/llm/lora_framework/lora_storage_service.cpp, src/llm/lora_framework/lora_storage_service_themisdb.cpp
- `sha256HexOfFile` -> _no external call-site detected (or indirect usage)_
- `fnv1a64` -> src/analytics/forecasting.cpp, src/importers/mdm_audit_trail.cpp, src/importers/postgres_importer.cpp, src/index/index_compression.cpp
- `fnv1a64_str` -> _no external call-site detected (or indirect usage)_
- `mixMetadata` -> _no external call-site detected (or indirect usage)_
- `nextFloat01` -> _no external call-site detected (or indirect usage)_
- `tokenize` -> src/analytics/cep_engine.cpp, src/analytics/nlp_text_analyzer.cpp, src/aql/aql_fewshot_example_library.cpp, src/aql/aql_syntax_highlighter.cpp
- `computeEmbedding` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `performance`

- **Docs:** [README](./performance/README.md) · [ARCHITECTURE](./performance/ARCHITECTURE.md) · [ROADMAP](./performance/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./performance/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./performance/SECURITY.md)
- **Public Header Count:** 43
- **Implementation File Count:** 31
- **Consumer Modules (Include-basiert):** llm, query, storage, tests:performance, tests:test_adaptive_query_compilation.cpp, tests:test_advanced_cache_manager.cpp, tests:test_aligned_vector_cache.cpp, tests:test_alignment_helpers.cpp, tests:test_huge_pages.cpp, tests:test_intelligent_prefetcher.cpp, tests:test_lirs_cache.cpp, tests:test_lockfree_histogram.cpp

### Symbol References (Funktion -> Nutzung)

- `AdaptiveQueryCompiler` -> _no external call-site detected (or indirect usage)_
- `compile` -> src/llm/grammar.cpp, src/prompt_engineering/prompt_template_compiler.cpp, src/query/aql_runner.cpp, src/query/query_compiler.cpp
- `is_compilable` -> tests/test_adaptive_query_compilation.cpp
- `invalidate` -> src/acceleration/vec_knn.cpp, src/analytics/jit_aggregation.cpp, src/cache/adaptive_query_cache.cpp, src/cache/cache_replication_coordinator.cpp
- `invalidateAll` -> src/analytics/jit_aggregation.cpp, src/query/query_compiler.cpp, tests/analytics/test_jit_aggregation.cpp, tests/test_adaptive_query_compilation.cpp
- `executionCount` -> tests/test_adaptive_query_compilation.cpp
- `isCompiled` -> src/analytics/jit_aggregation.cpp, src/query/query_compiler.cpp, tests/analytics/test_jit_aggregation.cpp, tests/test_adaptive_query_compilation.cpp
- `getStats` -> src/acceleration/vllm_resource_manager.cpp, src/analytics/anomaly_detection.cpp, src/analytics/cep_engine.cpp, src/analytics/streaming_window.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `plugins`

- **Docs:** [README](./plugins/README.md) · [ARCHITECTURE](./plugins/ARCHITECTURE.md) · [ROADMAP](./plugins/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./plugins/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./plugins/SECURITY.md)
- **Public Header Count:** 38
- **Implementation File Count:** 11
- **Consumer Modules (Include-basiert):** ethics_ai, llm, onnx_clip, rpc_grpc, server, stable_diffusion, tests:integration, tests:test_argument_store.cpp, tests:test_argument_store_standalone.cpp, tests:test_discourse_engine.cpp, tests:test_ethical_guidelines_manager.cpp, tests:test_ethics_ai_benchmark.cpp

### Symbol References (Funktion -> Nutzung)

- `AIPluginGenerator` -> _no external call-site detected (or indirect usage)_
- `generatePlugin` -> _no external call-site detected (or indirect usage)_
- `validatePrompt` -> src/aql/llm_aql_handler.cpp, src/llm/prompt_optimizer.cpp, src/prompt_engineering/prompt_optimizer.cpp, tests/test_llm_validation.cpp
- `isInitialized` -> src/acceleration/nccl_vector_backend.cpp, src/acceleration/rccl_vector_backend.cpp, src/index/gpu_vector_index.cpp, src/index/gpu_vector_index_vulkan.cpp
- `transcribe` -> src/content/audio_processor.cpp, src/content/stt_processor.cpp, src/voice/voice_assistant.cpp, src/whisper/tests/test_whisper_plugin.cpp
- `transcribeFile` -> src/whisper/tests/test_whisper_plugin.cpp, src/whisper/whisper_plugin.cpp
- `detectLanguage` -> src/analytics/nlp_text_analyzer.cpp, src/llm/ethical_guidelines_manager.cpp, src/storage/nlp_metadata_extractor.cpp, src/training/lora_data_selection.cpp
- `getModelId` -> src/llama_cpp/llama_cpp_plugin.cpp, src/stable_diffusion/sd_plugin.cpp, src/stable_diffusion/tests/test_sd_plugin.cpp, src/whisper/tests/test_whisper_plugin.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `process`

- **Docs:** [README](./process/README.md) · [ARCHITECTURE](./process/ARCHITECTURE.md) · [ROADMAP](./process/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./process/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./process/SECURITY.md)
- **Public Header Count:** 14
- **Implementation File Count:** 12
- **Consumer Modules (Include-basiert):** tests:security, tests:test_process_aris_xml.cpp, tests:test_process_module.cpp, tests:test_q3_module_interfaces.cpp

### Symbol References (Funktion -> Nutzung)

- `importXml` -> tests/security/test_process_parser_hardening.cpp, tests/test_process_module.cpp
- `importFile` -> _no external call-site detected (or indirect usage)_
- `exportXml` -> tests/test_process_module.cpp
- `exportFromJson` -> _no external call-site detected (or indirect usage)_
- `escapeXml_` -> _no external call-site detected (or indirect usage)_
- `nodeTypeToXmlTag_` -> _no external call-site detected (or indirect usage)_
- `xmlTagToNodeType_` -> _no external call-site detected (or indirect usage)_
- `loadFromJson` -> src/aql/aql_lora_finetuner.cpp, src/llm/vision_config.cpp, src/security/access_control_manager.cpp, src/security/rbac.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `projects`

- **Docs:** [README](./projects/README.md) · [ARCHITECTURE](./projects/ARCHITECTURE.md) · [ROADMAP](./projects/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./projects/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./projects/SECURITY.md)
- **Public Header Count:** 8
- **Implementation File Count:** 5
- **Consumer Modules (Include-basiert):** tests:test_projects.cpp, tests:test_q3_module_interfaces.cpp

### Symbol References (Funktion -> Nutzung)

- `toJson` -> src/analytics/diff_engine.cpp, src/api/geo_index_hooks.cpp, src/aql/aql_lora_finetuner.cpp, src/cache/cache_replication.cpp
- `fromJson` -> src/analytics/diff_engine.cpp, src/cache/semantic_cache.cpp, src/cdc/changefeed.cpp, src/cdc/consumer_group.cpp
- `DocumentManager` -> _no external call-site detected (or indirect usage)_
- `uploadDocument` -> _no external call-site detected (or indirect usage)_
- `getDocument` -> src/ingestion/ingestion_sinks.cpp, tests/test_behoerden_genehmigungsverfahren_e2e.cpp, tests/test_bimschv_genehmigungsverfahren_e2e.cpp, tests/test_cross_module_german_egov.cpp
- `getDocumentBlob` -> _no external call-site detected (or indirect usage)_
- `getDocumentChunks` -> _no external call-site detected (or indirect usage)_
- `getChunk` -> src/content/content_manager.cpp, src/sharding/raft_log.cpp, tests/test_content_features.cpp, tests/test_raft_log.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `prompt_engineering`

- **Docs:** [README](./prompt_engineering/README.md) · [ARCHITECTURE](./prompt_engineering/ARCHITECTURE.md) · [ROADMAP](./prompt_engineering/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./prompt_engineering/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./prompt_engineering/SECURITY.md)
- **Public Header Count:** 31
- **Implementation File Count:** 31
- **Consumer Modules (Include-basiert):** server, tests:test_chain_of_thought.cpp, tests:test_context_window_manager.cpp, tests:test_cot_tracer.cpp, tests:test_distributed_knowledge_integration.cpp, tests:test_dspy_module.cpp, tests:test_feedback_collector.cpp, tests:test_feedback_collector_scaling.cpp, tests:test_meta_prompt_generator.cpp, tests:test_meta_prompt_llm_provider.cpp, tests:test_prompt_ab_experiment.cpp, tests:test_prompt_engineering_integration.cpp

### Symbol References (Funktion -> Nutzung)

- `attackCategoryName` -> _no external call-site detected (or indirect usage)_
- `addTestCase` -> src/updates/schema_migration_tester.cpp, tests/test_prompt_engineering_phase6.cpp, tests/test_schema_migration_tester.cpp
- `loadDefaultTestSuite` -> tests/test_prompt_engineering_phase6.cpp
- `runAll` -> src/updates/preflight_health_check.cpp, tests/test_preflight_health_check.cpp, tests/test_prompt_engineering_phase6.cpp
- `runOne` -> tests/test_prompt_engineering_phase6.cpp
- `testCases` -> tests/test_prompt_engineering_phase6.cpp
- `SimpleAdversarialTester` -> _no external call-site detected (or indirect usage)_
- `setDetectorFn` -> tests/test_prompt_engineering_phase6.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `query`

- **Docs:** [README](./query/README.md) · [ARCHITECTURE](./query/ARCHITECTURE.md) · [ROADMAP](./query/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./query/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./query/SECURITY.md)
- **Public Header Count:** 69
- **Implementation File Count:** 44
- **Consumer Modules (Include-basiert):** aql, ethics_ai, exporters, graph, main.cpp, scheduler, server, tests:geo, tests:integration, tests:performance, tests:query, tests:test_adaptive_join_strategies.cpp

### Symbol References (Funktion -> Nutzung)

- `joinAlgorithmName` -> tests/test_adaptive_join_strategies.cpp
- `estimateJoinCost` -> tests/test_adaptive_join_strategies.cpp
- `executeHashJoin` -> _no external call-site detected (or indirect usage)_
- `executeMergeJoin` -> _no external call-site detected (or indirect usage)_
- `executeNestedLoopJoin` -> _no external call-site detected (or indirect usage)_
- `executeIndexNestedLoopJoin` -> _no external call-site detected (or indirect usage)_
- `executeGraceHashJoin` -> _no external call-site detected (or indirect usage)_
- `executeBroadcastJoin` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `rag`

- **Docs:** [README](./rag/README.md) · [ARCHITECTURE](./rag/ARCHITECTURE.md) · [ROADMAP](./rag/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./rag/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./rag/SECURITY.md)
- **Public Header Count:** 56
- **Implementation File Count:** 56
- **Consumer Modules (Include-basiert):** llama_cpp, tests:test_ab_testing_framework.cpp, tests:test_bayesian_optimizer.cpp, tests:test_claim_extractor.cpp, tests:test_clo_loops.cpp, tests:test_continuous_learning_client.cpp, tests:test_continuous_learning_orchestrator.cpp, tests:test_distributed_knowledge_integration.cpp, tests:test_explainability_reason_builder.cpp, tests:test_geval.cpp, tests:test_knowledge_gap_detector.cpp, tests:test_knowledge_gap_retrieval_callback.cpp

### Symbol References (Funktion -> Nutzung)

- `ABTestingFramework` -> _no external call-site detected (or indirect usage)_
- `startTest` -> src/base/ab_test_manager.cpp, tests/test_ab_test_manager.cpp, tests/test_ab_testing_framework.cpp
- `recordObservation` -> tests/test_ab_testing_framework.cpp
- `shouldUseTreatment` -> src/base/ab_test_manager.cpp, tests/test_ab_test_manager.cpp, tests/test_ab_testing_framework.cpp
- `evaluateTest` -> src/base/ab_test_manager.cpp, tests/test_ab_test_manager.cpp, tests/test_ab_testing_framework.cpp
- `getActiveTests` -> src/base/ab_test_manager.cpp, tests/test_ab_test_manager.cpp, tests/test_ab_testing_framework.cpp
- `getTestStatus` -> src/base/ab_test_manager.cpp, tests/test_ab_test_manager.cpp, tests/test_ab_testing_framework.cpp
- `completeTest` -> tests/test_ab_testing_framework.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `replication`

- **Docs:** [README](./replication/README.md) · [ARCHITECTURE](./replication/ARCHITECTURE.md) · [ROADMAP](./replication/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./replication/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./replication/SECURITY.md)
- **Public Header Count:** 13
- **Implementation File Count:** 10
- **Consumer Modules (Include-basiert):** tests:test_geo_replication_consistency.cpp, tests:test_ha_enhancements.cpp, tests:test_logical_replication.cpp, tests:test_multi_region_active_active.cpp, tests:test_q3_module_interfaces.cpp, tests:test_replication_crdt_types.cpp, tests:test_replication_ha.cpp, tests:test_replication_new_features.cpp, tests:test_replication_raft_v2.cpp

### Symbol References (Funktion -> Nutzung)

- `resolve` -> src/config/config_path_resolver.cpp, src/content/async_ingestion_worker.cpp, src/importers/conflict_resolver.cpp, src/importers/postgres_importer.cpp
- `strategyName` -> tests/test_replication_ha.cpp, tests/test_replication_new_features.cpp
- `selectBase` -> _no external call-site detected (or indirect usage)_
- `mergeJson` -> _no external call-site detected (or indirect usage)_
- `FieldLevelMergeResolver` -> tests/test_replication_new_features.cpp
- `mergeFields` -> _no external call-site detected (or indirect usage)_
- `cancel` -> src/cdc/changefeed.cpp, src/cdc/ws_transport.cpp, src/importers/flatfile_importer.cpp, src/importers/gui_import_wizard.cpp
- `subscribe` -> src/analytics/cep_engine.cpp, src/api/graphql_ws_handler.cpp, src/cdc/changefeed.cpp, src/cdc/ws_transport.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `rpc_grpc`

- **Docs:** [README](./rpc_grpc/README.md) · [ARCHITECTURE](./rpc_grpc/ARCHITECTURE.md) · [ROADMAP](./rpc_grpc/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./rpc_grpc/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./rpc_grpc/SECURITY.md)
- **Public Header Count:** 0
- **Implementation File Count:** 1
- **Consumer Modules (Include-basiert):** tests:test_bidi_stream_adapter.cpp, tests:test_grpc_observability.cpp, tests:test_grpc_plugin.cpp, tests:test_grpc_plugin_lifecycle.cpp

### Symbol References (Funktion -> Nutzung)

- `createPlugin` -> src/ethics_ai/ethics_ai_plugin.cpp, src/importers/postgres_importer.cpp, src/llama_cpp/llama_cpp_registrar.cpp, src/llama_cpp/tests/test_llama_cpp_plugin.cpp
- `destroyPlugin` -> src/ethics_ai/ethics_ai_plugin.cpp, src/importers/postgres_importer.cpp, tests/test_ethics_ai_plugin.cpp, tests/test_importer_plugin_api.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `scheduler`

- **Docs:** [README](./scheduler/README.md) · [ARCHITECTURE](./scheduler/ARCHITECTURE.md) · [ROADMAP](./scheduler/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./scheduler/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./scheduler/SECURITY.md)
- **Public Header Count:** 9
- **Implementation File Count:** 9
- **Consumer Modules (Include-basiert):** maintenance, server, tests:llm, tests:test_auth_middleware.cpp, tests:test_chaos_scheduler.cpp, tests:test_distributed_task_coordinator.cpp, tests:test_event_trigger.cpp, tests:test_external_scheduler_adapter.cpp, tests:test_hybrid_retention_manager.cpp, tests:test_scheduler_integration.cpp, tests:test_task_audit.cpp, tests:test_task_result_store.cpp

### Symbol References (Funktion -> Nutzung)

- `DistributedTaskCoordinator` -> tests/test_distributed_task_coordinator.cpp
- `isLeader` -> src/network/raft_load_balancer.cpp, src/replication/replication_manager.cpp, src/server/distributed_gateway.cpp, src/sharding/distributed_coordinator.cpp
- `getCurrentLeader` -> src/sharding/distributed_coordinator.cpp, tests/test_distributed_coordinator.cpp
- `getLocalNodeId` -> tests/test_distributed_task_coordinator.cpp
- `activateScheduler` -> _no external call-site detected (or indirect usage)_
- `deactivateScheduler` -> _no external call-site detected (or indirect usage)_
- `isSchedulerActive` -> tests/test_distributed_task_coordinator.cpp
- `registerTask` -> src/maintenance/database_maintenance_orchestrator.cpp, src/server/http_server.cpp, src/server/task_scheduler_api_handler.cpp, tests/test_chaos_scheduler.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `search`

- **Docs:** [README](./search/README.md) · [ARCHITECTURE](./search/ARCHITECTURE.md) · [ROADMAP](./search/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./search/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./search/SECURITY.md)
- **Public Header Count:** 20
- **Implementation File Count:** 20
- **Consumer Modules (Include-basiert):** tests:test_autocomplete.cpp, tests:test_cross_lingual_search.cpp, tests:test_distributed_hybrid_search.cpp, tests:test_faceted_search.cpp, tests:test_fuzzy_matcher.cpp, tests:test_hybrid_search.cpp, tests:test_hybrid_search_integration.cpp, tests:test_learning_to_rank.cpp, tests:test_llm_query_rewriter.cpp, tests:test_llm_reranker.cpp, tests:test_multi_field_search.cpp, tests:test_multi_modal_search.cpp

### Symbol References (Funktion -> Nutzung)

- `AutocompleteEngine` -> tests/test_autocomplete.cpp
- `suggest` -> src/aql/aql_optimizer_advisor.cpp, src/aql/aql_rollback_suggester.cpp, src/rag/bayesian_optimizer.cpp, src/rag/continuous_learning_orchestrator.cpp
- `suggestByPrefix` -> tests/test_autocomplete.cpp
- `suggestPopular` -> tests/test_autocomplete.cpp
- `ConversationalSearch` -> tests/test_search_future_interfaces.cpp
- `search` -> src/acceleration/faiss_gpu_backend.cpp, src/api/themisdb_grpc_service.cpp, src/aql/aql_migration_assistant.cpp, src/aql/aql_query_template_library.cpp
- `reformulate` -> tests/test_search_future_interfaces.cpp
- `clearHistory` -> src/llm/prompt_optimizer.cpp, src/prompt_engineering/prompt_optimizer.cpp, tests/test_prompt_optimizer.cpp, tests/test_search_future_interfaces.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `security`

- **Docs:** [README](./security/README.md) · [ARCHITECTURE](./security/ARCHITECTURE.md) · [ROADMAP](./security/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./security/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./security/SECURITY.md)
- **Public Header Count:** 44
- **Implementation File Count:** 42
- **Consumer Modules (Include-basiert):** auth, core, demo_encryption.cpp, exporters, governance, index, llm, main_server.cpp, network, observability, query, scheduler

### Symbol References (Funktion -> Nutzung)

- `AccessControl` -> _no external call-site detected (or indirect usage)_
- `authenticate` -> src/auth/api_key_authenticator.cpp, src/auth/ldap_authenticator.cpp, src/auth/mtls_authenticator.cpp, src/auth/oauth_device_flow.cpp
- `changePassword` -> tests/test_access_control.cpp
- `enrollMFA` -> tests/test_access_control.cpp
- `verifyMFA` -> _no external call-site detected (or indirect usage)_
- `disableMFA` -> _no external call-site detected (or indirect usage)_
- `authorize` -> src/api/ws_handler.cpp, src/auth/principal_validator.cpp, src/server/auth_middleware.cpp, src/server/cache_admin_api_handler.cpp
- `checkPermission` -> tests/security/attack-vectors/authentication/test_authentication_attack_vectors.cpp, tests/security/test_security_negative_integration.cpp, tests/test_access_control.cpp, tests/test_rbac_comprehensive.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `server`

- **Docs:** [README](./server/README.md) · [ARCHITECTURE](./server/ARCHITECTURE.md) · [ROADMAP](./server/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./server/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./server/SECURITY.md)
- **Public Header Count:** 122
- **Implementation File Count:** 120
- **Consumer Modules (Include-basiert):** api, auth, llm, main_server.cpp, security, tests:integration, tests:llm, tests:security, tests:test_adaptive_throttling_comprehensive.cpp, tests:test_anomaly_detection.cpp, tests:test_api_auth_config.cpp, tests:test_api_gateway.cpp

### Symbol References (Funktion -> Nutzung)

- `AdaptiveRateLimiter` -> _no external call-site detected (or indirect usage)_
- `recordSample` -> tests/test_rate_limiting_improvements.cpp
- `allowRequest` -> src/aql/llm_aql_handler.cpp, src/auth/auth_rate_limiter.cpp, src/cache/adaptive_query_cache.cpp, src/sharding/circuit_breaker.cpp
- `getCurrentCapacity` -> tests/test_rate_limiting_improvements.cpp
- `pruneAndAdapt` -> _no external call-site detected (or indirect usage)_
- `computeP99` -> _no external call-site detected (or indirect usage)_
- `computeErrorRate` -> _no external call-site detected (or indirect usage)_
- `AdminApiHandler` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `sharding`

- **Docs:** [README](./sharding/README.md) · [ARCHITECTURE](./sharding/ARCHITECTURE.md) · [ROADMAP](./sharding/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./sharding/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./sharding/SECURITY.md)
- **Public Header Count:** 87
- **Implementation File Count:** 80
- **Consumer Modules (Include-basiert):** aql, llm, main_server.cpp, query, server, storage, tests:integration, tests:query, tests:test_adapter_sync.cpp, tests:test_adaptive_shard_rebalancer.cpp, tests:test_adaptive_shard_router.cpp, tests:test_capability_matcher.cpp

### Symbol References (Funktion -> Nutzung)

- `AdaptiveShardRouter` -> tests/test_llm_raid_integration.cpp, tests/test_llm_raid_routing.cpp
- `executeAdaptiveQuery` -> tests/test_adaptive_shard_router.cpp
- `updateAdapterCapability` -> tests/test_adaptive_shard_router.cpp, tests/test_distributed_knowledge_integration.cpp, tests/test_llm_raid_integration.cpp, tests/test_llm_raid_routing.cpp
- `updateShardLLMLoad` -> tests/test_llm_raid_routing.cpp
- `routeByDomain` -> tests/test_adaptive_shard_router.cpp, tests/test_distributed_knowledge_integration.cpp, tests/test_llm_raid_integration.cpp, tests/test_llm_raid_routing.cpp
- `getAdapterAccuracyDelta` -> src/query/query_federation.cpp, tests/test_adaptive_shard_router.cpp
- `getAdaptiveStatistics` -> tests/test_adaptive_shard_router.cpp
- `updateAdaptiveConfig` -> tests/test_adaptive_shard_router.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `stable_diffusion`

- **Docs:** [README](./stable_diffusion/README.md) · [ARCHITECTURE](./stable_diffusion/ARCHITECTURE.md) · [ROADMAP](./stable_diffusion/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./stable_diffusion/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./stable_diffusion/SECURITY.md)
- **Public Header Count:** 5
- **Implementation File Count:** 6
- **Consumer Modules (Include-basiert):** none detected

### Symbol References (Funktion -> Nutzung)

- `fromJson` -> src/analytics/diff_engine.cpp, src/cache/semantic_cache.cpp, src/cdc/changefeed.cpp, src/cdc/consumer_group.cpp
- `toJson` -> src/analytics/diff_engine.cpp, src/api/geo_index_hooks.cpp, src/aql/aql_lora_finetuner.cpp, src/cache/cache_replication.cpp
- `isInitialized` -> src/acceleration/nccl_vector_backend.cpp, src/acceleration/rccl_vector_backend.cpp, src/index/gpu_vector_index.cpp, src/index/gpu_vector_index_vulkan.cpp
- `generate` -> src/aql/docs_assistant_functions.cpp, src/aql/llm_aql_handler.cpp, src/ingestion/ingestion_quality_judge.cpp, src/ingestion/llm_adapter.cpp
- `getModelId` -> src/llama_cpp/llama_cpp_plugin.cpp, src/whisper/tests/test_whisper_plugin.cpp, src/whisper/whisper_plugin.cpp
- `free_sd_ctx` -> _no external call-site detected (or indirect usage)_
- `samplerFromString` -> _no external call-site detected (or indirect usage)_
- `SDPlugin` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `storage`

- **Docs:** [README](./storage/README.md) · [ARCHITECTURE](./storage/ARCHITECTURE.md) · [ROADMAP](./storage/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./storage/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./storage/SECURITY.md)
- **Public Header Count:** 49
- **Implementation File Count:** 51
- **Consumer Modules (Include-basiert):** acceleration, analytics, api, cache, cdc, content, demo_encryption.cpp, ethics_ai, geo, graph, index, llm

### Symbol References (Funktion -> Nutzung)

- `AdaptiveCompactionScheduler` -> _no external call-site detected (or indirect usage)_
- `recordRead` -> tests/test_adaptive_compaction.cpp, tests/test_tiered_storage.cpp
- `recordWrite` -> src/server/entity_api_handler.cpp, src/sharding/multi_primary_coordinator.cpp, src/sharding/replica_consistency.cpp, tests/test_adaptive_compaction.cpp
- `predictCompactionImpact` -> tests/test_adaptive_compaction.cpp
- `isLowLoadPeriod` -> tests/test_adaptive_compaction.cpp
- `shouldTriggerCompaction` -> tests/test_adaptive_compaction.cpp
- `getAdaptedConfig` -> tests/test_adaptive_compaction.cpp
- `applyAdaptedConfig` -> tests/test_adaptive_compaction.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `temporal`

- **Docs:** [README](./temporal/README.md) · [ARCHITECTURE](./temporal/ARCHITECTURE.md) · [ROADMAP](./temporal/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./temporal/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./temporal/SECURITY.md)
- **Public Header Count:** 16
- **Implementation File Count:** 15
- **Consumer Modules (Include-basiert):** geo, storage, tests:geo, tests:storage, tests:temporal, tests:test_bitemporal_join.cpp, tests:test_cross_module_temporal_bitemporal.cpp

### Symbol References (Funktion -> Nutzung)

- `validate` -> src/api/ws_handler.cpp, src/aql/aql_query_builder.cpp, src/aql/aql_query_validator.cpp, src/aql/llm_aql_handler.cpp
- `BiTemporalTable` -> _no external call-site detected (or indirect usage)_
- `insertWithValidTime` -> tests/temporal/test_bi_temporal.cpp, tests/temporal/test_temporal_query_engine.cpp, tests/temporal/test_temporal_v18_v19.cpp, tests/test_cross_module_temporal_bitemporal.cpp
- `updateForValidTime` -> tests/temporal/test_bi_temporal.cpp
- `deleteForValidTime` -> tests/temporal/test_bi_temporal.cpp
- `queryBiTemporal` -> tests/temporal/test_bi_temporal.cpp
- `queryCurrentByValidTime` -> tests/temporal/test_bi_temporal.cpp, tests/temporal/test_temporal_v18_v19.cpp
- `findOverlaps` -> tests/temporal/test_bi_temporal.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `themis`

- **Docs:** [README](./themis/README.md) · [ARCHITECTURE](./themis/ARCHITECTURE.md) · [ROADMAP](./themis/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./themis/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./themis/SECURITY.md)
- **Public Header Count:** 54
- **Implementation File Count:** 11
- **Consumer Modules (Include-basiert):** acceleration, analytics, api, base, core, geo, gpu, index, llm, main_server.cpp, network, plugins

### Symbol References (Funktion -> Nutzung)

- `ABTestManager` -> src/base/ab_test_manager.cpp
- `setStorageEngine` -> src/base/ab_test_manager.cpp, tests/test_ab_test_manager.cpp
- `setMetricsCollector` -> src/base/ab_test_manager.cpp, src/metadata/index_recommender.cpp, tests/test_ab_test_manager.cpp, tests/test_continuous_batch_scheduler.cpp
- `startTest` -> src/base/ab_test_manager.cpp, src/rag/ab_testing_framework.cpp, src/rag/continuous_learning_orchestrator.cpp, tests/test_ab_test_manager.cpp
- `promoteTest` -> src/base/ab_test_manager.cpp, tests/test_ab_test_manager.cpp
- `rollbackTest` -> src/base/ab_test_manager.cpp, tests/test_ab_test_manager.cpp
- `cancelTest` -> src/base/ab_test_manager.cpp, src/rag/ab_testing_framework.cpp, tests/test_ab_test_manager.cpp, tests/test_ab_testing_framework.cpp
- `shouldUseTreatment` -> src/base/ab_test_manager.cpp, src/rag/ab_testing_framework.cpp, tests/test_ab_test_manager.cpp, tests/test_ab_testing_framework.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `timeseries`

- **Docs:** [README](./timeseries/README.md) · [ARCHITECTURE](./timeseries/ARCHITECTURE.md) · [ROADMAP](./timeseries/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./timeseries/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./timeseries/SECURITY.md)
- **Public Header Count:** 22
- **Implementation File Count:** 23
- **Consumer Modules (Include-basiert):** network, scheduler, server, tests:test_adaptive_flush_controller.cpp, tests:test_binary_protocol_buffers.cpp, tests:test_chunk_level_encryption.cpp, tests:test_continuous_agg.cpp, tests:test_continuous_agg_comprehensive.cpp, tests:test_cross_module_timeseries_forecasting.cpp, tests:test_downsampling.cpp, tests:test_gorilla.cpp, tests:test_gorilla_codec_edge_cases.cpp

### Symbol References (Funktion -> Nutzung)

- `addBatch` -> src/index/vector_auto_buffer.cpp, src/index/vector_index.cpp, tests/test_adaptive_flush_controller.cpp, tests/test_vector_stats_standalone.cpp
- `flush` -> src/analytics/streaming_window.cpp, src/cache/warmup.cpp, src/cdc/changefeed_buffer.cpp, src/cdc/kafka_cdc_producer.cpp
- `getStats` -> src/acceleration/vllm_resource_manager.cpp, src/analytics/anomaly_detection.cpp, src/analytics/cep_engine.cpp, src/analytics/streaming_window.cpp
- `isBackpressured` -> tests/test_adaptive_flush_controller.cpp
- `flushThread` -> src/cdc/changefeed_buffer.cpp, src/index/graph_auto_buffer.cpp, src/index/vector_auto_buffer.cpp
- `flushInternal` -> src/cdc/changefeed_buffer.cpp, src/index/graph_auto_buffer.cpp, src/index/vector_auto_buffer.cpp
- `watermarkThreshold` -> _no external call-site detected (or indirect usage)_
- `watermarkReached` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `toolbox`

- **Docs:** [README](./toolbox/README.md) · [ARCHITECTURE](./toolbox/ARCHITECTURE.md) · [ROADMAP](./toolbox/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./toolbox/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./toolbox/SECURITY.md)
- **Public Header Count:** 3
- **Implementation File Count:** 3
- **Consumer Modules (Include-basiert):** tests:test_content_toolbox_bridge.cpp, tests:test_rag_ingestion_bridge.cpp, tests:test_toolbox_ingestion.cpp

### Symbol References (Funktion -> Nutzung)

- `ContentToolboxBridge` -> tests/test_content_toolbox_bridge.cpp
- `ingest` -> src/analytics/streaming_window.cpp, src/ingestion/api_connector.cpp, src/ingestion/cdc_connector.cpp, src/ingestion/database_connector.cpp
- `enrichExisting` -> _no external call-site detected (or indirect usage)_
- `toolbox` -> src/aql/aql_ingestion_bridge.cpp, src/rag/rag_ingestion_bridge.cpp, tests/test_rag_ingestion_bridge.cpp, tests/test_toolbox_ingestion.cpp
- `contentManager` -> _no external call-site detected (or indirect usage)_
- `graphWriter` -> src/aql/aql_ingestion_bridge.cpp, src/rag/rag_ingestion_bridge.cpp, tests/test_content_toolbox_bridge.cpp, tests/test_rag_ingestion_bridge.cpp
- `vectorWriter` -> src/rag/rag_ingestion_bridge.cpp, tests/test_rag_ingestion_bridge.cpp
- `IngestionToolbox` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `training`

- **Docs:** [README](./training/README.md) · [ARCHITECTURE](./training/ARCHITECTURE.md) · [ROADMAP](./training/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./training/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./training/SECURITY.md)
- **Public Header Count:** 14
- **Implementation File Count:** 14
- **Consumer Modules (Include-basiert):** rag, tests:test_ada_lora_adapter.cpp, tests:test_advanced_training_features.cpp, tests:test_auto_labeler_db_fetch.cpp, tests:test_auto_labeler_production.cpp, tests:test_clo_loops.cpp, tests:test_continuous_learning_orchestrator.cpp, tests:test_cross_module_training_governance.cpp, tests:test_database_domain_auto_labeler.cpp, tests:test_incremental_lora_trainer.cpp, tests:test_kge_vector_search.cpp, tests:test_knowledge_graph_production.cpp

### Symbol References (Funktion -> Nutzung)

- `AdaLoRAAdapter` -> _no external call-site detected (or indirect usage)_
- `addLayer` -> src/llm/lora_framework/gpu_training_loop.cpp, src/llm/lora_framework/lora_training_service.cpp, tests/test_ada_lora_adapter.cpp, tests/test_cross_module_training_governance.cpp
- `removeLayer` -> tests/test_ada_lora_adapter.cpp, tests/test_training_lora_adapter.cpp
- `hasLayer` -> tests/test_ada_lora_adapter.cpp, tests/test_training_lora_adapter.cpp
- `layerNames` -> tests/test_ada_lora_adapter.cpp, tests/test_training_lora_adapter.cpp
- `layerCount` -> tests/test_ada_lora_adapter.cpp, tests/test_cross_module_training_governance.cpp, tests/test_training_lora_adapter.cpp
- `updateImportance` -> tests/test_ada_lora_adapter.cpp, tests/test_cross_module_training_governance.cpp
- `updateAllImportances` -> tests/test_ada_lora_adapter.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `transaction`

- **Docs:** [README](./transaction/README.md) · [ARCHITECTURE](./transaction/ARCHITECTURE.md) · [ROADMAP](./transaction/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./transaction/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./transaction/SECURITY.md)
- **Public Header Count:** 16
- **Implementation File Count:** 15
- **Consumer Modules (Include-basiert):** api, main.cpp, main_server.cpp, network, server, storage, tests:db, tests:test_adaptive_deadlock_prevention.cpp, tests:test_api_integration.cpp, tests:test_aql_path_constraints.cpp, tests:test_aql_shortestpath.cpp, tests:test_branch_conflict_resolution.cpp

### Symbol References (Funktion -> Nutzung)

- `toJson` -> src/analytics/diff_engine.cpp, src/api/geo_index_hooks.cpp, src/aql/aql_lora_finetuner.cpp, src/cache/cache_replication.cpp
- `fromJson` -> src/analytics/diff_engine.cpp, src/cache/semantic_cache.cpp, src/cdc/changefeed.cpp, src/cdc/consumer_group.cpp
- `BranchManager` -> _no external call-site detected (or indirect usage)_
- `setMergeEngine` -> src/server/http_server.cpp, tests/test_branch_conflict_resolution.cpp
- `createBranch` -> src/prompt_engineering/prompt_version_control.cpp, src/server/branch_api_handler.cpp, tests/test_branch_conflict_resolution.cpp, tests/test_branch_integration.cpp
- `getBranch` -> src/server/branch_api_handler.cpp, tests/test_branch_manager.cpp
- `listBranches` -> src/prompt_engineering/prompt_version_control.cpp, src/server/branch_api_handler.cpp, tests/test_branch_integration.cpp, tests/test_branch_manager.cpp
- `switchBranch` -> src/server/branch_api_handler.cpp, tests/test_branch_integration.cpp, tests/test_branch_manager.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `updates`

- **Docs:** [README](./updates/README.md) · [ARCHITECTURE](./updates/ARCHITECTURE.md) · [ROADMAP](./updates/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./updates/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./updates/SECURITY.md)
- **Public Header Count:** 21
- **Implementation File Count:** 21
- **Consumer Modules (Include-basiert):** tests:test_automatic_schema_migration.cpp, tests:test_binary_delta_patches.cpp, tests:test_blue_green_deployment.cpp, tests:test_canary_rollout.cpp, tests:test_coordinated_update_manager.cpp, tests:test_dependency_resolution_engine.cpp, tests:test_distributed_cluster_updates.cpp, tests:test_hardware_telemetry.cpp, tests:test_in_place_schema_migrator.cpp, tests:test_manifest_database_file_deletion.cpp, tests:test_multi_tenant_update_scheduling.cpp, tests:test_notification_webhook.cpp

### Symbol References (Funktion -> Nutzung)

- `deployToStandby` -> tests/test_blue_green_deployment.cpp
- `promote` -> tests/test_blue_green_deployment.cpp
- `rollback` -> src/base/hot_reload_manager.cpp, src/index/graph_index.cpp, src/index/property_graph.cpp, src/index/secondary_index.cpp
- `reportSuccess` -> src/utils/grpc_channel_pool.cpp, tests/test_blue_green_deployment.cpp, tests/test_canary_rollout.cpp, tests/test_grpc_channel_pool.cpp
- `reportError` -> tests/test_blue_green_deployment.cpp, tests/test_canary_rollout.cpp
- `errorRate` -> src/query/approximate_aggregator.cpp, src/server/graph_api_handler.cpp, tests/test_blue_green_deployment.cpp, tests/test_canary_rollout.cpp
- `shouldRollback` -> tests/test_blue_green_deployment.cpp, tests/test_canary_rollout.cpp
- `status` -> src/analytics/analytics_export.cpp, src/analytics/arrow_flight.cpp, src/auth/rocksdb_token_blacklist.cpp, src/base/module_sandbox.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `user_storage_encrypted`

- **Docs:** [README](./user_storage_encrypted/README.md) · [ARCHITECTURE](./user_storage_encrypted/ARCHITECTURE.md) · [ROADMAP](./user_storage_encrypted/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./user_storage_encrypted/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./user_storage_encrypted/SECURITY.md)
- **Public Header Count:** 8
- **Implementation File Count:** 4
- **Consumer Modules (Include-basiert):** tests:test_user_storage_v03.cpp

### Symbol References (Funktion -> Nutzung)

- `createContainer` -> _no external call-site detected (or indirect usage)_
- `mountContainer` -> _no external call-site detected (or indirect usage)_
- `unmountContainer` -> _no external call-site detected (or indirect usage)_
- `isMounted` -> _no external call-site detected (or indirect usage)_
- `getBackendName` -> src/geo/geo_faiss_knn.cpp, src/llm/attention/flash_attention.cpp, tests/test_flash_attention_correctness.cpp
- `getBackendVersion` -> tests/test_user_storage_v03.cpp
- `checkAvailability` -> tests/test_user_storage_v03.cpp
- `GocryptfsBackend` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `utils`

- **Docs:** [README](./utils/README.md) · [ARCHITECTURE](./utils/ARCHITECTURE.md) · [ROADMAP](./utils/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./utils/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./utils/SECURITY.md)
- **Public Header Count:** 64
- **Implementation File Count:** 46
- **Consumer Modules (Include-basiert):** acceleration, analytics, api, aql, auth, base, cache, cdc, chimera, content, core, ethics_ai

### Symbol References (Funktion -> Nutzung)

- `AuditLogger` -> _no external call-site detected (or indirect usage)_
- `logEvent` -> src/acceleration/plugin_loader.cpp, src/acceleration/plugin_security.cpp, src/base/module_loader.cpp, src/content/content_security.cpp
- `verifyChainIntegrity` -> tests/test_audit_logging_comprehensive.cpp, tests/test_task_scheduler_siem_integration.cpp
- `flush` -> src/analytics/streaming_window.cpp, src/cache/warmup.cpp, src/cdc/changefeed_buffer.cpp, src/cdc/kafka_cdc_producer.cpp
- `getChainState` -> src/security/security_evidence_collector.cpp, tests/test_audit_logging_comprehensive.cpp
- `enumerateEntries` -> src/main_server.cpp, tests/test_audit_logger.cpp
- `archiveOldEntries` -> src/main_server.cpp, tests/test_audit_logger.cpp, tests/test_audit_logger_production.cpp
- `purgeOldEntries` -> src/main_server.cpp, tests/test_audit_logger.cpp, tests/test_audit_logger_production.cpp

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `voice`

- **Docs:** [README](./voice/README.md) · [ARCHITECTURE](./voice/ARCHITECTURE.md) · [ROADMAP](./voice/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./voice/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./voice/SECURITY.md)
- **Public Header Count:** 18
- **Implementation File Count:** 19
- **Consumer Modules (Include-basiert):** server, tests:integration, tests:test_voice_assistant.cpp, tests:test_voice_browser_streaming.cpp, tests:test_voice_coverage.cpp, tests:test_voice_production.cpp, tests:test_voice_security_features.cpp, tests:test_voice_telephony.cpp

### Symbol References (Funktion -> Nutzung)

- `NoiseSuppressor` -> _no external call-site detected (or indirect usage)_
- `suppress` -> tests/test_voice_production.cpp
- `isRNNoiseEnabled` -> tests/test_voice_production.cpp
- `resampleLinear` -> _no external call-site detected (or indirect usage)_
- `processRNNoiseFrames` -> _no external call-site detected (or indirect usage)_
- `processFrame` -> src/content/video_processor.cpp, tests/test_voice_production.cpp
- `applyNoiseReduction` -> tests/test_voice_production.cpp
- `applyRNNoiseSuppression` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.

## Module `whisper`

- **Docs:** [README](./whisper/README.md) · [ARCHITECTURE](./whisper/ARCHITECTURE.md) · [ROADMAP](./whisper/ROADMAP.md) · [FUTURE_ENHANCEMENTS](./whisper/FUTURE_ENHANCEMENTS.md)
- **Security Notes:** [SECURITY](./whisper/SECURITY.md)
- **Public Header Count:** 5
- **Implementation File Count:** 7
- **Consumer Modules (Include-basiert):** none detected

### Symbol References (Funktion -> Nutzung)

- `readFile` -> src/auth/jwks_security.cpp, src/llm/lora_framework/lora_checkpoint_manager.cpp, src/sharding/pki_shard_certificate.cpp, src/updates/delta_update_engine.cpp
- `canRead` -> _no external call-site detected (or indirect usage)_
- `parseWav` -> _no external call-site detected (or indirect usage)_
- `shellEscape` -> _no external call-site detected (or indirect usage)_
- `addReader` -> _no external call-site detected (or indirect usage)_
- `fromJson` -> src/analytics/diff_engine.cpp, src/cache/semantic_cache.cpp, src/cdc/changefeed.cpp, src/cdc/consumer_group.cpp
- `toJson` -> src/analytics/diff_engine.cpp, src/api/geo_index_hooks.cpp, src/aql/aql_lora_finetuner.cpp, src/cache/cache_replication.cpp
- `WhisperPlugin` -> _no external call-site detected (or indirect usage)_

### Audit Focus

- **Missing links:** Symbole ohne externe Nutzung gegen Architektur-/Roadmap-Anspruch prüfen.
- **Bottlenecks:** Stark konsumierte Module und Hot-Path-Symbole benchmarken/profilen.
- **Security/Governance:** Sensitive Call-Sites mit Modul-`SECURITY.md`, Root-`SECURITY.md`, `GOVERNANCE.md` abgleichen.
- **Performance:** Kritische Pfade gegen `PERFORMANCE_EXPECTATIONS.md` und Modul-Benchmarks validieren.
