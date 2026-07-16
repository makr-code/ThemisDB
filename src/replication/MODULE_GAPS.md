# replication — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **replication** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 1519
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 16
- **HIGH**: 194
- **MEDIUM**: 1307
- **LOW**: 2

### By Type

- allocation_loop: 1
- arithmetic_overflow: 1
- braces_imbalance: 4
- braces_imbalance_midfile: 9
- circular_lock_ordering: 96
- copy_overhead: 5
- db_connection_leak: 5
- duplicate_qualified_signature: 9
- hardcoded_path: 1
- iterator_invalidation: 2
- legacy_or_compat_path: 1
- lock_contention: 11
- manual_cleanup: 11
- missing_noexcept_on_move: 2
- missing_volatile: 14
- module_doc_linkset_drift: 2
- multiplication_overflow: 1
- no_timeout: 10
- null_dereference: 1
- o_n_squared: 8
- pointer_arithmetic_unbounded: 8
- range_temporary: 21
- repeated_lookup: 1
- resource_leaked_in_exception: 2
- scope_mismatch: 1262
- silent_error_swallow: 2
- string_concat_loop: 1
- todo_as_productionlogic: 20
- unchecked_array_index: 1
- unchecked_result: 6
- uninitialized_array: 1

## Top 20 Gaps

- [braces_imbalance] observability.cpp:1 (CRITICAL)
- [braces_imbalance] policy.cpp:1 (CRITICAL)
- [scope_mismatch] observability.cpp:34 (CRITICAL)
- [multiplication_overflow] replication_manager.cpp:549 (CRITICAL)
- [no_timeout] replication_manager.cpp:558 (CRITICAL)
- [no_timeout] logical_replication.cpp:647 (CRITICAL)
- [no_timeout] replication_manager.cpp:654 (CRITICAL)
- [no_timeout] logical_replication.cpp:702 (CRITICAL)
- [iterator_invalidation] replication_manager.cpp:2769 (CRITICAL)
- [no_timeout] replication_manager.cpp:3331 (CRITICAL)
- [iterator_invalidation] replication_manager.cpp:4052 (CRITICAL)
- [no_timeout] replication_manager.cpp:4170 (CRITICAL)
- [no_timeout] replication_manager.cpp:6024 (CRITICAL)
- [no_timeout] replication_manager.cpp:6059 (CRITICAL)
- [no_timeout] replication_manager.cpp:6857 (CRITICAL)
- [no_timeout] replication_manager.cpp:6895 (CRITICAL)
- [braces_imbalance] logical_replication.cpp:1 (HIGH)
- [braces_imbalance] replication_manager.cpp:1 (HIGH)
- [circular_lock_ordering] replication_slot.cpp:74 (HIGH)
- [circular_lock_ordering] replication_slot.cpp:84 (HIGH)

... and 1499 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
