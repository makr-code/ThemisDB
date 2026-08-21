# Benchmarks Target Structure Plan

Status: Proposed target architecture
Date: 2026-08-21
Scope: Cleanup and normalization of [benchmarks](benchmarks)

## 1. Objectives

- Separate active benchmark sources from historical reports and ad-hoc scripts.
- Make discovery deterministic: one place for code, one for docs, one for runs.
- Reduce root-level file sprawl in [benchmarks](benchmarks).
- Use a flat, module-specific directory layout directly under [benchmarks](benchmarks), analogous to [src](src) and [include](include).
- Align with canonical governance documents:
  - [benchmarks/BENCHMARK_STANDARDS.md](benchmarks/BENCHMARK_STANDARDS.md)
  - [benchmarks/MEASUREMENT_HYGIENE.md](benchmarks/MEASUREMENT_HYGIENE.md)
  - [benchmarks/README.md](benchmarks/README.md)

## 2. Target Layout (Soll-Struktur)

```text
benchmarks/
  README.md
  BENCHMARK_STANDARDS.md
  MEASUREMENT_HYGIENE.md
  ROADMAP.md
  FUTURE_ENHANCEMENTS.md
  SECURITY.md
  ARCHITECTURE.md
  INDEX.md

  cmake/
    CMakeLists.benchmarks.modular.cmake
    benchmark_target_mapping.json

  include/
    bench_fixtures.h
    benchmark_artifact_preflight.h
    epic_benchmark_scenarios.h

  # Flat module-specific benchmark source folders (analog src/include):
  acceleration/
  ai/
  analytics/
  ann/
  api/
  aql/
  auth/
  cache/
  cdc/
  chimera/
  content/
  core/
  distributed_knowledge/
  distributed_tensor/
  document/
  ethics_ai/
  evaluation/
  exporters/
  failover/
  geo/
  gpu/
  graph/
  importers/
  index/
  ingestion/
  ldbc/
  llama_cpp/
  llm/
  maintenance/
  metadata/
  mmdb/
  monitoring/
  network/
  observability/
  onnx_clip/
  performance/
  performance_optimizations/
  plugins/
  process/
  prompt_engineering/
  query/
  rag/
  replication/
  retrieval/
  rpc_grpc/
  scheduler/
  scraper/
  search/
  security/
  sharding/
  stable_diffusion/
  storage/
  temporal/
  tensor/
  themis/
  timeseries/
  toolbox/
  tpc/
  training/
  transaction/
  updates/
  user_storage_encrypted/
  utils/
  voice/
  whisper/
  ycsb/

  # Program-oriented suites kept at top level when they are suite domains:
  wave2/
  wave3/
  wave4/
  wave5/
  wave7/
  wave8/
  wave9/

  runners/
    python/
    shell/
    powershell/

  tools/
    analysis/
    validation/
    generation/

  data/
    profiles/
    templates/
    sample_inputs/

  outputs/
    current/
    baselines/
    comparisons/

  docs/
    guide/
    runbooks/
    reports/
    historical/
```

## 3. Mapping from Current State

### 3.1 Root `bench_*.cpp` files

Move root benchmark sources directly to module folders under [benchmarks](benchmarks):

- `bench_*llm*` -> [benchmarks/llm](benchmarks/llm)
- `bench_*gpu*` / `bench_*cuda*` / `bench_*vulkan*` -> [benchmarks/gpu](benchmarks/gpu)
- `bench_*graph*` / `bench_*pagerank*` -> [benchmarks/graph](benchmarks/graph)
- `bench_*query*` / `bench_*aql*` -> [benchmarks/query](benchmarks/query) or [benchmarks/aql](benchmarks/aql)
- `bench_*shard*` / `bench_*replication*` / `bench_*changefeed*` -> [benchmarks/sharding](benchmarks/sharding) or [benchmarks/replication](benchmarks/replication)
- `bench_*crud*` / `bench_*mvcc*` / `bench_*wal*` -> [benchmarks/storage](benchmarks/storage)

Rule: keep folder depth low and module-aligned. Do not introduce `benchmarks/src/<module>`.

### 3.2 Scripts and Python tools

