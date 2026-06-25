# cache — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **cache** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 1571
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 11
- **HIGH**: 227
- **MEDIUM**: 1331
- **LOW**: 2

### By Type

- blocking_no_timeout: 3
- braces_imbalance: 3
- braces_imbalance_midfile: 3
- circular_lock_ordering: 114
- command_injection: 1
- db_connection_leak: 1
- deadlock_risk: 15
- delete_no_nullptr: 2
- delete_without_nullptr: 2
- duplicate_qualified_signature: 14
- generic_catch: 1
- legacy_or_compat_path: 2
- lock_contention: 8
- manual_cleanup: 3
- memory_order: 1
- missing_dtor: 3
- missing_noexcept_on_move: 2
- missing_volatile: 4
- module_doc_linkset_drift: 2
- no_retry_logic: 2
- no_timeout: 4
- null_dereference: 57
- o_n_squared: 1
- range_temporary: 7
- scope_mismatch: 1287
- stale_doc_section_reference: 3
- todo_as_productionlogic: 23
- uncaught_exception: 1
- uninitialized_access: 1
- uninitialized_array: 1

## Top 20 Gaps

- [braces_imbalance] distributed_cache_coordinator.cpp:1 (CRITICAL)
- [braces_imbalance] predictive_prefetcher.cpp:1 (CRITICAL)
- [no_timeout] adaptive_query_cache.cpp:128 (CRITICAL)
- [blocking_no_timeout] cache_replication_coordinator.cpp:314 (CRITICAL)
- [no_timeout] cache_replication_coordinator.cpp:314 (CRITICAL)
- [missing_dtor] distributed_cache_coordinator.cpp:406 (CRITICAL)
- [missing_dtor] distributed_cache_coordinator.cpp:410 (CRITICAL)
- [blocking_no_timeout] adaptive_query_cache.cpp:985 (CRITICAL)
- [no_timeout] adaptive_query_cache.cpp:985 (CRITICAL)
- [blocking_no_timeout] adaptive_query_cache.cpp:994 (CRITICAL)
- [no_timeout] adaptive_query_cache.cpp:994 (CRITICAL)
- [braces_imbalance] adaptive_query_cache.cpp:1 (HIGH)
- [circular_lock_ordering] distributed_cache_coordinator.cpp:57 (HIGH)
- [circular_lock_ordering] cache_replication_coordinator.cpp:63 (HIGH)
- [circular_lock_ordering] redis_cache_coordinator.cpp:66 (HIGH)
- [null_dereference] bounded_lru_cache.cpp:69 (HIGH)
- [circular_lock_ordering] cache_replication_coordinator.cpp:71 (HIGH)
- [lock_contention] semantic_cache.cpp:75 (HIGH)
- [scope_mismatch] semantic_cache.cpp:76 (HIGH)
- [scope_mismatch] warmup.cpp:78 (HIGH)

... and 1551 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
