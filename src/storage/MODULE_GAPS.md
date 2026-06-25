# storage — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **storage** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 4717
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 80
- **HIGH**: 479
- **MEDIUM**: 4155
- **LOW**: 3

### By Type

- allocation_loop: 1
- arithmetic_overflow: 7
- blocking_no_timeout: 2
- braces_imbalance: 12
- braces_imbalance_midfile: 6
- circular_lock_ordering: 39
- command_injection: 2
- copy_overhead: 16
- coupling_risk_sharding_storage: 2
- critical_function_noexcept: 2
- db_connection_leak: 23
- deadlock_risk: 2
- delete_no_nullptr: 9
- delete_without_nullptr: 9
- duplicate_qualified_signature: 11
- exception_in_destructor: 5
- expensive_inner_op: 1
- generic_catch: 11
- getsnapshot\(\): 3
- gpu_memory_leak: 2
- hardcoded_path: 3
- iterator_invalidation: 5
- legacy_or_compat_path: 26
- lock_contention: 15
- manual_cleanup: 25
- memory_order: 8
- missing_adr_reference: 1
- missing_dtor: 2
- missing_noexcept_on_move: 3
- missing_override_keyword: 4
- missing_resource_limits: 1
- missing_volatile: 8
- module_doc_linkset_drift: 2
- multiplication_overflow: 1
- new_without_delete: 1
- new_without_raii: 1
- no_retry_logic: 13
- no_timeout: 13
- no_transit_encryption: 38
- null_dereference: 44
- o_n_squared: 3
- path_traversal: 6
- pointer_arithmetic_unbounded: 5
- posix_only_api: 5
- pure_virtual_unimplemented: 2
- range_temporary: 22
- repeated_lookup: 1
- repeated_search: 4
- resource_leaked_in_exception: 4
- scope_mismatch: 3970
- shift_overflow: 3
- silent_error_swallow: 1
- simulation_stub_marker: 5
- size_assumption: 51
- smart_ptr_misuse: 2
- stale_doc_section_reference: 2
- string_concat_loop: 2
- todo_as_productionlogic: 115
- uncaught_exception: 13
- unchecked_array_index: 6
- unchecked_cuda_call: 36
- unchecked_memcpy: 2
- unchecked_result: 22
- uninitialized_access: 32
- uninitialized_array: 1
- uninitialized_variable: 20
- use_after_free_gpu: 1
- windows_only_api: 2

## Top 20 Gaps

- [braces_imbalance] blob_backend_gcs.cpp:1 (CRITICAL)
- [braces_imbalance] database_connection_manager.cpp:1 (CRITICAL)
- [braces_imbalance] gguf_metadata.cpp:1 (CRITICAL)
- [braces_imbalance] storage_parquet_exporter.cpp:1 (CRITICAL)
- [braces_imbalance] tensor_compaction_filter.cpp:1 (CRITICAL)
- [braces_imbalance] wom_tree.cpp:1 (CRITICAL)
- [exception_in_destructor] compaction_manager.cpp:54 (CRITICAL)
- [exception_in_destructor] index_maintenance.cpp:54 (CRITICAL)
- [smart_ptr_misuse] streaming_ingest_manager.cpp:64 (CRITICAL)
- [scope_mismatch] wom_tree.cpp:70 (CRITICAL)
- [iterator_invalidation] columnar_cache.cpp:105 (CRITICAL)
- [scope_mismatch] wom_tree.cpp:107 (CRITICAL)
- [blocking_no_timeout] concurrent_write_controller.cpp:113 (CRITICAL)
- [no_timeout] concurrent_write_controller.cpp:113 (CRITICAL)
- [iterator_invalidation] hamming_coder.cpp:114 (CRITICAL)
- [exception_in_destructor] blob_backend_azure.cpp:117 (CRITICAL)
- [unchecked_memcpy] erasure_coder_factory.cpp:123 (CRITICAL)
- [blocking_no_timeout] concurrent_write_controller.cpp:125 (CRITICAL)
- [no_timeout] concurrent_write_controller.cpp:125 (CRITICAL)
- [new_without_raii] database_connection_manager.cpp:129 (CRITICAL)

... and 4697 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
