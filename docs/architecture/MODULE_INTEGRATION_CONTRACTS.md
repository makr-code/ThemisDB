# Cross-Module Integration Contracts

## Method

- Direct dependencies in this document are **include-derived** edges: `module A -> module B` exists when a file under `src/A/**` or `include/A/**` includes a path rooted at `B/`.
- Scan scope: all C/C++ headers and sources under `src/**` and `include/**` in the current clone.
- This captures **direct compile-time coupling**, not the full runtime call graph.
- To reduce that blind spot, this document also links a **runtime integration overlay** for release-critical cross-module execution paths that are evidenced in concrete handler/orchestrator/coordinator files.

## Summary

| Signal | Value | Notes |
|---|---:|---|
| Top-level `src/` module paths | 72 | includes runtime, thin, docs-only, and source-root support paths |
| Direct inter-module include edges | 392 | unique `module -> module` edges only |
| Code-bearing module paths | 69 | paths with at least one colocated C/C++ file |
| Docs-only module paths | 3 | `ai_working`, `llm_streaming`, `vector_search` |
| Non-trivial SCCs | 1 | one large strongly connected component remains in the include graph |
| Zero-outgoing modules | 7 | `ai_working`, `chaos`, `evaluation`, `execution`, `llm_streaming`, `retrieval`, `vector_search` |
| Runtime-critical integration overlay | 10 edges | explicit execution-path evidence for handler, coordinator, and orchestrator wiring |

## Coupling Hubs

| Type | Module | Count | Evidence anchor |
|---|---|---:|---|
| outgoing deps | `server` | 35 | `src/server/http_server.cpp:87` |
| outgoing deps | `llm` | 22 | `src/llm/llama_resource_manager.cpp:17` |
| outgoing deps | `query` | 17 | `src/query/query_optimizer.cpp:19` |
| outgoing deps | `rag` | 16 | `src/rag/continuous_learning_orchestrator.cpp:19` |
| outgoing deps | `storage` | 13 | `include/storage/tiered_storage.h:26` |
| outgoing deps | `index` | 11 | `src/index/gpu_vector_index.cpp:15` |
| outgoing deps | `aql` | 11 | `src/aql/aql_optimizer_advisor.cpp:22` |
| outgoing deps | `graph` | 10 | `include/graph/scheduled_edge_refresh.h:14` |
| incoming consumers | `utils` | 51 | `include/utils/` or `src/utils/` |
| incoming consumers | `storage` | 35 | `include/storage/` or `src/storage/` |
| incoming consumers | `index` | 25 | `include/index/` or `src/index/` |
| incoming consumers | `themis` | 23 | `include/themis/` or `src/themis/` |
| incoming consumers | `security` | 20 | `include/security/` or `src/security/` |
| incoming consumers | `llm` | 18 | `include/llm/` or `src/llm/` |
| incoming consumers | `observability` | 17 | `include/observability/` or `src/observability/` |
| incoming consumers | `query` | 15 | `include/query/` or `src/query/` |

## Circular Dependency Findings

| SCC | Modules | Interpretation |
|---|---|---|
| SCC-01 | `acceleration`, `access_model`, `analytics`, `api`, `aql`, `auth`, `cache`, `cdc`, `config`, `content`, `core`, `distributed_knowledge`, `document`, `ethics_ai`, `exporters`, `geo`, `governance`, `gpu`, `graph`, `importers`, `index`, `ingestion`, `llama_cpp`, `llm`, `maintenance`, `metadata`, `network`, `observability`, `performance`, `plugins`, `projects`, `prompt_engineering`, `query`, `rag`, `replication`, `scheduler`, `security`, `server`, `sharding`, `storage`, `temporal`, `tensor`, `themis`, `timeseries`, `toolbox`, `training`, `transaction`, `updates`, `utils`, `voice` | The compile-time graph still contains one 50-module strongly connected component rooted around shared hubs such as `utils`, `storage`, `themis`, `query`, `llm`, and `server`; architectural review should treat these as tightly coupled until the include graph is flattened. |

## Runtime Integration Overlay

