# temporal — MODULE_GAPS.md (historical scanner snapshot)

Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: archived
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors

Exact model backend not verifiable from final document text alone.

> **Historical evidence only:** The counts below are retained from an older
> Phase 5 scanner output. The scan date, scanner version, and reproducible input
> set are not recorded here. They are not verified current findings and must not
> be used to claim present-day vulnerabilities or defects. No rescan was
> performed for this documentation update.

## Summary

- **Total Gaps**: 1257
- **Status**: Historical, unverified scanner snapshot
- **Historical scan location**: `C:\Projects\ThemisDB` (L0 scan with Phase 5 filtering)

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

**Historical Phase 5 note:** The original report said external GitHub
submodules were filtered. The filtering implementation and current scan
coverage were not independently verified. **TODO:** regenerate this report
using the maintained scanner and record its version, date, and reproducible
scope before promoting any finding to a current issue.
