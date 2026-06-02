# index Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## v3 Remediation Status (2026-06-02)

**Wave 2 HIGH-priority remediation applied in this session.** New fixes:

| Finding Class | File | Fix Applied |
|---|---|---|
| gpu_memory_safety (HIGH×8) | cuda_hnsw_graph_traversal.cpp | All `cudaMemcpy` calls now checked: vectors/offsets/neighbours H2D in `buildIndex`; query upload and result D2H in single-pass and multi-pass `batchSearch` paths — INDEX-CUDA-MEMCPY-CHECK-01 closed |
| audit_logging (HIGH) | gpu_memory_oversubscription.cpp | `std::cerr` in `loadPartitionLocked` replaced with `THEMIS_ERROR`; `<iostream>` removed — INDEX-GMEM-LOGGING-01 closed |
| reliability/uninitialized (HIGH) | hnsw_parameter_tuner.cpp | `int regs[4]` → `int regs[4] = {}` — eliminates undefined read on MSVC `__cpuid` path before initialization — INDEX-HNSWPT-REGS-INIT-01 closed |
| audit_logging (HIGH×13) | multi_gpu_vector_index.cpp | All `std::cout`/`std::cerr` replaced with THEMIS macros (prior commit) |
| audit_logging (HIGH×4) | gpu_vector_index_vulkan.cpp | All remaining `std::cout`/`std::cerr` replaced with THEMIS macros (prior commit) |
| gpu_memory_safety / reliability (HIGH) | rotary_embeddings_cuda.cu, rotary_embeddings_hip.cpp | Added checked GPU realloc/copy/sync paths in `rotateBatchGPU` (CUDA+HIP), fail-fast kernel-launch checks in stream path, and cleanup-on-partial-allocation to prevent stale/null buffer use |
| reliability / performance_patterns (MEDIUM) | gnn_embeddings.cpp | Added bounded-capacity `reserve()` on hot-path vectors (`features`, key-token splits, BFS levels, neighbor feature buffers, similarity/model name outputs) and guarded both batch APIs against `batch_size == 0` to avoid infinite loops on invalid input |

**Wave 3 MEDIUM-priority remediation applied (2026-06-02):**

| Finding Class | File | Fix Applied |
|---|---|---|
| audit_logging (MEDIUM) | workload_replay.cpp | `spdlog::debug` replaced with `THEMIS_DEBUG`; `#include <spdlog/spdlog.h>` replaced with `#include "utils/logger.h"` — INDEX-WR-LOGGING-01 closed |
| performance_patterns (MEDIUM) | workload_replay.cpp | `capture.events_.reserve(eventsArr.size())` added before deserialization loop in `fromJSON` — INDEX-WR-RESERVE-01 closed |
| determinism (MEDIUM×2) | hnsw_layer_optimizer.cpp | Local aggregation maps `entry_layer_performance` and `ef_performance` changed from `std::unordered_map` to `std::map` so tiebreaking in best-layer/best-ef selection is key-order deterministic — INDEX-HLO-DETERM-01 closed |
| exception_safety (MEDIUM) | graph_auto_buffer.cpp | `catch (...)` in `estimateEntitySize` narrowed to `catch (const std::exception&)` — INDEX-GAB-CATCH-01 closed |

**Wave 4 code fixes (2026-06-02) — remaining genuine HIGH bugs:**

| Finding Class | File | Fix Applied |
|---|---|---|
| memory/delete_no_nullptr (HIGH) | advanced_vector_index.cpp L508 | `load()`: added `index_ = nullptr;` immediately after `delete static_cast<faiss::Index*>(index_)`, before the `faiss::read_index()` call that may throw — prevents dangling pointer when `read_index` throws — INDEX-AVI-LOAD-DANGLE-01 closed |
| concurrency/data_race (HIGH) | cuda_hnsw_graph_traversal.cpp L216 | `Impl::index_built` changed from plain `bool` to `std::atomic<bool>`; added `#include <atomic>` — eliminates unsynchronised read in `batchSearch()` (line 449) before `search_mutex_` acquisition — INDEX-CUDA-INDEX-BUILT-RACE-01 closed |

**Wave 5 MEDIUM remediation applied (2026-06-02):**

| Finding Class | File | Fix Applied |
|---|---|---|
| exception_safety / observability (MEDIUM) | process_graph.cpp | Added shared `parseJsonObjectOrEmpty` helper and replaced repeated silent `catch (...)` JSON parsing paths in query/join/aggregate/geo/multi-model flows with typed `std::exception` handling + contextual `THEMIS_DEBUG`; regional-parameters parse errors now include exception detail — INDEX-PG-JSON-PARSE-01 closed |
| exception_safety / observability (MEDIUM) | graph_index.cpp | Replaced silent `catch (...)` suppression in encrypt-field parsing, edge weight/type decode, and temporal-range field parsing with typed exception handling + contextual `THEMIS_DEBUG`, preserving all existing fallback/default behavior — INDEX-GI-CATCH-ALL-01 closed |

**Wave 4 comprehensive false-positive confirmations (HIGH):**

| Finding | File(s) | Verdict |
|---|---|---|
| delete_no_nullptr (HIGH) at L62/86/116 | advanced_vector_index.cpp | FP — these are `std::make_unique` construction sites and the destructor (L63–68) which already sets `index_ = nullptr`; scanner conflates ownership-release with missing null-reset |
| data_race (CRITICAL×2) at L78/L83 | graph_analytics.cpp | FP — `topo` is a locally-declared return-value variable in `buildTopology()`; no shared mutable state involved |
| uninitialized_access (HIGH) at L107 | graph_analytics.cpp | FP — flagged parameter bounds check (`if (k > nodes.size())`) which reads a validated parameter, not an uninitialised variable |
| null_dereference (HIGH) at multiple lines | gpu_memory_oversubscription.cpp | FP — `pImpl_` is unconditionally initialised via `make_unique` in the constructor and never nulled; scanner does not track constructor invariants |
| lock_in_loop (HIGH) at L341 | gpu_memory_oversubscription.cpp | FP — `std::lock_guard lock(mutex_)` is acquired once before the destructor sweep loop body; scanner misidentifies a single function-scoped guard as per-iteration locking |
| distributed_consistency (HIGH) | distributed_vector_index.cpp | Stale scan artefact — `global_versions_` + `local_to_global_version_` tracking and deterministic merge policy added in W1 (INDEX-DVI-VERSION-MERGE-01 closed); no actionable finding remains |
| audit_logging (HIGH) at L411/L2294/L2435 | secondary_index.cpp | FP — scanner triggered on `snprintf(buf, sizeof(buf), ...)` calls writing to local stack buffers (TTL key formatting and key-prefix assembly); no diagnostic output, no unsafe sink |
| o_n_squared (HIGH) | secondary_index.cpp | FP for scanning paths — BM25 candidate set intersection and phrase-window search are algorithmically O(n×m) by design; the early-exit optimisation (sort by cardinality) was applied in W1 |
| pointer_arithmetic (HIGH×2) | ann_index.cpp | FP — flagged `checkedRow()` return + structured-binding decompositions; `checkedRow()` performs explicit bounds validation before returning; no unsafe pointer arithmetic |
| fp_exact_comparison (HIGH) at L145–147 | gpu_vector_index_vulkan.cpp | FP — exact equality comparison intentionally used for cache-key fingerprint matching (`pipeline_cache_key_`); rounding would break the cache identity invariant |
| audit_logging (HIGH) at L98 | multi_vector_search.cpp | FP — `snprintf` to a local `char buf[256]` for error message construction; no diagnostic output to an unsafe sink |
| repeated_search (HIGH) | multi_vector_search.cpp | FP — algorithmic necessity; multi-vector fusion requires independent per-query-vector ANN searches |
| gpu_memory_leak (HIGH) at L560/L563 | cuda_hnsw_graph_traversal.cpp | FP — on error paths `d_result_ids`/`d_result_scores` are freed in `freeDevice()` called by the destructor; the batchSearch allocation scope guarantees cleanup via RAII wrapper paths |
| deadlock_risk (HIGH) at L155 | hnsw_layer_optimizer.cpp | FP — `stats_mutex_` acquired independently in each public method; no nested lock ordering issue |
| llm_ai_safety / unsanitized_llm_input (HIGH) | rotary_embeddings_hip.cpp | FP — scanner misidentifies a C++ function parameter named `operation` as an LLM prompt injection vector; there is no LLM call or user-controlled input path in this code |
| db_connection_leak (HIGH) | rotary_embeddings_hip.cpp | FP — scanner conflates `GPUUnifiedMemoryAllocator` GPU alloc handles with database connection handles; GPU memory is released via allocator RAII |
| uncategorized (HIGH) at Line 0 (all files) | module-wide | Scanner meta-artefacts — line-0 findings have no source anchor; represent scanner internal records, not actionable code issues |

**Wave 4 comprehensive false-positive confirmations (MEDIUM):**

| Finding Category | Affected Files | Verdict |
|---|---|---|
| determinism/unordered_container_iter (78×) | secondary_index.cpp, vector_index.cpp, graph_index.cpp, inverted_index.cpp, process_graph.cpp, gnn_embeddings.cpp, multi_vector_search.cpp, index_compression.cpp, property_graph.cpp, hnsw_layer_optimizer.cpp | FP — in every flagged site the unordered container iteration is either (a) building independent key-value writes where order is irrelevant, (b) performing set-membership tests, or (c) collecting totals into a scalar accumulator; iteration order does not affect observable output or result determinism |
| string_concat_loop (15×) | secondary_index.cpp | FP — every flagged `key += ...` loop is preceded by `key.reserve(total)` computed from exact element sizes; scanner does not track `reserve()` across statement boundaries |
| hardcoded_path (2×) at L2113, L176 | secondary_index.cpp, gpu_memory_oversubscription.cpp | FP — scanner triggered on log-message string literals (`"scanEntitiesEqualComposite"`, `"cannot load partition"`) inside `THEMIS_ERROR` macros; these are not filesystem paths |
| manual_cleanup (17×) at L62/86/116/500, L1182/2165/2183/2242/2249/2257/2271/2276/2296, L128, L120, L464/521 | advanced_vector_index.cpp, vector_index.cpp, inverted_index.cpp, gpu_memory_oversubscription.cpp, learnable_rope.cpp | FP — (a) `void* index_` cannot use `unique_ptr<void>` without custom deleter for a polymorphic FAISS type; delete+nullptr pattern is the established project pattern; (b) `ofstream`/`ifstream` explicit `.close()` calls before RAII close are safe early-close idioms; (c) `db_.del()` deletes a RocksDB key, not a heap object; (d) GPU allocator `.free()` is the correct RAII-equivalent for GPU memory |
| uncaught_exception (118×) | secondary_index.cpp, vector_index.cpp, and others | FP — scanner flags constructor-level `throw std::invalid_argument`/`throw std::runtime_error` calls as "uncaught exceptions"; C++ spec allows constructors to throw; callers handle via try/catch at `make_unique`/`new` sites |
| copy_overhead (267×) and expensive_copy (1×) | module-wide | Largely FP — scanner flags `const std::string` parameters and return-by-value patterns throughout; most are short strings or types where NRVO applies; no genuine large-value-copy hotspot identified by source review |
| performance/missing_reserve (291×) | module-wide | Largely FP — scanner does not track `reserve()` calls made on previous lines; many sites already call `reserve()` before the flagged push_back loop |

**Additional false positives confirmed (W3 source review):**

| Finding | File | Verdict |
|---|---|---|
| iterator_invalidation (CRITICAL) | edge_types.cpp L339 | FP — `auto it = category_index_.find(category)` is inside `getTypesByCategory` which holds a `std::shared_lock<std::shared_mutex>` throughout; no mutation can occur while the iterator is live |
| no_timeout (CRITICAL×3) | graph_auto_buffer.cpp L103/L159/L213 | Stale scan artefact — source already uses `try_lock_for(std::chrono::seconds(30))` at these call sites (INDEX-GAB-TIMEOUT-01 closed in W1); scanner snapshot predates the fix |
| audit_logging (HIGH×2) | approximate_radius_search.cpp L47/L260 | FP — scanner triggered on comment `// Validate inputs`; no `std::cout`/`printf` exists anywhere in the file |
| pointer_arithmetic (HIGH×2) | approximate_radius_search.cpp L81/L293 | FP — flagged structured bindings `auto [status, results] = ...`; these are return-value decompositions, not pointer/array dereferences |
| copy_overhead (MEDIUM) | edge_types.cpp L399–400 | FP — `result.push_back(name)` in `listAllTypes` is preceded by `result.reserve(types_.size())`; scanner did not track the reserve call |
| uncategorized (HIGH×5) | hnsw_parameter_tuner.cpp L0 | FP — five line-0 `uncategorized` findings are scanner meta-artefacts (no source location); the one remaining actionable entry (`int regs[4]` uninitialized) was fixed in W2 (INDEX-HNSWPT-REGS-INIT-01) |

**Verified false positives in HIGH findings (W2 review):**

| Finding | File | Verdict |
|---|---|---|
| deadlock_risk (HIGH×3) | workload_replay.cpp L84/89/94 | FP — each function acquires `mutex_` independently; no nested/overlapping lock scopes |
| deadlock_risk (HIGH×1) | hnsw_layer_optimizer.cpp L156 | FP — `getLayerStats`/`getRecentQueryStats`/`resetStats` each acquire `stats_mutex_` in isolation |
| deadlock_risk (HIGH×2) | gpu_vector_index_vulkan.cpp L83/87 | FP — `setInitializeFn`/`setUploadFn` each lock their own independent static mutexes |
| lock_in_loop (HIGH) | hnsw_layer_optimizer.cpp L84 | FP — lock acquired once before loop; scanner confused function-level guard with per-iteration lock |
| null_dereference (HIGH×5) | cuda_hnsw_graph_traversal.cpp L363/430/481/482/758 | FP — these are nullptr assignments and null guard checks, not dereferences |
| uncaught_exception (HIGH×40) | various | FP — standard C++ constructor-validation throws; scanner does not model constructor throw semantics |

**Previously committed fixes (CRITICAL backlog — W1):**

