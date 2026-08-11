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

## Sourcecode Verification (Module: vector_search/readme)

- Verified files:
  - src/vector_search/ann_index.cpp
  - src/vector_search/distance_metrics.cpp
  - src/vector_search/index_builder.cpp
  - src/vector_search/query_executor.cpp
  - src/vector_search/index_partitioner.cpp
  - src/vector_search/recall_optimizer.cpp
  - src/vector_search/vector_quantization.cpp
  - src/vector_search/search_cache.cpp
- Verified behavior surfaces:
  - ANN indexing and efficient similarity search paths
  - distance computation and scoring
  - partitioning and distributed query coordination
  - recall tuning and memory optimization
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical completion remains in CHANGELOG.md
