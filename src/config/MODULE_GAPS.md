# config — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **config** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 895
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 13
- **HIGH**: 33
- **MEDIUM**: 847
- **LOW**: 2

### By Type

- braces_imbalance: 2
- copy_overhead: 1
- db_connection_leak: 4
- exception_in_destructor: 2
- hardcoded_path: 3
- legacy_or_compat_path: 12
- lock_contention: 4
- manual_cleanup: 17
- memory_order: 1
- missing_dtor: 4
- missing_noexcept_on_move: 3
- missing_volatile: 2
- module_doc_linkset_drift: 2
- no_timeout: 4
- null_dereference: 5
- path_traversal: 1
- posix_only_api: 1
- range_temporary: 3
- resource_leaked_in_exception: 3
- scope_mismatch: 797
- size_assumption: 1
- smart_ptr_misuse: 1
- stale_doc_section_reference: 1
- string_concat_loop: 2
- todo_as_productionlogic: 12
- unchecked_array_index: 2
- uninitialized_access: 3
- uninitialized_array: 2

## Top 20 Gaps

- [braces_imbalance] config_file_watcher.cpp:1 (CRITICAL)
- [braces_imbalance] config_path_resolver.cpp:1 (CRITICAL)
- [missing_dtor] config_metrics_exporter.cpp:40 (CRITICAL)
- [missing_dtor] config_metrics_exporter.cpp:62 (CRITICAL)
- [no_timeout] config_file_watcher.cpp:244 (CRITICAL)
- [no_timeout] config_file_watcher.cpp:249 (CRITICAL)
- [exception_in_destructor] config_encrypted_store.cpp:348 (CRITICAL)
- [exception_in_destructor] config_encrypted_store.cpp:401 (CRITICAL)
- [no_timeout] config_file_watcher.cpp:422 (CRITICAL)
- [missing_dtor] config_file_watcher.cpp:428 (CRITICAL)
- [missing_dtor] config_file_watcher.cpp:429 (CRITICAL)
- [no_timeout] config_file_watcher.cpp:475 (CRITICAL)
- [smart_ptr_misuse] config_path_resolver.cpp:1348 (CRITICAL)
- [uninitialized_access] config_audit_log.cpp:16 (HIGH)
- [uninitialized_access] config_metrics_exporter.cpp:16 (HIGH)
- [memory_order] config_audit_log.cpp:53 (HIGH)
- [db_connection_leak] config_file_watcher.cpp:86 (HIGH)
- [posix_only_api] config_file_watcher.cpp:110 (HIGH)
- [lock_contention] config_path_resolver.cpp:141 (HIGH)
- [resource_leaked_in_exception] config_metrics_exporter.cpp:209 (HIGH)

... and 875 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
