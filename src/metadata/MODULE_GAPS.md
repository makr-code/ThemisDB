# metadata — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **metadata** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 999
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 8
- **HIGH**: 42
- **MEDIUM**: 947
- **LOW**: 2

### By Type

- blocking_no_timeout: 3
- braces_imbalance: 2
- circular_lock_ordering: 7
- copy_overhead: 12
- critical_function_noexcept: 1
- deadlock_risk: 1
- delete_no_nullptr: 2
- delete_without_nullptr: 2
- generic_catch: 1
- lock_contention: 3
- module_doc_linkset_drift: 2
- no_timeout: 3
- o_n_squared: 2
- range_temporary: 10
- repeated_search: 3
- resource_leaked_in_exception: 1
- scope_mismatch: 903
- sensitive_data_logging: 3
- string_concat_loop: 8
- todo_as_productionlogic: 24
- uncaught_exception: 1
- unchecked_result: 4
- uninitialized_access: 1

## Top 20 Gaps

- [braces_imbalance] catalog_exporter.cpp:1 (CRITICAL)
- [braces_imbalance] schema_consistency_checker.cpp:1 (CRITICAL)
- [blocking_no_timeout] schema_manager.cpp:187 (CRITICAL)
- [no_timeout] schema_manager.cpp:187 (CRITICAL)
- [blocking_no_timeout] schema_manager.cpp:210 (CRITICAL)
- [no_timeout] schema_manager.cpp:210 (CRITICAL)
- [blocking_no_timeout] schema_manager.cpp:233 (CRITICAL)
- [no_timeout] schema_manager.cpp:233 (CRITICAL)
- [uninitialized_access] schema_manager.cpp:16 (HIGH)
- [circular_lock_ordering] schema_consistency_checker.cpp:93 (HIGH)
- [circular_lock_ordering] statistics_collector.cpp:178 (HIGH)
- [deadlock_risk] statistics_collector.cpp:178 (HIGH)
- [lock_contention] statistics_collector.cpp:178 (HIGH)
- [scope_mismatch] schema_constraints.cpp:182 (HIGH)
- [scope_mismatch] schema_constraints.cpp:184 (HIGH)
- [range_temporary] schema_audit_log.cpp:226 (HIGH)
- [o_n_squared] index_recommender.cpp:231 (HIGH)
- [range_temporary] schema_audit_log.cpp:234 (HIGH)
- [lock_contention] schema_consistency_checker.cpp:254 (HIGH)
- [circular_lock_ordering] schema_manager.cpp:307 (HIGH)

... and 979 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
