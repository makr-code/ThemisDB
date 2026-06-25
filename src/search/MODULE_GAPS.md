# search — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **search** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 854
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 2
- **HIGH**: 71
- **MEDIUM**: 779
- **LOW**: 2

### By Type

- braces_imbalance: 1
- command_injection: 2
- exception_in_destructor: 1
- generic_catch: 1
- hardcoded_path: 2
- manual_cleanup: 1
- missing_noexcept_on_move: 1
- module_doc_linkset_drift: 2
- no_timeout: 1
- o_n_squared: 8
- range_temporary: 2
- repeated_search: 4
- scope_mismatch: 770
- stale_doc_section_reference: 1
- string_concat_loop: 1
- todo_as_productionlogic: 40
- uncaught_exception: 1
- unchecked_result: 3
- uninitialized_access: 11
- uninitialized_variable: 1

## Top 20 Gaps

- [no_timeout] search_result_stream.cpp:52 (CRITICAL)
- [exception_in_destructor] hybrid_search.cpp:97 (CRITICAL)
- [braces_imbalance] search_result_stream.cpp:1 (HIGH)
- [uninitialized_access] faceted_search.cpp:16 (HIGH)
- [uninitialized_access] llm_query_rewriter.cpp:16 (HIGH)
- [uninitialized_access] llm_reranker.cpp:16 (HIGH)
- [uninitialized_access] multi_field_search.cpp:16 (HIGH)
- [uninitialized_access] query_expander.cpp:16 (HIGH)
- [uninitialized_access] search_analytics.cpp:16 (HIGH)
- [uninitialized_access] llm_query_rewriter.cpp:41 (HIGH)
- [uninitialized_access] llm_reranker.cpp:42 (HIGH)
- [uninitialized_access] llm_reranker.cpp:48 (HIGH)
- [uninitialized_access] llm_reranker.cpp:51 (HIGH)
- [scope_mismatch] conversational_search.cpp:52 (HIGH)
- [repeated_search] query_expander.cpp:57 (HIGH)
- [scope_mismatch] autocomplete.cpp:58 (HIGH)
- [scope_mismatch] multi_modal_search.cpp:61 (HIGH)
- [scope_mismatch] cross_lingual_search.cpp:70 (HIGH)
- [scope_mismatch] llm_reranker.cpp:75 (HIGH)
- [scope_mismatch] llm_reranker.cpp:82 (HIGH)

... and 834 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
