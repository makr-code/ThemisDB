# network — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **network** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 2083
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 29
- **HIGH**: 491
- **MEDIUM**: 1561
- **LOW**: 2

### By Type

- allocation_loop: 1
- arithmetic_overflow: 1
- blocking_no_timeout: 2
- braces_imbalance: 8
- braces_imbalance_midfile: 76
- catch_all_swallow: 4
- circular_lock_ordering: 100
- command_injection: 3
- copy_overhead: 8
- db_connection_leak: 41
- deadlock_risk: 7
- duplicate_qualified_signature: 2
- endianness_assumption: 8
- exception_in_destructor: 1
- expensive_inner_op: 3
- generic_catch: 7
- hardcoded_path: 8
- legacy_or_compat_path: 1
- lock_contention: 8
- manual_cleanup: 42
- memory_order: 2
- missing_dtor: 4
- missing_noexcept_on_move: 3
- missing_volatile: 41
- module_doc_linkset_drift: 2
- no_retry_logic: 111
- no_timeout: 6
- null_dereference: 12
- o_n_squared: 1
- path_traversal: 1
- pointer_arithmetic_unbounded: 1
- posix_only_api: 1
- range_temporary: 23
- resource_leaked_in_exception: 8
- scope_mismatch: 1404
- sensitive_data_logging: 2
- size_assumption: 12
- smart_ptr_misuse: 3
- stale_doc_section_reference: 1
- string_concat_loop: 10
- todo_as_productionlogic: 47
- uncaught_exception: 22
- unchecked_array_index: 2
- unchecked_memcpy: 2
- unchecked_result: 16
- uninitialized_access: 4
- uninitialized_array: 3
- uninitialized_variable: 8

## Top 20 Gaps

- [braces_imbalance] kernel_bypass.cpp:1 (CRITICAL)
- [braces_imbalance] quic_server.cpp:1 (CRITICAL)
- [braces_imbalance] raft_load_balancer.cpp:1 (CRITICAL)
- [braces_imbalance] wire_protocol_performance.cpp:1 (CRITICAL)
- [braces_imbalance] wire_protocol_v2.cpp:1 (CRITICAL)
- [missing_dtor] socket_timeout_manager.cpp:71 (CRITICAL)
- [no_timeout] wire_protocol_zero_copy.cpp:112 (CRITICAL)
- [unchecked_memcpy] connection_compression.cpp:114 (CRITICAL)
- [no_timeout] wire_protocol_zero_copy.cpp:160 (CRITICAL)
- [missing_dtor] service_mesh.cpp:175 (CRITICAL)
- [missing_dtor] service_mesh.cpp:194 (CRITICAL)
- [smart_ptr_misuse] socket_timeout_manager.cpp:202 (CRITICAL)
- [exception_in_destructor] wire_protocol_zero_copy.cpp:220 (CRITICAL)
- [blocking_no_timeout] wire_protocol_performance.cpp:232 (CRITICAL)
- [no_timeout] wire_protocol_performance.cpp:232 (CRITICAL)
- [no_timeout] service_mesh.cpp:243 (CRITICAL)
- [unchecked_memcpy] io_uring_batcher.cpp:251 (CRITICAL)
- [db_connection_leak] raft_load_balancer.cpp:288 (CRITICAL)
- [db_connection_leak] raft_load_balancer.cpp:289 (CRITICAL)
- [db_connection_leak] raft_load_balancer.cpp:290 (CRITICAL)

... and 2063 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
