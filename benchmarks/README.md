> **Build:** `cmake --preset nightly-bench-sweep && cmake --build --preset nightly-bench-sweep`

# ThemisDB Benchmarks (`benchmarks/`)

Benchmark-Sammlung für Performance-, Skalierungs- und Integrationsmessungen.

## Struktur

- C++ Benchmarks: `bench_*.cpp`
- Python/Script-Orchestrierung: `*.py`, `*.sh`, `*.ps1`
- Teilbereiche: `ai/`, `analytics/`, `aql/`, `core/`, `query/`, `rag/`, `server/`, `transaction/`, `tpc/`, `ycsb/`, `mmdb/`, `ann/`, `ldbc/`, `chimera/`
- Roadmap-Scaffolds für die neuen Architektur-Epics: `epic1_retrieval/`, `epic2_evaluation/`, `epic3_distributed_tensor/`

## Reproduzierbare Basiskommandos

```bash
cmake --preset nightly-bench-sweep
cmake --build --preset nightly-bench-sweep
ctest --preset linux-release
```

Registrierungs-Audit:

```bash
python3 benchmarks/scripts/audit_benchmark_registration.py
```

Direkte Script-Läufe (Beispiele):

```bash
python3 benchmarks/hardware_scaling_benchmark.py --help
python3 benchmarks/run_benchmark_orchestrator.py --help
bash benchmarks/run_all_benchmarks.sh
```

## Wave 3: Full-Function / Production-nahe Suite

W3 deckt priorisierte End-to-End-nahe Critical-Workloads mit reproduzierbaren
Profilen (`read-heavy`, `write-heavy`, `mixed`) sowie Scale-/Stress-Dimensionen
(`small|medium|large`, Parallelität, Request-Mix) ab.

Run (produktionsnah, mit vorhandenen Binaries):

```bash
python3 benchmarks/wave3_benchmark_suite.py run \
  --benchmark-bin-dir <build>/bin \
  --profiles-file benchmarks/wave3_workload_profiles.json \
  --output benchmarks/benchmark_results/wave3_current.json
```

Vergleich + Guardrails (Regressionserkennung):

```bash
python3 benchmarks/wave3_benchmark_suite.py compare \
  --baseline benchmarks/benchmark_results/wave3_baseline.json \
  --current benchmarks/benchmark_results/wave3_current.json \
  --output benchmarks/benchmark_results/wave3_compare.json
```

## Installation

Benchmark-Buildartefakte werden über den CMake-Preset `nightly-bench-sweep` erzeugt.

## Usage

Ausführung erfolgt entweder über erzeugte Benchmark-Binaries (`bench_*`) oder über Orchestrierungs-Skripte in diesem Verzeichnis.

## CMake-Registrierungspolicy (Build/CTest-konform)

- Verbindlicher Standard: `BENCHMARK_STANDARDS.md`
- Historische Summary/Report-Dateien unter `benchmarks/docs/` sind Kontext,
  aber nicht kanonisch fuer aktuelle Benchmark-Regeln.
- Benchmarks, die synthetische oder emulierte Workloads statt echter Produktions-Implementierungen verwenden, müssen am Dateianfang einen `SIMULATION NOTE`-Block tragen. Die Markierung muss Zweck, Aktivierung, Produktionsdelta und Entfernungsplan benennen.
- **Top-Level Benchmarks** (`benchmarks/bench*.cpp`, `benchmark_*.cpp`) bleiben nur für noch nicht migrierte oder bewusst globale Targets im Root erhalten.
- **Modul-Benchmarks** folgen dem Muster `benchmarks/<module>/CMakeLists.txt` (analog `tests/<module>/...`), z. B. `benchmarks/ai/`, `benchmarks/analytics/`, `benchmarks/aql/`, `benchmarks/core/`, `benchmarks/query/`, `benchmarks/rag/`, `benchmarks/server/`, `benchmarks/transaction/`, `benchmarks/performance_optimizations/`.
- **Manuelle/Script-basierte Benchmarks** (`*.py`, `*.sh`, `*.ps1`) sind absichtlich **nicht** als CTest-Gates registriert und werden über ihre Runner ausgeführt.
- **Intentionale Exclusions** bleiben explizit dokumentiert (aktuell: `performance_optimizations/phase2/benchmark_phase2.cpp`, Placeholder/disabled).

