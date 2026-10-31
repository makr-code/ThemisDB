---
Author: "ThemisDB Team"
Created: "2026-07-20"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Module Testing Coverage

<!--
  Historical phase-4/5 case catalog, originally assembled 2026-07-20.
  File existence and registration have since been checked against current
  tests/aql/ and tests/aql/CMakeLists.txt; this is not a test execution report.
-->

## Overview

This document catalogs focused regression cases for selected AQL components.
The inventory below is not an exhaustive catalog of all AQL-related tests
elsewhere in `tests/`, and listing a test does not mean it was run or passed.

---

## Test File Index

| File | Phase | Coverage Area | Tests |
|------|-------|---------------|-------|
| `test_aql_validation_error_handling.cpp` | 4.1 | Validation error taxonomy | 8 |
| `test_aql_translation_recovery.cpp`      | 4.1 | Translation retry/recovery | 8 |
| `test_aql_bridge_degradation.cpp`        | 4.1 | Bridge/context degradation | 7 |
| `test_aql_schema_edge_cases.cpp`         | 4.2 | Schema context edge cases  | 6 |
| `test_aql_conversation_concurrency.cpp`  | 5   | Thread-safety of context   | 8 |
| `test_aql_provider_degradation.cpp`      | 5   | Provider failure scenarios | 8 |
| `test_aql_token_policy.cpp`              | 5   | Token budget policy        | 6 |
| `test_aql_circuit_breaker_policy.cpp`    | 5   | Circuit breaker state machine | 6 |

**Total Phase 4-5 new test cases: 57**

---

## Phase 4.1 — Error Taxonomy Tests (Block 4.1, 2026-07-19)

### `test_aql_validation_error_handling.cpp` (8 cases)

| Test Name | Coverage | Category |
|-----------|----------|----------|
| `ValidationErrorHandling_MalformedAQL_FailClosed` | Malformed AQL → FAIL_CLOSED | `ValidationError::MalformedAQL` |
| `ValidationErrorHandling_InjectionAttempt_FailClosed` | Injection detection → FAIL_CLOSED | `ValidationError::InjectionAttempt` |
| `ValidationErrorHandling_SchemaMismatch_WithSchemaContext` | Schema field in error context | `ValidationError::SchemaMismatch` |
| `ValidationErrorHandling_UnsupportedOperator_DiagnosticHint` | Unsupported operator hint | `ValidationError::UnsupportedOperator` |
| `ValidationErrorHandling_TypeMismatch_FieldTypeInfo` | Type mismatch field info | `ValidationError::TypeMismatch` |
| `ValidationErrorHandling_NullSchemaContext_FailClosed` | Null schema → FAIL_CLOSED | `ValidationError::NullSchemaContext` |
| `ValidationErrorHandling_MissingFieldMetadata_Hint` | Missing field metadata | `ValidationError::MissingFieldMetadata` |
| `ValidationErrorHandling_FormatForLogging_Completeness` | Log format completeness | All validation categories |

### `test_aql_translation_recovery.cpp` (8 cases)

| Test Name | Coverage | Category |
|-----------|----------|----------|
| `TranslationRecovery_GenerationFailed_RetryWithBackoff` | LLM failure → retry | `TranslationError::GenerationFailed` |
| `TranslationRecovery_RetryExhausted_FailClosed` | Retries exhausted | `TranslationError::RetryExhausted` |
| `TranslationRecovery_ContextOverflow_Degrade` | Context overflow degradation | `TranslationError::ContextOverflow` |
| `TranslationRecovery_ProviderUnavailable_CircuitBreak` | Provider unavailable | `TranslationError::ProviderUnavailable` |
| `TranslationRecovery_TimeoutExceeded_RetryWithBackoff` | Timeout retry | `TranslationError::TimeoutExceeded` |
| `TranslationRecovery_InvalidResponse_RetryWithFeedback` | Invalid LLM response | `TranslationError::InvalidResponse` |
| `TranslationRecovery_RecoveryStrategy_Routing` | Strategy routing logic | All translation categories |
| `TranslationRecovery_ErrorContext_IncludesAttemptNumber` | Retry count in context | `TranslationError::GenerationFailed` |

