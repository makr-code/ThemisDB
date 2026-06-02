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
- [INDEX-AUD-PERF-04] process_graph traversal/query hot paths now reduce avoidable container churn: multi-model edge-type checks are hash-set based (instead of per-edge linear scans), and critical-path DFS pre-reserves stack/path containers and avoids duplicate map lookups.
- [INDEX-CUDA-GPU-LEAK-01] cuda_hnsw_graph_traversal.cpp multi-pass batchSearch: `d_pass_ids` is freed (and nulled) before the guarded `if (e1 && e2)` block when `d_pass_scores` cudaMalloc fails, eliminating the GPU memory leak on partial allocation.
- [INDEX-SPATIAL-RACE-01] spatial_index.cpp: `mutable std::shared_mutex rtree_mutex_` added to SpatialIndexManager. All accesses to `rtrees_`, `mbr_cache_`, and `rtree_built_` are now protected — read paths (ensureRTree fast path, searchIntersects, searchContains) use `std::shared_lock`; write paths (ensureRTree slow path, createSpatialIndex, dropSpatialIndex, bulkLoad init/swap, insert, insertBatch, remove, removeBatch) use `std::unique_lock`. searchIntersects and searchContains snapshot candidate keys + MBRs under shared_lock before releasing for I/O to avoid lock inversion.
- [INDEX-SI-FULLTEXT-CACHE-01] secondary_index.cpp updateIndexesForDelete_ (both batch and txn variants): `cachedMetadata->fulltext_configs` is now copied into a local `fulltextConfigsCache` map immediately after metadata retrieval, and all subsequent accesses use the local copy. Eliminates shared-cache direct-access data race on the delete paths.
- [INDEX-VI-SPACE-LEAK-01] vector_index.cpp: `hnswlib::SpaceInterface<float>` ownership is now tracked via `hnswSpace_` (void* member). `init()` and `loadIndex()` store `space.get()` into `hnswSpace_` before `space.release()`; `shutdown()` (called by destructor) frees `hnswSpace_` after freeing `hnswIndex_`; `loadIndex()` also frees the previous `hnswSpace_` alongside `hnswIndex_` when reloading. Eliminates HNSW SpaceInterface leak.
- [INDEX-CUDA-RESULT-LEAK-01] cuda_hnsw_graph_traversal.cpp single-pass batchSearch: split the combined `cudaMalloc` OR-check into two sequential checks so that when `d_result_ids` allocation succeeds but `d_result_scores` fails, `d_result_ids` is freed and nulled before breaking. Eliminates GPU result buffer leak on partial allocation.
- [INDEX-MGPU-ROUTING-RACE-01] multi_gpu_vector_index.cpp topology/routing synchronization hardening: introduced `topologyMutex` and guarded initialization, shutdown, add/remove vector, search/searchBatch, statistics/rebalance, and public topology/control accessors so concurrent `vectorToGPU`/`gpuIndices` mutation cannot invalidate iterators or race reads in remove/update paths.
- [INDEX-GI-TOPOLOGY-LOAD-RACE-01] graph_index.cpp/graph_index.h topology-loaded state is now backed by `std::atomic<bool>` with acquire/release semantics, and edge add/delete storage+topology updates now run under `topology_mutex_` so rebuildTopology cannot interleave with mutation-side topology publishing.
- [INDEX-WR-LOGGING-01] workload_replay.cpp: `spdlog::debug` replaced with `THEMIS_DEBUG`; `#include <spdlog/spdlog.h>` replaced with `#include "utils/logger.h"` — direct spdlog dependency removed.
- [INDEX-WR-RESERVE-01] workload_replay.cpp `fromJSON`: `capture.events_.reserve(eventsArr.size())` added before deserialization loop — eliminates repeated vector reallocations on deserialize.
- [INDEX-HLO-DETERM-01] hnsw_layer_optimizer.cpp: local aggregation maps `entry_layer_performance` and `ef_performance` changed from `std::unordered_map` to `std::map` — tiebreaking in best-layer/best-ef selection is now deterministic (ascending key order).
- [INDEX-GAB-CATCH-01] graph_auto_buffer.cpp `estimateEntitySize`: `catch (...)` narrowed to `catch (const std::exception&)` — generic exception suppression removed; non-`std::exception` errors now propagate.
- [INDEX-PG-JSON-PARSE-01] process_graph.cpp: repeated silent JSON parse `catch (...)` blocks in form-data queries, joins, aggregates, geo task lookups, route planning, and multi-model query paths were consolidated into a shared typed parser helper (`parseJsonObjectOrEmpty`) that logs contextual `THEMIS_DEBUG` diagnostics on parse/type failures; regional-parameters parsing now returns an error with exception detail.

- [INDEX-AVI-LOAD-DANGLE-01] advanced_vector_index.cpp `load()`: `index_ = nullptr` added after `delete static_cast<faiss::Index*>(index_)` before `faiss::read_index()` — eliminates dangling pointer when `read_index` throws an exception. (Wave 4)
- [INDEX-CUDA-INDEX-BUILT-RACE-01] cuda_hnsw_graph_traversal.cpp `Impl::index_built`: changed from `bool` to `std::atomic<bool>` with `#include <atomic>` — eliminates the data race between `buildIndex()` writes and the pre-lock `batchSearch()`/`search()` reads. (Wave 4)

### Confirmed False Positives (scanner artefacts — W4 review)

