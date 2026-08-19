# analytics — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **analytics** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 3696 (scanner snapshot; see Gap Closure Updates for code-level reductions)
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5); Batch 4 closures applied 2026-08-19

### CRITICAL Gaps — Current Status (post-Batch 4)
- **Closed (code fixed)**: `prompt_injection`, `multiplication_overflow` (circuit breaker backoff), `missing_dtor` ×3, `iterator_invalidation` (jit_aggregation), `model_integrity_gap`, `no_transit_encryption` ×4
- **Closed (false positive / stale)**: `braces_imbalance` ×4 (all files depth=0 verified), `iterator_invalidation` automl:325 + olap:543 (stale line refs, code safe)
- **Noted / architecture-delegated**: `db_connection_leak` ×3 in streaming_window (connection ownership delegated to caller via ConnectionGuard RAII contract)
- **Remaining CRITICAL code-fixable**: 0

### Gap Closure Update (2026-08-19 — Batch 4)

- [x] Closed: `model_integrity_gap` for model import path by adding SHA-256 verification API and fail-closed enforcement toggle in `model_serving`.
- [x] Closed: `no_transit_encryption` defaults in TF serving path by switching to HTTPS default and explicit insecure transport opt-in.
- [x] Closed: `unvalidated_llm_output` hardening for fraud/5R/prediction tasks with strict schema/type/range/bounds validation.
- [x] Closed: `prompt_injection` in `llm_process_analyzer.cpp` — added `sanitizeUserContent()` helper that strips control characters (0x00–0x1F except \t/\n/\r), truncates oversized content (>32 KiB flood guard), and case-insensitively redacts known injection prefixes ("System:", "Ignore all", "### instruction", etc.). Applied to ALL data embeddings in `generatePrompt()` across all four task types. Logged via `spdlog::warn` when injection attempt is detected.
- [x] Closed: `multiplication_overflow` in `distributed_analytics.cpp` — the circuit breaker exponential backoff `config_.circuit_breaker_recovery_delay_ms * (1U << cb_info.recovery_attempts)` was UB when `recovery_attempts ≥ 32`. Fixed by capping the shift to `min(recovery_attempts, 30U)`.
- [x] Closed: `missing_dtor` in `anomaly_detection.cpp:233,241` — `~IFNode() = default;` and `~ITree() = default;` (previously closed in Phase 2 A-2).
- [x] Closed: `missing_dtor` in `forecasting.cpp:484` — `~HoltWintersParams() = default;` (previously closed in Phase 2 A-2).
- [x] Closed: `iterator_invalidation` in `jit_aggregation.cpp:309` — post-emplace re-fetch (previously closed in Phase 2 A-2).
- [x] Noted: `braces_imbalance` for `anomaly_detection.cpp`, `automl.cpp`, `cep_engine.cpp`, `distributed_analytics.cpp` — verified FALSE POSITIVE; all files have depth=0 brace balance.
- [x] Noted: `iterator_invalidation` for `automl.cpp:325` and `olap.cpp:543` — stale scanner line references; current code at those locations is safe (LabelEncoder::decode, std::map operator[] in separate iteration phase).
- [x] Noted: `db_connection_leak` for `streaming_window.cpp:441,523,687` — connection ownership contract documented with RAII boundary comments; actual connection management is delegated to the caller via `ConnectionGuard` per the existing architecture contract.
- [x] Phase 3 ROADMAP items closed: fail-closed verified for Arrow IPC/Parquet/Feather/Flight, consistent `spdlog::debug` late-record diagnostics added to SlidingWindow, SessionWindow, HoppingWindow.

**Batch 3 Wave Correlation (2026-08-14):**
- **Wave B Gaps** (~300 IMPL gaps): OLAP optimization, streaming-join backpressure, model-serving circuit-breaker enhancement, distributed merge diagnostics
- **Wave B DOC Gaps** (~200): Performance tuning guide, failure-mode runbook, operator dashboard documentation
- **Other IMPL Gaps** (~200): Iterator-invalidation fixes, O(N²) complexity reductions, transit-encryption enforcement
- **Other DOC Gaps** (~2,800): Inline comments, algorithm documentation, integration notes

