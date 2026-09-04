# ThemisDB Index Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The index module provides core indexing and retrieval acceleration for ThemisDB across vector, secondary, spatial, graph, and advanced index pathways, including GPU-aware execution, compression, quantization, and operational index lifecycle controls.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| index_manager.cpp | unified index coordination surfaces |
| vector_index.cpp | ANN/vector index lifecycle and search behavior |
| advanced_vector_index.cpp | advanced vector indexing extensions |
| gpu_vector_index.cpp | GPU vector index routing and execution |
| gpu_vector_index_vulkan.cpp | Vulkan-specific GPU vector path |
| secondary_index.cpp | secondary/range/composite index behavior |
| inverted_index.cpp | full-text inverted index surfaces |
| spatial_index.cpp | spatial indexing and geospatial query support |
| graph_index.cpp | graph index and adjacency traversal indexing |
| adaptive_index.cpp | adaptive index recommendation behavior |
| tiered_index_manager.cpp | tiered index placement and migration |
| index_compression.cpp | index compression codecs and controls |
| product_quantizer.cpp | product quantization routines |
| binary_quantizer.cpp | binary quantization routines |
| residual_quantizer.cpp | residual quantization routines |
| approximate_radius_search.cpp | approximate radius query behavior |
| distributed_vector_index.cpp | distributed vector index orchestration |
| multi_gpu_vector_index.cpp | multi-GPU index coordination |
| workload_replay.cpp | workload replay and tuning support |

## Scope

In scope:
- vector/secondary/spatial/graph indexing runtime behavior
- quantization/compression and GPU-aware acceleration paths
- index lifecycle/rebuild/tiering and recommendation support

Out of scope:
- query planner ownership outside index interfaces
- storage-engine ownership outside index module contracts
- client/UI ownership beyond index integration surfaces

## Runtime Behavior and Limits

- behavior depends on configured backend/capability and selected index strategies.
- unsupported GPU/capability paths degrade deterministically with bounded fallback behavior.
- `GPUVectorIndex` failover is explicit: when `allowCPUFallback=false`, backend gate/runtime failures fail closed instead of silently routing to CPU.
- rebuild/tiering and distributed features require explicit operational configuration.

**Production Readiness Status (Batch 3 verified 2026-08-14):**
- **Ready for production:** AnnFrontdoor (single-shard exact-first with CPU fallback), secondary indexes (range, composite), spatial indexes, graph indexes
- **Production-ready with limits:** Vector search (CPU-only, flat/HNSW), index compression (quantization codecs), adaptive index recommendation
- **Not yet production-ready:** GPU vector index CUDA backend (Wave B target Q4 2026), GPU vector index HIP backend (Wave B target Q4 2026), distributed vector index orchestration (Wave B target Q4 2026)

**Wave Alignment (see root ROADMAP.md § Program Execution Model):**
- **Wave B (Q3–Q4 2026):** ANN+vector integration gates, GPU backend validation (CUDA/HIP), hybrid retrieval Phase B buffer lifecycle RAII
- **Wave B Exit Criteria:** AnnFrontdoor+GPU parity on representative hardware, buffer concurrency ThreadSanitizer clean, benchmark gates locked
- **Tier 2 Functional Completeness:** Index performance is critical for retrieval workloads; hybrid retrieval Phase B blocks RAG Phase B

---

## Sourcecode Verification (Module: index/readme)

- Verified files:
  - src/index/index_manager.cpp
  - src/index/vector_index.cpp
  - src/index/advanced_vector_index.cpp
  - src/index/gpu_vector_index.cpp
  - src/index/gpu_vector_index_vulkan.cpp
  - src/index/secondary_index.cpp
  - src/index/inverted_index.cpp
  - src/index/spatial_index.cpp
  - src/index/graph_index.cpp
  - src/index/adaptive_index.cpp
  - src/index/tiered_index_manager.cpp
  - src/index/index_compression.cpp
  - src/index/product_quantizer.cpp
  - src/index/binary_quantizer.cpp
  - src/index/residual_quantizer.cpp
  - src/index/approximate_radius_search.cpp
  - src/index/distributed_vector_index.cpp
  - src/index/multi_gpu_vector_index.cpp
  - src/index/workload_replay.cpp
- Verified behavior surfaces:
  - index lifecycle/search/rebuild/tiering paths and acceleration boundaries
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md