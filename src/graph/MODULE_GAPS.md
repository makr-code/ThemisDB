# graph — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **graph** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 1578
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 10
- **HIGH**: 82
- **MEDIUM**: 1484
- **LOW**: 2

### By Type

- allocation_loop: 1
- arithmetic_overflow: 3
- braces_imbalance: 5
- circular_lock_ordering: 20
- copy_overhead: 9
- db_connection_leak: 4
- generic_catch: 3
- hardcoded_path: 4
- iterator_invalidation: 4
- legacy_or_compat_path: 5
- lock_contention: 1
- memory_order: 1
- missing_dtor: 2
- missing_noexcept_on_move: 1
- missing_volatile: 6
- module_doc_linkset_drift: 2
- null_dereference: 4
- o_n_squared: 6
- pointer_arithmetic_unbounded: 1
- repeated_search: 6
- scope_mismatch: 1427
- silent_error_swallow: 1
- size_assumption: 3
- string_concat_loop: 16
- todo_as_productionlogic: 26
- uncaught_exception: 2
- unchecked_result: 8
- uninitialized_access: 5
- uninitialized_variable: 2

## Top 20 Gaps

- [braces_imbalance] distributed_graph.cpp:1 (CRITICAL)
- [braces_imbalance] gpu_traversal.cpp:1 (CRITICAL)
- [braces_imbalance] scheduled_edge_refresh.cpp:1 (CRITICAL)
- [braces_imbalance] tensor_deduplication_manager.cpp:1 (CRITICAL)
- [scope_mismatch] gpu_traversal.cpp:43 (CRITICAL)
- [iterator_invalidation] gpu_traversal.cpp:174 (CRITICAL)
- [missing_dtor] ontology_manager.cpp:192 (CRITICAL)
- [iterator_invalidation] graph_query_optimizer.cpp:1412 (CRITICAL)
- [iterator_invalidation] graph_query_optimizer.cpp:2126 (CRITICAL)
- [missing_dtor] graph_query_optimizer.cpp:2800 (CRITICAL)
- [braces_imbalance] knowledge_graph_reasoner.cpp:1 (HIGH)
- [uninitialized_access] graph_query_optimizer.cpp:16 (HIGH)
- [uninitialized_access] path_constraints.cpp:16 (HIGH)
- [scope_mismatch] explain_plan.cpp:68 (HIGH)
- [unchecked_result] knowledge_graph_reasoner.cpp:68 (HIGH)
- [pointer_arithmetic_unbounded] graph_watermark.cpp:73 (HIGH)
- [scope_mismatch] explain_plan.cpp:92 (HIGH)
- [scope_mismatch] rotate_completion.cpp:95 (HIGH)
- [scope_mismatch] rotate_completion.cpp:109 (HIGH)
- [circular_lock_ordering] scheduled_edge_refresh.cpp:113 (HIGH)

... and 1558 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