**Production Readiness Status (Batch 3 verified 2026-08-14):**
- [x] Streaming window runtime limits implemented and tested (max_open_windows, eviction tracking)
- [x] Distributed analytics circuit breaker pattern with state machine and recovery
- [x] Model serving integration with fault-tolerance and degradation
- [~] Cross-cluster federated analytics hardening (Wave B target: Q4 2026)

### By Severity

- **CRITICAL**: 35
- **HIGH**: 412
- **MEDIUM**: 3247
- **LOW**: 2

### By Type

- allocation_loop: 1
- arithmetic_overflow: 1
- blocking_no_timeout: 1
- braces_imbalance: 9
- braces_imbalance_midfile: 201
- circular_lock_ordering: 14
- copy_overhead: 34
- data_race: 1
- db_connection_leak: 20
- duplicate_qualified_signature: 2
- exception_in_destructor: 1
- generic_catch: 11
- hardcoded_path: 19
- iterator_invalidation: 7
- legacy_or_compat_path: 4
- lock_contention: 9
- manual_cleanup: 1
- memory_order: 1
- missing_dtor: 3
- missing_noexcept_on_move: 11
- missing_override_keyword: 3
- missing_volatile: 64
- model_integrity_gap: 1
- module_doc_linkset_drift: 2
- multiplication_overflow: 2
- no_retry_logic: 1
- no_timeout: 1
- no_transit_encryption: 4
- o_n_squared: 38
- plaintext_transmission: 5
- pointer_arithmetic_unbounded: 14
- prompt_injection: 1
- range_temporary: 6
- repeated_search: 2
- scope_mismatch: 3028
- sensitive_data_logging: 5
- silent_error_swallow: 11
- size_assumption: 12
- stale_doc_section_reference: 4
- string_concat_loop: 18
- todo_as_productionlogic: 58
- uncaught_exception: 12
- unchecked_result: 14
- uninitialized_access: 27
- uninitialized_array: 6
- uninitialized_variable: 5
- unvalidated_llm_output: 1

## Top 20 Gaps

- [braces_imbalance] anomaly_detection.cpp:1 (CRITICAL)
- [braces_imbalance] automl.cpp:1 (CRITICAL)
- [braces_imbalance] cep_engine.cpp:1 (CRITICAL)
- [braces_imbalance] distributed_analytics.cpp:1 (CRITICAL)
- [prompt_injection] llm_process_analyzer.cpp:181 (CRITICAL)
- [missing_dtor] anomaly_detection.cpp:233 (CRITICAL)
- [missing_dtor] anomaly_detection.cpp:241 (CRITICAL)
- [multiplication_overflow] distributed_analytics.cpp:273 (CRITICAL)
- [iterator_invalidation] jit_aggregation.cpp:309 (CRITICAL)
- [iterator_invalidation] automl.cpp:325 (CRITICAL)
- [model_integrity_gap] model_serving.cpp:422 (CRITICAL)
- [db_connection_leak] streaming_window.cpp:441 (CRITICAL)
- [no_transit_encryption] ml_serving.cpp:481 (CRITICAL)
- [no_transit_encryption] ml_serving.cpp:482 (CRITICAL)
- [no_transit_encryption] ml_serving.cpp:483 (CRITICAL)
- [missing_dtor] forecasting.cpp:484 (CRITICAL)
- [no_transit_encryption] ml_serving.cpp:484 (CRITICAL)
- [db_connection_leak] streaming_window.cpp:523 (CRITICAL)
- [iterator_invalidation] olap.cpp:543 (CRITICAL)
- [db_connection_leak] streaming_window.cpp:687 (CRITICAL)

... and 3676 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