| Runtime edge | Role in execution | Evidence |
|---|---|---|
| `server` -> `query` | HTTP/AQL ingress dispatches query execution | `src/server/query_api_handler.cpp`, `src/server/http_server.cpp` |
| `server` -> `llm` | LLM API requests are wired directly into inference/plugin handlers | `src/server/llm_api_handler.cpp`, `src/server/llm_grpc_service.cpp` |
| `query` -> `storage` | query execution and entity lookup read/write RocksDB-backed storage surfaces | `include/query/query_engine.h`, `src/query/query_engine.cpp` |
| `query` -> `transaction` | multi-statement mutations wrap storage context in rollback-aware transaction proxy | `include/query/mutation_transaction.h`, `src/query/aql_runner.cpp` |
| `sharding` -> `transaction` | distributed commit and recovery interact with transaction-side global/recoverable coordinators | `src/sharding/distributed_transaction.cpp`, `src/transaction/global_transaction_manager.cpp` |
| `sharding` -> `storage` | shard transaction state and WAL-backed persistence rely on shared storage contracts | `src/sharding/transaction_wal.cpp`, `src/sharding/transaction_snapshot.cpp` |
| `search` -> `index` | ANN retrieval delegates to active vector runtime in `index` | `include/search/layered_retrieval_orchestrator.h`, `src/search/layered_retrieval_orchestrator.cpp` |
| `search` -> `tensor` | layered retrieval invokes tensor fingerprint candidates before graph/LLM steps | `include/search/layered_retrieval_orchestrator.h`, `src/search/layered_retrieval_orchestrator.cpp` |
| `search` -> `graph` | provenance expansion is driven by knowledge-graph reasoning in the retrieval chain | `include/search/layered_retrieval_orchestrator.h`, `src/search/layered_retrieval_orchestrator.cpp` |
| `search` -> `llm` | final layered retrieval answer can terminate in an LLM-backed answer stage | `include/search/layered_retrieval_orchestrator.h`, `src/search/layered_retrieval_orchestrator.cpp` |

**Interpretation:** use the include-derived inventory for global compile-time coupling, and use this overlay plus `docs/architecture/DATA_FLOW_PATHS.md` when the review question is about runtime orchestration, ownership, or release-critical behavior.

## Direct Dependency Inventory