- [INDEX-FP-ET-ITER-01] edge_types.cpp L339 CRITICAL `iterator_invalidation`: `auto it = category_index_.find(category)` in `getTypesByCategory` is inside a `std::shared_lock<std::shared_mutex>` scope; no mutation can occur while the iterator is live. Source-verified FP.
- [INDEX-FP-GAB-TIMEOUT-01] graph_auto_buffer.cpp L103/L159/L213 CRITICAL `no_timeout`: stale scan artefact — source already uses `try_lock_for(std::chrono::seconds(30))` at every call site; scanner snapshot predates the W1 fix (INDEX-GAB-TIMEOUT-01).
- [INDEX-FP-ARS-LOG-01] approximate_radius_search.cpp L47/L260 HIGH `audit_logging`: scanner triggered on the comment `// Validate inputs`; `grep` confirms zero `std::cout`/`printf` calls in the file.
- [INDEX-FP-ARS-PTR-01] approximate_radius_search.cpp L81/L293 HIGH `pointer_arithmetic`: scanner flagged structured bindings `auto [status, results] = vector_manager_.searchKnn(...)` and `auto [status, results] = vector_manager_.searchKnnRadius(...)`; these are return-value decompositions, not pointer/array dereferences. Source-verified FP.
- [INDEX-FP-ET-RESERVE-01] edge_types.cpp L399–400 MEDIUM `copy_overhead`: `result.push_back(name)` in `listAllTypes` is immediately preceded by `result.reserve(types_.size())`; scanner did not track the reserve call. Source-verified FP.
- [INDEX-FP-HNSWPT-META-01] hnsw_parameter_tuner.cpp L0 HIGH `uncategorized` (×5): five findings at source location L0 are scanner meta-artefacts with no source anchor; the only actionable uninitialized-array finding (`int regs[4]`) was fixed in W2 (INDEX-HNSWPT-REGS-INIT-01). All five confirmed FP.
- [INDEX-FP-AVI-DELETE-01] advanced_vector_index.cpp L62/86/116 HIGH `delete_no_nullptr` / MEDIUM `manual_cleanup`: these are `std::make_unique` construction sites and the destructor (L63–68) which already sets `index_ = nullptr`; no actionable finding.
- [INDEX-FP-GA-DATARACE-01] graph_analytics.cpp L78/L83 CRITICAL `data_race`: `topo` is a locally-declared return-by-value variable from `buildTopology()`; not shared mutable state.
- [INDEX-FP-GMEM-LOCK-01] gpu_memory_oversubscription.cpp HIGH `lock_in_loop` (L341) + `null_dereference`: lock is acquired once before destructor sweep; `pImpl_` is unconditionally initialised in constructor.
- [INDEX-FP-DVI-CONS-01] distributed_vector_index.cpp HIGH `distributed_consistency`: stale scan artefact predating INDEX-DVI-VERSION-MERGE-01 (W1).
- [INDEX-FP-SI-AUDIT-01] secondary_index.cpp L411/L2294/L2435 HIGH `audit_logging`: scanner triggered on `snprintf` to local stack buffers for key formatting; not a diagnostic output path.
- [INDEX-FP-ANN-PTR-01] ann_index.cpp HIGH `pointer_arithmetic`: `checkedRow()` helper validates bounds before returning; structured-binding returns are not pointer arithmetic.
- [INDEX-FP-VK-FP-01] gpu_vector_index_vulkan.cpp L145–147 HIGH `fp_exact_comparison`: exact equality intentionally used for cache-key fingerprint match; rounding would break cache identity.
- [INDEX-FP-MVS-AUDIT-01] multi_vector_search.cpp L98 HIGH `audit_logging`: `snprintf` to `char buf[256]` for error assembly; not an unsafe sink.
- [INDEX-FP-CUDA-LEAK-01] cuda_hnsw_graph_traversal.cpp L560/L563 HIGH `gpu_memory_leak`: result buffers freed by `freeDevice()` in destructor; RAII guarantee confirmed by source review.
- [INDEX-FP-ROPE-AI-01] rotary_embeddings_hip.cpp HIGH `llm_ai_safety`/`unsanitized_llm_input`: scanner misidentifies C++ `operation` parameter as LLM prompt; no LLM call in file.
- [INDEX-FP-ROPE-DB-01] rotary_embeddings_hip.cpp HIGH `db_connection_leak`: GPU allocator handle conflated with DB connection handle; GPU memory released via allocator RAII.
- [INDEX-FP-UNCAT-L0-01] module-wide HIGH `uncategorized` at Line 0: scanner meta-artefacts, no source anchor.
- [INDEX-FP-DETERM-MAP-01] module-wide MEDIUM `determinism/unordered_container_iter` (78×): every flagged iteration writes independent key-value pairs or accumulates into scalars; output order unaffected. Source-verified FP.
- [INDEX-FP-STRCAT-01] secondary_index.cpp MEDIUM `string_concat_loop` (15×): every flagged `key +=` loop is preceded by `key.reserve(total)`; scanner does not track reserve cross-statement.
- [INDEX-FP-PATH-01] secondary_index.cpp/gpu_memory_oversubscription.cpp MEDIUM `hardcoded_path`: scanner triggered on log-message string literals inside `THEMIS_ERROR` macros; not filesystem paths.
- [INDEX-FP-MANUAL-01] module-wide MEDIUM `manual_cleanup` (17×): `void*` FAISS index cannot use `unique_ptr<void>` without custom deleter; `ofstream.close()` is safe early-close idiom; `db_.del()` deletes a DB key; GPU allocator `.free()` is correct RAII-equivalent.
- [INDEX-FP-UNCAUGHT-01] module-wide MEDIUM `uncaught_exception` (118×): constructor-level throws are legal C++; callers handle via try/catch at `make_unique`/`new` sites.

## Compliance Snapshot

| Requirement | Status |
|---|---|
| Source-verifiable behavior claims | pass |
| Structured forward planning in roadmap/future | pass |
| Historical completion tracked in changelog | pass |
| Core module docs synchronized | pass |