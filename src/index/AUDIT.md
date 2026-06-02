# Audit Report - Index Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

## Summary

| Metric | Result |
|---|---|
| Build registration | pass |
| Source set size | 45+ implementation files in src/index |
| Focused test presence | pass |
| Open hardening findings | yes |
| Critical blockers | none identified |

## Verified Files

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

## Findings

### Open

1. [INDEX-AUD-01] backend parity and fallback edge hardening remains active.
- Severity: medium
- Evidence: roadmap/future retain active work for mixed-capability execution scenarios.
- Action: close deterministic regressions across backend degradation and fallback transitions.

2. [INDEX-AUD-02] lifecycle diagnostics need further tightening.
- Severity: medium
- Evidence: active follow-up work for rebuild/tiering/distributed incident observability.
- Action: unify taxonomy and diagnostics for lifecycle failure classes.

3. [INDEX-AUD-03] benchmark depth should broaden for advanced index workflows.
- Severity: low
- Evidence: core mapping is valid, but specialized distributed and advanced retrieval cases need deeper coverage.
- Action: add benchmark depth for advanced index and distribution-heavy workflows.

4. [INDEX-AUD-GI-01] _sensitive boolean fallback in addEdge — legacy encryption field selector.
- Severity: medium
- Evidence: graph_index.cpp addEdge path retains backwards-compat branch for pre-v2.1 documents using `_sensitive=true` instead of `encrypt_fields`.
- Action: Remove after data migration confirms no _sensitive=true records remain. Tracked via LEGACY_COMPAT comment in source.
- Status: annotated; removal pending migration

5. [INDEX-AUD-GI-02] _sensitive boolean fallback in updateEdge — duplicate of GI-01.
- Severity: medium
- Evidence: updateEdge path has same backwards-compat branch as addEdge.
- Action: Remove together with GI-01 after migration.
- Status: annotated; removal pending migration

6. [INDEX-AUD-GI-03] Legacy key format support (pre-v2.0 graph:out/in without graphId segment).
- Severity: low
- Evidence: parseOutKey_, parseInKey_, and scanEdges_ retain branches for the pre-v2.0 key format that omits the graphId segment.
- Action: Remove after confirming no pre-v2.0 graph keys remain in production storage.
- Status: annotated; removal pending storage migration confirmation

### Closed

- core index runtime surfaces are present and source-verified.
- documentation set is synchronized to source-verifiable claims.
- changelog/roadmap role separation is aligned to module governance pattern.
- [INDEX-AUD-LOG-01] hardcoded std::cout/std::cerr in gpu_vector_index.cpp replaced with THEMIS structured logging macros (THEMIS_INFO/WARN/ERROR). All 22 instances fixed; `#include <iostream>` removed.
- [INDEX-AUD-MEM-01] hnswlib index memory leak in VectorIndexManager: hnswIndex_ is now freed in shutdown() and in loadIndex() before replacing existing index pointer.
- [INDEX-AUD-RACE-01] secondary_index write/delete paths now snapshot cached metadata to local containers before processing, removing repeated shared-structure dereferences in hot loops.
- [INDEX-AUD-MEM-02] vector_index HNSW space allocation is RAII-managed via `std::unique_ptr` in init/load paths, preventing leaks when HNSW constructors throw.
- [INDEX-AUD-PERF-01] O(n²) phrase normalization in secondary_index.cpp::computeBM25Scores_ eliminated: normalized phrases are now precomputed once before the outer loop.
- [INDEX-AUD-PERF-02] Multiple missing reserve() calls fixed: tokenResults, values (composite scan), validateProcess errors/warnings, evaluateGateway_ targets, deserializeVisitedNodes nodes.
- [INDEX-AUD-DTOR-01] StackEntry missing destructor in process_graph.cpp: added ~StackEntry() = default.
- [INDEX-AUD-RACE-02] vector_index mutable cache/ID/HNSW state now uses a shared recursive state mutex in mutating/query/statistics paths, preventing unsynchronized concurrent access.
- [INDEX-AUD-PERF-03] secondary_index BM25 candidate intersection now processes smallest token sets first with empty-intersection early-exit to reduce high-volume container scan overhead.
- [INDEX-CUDA-GPU-LEAK-01] cuda_hnsw_graph_traversal.cpp multi-pass batchSearch: `d_pass_ids` is freed (and nulled) before the guarded `if (e1 && e2)` block when `d_pass_scores` cudaMalloc fails, eliminating the GPU memory leak on partial allocation.
- [INDEX-SPATIAL-RACE-01] spatial_index.cpp: `mutable std::shared_mutex rtree_mutex_` added to SpatialIndexManager. All accesses to `rtrees_`, `mbr_cache_`, and `rtree_built_` are now protected — read paths (ensureRTree fast path, searchIntersects, searchContains) use `std::shared_lock`; write paths (ensureRTree slow path, createSpatialIndex, dropSpatialIndex, bulkLoad init/swap, insert, insertBatch, remove, removeBatch) use `std::unique_lock`. searchIntersects and searchContains snapshot candidate keys + MBRs under shared_lock before releasing for I/O to avoid lock inversion.
- [INDEX-SI-FULLTEXT-CACHE-01] secondary_index.cpp updateIndexesForDelete_ (both batch and txn variants): `cachedMetadata->fulltext_configs` is now copied into a local `fulltextConfigsCache` map immediately after metadata retrieval, and all subsequent accesses use the local copy. Eliminates shared-cache direct-access data race on the delete paths.
- [INDEX-VI-SPACE-LEAK-01] vector_index.cpp: `hnswlib::SpaceInterface<float>` ownership is now tracked via `hnswSpace_` (void* member). `init()` and `loadIndex()` store `space.get()` into `hnswSpace_` before `space.release()`; `shutdown()` (called by destructor) frees `hnswSpace_` after freeing `hnswIndex_`; `loadIndex()` also frees the previous `hnswSpace_` alongside `hnswIndex_` when reloading. Eliminates HNSW SpaceInterface leak.
- [INDEX-CUDA-RESULT-LEAK-01] cuda_hnsw_graph_traversal.cpp single-pass batchSearch: split the combined `cudaMalloc` OR-check into two sequential checks so that when `d_result_ids` allocation succeeds but `d_result_scores` fails, `d_result_ids` is freed and nulled before breaking. Eliminates GPU result buffer leak on partial allocation.
- [INDEX-MGPU-ROUTING-RACE-01] multi_gpu_vector_index.cpp topology/routing synchronization hardening: introduced `topologyMutex` and guarded initialization, shutdown, add/remove vector, search/searchBatch, statistics/rebalance, and public topology/control accessors so concurrent `vectorToGPU`/`gpuIndices` mutation cannot invalidate iterators or race reads in remove/update paths.

## Compliance Snapshot

| Requirement | Status |
|---|---|
| Source-verifiable behavior claims | pass |
| Structured forward planning in roadmap/future | pass |
| Historical completion tracked in changelog | pass |
| Core module docs synchronized | pass |