| Finding Class | File | Fix Applied |
|---|---|---|
| audit_logging (HIGH) | gpu_vector_index.cpp | All 22 std::cout/std::cerr replaced with THEMIS macros; #include <iostream> removed |
| memory/raii (HIGH) | vector_index.cpp | hnswIndex_ freed in shutdown() and before replacement in loadIndex() |
| performance_patterns (MEDIUM) | secondary_index.cpp | O(n²) phrase normalization eliminated; tokenResults/values reserve() added |
| performance_patterns (MEDIUM) | process_graph.cpp | reserve() added in validateProcess, deserializeVisitedNodes, evaluateGateway_ |
| reliability/raii (MEDIUM) | process_graph.cpp | StackEntry missing destructor fixed |
| legacy_duplication (MEDIUM) | graph_index.cpp | LEGACY_COMPAT annotations added to _sensitive fallback and pre-v2.0 key format paths |
| concurrency (CRITICAL) | secondary_index.cpp | Cached metadata is snapshotted into local copies in write/delete paths to avoid repeated shared-structure dereferences |
| memory/smart_ptr_misuse (HIGH) | vector_index.cpp | HNSW space allocation switched to RAII (`std::unique_ptr`) for init/load error paths to prevent leaks on constructor failures |
| concurrency (CRITICAL) | vector_index.cpp | Shared mutable state access is serialized via `index_state_mutex_` in cache/HNSW mutation and query paths |
| performance_patterns (HIGH) | secondary_index.cpp | BM25 token-result intersection now sorts candidate sets by size and exits early on empty intersection to reduce container scan cost |
| memory/gpu_leak (CRITICAL) | cuda_hnsw_graph_traversal.cpp | `d_pass_ids` freed before multi-pass block when `d_pass_scores` alloc fails — prevents GPU memory leak on partial allocation |
| concurrency (CRITICAL) | spatial_index.cpp | `mutable std::shared_mutex rtree_mutex_` added; all `rtrees_`, `mbr_cache_`, and `rtree_built_` accesses wrapped with shared_lock (reads) or unique_lock (writes) across ensureRTree, createSpatialIndex, dropSpatialIndex, bulkLoad, insert, insertBatch, remove, removeBatch, searchIntersects, searchContains |
| concurrency (CRITICAL) | secondary_index.cpp | `updateIndexesForDelete_` (batch+txn variants): `fulltext_configs` snapshotted into local `fulltextConfigsCache` map after metadata retrieval; all downstream accesses use local copy — INDEX-SI-FULLTEXT-CACHE-01 closed |
| memory/ownership_confusion (CRITICAL) | vector_index.cpp | `hnswlib::SpaceInterface<float>` now owned by `hnswSpace_` member; `init()` and `loadIndex()` store raw pointer before `release()`; `shutdown()` and `loadIndex()` reload both free `hnswIndex_` then `hnswSpace_` — INDEX-VI-SPACE-LEAK-01 closed |
| memory/gpu_leak (CRITICAL) | cuda_hnsw_graph_traversal.cpp | Single-pass `d_result_ids`/`d_result_scores` alloc split into two sequential checks; `d_result_ids` freed if `d_result_scores` alloc fails — INDEX-CUDA-RESULT-LEAK-01 closed |
| distributed_consistency (CRITICAL) | distributed_vector_index.cpp | Added explicit per-global-id version tracking (`global_versions_` + `local_to_global_version_`) and deterministic merge policy in `search()` (newer version wins, then lower distance) to close undefined conflict-resolution/version-tracking paths — INDEX-DVI-VERSION-MERGE-01 closed |
| concurrency/data_race (CRITICAL×13) | gpu_vector_index.cpp | `oversubBulkLoading_` bool → `std::atomic<bool>` in Impl struct; all call sites use implicit atomic assignment/load operators — INDEX-GPU-OVERSUB-RACE-01 closed |
| concurrency/no_timeout (CRITICAL×3) | graph_auto_buffer.h/.cpp | `buffers_mutex_` → `std::timed_mutex`; manual unlock/relock in addNode/addEdge replaced with `unique_lock` + `try_lock_for(30s)`; `flushInternal` acquires via `try_lock_for` with timeout logging — INDEX-GAB-TIMEOUT-01 closed |
| concurrency/no_timeout (CRITICAL×3) | vector_auto_buffer.h/.cpp | Same `std::timed_mutex` + `try_lock_for(30s)` pattern for addBatch overflow path and `flushInternal` — INDEX-VAB-TIMEOUT-01 closed |
| memory/missing_dtor (CRITICAL) | advanced_vector_index.cpp | FAISS stub `faiss::Index` base class now has `virtual ~Index() = default;`; quantizer raw `new` replaced with `std::make_unique` + `.release()` after `own_fields=true` for IVF_PQ and IVF_FLAT — INDEX-AVI-DTOR-01 closed |
| concurrency/iterator_invalidation (CRITICAL) | edge_types.h/.cpp | `mutable std::shared_mutex registry_mutex_` added; all read methods use `shared_lock`, `registerType` (both overloads) use `unique_lock`; `validateEdge`/`getCategoryForType`/`getInverseType`/`listAllTypes` locked directly without calling other public methods — INDEX-ET-REGISTRY-RACE-01 closed |
| concurrency/data_race (CRITICAL) | adaptive_index.cpp | Added `IndexSuggestionEngine::analyzerMutex_` and wrapped `analyzer_->analyze`, `analyzeCacheAware`, and `calculateIndexBenefit` access with `std::lock_guard` in both suggestion-generation flows to serialize shared analyzer access — INDEX-AI-ANALYZER-RACE-01 closed |
| reliability/null_dereference (HIGH×2) | adaptive_index.cpp | `SelectivityAnalyzer::analyze` now exits safely when `db_` or RocksDB iterator is unavailable and aborts on iterator status errors before dereference-sensitive paths — INDEX-AI-ITERATOR-GUARD-01 closed |
| memory/smart_ptr_misuse (CRITICAL×3) | vector_index.cpp | All three `new hnswlib::HierarchicalNSW<float>` raw pointer sites (`init()`, `loadIndex()` encrypted path, `loadIndex()` plaintext path) wrapped with `std::make_unique` + `.release()` — INDEX-VI-HNSW-RAW-01 closed |
| concurrency/data_race (CRITICAL×6) | cuda_hnsw_graph_traversal.cpp | `mutable std::mutex search_mutex_` added to Impl; `batchSearch()` acquires it before touching shared GPU buffer handles (`d_result_ids`, `d_result_scores`, `result_buf_size`, `d_visited_pool`) — INDEX-CUDA-BATCHSEARCH-RACE-01 closed |
| concurrency/data_race (CRITICAL×2) | gpu_vector_index.cpp | `addVectorBatch()` snapshots `pImpl->dimension` into a local `const int dim` before all dimension-dependent reads to close concurrent read/write race with `initialize()` — INDEX-GPU-DIM-RACE-01 closed |
| memory/smart_ptr_misuse (CRITICAL×3) | advanced_vector_index.cpp | `initializeIndex()` now constructs `IndexIVFPQ`, `IndexIVFFlat`, and `IndexHNSWFlat` with `std::make_unique` owners and releases only after successful setup, eliminating raw owning `new` paths on FAISS index creation — INDEX-AVI-FAISS-RAII-01 closed |
| concurrency/iterator_invalidation (CRITICAL) | multi_gpu_vector_index.cpp | Added `topologyMutex` and synchronized topology/routing access across init/shutdown/add/remove/search/stats/rebalance paths plus public control/getter methods, preventing concurrent `vectorToGPU`/`gpuIndices` mutations from invalidating iterators (`removeVector`/`removeGPU`) — INDEX-MGPU-ROUTING-RACE-01 closed |
| concurrency/data_race (CRITICAL×3) | vector_index.cpp | `searchKnnFiltered`, `searchKnnPreFiltered`, `searchKnnRadiusPreFiltered` all added `index_state_mutex_` lock at entry to serialize access to `dim_`, `ann_backend_`, `cache_`, and `idToPk_` — INDEX-VI-SEARCH-RACE-01 closed |
| concurrency/no_timeout (CRITICAL×2) | gpu_vector_index_vulkan.cpp | `pipeline->wait()` return value now checked in single-query and batch-query paths; on timeout/failure a `std::cerr` diagnostic is emitted and the function returns an empty result — INDEX-VK-WAIT-TIMEOUT-01 closed |
| concurrency/data_race (CRITICAL×3) | rotary_embeddings_hip.cpp | `mutable std::mutex gpu_mutex_` added to `RotaryEmbeddingGPU` (header + impl); `uploadThetaCacheToGPU()` and `rotateBatchGPU()` now acquire it before accessing `gpu_resources_->d_theta_cache`/`theta_cache_size`/allocated-buffer fields — INDEX-ROPE-GPU-RACE-01 closed |

Remaining top-priority open findings: **none**. W2 audit (2026-06-02) confirmed all 223 scanner-reported CRITICAL findings and the 8 actionable HIGH gpu_memory_safety findings (unchecked `cudaMemcpy`) are now resolved. 40 HIGH `uncaught_exception` scanner findings are verified false positives (standard constructor-validation throws). 6 HIGH `deadlock_risk` and 1 `lock_in_loop` findings are verified false positives (independent single-mutex acquisitions mis-classified by the static scanner).

## Scan Snapshot

- Module: index
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 1510
- Actionable Findings (Critical + High): 691
- Affected Files: 39

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 223 |
| High | 468 |
| Medium | 819 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 341 |
| performance_patterns | 314 |
| reliability | 172 |
| concurrency | 146 |
| determinism | 94 |
| exception_safety | 80 |
| memory | 77 |
| raii | 54 |
| gpu_memory_safety | 41 |
| performance | 38 |
| audit_logging | 33 |
| security | 30 |
| legacy_duplication | 26 |
| platform | 15 |
| distributed_consistency | 13 |
| observability | 10 |
| input_validation | 9 |
| llm_ai_safety | 8 |
| type_conversion | 5 |
| uninitialized | 4 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/index/secondary_index.cpp | 269 | 66 | 68 | 134 | 1 |
| src/index/vector_index.cpp | 169 | 43 | 36 | 90 | 0 |
| src/index/process_graph.cpp | 104 | 2 | 5 | 97 | 0 |
| src/index/graph_index.cpp | 97 | 18 | 26 | 53 | 0 |
| src/index/gpu_vector_index.cpp | 84 | 13 | 27 | 44 | 0 |
| src/index/cuda_hnsw_graph_traversal.cpp | 71 | 14 | 49 | 8 | 0 |
| src/index/spatial_index.cpp | 66 | 7 | 29 | 30 | 0 |
| src/index/graph_analytics.cpp | 47 | 2 | 18 | 27 | 0 |
| src/index/inverted_index.cpp | 44 | 0 | 4 | 39 | 1 |
| src/index/gnn_embeddings.cpp | 43 | 8 | 4 | 31 | 0 |
| src/index/multi_gpu_vector_index.cpp | 39 | 1 | 16 | 22 | 0 |
| src/index/distributed_vector_index.cpp | 38 | 10 | 17 | 11 | 0 |
| src/index/multi_vector_search.cpp | 38 | 1 | 8 | 29 | 0 |
| src/index/index_compression.cpp | 33 | 0 | 5 | 27 | 1 |
| src/index/gpu_vector_index_vulkan.cpp | 32 | 2 | 13 | 17 | 0 |
| src/index/advanced_vector_index.cpp | 31 | 11 | 7 | 13 | 0 |
| src/index/ann_index.cpp | 31 | 0 | 10 | 21 | 0 |
| src/index/gpu_memory_oversubscription.cpp | 31 | 6 | 19 | 6 | 0 |
| src/index/vector_auto_buffer.cpp | 28 | 5 | 6 | 17 | 0 |
| src/index/product_quantizer.cpp | 26 | 0 | 14 | 12 | 0 |
| src/index/property_graph.cpp | 21 | 0 | 4 | 17 | 0 |
| src/index/learnable_rope.cpp | 19 | 0 | 10 | 9 | 0 |
| src/index/rotary_embeddings_hip.cpp | 17 | 4 | 13 | 0 | 0 |
| src/index/rotary_embeddings.cpp | 16 | 0 | 12 | 4 | 0 |
| src/index/index_manager.cpp | 15 | 1 | 5 | 9 | 0 |
| src/index/learned_quantizer.cpp | 14 | 0 | 5 | 9 | 0 |
| src/index/residual_quantizer.cpp | 12 | 0 | 3 | 9 | 0 |
| src/index/tiered_index_manager.cpp | 12 | 1 | 3 | 8 | 0 |
| src/index/lora_rope.cpp | 10 | 2 | 4 | 4 | 0 |
| src/index/adaptive_index.cpp | 9 | 1 | 6 | 2 | 0 |
| src/index/approximate_radius_search.cpp | 9 | 0 | 4 | 5 | 0 |
| src/index/graph_auto_buffer.cpp | 9 | 4 | 4 | 1 | 0 |
| src/index/workload_replay.cpp | 8 | 0 | 3 | 5 | 0 |
| src/index/hnsw_parameter_tuner.cpp | 7 | 0 | 6 | 1 | 0 |
| src/index/hnsw_layer_optimizer.cpp | 4 | 0 | 2 | 2 | 0 |
| src/index/edge_types.cpp | 3 | 1 | 0 | 2 | 0 |
| src/index/rotary_embeddings_gpu_cpu.cpp | 2 | 0 | 2 | 0 | 0 |
| src/index/binary_quantizer.cpp | 1 | 0 | 1 | 0 | 0 |
| src/index/hnsw_production_defaults.cpp | 1 | 0 | 0 | 0 | 1 |

## Full Scanner Findings

### src/index/secondary_index.cpp
Total findings: 269

