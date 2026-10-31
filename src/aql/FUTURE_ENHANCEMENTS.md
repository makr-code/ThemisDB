---
Author: "ThemisDB Team"
Created: "2026-07-19"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Module - Future Enhancements

<!-- Historical status snapshot validated 2026-07-19; not current readiness evidence. -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Historical Implementation Snapshot (v1.6.0 + 2026-07-19)

The checkmarks and status language in this section record a 2026-07-19 planning
snapshot; they are not current production-readiness or test-pass evidence.
Current behavior must be checked against implementation, headers, and tests.
**TODO:** Reconcile remaining backlog items and completion states with the
current module roadmap before using this file as an active delivery plan.

### Items reported as implemented in the historical snapshot

- [x] **Post-Generation AQL Validation in `translateNLToAQL()`** (v1.6.0)
  - AST validation post-generation with configurable error handling modes
  - Injection attempt detection in LLM-generated queries
  - Malformed AQL rejected early based on validation mode (WARN_ONLY, REJECT_ON_ERROR, RETRY_ON_ERROR)
  - Implemented in `llm_aql_handler.cpp` lines 1488-1527
  - Historical reported status: implemented; current validation policy is configurable.

- [~] **Thread Leak Elimination in `LLMTimeoutManager::executeWithTimeout()`** (v1.6.0)
  - Fixed thread creation without proper cleanup using std::jthread
  - Timeout thread properly joins or is detached with cleanup wrapper
  - Documented in `llm_timeout_manager.h` lines 95-122
  - Historical reported status only; implementation/source review is required for a current lifecycle claim.

- [x] **Per-Operation-Type Circuit Breakers** (v1.6.0)
  - Implemented circuit breaker pattern per operation (infer, rag, embed, finetune)
  - Independent failure tracking and state management per operation type
  - Fail-closed when error threshold exceeded
  - Implemented in `llm_aql_handler.cpp` lines 451-458, 1300+
  - Historical reported status only; see `include/aql/llm_aql_handler.h` for current configuration.

- [x] **Bounded Conversation History with Context-Window Budget** (v1.6.0)
  - Sliding window for conversation history with token budget enforcement
  - Configurable max_turns and max_history_tokens limits
  - OOM prevention through automatic eviction of oldest pairs
  - Implemented in `aql_conversation_context.cpp` lines 111-183
  - Historical reported status only; see `include/aql/aql_conversation_context.h` for current bounds.

## Historical Documentation & Roadmap Synchronization Report (Issue #5628, 2026-07-19)

- [x] **Doxygen API Documentation Expansion**
  - All public classes and methods now have @brief + @param + @return/@throws documentation
  - All .cpp and .h files have @file headers with maturity metadata
  - Reported coverage at that time: 33 header files and 34 implementation files; these counts do not match the current directory inventory.
  - Current coverage: **TODO** — regenerate/verify Doxygen coverage from the current source set.

- [x] **Roadmap Synchronization**
  - ROADMAP.md updated with v1.6.0 completion status
  - FUTURE_ENHANCEMENTS.md synchronized with current implementation
  - Phase 2 (Parser Integration) marked as ✅ COMPLETED 2026-07-19
  - Phase 3 (Documentation Consolidation) marked as ✅ COMPLETED 2026-07-19
  - Validation date updated from 2026-06-10 to 2026-07-19
  - Historical report only; it does not establish current synchronization.

## Phase 4: Error Handling and Edge Cases (2026-07-19)

