# index — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **index** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 7712
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

**Batch 3 Wave Correlation (2026-08-14):**
- **Wave B Gaps** (~800 IMPL gaps): GPU vector index CUDA backend (L2/cosine/inner-product kernels), HIP backend, buffer lifecycle RAII hardening, concurrency fixes
- **Wave B DOC Gaps** (~300): GPU backend documentation, cost model documentation, hybrid retrieval Phase B runbook
- **Other IMPL Gaps** (~600): O(N²) complexity reductions, lock-contention fixes, GPU-memory-leak elimination, null-dereference fixes
- **Other DOC Gaps** (~4,100): Inline comments (braces_imbalance_midfile), algorithm documentation, integration notes

**Hybrid Retrieval Phase Implementation Status (Batch 3 verified 2026-08-14):**
- [x] Phase A: AnnFrontdoor ready with CPU fallback, all six artifact kinds registered, observability wired
- [~] Phase B: Buffer lifecycle RAII + concurrency hardening in progress; ThreadSanitizer validation pending
- [~] Phase B: GPU ANN validation pending (blocked by gpu module gaps)
- [ ] Phase C: GPU ANN full integration (Wave B target Q4 2026)

### By Severity

- **CRITICAL**: 29
- **HIGH**: 3057
- **MEDIUM**: 4623
- **LOW**: 3

### By Type

- allocation_loop: 4
- arithmetic_overflow: 5
- blocking_no_timeout: 2
- braces_imbalance: 13
- braces_imbalance_midfile: 2662
- circular_lock_ordering: 11
- copy_overhead: 20
- critical_function_noexcept: 2
- db_connection_leak: 34
- deadlock_risk: 2
- delete_no_nullptr: 7
- delete_without_nullptr: 9
- duplicate_qualified_signature: 1
- exception_in_destructor: 2
- expensive_copy: 1
- generic_catch: 26
- gpu_memory_leak: 5
- hardcoded_path: 1
- iterator_invalidation: 12
- legacy_or_compat_path: 25
- lock_contention: 4
- manual_cleanup: 14
- memory_order: 1
- missing_adr_reference: 1
- missing_noexcept_on_move: 7
- missing_volatile: 1
- module_doc_linkset_drift: 2
- no_timeout: 2
- null_dereference: 1
- o_n_squared: 27
- pointer_arithmetic_unbounded: 7
- range_temporary: 10
- repeated_search: 3
- resource_leaked_in_exception: 2
- scope_mismatch: 4578
- silent_error_swallow: 1
- size_assumption: 13
- smart_ptr_misuse: 1
- stale_doc_section_reference: 3
- string_concat_loop: 15
- todo_as_productionlogic: 79
- uncaught_exception: 26
- unchecked_array_index: 6
- unchecked_cuda_call: 26
- unchecked_result: 17
- uninitialized_access: 9
- uninitialized_array: 1
- uninitialized_variable: 5
- use_after_free_gpu: 1
- user_controlled_size: 5

## Top 20 Gaps

- [braces_imbalance] cuda_hnsw_graph_traversal.cpp:1 (CRITICAL)
- [braces_imbalance] graph_index.cpp:1 (CRITICAL)
- [braces_imbalance] hnsw_production_defaults.cpp:1 (CRITICAL)
- [braces_imbalance] property_graph.cpp:1 (CRITICAL)
- [braces_imbalance] secondary_index.cpp:1 (CRITICAL)
- [braces_imbalance] spatial_index.cpp:1 (CRITICAL)
- [exception_in_destructor] graph_auto_buffer.cpp:52 (CRITICAL)
- [gpu_memory_leak] gpu_memory_oversubscription.cpp:53 (CRITICAL)
- [exception_in_destructor] vector_auto_buffer.cpp:66 (CRITICAL)
- [iterator_invalidation] vector_index.cpp:80 (CRITICAL)
- [iterator_invalidation] multi_vector_search.cpp:224 (CRITICAL)
- [iterator_invalidation] gpu_memory_oversubscription.cpp:230 (CRITICAL)
- [iterator_invalidation] graph_index.cpp:244 (CRITICAL)
- [iterator_invalidation] graph_index.cpp:247 (CRITICAL)
- [iterator_invalidation] graph_index.cpp:248 (CRITICAL)
- [gpu_memory_leak] cuda_hnsw_graph_traversal.cpp:362 (CRITICAL)
- [iterator_invalidation] edge_types.cpp:364 (CRITICAL)
- [gpu_memory_leak] cuda_hnsw_graph_traversal.cpp:370 (CRITICAL)
- [gpu_memory_leak] cuda_hnsw_graph_traversal.cpp:381 (CRITICAL)
- [iterator_invalidation] multi_vector_search.cpp:406 (CRITICAL)

... and 7692 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
