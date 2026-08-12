## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


# EPIC 2.2 Benchmark Framework

<!-- Status: implemented | Issue #5438 | validated: 2026-07-15 -->

## Summary

Benchmark matrix and scenario definitions for the layered retrieval stack.
Closes Issue #5438.

## Implemented Repository Surfaces

| File | Purpose |
|---|---|
| `src/evaluation/include/benchmark_matrix.h` | Core data structures: `BenchmarkScenario`, `BenchmarkDimension`, `BenchmarkEdgeCase`, `BenchmarkResult`, `BenchmarkEntry`, `BenchmarkMatrix` |
| `src/evaluation/src/benchmark_matrix.cc` | `BenchmarkMatrix` implementation (record, lookup, slice, compare, bestScenario, invalidation) |
| `tests/epic2_evaluation/benchmark_matrix_test.cc` | 35+ GTest unit tests covering all phases (record/lookup, edge cases, slices, coverage filter, comparison, move semantics) |
| `benchmarks/epic2_evaluation/benchmark_matrix_bench.cc` | Google Benchmark scenarios for HNSW, DiskANN, Tensor Mid-Layer, Graph validation, LLM/LoRA, commit overhead, snapshot rebuild, CPU/GPU break-even |

## Scenario Taxonomy

All scenarios are modelled as `BenchmarkScenario` enumerators:

| Scenario | Coverage |
|---|---|
| `HNSW_ANN_ONLY` | Pure HNSW ANN retrieval baseline |
| `DISKANN_ANN_ONLY` | DiskANN disk-resident graph index baseline |
| `ANN_TENSOR` | ANN + Tensor compression/routing |
| `ANN_TENSOR_DYNAMIC_UPDATE` | ANN + Tensor with active update worker |
| `ANN_TENSOR_SNAPSHOT_REBUILT` | ANN + Tensor post-snapshot-rebuild |
| `ANN_TENSOR_PATCH_REFIT` | ANN + Tensor after partial refit |
| `ANN_TENSOR_GRAPH` | Full stack: ANN + Tensor + Graph validation |
| `DIRECT_EXACT_GRAPH` | Direct exact-graph traversal without ANN pre-filter |
| `SUMMARY_FIRST_DISTRIBUTED` | Summary-first distributed shard routing |
| `DISTRIBUTED_EXACT_LOAD` | Direct exact shard load without summary pre-filter |
| `LLM_FULL_PROMPT` | LLM inference with full-context prompt |
| `LLM_TENSOR_COMPRESSED` | LLM inference with tensor-compressed evidence |
| `LORA_INFERENCE` | LoRA fine-tuned adapter applied at inference time |
| `COMMIT_OVERHEAD` | Transactional commit overhead vs. read path |
| `CPU_ONLY` | CPU-side baseline for GPU break-even |
| `GPU_ACCELERATED` | GPU-accelerated path for break-even comparison |

## Evaluation Dimensions

All dimensions are modelled as `BenchmarkDimension` enumerators, aligned with
`EVALUATION_FRAMEWORK.md` sections 3.1–3.6:

| Dimension | Axis |
|---|---|
| `RECALL_AT_K` | Retrieval quality |
| `PRECISION_AT_K` | Retrieval quality |
| `CANDIDATE_REDUCTION` | Retrieval quality |
| `QUERY_LATENCY_MS` | Latency |
| `REBUILD_LATENCY_MS` | Latency (snapshot) |
| `COMMIT_OVERHEAD_MS` | Latency (write path) |
| `QPS` | Throughput |
| `UPDATE_THROUGHPUT` | Throughput (tensor update worker) |
| `MEMORY_MB` | Cost / memory |
| `INDEX_BUILD_TIME_S` | Cost / build time |
| `COMPRESSION_RATIO` | Compression quality |
| `APPROXIMATION_LOSS` | Compression quality |
| `FAITHFULNESS_SCORE` | LLM / LoRA quality |
| `HALLUCINATION_RATE` | LLM / LoRA quality |
| `PROMPT_TOKEN_COUNT` | LLM / LoRA cost |
| `SHARD_FAN_OUT` | Distributed efficiency |
| `BYTES_TRANSFERRED` | Distributed efficiency |
| `GPU_SPEEDUP_FACTOR` | Break-even |

## Edge-Case Coverage

`BenchmarkEdgeCase` flags annotate anomalous result cells:

| Flag | Condition |
|---|---|
| `STALE_ARTIFACT` | Stale or invalidated artifact present at measurement time |
| `SHARD_SUMMARY_MISMATCH` | Distributed shard summary out of sync |
| `RESIDUAL_PLANNER_FALLBACK` | Query planner fell back to residual-sensitive mode |
| `UNMEASURED_COMBINATION` | New dataset/workload; no prior measurement |
| `INSUFFICIENT_METRIC_DATA` | Fewer than minimum required samples |

## Seven-Phase Gate

- [x] Phase 1: Design / API contract — scenario taxonomy, dimension taxonomy, edge-case flags
- [x] Phase 2: Core implementation — `BenchmarkMatrix` with record, lookup, slice, compare, bestScenario, invalidation
- [x] Phase 3: Error handling / edge cases — vacuous-insert guard, edge-case flags, stale/mismatch/fallback handling
- [x] Phase 4: Tests — 35+ GTest cases in `benchmark_matrix_test.cc`
- [x] Phase 5: Performance / hardening — Google Benchmark suite; HNSW/DiskANN/Tensor/Graph/LLM-LoRA simulations
- [x] Phase 6: Documentation — this document, Doxygen comments in all public APIs
- [ ] Phase 7: CI integration — wire `benchmark_matrix_bench` into CI smoke-test pipeline

## Benchmark Groups (Phase 5)

| Group | File | Scenarios Covered |
|---|---|---|
| `BM_Matrix_*` | `benchmark_matrix_bench.cc` | Matrix overhead (record, lookup, fill, slice, compare, serialise, invalidate) |
| `BM_ANN_HNSW_Simulation` | `benchmark_matrix_bench.cc` | `HNSW_ANN_ONLY` |
| `BM_ANN_DiskANN_Simulation` | `benchmark_matrix_bench.cc` | `DISKANN_ANN_ONLY` |
| `BM_Tensor_Update_Worker` | `benchmark_matrix_bench.cc` | `ANN_TENSOR_DYNAMIC_UPDATE` |
| `BM_Snapshot_Rebuild` | `benchmark_matrix_bench.cc` | `ANN_TENSOR_SNAPSHOT_REBUILT` |
| `BM_Commit_Overhead` | `benchmark_matrix_bench.cc` | `COMMIT_OVERHEAD` |
| `BM_Query_Routing_*` | `benchmark_matrix_bench.cc` | `SUMMARY_FIRST_DISTRIBUTED`, `DISTRIBUTED_EXACT_LOAD` |
| `BM_Graph_Validation` | `benchmark_matrix_bench.cc` | `ANN_TENSOR_GRAPH` |
| `BM_LLM_LoRA_Inference` | `benchmark_matrix_bench.cc` | `LORA_INFERENCE` |
| `BM_BreakEven_CPU` | `benchmark_matrix_bench.cc` | `CPU_ONLY` |
| `BM_Matrix_EndToEnd` | `benchmark_matrix_bench.cc` | Full matrix + planner query smoke-test |

## Production Readiness Checklist

- [x] Matrix is in source, documented with Doxygen, and self-contained (no external deps)
- [x] Dynamic tensor update workloads are covered (`ANN_TENSOR_DYNAMIC_UPDATE`, `BM_Tensor_Update_Worker`)
- [x] CPU/GPU break-even scenarios documented (`CPU_ONLY`, `GPU_ACCELERATED`, `BM_BreakEven_CPU`)
- [x] Fresh/stale comparison covered via `STALE_ARTIFACT` edge-case flag
- [x] Distributed shard mismatch covered via `SHARD_SUMMARY_MISMATCH` flag
- [ ] Real hnswlib/DiskANN bindings replace simulation benchmarks (Target: v2.0.0 / Q4 2026)
- [ ] GPU break-even benchmark wired to real CUDA kernel (Target: v2.0.0 / Q4 2026)
- [ ] CI pipeline smoke-test added for `benchmark_matrix_bench` (Phase 7)

## Known Issues & Limitations

- ANN simulation benchmarks use linear scan; not a substitute for real hnswlib/DiskANN throughput numbers.
- GPU break-even benchmark is CPU-only in CI; requires CUDA runner for real numbers.
- Large distributed GPU clusters not benchmarkable without multi-node CI.

## Breaking Changes

- None.  `BenchmarkMatrix` is a new class; no existing code is modified.

## References

- `EVALUATION_FRAMEWORK.md` — evaluation axes and metrics definitions
- `HARDWARE_REQUIREMENTS.md` — hardware constraints for benchmarks
- `docs/IMPLEMENTATION_ROADMAP.md` — EPIC 2 delivery tracking
- Issue #5438 — original issue