| Module | Direct module deps | Direct consumers | Example evidence |
|---|---|---:|---|
| `acceleration` | `geo`, `index`, `llm`, `storage`, `themis`, `utils` | 8 | `src/acceleration/geo_acceleration_bridge.cpp:65` · `src/acceleration/vec_knn.cpp:35` |
| `access_model` | `utils` | 2 | `src/access_model/access_coordinator.cpp:29` |
| `ai` | `ethics_ai`, `llm`, `plugins`, `utils` | 0 | `include/ai/cai_ethics_integration.h:15` · `include/ai/cai_ethics_integration.h:14` |
| `ai_working` | — | 0 | — |
| `analytics` | `cdc`, `index`, `security`, `storage`, `themis`, `transaction`, `utils` | 10 | `include/analytics/diff_engine.h:15` · `include/analytics/process_mining.h:15` |
| `api` | `cdc`, `distributed_knowledge`, `index`, `query`, `server`, `storage`, `themis`, `transaction`, `utils` | 3 | `include/api/graphql_ws_handler.h:67` · `include/api/federation_admin_handler.h:13` |
| `aql` | `analytics`, `distributed_knowledge`, `index`, `ingestion`, `llm`, `prompt_engineering`, `query`, `sharding`, `storage`, `toolbox`, `utils` | 4 | `src/aql/aql_optimizer_advisor.cpp:22` · `src/aql/llm_aql_handler.cpp:38` |
| `auth` | `security`, `server`, `utils` | 3 | `src/auth/ldap_authenticator.cpp:18` · `src/auth/principal_validator.cpp:18` |
| `base` | `acceleration`, `observability`, `themis`, `utils` | 0 | `src/base/module_loader.cpp:31` · `src/base/ab_test_manager.cpp:24` |
| `cache` | `access_model`, `core`, `index`, `observability`, `performance`, `security`, `storage`, `utils` | 5 | `include/cache/adaptive_query_cache.h:34` · `include/cache/eviction_policy.h:18` |
| `cdc` | `analytics`, `storage`, `utils` | 9 | `include/cdc/cdc_materialized_view.h:28` · `src/cdc/cdc_admin.cpp:17` |
| `chaos` | — | 0 | — |
| `chimera` | `index`, `query`, `utils` | 0 | `src/chimera/themisdb_adapter.cpp:27` · `src/chimera/themisdb_adapter.cpp:26` |
| `config` | `observability` | 5 | `src/config/config_metrics_exporter.cpp:16` |
| `content` | `cache`, `config`, `index`, `ingestion`, `llm`, `security`, `storage`, `utils` | 5 | `include/content/deduplication_checker.h:20` · `src/content/mime_detector.cpp:13` |
| `core` | `auth`, `index`, `observability`, `query`, `security`, `sharding`, `storage`, `themis`, `utils` | 7 | `include/core/config_validator.h:15` · `include/core/index_initialization.h:15` |
| `distributed_knowledge` | `governance`, `llm` | 8 | `include/distributed_knowledge/federated_rag_merger.h:22` · `include/distributed_knowledge/lora_federation_coordinator.h:17` |
| `distributed_tensor` | `observability`, `rag`, `tensor` | 0 | `src/distributed_tensor/include/manifest_store.h:29` · `src/distributed_tensor/src/distributed_planner.cc:8` |
| `document` | `projects`, `security`, `utils` | 2 | `include/document/document_manager_deprecated.h:12` · `include/document/encrypted_entities.h:15` |
| `ethics_ai` | `plugins`, `query`, `storage`, `utils` | 3 | `include/ethics_ai/ethics_ai_plugin_interface.h:15` · `src/ethics_ai/argument_store.cpp:24` |
| `evaluation` | — | 0 | — |
| `execution` | — | 0 | — |
| `exporters` | `governance`, `plugins`, `query`, `security`, `storage`, `utils` | 2 | `src/exporters/huggingface_hub_client.cpp:18` · `include/exporters/jsonl_llm_exporter.h:18` |
| `failover` | `replication`, `sharding`, `utils` | 0 | `include/failover/auto_failover_manager.h:34` · `include/failover/auto_failover_manager.h:35` |
| `geo` | `acceleration`, `storage`, `temporal`, `themis`, `utils` | 6 | `include/geo/gpu_kernel_dispatcher.h:20` · `src/geo/boost_cpu_exact_backend.cpp:46` |
| `governance` | `observability`, `security`, `themis`, `utils` | 5 | `src/governance/policy_manager.cpp:24` · `src/governance/policy_review.cpp:21` |
| `gpu` | `acceleration`, `themis`, `utils` | 1 | `src/gpu/query_accelerator.cpp:46` · `src/gpu/audit_log.cpp:17` |
| `graph` | `analytics`, `cdc`, `index`, `llm`, `observability`, `query`, `security`, `storage`, `themis`, `utils` | 4 | `include/graph/scheduled_edge_refresh.h:14` · `include/graph/scheduled_edge_refresh.h:15` |
| `image_analysis` | `plugins` | 0 | `src/image_analysis/yolov8_onnx_plugin.cpp:33` |
| `importers` | `content`, `plugins`, `utils` | 2 | `src/importers/huggingface_ingestion_plugin.cpp:13` · `src/importers/huggingface_ingestion_plugin.cpp:12` |
| `index` | `acceleration`, `config`, `geo`, `llm`, `metadata`, `observability`, `performance`, `security`, `storage`, `themis`, `utils` | 25 | `src/index/gpu_vector_index.cpp:15` · `src/index/vector_index.cpp:31` |
| `ingestion` | `document`, `governance`, `index`, `plugins`, `storage`, `utils` | 6 | `include/ingestion/ingestion_sinks.h:17` · `src/ingestion/huggingface_connector.cpp:14` |
| `llama_cpp` | `llm`, `rag`, `utils` | 1 | `src/llama_cpp/llama_cpp_registrar.cpp:15` · `src/llama_cpp/llama_cpp_plugin.cpp:16` |
| `llm` | `acceleration`, `cache`, `core`, `distributed_knowledge`, `ethics_ai`, `exporters`, `governance`, `index`, `ingestion`, `llama_cpp`, `metadata`, `observability`, `performance`, `plugins`, `query`, `rag`, `security`, `server`, `sharding`, `storage`, `themis`, `utils` | 18 | `src/llm/llama_resource_manager.cpp:17` · `src/llm/llm_prefix_cache.cpp:13` |
| `llm_streaming` | — | 0 | — |
| `llm_wiki` | `config`, `importers`, `llm`, `plugins` | 0 | `src/llm_wiki/process_policy_manager.cpp:6` · `src/llm_wiki/wikipedia/llm_wiki_plugin_impl.cpp:12` |
| `maintenance` | `observability`, `scheduler`, `sharding`, `storage`, `themis`, `utils` | 1 | `src/maintenance/database_maintenance_orchestrator.cpp:20` · `src/maintenance/database_maintenance_orchestrator.cpp:16` |
| `metadata` | `aql`, `cdc`, `index`, `observability`, `sharding`, `storage` | 7 | `include/metadata/aql_schema_bridge.h:18` · `src/metadata/schema_manager.cpp:17` |
| `network` | `index`, `query`, `security`, `sharding`, `storage`, `themis`, `timeseries`, `transaction`, `utils` | 2 | `src/network/wire_protocol_server.cpp:47` · `src/network/wire_protocol_server.cpp:57` |
| `observability` | `analytics`, `api`, `core`, `security`, `utils` | 17 | `include/observability/ml_anomaly_detector.h:22` · `src/observability/opentelemetry_tracer.cpp:16` |
| `onnx_clip` | `plugins` | 0 | `src/onnx_clip/onnx_clip_plugin.h:14` |
| `performance` | `query`, `storage`, `utils` | 8 | `include/performance/phase3/per_query_cost_model.h:27` · `src/performance/advanced_cache_manager.cpp:17` |
| `plugins` | `acceleration`, `content`, `core`, `themis`, `utils` | 14 | `src/plugins/plugin_manager.cpp:20` · `include/plugins/huggingface_ingestion_plugin.h:15` |
| `process` | `analytics`, `index`, `rag`, `storage`, `utils` | 0 | `include/process/process_graph_rag.h:25` · `src/process/process_model_manager.cpp:30` |
| `projects` | `index`, `storage` | 1 | `include/projects/DocumentManager/document_manager.h:22` · `include/projects/project_template.h:20` |
| `prompt_engineering` | `distributed_knowledge`, `ethics_ai`, `metadata`, `security`, `storage`, `utils` | 3 | `src/prompt_engineering/feedback_collector.cpp:15` · `include/prompt_engineering/context_window_manager.h:23` |
| `query` | `analytics`, `api`, `aql`, `cache`, `distributed_knowledge`, `geo`, `index`, `llm`, `metadata`, `observability`, `performance`, `scheduler`, `security`, `sharding`, `storage`, `themis`, `utils` | 15 | `src/query/query_optimizer.cpp:19` · `include/query/functions/graphql_functions.h:60` |
| `rag` | `distributed_knowledge`, `document`, `graph`, `index`, `ingestion`, `llm`, `observability`, `performance`, `prompt_engineering`, `security`, `storage`, `tensor`, `themis`, `toolbox`, `training`, `utils` | 7 | `src/rag/continuous_learning_orchestrator.cpp:19` · `src/rag/delegate_evaluator.cpp:13` |
| `replication` | `cdc`, `utils` | 2 | `include/replication/schema_cdc.h:15` · `src/replication/multi_tier_replication.cpp:21` |
| `retrieval` | — | 0 | — |
| `rpc_grpc` | `plugins` | 0 | `src/rpc_grpc/grpc_plugin.h:14` |
| `scheduler` | `cdc`, `observability`, `query`, `security`, `sharding`, `storage`, `themis`, `timeseries`, `utils` | 3 | `src/scheduler/task_scheduler.cpp:26` · `include/scheduler/task_scheduler.h:33` |
| `scraper` | `llm` | 0 | `src/scraper/scraper_llm_evaluator.cpp:21` |
| `search` | `core`, `graph`, `index`, `llm`, `sharding`, `storage`, `tensor`, `themis`, `utils` | 0 | `src/search/layered_retrieval_orchestrator.cpp:8` · `src/search/layered_retrieval_orchestrator.cpp:9` |
| `security` | `auth`, `core`, `query`, `server`, `storage`, `themis`, `utils` | 20 | `src/security/access_control.cpp:16` · `src/security/hsm_provider.cpp:38` |
| `server` | `analytics`, `api`, `aql`, `auth`, `cache`, `cdc`, `config`, `content`, `core`, `exporters`, `geo`, `governance`, `graph`, `importers`, `index`, `llm`, `maintenance`, `metadata`, `network`, `observability`, `performance`, `plugins`, `prompt_engineering`, `query`, `rag`, `scheduler`, `security`, `sharding`, `storage`, `themis`, `timeseries`, `transaction`, `updates`, `utils`, `voice` | 4 | `src/server/http_server.cpp:87` · `src/server/graphql_api_handler.cpp:14` |
| `sharding` | `cache`, `distributed_knowledge`, `storage`, `themis`, `transaction`, `utils` | 14 | `include/sharding/metadata_shard.h:20` · `src/sharding/adaptive_shard_router.cpp:14` |
| `stable_diffusion` | `plugins`, `utils` | 0 | `src/stable_diffusion/sd_plugin_registrar.cpp:15` · `src/stable_diffusion/sd_plugin.cpp:15` |
| `storage` | `access_model`, `analytics`, `cdc`, `index`, `llm`, `metadata`, `performance`, `sharding`, `temporal`, `tensor`, `themis`, `transaction`, `utils` | 35 | `include/storage/tiered_storage.h:26` · `include/storage/nlp_metadata_extractor.h:18` |
| `temporal` | `replication` | 2 | `include/temporal/temporal_conflict_resolver.h:34` |
| `tensor` | `index`, `ingestion`, `observability`, `storage`, `utils` | 4 | `src/tensor/adapter_repository.cpp:13` · `include/tensor/tensor_core_bridge.h:14` |
| `themis` | `acceleration`, `geo`, `gpu`, `index`, `network`, `query`, `rag`, `timeseries`, `utils` | 23 | `src/themis/module_security.cpp:21` · `include/themis/gpu/admin_api.h:21` |
| `timeseries` | `storage`, `utils` | 4 | `src/timeseries/hypertable.cpp:15` · `src/timeseries/aggregates.cpp:14` |
| `toolbox` | `aql`, `content`, `ingestion`, `rag`, `utils` | 2 | `src/toolbox/toolbox_builder.cpp:14` · `include/toolbox/content_toolbox_bridge.h:25` |
| `training` | `analytics`, `distributed_knowledge`, `graph`, `index`, `llm`, `query`, `storage`, `utils` | 1 | `src/training/auto_labeler.cpp:15` · `include/training/incremental_lora_trainer.h:18` |
| `transaction` | `analytics`, `cdc`, `index`, `llm`, `plugins`, `sharding`, `storage`, `utils` | 6 | `include/transaction/merge_engine.h:16` · `include/transaction/branch_manager.h:16` |
| `updates` | `acceleration`, `index`, `metadata`, `storage`, `utils` | 1 | `include/updates/manifest_database.h:17` · `include/updates/schema_migration_tester.h:21` |
| `user_storage_encrypted` | `security` | 0 | `src/user_storage_encrypted/multi_level_storage.cpp:15` |
| `utils` | `config`, `geo`, `observability`, `performance`, `security`, `sharding`, `storage`, `themis` | 51 | `src/utils/pii_detector.cpp:18` · `include/utils/geometric_distances.h:14` |
| `vector_search` | — | 0 | — |
| `voice` | `content`, `llm`, `utils` | 1 | `include/voice/voice_batch_processor.h:25` · `src/voice/voice_assistant_llm.cpp:13` |
| `whisper` | `plugins` | 0 | `src/whisper/whisper_plugin_registrar.cpp:11` |
