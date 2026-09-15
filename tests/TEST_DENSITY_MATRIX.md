# Source-Validated Test Density Matrix (`src/`)

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-09-14  
**Status:** active

## Scope

This file is the canonical current-state matrix for test-density planning across the `src/` tree.
It maps the current repository clone's `src/<module>` ownership to direct test evidence, direct benchmark evidence, and indirect cross-module/flow evidence.

Primary sources used for this matrix:
- `src/CROSS_MODULE_INTEGRATION.md`
- `docs/architecture/MODULE_ARCHITECTURE.md`
- `docs/architecture/MODULE_INTEGRATION_CONTRACTS.md`
- `tests/TESTING_STANDARDS.md`
- `tests/CMakeLists.txt`
- `tests/**/CMakeLists.txt`
- `tests/integration/**`
- `benchmarks/**`

Non-canonical historical reports such as `tests/TEST_COVERAGE_REPORT.md` are context only and are **not** counted as closure evidence here.

## Method

### Evidence Semantics

- **Direct tests** = module-owned test sources under `tests/<module>/`
- **Direct benchmarks** = module-owned benchmark sources under `benchmarks/<module>/`
- **Indirect evidence** = source-validated coverage via `tests/integration/**`, root-level `tests/test_cross_module_*.cpp`, epic-owned suites, or adjacent owner suites that exercise the module in a larger flow

### Explicit exclusions

- `tests/legacy/**` mirror trees are excluded from current closure accounting
- historical markdown summaries are excluded from closure accounting
- docs-only `src/` paths are excluded from code-bearing module counts

## Summary Snapshot (source-validated 2026-09-14)

- `src/` top-level module paths: **72**
- code-bearing module paths: **69**
- docs-only paths: **3** → `ai_working`, `llm_streaming`, `vector_search`
- code-bearing modules with direct module-owned tests: **65 / 69**
- code-bearing modules with direct module-owned benchmarks: **65 / 69**
- code-bearing modules with both direct tests and direct benchmarks: **63 / 69**

### Immediate ownership gaps

- **No module-owned tests and no module-owned benchmarks:** `distributed_tensor`, `execution`
- **No module-owned tests, but benchmarks exist:** `llama_cpp`, `stable_diffusion`
- **No module-owned benchmarks, but tests exist:** `evaluation`, `retrieval`

## Priority A — Integration hubs and flow owners

| Module | Direct tests | Direct benchmarks | Indirect flow / cross-module evidence | Primary closure gap |
|---|---|---|---|---|
| `server` | `tests/server/` | `benchmarks/server/` | `tests/integration/pipeline/*.cpp`, `tests/server/test_server_*` | Direct suites exist; remaining closure is explicit cross-module and `release_critical` traceability. |
| `llm` | `tests/llm/` | `benchmarks/llm/` | `tests/integration/pipeline/rag_ai_pipeline_test.cpp`, `tests/llm/test_llm_governance_pipeline_e2e_focused.cpp` | Direct suites exist; remaining closure is explicit cross-module and `release_critical` traceability. |
| `query` | `tests/query/` | `benchmarks/query/` | `tests/query/*.cpp`, `benchmarks/query/*.cpp`, `tests/integration/pipeline/*.cpp` | Direct suites exist; remaining closure is explicit cross-module and `release_critical` traceability. |
| `rag` | `tests/rag/` | `benchmarks/rag/` | `tests/rag/*.cpp`, `benchmarks/rag/*.cpp`, `tests/integration/pipeline/*.cpp` | Direct suites exist; remaining closure is explicit cross-module and `release_critical` traceability. |
| `storage` | `tests/storage/` | `benchmarks/storage/` | `tests/storage/*.cpp`, `benchmarks/storage/*.cpp`, `tests/integration/end_to_end/storage_pipeline_e2e_test.cpp` | Direct suites exist; remaining closure is explicit cross-module and `release_critical` traceability. |
| `index` | `tests/index/` | `benchmarks/index/` | `tests/index/*.cpp`, `benchmarks/index/*.cpp`, `tests/integration/pipeline/cross_module_ingest_index_query_test.cpp` | Direct suites exist; remaining closure is explicit cross-module and `release_critical` traceability. |
| `security` | `tests/security/` | `benchmarks/security/` | `tests/integration/pipeline/security_pipeline_test.cpp`, `tests/test_cross_module_security_governance.cpp` | Direct suites exist; remaining closure is explicit cross-module and `release_critical` traceability. |
| `sharding` | `tests/sharding/` | `benchmarks/sharding/` | `tests/integration/pipeline/transaction_replication_pipeline_test.cpp`, `tests/test_cross_module_query_sharding.cpp` | Direct suites exist; remaining closure is explicit cross-module and `release_critical` traceability. |
| `transaction` | `tests/transaction/` | `benchmarks/transaction/` | `tests/integration/pipeline/transaction_replication_pipeline_test.cpp`, `tests/transaction/test_transaction_*` | Direct suites exist; remaining closure is explicit cross-module and `release_critical` traceability. |
| `observability` | `tests/observability/` | `benchmarks/observability/` | `tests/integration/test_cross_functional_voice_observability.cpp`, `tests/integration/pipeline/w9b_sla_compliance_uptime_test.cpp` | Direct suites exist; remaining closure is explicit cross-module and `release_critical` traceability. |

