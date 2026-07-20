# AQL Module Testing Coverage

<!--
  ThemisDB | File: TESTING_COVERAGE.md | Version: 0.0.1
  Author: copilot-swe-agent[bot] | Phase 5: Unified Testing Documentation
  Updated: 2026-07-20
-->

## Overview

This document tracks all test cases for the AQL module across Phases 4-5 of
Issue #5628 (AQL Error Handling & Unified Testing). Tests are self-contained
and require only GTest + `include/aql/aql_error_types.h`.

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
| Validation error taxonomy | 14 | `include/aql/aql_error_types.h` |
| Translation recovery | 8  | `src/aql/llm_aql_handler.cpp` |
| Bridge/context degradation | 7 | `src/aql/llm_aql_embedding_bridge.cpp` |
| Schema edge cases | 6  | `include/aql/aql_error_types.h` |
| Concurrency safety | 8  | Thread-safe context patterns |
| Provider degradation | 8  | Mock provider failure modes |
| Token policy | 6  | Token budget enforcement |
| Circuit breaker | 6  | State machine transitions |
| **Total Phase 4-5** | **63** | |

---

## CMake Integration

All `test_aql_*.cpp` files in `tests/aql/` are auto-discovered by the
`tests/aql/CMakeLists.txt` glob pattern and registered as
`module_aql_*_focused` targets. No CMakeLists.txt edits required for
new test files.

---

## Known Gaps / Planned Extensions

- Integration tests with real `LLMAQLHandler` (requires provider mock injection)
- Fuzz tests for injection detection edge cases
- Mutation testing for circuit breaker state transitions
- Performance regression tests for schema validation at scale (Phase 6)
