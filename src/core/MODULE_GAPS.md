---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Exact model backend not verifiable from final document text alone.

# core — MODULE_GAPS.md (Historical Scanner Snapshot; Not Current Findings)

> Historical scanner snapshot only. The counts, severities, and source locations
> below have not been independently revalidated against the current source tree.
> No current gap scan was run for this review; these counts are not current
> findings or release evidence.

This file preserves the historical output of a gap-scanner run (Phase 5 with external submodule filtering); do not use it as the current inventory of Core defects.

## Summary

- **Total Gaps**: 473
- **Status**: Historical and unverified against the current source tree
- **Scanner snapshot**: `C:\Projects\ThemisDB` (L0 full scan with Phase 5; date not recorded)

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

**Historical Phase 5 note**: The original scanner report says external GitHub submodules were excluded. That filtering and the claimed scope accuracy were not re-run or verified against the current tree; treat the listed locations and counts as historical only.
