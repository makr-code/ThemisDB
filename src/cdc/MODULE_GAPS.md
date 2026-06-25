# cdc — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **cdc** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 1091
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 11
- **HIGH**: 61
- **MEDIUM**: 1017
- **LOW**: 2

### By Type

- blocking_no_timeout: 2
- braces_imbalance: 2
- braces_imbalance_midfile: 2
- circular_lock_ordering: 6
- copy_overhead: 4
- db_connection_leak: 7
- delete_no_nullptr: 2
- delete_without_nullptr: 2
- exception_in_destructor: 3
- generic_catch: 1
- legacy_or_compat_path: 6
- lock_contention: 2
- manual_cleanup: 4
- memory_order: 2
- module_doc_linkset_drift: 2
- no_timeout: 2
- o_n_squared: 3
- pointer_arithmetic_unbounded: 3
- range_temporary: 2
- repeated_search: 1
- resource_leaked_in_exception: 4
- scope_mismatch: 973
- sensitive_data_logging: 1
- size_assumption: 9
- smart_ptr_misuse: 3
- stale_doc_section_reference: 1
- todo_as_productionlogic: 25
- uncaught_exception: 8
- unchecked_result: 1
- uninitialized_access: 7
- uninitialized_variable: 1

## Top 20 Gaps

- [braces_imbalance] kafka_cdc_producer.cpp:1 (CRITICAL)
- [exception_in_destructor] tenant_buffer_manager.cpp:37 (CRITICAL)
- [exception_in_destructor] changefeed_buffer.cpp:43 (CRITICAL)
- [blocking_no_timeout] changefeed_buffer.cpp:164 (CRITICAL)
- [no_timeout] changefeed_buffer.cpp:164 (CRITICAL)
- [smart_ptr_misuse] dead_letter_queue.cpp:178 (CRITICAL)
- [smart_ptr_misuse] tenant_buffer_manager.cpp:196 (CRITICAL)
- [blocking_no_timeout] changefeed_buffer.cpp:219 (CRITICAL)
- [no_timeout] changefeed_buffer.cpp:219 (CRITICAL)
- [exception_in_destructor] outbox.cpp:236 (CRITICAL)
- [smart_ptr_misuse] tenant_buffer_manager.cpp:375 (CRITICAL)
- [braces_imbalance] changefeed.cpp:1 (HIGH)
- [uninitialized_access] consumer_group.cpp:5 (HIGH)
- [uninitialized_access] cdc_admin.cpp:16 (HIGH)
- [uninitialized_access] changefeed.cpp:16 (HIGH)
- [uninitialized_access] dead_letter_queue.cpp:16 (HIGH)
- [uninitialized_access] kafka_cdc_producer.cpp:16 (HIGH)
- [uninitialized_access] ws_transport.cpp:16 (HIGH)
- [uncaught_exception] consumer_group.cpp:60 (HIGH)
- [size_assumption] changefeed.cpp:66 (HIGH)

... and 1071 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