- [x] **Block 4.1: Error Taxonomy Definition — COMPLETED**
  - AQL Error Context Framework: `include/aql/aql_error_types.h`
    - AQLErrorContext class with diagnostic metadata (operation type, line/token position, schema context, retry count, recoverability)
    - Namespaced error categories: ValidationError (7 types), TranslationError (5 types), BridgeError (6 types), ProviderError (7 types)
    - Recovery strategy mapping: FAIL_CLOSED, RETRY_WITH_BACKOFF, DEGRADE_GRACEFULLY, CIRCUIT_BREAK
  - Error Recovery Matrix: `src/aql/ERROR_RECOVERY_MATRIX.md`
    - Detailed recovery specifications per error type (severity, max retries, backoff strategy, diagnostics)
    - Per-component error handling (validation, translation, embedding bridge, conversation context)
    - Metrics and observability recommendations with Prometheus counters and structured logging
  - Unit Test Infrastructure (23 test cases total):
    - `tests/aql/test_aql_validation_error_handling.cpp` (8 tests covering validation error paths)
    - `tests/aql/test_aql_translation_recovery.cpp` (8 tests covering translation recovery with backoff timing)
    - `tests/aql/test_aql_bridge_degradation.cpp` (7 tests covering bridge/context degradation)
  - Status: ✅ PRODUCTION-READY

- [x] **Block 4.2: Validation Component Hardening — COMPLETED**
  - Modify validateAQLWithParser() to use AQLErrorContext instead of generic error strings
  - Add detailed error diagnostics: AST location, token position, schema field, suggested fixes
  - Integrate Prometheus metrics for error type distribution
  - 2026-08-24 follow-up: `LLMValidationPipeline` now fails fast when the LLM client is unavailable and records structured generation/validation metrics.

- [x] **Block 4.3: Translation Pipeline Error Handling — COMPLETED**
  - Enhance translateNLToAQL() with structured retry logic (exponential backoff, max 3 retries)
  - Integrate InvalidResponse retry with error feedback in LLM prompt
  - Add circuit breaker checks before provider calls
  - 2026-08-24 follow-up: all `translateNLToAQL*` retry loops now consume `validation_config.max_retries` instead of hardcoded retry defaults.

- [x] **Block 4.4: Bridge/Helper Component Diagnostics — COMPLETED**
  - Update llm_aql_embedding_bridge.cpp to handle provider failures gracefully
  - Update aql_conversation_context.cpp to handle context bound exhaustion
  - Add error context to all helper components (highlighter, scorer, few-shot library)
  - 2026-08-24 follow-up: `LLMExtractiveCompressor` now has non-placeholder availability checks, deterministic ranking fallback, and warning-level persistence diagnostics.

## Scope

- hardening and refinement of NL-to-AQL and command-assistance flows
- expansion of quality controls across validation/scoring/context/tooling surfaces
- stronger performance and reliability guarantees for high-volume assistance usage

## Design Constraints

- assistance contracts remain backward compatible within major release line.
- generated-query flows remain fail-closed on invalid or unsupported states.
- context and helper components remain bounded under sustained usage.
- optional integrations degrade safely with explicit diagnostics.

## Required Interfaces

| Interface | Requirement |
|---|---|
| translation interfaces | deterministic output/error semantics for generated-query paths |
| validation interfaces | structural and schema-aware issue coverage with consistent severity handling |
| scoring/context interfaces | bounded state and explainable confidence behavior |
| bridge/tooling interfaces | explicit capability checks and fallback semantics |

## Implementation Notes

- continue tightening post-generation validation policy handling.
- standardize diagnostics across translation, scoring, and bridge paths.
- expand deterministic concurrency coverage for conversation and agent workflows.
- reduce benchmark ambiguity by adding focused path-specific performance cases.

## Test Strategy

- unit and integration tests for translation, validation, and bridge edges.
- adversarial-input and degraded-mode regression suites.
- concurrency and bounded-context regression coverage.
- release-profile benchmark validation for mapped targets.

## Performance Targets

- translation-assistance hot paths remain within release regression budgets.
- highlighter/scorer/few-shot paths retain stable p95/p99 profiles versus baseline.
- benchmark manifest completeness reaches no-missing-case status for mapped AQL targets.

## Security / Reliability

- preserve strict input-handling and fail-closed generated-query safety controls.
- maintain bounded context and tool/bridge behavior under load.
- enforce explicit capability checks for optional provider integrations.
- keep diagnostics actionable for production triage and review workflows.