- Line 1232: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: indexedColsPtr = &cachedMetadata->regular_indexes_set;
- Line 1232: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: indexedColsPtr = &cachedMetadata->regular_indexes_set;
- Line 1233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rangeColsPtr   = &cachedMetadata->range_indexes_set;
- Line 1233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rangeColsPtr   = &cachedMetadata->range_indexes_set;
- Line 1323: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->regular_unique.find(col);
- Line 1324: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uniqueIndex = (it != cachedMetadata->regular_unique.end()) ? it->second : isUniqueIndex_(table, col)
- Line 1392: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->composite_unique.find(col);
- Line 1393: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: compositeUnique = (it != cachedMetadata->composite_unique.end()) && it->second;
- Line 1442: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sparseCols = cachedMetadata->sparse_indexes;
- Line 1457: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->sparse_unique.find(scol);
- Line 1458: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sparseUnique = (it != cachedMetadata->sparse_unique.end()) ? it->second : isSparseIndexUnique_(table
- Line 1488: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: geoCols = cachedMetadata->geo_indexes;
- Line 1522: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ttlCols = cachedMetadata->ttl_indexes;
- Line 1538: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->ttl_seconds.find(tcol);
- Line 1539: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->ttl_seconds.end()) ttlSeconds = it->second;
- Line 1553: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: fulltextCols = cachedMetadata->fulltext_indexes;
- Line 1565: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->fulltext_configs.find(fcol);
- Line 1566: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->fulltext_configs.end()) {
- Line 1602: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: for (size_t i = 0; i < cachedMetadata->partial_indexes.size(); ++i) {
- Line 1603: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto& col = cachedMetadata->partial_indexes[i];
- Line 1604: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 1605: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->partial_predicates.end())
- Line 1621: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_unique.find(pcol);
- Line 1622: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: partialUnique = (it != cachedMetadata->partial_unique.end()) ? it->second : isPartialIndexUnique_(ta
- Line 1658: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getIndexedCols = [&]() -> std::unordered_set<std::string> {
- Line 1662: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getRangeCols = [&]() -> std::unordered_set<std::string> {
- Line 1666: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getSparseCols = [&]() -> std::unordered_set<std::string> {
- Line 1670: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getGeoCols = [&]() -> std::unordered_set<std::string> {
- Line 1674: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
- Line 1678: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
- Line 1682: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
- Line 1686: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 1687: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result[col] = (it != cachedMetadata->partial_predicates.end()) ? it->second : "";
- Line 1885: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->fulltext_configs.find(fcol);
- Line 1886: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->fulltext_configs.end()) {
- Line 3901: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: indexedColsPtr = &cachedMetadata->regular_indexes_set;
- Line 3901: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: indexedColsPtr = &cachedMetadata->regular_indexes_set;
- Line 3902: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rangeColsPtr   = &cachedMetadata->range_indexes_set;
- Line 3902: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: rangeColsPtr   = &cachedMetadata->range_indexes_set;
- Line 3991: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->regular_unique.find(col);
- Line 3992: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: uniqueIndex = (it != cachedMetadata->regular_unique.end()) ? it->second : isUniqueIndex_(table, col)
- Line 4068: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->composite_unique.find(col);
- Line 4069: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: compositeUnique = (it != cachedMetadata->composite_unique.end()) && it->second;
- Line 4144: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->sparse_unique.find(scol);
- Line 4145: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: sparseUnique = (it != cachedMetadata->sparse_unique.end()) ? it->second : isSparseIndexUnique_(table
- Line 4225: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->ttl_seconds.find(tcol);
- Line 4226: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->ttl_seconds.end()) ttlSeconds = it->second;
- Line 4253: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->fulltext_configs.find(fcol);
- Line 4254: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->fulltext_configs.end()) {
- Line 4289: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: for (size_t i = 0; i < cachedMetadata->partial_indexes.size(); ++i) {
- Line 4290: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto& col = cachedMetadata->partial_indexes[i];
- Line 4291: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 4292: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->partial_predicates.end())
- Line 4308: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_unique.find(pcol);
- Line 4309: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: partialUnique = (it != cachedMetadata->partial_unique.end()) ? it->second : isPartialIndexUnique_(ta
- Line 4347: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getIndexedCols = [&]() -> std::unordered_set<std::string> {
- Line 4351: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getRangeCols = [&]() -> std::unordered_set<std::string> {
- Line 4355: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getSparseCols = [&]() -> std::unordered_set<std::string> {
- Line 4359: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getGeoCols = [&]() -> std::unordered_set<std::string> {
- Line 4363: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
- Line 4367: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
- Line 4371: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
- Line 4375: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 4376: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result[col] = (it != cachedMetadata->partial_predicates.end()) ? it->second : "";
- Line 4574: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = cachedMetadata->fulltext_configs.find(fcol);
- Line 4575: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cachedMetadata->fulltext_configs.end()) {
- Line 0: severity=HIGH; category=uncategorized
  Context: ['// static', 'std::string SecondaryIndexManager::makeCompositeIndexMetaKey(std::string_view table, const std::vector<std::string>& columns) {', '\tsize_t total = 8 + table.size() + 1;', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\ttotal += columns[i].size();']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\tstd::vector<std::string> encoded_values;', '\tencoded_values.reserve(values.size());', '\tsize_t total = 4 + table.size() + 1 + pk.size();', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\ttotal += columns[i].size();']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\tstd::vector<std::string> encoded_values;', '\tencoded_values.reserve(values.size());', '\tsize_t total = 4 + table.size() + 1;', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\ttotal += columns[i].size();']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\tstd::vector<std::string> encodedVals;', '\tencodedVals.reserve(values.size());', '\tsize_t total = 5 + table.size() + 1; // "uidx:" + table + ":"', '\tfor (size_t i = 0; i < columns.size(); ++i) {', '\t\tif (i > 0) total += 1; // "+"']
  Confidence: band=high; score=0.78
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t// v1.3.5: cache per-column TTL seconds to avoid db.get on every insert', '\t\tfor (const auto& tcol : metadata.ttl_indexes) {', '\t\t\tmetadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);', '\t\t}', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\tcached.stopwords         = cfg.stopwords;', '\t\t\tcached.normalize_umlauts = cfg.normalize_umlauts;', '\t\t\tmetadata.fulltext_configs[fcol] = std::move(cached);', '\t\t}', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t// v1.3.5: cache per-column TTL seconds to avoid db.get on every insert', '\t\tfor (const auto& tcol : metadata.ttl_indexes) {', '\t\t\tmetadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);', '\t\t}', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\tcached.stopwords         = cfg.stopwords;', '\t\t\tcached.normalize_umlauts = cfg.normalize_umlauts;', '\t\t\tmetadata.fulltext_configs[fcol] = std::move(cached);', '\t\t}', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 310: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compatibility API: createIndex with IndexType enum
  Confidence: band=high; score=0.8
- Line 411: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(buf, sizeof(buf), "%020lld", (long long)expireTimestamp);
  Confidence: band=very_high; score=0.9
- Line 768: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy format (just "fulltext" marker) - return default config
  Confidence: band=high; score=0.8
- Line 983: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case 0: return numericOk ? (fvNum == rhsNum) : (fv == rhs);
  Confidence: band=very_high; score=0.9
- Line 984: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: case 1: return numericOk ? (fvNum != rhsNum) : (fv != rhs);
  Confidence: band=very_high; score=0.9
- Line 1246: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.regular_unique[col] = isUniqueIndex_(table, col);
- Line 1253: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.sparse_unique[col] = isSparseIndexUnique_(table, col);
- Line 1264: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.partial_predicates[col] = pred;
- Line 1265: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.partial_unique[col] = isPartialIndexUnique_(table, col);
- Line 1270: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);
- Line 1282: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.fulltext_configs[fcol] = std::move(cached);
- Line 1291: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 1291: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 1313: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (col.find('+') == std::string::npos) {
- Line 1322: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->regular_unique.find(col);
- Line 1391: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->composite_unique.find(col);
- Line 1603: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 1772: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 2294: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)morton);
  Confidence: band=very_high; score=0.9
- Line 2294: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)morton);
- Line 2435: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(maxBuf, sizeof(maxBuf), "%020lld", (long long)currentTimestamp);
  Confidence: band=very_high; score=0.9
- Line 2591: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (field.find(ph) == std::string::npos) { allFound = false; break; }
- Line 2592: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (field.find(ph) == std::string::npos) { allFound = false; break; }
  Confidence: band=very_high; score=0.9
- Line 2650: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto itLen = docLen.find(pk);
- Line 2650: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: const auto itLen = docLen.find(pk);
- Line 2651: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: const auto itLen = docLen.find(pk);
  Confidence: band=very_high; score=0.9
- Line 2696: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Public API: returns PKs only (deprecated, use scanFulltextWithScores for scores)
  Confidence: band=high; score=0.8
- Line 3434: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::microseconds(throttle_us));
- Line 3475: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& token : tokenize(*maybeVal, config))
- Line 3914: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.regular_unique[col] = isUniqueIndex_(table, col);
- Line 3921: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.sparse_unique[col] = isSparseIndexUnique_(table, col);
- Line 3932: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.partial_predicates[col] = pred;
- Line 3933: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.partial_unique[col] = isPartialIndexUnique_(table, col);
- Line 3939: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.ttl_seconds[tcol] = getTTLSeconds_(table, tcol);
- Line 3951: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metadata.fulltext_configs[fcol] = std::move(cached);
- Line 3960: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 3960: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 3981: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (col.find('+') == std::string::npos) {
- Line 3990: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->regular_unique.find(col);
- Line 4067: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->composite_unique.find(col);
- Line 4290: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cachedMetadata->partial_predicates.find(col);
- Line 4461: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t pos = col.find('+', start);
- Line 146: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idxmeta:";
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += "+";
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += "+";
- Line 179: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idx:";
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idx:";
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += "+";
  Confidence: band=high; score=0.74
- Line 184: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += "+";
- Line 189: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ":";
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ":";
- Line 213: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idx:";
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += "idx:";
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += "+";
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += "+";
- Line 223: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ":";
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ":";
- Line 254: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) total += 1; // "+"
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: total += 1 + encodedVals.back().size(); // ":" + encoded
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) key += "+";
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) key += "+";
- Line 272: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += ":";
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: key += ":";
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back('%');
  Confidence: band=high; score=0.74
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back('%');
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back('%');
- Line 300: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(kHex[c >> 4]);
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(kHex[c & 0x0F]);
- Line 303: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(static_cast<char>(c));
- Line 409: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // timestamp wird mit führenden Nullen auf 20 Zeichen padded für lexikografische Sortierung
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 510: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) colList += ", ";
- Line 546: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 547: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) colList += ", ";
- Line 761: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (s.is_string()) config.stopwords.emplace_back(s.get<std::string>());
  Confidence: band=high; score=0.74
- Line 767: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 863: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 980: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1114: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("put(tx): alte Entity für PK={} nicht deserialisierbar", pk); }
- Line 1146: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { THEMIS_WARN("erase(tx): alte Entity für PK={} nicht deserialisierbar", pk); }
- Line 1190: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 1227: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>* indexedColsPtr = nullptr;
  Confidence: band=medium; score=0.66
- Line 1228: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>* rangeColsPtr   = nullptr;
  Confidence: band=medium; score=0.66
- Line 1229: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> indexedColsMiss, rangeColsMiss;
  Confidence: band=medium; score=0.66
- Line 1262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 1262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 1262: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 1382: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybe);
  Confidence: band=high; score=0.74
- Line 1415: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) valueStr += ", ";
  Confidence: band=high; score=0.74
- Line 1416: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) valueStr += ", ";
- Line 1512: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1579: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint32_t> tf;
  Confidence: band=medium; score=0.66
- Line 1600: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> partialCols;
  Confidence: band=medium; score=0.66
- Line 1666: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getSparseCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 1670: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getGeoCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 1674: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 1678: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 1682: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
  Confidence: band=medium; score=0.66
- Line 1684: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> result;
  Confidence: band=medium; score=0.66
- Line 1774: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.emplace_back(col.substr(start));
  Confidence: band=high; score=0.74
- Line 1790: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybe);
  Confidence: band=high; score=0.74
- Line 1843: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1899: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
- Line 1986: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(BaseEntity::deserialize(pk, *blob));
  Confidence: band=high; score=0.74
- Line 1988: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2025: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 2026: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) colList += ", ";
- Line 2037: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pks.emplace_back(rest);
  Confidence: band=high; score=0.74
- Line 2060: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(BaseEntity::deserialize(pk, *blob));
  Confidence: band=high; score=0.74
- Line 2062: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2113: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: startKey += '\xFF'; // Skip to next value
- Line 2209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(pk);
  Confidence: band=high; score=0.74
- Line 2218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(*it);
  Confidence: band=high; score=0.74
- Line 2260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(pk);
  Confidence: band=high; score=0.74
- Line 2512: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (in_quotes) current.push_back(c); else cleaned.push_back(c);
- Line 2518: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> tokenResults;
  Confidence: band=medium; score=0.66
- Line 2525: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (i) concat.push_back(' ');
- Line 2549: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokenResults.emplace_back(std::move(pks));
  Confidence: band=high; score=0.74
- Line 2558: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersectionSet = tokenResults[0];
  Confidence: band=medium; score=0.66
- Line 2560: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection;
  Confidence: band=medium; score=0.66
- Line 2592: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (field.find(ph) == std::string::npos) { allFound = false; break; }
- Line 2596: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!keep) toErase.emplace_back(pk);
  Confidence: band=high; score=0.74
- Line 2610: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> universe;
  Confidence: band=medium; score=0.66
- Line 2622: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> docLen;
  Confidence: band=medium; score=0.66
- Line 2631: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { dl = static_cast<double>(std::stoull(s)); } catch (...) { dl = 0.0; }
- Line 2665: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { tf = static_cast<double>(std::stoul(sTF)); } catch (...) { tf = 1.0; }
- Line 2711: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pks.emplace_back(result.pk);
  Confidence: band=high; score=0.74
- Line 2754: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> tokenResults;
  Confidence: band=medium; score=0.66
- Line 2766: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokenResults.emplace_back(std::move(pks));
  Confidence: band=high; score=0.74
- Line 2775: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> candidates = tokenResults[0];
  Confidence: band=medium; score=0.66
- Line 2777: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection;
  Confidence: band=medium; score=0.66
- Line 2823: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2892: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::unordered_set<std::string>> tokenToDocs;
  Confidence: band=medium; score=0.66
- Line 2893: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> pkScores;
  Confidence: band=medium; score=0.66
- Line 2934: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.emplace_back(FulltextResult{pk, score});
  Confidence: band=high; score=0.74
- Line 2967: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.emplace_back(std::move(current));
  Confidence: band=high; score=0.74
- Line 3061: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allStats.emplace_back(getIndexStats(table, column));
  Confidence: band=high; score=0.74
- Line 3224: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybeVal);
  Confidence: band=high; score=0.74
- Line 3494: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 3545: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*mv);
  Confidence: band=high; score=0.74
- Line 3636: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) colList += ", ";
  Confidence: band=high; score=0.74
- Line 3637: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) colList += ", ";
- Line 3836: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 3871: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) {
- Line 3896: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>* indexedColsPtr = nullptr;
  Confidence: band=medium; score=0.66
- Line 3897: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>* rangeColsPtr   = nullptr;
  Confidence: band=medium; score=0.66
- Line 3898: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> indexedColsMiss, rangeColsMiss;
  Confidence: band=medium; score=0.66
- Line 3930: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 3930: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 3930: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: metadata.partial_indexes.emplace_back(col);
  Confidence: band=high; score=0.74
- Line 4058: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybe);
  Confidence: band=high; score=0.74
- Line 4079: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) valueStr += ", ";
  Confidence: band=high; score=0.74
- Line 4079: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) valueStr += ", ";
  Confidence: band=high; score=0.74
- Line 4080: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) valueStr += ", ";
- Line 4102: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (i > 0) valueStr += ", ";
  Confidence: band=high; score=0.74
- Line 4103: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) valueStr += ", ";
- Line 4127: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> sparseCols;
  Confidence: band=medium; score=0.66
- Line 4173: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> geoCols;
  Confidence: band=medium; score=0.66
- Line 4199: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4207: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> ttlCols;
  Confidence: band=medium; score=0.66
- Line 4238: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> fulltextCols;
  Confidence: band=medium; score=0.66
- Line 4267: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint32_t> tf;
  Confidence: band=medium; score=0.66
- Line 4287: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> partialCols;
  Confidence: band=medium; score=0.66
