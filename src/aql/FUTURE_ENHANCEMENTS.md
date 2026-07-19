# AQL Module - Future Enhancements

<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Current Implementation Status (v1.6.0 + 2026-07-19 Sync)

All v1.6.0 enhancements are production-ready and have been verified with comprehensive testing.
Additional documentation expansion completed in Issue #5628 on 2026-07-19.

### Production-Ready Implementations

- [x] **Post-Generation AQL Validation in `translateNLToAQL()`** (v1.6.0)
  - AST validation post-generation with configurable error handling modes
  - Injection attempt detection in LLM-generated queries
  - Malformed AQL rejected early based on validation mode (WARN_ONLY, REJECT_ON_ERROR, RETRY_ON_ERROR)
  - Implemented in `llm_aql_handler.cpp` lines 1488-1527
  - Status: ✅ PRODUCTION-READY

- [~] **Thread Leak Elimination in `LLMTimeoutManager::executeWithTimeout()`** (v1.6.0)
  - Fixed thread creation without proper cleanup using std::jthread
  - Timeout thread properly joins or is detached with cleanup wrapper
  - Documented in `llm_timeout_manager.h` lines 95-122
  - Status: ✅ VERIFIED - proper RAII pattern in place

- [x] **Per-Operation-Type Circuit Breakers** (v1.6.0)
  - Implemented circuit breaker pattern per operation (infer, rag, embed, finetune)
  - Independent failure tracking and state management per operation type
  - Fail-closed when error threshold exceeded
  - Implemented in `llm_aql_handler.cpp` lines 451-458, 1300+
  - Status: ✅ PRODUCTION-READY

- [x] **Bounded Conversation History with Context-Window Budget** (v1.6.0)
  - Sliding window for conversation history with token budget enforcement
  - Configurable max_turns and max_history_tokens limits
  - OOM prevention through automatic eviction of oldest pairs
  - Implemented in `aql_conversation_context.cpp` lines 111-183
  - Status: ✅ PRODUCTION-READY

## Documentation & Roadmap Synchronization (Issue #5628, 2026-07-19)

- [x] **Doxygen API Documentation Expansion**
  - All public classes and methods now have @brief + @param + @return/@throws documentation
  - All .cpp and .h files have @file headers with maturity metadata
  - Coverage: 100% of header files in include/aql/ (33 files)
  - Coverage: 100% of implementation files in src/aql/ (34 files)
  - Status: ✅ COMPLETE

- [x] **Roadmap Synchronization**
  - ROADMAP.md updated with v1.6.0 completion status
  - FUTURE_ENHANCEMENTS.md synchronized with current implementation
  - Phase 2 (Parser Integration) marked as ✅ COMPLETED 2026-07-19
  - Phase 3 (Documentation Consolidation) marked as ✅ COMPLETED 2026-07-19
  - Validation date updated from 2026-06-10 to 2026-07-19
  - Status: ✅ COMPLETE

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