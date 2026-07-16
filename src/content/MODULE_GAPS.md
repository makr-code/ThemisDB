# content — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **content** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 3222
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 48
- **HIGH**: 402
- **MEDIUM**: 2770
- **LOW**: 2

### By Type

- allocation_loop: 2
- arithmetic_overflow: 5
- blocking_no_timeout: 3
- braces_imbalance: 22
- braces_imbalance_midfile: 47
- cast_to_smaller_type: 9
- circular_lock_ordering: 43
- copy_overhead: 9
- deadlock_risk: 2
- delete_no_nullptr: 1
- delete_without_nullptr: 1
- exception_in_destructor: 1
- expensive_inner_op: 2
- generic_catch: 8
- hardcoded_path: 15
- legacy_or_compat_path: 6
- manual_cleanup: 16
- missing_noexcept_on_move: 6
- missing_resource_limits: 5
- missing_volatile: 3
- module_doc_linkset_drift: 2
- multiplication_overflow: 1
- new_without_raii: 1
- no_timeout: 6
- null_dereference: 15
- o_n_squared: 2
- pointer_arithmetic_unbounded: 220
- posix_only_api: 2
- range_temporary: 4
- resource_leaked_in_exception: 1
- scope_mismatch: 2629
- silent_error_swallow: 4
- smart_ptr_misuse: 1
- stale_doc_section_reference: 10
- string_concat_loop: 14
- todo_as_productionlogic: 73
- uncaught_exception: 14
- unchecked_array_index: 1
- unchecked_result: 9
- uninitialized_access: 3
- uninitialized_array: 1
- uninitialized_variable: 3

## Top 20 Gaps

- [braces_imbalance] abuse_detector.cpp:1 (CRITICAL)
- [braces_imbalance] archive_processor.cpp:1 (CRITICAL)
- [braces_imbalance] audio_processor.cpp:1 (CRITICAL)
- [braces_imbalance] cad_processor.cpp:1 (CRITICAL)
- [braces_imbalance] content_manager.cpp:1 (CRITICAL)
- [braces_imbalance] content_manager_embedding.cpp:1 (CRITICAL)
- [braces_imbalance] content_metrics.cpp:1 (CRITICAL)
- [braces_imbalance] deduplication_checker.cpp:1 (CRITICAL)
- [braces_imbalance] image_processor.cpp:1 (CRITICAL)
- [braces_imbalance] language_detector.cpp:1 (CRITICAL)
- [braces_imbalance] text_processor.cpp:1 (CRITICAL)
- [braces_imbalance] audio_extractor_adapter.cpp:1 (CRITICAL)
- [braces_imbalance] image_extractor_adapter.cpp:1 (CRITICAL)
- [braces_imbalance] office_extractor_adapter.cpp:1 (CRITICAL)
- [braces_imbalance] pdf_extractor_adapter.cpp:1 (CRITICAL)
- [braces_imbalance] text_extractor_adapter.cpp:1 (CRITICAL)
- [scope_mismatch] content_manager_embedding.cpp:14 (CRITICAL)
- [scope_mismatch] image_extractor_adapter.cpp:25 (CRITICAL)
- [scope_mismatch] image_extractor_adapter.cpp:26 (CRITICAL)
- [scope_mismatch] pdf_extractor_adapter.cpp:26 (CRITICAL)

... and 3202 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