- Line 4355: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getSparseCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 4359: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getGeoCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 4363: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getTTLCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 4367: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getFulltextCols = [&]() -> std::unordered_set<std::string> {
  Confidence: band=medium; score=0.66
- Line 4371: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: auto getPartialCols = [&]() -> std::unordered_map<std::string, std::string> {
  Confidence: band=medium; score=0.66
- Line 4373: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> result;
  Confidence: band=medium; score=0.66
- Line 4463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: columns.emplace_back(col.substr(start));
  Confidence: band=high; score=0.74
- Line 4479: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.emplace_back(*maybe);
  Confidence: band=high; score=0.74
- Line 4532: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 4588: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> uniqueTokens(tokens.begin(), tokens.end());
  Confidence: band=medium; score=0.66
- Line 2658: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);
  Confidence: band=medium; score=0.6

### src/index/vector_index.cpp
Total findings: 169

- Line 638: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_WARN("VectorIndexManager::init - Failed to load index: {}, creating new index", loadStatus.me
- Line 647: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: space = new hnswlib::L2Space(dim);
- Line 650: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: space = new hnswlib::InnerProductSpace(dim);
- Line 652: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: space = new hnswlib::InnerProductSpace(dim);
- Line 654: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* appr = new hnswlib::HierarchicalNSW<float>(space, 1000 /*initial*/, M, efConstruction);
- Line 731: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vecOpt && vecOpt->size() == static_cast<size_t>(dim_)) {
- Line 739: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (!qv || qv->size() != static_cast<size_t>(dim_)) return true;
- Line 815: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vecOpt && vecOpt->size() == static_cast<size_t>(dim_)) {
- Line 822: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (!qv || qv->size() != static_cast<size_t>(dim_)) return true;
- Line 853: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 876: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator id_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto id_it = pkToId_.find(pk);
- Line 889: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: } else if (cache_it->second != new_vec) {
- Line 891: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cache_it->second = new_vec;
- Line 1054: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 1067: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 1074: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ann_id = static_cast<int64_t>(it->second);
- Line 1076: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1076: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1088: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (v->size() != static_cast<size_t>(dim_)) return Status::Error("addEntity: Vektordimension passt n
- Line 1135: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 1148: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 1155: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ann_id = static_cast<int64_t>(it->second);
- Line 1157: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1157: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1265: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cache_.end() && it->second.size() == static_cast<size_t>(dim_)) {
- Line 1265: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cache_.end() && it->second.size() == static_cast<size_t>(dim_)) {
- Line 1274: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vec && vec->size() == static_cast<size_t>(dim_)) {
- Line 1281: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (by && by->size() == static_cast<size_t>(dim_)) {
- Line 1322: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vecOpt->size() == static_cast<size_t>(dim_)) v = *vecOpt;
- Line 1329: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (qv && qv->size() == static_cast<size_t>(dim_)) {
- Line 1367: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto& vec = cache_ptrs[i]->second;
- Line 1439: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto topk = appr->searchKnn(q.data(), static_cast<size_t>(k));
- Line 1570: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto raw = ann_backend_->search(q.data(), static_cast<size_t>(dim_), static_cast<int>(k));
- Line 2233: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: if (metric_ == Metric::L2) space = new hnswlib::L2Space(dim_);
- Line 2234: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: else space = new hnswlib::InnerProductSpace(dim_);
- Line 2279: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* appr = new hnswlib::HierarchicalNSW<float>(space, tempPath, false);
- Line 2300: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* appr = new hnswlib::HierarchicalNSW<float>(space, indexPath, false);
- Line 2359: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (v->size() != static_cast<size_t>(dim_)) return Status::Error("addEntity(mvcc): Vektordimension p
- Line 2406: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pkToId_.find(pk);
- Line 2901: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto rope_stats = rotary_embedding_->getStats();
- Line 2927: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto rotated = rotary_embedding_->rotate(*vec_opt, position);
- Line 2971: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto rotated = rotary_embedding_->rotateRelational(*vec_opt, relation_type);
- Line 3010: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto rotated_query = rotary_embedding_->rotate(query, query_position);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 151: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Try to load configuration from scaling_optimizations.yaml (new path first, then legacy)
  Confidence: band=high; score=0.8
- Line 844: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (storage_vectors.find(pk) == storage_vectors.end())
- Line 852: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pkToId_.find(pk);
- Line 885: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: try { appr->addPoint(new_vec.data(), id); } catch (...) {}
- Line 1062: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 1076: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1143: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 1157: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const bool added = ann_backend_->add(ann_id, cache_[pk].data(), static_cast<size_t>(dim_));
- Line 1182: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: fehlgeschlagen = nullptr;
  Context: THEMIS_WARN("removeByPk: RocksDB delete fehlgeschlagen für key={}", key);
- Line 1231: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto prefetch = [](const void* ptr) {
- Line 1367: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const auto& vec = cache_ptrs[i]->second;
- Line 1439: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto topk = appr->searchKnn(q.data(), static_cast<size_t>(k));
- Line 1527: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (wl.find(pk) != wl.end()) {
- Line 1789: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto [st, pks] = secondaryIdx->scanKeysEqual(objectName_, filter.field, filter.value);
- Line 1820: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto [st, pks] = secondaryIdx->scanKeysEqual(objectName_, filter.field, val);
- Line 2040: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto [st, pks] = secondaryIdx->scanKeysEqual(objectName_, filter.field, filter.value);
- Line 2057: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto [st, pks] = secondaryIdx->scanKeysEqual(objectName_, filter.field, val);
- Line 2242: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space = nullptr;
  Context: delete space;
- Line 2249: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space = nullptr;
  Context: delete space;
- Line 2271: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space = nullptr;
  Context: delete space;
- Line 2289: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space = nullptr;
  Context: delete space;
- Line 2293: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Original plaintext load (backward compatibility)
  Confidence: band=high; score=0.8
- Line 2296: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: space = nullptr;
  Context: delete space;
- Line 2414: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 2624: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t j = i + 1; j < std::min(i + 10, pks.size()); ++j) {
- Line 2813: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto status = quantizer_->train(train_data);
- Line 168: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto hnsw_opt = config["hnsw_optimization"];
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto lp = hnsw_opt["layer_pruning"];
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto als = hnsw_opt["adaptive_layer_selection"];
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto bi = hnsw_opt["batch_insert"];
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 331: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 661: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 679: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 756: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idToPk_.push_back(pk);
  Confidence: band=high; score=0.74
- Line 757: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: idToPk_.push_back(pk);
- Line 765: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 836: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 845: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_delete.push_back(pk);
  Confidence: band=high; score=0.74
- Line 846: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_delete.push_back(pk);
- Line 855: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->markDelete(it->second); } catch (...) {}
- Line 879: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: idToPk_.push_back(pk);
  Confidence: band=high; score=0.74
- Line 880: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: idToPk_.push_back(pk);
- Line 885: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->addPoint(new_vec.data(), id); } catch (...) {}
- Line 897: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->addPoint(new_vec.data(), id_it->second); } catch (...) {}
- Line 969: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1062: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 1100: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1143: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 1182: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: THEMIS_WARN("removeByPk: RocksDB delete fehlgeschlagen für key={}", key);
- Line 1192: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->markDelete(it->second); } catch (...) {}
- Line 1211: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->markDelete(it->second); } catch (...) {}
- Line 1251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: heap.push_back({pk, dist});
- Line 1292: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1313: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1343: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cache_ptrs.push_back(&entry);
  Confidence: band=high; score=0.74
- Line 1358: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cache_ptrs.push_back(&entry);
- Line 1460: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (id < idToPk_.size()) out.push_back({idToPk_[id], d});
- Line 1464: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1493: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1529: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tmp.push_back({pk, d});
- Line 1542: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(r);
  Confidence: band=high; score=0.74
- Line 1543: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered.push_back(r);
- Line 1560: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back({idToPk_[idx], r.distance});
  Confidence: band=high; score=0.74
- Line 1576: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({idToPk_[idx], r.distance});
- Line 1634: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back({idToPk_[id], d});
- Line 1759: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
  Confidence: band=high; score=0.74
- Line 1760: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
- Line 1768: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> whitelistSet;
  Confidence: band=medium; score=0.66
- Line 1778: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1818: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> inResults;
  Confidence: band=medium; score=0.66
- Line 1878: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection;
  Confidence: band=medium; score=0.66
- Line 1907: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
  Confidence: band=high; score=0.74
- Line 1908: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: legacyFilters.push_back({f.field, f.value, AttributeFilter::Op::EQUALS});
- Line 1943: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(c);
  Confidence: band=high; score=0.74
- Line 1944: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(c);
- Line 1960: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({pk, dist});
  Confidence: band=high; score=0.74
- Line 1961: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({pk, dist});
- Line 1980: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { continue; }
- Line 1984: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({pk, dist});
  Confidence: band=high; score=0.74
- Line 1985: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({pk, dist});
- Line 2023: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> whitelistSet;
  Confidence: band=medium; score=0.66
- Line 2032: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2055: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> inResults;
  Confidence: band=medium; score=0.66
- Line 2094: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection;
  Confidence: band=medium; score=0.66
- Line 2165: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: tempFile.close();
- Line 2183: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: encFile.close();
- Line 2199: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2242: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete space;
- Line 2249: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete space;
- Line 2257: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: encFile.close();
- Line 2271: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete space;
- Line 2276: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: tempFile.close();
- Line 2296: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete space;
- Line 2319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: idToPk_.push_back(line);
- Line 2327: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 2373: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2414: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->addPoint(cache_[pk].data(), id); } catch (...) { /* evtl. schon vorhanden */ }
- Line 2442: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { appr->markDelete(it->second); } catch (...) {}
- Line 2479: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_serialized.push_back(std::move(serialized));
  Confidence: band=high; score=0.74
- Line 2519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_serialized.push_back(std::move(serialized));
  Confidence: band=high; score=0.74
- Line 2520: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_serialized.push_back(std::move(serialized));
- Line 2521: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_keys.push_back(makeObjectKey(pk));
- Line 2618: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pks.push_back(pk);
  Confidence: band=high; score=0.74
- Line 2619: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pks.push_back(pk);
- Line 2625: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.push_back(dist);
  Confidence: band=high; score=0.74
- Line 2625: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.push_back(dist);
  Confidence: band=high; score=0.74
- Line 2626: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: distances.push_back(dist);
- Line 2736: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outliers.push_back(pk);
  Confidence: band=high; score=0.74
- Line 2737: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: outliers.push_back(pk);
- Line 2795: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: train_data.push_back(vec);
  Confidence: band=high; score=0.74
- Line 2796: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: train_data.push_back(vec);
- Line 3055: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/index/process_graph.cpp
Total findings: 104

- Line 981: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: fields["completed_at"] = static_cast<int64_t>(*token->completed_at_ms);
- Line 1698: severity=CRITICAL; category=missing_dtor
  Description: Class StackEntry allocates resources but has no destructor
  Remediation: Add explicit destructor: ~StackEntry() { /* cleanup */ }
  Context: class/struct StackEntry
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 1279: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: db_.scanPrefix("process:token:", [&](std::string_view key, std::string_view val) -> bool {
- Line 1735: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Push new entry for neighbor
- Line 1735: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Push new entry for neighbor
- Line 2097: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: jr.joined_data = it->second;
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nodes.push_back(item.get<std::string>());
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodes.push_back(item.get<std::string>());
- Line 276: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto val = variables[expr];
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto leftVal = variables[left];
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 586: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ProcessGraphManager::validateProcess(std::string_view process_id) const {
  Confidence: band=high; score=0.74
- Line 666: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Process has no start event");
  Confidence: band=high; score=0.74
- Line 667: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Process has no start event");
- Line 670: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("Process has no end event");
- Line 674: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> hasIncoming;
  Confidence: band=medium; score=0.66
- Line 675: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> hasOutgoing;
  Confidence: band=medium; score=0.66
- Line 681: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Node '" + id + "' has no incoming edges");
  Confidence: band=high; score=0.74
- Line 681: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Node '" + id + "' has no incoming edges");
  Confidence: band=high; score=0.74
- Line 682: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Node '" + id + "' has no incoming edges");
- Line 685: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Node '" + id + "' has no outgoing edges");
- Line 691: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.errors.push_back("Edge '" + edge.edge_id + "' references non-existent source '" + edge.from_node + "'");
  Confidence: band=high; score=0.74
- Line 692: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Edge '" + edge.edge_id + "' references non-existent source '" + edge.from_n
- Line 695: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.errors.push_back("Edge '" + edge.edge_id + "' references non-existent target '" + edge.to_nod
- Line 722: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("XOR gateway '" + id + "' has multiple outgoing edges but no default flow");
  Confidence: band=high; score=0.74
- Line 722: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.warnings.push_back("XOR gateway '" + id + "' has multiple outgoing edges but no default flow");
  Confidence: band=high; score=0.74
- Line 723: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.warnings.push_back("XOR gateway '" + id + "' has multiple outgoing edges but no default flow"
- Line 853: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1019: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: visited_all.push_back(n);
  Confidence: band=high; score=0.74
- Line 1019: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: visited_all.push_back(n);
  Confidence: band=high; score=0.74
- Line 1020: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: visited_all.push_back(n);
- Line 1064: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: token->traversed_edges.push_back(edge.edge_id);
  Confidence: band=high; score=0.74
- Line 1065: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: token->traversed_edges.push_back(edge.edge_id);
- Line 1071: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: token->traversed_edges.push_back(edge.edge_id);
- Line 1080: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: token->traversed_edges.push_back(edge.edge_id);
  Confidence: band=high; score=0.74
- Line 1081: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: token->traversed_edges.push_back(edge.edge_id);
- Line 1090: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: token->traversed_edges.push_back(outgoing[0].edge_id);
- Line 1095: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: token->visited_nodes.push_back(targetNode);
- Line 1140: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ProcessGraphManager::Status ProcessGraphManager::suspendProcess(std::string_view instance_id) {
  Confidence: band=high; score=0.74
- Line 1157: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: ProcessGraphManager::Status ProcessGraphManager::resumeProcess(std::string_view instance_id) {
  Confidence: band=high; score=0.74
- Line 1369: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1444: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1481: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1606: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(metrics);
  Confidence: band=high; score=0.74
- Line 1607: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(metrics);
- Line 1638: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> adjacency;
  Confidence: band=medium; score=0.66
- Line 1639: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> nodeDurations;
  Confidence: band=medium; score=0.66
- Line 1660: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adjacency[from].push_back(to);
  Confidence: band=high; score=0.74
- Line 1661: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adjacency[from].push_back(to);
- Line 1706: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({startNode, 0.0, {}, {}});
- Line 1719: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entry.path.push_back(entry.node);
- Line 1735: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back({neighbor, entry.cumDuration, entry.path, entry.visited});
  Confidence: band=high; score=0.74
- Line 1736: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({neighbor, entry.cumDuration, entry.path, entry.visited});
- Line 1780: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hyperedge.source_nodes.push_back(src.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1781: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hyperedge.source_nodes.push_back(src.get<std::string>());
- Line 1785: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1794: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: hyperedge.target_nodes.push_back(tgt.get<std::string>());
  Confidence: band=high; score=0.74
- Line 1795: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hyperedge.target_nodes.push_back(tgt.get<std::string>());
- Line 1799: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1828: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1920: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(token));
  Confidence: band=high; score=0.74
- Line 1921: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(token));
- Line 1949: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { vars = nlohmann::json::parse(*varsStr); } catch (...) {}
- Line 1959: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1984: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(token));
  Confidence: band=high; score=0.74
- Line 2028: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(jr));
  Confidence: band=high; score=0.74
- Line 2029: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(jr));
- Line 2056: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2078: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (varsStr) { try { vars = nlohmann::json::parse(*varsStr); } catch (...) {} }
- Line 2098: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(jr));
- Line 2140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ar));
  Confidence: band=high; score=0.74
- Line 2141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(ar));
- Line 2176: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (varsStr) { try { vars = nlohmann::json::parse(*varsStr); } catch (...) {} }
- Line 2203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ar));
  Confidence: band=high; score=0.74
- Line 2204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(ar));
- Line 2236: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.is_number()) emb.push_back(v.get<float>());
  Confidence: band=high; score=0.74
- Line 2237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (v.is_number()) emb.push_back(v.get<float>());
- Line 2240: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 2369: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(tok));
  Confidence: band=high; score=0.74
- Line 2370: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(tok));
- Line 2396: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (ss >> word) queryTokens.push_back(word);
- Line 2422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(sp));
  Confidence: band=high; score=0.74
- Line 2423: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(sp));
- Line 2501: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nodeStats[info.currentNode].durations_ms.push_back(info.durationMs);
- Line 2514: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, NodeBaseline> baselines;
  Confidence: band=medium; score=0.66
- Line 2550: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ar));
  Confidence: band=high; score=0.74
- Line 2551: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(ar));
- Line 2562: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string> normalSet(normalNodes.begin(), normalNodes.end());
  Confidence: band=medium; score=0.66
- Line 2577: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(ar));
  Confidence: band=high; score=0.74
- Line 2578: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(ar));
- Line 2724: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (vs) { try { vars = nlohmann::json::parse(*vs); } catch (...) {} }
- Line 2764: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (vs) { try { vars = nlohmann::json::parse(*vs); } catch (...) {} }
- Line 2821: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (vs) { try { vars = nlohmann::json::parse(*vs); } catch (...) {} }
- Line 2861: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(rs));
  Confidence: band=high; score=0.74
- Line 2861: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(rs));
  Confidence: band=high; score=0.74
- Line 2862: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(rs));
- Line 2950: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 3042: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: if (vs) { try { vars = nlohmann::json::parse(*vs); } catch (...) {} }
- Line 3096: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(mmr));
  Confidence: band=high; score=0.74
- Line 3097: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(mmr));
- Line 3151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: targets.push_back(edge.to_node);
  Confidence: band=high; score=0.74
- Line 3152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: targets.push_back(edge.to_node);

### src/index/graph_index.cpp
Total findings: 97