Hinweis zur CTest-Integration: Benchmarks sind standardmäßig keine obligatorischen CTest-Checks; sie werden über dedizierte Benchmark-Presets/Runner und Build-Targets ausgeführt.

## Kompatibilitaetsmatrix (Cleanup-Transition)

| Altpfad | Neupfad | Status |
|---|---|---|
| `benchmarks/bench_crud.cpp` | `benchmarks/storage/bench_crud.cpp` | migriert |
| `benchmarks/bench_mvcc.cpp` | `benchmarks/storage/bench_mvcc.cpp` | migriert |
| `benchmarks/bench_wal_stress.cpp` | `benchmarks/storage/bench_wal_stress.cpp` | migriert |
| `benchmarks/bench_adaptive_query_cache.cpp` | `benchmarks/query/bench_adaptive_query_cache.cpp` | migriert |
| `benchmarks/bench_adaptive_query_compilation.cpp` | `benchmarks/query/bench_adaptive_query_compilation.cpp` | migriert |
| `benchmarks/bench_continuous_query.cpp` | `benchmarks/query/bench_continuous_query.cpp` | migriert |
| `benchmarks/bench_aql_geo_filter.cpp` | `benchmarks/aql/bench_aql_geo_filter.cpp` | migriert |
| `benchmarks/bench_hybrid_aql_sugar.cpp` | `benchmarks/aql/bench_hybrid_aql_sugar.cpp` | migriert |
| `benchmarks/bench_pagerank.cpp` | `benchmarks/graph/bench_pagerank.cpp` | migriert |
| `benchmarks/bench_spatial_index.cpp` | `benchmarks/geo/bench_spatial_index.cpp` | migriert |
| `benchmarks/bench_spatial_join.cpp` | `benchmarks/geo/bench_spatial_join.cpp` | migriert |
| `benchmarks/bench_changefeed_throughput.cpp` | `benchmarks/cdc/bench_changefeed_throughput.cpp` | migriert |
| `benchmarks/bench_shard_resource_manager.cpp` | `benchmarks/sharding/bench_shard_resource_manager.cpp` | migriert |
| `benchmarks/bench_shard_routing.cpp` | `benchmarks/sharding/bench_shard_routing.cpp` | migriert |
| `benchmarks/bench_ann_cpu_gpu_dispatch.cpp` | `benchmarks/ann/bench_ann_cpu_gpu_dispatch.cpp` | migriert |
| `benchmarks/bench_embedded_llm.cpp` | `benchmarks/llm/bench_embedded_llm.cpp` | migriert |
| `benchmarks/bench_multi_gpu_scaling.cpp` | `benchmarks/gpu/bench_multi_gpu_scaling.cpp` | migriert |
| `benchmarks/bench_multi_gpu_lora_advanced.cpp` | `benchmarks/gpu/bench_multi_gpu_lora_advanced.cpp` | migriert |
| `benchmarks/bench_lora_gpu.cpp` | `benchmarks/gpu/bench_lora_gpu.cpp` | migriert |
| `benchmarks/bench_qlora_gpu_kernels.cpp` | `benchmarks/gpu/bench_qlora_gpu_kernels.cpp` | migriert |
| `benchmarks/bench_tensor_cpu_gpu_dispatch.cpp` | `benchmarks/gpu/bench_tensor_cpu_gpu_dispatch.cpp` | migriert |
| `benchmarks/bench_vector_search.cpp` | `benchmarks/ann/bench_vector_search.cpp` | migriert |
| `benchmarks/bench_vector_prefilter.cpp` | `benchmarks/ann/bench_vector_prefilter.cpp` | migriert |
| `benchmarks/bench_hybrid_vector_geo.cpp` | `benchmarks/ann/bench_hybrid_vector_geo.cpp` | migriert |
| `benchmarks/bench_vector_compression_lossless.cpp` | `benchmarks/ann/bench_vector_compression_lossless.cpp` | migriert |
| `benchmarks/bench_binary_quantization.cpp` | `benchmarks/ann/bench_binary_quantization.cpp` | migriert |
| `benchmarks/bench_product_quantization.cpp` | `benchmarks/ann/bench_product_quantization.cpp` | migriert |
| `benchmarks/bench_residual_quantization.cpp` | `benchmarks/ann/bench_residual_quantization.cpp` | migriert |
| `benchmarks/bench_learned_quantization.cpp` | `benchmarks/ann/bench_learned_quantization.cpp` | migriert |
| `benchmarks/bench_lossy_vs_lossless.cpp` | `benchmarks/ann/bench_lossy_vs_lossless.cpp` | migriert |
| `benchmarks/bench_data_transfer.cpp` | `benchmarks/gpu/bench_data_transfer.cpp` | migriert |
| `benchmarks/bench_backend_comparison.cpp` | `benchmarks/gpu/bench_backend_comparison.cpp` | migriert |
| `benchmarks/bench_cuda_vs_cpu.cpp` | `benchmarks/gpu/bench_cuda_vs_cpu.cpp` | migriert |
| `benchmarks/bench_ycsb.cpp` | `benchmarks/ycsb/bench_ycsb.cpp` | migriert |
| `benchmarks/bench_tpcc.cpp` | `benchmarks/tpc/bench_tpcc.cpp` | migriert |
| `benchmarks/bench_tpch.cpp` | `benchmarks/tpc/bench_tpch.cpp` | migriert |
| `benchmarks/bench_olap_performance.cpp` | `benchmarks/analytics/bench_olap_performance.cpp` | migriert |
| `benchmarks/bench_olap_analytics.cpp` | `benchmarks/analytics/bench_olap_analytics.cpp` | migriert |
| `benchmarks/bench_importer_throughput.cpp` | `benchmarks/importers/bench_importer_throughput.cpp` | migriert |
| `benchmarks/bench_embedding_cache_performance.cpp` | `benchmarks/cache/bench_embedding_cache_performance.cpp` | migriert |
| `benchmarks/bench_mmdb.cpp` | `benchmarks/mmdb/bench_mmdb.cpp` | migriert |
| `benchmarks/bench_random_access_prefetch.cpp` | `benchmarks/performance/bench_random_access_prefetch.cpp` | migriert |
| `benchmarks/bench_snapshot_manager.cpp` | `benchmarks/maintenance/bench_snapshot_manager.cpp` | migriert |
| `benchmarks/bench_update_pipeline.cpp` | `benchmarks/maintenance/bench_update_pipeline.cpp` | migriert |
| `benchmarks/bench_metrics_collector.cpp` | `benchmarks/maintenance/bench_metrics_collector.cpp` | migriert |
| `benchmarks/bench_lock_contention.cpp` | `benchmarks/maintenance/bench_lock_contention.cpp` | migriert |
| `benchmarks/bench_saga_compensation.cpp` | `benchmarks/maintenance/bench_saga_compensation.cpp` | migriert |
| `benchmarks/bench_sanity.cpp` | `benchmarks/maintenance/bench_sanity.cpp` | migriert |
| `benchmarks/bench_security.cpp` | `benchmarks/security/bench_security.cpp` | migriert |
| `benchmarks/bench_hot_reload_manager.cpp` | `benchmarks/maintenance/bench_hot_reload_manager.cpp` | migriert |
| `benchmarks/bench_di_logging.cpp` | `benchmarks/observability/bench_di_logging.cpp` | migriert |
| `benchmarks/bench_compliance_security_governance.cpp` | `benchmarks/security/bench_compliance_security_governance.cpp` | migriert |
| `benchmarks/bench_plugin_hot_plug.cpp` | `benchmarks/plugins/bench_plugin_hot_plug.cpp` | migriert |
| `benchmarks/bench_plugin_system.cpp` | `benchmarks/plugins/bench_plugin_system.cpp` | migriert |
| `benchmarks/bench_docker_raid_comprehensive.cpp` | `benchmarks/maintenance/bench_docker_raid_comprehensive.cpp` | migriert |
| `benchmarks/bench_hot_reload_manager.cpp` | `benchmarks/maintenance/bench_hot_reload_manager.cpp` | migriert |
| `benchmarks/bench_di_logging.cpp` | `benchmarks/observability/bench_di_logging.cpp` | migriert |
| `benchmarks/bench_auto_buffers.cpp` | `benchmarks/base/bench_auto_buffers.cpp` | migriert |
| `benchmarks/bench_base_hot_paths.cpp` | `benchmarks/base/bench_base_hot_paths.cpp` | migriert |
| `benchmarks/bench_batch_insert.cpp` | `benchmarks/storage/bench_batch_insert.cpp` | migriert |
| `benchmarks/bench_blob_zstd.cpp` | `benchmarks/storage/bench_blob_zstd.cpp` | migriert |
| `benchmarks/bench_cycle_metrics.cpp` | `benchmarks/monitoring/bench_cycle_metrics.cpp` | migriert |
| `benchmarks/bench_cross_functional_end_to_end.cpp` | `benchmarks/process/bench_cross_functional_end_to_end.cpp` | migriert |
| `benchmarks/bench_parquet_export.cpp` | `benchmarks/exporters/bench_parquet_export.cpp` | migriert |
| `benchmarks/bench_csv_export.cpp` | `benchmarks/exporters/bench_csv_export.cpp` | migriert |
| `benchmarks/bench_policy_evaluation.cpp` | `benchmarks/governance/bench_policy_evaluation.cpp` | migriert |
| `benchmarks/bench_governance_policy_latency.cpp` | `benchmarks/governance/bench_governance_policy_latency.cpp` | migriert |
| `benchmarks/bench_task_scheduler.cpp` | `benchmarks/scheduler/bench_task_scheduler.cpp` | migriert |
| `benchmarks/bench_approximate_radius_search.cpp` | `benchmarks/search/bench_approximate_radius_search.cpp` | migriert |
| `benchmarks/bench_video_processor.cpp` | `benchmarks/process/bench_video_processor.cpp` | migriert |
| `benchmarks/bench_lora_framework.cpp` | `benchmarks/llm/bench_lora_framework.cpp` | migriert |
| `benchmarks/bench_multi_lora_fusion.cpp` | `benchmarks/llm/bench_multi_lora_fusion.cpp` | migriert |
| `benchmarks/bench_lora_auto_binding.cpp` | `benchmarks/llm/bench_lora_auto_binding.cpp` | migriert |
| `benchmarks/bench_rotary_embeddings.cpp` | `benchmarks/llm/bench_rotary_embeddings.cpp` | migriert |
| `benchmarks/bench_fused_kernels.cpp` | `benchmarks/gpu/bench_fused_kernels.cpp` | migriert |
| `benchmarks/bench_fused_lora_kernels.cpp` | `benchmarks/gpu/bench_fused_lora_kernels.cpp` | migriert |
| `benchmarks/bench_mixed_precision_perf.cpp` | `benchmarks/gpu/bench_mixed_precision_perf.cpp` | migriert |
| `benchmarks/bench_vulkan_lora.cpp` | `benchmarks/gpu/bench_vulkan_lora.cpp` | migriert |
| `benchmarks/bench_active_vram_allocator.cpp` | `benchmarks/gpu/bench_active_vram_allocator.cpp` | migriert |
| `benchmarks/bench_api_transport.cpp` | `benchmarks/api/bench_api_transport.cpp` | migriert |
| `benchmarks/bench_api_release_gates.cpp` | `benchmarks/api/bench_api_release_gates.cpp` | migriert |
| `benchmarks/bench_base_wasm_sandbox.cpp` | `benchmarks/base/bench_base_wasm_sandbox.cpp` | migriert |
| `benchmarks/bench_advanced_patterns.cpp` | `benchmarks/base/bench_advanced_patterns.cpp` | migriert |
| `benchmarks/bench_comprehensive.cpp` | `benchmarks/base/bench_comprehensive.cpp` | migriert |
| `benchmarks/bench_compression.cpp` | `benchmarks/base/bench_compression.cpp` | migriert |
| `benchmarks/bench_edge_cases_comprehensive.cpp` | `benchmarks/base/bench_edge_cases_comprehensive.cpp` | migriert |
| `benchmarks/bench_hotspots_micro.cpp` | `benchmarks/base/bench_hotspots_micro.cpp` | migriert |
| `benchmarks/bench_encryption.cpp` | `benchmarks/security/bench_encryption.cpp` | migriert |
| `benchmarks/bench_extended_context.cpp` | `benchmarks/llm/bench_extended_context.cpp` | migriert |
| `benchmarks/bench_geojson_parse.cpp` | `benchmarks/geo/bench_geojson_parse.cpp` | migriert |
| `benchmarks/bench_thread_pool_saturation.cpp` | `benchmarks/scheduler/bench_thread_pool_saturation.cpp` | migriert |
| `benchmarks/bench_insert_profiling.cpp` | `benchmarks/storage/bench_insert_profiling.cpp` | migriert |
| `benchmarks/bench_gnn_embeddings.cpp` | `benchmarks/graph/bench_gnn_embeddings.cpp` | migriert |
| `benchmarks/bench_image_analysis.cpp` | `benchmarks/image_analysis/bench_image_analysis.cpp` | migriert |
| `benchmarks/bench_image_analysis_latency.cpp` | `benchmarks/image_analysis/bench_image_analysis_latency.cpp` | migriert |
| `benchmarks/bench_gossip_config.cpp` | `benchmarks/sharding/bench_gossip_config.cpp` | migriert |
| `benchmarks/bench_lora_inline.cpp` | `benchmarks/llm/bench_lora_inline.cpp` | migriert |
| `benchmarks/bench_lora_training.cpp` | `benchmarks/llm/bench_lora_training.cpp` | migriert |
| `benchmarks/bench_ssm_phase0_baseline.cpp` | `benchmarks/storage/bench_ssm_phase0_baseline.cpp` | migriert |
| `benchmarks/bench_scalability_comprehensive.cpp` | `benchmarks/base/bench_scalability_comprehensive.cpp` | migriert |
| `benchmarks/bench_simd_distance.cpp` | `benchmarks/base/bench_simd_distance.cpp` | migriert |
| `benchmarks/bench_simple_insert_test.cpp` | `benchmarks/storage/bench_simple_insert_test.cpp` | migriert |
| `benchmarks/bench_text_extraction.cpp` | `benchmarks/content/bench_text_extraction.cpp` | migriert |
| `benchmarks/bench_latency_comprehensive.cpp` | `benchmarks/base/bench_latency_comprehensive.cpp` | migriert |
| `benchmarks/bench_gorilla_codec.cpp` | `benchmarks/base/bench_gorilla_codec.cpp` | migriert |
| `benchmarks/bench_user_storage_mount_latency.cpp` | `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp` | migriert |
| `benchmarks/bench_ai_plugin_generator.cpp` | `benchmarks/ai/bench_ai_plugin_generator.cpp` | migriert |
| `benchmarks/benchmark_distributed_hybrid_search.cpp` | `benchmarks/search/benchmark_distributed_hybrid_search.cpp` | migriert |
| `benchmarks/benchmark_hybrid_search.cpp` | `benchmarks/search/benchmark_hybrid_search.cpp` | migriert |
| `benchmarks/benchmark_image_analysis.cpp` | `benchmarks/image_analysis/benchmark_image_analysis.cpp` | migriert |
| `benchmarks/bench_flash_attention.cpp` | `benchmarks/llm/bench_flash_attention.cpp` | migriert |
| `benchmarks/bench_hsm_provider.cpp` | `benchmarks/security/bench_hsm_provider.cpp` | migriert |
| `benchmarks/bench_legal_lora_pipeline.cpp` | `benchmarks/llm/bench_legal_lora_pipeline.cpp` | migriert |
| `benchmarks/bench_locality_aware_router.cpp` | `benchmarks/sharding/bench_locality_aware_router.cpp` | migriert |
| `benchmarks/bench_multitask_lora_training.cpp` | `benchmarks/llm/bench_multitask_lora_training.cpp` | migriert |
| `benchmarks/bench_phase1_flash_attention.cpp` | `benchmarks/llm/bench_phase1_flash_attention.cpp` | migriert |
| `benchmarks/bench_phase3_optimization.cpp` | `benchmarks/performance_optimizations/bench_phase3_optimization.cpp` | migriert |
| `benchmarks/bench_raid_lora.cpp` | `benchmarks/llm/bench_raid_lora.cpp` | migriert |
| `benchmarks/bench_v1_3_0_features.cpp` | `benchmarks/performance_optimizations/bench_v1_3_0_features.cpp` | migriert |
| `benchmarks/bench_v1_3_4_optimizations.cpp` | `benchmarks/performance_optimizations/bench_v1_3_4_optimizations.cpp` | migriert |
| `benchmarks/llm_bench.cpp` | `benchmarks/llm/llm_bench.cpp` | migriert |
| `benchmarks/kernel_block_size_bench.cpp` | `benchmarks/gpu/kernel_block_size_bench.cpp` | migriert |
| `benchmarks/bounded_kernel_validation.cpp` | `benchmarks/performance_optimizations/bounded_kernel_validation.cpp` | migriert |
| `benchmarks/docs/BENCHMARK_STATUS.md` | `benchmarks/docs/historical/2026-08/BENCHMARK_STATUS.md` | archiviert (Redirect-Stub am Altpfad) |
| `benchmarks/docs/DOCKER_BENCHMARKS_STATUS_REPORT.md` | `benchmarks/docs/historical/2026-08/DOCKER_BENCHMARKS_STATUS_REPORT.md` | archiviert (Redirect-Stub am Altpfad) |

