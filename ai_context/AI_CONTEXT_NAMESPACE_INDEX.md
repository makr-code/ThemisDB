# Namespace Index (Alphabetical)

**Datum:** 2026-08-03  
**Status:** Active  
**Primary:** include/<module>/*.h, src/<module>/*.cpp  
**Bezug:** AI-freundliche Namespace-Navigation

## Public Namespaces (Core & Module APIs)

| Namespace | Module | Purpose | Key Headers |
|-----------|--------|---------|-------------|
| `themis::acceleration` | acceleration | GPU backends (CUDA, OpenCL, TPU dispatch) | acceleration.h, cuda_backend.h |
| `themis::access_model` | access_model | Row/column access policies, security enforcement | access_model.h, policy.h |
| `themis::ai` | ai | AI/ML orchestration, model registry, inference | ai_orchestrator.h |
| `themis::analytics` | analytics | Statistical analysis, Arrow-based export | arrow_flight.h, analytics.h |
| `themis::api` | api | HTTP/gRPC/GraphQL transport, versioning, error taxonomy | api_transport_contracts.h, api_error_taxonomy.h |
| `themis::aql` | aql | Query language: parsing, AST, execution bridge | aql_parser.h, aql_compiler.h |
| `themis::auth` | auth | Authentication, principals, authorization | auth_principal_contract.h, auth_provider.h |
| `themis::resource` | base | Resource management, RAII wrappers, scoped handles | resource.h, scoped_lock.h |
| `themis::cache` | cache | Query result cache, semantic cache, plan cache | query_cache.h, semantic_cache.h |
| `themis::cdc` | cdc | Change Data Capture, streaming event pipelines | cdc_processor.h, cdc_sink.h |
| `themis::chaos` | chaos | Chaos injection, failure scenarios, resilience testing | chaos_injector.h, failure_scenarios.h |
| `themis::chimera` | chimera | Plugin system, Chimera adapter pattern | plugin_interface.h, adapter.h |
| `themis::config` | config | Configuration management, environment variables | config_manager.h, environment.h |
| `themis::content` | content | Full-text indexing, content processing pipeline | content_indexer.h, text_analyzer.h |
| `themis::core` | core | Core MVCC, transaction coordinator, snapshot isolation | mvcc.h, transaction_manager.h |
| `themis::distributed_knowledge` | distributed_knowledge | Distributed knowledge graphs, federated queries | kg_federation.h, distributed_kg.h |
| `themis::distributed_tensor` | distributed_tensor | Distributed tensor operations, sharding strategy | distributed_tensor.h, shard_coordinator.h |
| `themis::document` | document | Document store operations, JSON/BSON handling | document_store.h, bson_codec.h |
| `themis::ethics_ai` | ethics_ai | AI ethics validation, bias detection, compliance checks | ethics_validator.h, bias_detector.h |
| `themis::evaluation` | evaluation | Model evaluation framework, metrics collection | evaluator.h, metrics.h |
| `themis::execution` | execution | Query execution engine, operator implementations | executor.h, operator_registry.h |
| `themis::exporters` | exporters | Data export: Parquet, Arrow, CSV, JSON | exporter_factory.h, format_registry.h |
| `themis::failover` | failover | Auto-failover, disaster recovery, state transitions | auto_failover_manager.h, dr_manager.h |
| `themis::geo` | geo | Geospatial indexing (R-tree), PostGIS compatibility | geo_index.h, spatial_query.h |
| `themis::governance` | governance | Governance rules, compliance tracking, audit logging | governance_engine.h, audit_log.h |
| `themis::gpu` | gpu | GPU memory management, kernel dispatch, device pool | gpu_memory.h, kernel_dispatcher.h |
| `themis::graph` | graph | Graph model, traversal, GQL support | graph_store.h, graph_traversal.h |
| `themis::importers` | importers | Data import, ETL, format-specific readers | importer_factory.h, csv_importer.h |
| `themis::index` | index | Index structures: HNSW, B-tree, R-tree lifecycle | hnsw_index.h, btree.h, index_manager.h |
| `themis::ingestion` | ingestion | Batch & streaming ingestion, buffering | ingestion_pipeline.h, batch_loader.h |
| `themis::llama_cpp` | llama_cpp | Llama.cpp model binding, local inference | llama_cpp_backend.h |
| `themis::llm` | llm | LLM inference, model switching, context mgmt | inference_engine.h, model_router.h |
| `themis::llm_wiki` | llm_wiki | LLM-assisted wiki/RAG knowledge retrieval | llm_wiki_plugin_interface.h |
| `themis::maintenance` | maintenance | Garbage collection, compaction, maintenance tasks | maintenance_coordinator.h, gc_policy.h |
| `themis::metadata` | metadata | Metadata store, schema registry, catalog | metadata_store.h, schema_registry.h |
| `themis::network` | network | Network transport, connection pooling, messaging | network_interface.h, connection_pool.h |
| `themis::observability` | observability | Metrics, logging, tracing, hooks | metrics_registry.h, tracer.h |
| `themis::onnx_clip` | onnx_clip | ONNX & CLIP for multimodal embeddings | onnx_backend.h, clip_embedder.h |
| `themis::performance` | performance | Performance tracking, optimization utilities | perf_tracker.h, optimization_hints.h |
| `themis::plugins` | plugins | Plugin loader, manifest parsing, lifecycle | plugin_manager.h, plugin_manifest.h |
| `themis::process` | process | Process lifecycle, startup, shutdown | process_manager.h, init_sequence.h |
| `themis::projects` | projects | Project/tenant management, multi-tenancy | project_manager.h, tenant_isolation.h |
| `themis::prompt_engineering` | prompt_engineering | Prompt templates, engineering utilities | prompt_library.h, template_engine.h |
| `themis::query` | query | Query planning, cost models, optimization | query_planner.h, cost_model.h |
| `themis::rag` | rag | Retrieval-augmented generation, context assembly | rag_pipeline.h, context_assembler.h |
| `themis::replication` | replication | Replication coordination, consistency protocols | replication_manager.h, consistency_protocol.h |
| `themis::retrieval` | retrieval | Vector/semantic retrieval, similarity search | vector_retriever.h, similarity_search.h |
| `themis::rpc_grpc` | rpc_grpc | gRPC service definitions, RPC contracts | themisdb_service.grpc.pb.h |
| `themis::scheduler` | scheduler | Task scheduling, job coordination, futures | job_scheduler.h, futures.h |
| `themis::scraper` | scraper | Web scraping, content ingestion drivers | web_scraper.h, scraper_driver.h |
| `themis::search` | search | Full-text & semantic search, ranking | search_index.h, ranker.h |
| `themis::security` | security | Security primitives, encryption, HSM integration | encryption.h, hsm_interface.h |
| `themis::server` | server | HTTP server, request handling, middleware | http_server.h, middleware.h |
| `themis::sharding` | sharding | Sharding strategy, range-based distribution | shard_manager.h, shard_router.h |
| `themis::stable_diffusion` | stable_diffusion | Stable Diffusion image generation backend | stable_diffusion_backend.h |
| `themis::storage` | storage | RocksDB backend, K-V layer, LSM-tree | rocksdb_backend.h, storage_engine.h |
| `themis::temporal` | temporal | Temporal data types, time optimization | temporal_type.h, temporal_index.h |
| `themis::tensor` | tensor | Tensor operations, numerical computing, SIMD | tensor.h, linear_algebra.h |
| `themis` | themis | Root namespace, forward declarations | themis_export.h |
| `themis::timeseries` | timeseries | Time-series indexing, aggregation, retention | timeseries_index.h, aggregation.h |
| `themis::toolbox` | toolbox | General utilities, algorithms, helpers | hash_utils.h, string_utils.h |
| `themis::training` | training | Model training, fine-tuning, LoRA, incremental | trainer.h, lora_adapter.h |
| `themis::transaction` | transaction | Transaction coordinator, 2PC/3PC/SAGA patterns | transaction_coordinator.h, saga_orchestrator.h |
| `themis::updates` | updates | Update operations, mutations, conflict resolution | update_engine.h, conflict_resolver.h |
| `themis::user_storage_encrypted` | user_storage_encrypted | User data encryption, field-level secrets | encrypted_storage.h |
| `themis::utils` | utils | String, math, container utilities | string_utils.h, container_utils.h |
| `themis::voice` | voice | Voice I/O, speech synthesis & recognition | voice_engine.h, speaker_recognition.h |
| `themis::whisper` | whisper | OpenAI Whisper binding, speech-to-text | whisper_backend.h |

## Private Namespaces (`detail::`, `internal::`)

**Pattern:** `themis::<module>::detail::` or `themis::<module>::internal::`

- Implementation details, not part of public API
- Auto-generated protobuf message namespaces: `themis::rpc_grpc::pb`
- Test helpers: `themis::<module>::test::` (in test files only)

**Note:** Do not depend on `detail::` or `internal::` namespaces in production code.

## Cross-Cutting Namespaces

| Namespace | Modules | Purpose |
|-----------|---------|---------|
| `themis::network` | network, server, rpc_grpc | Shared transport contracts |
| `themis::security` | security, auth, governance, access_model | Security primitives |
| `themis::observability` | observability, performance, chaos | Monitoring & tracing |

## Namespace-to-Header Mapping Convention

For each module `<mod>`:

- `include/<mod>/<primary_type>.h` → exports `themis::<mod>::*`
- `include/<mod>/<mod>.h` → main include (often re-exports public API)
- `include/<mod>/<type>_impl.h` → may contain `themis::<mod>::detail::*`

**Example (api module):**
```cpp
// include/api/api_transport_contracts.h
namespace themis::api {
  class ITransportContract { /* ... */ };
}
```

---

**Zuletzt geprueft (Namespace Index):** 2026-08-03
