# metadata — MODULE_GAPS.md (Historical Phase 5 Snapshot)

**Author:** ThemisDB Contributors  
**Created:** 2026-10-31  
**Last Updated:** 2026-10-31  
**Status:** archived  
**AI_Used:** true  
**AI_Models:** unknown  
**AI_Contributor:** GitHub Copilot coding agent / ThemisDB documentation orchestrator  
**Human_Review:** false  
**Human_Author:** ThemisDB Contributors  
**Source level:** level1 — module-local  
**SOT domain:** module-behavior  
**Canonical source references:** [metadata source tree](./), [module roadmap](./ROADMAP.md)  
**Related issue / milestone:** [#7362](https://github.com/makr-code/ThemisDB/issues/7362) · DOC-WEEKLY-2026-W44

> Exact model backend not verifiable from final document text alone.

> Historical scanner output only. The scan date is not recorded, and its
> counts/findings have not been verified against current source. Do not treat
> this snapshot as a current gap report or severity assessment.

This file documents all documentation and code quality gaps in the **metadata** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 999
- **Status**: Historical Phase 5 scan output (not revalidated)
- **Scan date**: Not recorded in this artifact

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