Hinweis: Build-Targetnamen fuer den Storage-Pilot bleiben unveraendert (`bench_crud`, `bench_mvcc`, `bench_wal_stress`), obwohl die Quelldateien modul-spezifisch verschoben wurden.

## Methodik-Hinweis

Historische Ergebniszahlen in älteren Reports bleiben als Historie erhalten. Für aktuelle Aussagen sind reproduzierbare Kommandos, Parameter und Artefakte (`results/`, JSON/CSV-Ausgaben) maßgeblich.

## Messhygiene (Welle 1 / PR-A)

Priorisierte C++-Benchmarks wurden im Rahmen der Benchmark-Härtung (Welle 1) standardisiert. Die verbindlichen Regeln für reproduzierbare Messungen sind in [`MEASUREMENT_HYGIENE.md`](MEASUREMENT_HYGIENE.md) dokumentiert.

Kurzfassung der Pflichtregeln:
- **Fester RNG-Seed**: `kCanonicalRngSeed = 42` (aus `bench_fixtures.h`) — kein `std::random_device`.
- **OS-Temp-Pfade**: DB-Artefakte unter `std::filesystem::temp_directory_path()` mit Timestamp-Suffix.
- **Setup/Mess-Trennung**: Setup-Code in `SetUp()`, nicht im Mess-Loop.
- **`UseRealTime()`**: Pflicht bei I/O-gebundenen Benchmarks.
- **Konsistente Metriken**: `SetItemsProcessed()`, `qps`-Counter, Dataset-Größen.

## Navigation

- Bereichsindex: [`INDEX.md`](INDEX.md)
- Roadmap: [`ROADMAP.md`](ROADMAP.md)
- Zielstruktur-Plan (Cleanup): [`TARGET_STRUCTURE_PLAN.md`](TARGET_STRUCTURE_PLAN.md)
- Benchmark-Standard (kanonisch): [`BENCHMARK_STANDARDS.md`](BENCHMARK_STANDARDS.md)
- Messhygiene: [`MEASUREMENT_HYGIENE.md`](MEASUREMENT_HYGIENE.md)
- Erweiterungen: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md)
- Mapping Alt -> Kanonisch (Tests/Benchmarks): [`../tests/TEST_BENCHMARK_DOC_CANONICAL_MAPPING.md`](../tests/TEST_BENCHMARK_DOC_CANONICAL_MAPPING.md)
