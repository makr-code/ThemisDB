# query — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **query** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 4614
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

**Batch 3 Wave Correlation (2026-08-14):**
- **Wave A Gaps** (~200 IMPL gaps): Query planning determinism, timeout enforcement, cancellation semantics, federated execution error handling
- **Wave A DOC Gaps** (~150): Thread-safety model for optimizer, query cancellation flow documentation, failure-mode runbook
- **Wave B Gaps** (~300 IMPL gaps): Distributed execution baselines, ANN+graph hybrid planner, parallel optimization, benchmark gates
- **Wave B DOC Gaps** (~200): Cost model documentation, planner decision logic, performance tuning guide
- **Other Gaps** (~3,600): Inline comments, algorithm notes, null-pointer checks, resource-leak fixes

**Phase Implementation Status (Batch 3 verified 2026-08-14):**
- [x] Phase 1-6: Complete (parser, optimizer, executor, federation, caching, documentation)
- [x] AQL LLM Integration Phase 1-4: Complete (parser validation, metrics, documentation, SLA tests)
- [x] AQL Mutations Phase 1-5: Complete (INSERT/UPDATE/REMOVE/UPSERT, transactions, atomicity)
- [~] Wave B Hybrid Planner: In progress (single-shard ANN+graph scope, parallel optimization pending)

### By Severity

- **CRITICAL**: 72
- **HIGH**: 433
- **MEDIUM**: 4106
- **LOW**: 3

### By Type

- allocation_loop: 1
- arithmetic_overflow: 2
- blocking_no_timeout: 12
- braces_imbalance: 14
- braces_imbalance_midfile: 121
- catch_all_swallow: 21
- circular_lock_ordering: 22
- copy_overhead: 35
- critical_function_noexcept: 1
- db_connection_leak: 3
- deadlock_risk: 3
- delete_no_nullptr: 1
- delete_without_nullptr: 1
- duplicate_qualified_signature: 2
- exception_in_destructor: 1
- expensive_copy: 1
- function_return_truncation: 7
- generic_catch: 21
- iterator_invalidation: 15
- legacy_or_compat_path: 18
- lock_contention: 8
- manual_cleanup: 3
- memory_order: 1
- missing_noexcept_on_move: 6
- missing_volatile: 21
- module_doc_linkset_drift: 4
- multiplication_overflow: 6
- no_timeout: 12
- null_dereference: 60
- o_n_squared: 23
- plaintext_transmission: 3
- pointer_arithmetic_unbounded: 2
- posix_only_api: 2
- range_temporary: 4
- repeated_search: 2
- scope_mismatch: 3863
- size_assumption: 1
- smart_ptr_misuse: 1
- stale_doc_section_reference: 7
- string_concat_loop: 61
- todo_as_productionlogic: 101
- uncaught_exception: 25
- unchecked_array_index: 7
- unchecked_result: 55
- uninitialized_access: 28
- uninitialized_array: 1
- uninitialized_variable: 5

## Top 20 Gaps

- [braces_imbalance] continuous_query_engine.cpp:1 (CRITICAL)
- [braces_imbalance] continuous_query_planner.cpp:1 (CRITICAL)
- [braces_imbalance] cypher_parser.cpp:1 (CRITICAL)
- [braces_imbalance] materialized_view.cpp:1 (CRITICAL)
- [braces_imbalance] query_engine.cpp:1 (CRITICAL)
- [braces_imbalance] query_rewrite_rule.cpp:1 (CRITICAL)
- [braces_imbalance] semantic_cache.cpp:1 (CRITICAL)
- [braces_imbalance] sql_parser.cpp:1 (CRITICAL)
- [braces_imbalance] fulltext_functions.cpp:1 (CRITICAL)
- [braces_imbalance] process_mining_functions.cpp:1 (CRITICAL)
- [braces_imbalance] tensor_functions.cpp:1 (CRITICAL)
- [braces_imbalance] udf_registry.cpp:1 (CRITICAL)
- [scope_mismatch] continuous_query_planner.cpp:24 (CRITICAL)
- [blocking_no_timeout] query_canceller.cpp:49 (CRITICAL)
- [no_timeout] query_canceller.cpp:49 (CRITICAL)
- [db_connection_leak] cq_watermark.cpp:60 (CRITICAL)
- [iterator_invalidation] query_rewrite_rule.cpp:105 (CRITICAL)
- [multiplication_overflow] tensor_aware_query_optimizer.cpp:113 (CRITICAL)
- [multiplication_overflow] tensor_aware_query_optimizer.cpp:118 (CRITICAL)
- [multiplication_overflow] tensor_aware_query_optimizer.cpp:123 (CRITICAL)

... and 4594 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