### `test_aql_bridge_degradation.cpp` (7 cases)

| Test Name | Coverage | Category |
|-----------|----------|----------|
| `AQLBridgeDegradation_EmbeddingFailed_FallbackToKeywords` | Embedding → keyword fallback | `BridgeError::ExecutionFailed` |
| `AQLBridgeDegradation_BridgeTimeout_RetryOnce` | Timeout + retry | `BridgeError::TimeoutExceeded` |
| `AQLBridgeDegradation_ResourceExhausted_ReduceBatchSize` | GPU OOM → batch reduction | `BridgeError::ResourceExhausted` |
| `AQLBridgeDegradation_ContextBoundExceeded_HistoryTruncation` | Context overflow eviction | `BridgeError::ContextBoundExceeded` |
| `AQLBridgeDegradation_MultipleErrors_Precedence` | Multi-error priority | Multiple |
| `AQLBridgeDegradation_ErrorContext_PreservationAcrossFallback` | Error context in fallback | `BridgeError::EmbeddingGenerationFailed` |
| `AQLBridgeDegradation_ConversationContext_MultipleEvictions` | Multi-eviction tracking | `BridgeError::ContextBoundExceeded` |

---

## Phase 4.2 — Schema Edge Cases (Block 4.2, 2026-07-20)

### `test_aql_schema_edge_cases.cpp` (6 cases)

| Test Name | Coverage | Category |
|-----------|----------|----------|
| `SchemaEdgeCases_NullSchemaContextIsRejected` | Null schema → FAIL_CLOSED | `ValidationError::NullSchemaContext` |
| `SchemaEdgeCases_EmptySchemaContextHandledGracefully` | Empty AQL → diagnostic | `ValidationError::MalformedAQL` |
| `SchemaEdgeCases_MissingCollectionMetadataReturnsError` | Unregistered collection | `ValidationError::MissingFieldMetadata` |
| `SchemaEdgeCases_TypeMismatchInFieldDefinitionDetected` | Float vs string mismatch | `ValidationError::TypeMismatch` |
| `SchemaEdgeCases_LargeSchemaHandledEfficiently` | 500-field schema performance | N/A (perf smoke) |
| `SchemaEdgeCases_ErrorContextContainsDiagnosticHint` | Log format with hints | `ValidationError::SchemaMismatch` |

---

## Phase 5 — Unified Testing (2026-07-20)

### `test_aql_conversation_concurrency.cpp` (8 cases)

| Test Name | Coverage |
|-----------|----------|
| `ConversationConcurrency_ParallelContextAccessIsSafe` | 8-thread concurrent addTurn |
| `ConversationConcurrency_ConcurrentCircuitBreakerTransitions` | 6-thread CB state transitions |
| `ConversationConcurrency_TokenBudgetExhaustionRaceCondition` | 2-thread budget exhaustion race |
| `ConversationConcurrency_ConcurrentContextEviction` | 4-thread eviction under tight budget |
| `ConversationConcurrency_HistoryConsistencyUnderInterleavedAccess` | 3W+3R interleaved access |
| `ConversationConcurrency_NoDeadlockInLockOrdering` | Opposite-order lock acquisition |
| `ConversationConcurrency_ConcurrentValidationPipelineCalls` | 8-thread AQLErrorContext creation |
| `ConversationConcurrency_StressTestConcurrentTurns` | 16-thread × 50-turn stress |

### `test_aql_provider_degradation.cpp` (8 cases)

| Test Name | Coverage |
|-----------|----------|
| `ProviderDegradation_InferProviderUnavailableReturnsError` | Infer always-fail |
| `ProviderDegradation_RAGProviderTimeoutFallsBack` | RAG fail → zero-shot |
| `ProviderDegradation_EmbedProviderFailureDegrades` | Embed fail → Jaccard fallback |
| `ProviderDegradation_MultipleProvidersUnavailableErrorPriority` | All 3 providers fail, priority |
| `ProviderDegradation_ProviderRecoveryHandledCorrectly` | FailAfterN tracker |
| `ProviderDegradation_CircuitBreakerActivatesUnderSustainedFailures` | CB trips after sustained failures |
| `ProviderDegradation_FewShotFallbackToTemplateLibrary` | RAG fail → template library |
| `ProviderDegradation_DiagnosticAccuracyUserFacingMessages` | Hint content per category |

