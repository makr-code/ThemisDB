# ThemisDB Vector Search Module

<!-- Status: PRODUCTION_CANDIDATE | Phase 1-3 complete | validated: 2026-08-10 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The Vector Search module provides high-performance similarity search and nearest-neighbor retrieval infrastructure for embedding-based workloads in ThemisDB, including approximate nearest neighbors (ANN), indexing strategies, and distance metric support.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| ann_index.cpp | approximate nearest neighbor index construction and maintenance |
| distance_metrics.cpp | distance computation and similarity scoring support |
| index_builder.cpp | index building and optimization paths |
| query_executor.cpp | vector query planning and execution |
| index_partitioner.cpp | partitioning and sharding strategies for distributed search |
| recall_optimizer.cpp | recall tuning and accuracy calibration |
| vector_quantization.cpp | quantization and compression for memory efficiency |
| search_cache.cpp | caching and prefetch for repeated search patterns |

## Scope

In scope:
- ANN indexing and query execution surfaces
- distance metrics and similarity scoring
- distributed partitioning and query coordination
- recall optimization and memory efficiency
- vector search observability and SLO monitoring

Out of scope:
- core embedding model training or inference
- non-vector search query planning
- business-domain information retrieval logic outside search runtime boundaries

## Runtime Behavior and Limits

- behavior depends on configured indexing algorithm, distance metric, and quantization policy
- search operations return ranked results with distance/similarity scores
- performance depends on index structure, query distribution, and hardware availability

## Implementation Integration

The vector search module provides high-level abstractions for similarity search. Core algorithm implementations (HNSW, IVF) and distance computation kernels are **integrated from**:
- `include/index/` — HNSW/IVF algorithm foundations and index structures
- `include/utils/` — SIMD-accelerated distance computation helpers
- `include/storage/` — Planned index persistence (Phase 6)

Tests and benchmarks verify the complete end-to-end vector search pipeline:
- Integration tests: `tests/integration/test_vector_search_soak.cpp` (60-sec durability, recall ≥ 0.9)
- Stress tests: `tests/vector_search/test_vector_search_highcardinality_stress.cpp` (10k vectors, concurrent operations)
- Benchmarks: `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp`, `benchmarks/ann/bench_vector_search.cpp`, `benchmarks/search/bench_vector_search_gates.cpp`

## Documentation References

- **Implementation Phases & Status:** [ROADMAP.md](ROADMAP.md)
- **Delivered Artefacts & History:** [CHANGELOG.md](CHANGELOG.md)
- **Future Planning:** [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md)
- **Architecture & Design:** [ARCHITECTURE.md](ARCHITECTURE.md)
- **Module Audit & Compliance:** [AUDIT.md](AUDIT.md)
- **Security & Threat Model:** [SECURITY.md](SECURITY.md)
- **Operational Constraints:** [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md)
- **Performance Baselines:** [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md)
- **Known Gaps & Limitations:** [MODULE_GAPS.md](MODULE_GAPS.md)
