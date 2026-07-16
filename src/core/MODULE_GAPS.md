# core — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **core** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 473
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 7
- **HIGH**: 18
- **MEDIUM**: 445
- **LOW**: 3

### By Type

- blocking_no_timeout: 1
- braces_imbalance: 2
- circular_lock_ordering: 4
- critical_function_noexcept: 1
- db_connection_leak: 6
- expensive_inner_op: 1
- lock_contention: 3
- manual_cleanup: 1
- missing_dtor: 3
- missing_volatile: 3
- module_doc_linkset_drift: 4
- no_retry_logic: 1
- no_timeout: 1
- posix_only_api: 2
- range_temporary: 1
- scope_mismatch: 395
- size_assumption: 1
- string_concat_loop: 24
- todo_as_productionlogic: 17
- uninitialized_access: 2

## Top 20 Gaps

- [braces_imbalance] concerns_context.cpp:1 (CRITICAL)
- [braces_imbalance] redis_cache.cpp:1 (CRITICAL)
- [missing_dtor] redis_cache.cpp:233 (CRITICAL)
- [missing_dtor] redis_cache.cpp:277 (CRITICAL)
- [missing_dtor] redis_cache.cpp:303 (CRITICAL)
- [blocking_no_timeout] lockfree_metrics.cpp:436 (CRITICAL)
- [no_timeout] lockfree_metrics.cpp:436 (CRITICAL)
- [no_retry_logic] redis_cache.cpp:264 (HIGH)
- [posix_only_api] redis_cache.cpp:280 (HIGH)
- [posix_only_api] redis_cache.cpp:306 (HIGH)
- [size_assumption] zero_copy_logger.cpp:318 (HIGH)
- [db_connection_leak] lockfree_metrics.cpp:346 (HIGH)
- [db_connection_leak] lockfree_metrics.cpp:381 (HIGH)
- [db_connection_leak] lockfree_metrics.cpp:425 (HIGH)
- [db_connection_leak] lockfree_metrics.cpp:428 (HIGH)
- [circular_lock_ordering] redis_cache.cpp:541 (HIGH)
- [circular_lock_ordering] redis_cache.cpp:698 (HIGH)
- [lock_contention] redis_cache.cpp:698 (HIGH)
- [db_connection_leak] redis_cache.cpp:792 (HIGH)
- [circular_lock_ordering] redis_cache.cpp:796 (HIGH)

... and 453 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