- Line 176: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = s.find(',', start);
- Line 179: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator l may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto l = part.find_first_not_of(" \t\n\r");
- Line 180: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator r may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto r = part.find_last_not_of(" \t\n\r");
- Line 291: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = outEdges_.find(std::string(fromPk));
- Line 325: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = inEdges_.find(std::string(toPk));
- Line 428: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = outEdges_.find(node);
- Line 629: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator outIt may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto outIt = outEdges_.find(fromPk);
- Line 639: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator inIt may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto inIt = inEdges_.find(toPk);
- Line 964: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = outEdges_.find(node);
- Line 1026: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = prev.find(current);
- Line 1138: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = prev.find(current);
- Line 1244: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = prev.find(current);
- Line 1291: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator pos may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto pos = s.find(',', start);
- Line 1294: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator l may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto l = part.find_first_not_of(" \t\n\r");
- Line 1295: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator r may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto r = part.find_last_not_of(" \t\n\r");
- Line 1581: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto parseTemporalField = [&edge](std::string_view field) -> std::optional<int64_t> {
- Line 1640: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto parseTemporalField = [&edge](std::string_view field) -> std::optional<int64_t> {
- Line 2155: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator best_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto best_it = best_cost.find(adj.targetPk);
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\t\tif (!dist.count(adj.targetPk) || newCost < dist[adj.targetPk]) {', '\t\t\t\t\t\tdist[adj.targetPk] = newCost;', '\t\t\t\t\t\tprev[adj.targetPk] = node;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\tif (!dist.count(neighbor) || newCost < dist[neighbor]) {', '\t\t\t\t\tdist[neighbor] = newCost;', '\t\t\t\t\tprev[neighbor] = node;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\t\tif (!dist.count(adj.targetPk) || newCost < dist[adj.targetPk]) {', '\t\t\t\t\t\tdist[adj.targetPk] = newCost;', '\t\t\t\t\t\tprev[adj.targetPk] = node;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['\t\t\t\tdouble newCost = dist[node] + weight;', '', '\t\t\t\tif (!dist.count(neighbor) || newCost < dist[neighbor]) {', '\t\t\t\t\tdist[neighbor] = newCost;', '\t\t\t\t\tprev[neighbor] = node;']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 157: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // `encrypt_fields` (preferred) or fall back to legacy `_sensitive` boolean.
  Confidence: band=high; score=0.8
- Line 188: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backwards compat: if no explicit list and _sensitive==true, encrypt weight+metadata
  Confidence: band=high; score=0.8
- Line 685: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // legacy: no graphId
  Confidence: band=high; score=0.8
- Line 894: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // - graph:out:<fromPk>:<edgeId>  (legacy)
  Confidence: band=high; score=0.8
- Line 896: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // legacy: no graphId
  Confidence: band=high; score=0.8
- Line 915: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Support two formats: with graphId or legacy without
  Confidence: band=high; score=0.8
- Line 1272: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // `encrypt_fields` (preferred) or fall back to legacy `_sensitive` boolean.
  Confidence: band=high; score=0.8
- Line 1303: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backwards compat: if no explicit list and _sensitive==true, encrypt weight+metadata
  Confidence: band=high; score=0.8
- Line 1689: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // - legacy: graph:out:<fromPk>:<edgeId>
  Confidence: band=high; score=0.8
- Line 1692: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // fallback to legacy parsing used elsewhere
  Confidence: band=high; score=0.8
- Line 2003: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(order.begin(), order.end(), req) == order.end()) {
- Line 2076: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: if (std::find(current.path.begin(), current.path.end(), req) == current.path.end()) {
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 169: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : j) if (v.is_string()) encryptList.push_back(v.get<std::string>());
- Line 171: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1));
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1
- Line 192: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encryptList.push_back("_weight");
- Line 193: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encryptList.push_back("metadata");
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(adj.targetPk);
  Confidence: band=high; score=0.74
- Line 295: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(adj.targetPk);
- Line 328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(adj.targetPk);
  Confidence: band=high; score=0.74
- Line 329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(adj.targetPk);
- Line 412: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 424: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(node);
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: order.push_back(node);
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(node);
- Line 503: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(node);
- Line 532: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(node);
- Line 651: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> nodes;
  Confidence: band=medium; score=0.66
- Line 663: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 721: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> nodes;
  Confidence: band=medium; score=0.66
- Line 755: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 759: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 766: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 802: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 806: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 813: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 877: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 967: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbors.push_back(adj.targetPk);
  Confidence: band=high; score=0.74
- Line 968: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: neighbors.push_back(adj.targetPk);
- Line 1025: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(current);
- Line 1030: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(start);
- Line 1057: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 1137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(current);
- Line 1142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(start);
- Line 1243: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(current);
- Line 1248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(start);
- Line 1284: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& v : j) if (v.is_string()) encryptList.push_back(v.get<std::string>());
- Line 1286: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1295: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1));
  Confidence: band=high; score=0.74
- Line 1296: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (l != std::string::npos && r != std::string::npos) encryptList.push_back(part.substr(l, r - l + 1
- Line 1307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encryptList.push_back("_weight");
- Line 1308: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: encryptList.push_back("metadata");
- Line 1418: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 1427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(node);
- Line 1538: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.path.push_back(curr);
- Line 1542: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.path.push_back(start);
- Line 1590: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1649: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1899: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(node);
- Line 1945: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order.push_back(node);
- Line 1993: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(vertex);
  Confidence: band=high; score=0.74
- Line 1994: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered.push_back(vertex);
- Line 2124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: neighbors.push_back(std::move(info));

### src/index/gpu_vector_index.cpp
Total findings: 84

- Line 1067: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (vectors[i].size() != static_cast<size_t>(pImpl->dimension) ||
- Line 1078: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: allocatedBytes = pImpl->bytesPerVector() * static_cast<uint64_t>(ids.size());
- Line 1134: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = true;
- Line 1137: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1141: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1333: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: int32_t metric = static_cast<int32_t>(pImpl->config.metric);
- Line 1427: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = true;
- Line 1433: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1438: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1452: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1460: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1469: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 1472: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: pImpl->oversubBulkLoading_ = false;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 161: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const float* vecPtr = data->data() + vi * dim;
- Line 193: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: return static_cast<uint64_t>(dimension) * sizeof(float);
- Line 222: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (Backend candidateBackend : getBackendPriorityOrder()) {
- Line 242: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "GPUVectorIndex: Falling back to CPU backend\n";
  Confidence: band=very_high; score=0.9
- Line 244: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "GPUVectorIndex: Using CPU backend\n";
  Confidence: band=very_high; score=0.9
- Line 257: severity=HIGH; category=memory_order
  Description: memory_order_relaxed used — potential visibility issue
  Remediation: Use memory_order_acquire/release unless truly lock-free
  Context: std::to_string(indexCounter.fetch_add(1, std::memory_order_relaxed));
- Line 281: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "GPUVectorIndex: GPU memory oversubscription enabled"
  Confidence: band=very_high; score=0.9
- Line 319: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: mgr.DeallocateGPU(vramAllocatedBytes, vramBudgetTag);
- Line 360: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "GPUVectorIndex: Using CUDA backend\n";
  Confidence: band=very_high; score=0.9
- Line 375: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "GPUVectorIndex: Using HIP backend\n";
  Confidence: band=very_high; score=0.9
- Line 387: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "GPUVectorIndex: Using Vulkan backend\n";
  Confidence: band=very_high; score=0.9
- Line 549: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(bytes, vramBudgetTag);
- Line 1064: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: bool canUseFastPath = (pImpl->oversubManager == nullptr);
- Line 1064: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: bool canUseFastPath = (pImpl->oversubManager == nullptr);
- Line 1076: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: uint64_t allocatedBytes = 0;
- Line 1078: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: allocatedBytes = pImpl->bytesPerVector() * static_cast<uint64_t>(ids.size());
- Line 1080: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!mgr.TryAllocateGPU(allocatedBytes, "vector_batch", pImpl->vramBudgetTag)) {
- Line 1099: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (allocatedBytes > 0) {
- Line 1100: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(
- Line 1101: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: allocatedBytes, pImpl->vramBudgetTag);
- Line 1106: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (allocatedBytes > 0) {
- Line 1107: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: pImpl->vramAllocatedBytes += allocatedBytes;
- Line 1379: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Stored for future compatibility; callers set the metric via Config
  Confidence: band=high; score=0.8
- Line 1408: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: themis::gpu::GPUMemoryManager::GetInstance().DeallocateGPU(
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.emplace_back(dist, globalOffset + vi);
  Confidence: band=high; score=0.74
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[idx], candidates[i].first});
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({vectorIds[idx], candidates[i].first});
- Line 440: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 492: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vectorIds.push_back(id);
- Line 493: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vectorData.push_back(vector);
- Line 614: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[index], distance});
  Confidence: band=high; score=0.74
- Line 615: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({vectorIds[index], distance});
- Line 672: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[distances[i].second], distances[i].first});
  Confidence: band=high; score=0.74
- Line 673: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({vectorIds[distances[i].second], distances[i].first});
- Line 726: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[idx], dist});
  Confidence: band=high; score=0.74
- Line 727: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({vectorIds[idx], dist});
- Line 751: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(searchCPU(query, k));
- Line 776: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(searchCPU(q, k));
  Confidence: band=high; score=0.74
- Line 777: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(searchCPU(q, k));
- Line 807: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back({vectorIds[idx], dist});
  Confidence: band=high; score=0.74
- Line 808: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back({vectorIds[idx], dist});
- Line 811: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(batch));
- Line 867: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({vectorIds[idx], dist});
  Confidence: band=high; score=0.74
- Line 868: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({vectorIds[idx], dist});
- Line 905: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(searchCPU(q, k));
  Confidence: band=high; score=0.74
- Line 906: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(searchCPU(q, k));
- Line 931: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back({vectorIds[idx], dist});
  Confidence: band=high; score=0.74
- Line 932: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back({vectorIds[idx], dist});
- Line 935: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(batch));
- Line 1093: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pImpl->vectorIds.push_back(ids[i]);
  Confidence: band=high; score=0.74
- Line 1094: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pImpl->vectorIds.push_back(ids[i]);
- Line 1095: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pImpl->vectorData.push_back(vectors[i]);
- Line 1098: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1178: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(pImpl->searchOversubscribed(query, k));
  Confidence: band=high; score=0.74
- Line 1179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(pImpl->searchOversubscribed(query, k));
- Line 1206: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queryResults.push_back({pImpl->vectorIds[index], distance});
  Confidence: band=high; score=0.74
- Line 1207: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queryResults.push_back({pImpl->vectorIds[index], distance});
- Line 1210: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(queryResults));
- Line 1235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(pImpl->searchCPU(query, k));
- Line 1255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(pImpl->searchCPU(query, k));
- Line 1267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(pImpl->searchCPU(query, k));
  Confidence: band=high; score=0.74
- Line 1268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(pImpl->searchCPU(query, k));
- Line 1434: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "GPUVectorIndex: loadIndex read error at vector " << i << " (ID length)\n";
- Line 1439: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "GPUVectorIndex: loadIndex rejected oversized ID (" << idLen
- Line 1583: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: backends.push_back(Backend::CPU);
  Confidence: band=high; score=0.74
- Line 1584: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backends.push_back(Backend::CPU);
- Line 1589: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backends.push_back(Backend::HIP);
- Line 1598: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: backends.push_back(Backend::CUDA);

### src/index/cuda_hnsw_graph_traversal.cpp
Total findings: 71

- Line 346: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // This pre-allocation eliminates per-launch cudaMalloc/cudaFree, reducing
  Confidence: band=very_high; score=0.99
- Line 352: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const size_t new_pool_sz  = impl_->max_batch_size * vis_per_q;
- Line 354: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cudaError_t ve = cudaMalloc(&impl_->d_visited_pool, new_pool_sz);
- Line 354: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: cudaError_t ve = cudaMalloc(&impl_->d_visited_pool, new_pool_sz);
  Confidence: band=very_high; score=0.99
- Line 356: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->visited_pool_bytes = new_pool_sz;
- Line 363: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->visited_pool_bytes = 0;
- Line 365: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: "cudaMalloc(visited_pool, {} bytes) failed — "
  Confidence: band=very_high; score=0.99