## Priority B — Boundary and adapter modules

| Module | Direct tests | Direct benchmarks | Indirect flow / cross-module evidence | Primary closure gap |
|---|---|---|---|---|
| `api` | `tests/api/` | `benchmarks/api/` | `tests/integration/test_graphql_e2e.cpp`, `tests/server/test_server_openapi_drift_focused.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `network` | `tests/network/` | `benchmarks/network/` | `tests/integration/rpc/rpc_service_integration_test.cpp`, `tests/integration/test_graphql_e2e.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `rpc_grpc` | `tests/rpc_grpc/` | `benchmarks/rpc_grpc/` | `tests/integration/rpc/rpc_service_integration_test.cpp`, `tests/rpc_grpc/test_rpc_grpc_integration_focused.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `auth` | `tests/auth/` | `benchmarks/auth/` | `tests/integration/rpc/rpc_service_integration_test.cpp`, `tests/test_cross_module_security_governance.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `governance` | `tests/governance/` | `benchmarks/governance/` | `tests/test_cross_module_security_governance.cpp`, `tests/test_cross_module_training_governance.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `plugins` | `tests/plugins/` | `benchmarks/plugins/` | `tests/plugins/test_plugin_manager*.cpp`, `tests/plugins/test_plugin_registrar_fail_closed.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `importers` | `tests/importers/` | `benchmarks/importers/` | `tests/test_cross_module_german_egov.cpp`, `tests/ingestion/test_ingestion_features.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `exporters` | `tests/exporters/` | `benchmarks/exporters/` | `tests/integration/pipeline/analytics_export_pipeline_test.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `content` | `tests/content/` | `benchmarks/content/` | `tests/test_cross_module_german_egov.cpp`, `tests/content/test_content_toolbox_bridge.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `scheduler` | `tests/scheduler/` | `benchmarks/scheduler/` | `tests/integration/test_load_balancing.cpp`, `tests/scheduler/test_scheduler_integration.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |

## Priority C — Remaining code-bearing modules

