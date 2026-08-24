# aql — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **aql** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps (original scan)**: 1993
- **Gaps Closed (2026-08-24 Wave A–D batch)**: 9 confirmed code gaps + 7 scanner false positives documented
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: 2026-08-24 (Wave A–D gap closure batch)

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

---

## 2026-08-24 Wave A–D Gap Closure Evidence

### CLOSED (production code fixed)

| Gap Type | File | Line | Fix Applied |
|---|---|---|---|
| `todo_as_productionlogic` | `llm_extractive_compressor.cpp` | 289 | Replaced turn-count heuristic with deterministic bag-of-words cosine similarity in `computeSimilarity()` |
| `silent_error_swallow` | `docs_assistant_functions.cpp` | 236 | `catch(...)` now emits `spdlog::debug` before returning `"unknown"` |
| `silent_error_swallow` | `docs_assistant_functions.cpp` | 278 | `catch(...)` now emits `spdlog::debug` before LLM fallback |
| `silent_error_swallow` | `docs_assistant_functions.cpp` | 510 | `catch(...)` on `getStats()` now emits `spdlog::debug` |
| `silent_error_swallow` | `classify_bridge.cpp` | 232 | `catch(...)` now emits `spdlog::debug` on registry failure |
| `silent_error_swallow` | `aql_query_validator.cpp` | 102 | `catch(...)` now emits `spdlog::debug`; `spdlog/spdlog.h` added to includes |
| `silent_error_swallow` | `aql_optimizer_advisor.cpp` | 48 | `catch(...)` now emits `spdlog::debug`; `spdlog/spdlog.h` added to includes |
| `unvalidated_llm_output` | `aql_agent.cpp` | ~130 | Per-step response length guard (`max_tokens_per_step × 8 bytes`) with `spdlog::warn` on truncation |
| `unvalidated_llm_output` | `aql_query_builder.cpp` | ~635 | 256-byte per-suggestion guard; oversized lines discarded |

### CONFIRMED FALSE POSITIVES (no code change required)

| Gap Type | File | Line | Evidence |
|---|---|---|---|
| `smart_ptr_misuse` | `aql_lora_finetuner.cpp` | 338 | Line 338 is `samples_.push_back(makeSample(...))` — no raw pointer. Scanner artifact. |
| `exception_in_destructor` | `aql_ingestion_bridge.cpp` | 47 | `~AQLIngestionBridge() noexcept = default` — destructor is already `noexcept`. Scanner artifact. |
| `new_without_raii`/`new_without_delete`/`smart_ptr_misuse` | `docs_assistant_functions.cpp` | 554 | Meyer's singleton (`static DocsAssistantFunctions instance`) is RAII-safe per C++11 §6.7. Scanner artifact. |
| `prompt_injection` | `llm_aql_handler.cpp` | 1813 | `sanitizePromptInput()` + collection scope/ACL checks already in place. Scanner artifact. |
| `braces_imbalance` | multiple | 1 | Reported at line 1 of each file due to Doxygen maturity block. Brace balance is correct. Scanner artifact. |
| `uninitialized_access` | multiple | 16 | Reported at first `#include` line. No uninitialized members. Scanner artifact. |
| `scope_mismatch` (1698 total) | multiple | various | Scanner flags every `namespace` nesting as mismatch. All namespaces balanced. Scanner artifact. |
