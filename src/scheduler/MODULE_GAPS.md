# scheduler — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **scheduler** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 1493
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 6
- **HIGH**: 291
- **MEDIUM**: 1194
- **LOW**: 2

### By Type

- allocation_loop: 1
- braces_imbalance: 3
- braces_imbalance_midfile: 223
- circular_lock_ordering: 30
- copy_overhead: 6
- critical_function_noexcept: 2
- db_connection_leak: 2
- exception_in_destructor: 4
- hardcoded_path: 1
- legacy_or_compat_path: 5
- lock_contention: 3
- manual_cleanup: 2
- missing_volatile: 3
- module_doc_linkset_drift: 2
- null_dereference: 8
- o_n_squared: 2
- pointer_arithmetic_unbounded: 4
- range_temporary: 3
- scope_mismatch: 1151
- sensitive_data_logging: 5
- string_concat_loop: 9
- todo_as_productionlogic: 17
- uncaught_exception: 2
- unchecked_result: 1
- unhandled_critical_operation: 1
- uninitialized_access: 3

## Top 20 Gaps

- [braces_imbalance] hybrid_retention_manager.cpp:1 (CRITICAL)
- [exception_in_destructor] hybrid_retention_manager.cpp:56 (CRITICAL)
- [exception_in_destructor] distributed_task_coordinator.cpp:67 (CRITICAL)
- [exception_in_destructor] event_trigger.cpp:119 (CRITICAL)
- [unhandled_critical_operation] task_audit_manager.cpp:155 (CRITICAL)
- [exception_in_destructor] event_trigger.cpp:492 (CRITICAL)
- [braces_imbalance] distributed_task_coordinator.cpp:1 (HIGH)
- [braces_imbalance] task_scheduler.cpp:1 (HIGH)
- [uninitialized_access] distributed_task_coordinator.cpp:16 (HIGH)
- [circular_lock_ordering] event_trigger.cpp:124 (HIGH)
- [scope_mismatch] task_scheduler.cpp:128 (HIGH)
- [circular_lock_ordering] distributed_task_coordinator.cpp:137 (HIGH)
- [circular_lock_ordering] event_trigger.cpp:142 (HIGH)
- [circular_lock_ordering] distributed_task_coordinator.cpp:165 (HIGH)
- [circular_lock_ordering] event_trigger.cpp:165 (HIGH)
- [circular_lock_ordering] distributed_task_coordinator.cpp:195 (HIGH)
- [allocation_loop] event_trigger.cpp:247 (HIGH)
- [lock_contention] event_trigger.cpp:279 (HIGH)
- [db_connection_leak] distributed_task_coordinator.cpp:297 (HIGH)
- [circular_lock_ordering] event_trigger.cpp:306 (HIGH)

... and 1473 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