- Root `*.py` benchmark runners -> [benchmarks/runners/python](benchmarks/runners/python)
- Root `*.sh` -> [benchmarks/runners/shell](benchmarks/runners/shell)
- Root `*.ps1` -> [benchmarks/runners/powershell](benchmarks/runners/powershell)
- Non-runner analyzers (`analyze_*`, `validate_*`, `*_detector.py`) -> [benchmarks/tools/analysis](benchmarks/tools/analysis) or [benchmarks/tools/validation](benchmarks/tools/validation)

### 3.3 Documentation

- Living docs (quickstart, runbook, standards usage) -> [benchmarks/docs/guide](benchmarks/docs/guide)
- Operational runbooks (wave, docker, raid) -> [benchmarks/docs/runbooks](benchmarks/docs/runbooks)
- Point-in-time summaries/reports -> [benchmarks/docs/reports](benchmarks/docs/reports)
- Archived/superseded docs -> [benchmarks/docs/historical](benchmarks/docs/historical)

## 4. Naming Conventions

- Source files: `bench_<domain>_<scenario>.cpp`
- Python runners: `run_<scope>.py`
- Analysis scripts: `analyze_<scope>.py`
- Validation scripts: `validate_<scope>.py`
- Runbooks: `RUNBOOK_<scope>.md`
- Reports: `REPORT_<scope>_<yyyymmdd>.md`

## 5. CMake / Execution Model

- Keep one top-level [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt), but include modular sublists from module folders directly under [benchmarks](benchmarks) (for example [benchmarks/storage](benchmarks/storage), [benchmarks/llm](benchmarks/llm), [benchmarks/graph](benchmarks/graph)).
- Register benchmark executables by module to avoid monolithic root sections.
- Keep smoke/CI benchmark test registration explicit and minimal.
- All benchmark policy references must point to:
  - [benchmarks/BENCHMARK_STANDARDS.md](benchmarks/BENCHMARK_STANDARDS.md)
  - [benchmarks/MEASUREMENT_HYGIENE.md](benchmarks/MEASUREMENT_HYGIENE.md)

## 6. Migration Phases

### Phase A: Stabilize (no behavioral changes)

- Create target directories.
- Move historical docs into [benchmarks/docs/historical](benchmarks/docs/historical) with redirect stubs.
- Introduce [benchmarks/TARGET_STRUCTURE_PLAN.md](benchmarks/TARGET_STRUCTURE_PLAN.md) and index links.

### Phase B: Source normalization

- Move root `bench_*.cpp` incrementally into module directories directly under [benchmarks](benchmarks).
- Update [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) after each domain move.
- Verify per-domain build targets before next move.

### Phase C: Runner/tool separation

- Move scripts into `runners/` and `tools/`.
- Keep compatibility wrappers at old paths for one transition cycle.

### Phase D: Cutover

- Remove deprecated wrappers.
- Finalize docs index and contributor instructions.
- Freeze naming and path rules in standards docs.

## 7. Definition of Done

- No active benchmark source remains as unmanaged root sprawl.
- [benchmarks/README.md](benchmarks/README.md) and [benchmarks/INDEX.md](benchmarks/INDEX.md) reflect only target structure.
- Historical reports are clearly separated from active guides.
- CMake benchmark registration is modular and traceable by domain.
- CI benchmark entrypoints run only through canonical runner paths.

## 8. Immediate Next Actions (recommended)

1. Create/normalize structural folders (`runners`, `tools`, `outputs`, `docs/historical`) and keep module folders directly under [benchmarks](benchmarks).
2. Migrate one pilot domain first: `storage` (`bench_crud.cpp`, `bench_mvcc.cpp`, `bench_wal_stress.cpp`).
3. Migrate two high-noise docs into `docs/historical` with redirect stubs.
4. Add a small compatibility matrix in [benchmarks/README.md](benchmarks/README.md).

## 9. Execution Snapshot (2026-08-21)

Completed in repository:

- Structural folders created:
  - [benchmarks/runners/python](benchmarks/runners/python)
  - [benchmarks/runners/shell](benchmarks/runners/shell)
  - [benchmarks/runners/powershell](benchmarks/runners/powershell)
  - [benchmarks/tools/analysis](benchmarks/tools/analysis)
  - [benchmarks/tools/validation](benchmarks/tools/validation)
  - [benchmarks/tools/generation](benchmarks/tools/generation)
  - [benchmarks/outputs/current](benchmarks/outputs/current)
  - [benchmarks/outputs/baselines](benchmarks/outputs/baselines)
  - [benchmarks/outputs/comparisons](benchmarks/outputs/comparisons)
  - [benchmarks/docs/historical/2026-08](benchmarks/docs/historical/2026-08)