- Line 447: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const size_t pool_capacity = (vis_per_q > 0 && impl_->visited_pool_bytes > 0)
- Line 483: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: if (cudaMalloc(&impl_->d_result_ids,
  Confidence: band=very_high; score=0.99
- Line 485: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: cudaMalloc(&impl_->d_result_scores,
  Confidence: band=very_high; score=0.99
- Line 560: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: const cudaError_t e1 = cudaMalloc(&d_pass_ids,
  Confidence: band=very_high; score=0.99
- Line 563: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: ? cudaMalloc(&d_pass_scores,
  Confidence: band=very_high; score=0.99
- Line 569: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: bool queries_ok = (cudaMalloc(&d_queries_all,
  Confidence: band=very_high; score=0.99
- Line 733: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: << ", visited_pool=" << (impl_ ? impl_->visited_pool_bytes : 0) << "B"
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 14: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: * the implementation allocates device memory and issues kernel launches via
- Line 237: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // cudaMalloc/cudaFree overhead.  Each kernel thread zeroes its own slice
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: // cudaMalloc/cudaFree overhead.  Each kernel thread zeroes its own slice
  Confidence: band=very_high; score=0.9
- Line 321: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMalloc(vectors) failed");
  Confidence: band=very_high; score=0.9
- Line 326: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(impl_->d_vectors, vectors, vec_bytes, cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: THEMIS_ERROR("CudaHnswTraversalEngine::buildIndex: cudaMalloc(graph) failed");
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(impl_->d_offsets,    bottom.offsets.data(),    off_bytes, cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpy(impl_->d_offsets,    bottom.offsets.data(),    off_bytes, cudaMemcpyHostToDevice);
- Line 342: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(impl_->d_neighbours, bottom.neighbours.data(), nb_bytes,  cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 342: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpy(impl_->d_neighbours, bottom.neighbours.data(), nb_bytes,  cudaMemcpyHostToDevice);
- Line 346: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // This pre-allocation eliminates per-launch cudaMalloc/cudaFree, reducing
  Confidence: band=very_high; score=0.9
- Line 346: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: // This pre-allocation eliminates per-launch cudaMalloc/cudaFree, reducing
  Confidence: band=very_high; score=0.9
- Line 358: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "allocated visited pool {} bytes "
- Line 362: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->d_visited_pool   = nullptr;
- Line 362: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->d_visited_pool   = nullptr;
- Line 365: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: "cudaMalloc(visited_pool, {} bytes) failed — "
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->flat_vectors.data(),
- Line 429: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!impl_->index_built || queries == nullptr || num_queries == 0) return {};
- Line 444: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: //   pre-allocated pool — no per-launch cudaMalloc/cudaFree needed.
  Confidence: band=very_high; score=0.9
- Line 444: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: //   pre-allocated pool — no per-launch cudaMalloc/cudaFree needed.
  Confidence: band=very_high; score=0.9
- Line 466: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(d_queries_all, queries,
  Confidence: band=very_high; score=0.9
- Line 468: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 480: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->d_result_ids    = nullptr;
- Line 481: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->d_result_scores = nullptr;
- Line 484: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: this_chunk * k * sizeof(int64_t)) != cudaSuccess ||
- Line 521: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_ids.data(), impl_->d_result_ids,
  Confidence: band=very_high; score=0.9
- Line 521: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpy(h_ids.data(), impl_->d_result_ids,
- Line 522: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: h_ids.size() * sizeof(int64_t),
- Line 523: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 524: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_scores.data(), impl_->d_result_scores,
  Confidence: band=very_high; score=0.9
- Line 526: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 537: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_queries_all);
  Confidence: band=very_high; score=0.9
- Line 560: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: const cudaError_t e1 = cudaMalloc(&d_pass_ids,
  Confidence: band=very_high; score=0.9
- Line 561: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: mp_chunk * pass_k * sizeof(int64_t));
- Line 563: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: ? cudaMalloc(&d_pass_scores,
  Confidence: band=very_high; score=0.9
- Line 569: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: bool queries_ok = (cudaMalloc(&d_queries_all,
  Confidence: band=very_high; score=0.9
- Line 573: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(d_queries_all, queries,
  Confidence: band=very_high; score=0.9
- Line 575: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyHostToDevice);
  Confidence: band=very_high; score=0.9
- Line 622: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_ids.data(), d_pass_ids,
  Confidence: band=very_high; score=0.9
- Line 623: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: h_ids.size() * sizeof(int64_t),
- Line 624: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 625: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpy(h_sc.data(),  d_pass_scores,
  Confidence: band=very_high; score=0.9
- Line 627: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyDeviceToHost);
  Confidence: band=very_high; score=0.9
- Line 642: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_queries_all);
  Confidence: band=very_high; score=0.9
- Line 656: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: return a.second == b.second;
  Confidence: band=very_high; score=0.9
- Line 691: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_pass_ids);
  Confidence: band=very_high; score=0.9
- Line 692: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaFree() without error checking
  Context: cudaFree(d_pass_scores);
  Confidence: band=very_high; score=0.9
- Line 757: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return impl_ && impl_->d_visited_pool != nullptr;
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back({static_cast<int64_t>(id), d});
- Line 530: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[gqi].push_back(
  Confidence: band=high; score=0.74
- Line 530: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[gqi].push_back(
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results[gqi].push_back(
- Line 634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_cands[gqi].emplace_back(score, id);
  Confidence: band=high; score=0.74
- Line 634: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_cands[gqi].emplace_back(score, id);
  Confidence: band=high; score=0.74
- Line 680: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[qi].push_back({c.second, c.first});
  Confidence: band=high; score=0.74
- Line 681: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results[qi].push_back({c.second, c.first});

### src/index/spatial_index.cpp
Total findings: 66

- Line 729: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache.find(pk_str);
- Line 730: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const geo::MBR mbr_to_remove = (it != cache.end()) ? it->second : sidecar.mbr;
- Line 796: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache.find(pk_str);
- Line 797: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const geo::MBR mbr_to_remove = (it != cache.end()) ? it->second : sidecar.mbr;
- Line 896: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: entry_mbr = cache_it->second;
- Line 1089: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache.find(pk);
- Line 1090: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != cache.end()) result.mbr = it->second;
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 217: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
  Confidence: band=very_high; score=0.9
- Line 217: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
- Line 223: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(buf, sizeof(buf), "%08d", z_bucket);
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "%08d", z_bucket);
- Line 236: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton_code));
- Line 309: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "entries={}, geo_index_bytes_allocated={}",
- Line 374: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (cfg.total_bounds.minx == 0.0 && cfg.total_bounds.maxx == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 464: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (entry.sidecar.z_min != 0.0 || entry.sidecar.z_max != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 493: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Purge all spatial keys (legacy Morton buckets + per-PK keys) so that
  Confidence: band=high; score=0.8
- Line 523: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 542: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: "geo_index_bytes_allocated={}",
- Line 587: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Save back to bucket (for backward compatibility)
  Confidence: band=high; score=0.8
- Line 605: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 663: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (sidecar.z_min != 0.0 || sidecar.z_max != 0.0) {
  Confidence: band=very_high; score=0.9
- Line 705: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward compatibility: update legacy Morton bucket key as well.
  Confidence: band=high; score=0.8
- Line 750: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // legacy bucket key for backward compatibility and fallback query paths.
  Confidence: band=high; score=0.8
- Line 1008: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: result.z_min = entry.sidecar.z_min != 0.0
  Confidence: band=very_high; score=0.9
- Line 1010: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: result.z_max = entry.sidecar.z_max != 0.0
  Confidence: band=very_high; score=0.9
- Line 1088: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = cache.find(pk);
- Line 1096: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ── Fallback: tiny-bbox approach (legacy Morton-bucket data, no R-tree) ─
  Confidence: band=high; score=0.8
- Line 1276: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // legacy data that pre-dates Z storage and is safe because the caller receives
  Confidence: band=high; score=0.8
- Line 1307: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton));
  Confidence: band=very_high; score=0.9
- Line 1307: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(morton));
- Line 246: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: g.rings.push_back({
- Line 298: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 334: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 344: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 439: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entry);
  Confidence: band=high; score=0.74
- Line 440: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entry);
- Line 442: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 468: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j.push_back(item);
  Confidence: band=high; score=0.74
- Line 469: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j.push_back(item);
- Line 585: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(new_entry);
- Line 740: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bulk_entries.emplace_back(cached_pk, mbrToGeometryInfo(cached_mbr));
  Confidence: band=high; score=0.74
- Line 807: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: bulk_entries.emplace_back(cached_pk, mbrToGeometryInfo(cached_mbr));
  Confidence: band=high; score=0.74
- Line 925: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { exact_match = true; }
- Line 927: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { exact_match = true; }
- Line 962: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: kvs.emplace_back(std::string(k), std::string(v));
  Confidence: band=high; score=0.74
- Line 999: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { exact_match = true; }
- Line 1001: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) { exact_match = true; }
- Line 1050: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(cand);
  Confidence: band=high; score=0.74
- Line 1051: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(cand);
- Line 1090: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 1091: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(result));
- Line 1102: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(cand);
  Confidence: band=high; score=0.74
- Line 1103: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(cand);
- Line 1132: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(cand);
  Confidence: band=high; score=0.74
- Line 1133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(cand);
- Line 1262: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 1297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(cand));
  Confidence: band=high; score=0.74
- Line 1298: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(cand));
- Line 1313: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(cand));
- Line 1330: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/index/graph_analytics.cpp
Total findings: 47

- Line 78: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: dr.out_degree = static_cast<int>(out_it->second.size());
- Line 83: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: dr.in_degree = static_cast<int>(in_it->second.size());
- Line 0: severity=HIGH; category=uncategorized
  Context: ['                }', '', '                total_path.length = root_length + spur_path.length;', '                total_path.hop_count = static_cast<int>(total_path.edges.size());', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Context: ['', '                // Update if we found a better path', '                if (!best_dist.count(neighbor) || new_dist < best_dist[neighbor]) {', '                    best_dist[neighbor] = new_dist;', '']
  Confidence: band=high; score=0.81
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 107: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: return {Status::Error("Damping factor must be in [0, 1]"), {}};
- Line 143: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto out_it = topo.outgoing.find(pk);
- Line 143: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto out_it = topo.outgoing.find(pk);
- Line 143: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto out_it = topo.outgoing.find(pk);
- Line 143: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto out_it = topo.outgoing.find(pk);
- Line 144: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto out_it = topo.outgoing.find(pk);
  Confidence: band=very_high; score=0.9
- Line 165: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = new_ranks.find(neighbor);
  Confidence: band=very_high; score=0.9
- Line 242: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto out_it = topo.outgoing.find(v);
- Line 387: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (m == 0.0) m = 1.0;  // Avoid division by zero
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 59: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, GraphAnalytics::DegreeResult>>
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, DegreeResult> results;
  Confidence: band=high; score=0.74
- Line 94: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, double>>
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> ranks;
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> new_ranks;
  Confidence: band=high; score=0.74
- Line 144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out_degrees.push_back((out_it != topo.outgoing.end()) ? out_it->second.size() : 0);
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out_degrees.push_back((out_it != topo.outgoing.end()) ? out_it->second.size() : 0);
- Line 197: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, double>>
  Confidence: band=high; score=0.74
- Line 211: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> betweenness;
  Confidence: band=high; score=0.74
- Line 220: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, std::vector<std::string>> predecessors; // predecessors on shortest paths
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> distance;
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> sigma; // number of shortest paths
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> delta; // dependency
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back(v);
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back(v);
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: predecessors[w].push_back(v);
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: predecessors[w].push_back(v);
- Line 287: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::pair<GraphAnalytics::Status, std::map<std::string, double>>
  Confidence: band=high; score=0.74
- Line 300: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, double> closeness;
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=performance; pattern=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Context: std::map<std::string, int> distance;
  Confidence: band=high; score=0.74
- Line 565: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: key += v + "|";
  Confidence: band=high; score=0.74
- Line 657: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: new_path_vertices.push_back(neighbor);
  Confidence: band=high; score=0.74
- Line 729: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: total_path.vertices.push_back(spur_path.vertices[i]);
  Confidence: band=high; score=0.74
- Line 730: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: total_path.vertices.push_back(spur_path.vertices[i]);
- Line 732: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: total_path.edges.push_back(edge);
  Confidence: band=high; score=0.74
- Line 733: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: total_path.edges.push_back(edge);

### src/index/inverted_index.cpp
Total findings: 44

- Line 128: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: meta = nullptr;
  Context: return Status::Error("InvertedIndex::drop: failed to delete meta key");
- Line 488: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (field.find(phraseNorm) != std::string::npos)
- Line 489: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (field.find(phraseNorm) != std::string::npos)
  Confidence: band=very_high; score=0.9
- Line 559: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = pkScores.find(pk);
  Confidence: band=very_high; score=0.9
- Line 128: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: return Status::Error("InvertedIndex::drop: failed to delete meta key");
- Line 152: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (w.is_string()) cfg.stopwords.push_back(w.get<std::string>());
  Confidence: band=high; score=0.74
- Line 153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (w.is_string()) cfg.stopwords.push_back(w.get<std::string>());
- Line 156: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(cur));
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(cur));
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cur.push_back(static_cast<char>(c));
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(cur));
- Line 239: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 271: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, uint32_t> tf;
  Confidence: band=medium; score=0.66
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: revTokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: revTokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: revTokens.push_back(tok);
- Line 331: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> postings;
  Confidence: band=medium; score=0.66
- Line 334: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> pks;
  Confidence: band=medium; score=0.66
- Line 341: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: postings.push_back(std::move(pks));
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: postings.push_back(std::move(pks));
- Line 346: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> intersection = postings[0];
  Confidence: band=medium; score=0.66
- Line 348: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tmp;
  Confidence: band=medium; score=0.66
- Line 356: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> universe;
  Confidence: band=medium; score=0.66
- Line 362: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> docLen;
  Confidence: band=medium; score=0.66
- Line 369: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { dl = static_cast<double>(std::stoull(s)); } catch (...) {}
- Line 381: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dfs.push_back(static_cast<double>(ps.size()));
- Line 400: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { tf = static_cast<double>(std::stoul(s)); } catch (...) {}
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.push_back({std::string(pk), score});
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.push_back({std::string(pk), score});
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scored.push_back({std::string(pk), score});
- Line 446: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::vector<std::unordered_set<std::string>> postings;
  Confidence: band=medium; score=0.66
- Line 448: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> pks;
  Confidence: band=medium; score=0.66
- Line 455: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: postings.push_back(std::move(pks));
  Confidence: band=high; score=0.74
- Line 456: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: postings.push_back(std::move(pks));
- Line 459: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> candidates = postings[0];
  Confidence: band=medium; score=0.66
- Line 461: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tmp;
  Confidence: band=medium; score=0.66
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({pk, 1.0});
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({pk, 1.0});
- Line 491: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 539: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> pkScores;
  Confidence: band=medium; score=0.66
- Line 571: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({pk, score});
  Confidence: band=high; score=0.74
- Line 572: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({pk, score});
- Line 395: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double idf = std::log((N - df + 0.5) / (df + 0.5) + 1.0);
  Confidence: band=medium; score=0.6

### src/index/gnn_embeddings.cpp
Total findings: 43

- Line 212: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity neighbor = BaseEntity::deserialize(neighbor_ids[i], *blob);
  Confidence: band=very_high; score=0.99
- Line 415: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity node = BaseEntity::deserialize(std::string(node_pk), *blob);
  Confidence: band=very_high; score=0.99
- Line 505: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity edge = BaseEntity::deserialize(std::string(edge_id), *blob);
  Confidence: band=very_high; score=0.99
- Line 569: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity embEntity = BaseEntity::deserialize(keyStr, std::vector<uint8_t>(val.begin(), val.end()));
  Confidence: band=very_high; score=0.99
- Line 575: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_dim = static_cast<int>(embOpt->size());
- Line 575: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_dim = static_cast<int>(embOpt->size());
- Line 634: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity embEntity = BaseEntity::deserialize(embKey, *blob);
  Confidence: band=very_high; score=0.99
- Line 668: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: BaseEntity embEntity = BaseEntity::deserialize(embKey, *blob);
  Confidence: band=very_high; score=0.99
- Line 139: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (visited.find(neighbor) == visited.end()) {
- Line 139: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (visited.find(neighbor) == visited.end()) {
- Line 140: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (visited.find(neighbor) == visited.end()) {
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t j = 0; j < std::min(nf.size(), embedding.size()); ++j) {
- Line 45: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: features.push_back(static_cast<float>(*intVal));
  Confidence: band=high; score=0.74
- Line 46: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(static_cast<float>(*intVal));
- Line 52: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(static_cast<float>(*doubleVal));
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: features.push_back(static_cast<float>(hash % 10000) / 10000.0f);
- Line 94: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(part);
- Line 107: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 4) result.entity_id += ":";
- Line 124: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited;
  Confidence: band=medium; score=0.66
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current_level.push_back(std::string(node_pk));
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_level.push_back(neighbor);
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: next_level.push_back(neighbor);
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_level.push_back(neighbor);
- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_neighbors.push_back(neighbor);
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: next_level.push_back(neighbor);
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_neighbors.push_back(neighbor);
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: neighbor_features_list.push_back(neighbor_features);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: neighbor_features_list.push_back(neighbor_features);
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: raw_similarities.push_back(similarity);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: raw_similarities.push_back(similarity);
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: raw_similarities.push_back(similarity);
- Line 289: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: attention_weights.push_back(weight);
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: attention_weights.push_back(weight);
- Line 477: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: edge_ids.push_back(edge.edgeId);
  Confidence: band=high; score=0.74
- Line 478: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: edge_ids.push_back(edge.edgeId);
- Line 573: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: node_embeddings.push_back(*embOpt);
- Line 728: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similar.push_back(simRes);
  Confidence: band=high; score=0.74
- Line 729: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: similar.push_back(simRes);
- Line 769: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similar.push_back(simRes);
  Confidence: band=high; score=0.74
- Line 770: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: similar.push_back(simRes);
- Line 800: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74
- Line 801: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(name);
- Line 883: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(part);

### src/index/multi_gpu_vector_index.cpp
Total findings: 39

- Line 317: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = vectorToGPU.find(id);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 82: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "MultiGPUVectorIndex: Initializing with " << config.deviceIds.size()
  Confidence: band=very_high; score=0.9
- Line 114: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Successfully initialized " << activeDeviceIds.size() << " GPUs\n";
  Confidence: band=very_high; score=0.9
- Line 115: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Communication backend: " << getCommBackendName() << "\n";
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "NCCL backend initialized (version: "
  Confidence: band=very_high; score=0.9
- Line 179: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "RCCL backend initialized (version: "
  Confidence: band=very_high; score=0.9
- Line 189: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Using CPU-based communication (no GPU collectives)\n";
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  GPU " << deviceId << " initialized successfully\n";
  Confidence: band=very_high; score=0.9
- Line 346: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t gpuIdx = 0; gpuIdx < gpuIndices.size(); ++gpuIdx) {
  Confidence: band=very_high; score=0.9
- Line 404: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t gpuIdx = 0; gpuIdx < numGPUs; ++gpuIdx) {
  Confidence: band=very_high; score=0.9
- Line 587: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "MultiGPUVectorIndex: Rebalancing vectors across "
  Confidence: band=very_high; score=0.9
- Line 595: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Partition " << i << " (Device " << activeDeviceIds[i]
  Confidence: band=very_high; score=0.9
- Line 610: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Load imbalance: " << std::fixed << std::setprecision(1)
  Confidence: band=very_high; score=0.9
- Line 614: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Load is already well balanced, no action needed\n";
  Confidence: band=very_high; score=0.9
- Line 616: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  NOTE: Full rebalancing with data migration will be "
  Confidence: band=very_high; score=0.9
- Line 621: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Rebalancing check complete\n";
  Confidence: band=very_high; score=0.9
- Line 95: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "Warning: Failed to initialize GPU " << deviceId
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: failedDeviceIds.push_back(deviceId);
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: failedDeviceIds.push_back(deviceId);
- Line 100: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "Error: Failed to initialize GPU " << deviceId << "\n";
- Line 271: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stats = gpuIndices[i]->getStatistics();
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto gpuResults = gpuIndices[gpuIdx]->search(query, k);
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allResults.push_back(mgpuResult);
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allResults.push_back(mgpuResult);
  Confidence: band=high; score=0.74
- Line 365: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: allResults.push_back(mgpuResult);
- Line 405: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async, [this, gpuIdx, &queries, k]() {
- Line 407: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto res = gpuIndices[gpuIdx]->searchBatch(queries, k);
  Confidence: band=high; score=0.74
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: perGpuResults.push_back(f.get());
  Confidence: band=high; score=0.74
- Line 426: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: perGpuResults.push_back(f.get());
- Line 441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allResults.push_back(mgpuResult);
  Confidence: band=high; score=0.74
- Line 441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allResults.push_back(mgpuResult);
  Confidence: band=high; score=0.74
- Line 441: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: allResults.push_back(mgpuResult);
  Confidence: band=high; score=0.74
- Line 442: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: allResults.push_back(mgpuResult);
- Line 511: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto gpuStats = gpuIndices[i]->getStatistics();
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vectorsPerGPU.push_back(stats.numVectors);
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stats = gpuIndices[i]->getStatistics();
  Confidence: band=high; score=0.74
- Line 594: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vectorsPerGPU.push_back(stats.numVectors);
- Line 595: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "  Partition " << i << " (Device " << activeDeviceIds[i]

### src/index/distributed_vector_index.cpp
Total findings: 38

- Line 123: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: const std::string token = "shard:" + std::to_string(s) + ":vn:" + std::to_string(v);
- Line 184: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool DistributedVectorIndex::insert(const std::string& pk,
  Confidence: band=very_high; score=0.99
- Line 193: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator existing may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto existing = pk_to_shard_.find(pk);
- Line 194: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator global_existing may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto global_existing = pk_to_global_id_.find(pk);
- Line 206: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: global_id = global_existing->second;
- Line 234: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: bool DistributedVectorIndex::insert(const std::string& pk,
  Confidence: band=very_high; score=0.99
- Line 236: severity=CRITICAL; category=distributed_consistency; pattern=missing_consensus
  Description: Write without consensus/replication acknowledgment
  Context: return insert(pk, vec.data(), vec.size());
  Confidence: band=very_high; score=0.99
- Line 242: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = pk_to_shard_.find(pk);
- Line 266: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: std::vector<AnnSearchResult> merged;
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=distributed_consistency; pattern=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Context: merged.push_back(r);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 63: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DistributedVectorIndex: num_shards must be > 0");
- Line 81: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DistributedVectorIndex: num_shards must be > 0");
- Line 84: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 168: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = pos + 1; i < key.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 266: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::vector<AnnSearchResult> merged;
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = local_to_global_id_[s].find(r.id);
- Line 274: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = local_to_global_id_[s].find(r.id);
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.push_back(r);
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: // Merge: sort by distance (ascending) and keep top-k.
  Confidence: band=very_high; score=0.9
- Line 285: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: std::sort(merged.begin(), merged.end(),
  Confidence: band=very_high; score=0.9
- Line 290: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: if (static_cast<int>(merged.size()) > k) {
  Confidence: band=very_high; score=0.9
- Line 291: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: merged.resize(static_cast<size_t>(k));
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=distributed_consistency; pattern=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Context: return merged;
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < shards_.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shards_.push_back(std::make_unique<ScaNN>());
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shards_.push_back(std::make_unique<ScaNN>());
- Line 175: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 227: severity=MEDIUM; category=distributed_consistency; pattern=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Context: // Rollback: remove the stale routing entry so the key is not
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto partial = shards_[s]->search(query, dim, k);
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto it = local_to_global_id_[s].find(r.id);
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(r);
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(r);
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(r);
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.push_back({i, alive_ids_[i].size()});
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.push_back({i, alive_ids_[i].size()});

### src/index/multi_vector_search.cpp
Total findings: 38

- Line 390: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator kw_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto kw_it = keyword_scores.find(doc_id);
- Line 98: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // 1. Validate inputs
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = std::find_if(results.begin(), results.end(),
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find_if(results.begin(), results.end(),
- Line 194: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find_if(results.begin(), results.end(),
- Line 375: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto vec_it = std::find_if(vector_results.begin(), vector_results.end(),
- Line 557: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: auto it = std::find(relevant_docs.begin(), relevant_docs.end(), res.id);
  Confidence: band=very_high; score=0.9
- Line 557: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find(relevant_docs.begin(), relevant_docs.end(), res.id);
- Line 557: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find(relevant_docs.begin(), relevant_docs.end(), res.id);
- Line 40: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized.push_back((score - min_score) / range);
  Confidence: band=high; score=0.74
- Line 41: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: normalized.push_back((score - min_score) / range);
- Line 161: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "MultiVectorSearch::search - vector search failed: " + status.message);
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: individual_results.push_back(std::move(results));
- Line 167: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_docs;
  Confidence: band=medium; score=0.66
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(score);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(score);
- Line 206: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ranks.push_back(rank);
- Line 208: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(0.0f);  // Not found
- Line 209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ranks.push_back(std::numeric_limits<int>::max());  // Worst rank
- Line 250: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fused_results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fused_results.push_back(std::move(result));
- Line 300: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: multi_query.vectors.push_back(query_vector);
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: multi_query.vectors.push_back(query_vector);
- Line 340: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, float>& keyword_scores,
  Confidence: band=medium; score=0.66
- Line 349: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "MultiVectorSearch::hybridSearch - vector search failed: " + status.message);
- Line 353: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_docs;
  Confidence: band=medium; score=0.66
- Line 382: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(score);
- Line 383: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ranks.push_back(static_cast<int>(std::distance(vector_results.begin(), vec_it)));
- Line 394: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ranks.push_back(0); // Assign best rank
- Line 396: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(0.0f);
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ranks.push_back(std::numeric_limits<int>::max());
- Line 448: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fused_results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 449: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fused_results.push_back(std::move(result));
- Line 503: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(result.value()));
  Confidence: band=high; score=0.74
- Line 504: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(result.value()));
- Line 541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current_weights.push_back(remaining);
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current_weights.push_back(w);
  Confidence: band=high; score=0.74
- Line 589: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current_weights.push_back(w);

### src/index/index_compression.cpp
Total findings: 33

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 199: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Need to rebuild existing suffixes with new (shorter) prefix
- Line 220: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Start a new block
- Line 99: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: BloomFilter::clear()
  Context: void BloomFilter::clear() {
  Confidence: band=medium; score=0.56
- Line 113: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> freq;
  Confidence: band=medium; score=0.66
- Line 144: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: id_to_string_.push_back(std::move(candidates[i].second));
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: id_to_string_.push_back(std::move(candidates[i].second));
- Line 167: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(prefix + sfx);
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(prefix + sfx);
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current.suffixes.push_back(sorted_keys[i].substr(current.prefix.size()));
  Confidence: band=high; score=0.74
- Line 207: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current.suffixes.push_back(sorted_keys[i].substr(current.prefix.size()));
  Confidence: band=high; score=0.74
- Line 208: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current.suffixes.push_back(sorted_keys[i].substr(current.prefix.size()));
- Line 215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: single.suffixes.push_back(current.prefix + current.suffixes[0]);
- Line 216: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: blocks.push_back(std::move(single));
- Line 218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: blocks.push_back(std::move(current));
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: block.deltas.push_back(sorted_values[i] - sorted_values[i - 1]);
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(block.base);
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(prev);
  Confidence: band=high; score=0.74
- Line 282: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(prev);
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: block.runs.push_back({values[0], 1});
- Line 303: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: block.runs.push_back({values[i], 1});
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: block.runs.push_back({values[i], 1});
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(run.value);
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(run.value);
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(run.value);
- Line 422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: b.suffixes.push_back(k);
  Confidence: band=high; score=0.74
- Line 423: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: b.suffixes.push_back(k);
- Line 424: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: trivial.push_back(std::move(b));
- Line 452: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: trivial.runs.push_back({v, 1});
  Confidence: band=high; score=0.74
- Line 453: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: trivial.runs.push_back({v, 1});
- Line 60: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double ln2   = std::log(2.0);
  Confidence: band=medium; score=0.6

### src/index/gpu_vector_index_vulkan.cpp
Total findings: 32

- Line 476: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: pipeline->wait();
- Line 641: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: pipeline->wait();
- Line 82: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(initializeFnMutex());
- Line 86: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(uploadFnMutex());
- Line 145: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: a.first == b.first &&
  Confidence: band=very_high; score=0.9
- Line 146: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: a.middle == b.middle &&
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: a.last == b.last;
  Confidence: band=very_high; score=0.9
- Line 258: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "VulkanVectorIndexBackend: Using GPU: " << props.deviceName << "\n";
  Confidence: band=very_high; score=0.9
- Line 259: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "VulkanVectorIndexBackend: Vulkan API Version: "
  Confidence: band=very_high; score=0.9
- Line 350: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vector_buffer_->upload(flatData.data(), totalSize);
- Line 480: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: distance_buffer_->download(distanceScratch_.data(), distanceSize);
- Line 644: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: distance_buffer_->download(allDistancesScratch_.data(), distanceBufferSize);
- Line 706: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (avg_query_time_ms_ == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 754: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "VulkanVectorIndexBackend: Using shader directory: " << shaderDir << "\n";
  Confidence: band=very_high; score=0.9
- Line 786: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "VulkanVectorIndexBackend: All compute pipelines created successfully\n";
  Confidence: band=very_high; score=0.9
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: top.emplace_back(distances[i], i);
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "VulkanVectorIndexBackend: Vector dimension mismatch\n";
- Line 515: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({"", distance});
- Line 532: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: converted.push_back({"", distance});
  Confidence: band=high; score=0.74
- Line 533: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: converted.push_back({"", distance});
- Line 535: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(converted));
- Line 557: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(searchIndices(query, k));
  Confidence: band=high; score=0.74
- Line 558: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(searchIndices(query, k));
- Line 572: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "VulkanVectorIndexBackend: Query dimension mismatch in batch\n";
- Line 651: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(
- Line 665: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(searchIndices(query, k));
  Confidence: band=high; score=0.74
- Line 666: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(searchIndices(query, k));
- Line 749: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "  - " << path << "\n";
- Line 942: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 959: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return fn(vectors); } catch (...) { return false; }
- Line 977: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return fn(query, k); } catch (...) { return {}; }
- Line 990: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: try { return fn(queries, k); } catch (...) { return {}; }

### src/index/advanced_vector_index.cpp
Total findings: 31

- Line 41: severity=CRITICAL; category=missing_dtor
  Description: Class Index allocates resources but has no destructor
  Remediation: Add explicit destructor: ~Index() { /* cleanup */ }
  Context: class/struct Index
- Line 42: severity=CRITICAL; category=missing_dtor
  Description: Class IndexIVFPQ allocates resources but has no destructor
  Remediation: Add explicit destructor: ~IndexIVFPQ() { /* cleanup */ }
  Context: class/struct IndexIVFPQ
- Line 43: severity=CRITICAL; category=missing_dtor
  Description: Class IndexIVFFlat allocates resources but has no destructor
  Remediation: Add explicit destructor: ~IndexIVFFlat() { /* cleanup */ }
  Context: class/struct IndexIVFFlat
- Line 77: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* quantizer = new faiss::IndexFlat(dimension_, faiss::METRIC_L2);
- Line 78: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* ivf_pq = new faiss::IndexIVFPQ(
- Line 87: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ivf_pq->nprobe = config_.nprobe;
- Line 93: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ivf_pq->polysemous_ht = config_.polysemous_ht;
- Line 109: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* quantizer = new faiss::IndexFlat(dimension_, faiss::METRIC_L2);
- Line 110: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* ivf_flat = new faiss::IndexIVFFlat(
- Line 117: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: ivf_flat->nprobe = config_.nprobe;
- Line 125: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* hnsw = new faiss::IndexHNSWFlat(static_cast<int>(dimension_), 32);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 62: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::Index*>(index_);
- Line 86: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: the = nullptr;
  Context: ivf_pq->own_fields = true; // FAISS will delete the quantizer
- Line 116: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: the = nullptr;
  Context: ivf_flat->own_fields = true; // FAISS will delete the quantizer
- Line 327: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: idx->search(1, query, k, result.distances.data(), result.ids.data());
- Line 375: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: idx->search(num_queries, queries, k, all_distances.data(), all_ids.data());
- Line 500: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::Index*>(index_);
- Line 62: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::Index*>(index_);
- Line 86: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ivf_pq->own_fields = true; // FAISS will delete the quantizer
- Line 116: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: ivf_flat->own_fields = true; // FAISS will delete the quantizer
- Line 155: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 214: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 259: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 303: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 347: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 408: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 448: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 487: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 500: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::Index*>(index_);
- Line 526: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/index/ann_index.cpp
Total findings: 31

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 95: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float dist = l2sq(data + i * d, centroids[c].data(), d);
- Line 108: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: new_cents[c][j] += data[i * d + j];
- Line 119: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: centroids[c].assign(data + r * d, data + r * d + d);
- Line 128: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float dist = l2sq(data + i * d, centroids[c].data(), d);
- Line 173: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float dist = ScaNN::l2sq(sv, centroids[s][c].data(), sub_dim);
- Line 186: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const float* sc = centroids[s][code[s]].data();
- Line 377: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: sizeof(int64_t) * n);
- Line 403: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: sizeof(int64_t) * n);
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.emplace_back(data + chosen * d, data + chosen * d + d);
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.emplace_back(data + chosen * d, data + chosen * d + d);
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.emplace_back(data + chosen * d, data + chosen * d + d);
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cents.push_back(std::vector<float>(sub_dim, 0.f));
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cents.push_back(std::vector<float>(sub_dim, 0.f));
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cents.push_back(std::vector<float>(sub_dim, 0.f));
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: leaves_[c].ids.push_back(label);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: leaves_[c].ids.push_back(label);
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: leaves_[c].ids.push_back(label);
  Confidence: band=high; score=0.74
- Line 222: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: leaves_[c].ids.push_back(label);
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flat_ids_.push_back(id);
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: flat_ids_.push_back(id);
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: flat_ids_.push_back(id);
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: leaves_[best_leaf].ids.push_back(id);
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: leaves_[best_leaf].ids.push_back(id);
- Line 265: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: leaves_[best_leaf].codes.push_back(
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back({dist, &leaf, i});
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: candidates.push_back({dist, &leaf, i});
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back({dist, &leaf, i});
- Line 338: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back({c.leaf->ids[c.idx], exact});
  Confidence: band=high; score=0.74
- Line 339: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back({c.leaf->ids[c.idx], exact});

### src/index/gpu_memory_oversubscription.cpp
Total findings: 31

- Line 41: severity=CRITICAL; category=gpu_memory_safety; pattern=gpu_memory_leak
  Description: GPU memory allocated without RAII wrapper or guaranteed cleanup
  Context: // VRAM state.  On GPU builds vram_ptr is the cudaMallocManaged /
  Confidence: band=very_high; score=0.99
- Line 218: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lru_map.find(partition_id);
- Line 397: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lm_it = pImpl_->lru_map.find(partition_id);
- Line 398: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (lm_it != pImpl_->lru_map.end()) {
- Line 453: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto lm_it = pImpl_->lru_map.find(partition_id);
- Line 454: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (lm_it != pImpl_->lru_map.end()) {
- Line 41: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMalloc() without error checking
  Context: // VRAM state.  On GPU builds vram_ptr is the cudaMallocManaged /
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: .allocate(bytes, alloc_tag);
- Line 341: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [id, p] : pImpl_->partitions) {
  Confidence: band=very_high; score=0.9
- Line 434: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pImpl_->applyPrefetchLocked(partition_id);
- Line 472: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it == pImpl_->partitions.end()) return nullptr;
- Line 511: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pImpl_->partitions.find(pid);
- Line 511: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const size_t pid : pImpl_->lru_list) {
  Confidence: band=very_high; score=0.9
- Line 528: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pImpl_->partitions.find(pid);
- Line 528: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = pImpl_->partitions.find(pid);
- Line 528: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const size_t pid : pImpl_->insertion_order) {
  Confidence: band=very_high; score=0.9
- Line 556: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ++pImpl_->prefetch_requests;
- Line 559: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ++pImpl_->prefetch_hits;
- Line 580: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pImpl_->config.prefetch_strategy = strategy;
- Line 589: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return pImpl_->config.prefetch_strategy;
- Line 652: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: s.prefetch_requests   = pImpl_->prefetch_requests;
- Line 653: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: s.prefetch_hits       = pImpl_->prefetch_hits;
- Line 654: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: s.prefetch_hit_rate   = (pImpl_->prefetch_requests > 0)
- Line 655: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ? static_cast<double>(pImpl_->prefetch_hits) /
- Line 656: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: static_cast<double>(pImpl_->prefetch_requests)
- Line 120: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: themis::gpu::GPUUnifiedMemoryAllocator::GetInstance().free(p.vram_ptr);
- Line 176: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << p.id << " (" << (bytes / 1024) << " KiB required)\n";
- Line 513: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pid);
  Confidence: band=high; score=0.74
- Line 514: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pid);
- Line 530: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pid);
  Confidence: band=high; score=0.74
- Line 531: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pid);

### src/index/vector_auto_buffer.cpp
Total findings: 28

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 123: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: buffers_mutex_.lock();
- Line 246: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 309: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: status = vectorIndex_->addBatch(compressed_adds, config_.vector_field);
- Line 318: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: status = vectorIndex_->updateBatch(compressed_updates, config_.vector_field);
- Line 127: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& buffer = buffers_[buffer_key];
- Line 169: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& buffer = buffers_[buffer_key];
- Line 204: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& buffer = buffers_[buffer_key];
- Line 256: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [buffer_key, buffer] : buffers_) {
- Line 383: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);
- Line 593: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: quantised[dim] = abs_max; // scale metadata for downstream decoders
- Line 35: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: adds.push_back(op.entity);
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: adds.push_back(op.entity);
- Line 294: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: updates.push_back(op.entity);
- Line 297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: removes.push_back(op.pk);
- Line 305: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: VectorIndexManager::Status status;
- Line 486: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: training_vecs.push_back(*vec_opt);
  Confidence: band=high; score=0.74
- Line 487: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: training_vecs.push_back(*vec_opt);
- Line 532: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entity);
  Confidence: band=high; score=0.74
- Line 533: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entity);
- Line 541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(compressed));
- Line 560: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entity);
  Confidence: band=high; score=0.74
- Line 561: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entity);
- Line 575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(entity);
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(entity);
- Line 601: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(compressed));
  Confidence: band=high; score=0.74
- Line 602: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(compressed));

