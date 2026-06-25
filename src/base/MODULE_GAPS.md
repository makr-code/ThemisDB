# base — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **base** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 829
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 32
- **HIGH**: 56
- **MEDIUM**: 739
- **LOW**: 2

### By Type

- blocking_no_timeout: 5
- braces_imbalance: 5
- braces_imbalance_midfile: 2
- circular_lock_ordering: 4
- command_injection: 1
- copy_overhead: 1
- db_connection_leak: 3
- duplicate_qualified_signature: 5
- legacy_or_compat_path: 7
- lock_contention: 1
- manual_cleanup: 6
- missing_dtor: 2
- missing_noexcept_on_move: 1
- missing_volatile: 4
- module_doc_linkset_drift: 2
- no_timeout: 8
- no_transit_encryption: 16
- null_dereference: 2
- o_n_squared: 2
- path_traversal: 1
- posix_only_api: 7
- range_temporary: 4
- repeated_search: 2
- resource_leaked_in_exception: 5
- scope_mismatch: 692
- sensitive_data_logging: 6
- size_assumption: 1
- string_concat_loop: 9
- todo_as_productionlogic: 16
- unchecked_array_index: 2
- unchecked_result: 2
- uninitialized_access: 4
- uninitialized_array: 1

## Top 20 Gaps

- [braces_imbalance] hot_reload_manager.cpp:1 (CRITICAL)
- [blocking_no_timeout] remote_registry_client.cpp:73 (CRITICAL)
- [no_timeout] remote_registry_client.cpp:73 (CRITICAL)
- [missing_dtor] remote_registry_client.cpp:105 (CRITICAL)
- [missing_dtor] remote_registry_client.cpp:110 (CRITICAL)
- [no_timeout] remote_registry_client.cpp:127 (CRITICAL)
- [blocking_no_timeout] remote_registry_client.cpp:152 (CRITICAL)
- [no_timeout] remote_registry_client.cpp:152 (CRITICAL)
- [blocking_no_timeout] remote_registry_client.cpp:160 (CRITICAL)
- [no_timeout] remote_registry_client.cpp:160 (CRITICAL)
- [no_timeout] remote_registry_client.cpp:188 (CRITICAL)
- [no_transit_encryption] remote_registry_client.cpp:547 (CRITICAL)
- [no_transit_encryption] remote_registry_client.cpp:548 (CRITICAL)
- [no_transit_encryption] remote_registry_client.cpp:549 (CRITICAL)
- [no_transit_encryption] remote_registry_client.cpp:550 (CRITICAL)
- [no_transit_encryption] remote_registry_client.cpp:551 (CRITICAL)
- [no_transit_encryption] remote_registry_client.cpp:552 (CRITICAL)
- [no_transit_encryption] remote_registry_client.cpp:553 (CRITICAL)
- [no_transit_encryption] remote_registry_client.cpp:555 (CRITICAL)
- [no_transit_encryption] remote_registry_client.cpp:675 (CRITICAL)

... and 809 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