- Storage pilot migration completed:
  - [benchmarks/storage/bench_crud.cpp](benchmarks/storage/bench_crud.cpp)
  - [benchmarks/storage/bench_mvcc.cpp](benchmarks/storage/bench_mvcc.cpp)
  - [benchmarks/storage/bench_wal_stress.cpp](benchmarks/storage/bench_wal_stress.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep existing target names.

- Larger batch migration completed:
  - Query:
    - [benchmarks/query/bench_adaptive_query_cache.cpp](benchmarks/query/bench_adaptive_query_cache.cpp)
    - [benchmarks/query/bench_adaptive_query_compilation.cpp](benchmarks/query/bench_adaptive_query_compilation.cpp)
    - [benchmarks/query/bench_continuous_query.cpp](benchmarks/query/bench_continuous_query.cpp)
  - AQL:
    - [benchmarks/aql/bench_aql_geo_filter.cpp](benchmarks/aql/bench_aql_geo_filter.cpp)
    - [benchmarks/aql/bench_hybrid_aql_sugar.cpp](benchmarks/aql/bench_hybrid_aql_sugar.cpp)
  - Graph/Geo:
    - [benchmarks/graph/bench_pagerank.cpp](benchmarks/graph/bench_pagerank.cpp)
    - [benchmarks/geo/bench_spatial_index.cpp](benchmarks/geo/bench_spatial_index.cpp)
    - [benchmarks/geo/bench_spatial_join.cpp](benchmarks/geo/bench_spatial_join.cpp)
  - CDC/Sharding:
    - [benchmarks/cdc/bench_changefeed_throughput.cpp](benchmarks/cdc/bench_changefeed_throughput.cpp)
    - [benchmarks/sharding/bench_shard_resource_manager.cpp](benchmarks/sharding/bench_shard_resource_manager.cpp)
    - [benchmarks/sharding/bench_shard_routing.cpp](benchmarks/sharding/bench_shard_routing.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated for new paths while preserving target names.

- LLM/GPU/ANN batch migration completed:
  - LLM:
    - [benchmarks/llm/bench_embedded_llm.cpp](benchmarks/llm/bench_embedded_llm.cpp)
  - ANN:
    - [benchmarks/ann/bench_ann_cpu_gpu_dispatch.cpp](benchmarks/ann/bench_ann_cpu_gpu_dispatch.cpp)
  - GPU:
    - [benchmarks/gpu/bench_multi_gpu_scaling.cpp](benchmarks/gpu/bench_multi_gpu_scaling.cpp)
    - [benchmarks/gpu/bench_multi_gpu_lora_advanced.cpp](benchmarks/gpu/bench_multi_gpu_lora_advanced.cpp)
    - [benchmarks/gpu/bench_lora_gpu.cpp](benchmarks/gpu/bench_lora_gpu.cpp)
    - [benchmarks/gpu/bench_qlora_gpu_kernels.cpp](benchmarks/gpu/bench_qlora_gpu_kernels.cpp)
    - [benchmarks/gpu/bench_tensor_cpu_gpu_dispatch.cpp](benchmarks/gpu/bench_tensor_cpu_gpu_dispatch.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) checks and source paths aligned to module folders.

- ANN + GPU backend-comparison migration completed:
  - ANN:
    - [benchmarks/ann/bench_vector_search.cpp](benchmarks/ann/bench_vector_search.cpp)
    - [benchmarks/ann/bench_vector_prefilter.cpp](benchmarks/ann/bench_vector_prefilter.cpp)
    - [benchmarks/ann/bench_hybrid_vector_geo.cpp](benchmarks/ann/bench_hybrid_vector_geo.cpp)
    - [benchmarks/ann/bench_vector_compression_lossless.cpp](benchmarks/ann/bench_vector_compression_lossless.cpp)
    - [benchmarks/ann/bench_binary_quantization.cpp](benchmarks/ann/bench_binary_quantization.cpp)
    - [benchmarks/ann/bench_product_quantization.cpp](benchmarks/ann/bench_product_quantization.cpp)
    - [benchmarks/ann/bench_residual_quantization.cpp](benchmarks/ann/bench_residual_quantization.cpp)
    - [benchmarks/ann/bench_learned_quantization.cpp](benchmarks/ann/bench_learned_quantization.cpp)
    - [benchmarks/ann/bench_lossy_vs_lossless.cpp](benchmarks/ann/bench_lossy_vs_lossless.cpp)
  - GPU:
    - [benchmarks/gpu/bench_data_transfer.cpp](benchmarks/gpu/bench_data_transfer.cpp)
    - [benchmarks/gpu/bench_backend_comparison.cpp](benchmarks/gpu/bench_backend_comparison.cpp)
    - [benchmarks/gpu/bench_cuda_vs_cpu.cpp](benchmarks/gpu/bench_cuda_vs_cpu.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) checks and source paths aligned to module folders.

- TPC/YCSB/OLAP migration completed:
  - YCSB:
    - [benchmarks/ycsb/bench_ycsb.cpp](benchmarks/ycsb/bench_ycsb.cpp)
  - TPC:
    - [benchmarks/tpc/bench_tpcc.cpp](benchmarks/tpc/bench_tpcc.cpp)
    - [benchmarks/tpc/bench_tpch.cpp](benchmarks/tpc/bench_tpch.cpp)
  - Analytics (OLAP):
    - [benchmarks/analytics/bench_olap_performance.cpp](benchmarks/analytics/bench_olap_performance.cpp)
    - [benchmarks/analytics/bench_olap_analytics.cpp](benchmarks/analytics/bench_olap_analytics.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) checks and source paths aligned to module folders.

- Importer/Cache/Content/MMDB migration completed:
  - Importers:
    - [benchmarks/importers/bench_importer_throughput.cpp](benchmarks/importers/bench_importer_throughput.cpp)
  - Cache:
    - [benchmarks/cache/bench_embedding_cache_performance.cpp](benchmarks/cache/bench_embedding_cache_performance.cpp)
  - Content:
    - [benchmarks/content/bench_content_versioning.cpp](benchmarks/content/bench_content_versioning.cpp)
    - [benchmarks/content/bench_content_processor_paths.cpp](benchmarks/content/bench_content_processor_paths.cpp)
  - MMDB:
    - [benchmarks/mmdb/bench_mmdb.cpp](benchmarks/mmdb/bench_mmdb.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep target names stable and avoid duplicate content registrations.

- Performance/Maintenance/Security migration completed:
  - Performance:
    - [benchmarks/performance/bench_random_access_prefetch.cpp](benchmarks/performance/bench_random_access_prefetch.cpp)
  - Maintenance:
    - [benchmarks/maintenance/bench_snapshot_manager.cpp](benchmarks/maintenance/bench_snapshot_manager.cpp)
    - [benchmarks/maintenance/bench_update_pipeline.cpp](benchmarks/maintenance/bench_update_pipeline.cpp)
    - [benchmarks/maintenance/bench_metrics_collector.cpp](benchmarks/maintenance/bench_metrics_collector.cpp)
    - [benchmarks/maintenance/bench_lock_contention.cpp](benchmarks/maintenance/bench_lock_contention.cpp)
    - [benchmarks/maintenance/bench_saga_compensation.cpp](benchmarks/maintenance/bench_saga_compensation.cpp)
    - [benchmarks/maintenance/bench_sanity.cpp](benchmarks/maintenance/bench_sanity.cpp)
  - Security:
    - [benchmarks/security/bench_security.cpp](benchmarks/security/bench_security.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) and [benchmarks/security/CMakeLists.txt](benchmarks/security/CMakeLists.txt) updated to avoid duplicate registration.

- Observability/Plugins/Docker/Security-compliance migration completed:
  - Observability:
    - [benchmarks/observability/bench_di_logging.cpp](benchmarks/observability/bench_di_logging.cpp)
  - Maintenance:
    - [benchmarks/maintenance/bench_hot_reload_manager.cpp](benchmarks/maintenance/bench_hot_reload_manager.cpp)
    - [benchmarks/maintenance/bench_docker_raid_comprehensive.cpp](benchmarks/maintenance/bench_docker_raid_comprehensive.cpp)
  - Plugins:
    - [benchmarks/plugins/bench_plugin_hot_plug.cpp](benchmarks/plugins/bench_plugin_hot_plug.cpp)
    - [benchmarks/plugins/bench_plugin_system.cpp](benchmarks/plugins/bench_plugin_system.cpp)
  - Security compliance:
    - [benchmarks/security/bench_compliance_security_governance.cpp](benchmarks/security/bench_compliance_security_governance.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) and [benchmarks/security/CMakeLists.txt](benchmarks/security/CMakeLists.txt) updated to keep canonical targets stable.

- Base/Storage/Monitoring/Process migration completed:
  - Base:
    - [benchmarks/base/bench_auto_buffers.cpp](benchmarks/base/bench_auto_buffers.cpp)
    - [benchmarks/base/bench_base_hot_paths.cpp](benchmarks/base/bench_base_hot_paths.cpp)
  - Storage:
    - [benchmarks/storage/bench_batch_insert.cpp](benchmarks/storage/bench_batch_insert.cpp)
    - [benchmarks/storage/bench_blob_zstd.cpp](benchmarks/storage/bench_blob_zstd.cpp)
  - Monitoring:
    - [benchmarks/monitoring/bench_cycle_metrics.cpp](benchmarks/monitoring/bench_cycle_metrics.cpp)
  - Process:
    - [benchmarks/process/bench_cross_functional_end_to_end.cpp](benchmarks/process/bench_cross_functional_end_to_end.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep canonical targets stable.

- Exporters/Governance migration completed:
  - Exporters:
    - [benchmarks/exporters/bench_parquet_export.cpp](benchmarks/exporters/bench_parquet_export.cpp)
    - [benchmarks/exporters/bench_csv_export.cpp](benchmarks/exporters/bench_csv_export.cpp)
  - Governance:
    - [benchmarks/governance/bench_governance_policy_latency.cpp](benchmarks/governance/bench_governance_policy_latency.cpp)
    - [benchmarks/governance/bench_policy_evaluation.cpp](benchmarks/governance/bench_policy_evaluation.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep canonical targets stable.

- Scheduler/Search/Process video migration completed:
  - Scheduler:
    - [benchmarks/scheduler/bench_task_scheduler.cpp](benchmarks/scheduler/bench_task_scheduler.cpp)
  - Search:
    - [benchmarks/search/bench_approximate_radius_search.cpp](benchmarks/search/bench_approximate_radius_search.cpp)
  - Process:
    - [benchmarks/process/bench_video_processor.cpp](benchmarks/process/bench_video_processor.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep canonical targets stable.

- LLM/GPU follow-up migration completed:
  - LLM:
    - [benchmarks/llm/bench_lora_framework.cpp](benchmarks/llm/bench_lora_framework.cpp)
    - [benchmarks/llm/bench_multi_lora_fusion.cpp](benchmarks/llm/bench_multi_lora_fusion.cpp)
    - [benchmarks/llm/bench_lora_auto_binding.cpp](benchmarks/llm/bench_lora_auto_binding.cpp)
    - [benchmarks/llm/bench_rotary_embeddings.cpp](benchmarks/llm/bench_rotary_embeddings.cpp)
  - GPU:
    - [benchmarks/gpu/bench_fused_kernels.cpp](benchmarks/gpu/bench_fused_kernels.cpp)
    - [benchmarks/gpu/bench_fused_lora_kernels.cpp](benchmarks/gpu/bench_fused_lora_kernels.cpp)
    - [benchmarks/gpu/bench_mixed_precision_perf.cpp](benchmarks/gpu/bench_mixed_precision_perf.cpp)
    - [benchmarks/gpu/bench_vulkan_lora.cpp](benchmarks/gpu/bench_vulkan_lora.cpp)
    - [benchmarks/gpu/bench_active_vram_allocator.cpp](benchmarks/gpu/bench_active_vram_allocator.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep canonical targets stable.

- API/Base wasm migration completed:
  - API:
    - [benchmarks/api/bench_api_transport.cpp](benchmarks/api/bench_api_transport.cpp)
    - [benchmarks/api/bench_api_release_gates.cpp](benchmarks/api/bench_api_release_gates.cpp)
  - Base:
    - [benchmarks/base/bench_base_wasm_sandbox.cpp](benchmarks/base/bench_base_wasm_sandbox.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep canonical targets stable.

- Base/Security/LLM context migration completed:
  - Base:
    - [benchmarks/base/bench_advanced_patterns.cpp](benchmarks/base/bench_advanced_patterns.cpp)
    - [benchmarks/base/bench_comprehensive.cpp](benchmarks/base/bench_comprehensive.cpp)
    - [benchmarks/base/bench_compression.cpp](benchmarks/base/bench_compression.cpp)
    - [benchmarks/base/bench_edge_cases_comprehensive.cpp](benchmarks/base/bench_edge_cases_comprehensive.cpp)
    - [benchmarks/base/bench_hotspots_micro.cpp](benchmarks/base/bench_hotspots_micro.cpp)
  - Security:
    - [benchmarks/security/bench_encryption.cpp](benchmarks/security/bench_encryption.cpp)
  - LLM:
    - [benchmarks/llm/bench_extended_context.cpp](benchmarks/llm/bench_extended_context.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep canonical targets stable.

- Geo/Scheduler/Storage/Graph migration completed:
  - Geo:
    - [benchmarks/geo/bench_geojson_parse.cpp](benchmarks/geo/bench_geojson_parse.cpp)
  - Scheduler:
    - [benchmarks/scheduler/bench_thread_pool_saturation.cpp](benchmarks/scheduler/bench_thread_pool_saturation.cpp)
  - Storage:
    - [benchmarks/storage/bench_insert_profiling.cpp](benchmarks/storage/bench_insert_profiling.cpp)
  - Graph:
    - [benchmarks/graph/bench_gnn_embeddings.cpp](benchmarks/graph/bench_gnn_embeddings.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep canonical targets stable.

- Image analysis / Sharding migration completed:
  - Image analysis:
    - [benchmarks/image_analysis/bench_image_analysis.cpp](benchmarks/image_analysis/bench_image_analysis.cpp)
    - [benchmarks/image_analysis/bench_image_analysis_latency.cpp](benchmarks/image_analysis/bench_image_analysis_latency.cpp)
  - Sharding:
    - [benchmarks/sharding/bench_gossip_config.cpp](benchmarks/sharding/bench_gossip_config.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep canonical targets stable.

- Base/Storage/LLM batch migration completed:
  - LLM:
    - [benchmarks/llm/bench_lora_inline.cpp](benchmarks/llm/bench_lora_inline.cpp)
    - [benchmarks/llm/bench_lora_training.cpp](benchmarks/llm/bench_lora_training.cpp)
  - Base:
    - [benchmarks/base/bench_scalability_comprehensive.cpp](benchmarks/base/bench_scalability_comprehensive.cpp)
    - [benchmarks/base/bench_simd_distance.cpp](benchmarks/base/bench_simd_distance.cpp)
  - Storage:
    - [benchmarks/storage/bench_simple_insert_test.cpp](benchmarks/storage/bench_simple_insert_test.cpp)
    - [benchmarks/storage/bench_ssm_phase0_baseline.cpp](benchmarks/storage/bench_ssm_phase0_baseline.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) updated to keep canonical targets stable.

- Content/Base/UserStorageEncrypted migration completed:
  - Content:
    - [benchmarks/content/bench_text_extraction.cpp](benchmarks/content/bench_text_extraction.cpp)
  - Base:
    - [benchmarks/base/bench_latency_comprehensive.cpp](benchmarks/base/bench_latency_comprehensive.cpp)
    - [benchmarks/base/bench_gorilla_codec.cpp](benchmarks/base/bench_gorilla_codec.cpp)
  - UserStorageEncrypted:
    - [benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp](benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp)
  - [benchmarks/CMakeLists.txt](benchmarks/CMakeLists.txt) and [benchmarks/content/CMakeLists.txt](benchmarks/content/CMakeLists.txt) updated to keep canonical targets stable.

- Historical docs migration completed (copy + redirect stubs):
  - [benchmarks/docs/BENCHMARK_STATUS.md](benchmarks/docs/BENCHMARK_STATUS.md)
  - [benchmarks/docs/DOCKER_BENCHMARKS_STATUS_REPORT.md](benchmarks/docs/DOCKER_BENCHMARKS_STATUS_REPORT.md)

- Compatibility matrix added in [benchmarks/README.md](benchmarks/README.md).

Remaining high-impact step:

- Continue root `bench_*.cpp` migration domain-by-domain (gpu, llm, query, graph, sharding, etc.) with incremental CMake updates and build verification.
