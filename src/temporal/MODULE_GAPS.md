# temporal — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **temporal** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 1257
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 11
- **HIGH**: 75
- **MEDIUM**: 1169
- **LOW**: 2

### By Type

- arithmetic_overflow: 1
- braces_imbalance: 2
- circular_lock_ordering: 7
- copy_overhead: 16
- db_connection_leak: 2
- duplicate_qualified_signature: 1
- iterator_invalidation: 4
- lock_contention: 4
- manual_cleanup: 9
- missing_noexcept_on_move: 3
- missing_volatile: 1
- module_doc_linkset_drift: 2
- no_timeout: 6
- null_dereference: 4
- o_n_squared: 7
- path_traversal: 5
- range_temporary: 4
- repeated_search: 2
- scope_mismatch: 1141
- size_assumption: 2
- string_concat_loop: 1
- todo_as_productionlogic: 30
- unchecked_result: 1
- uninitialized_access: 1
- uninitialized_variable: 1

## Top 20 Gaps

- [braces_imbalance] temporal_compressor.cpp:1 (CRITICAL)
- [iterator_invalidation] retention_manager.cpp:80 (CRITICAL)
- [iterator_invalidation] snapshot_manager.cpp:332 (CRITICAL)
- [no_timeout] temporal_cdc.cpp:333 (CRITICAL)
- [no_timeout] temporal_cdc.cpp:358 (CRITICAL)
- [iterator_invalidation] temporal_tier_manager.cpp:365 (CRITICAL)
- [no_timeout] temporal_cdc.cpp:396 (CRITICAL)
- [no_timeout] temporal_cdc.cpp:434 (CRITICAL)
- [no_timeout] temporal_cdc.cpp:469 (CRITICAL)
- [iterator_invalidation] temporal_tier_manager.cpp:563 (CRITICAL)
- [no_timeout] temporal_cdc.cpp:631 (CRITICAL)
- [braces_imbalance] temporal_cdc.cpp:1 (HIGH)
- [circular_lock_ordering] snapshot_manager.cpp:64 (HIGH)
- [missing_noexcept_on_move] temporal_aggregator.cpp:65 (HIGH)
- [scope_mismatch] bitemporal_join.cpp:68 (HIGH)
- [arithmetic_overflow] interval_tree_index.cpp:71 (HIGH)
- [scope_mismatch] temporal_aggregator.cpp:76 (HIGH)
- [circular_lock_ordering] snapshot_manager.cpp:78 (HIGH)
- [scope_mismatch] snapshot_manager.cpp:82 (HIGH)
- [scope_mismatch] snapshot_manager.cpp:87 (HIGH)

... and 1237 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