### `test_aql_token_policy.cpp` (6 cases)

| Test Name | Coverage |
|-----------|----------|
| `TokenPolicy_BudgetExactlyExhaustedBoundaryCondition` | Exact fit at boundary |
| `TokenPolicy_SingleTurnExceedingBudgetErrors` | Turn > total budget → reject |
| `TokenPolicy_HistoryTruncatedOldestFirst` | FIFO eviction order |
| `TokenPolicy_MaxTurnsLimitEnforced` | max_turns = 3 with 6 additions |
| `TokenPolicy_TokenCountingAccuracy` | currentTokens == sumTokenCounts |
| `TokenPolicy_PolicyBehaviorAtBoundary` | Budget=1, empty strings |

### `test_aql_circuit_breaker_policy.cpp` (6 cases)

| Test Name | Coverage |
|-----------|----------|
| `CircuitBreakerPolicy_FailureThresholdTriggersOpen` | CLOSED → OPEN on threshold |
| `CircuitBreakerPolicy_SuccessTransitionsFromHalfOpenToClosed` | HALF_OPEN → CLOSED on success |
| `CircuitBreakerPolicy_TimeoutTransitionsFromHalfOpenToOpen` | HALF_OPEN → OPEN on failure |
| `CircuitBreakerPolicy_PerOperationTypeIsolation` | Infer/RAG/Embed independence |
| `CircuitBreakerPolicy_HalfOpenAllowsLimitedRequests` | max_requests = 2 in HALF_OPEN |
| `CircuitBreakerPolicy_ExplicitResetBehavior` | forceReset() idempotency |

---

## Fixture Files

| File | Purpose |
|------|---------|
| `tests/aql/fixtures/mock_provider_factory.h` | MockInferProvider, MockRAGProvider, MockEmbedProvider with configurable failure modes |
| `tests/aql/fixtures/schema_context_builder.h` | SchemaContextBuilder with preset schemas (users/products/orders) and invalid variants |

---

## Coverage Summary

| Category | Test Count | Source Files Covered |
|----------|-----------|----------------------|
| Validation error taxonomy | 8 | `include/aql/aql_error_types.h` |
| Translation recovery | 8  | `src/aql/llm_aql_handler.cpp` |
| Bridge/context degradation | 7 | `src/aql/llm_aql_embedding_bridge.cpp` |
| Schema edge cases | 6  | `include/aql/aql_error_types.h` |
| Concurrency safety | 8  | Thread-safe context patterns |
| Provider degradation | 8  | Mock provider failure modes |
| Token policy | 6  | Token budget enforcement |
| Circuit breaker | 6  | State machine transitions |
| **Total Phase 4-5 case catalog** | **57** | |

---

## CMake Integration

`tests/aql/CMakeLists.txt` uses `file(GLOB ...)` for `test_aql_*.cpp`, creates
`module_aql_<test-stem>_focused` executables, and registers the corresponding
CTest entries with the `aql` label and a 120-second timeout. The glob does not
use `CONFIGURE_DEPENDS`; reconfigure CMake after changing the matching file set.
Target creation is conditional on the repository's test configuration and
linkable dependencies. This source-level registration is not evidence that a
target built or that its tests passed.

---

## Known Gaps / Planned Extensions

- **TODO:** Reconcile this historical case catalog with the full current test
  inventory and implementation coverage.
- **TODO:** Verify whether focused fuzz and mutation-testing targets exist
  before describing them as missing; this document has not audited tests outside
  `tests/aql/`.
- Performance targets and measured values are documented separately in
  [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md); the catalog
  here does not establish that they were run.
