# aql — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **aql** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 1993
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 13
- **HIGH**: 151
- **MEDIUM**: 1827
- **LOW**: 2

### By Type

- braces_imbalance: 6
- braces_imbalance_midfile: 1
- circular_lock_ordering: 3
- command_injection: 4
- copy_overhead: 41
- db_connection_leak: 2
- exception_in_destructor: 1
- generic_catch: 4
- hardcoded_path: 25
- legacy_or_compat_path: 10
- missing_noexcept_on_move: 7
- missing_resource_limits: 14
- module_doc_linkset_drift: 2
- new_without_delete: 1
- new_without_raii: 1
- no_retry_logic: 9
- null_dereference: 17
- o_n_squared: 7
- pointer_arithmetic_unbounded: 7
- prompt_injection: 1
- range_temporary: 2
- repeated_lookup: 2
- repeated_search: 3
- resource_leaked_in_exception: 5
- scope_mismatch: 1698
- sensitive_data_logging: 2
- silent_error_swallow: 1
- smart_ptr_misuse: 2
- string_concat_loop: 6
- todo_as_productionlogic: 40
- uncaught_exception: 29
- unchecked_result: 12
- uninitialized_access: 12
- uninitialized_variable: 5
- unsanitized_llm_input: 4
- unvalidated_llm_output: 7

## Top 20 Gaps

- [braces_imbalance] aql_agent.cpp:1 (CRITICAL)
- [braces_imbalance] aql_confidence_scorer.cpp:1 (CRITICAL)
- [braces_imbalance] aql_query_template_library.cpp:1 (CRITICAL)
- [braces_imbalance] aql_syntax_highlighter.cpp:1 (CRITICAL)
- [braces_imbalance] llm_aql_handler.cpp:1 (CRITICAL)
- [scope_mismatch] aql_confidence_scorer.cpp:28 (CRITICAL)
- [scope_mismatch] aql_confidence_scorer.cpp:29 (CRITICAL)
- [exception_in_destructor] aql_ingestion_bridge.cpp:47 (CRITICAL)
- [smart_ptr_misuse] aql_lora_finetuner.cpp:338 (CRITICAL)
- [new_without_raii] docs_assistant_functions.cpp:554 (CRITICAL)
- [new_without_delete] docs_assistant_functions.cpp:554 (CRITICAL)
- [smart_ptr_misuse] docs_assistant_functions.cpp:554 (CRITICAL)
- [prompt_injection] llm_aql_handler.cpp:1813 (CRITICAL)
- [braces_imbalance] llm_semantic_validator.cpp:1 (HIGH)
- [uninitialized_access] aql_confidence_scorer.cpp:16 (HIGH)
- [uninitialized_access] aql_query_builder.cpp:16 (HIGH)
- [uninitialized_access] aql_query_template_library.cpp:16 (HIGH)
- [uninitialized_access] aql_query_validator.cpp:16 (HIGH)
- [uninitialized_access] llm_metrics_collector.cpp:16 (HIGH)
- [silent_error_swallow] llm_aql_embedding_bridge.cpp:39 (HIGH)

... and 1973 more gaps.

## Manual Remediation Batch (2026-08-24)

Closed high-confidence production gaps in this batch:

- Replaced hardcoded retry-attempt defaults in `translateNLToAQL*` with `validation_config.max_retries` wiring.
- Replaced `LLMValidationPipeline` TODO-metric placeholders with real `LLMMetricsCollector` calls.
- Added LLM readiness gate and parser-feedback reinjection in `LLMValidationPipeline` retry flow.
- Replaced `LLMExtractiveCompressor::isAvailable()` TODO return path with real client readiness checks.
- Replaced silent compressor persistence catch path with warning-level diagnostics.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
