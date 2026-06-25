# api — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **api** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 601
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 6
- **HIGH**: 45
- **MEDIUM**: 548
- **LOW**: 2

### By Type

- blocking_no_timeout: 1
- braces_imbalance: 3
- circular_lock_ordering: 2
- db_connection_leak: 1
- delete_no_nullptr: 4
- delete_without_nullptr: 4
- generic_catch: 3
- legacy_or_compat_path: 2
- lock_contention: 1
- missing_audit_log: 1
- missing_volatile: 6
- module_doc_linkset_drift: 2
- no_timeout: 1
- null_dereference: 1
- pointer_arithmetic_unbounded: 1
- range_temporary: 1
- resource_leaked_in_exception: 2
- scope_mismatch: 527
- sensitive_data_logging: 1
- smart_ptr_misuse: 1
- stale_doc_section_reference: 1
- string_concat_loop: 5
- todo_as_productionlogic: 17
- uncaught_exception: 4
- uninitialized_access: 9

## Top 20 Gaps

- [braces_imbalance] graphql.cpp:1 (CRITICAL)
- [braces_imbalance] otlp_exporter.cpp:1 (CRITICAL)
- [braces_imbalance] tracing_middleware.cpp:1 (CRITICAL)
- [blocking_no_timeout] grpc_server.cpp:242 (CRITICAL)
- [no_timeout] grpc_server.cpp:242 (CRITICAL)
- [smart_ptr_misuse] graphql.cpp:1675 (CRITICAL)
- [pointer_arithmetic_unbounded] graphql_aql_resolver.cpp:39 (HIGH)
- [uncaught_exception] graphql_aql_resolver.cpp:58 (HIGH)
- [scope_mismatch] graphql_aql_resolver.cpp:87 (HIGH)
- [scope_mismatch] graphql_aql_resolver.cpp:89 (HIGH)
- [scope_mismatch] graphql_aql_resolver.cpp:91 (HIGH)
- [scope_mismatch] graphql_aql_resolver.cpp:93 (HIGH)
- [scope_mismatch] graphql_aql_resolver.cpp:95 (HIGH)
- [scope_mismatch] graphql_aql_resolver.cpp:102 (HIGH)
- [scope_mismatch] graphql_ws_handler.cpp:108 (HIGH)
- [scope_mismatch] graphql_ws_handler.cpp:116 (HIGH)
- [scope_mismatch] graphql_aql_resolver.cpp:119 (HIGH)
- [scope_mismatch] graphql_aql_resolver.cpp:121 (HIGH)
- [scope_mismatch] graphql_aql_resolver.cpp:123 (HIGH)
- [scope_mismatch] graphql_aql_resolver.cpp:125 (HIGH)

... and 581 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