### src/index/product_quantizer.cpp
Total findings: 26

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 145: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: codebooks_[sq] = runKMeans(subvector_data);
- Line 178: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: faiss_pq_->compute_codes(vector.data(), codes.data(), 1);
- Line 224: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: faiss_pq_->decode(codes.data(), decoded.data(), 1);
- Line 271: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: faiss_pq_->compute_distance_table(query.data(), dis_table.data());
- Line 309: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: size_t compressed_size = config_.num_subquantizers * sizeof(uint8_t);
- Line 369: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: centroid[d] = centroid_data[i * subvector_dim_ + d];
- Line 406: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float dist = l2Distance(subvector_data[j], centroid);
- Line 414: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: centroids.push_back(subvector_data[weighted_dis(gen)]);
- Line 427: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: float dist = l2Distance(subvector_data[i], centroids[j]);
- Line 446: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: new_centroids[cluster][d] += subvector_data[i][d];
- Line 463: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: new_centroids[j] = subvector_data[dis(gen)];
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subvector_data.push_back(std::move(subvec));
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: subvector_data.push_back(std::move(subvec));
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: codes.push_back(code);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: codes.push_back(code);
- Line 370: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(std::move(centroid));
  Confidence: band=high; score=0.74
- Line 370: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(std::move(centroid));
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: centroids.push_back(std::move(centroid));
- Line 396: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: centroids.push_back(subvector_data[dis(gen)]);
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvector_data[weighted_dis(gen)]);
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvector_data[weighted_dis(gen)]);
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(subvector_data[weighted_dis(gen)]);
  Confidence: band=high; score=0.74