| Module | Direct tests | Direct benchmarks | Indirect flow / cross-module evidence | Primary closure gap |
|---|---|---|---|---|
| `acceleration` | `tests/acceleration/` | `benchmarks/acceleration/` | `tests/test_cross_module_acceleration_index.cpp`, `tests/acceleration/test_ai_hardware_dispatcher.cpp` | Roadmap-open hardening remains; source evidence exists but closure is not complete. |
| `access_model` | `tests/access_model/` | `benchmarks/access_model/` | `tests/access_model/*.cpp`, `benchmarks/access_model/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `ai` | `tests/ai/` | `benchmarks/ai/` | `tests/ai/test_ai_plugin_generator.cpp`, `tests/llm/test_ai_orchestrator.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `analytics` | `tests/analytics/` | `benchmarks/analytics/` | `tests/integration/pipeline/analytics_export_pipeline_test.cpp`, `tests/test_cross_module_timeseries_forecasting.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `aql` | `tests/aql/` | `benchmarks/aql/` | `tests/aql/*.cpp`, `benchmarks/aql/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `base` | `tests/base/` | `benchmarks/base/` | `tests/base/*.cpp`, `benchmarks/base/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `cache` | `tests/cache/` | `benchmarks/cache/` | `tests/test_cross_module_cache_anomaly.cpp`, `tests/query/test_query_cache_manager.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `cdc` | `tests/cdc/` | `benchmarks/cdc/` | `tests/cdc/*.cpp`, `benchmarks/cdc/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `chaos` | `tests/chaos/` | `benchmarks/chaos/` | `tests/chaos/*.cpp`, `benchmarks/chaos/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `chimera` | `tests/chimera/` | `benchmarks/chimera/` | `tests/chimera/test_themisdb_adapter.cpp`, `tests/chimera/test_chimera_prepared_statements.cpp` | Roadmap-open hardening remains; source evidence exists but closure is not complete. |
| `config` | `tests/config/` | `benchmarks/config/` | `tests/config/*.cpp`, `benchmarks/config/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `core` | `tests/core/` | `benchmarks/core/` | `tests/core/*.cpp`, `benchmarks/core/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `distributed_knowledge` | `tests/distributed_knowledge/` | `benchmarks/distributed_knowledge/` | `tests/distributed_knowledge/test_federated_rag_merger_focused.cpp`, `tests/distributed_knowledge/test_lora_federation_coordinator_focused.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `distributed_tensor` | — | — | `tests/epic3_distributed_tensor/`, `tests/tensor/test_tensor_mid_layer_integration.cpp` | No module-owned tests or benchmarks; closure must start with dedicated owner suites. |
| `document` | `tests/document/` | `benchmarks/document/` | `tests/test_cross_module_german_egov.cpp`, `benchmarks/document/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `ethics_ai` | `tests/ethics_ai/` | `benchmarks/ethics_ai/` | `tests/ethics_ai/*.cpp`, `benchmarks/ethics_ai/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `evaluation` | `tests/evaluation/` | — | `tests/evaluation/test_evaluation_*.cpp`, `tests/epic2_evaluation/*.cc` | Direct tests exist, but module-owned benchmark/perf evidence is missing. |
| `execution` | — | — | `tests/test_parallel_executor.cpp`, `tests/test_vectorized_execution.cpp`, `tests/integration/test_load_balancing.cpp`, `tests/integration/test_resource_pooling.cpp` | No module-owned tests or benchmarks; closure must start with dedicated owner suites. |
| `failover` | `tests/failover/` | `benchmarks/failover/` | `tests/failover/*.cpp`, `benchmarks/failover/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `geo` | `tests/geo/` | `benchmarks/geo/` | `tests/test_cross_module_geo_spatial.cpp`, `tests/query/test_geospatial_phase2_integration.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `gpu` | `tests/gpu/` | `benchmarks/gpu/` | `tests/gpu/*.cpp`, `benchmarks/gpu/*.cpp` | Roadmap-open hardening remains; source evidence exists but closure is not complete. |
| `graph` | `tests/graph/` | `benchmarks/graph/` | `tests/integration/pipeline/rag_ai_pipeline_test.cpp`, `tests/test_cross_module_graph_lineage.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `image_analysis` | `tests/image_analysis/` | `benchmarks/image_analysis/` | `tests/image_analysis/test_image_analysis_phase1_focused.cpp`, `tests/test_image_analysis_quality.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `ingestion` | `tests/ingestion/` | `benchmarks/ingestion/` | `tests/integration/pipeline/ingestion_pipeline_test.cpp`, `tests/integration/pipeline/cross_module_ingest_index_query_test.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `llama_cpp` | — | `benchmarks/llama_cpp/` | `tests/llm/test_llama_cpp_plugin_validation_gates.cpp`, `tests/llm/test_llama_cpp_real_inference.cpp`, `tests/llm/test_llama_wrapper_state.cpp` | No module-owned test sources; indirect evidence exists but dedicated suites are missing. |
| `llm_wiki` | `tests/llm_wiki/` | `benchmarks/llm_wiki/` | `tests/llm/test_llm_wiki_*.cpp`, `benchmarks/llm_wiki/*.cpp` | Roadmap-open hardening remains; source evidence exists but closure is not complete. |
| `maintenance` | `tests/maintenance/` | `benchmarks/maintenance/` | `tests/maintenance/test_maintenance_*_focused.cpp`, `benchmarks/maintenance/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `metadata` | `tests/metadata/` | `benchmarks/metadata/` | `tests/metadata/*.cpp`, `benchmarks/metadata/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `onnx_clip` | `tests/onnx_clip/` | `benchmarks/onnx_clip/` | `tests/onnx_clip/test_onnx_clip_*.cpp`, `tests/test_onnx_clip_plugin.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `performance` | `tests/performance/` | `benchmarks/performance/` | `tests/performance/*.cpp`, `benchmarks/performance/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `process` | `tests/process/` | `benchmarks/process/` | `tests/process/test_process_*`, `benchmarks/process/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `projects` | `tests/projects/` | `benchmarks/projects/` | `tests/projects/*.cpp`, `benchmarks/projects/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `prompt_engineering` | `tests/prompt_engineering/` | `benchmarks/prompt_engineering/` | `tests/prompt_engineering/*.cpp`, `benchmarks/prompt_engineering/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `replication` | `tests/replication/` | `benchmarks/replication/` | `tests/replication/*.cpp`, `benchmarks/replication/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `retrieval` | `tests/retrieval/` | — | `tests/search/test_layered_retrieval_integration_phase4.cpp`, `tests/rag/test_rag_adaptive_retrieval.cpp` | Direct tests exist, but module-owned benchmark/perf evidence is missing. |
| `scraper` | `tests/scraper/` | `benchmarks/scraper/` | `tests/scraper/test_scraper_plugin.cpp`, `tests/scraper/test_scraper_*_focused.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `search` | `tests/search/` | `benchmarks/search/` | `tests/search/*.cpp`, `benchmarks/search/*.cpp` | Roadmap-open hardening remains; source evidence exists but closure is not complete. |
| `stable_diffusion` | — | `benchmarks/stable_diffusion/` | `tests/plugins/test_plugin_registrar_fail_closed.cpp`, `benchmarks/stable_diffusion/*.cpp` | No module-owned test sources; indirect evidence exists but dedicated suites are missing. |
| `temporal` | `tests/temporal/` | `benchmarks/temporal/` | `tests/test_cross_module_temporal_bitemporal.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `tensor` | `tests/tensor/` | `benchmarks/tensor/` | `tests/integration/pipeline/rag_ai_pipeline_test.cpp`, `tests/tensor/test_tensor_mid_layer_integration.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `themis` | `tests/themis/` | `benchmarks/themis/` | `tests/themis/*.cpp`, `benchmarks/themis/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `timeseries` | `tests/timeseries/` | `benchmarks/timeseries/` | `tests/timeseries/*.cpp`, `tests/test_cross_module_timeseries_forecasting.cpp`, `benchmarks/timeseries/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `toolbox` | `tests/toolbox/` | `benchmarks/toolbox/` | `tests/toolbox/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `training` | `tests/training/` | `benchmarks/training/` | `tests/test_cross_module_training_governance.cpp`, `tests/rag/test_rag_rlaif_trainer.cpp` | Roadmap-open hardening remains; source evidence exists but closure is not complete. |
| `updates` | `tests/updates/` | `benchmarks/updates/` | `tests/updates/*.cpp`, `benchmarks/updates/*.cpp` | Roadmap-open hardening remains; source evidence exists but closure is not complete. |
| `user_storage_encrypted` | `tests/user_storage_encrypted/` | `benchmarks/user_storage_encrypted/` | `tests/user_storage_encrypted/*.cpp`, `benchmarks/user_storage_encrypted/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `utils` | `tests/utils/` | `benchmarks/utils/` | `tests/utils/`, `benchmarks/utils/*.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `voice` | `tests/voice/` | `benchmarks/voice/` | `tests/integration/voice/test_voice_endurance_stress.cpp`, `tests/integration/test_cross_functional_voice_observability.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |
| `whisper` | `tests/whisper/` | `benchmarks/whisper/` | `tests/whisper/test_whisper_stub_transcribe_bridge.cpp` | Owned suites exist; next step is explicit flow and `release_critical` mapping. |

## Critical flow coverage matrix

Legend:
- **D** = direct existing evidence in current source tree
- **I** = indirect source-validated evidence through adjacent or shared suites
- **—** = not yet explicitly evidenced in the current matrix

| Flow | focused / unit | integration | cross-module | flow / pipeline | e2e | chaos / recovery | stress / soak | benchmark / perf | `release_critical` | Primary evidence | Current closure gap |
|---|---|---|---|---|---|---|---|---|---|---|---|
| `server -> query -> storage -> transaction` | D | D | I | D | D | D | D | D | D | `tests/integration/pipeline/query_execution_pipeline_test.cpp`, `tests/integration/end_to_end/storage_pipeline_e2e_test.cpp`, `tests/integration/pipeline/w5a_e2e_critical_journeys_test.cpp`, `tests/integration/pipeline/w9c_chaos_fault_tolerance_test.cpp`, `tests/integration/pipeline/w6b_stress_soak_stability_test.cpp`, `tests/integration/pipeline/w7c_endurance_stability_test.cpp` | Focused, pipeline, e2e, and chaos anchors are wired into one Wave A aggregate target, and both benchmark plus long-run evidence now have explicit aggregate build/ctest entrypoints; dependency-complete validation remains pending. |
| `sharding <-> transaction` | D | D | I | D | I | D | D | D | D | `tests/integration/pipeline/transaction_replication_pipeline_test.cpp`, `tests/sharding/test_sharding_chaos.cpp`, `tests/transaction/test_transaction_chaos.cpp`, `tests/integration/test_sharding_distributed_write_soak.cpp`, `tests/integration/test_replication_soak_60min.cpp` | Focused, pipeline, and chaos release-critical anchors are wired into one Wave A aggregate target, and the sharding/transaction benchmark plus long-run soak entrypoints are now explicit. |
| `server <-> llm` | D | D | I | I | D | D | D | D | I | `tests/llm/test_llm_governance_pipeline_e2e_focused.cpp`, `tests/llm/test_llm_multi_model_integration.cpp`, `tests/llm/test_streaming_handler.cpp`, `tests/server/test_server_gateway_resilience_focused.cpp`, `tests/integration/voice/test_voice_endurance_stress.cpp`, `benchmarks/server/bench_server_http3_gates.cpp`, `benchmarks/llm/bench_llm_inference_performance.cpp` | Focused, pipeline, and dedicated multi-model round-trip anchors are wired into one Wave A aggregate target, and the server/llm benchmark plus long-run evidence now have explicit aggregate build/ctest entrypoints. |
| `search -> index -> tensor -> graph -> llm` | D | D | D | I | I | I | D | D | D | `tests/search/test_search_integration_phase4.cpp`, `tests/index/test_distributed_vector_index.cpp`, `tests/tensor/test_tensor_stress_suite_focused.cpp`, `tests/search/test_search_distributed_merge_stress.cpp`, `tests/graph/test_graph_distributed.cpp`, `benchmarks/search/bench_layered_retrieval_phase5.cpp`, `benchmarks/rag/bench_fts_phase_b.cpp` | Search-stack benchmark and long-run stress evidence now have explicit aggregate build/ctest entrypoints, but deeper end-to-end and chaos-style flow ownership remains thinner than the primary SQST and sharding flows. |

## Canonical closure execution plan

- Execution plan: `TEST_DENSITY_WAVE_PLAN.md`
- Wave A backlog: `TEST_DENSITY_WAVE_A_BACKLOG.md`
- Governance basis: root Wave A -> B -> C -> D release model on `develop`

## Closure waves

### Wave A — release-critical core flow closure

Target modules:
- `query`
- `index`
- `rag`
- `transaction`
- `llm_wiki`
- `search`
- `server`
- `sharding`
- `llm`
- `storage`

Wave A exit expectation:
- all open test-density roadmap items reclassified with source evidence or closed with direct suites
- explicit `release_critical` mapping for each module's critical test path
- every canonical release-critical flow mapped to one owned focused/integration/perf gate path, with any unified-only supplemental evidence explicitly documented
- benchmark smoke entries for the canonical Wave A perf anchors with explicit aggregate build targets and CTest labels
- representative benchmark / soak artifacts linked from one module-local closure block, with slow-lane long-run evidence separated from fast Wave A gates

### Wave B — boundary, API, and policy densification

Target modules:
- `api`
- `network`
- `rpc_grpc`
- `auth`
- `governance`
- `plugins`
- `importers`
- `exporters`
- `content`
- `scheduler`
- `observability`

Wave B exit expectation:
- each boundary module has direct suite ownership plus at least one assigned production-near flow
- policy / contract / boundary regression reruns are reproducible from commands, labels, and current presets
- no boundary module remains covered only by implied adjacency

### Wave C — ownership gap closure

Target modules:
- `distributed_tensor`
- `execution`
- `llama_cpp`
- `stable_diffusion`
- `evaluation`
- `retrieval`

Wave C exit expectation:
- no code-bearing module relies on indirect evidence alone where module-owned suites are feasible
- thin modules either gain dedicated owner suites or receive an explicit accepted-indirect-evidence rule

### Wave D — hardening and sign-off

Target scope:
- repo-wide closure audit across Waves A-C

Wave D exit expectation:
- soak / stress / chaos / representative-hardware evidence is back-linked to owning modules and canonical flows
- historical report files are context only and no longer used for operational closure decisions
- module ownership, flow evidence, and gate paths are synchronized in one canonical closure narrative

## Definition of "test density closed"

`src/` test density should only be treated as closed when all of the following are true:

1. every code-bearing module has direct or explicitly accepted indirect evidence
2. each canonical release-critical flow has focused, integration, cross-module, pipeline, e2e, chaos/recovery, stress/soak, benchmark/perf, and `release_critical` proof
3. hub-module roadmap items are either closed or intentionally deferred with source-backed rationale
4. evidence is reproducible from source, CTest registration, and labels instead of historical summary documents