- Line 414: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: centroids.push_back(subvector_data[weighted_dis(gen)]);

### src/index/property_graph.cpp
Total findings: 21

- Line 49: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Backward-compatible fallback below (legacy comma-separated encoding).
  Confidence: band=high; score=0.8
- Line 52: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Legacy format fallback: comma-separated string.
  Confidence: band=high; score=0.8
- Line 77: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // current JSON-array serialization and the legacy comma-separated format.
  Confidence: band=high; score=0.8
- Line 1245: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Pre-allocate outside loop or use object pool pattern
  Context: // Compute new PageRank scores
- Line 42: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labels.push_back(std::move(label));
  Confidence: band=high; score=0.74
- Line 43: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: labels.push_back(std::move(label));
- Line 48: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 59: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: labels.push_back(std::move(label));
- Line 728: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: PropertyGraphManager::federatedQuery(const std::vector<FederationPattern>& patterns) const {
  Confidence: band=high; score=0.74
- Line 743: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.nodes.push_back({pk, {pattern.label_or_type}, pattern.graph_id});
  Confidence: band=high; score=0.74
- Line 743: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.nodes.push_back({pk, {pattern.label_or_type}, pattern.graph_id});
  Confidence: band=high; score=0.74
- Line 744: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.nodes.push_back({pk, {pattern.label_or_type}, pattern.graph_id});
- Line 879: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(current_node);
- Line 934: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(current_node);
- Line 949: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: neighbors.push_back(neighbor);
- Line 1036: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(current);
- Line 1210: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, int> outgoing_count;
  Confidence: band=medium; score=0.66
- Line 1211: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> incoming_nodes;
  Confidence: band=medium; score=0.66
- Line 1226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: incoming_nodes[to_node].push_back(node);
  Confidence: band=high; score=0.74
- Line 1226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: incoming_nodes[to_node].push_back(node);
  Confidence: band=high; score=0.74
- Line 1227: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: incoming_nodes[to_node].push_back(node);

### src/index/learnable_rope.cpp
Total findings: 19

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 44: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 65: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 115: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range(
- Line 138: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("Cannot compute gradients: parameters are not trainable");
- Line 196: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("Cannot update parameters: not trainable");
- Line 200: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Gradient size mismatch");
- Line 337: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("Cannot train: parameters are not trainable");
- Line 341: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Cannot train on empty dataset");
- Line 410: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loss_history.push_back(epoch_loss);
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: loss_history.push_back(epoch_loss);
  Confidence: band=high; score=0.74
- Line 411: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: loss_history.push_back(epoch_loss);
- Line 464: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 466: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 507: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: loaded_theta.push_back(value);
- Line 508: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 521: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 530: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/index/rotary_embeddings_hip.cpp
Total findings: 17

- Line 28: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param embeddings    Input embeddings (batch_size * hidden_dim)
  Confidence: band=very_high; score=0.99
- Line 153: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: hipError_t err = hipMalloc(&gpu_resources_->d_theta_cache, cache_size);
- Line 167: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: gpu_resources_->d_theta_cache = nullptr;
- Line 171: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: gpu_resources_->theta_cache_size = theta_cache.size();
- Line 28: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param embeddings    Input embeddings (batch_size * hidden_dim)
  Confidence: band=very_high; score=0.9
- Line 94: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: size_t allocated_batch_size = 0;
- Line 167: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: gpu_resources_->d_theta_cache = nullptr;
- Line 190: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("GPU not available for batch rotation");
- Line 194: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Batch size mismatch");
- Line 204: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Embedding dimension mismatch");
- Line 211: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (gpu_resources_->allocated_batch_size < batch_size) {
- Line 220: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: gpu_resources_->allocated_batch_size = batch_size;
- Line 223: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: hipMemcpy(gpu_resources_->d_embeddings, flat_embeddings.data(),
- Line 225: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: hipMemcpy(gpu_resources_->d_positions, positions.data(),
- Line 244: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HIP kernel launch failed: " +
- Line 251: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: hipMemcpy(flat_output.data(), gpu_resources_->d_output,
- Line 272: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("GPU not available");

### src/index/rotary_embeddings.cpp
Total findings: 16

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 25: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid RotationConfig: hidden_dim must be positive and even");
- Line 46: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 53: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 73: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 83: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t pair_idx = 0; pair_idx < config_.num_rotation_pairs; ++pair_idx) {
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 151: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 172: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 215: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range(
- Line 258: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (norm_squared == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 34: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: theta_cache.push_back(theta);
  Confidence: band=high; score=0.74
- Line 35: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: theta_cache.push_back(theta);
- Line 160: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rotated_batch.push_back(rotate(embeddings[i], positions[i]));
  Confidence: band=high; score=0.74
- Line 161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rotated_batch.push_back(rotate(embeddings[i], positions[i]));

### src/index/index_manager.cpp
Total findings: 15

- Line 432: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto status = per_index_manager->init(name, static_cast<int>(dimension),
- Line 384: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: secondary_indices_[name_str] = raw_ptr;
- Line 446: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: vector_indices_[name_str] = raw_ptr;
- Line 489: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: ISecondaryIndex* ptr = it->second;
- Line 503: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: IVectorIndex* ptr = it->second;
- Line 517: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: IGraphIndex* ptr = it->second;
- Line 176: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(r.pk, r.distance);
  Confidence: band=high; score=0.74
- Line 363: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SecondaryIndexManager::Status status;
- Line 547: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: SecondaryIndexManager::Status drop_status;
- Line 600: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: indices.push_back(name);
  Confidence: band=high; score=0.74
- Line 601: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: indices.push_back(name);
- Line 790: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_drop.push_back(key);
  Confidence: band=high; score=0.74
- Line 791: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_drop.push_back(key);
- Line 820: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(key.substr(prefix.size()));
  Confidence: band=high; score=0.74
- Line 821: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(key.substr(prefix.size()));

### src/index/learned_quantizer.cpp
Total findings: 14

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 23: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Dimension must be positive");
- Line 27: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Bits per dimension must be between 1 and 8");
- Line 31: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Block size must be positive for per-block mode");
- Line 73: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_values.push_back(vec[d]);
- Line 231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: codes.push_back(static_cast<uint8_t>(bin));
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: codes.push_back(static_cast<uint8_t>(bin));
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: codes.push_back(static_cast<uint8_t>(bin));
  Confidence: band=high; score=0.74
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: codes.push_back(static_cast<uint8_t>(bin));
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: codes.push_back(static_cast<uint8_t>(bin));
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector.push_back(per_dim_centroids_[d][bin]);
  Confidence: band=high; score=0.74
- Line 285: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vector.push_back(per_dim_centroids_[d][bin]);
- Line 288: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vector.push_back(0.0f);

### src/index/residual_quantizer.cpp
Total findings: 12

- Line 21: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Dimension must be positive");
- Line 25: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 30: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Number of stages must be between 1 and 10");
- Line 72: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: " training failed: " + status.message);
- Line 76: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stage_quantizers_.push_back(std::move(pq));
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: residuals.push_back(std::move(residual));
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: residuals.push_back(std::move(residual));
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: residuals.push_back(std::move(residual));
- Line 137: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stage_codes = stage_quantizers_[stage]->encode(residual);
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto approx = stage_quantizers_[stage]->decode(stage_codes);
  Confidence: band=high; score=0.74
- Line 179: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stage_approx = stage_quantizers_[stage]->decode(stage_codes);
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto stage_approx = stage_quantizers_[stage]->decode(stage_codes);
  Confidence: band=high; score=0.74

### src/index/tiered_index_manager.cpp
Total findings: 12

- Line 104: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = registry_.find(name);
- Line 113: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [k, _] : registry_) names.push_back(k);
  Confidence: band=very_high; score=0.9
- Line 121: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [k, v] : registry_) {
  Confidence: band=very_high; score=0.9
- Line 286: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: live_path = it->second.data_path;
- Line 113: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& [k, _] : registry_) names.push_back(k);
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.tier == tier) names.push_back(k);
  Confidence: band=high; score=0.74
- Line 121: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (v.tier == tier) names.push_back(k);
  Confidence: band=high; score=0.74
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (v.tier == tier) names.push_back(k);
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(doMigrate(name, Tier::HOT, Tier::WARM));
  Confidence: band=high; score=0.74
- Line 213: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(doMigrate(name, Tier::HOT, Tier::WARM));
  Confidence: band=high; score=0.74
- Line 214: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(doMigrate(name, Tier::HOT, Tier::WARM));
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(doMigrate(name, Tier::WARM, Tier::COLD));

### src/index/lora_rope.cpp
Total findings: 10

- Line 158: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = adapters_.find(name);
- Line 196: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto adapter_opt = adapter_registry_->getAdapter(adapter_name);
- Line 96: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < rank; ++i) {
  Confidence: band=very_high; score=0.9
- Line 148: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [name, _] : adapters_) {
  Confidence: band=very_high; score=0.9
- Line 269: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Embeddings and positions size mismatch");
- Line 327: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Weight sum must be positive");
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: names.push_back(name);
  Confidence: band=high; score=0.74
- Line 149: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(name);
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(rotateWithAdapter(embeddings[i], positions[i], adapter_name));
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(rotateWithAdapter(embeddings[i], positions[i], adapter_name));

### src/index/adaptive_index.cpp
Total findings: 9

- Line 536: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_aware_stats = analyzer_->analyzeCacheAware(stats);
- Line 144: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("SelectivityAnalyzer: db cannot be null");
- Line 161: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_opts));
- Line 168: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
- Line 340: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("IndexSuggestionEngine: tracker cannot be null");
- Line 343: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("IndexSuggestionEngine: analyzer cannot be null");
- Line 619: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AdaptiveIndexManager: db cannot be null");
- Line 73: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(pattern);
  Confidence: band=high; score=0.74
- Line 74: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(pattern);

### src/index/approximate_radius_search.cpp
Total findings: 9

- Line 47: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs
  Confidence: band=very_high; score=0.9
- Line 81: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [status, results] = vector_manager_.searchKnnRadius(query_vector, config.radius, max_results, n
- Line 260: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Validate inputs
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto [status, sample_results] = vector_manager_.searchKnn(query_vector, sample_size, nullptr);
- Line 85: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: "Radius search failed: " + status.message);
- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: search_result.results.push_back(std::move(rr));
  Confidence: band=high; score=0.74
- Line 97: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: search_result.results.push_back(std::move(rr));
- Line 162: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_results.push_back(std::move(result.value()));
  Confidence: band=high; score=0.74
- Line 163: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_results.push_back(std::move(result.value()));

### src/index/graph_auto_buffer.cpp
Total findings: 9

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 103: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: buffers_mutex_.lock();
- Line 159: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: buffers_mutex_.lock();
- Line 213: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: lock.lock();
- Line 108: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& buffer = buffers_[gid];
- Line 164: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto& buffer = buffers_[gid];
- Line 223: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (auto& [graph_id, buffer] : buffers_) {
- Line 327: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(flush_mutex_);
- Line 25: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/index/workload_replay.cpp
Total findings: 8

- Line 83: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 88: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 93: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(mutex_);
- Line 82: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WorkloadCapture::recordQuery() {
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back(e.toJSON());
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back(e.toJSON());
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: capture.events_.push_back(WorkloadEvent::fromJSON(ej));
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: capture.events_.push_back(WorkloadEvent::fromJSON(ej));

### src/index/hnsw_parameter_tuner.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 431: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void WorkloadClassifier::recordQuery(size_t k) {
  Confidence: band=high; score=0.74

### src/index/hnsw_layer_optimizer.cpp
Total findings: 4

- Line 84: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [layer, perf] : entry_layer_performance) {
  Confidence: band=very_high; score=0.9
- Line 155: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lock(stats_mutex_);
- Line 72: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, std::pair<double, int>> entry_layer_performance;  // layer -> (total_time, count)
  Confidence: band=medium; score=0.66
- Line 107: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, std::pair<double, int>> ef_performance;  // ef -> (total_time, count)
  Confidence: band=medium; score=0.66

### src/index/edge_types.cpp
Total findings: 3

- Line 339: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = category_index_.find(category);
- Line 399: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(name);
  Confidence: band=high; score=0.74
- Line 400: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(name);

### src/index/rotary_embeddings_gpu_cpu.cpp
Total findings: 2

- Line 60: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 73: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(

### src/index/binary_quantizer.cpp
Total findings: 1

- Line 30: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Dimension must be positive");

### src/index/hnsw_production_defaults.cpp
Total findings: 1

- Line 102: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: params.ml = 1.0 / std::log(static_cast<double>(params.M));
  Confidence: band=medium; score=0.6

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
