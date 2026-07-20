# AQL Error Recovery Matrix — Phase 4 (2026-07-19)

**Purpose:** Define explicit recovery paths and fail-closed/fail-open decisions for all error types in the AQL module.

**Document Status:** PRODUCTION-READY (Phase 4)  
**Last Updated:** 2026-07-19  
**Version:** 1.0  

---

## Executive Summary

This matrix consolidates error recovery strategies across all AQL assistance components:
- **Validation** (schema, syntax, injection detection)
- **Translation** (NL-to-AQL generation, retry logic)
- **Bridge** (embedding, highlighter, scorer, context management)
- **Provider** (LLM inference, RAG, embeddings, fine-tuning)

**Key Principle:** Fail-closed by default; degrade gracefully only when explicitly safe.

---

## Error Type Classification

### 1. Validation Errors

Errors during AQL query validation and schema verification.

| Error Category | Severity | Recovery Strategy | Fail-Closed? | Retry? | Diagnostic Action |
|---|---|---|---|---|---|
| **MalformedAQL** | CRITICAL | FAIL_CLOSED | YES | NO | Return structured error with AST location + token position |
| **InjectionAttempt** | CRITICAL | FAIL_CLOSED | YES | NO | Log security event + return injection-detection diagnostic |
| **SchemaMismatch** | HIGH | FAIL_CLOSED | YES | NO | Return field not found + suggest valid fields from schema |
| **TypeMismatch** | HIGH | FAIL_CLOSED | YES | NO | Return type mismatch + expected type from schema |
| **UnsupportedOperator** | MEDIUM | FAIL_CLOSED | YES | NO | Return unsupported operator + suggest alternatives |
| **NullSchemaContext** | HIGH | FAIL_CLOSED | YES | NO | Request schema context explicitly from caller |
| **MissingFieldMetadata** | MEDIUM | FAIL_CLOSED | YES | NO | Return incomplete metadata diagnostic |

**Implementation:** All validation errors are **non-recoverable** by default. Return structured `AQLErrorContext` with diagnostic hints.

---

### 2. Translation Errors

Errors during NL-to-AQL translation via LLM generation.

| Error Category | Severity | Recovery Strategy | Retry? | Max Retries | Backoff | Diagnostic Action |
|---|---|---|---|---|---|---|
| **GenerationFailed** | HIGH | RETRY_WITH_BACKOFF | YES | 3 | Exponential (100ms, 500ms, 2s) | Log attempt count + underlying LLM error |
| **RetryExhausted** | CRITICAL | PROPAGATE_ERROR | NO | — | — | Return retry exhaustion error + last error reason |
| **ContextOverflow** | MEDIUM | DEGRADE_GRACEFULLY | NO | — | — | Suggest conversation reset or simpler query |
| **ProviderUnavailable** | HIGH | RETRY_WITH_BACKOFF | YES | 2 | Exponential | Check circuit breaker state + retry |
| **TimeoutExceeded** | HIGH | RETRY_WITH_BACKOFF | YES | 1 | Linear (5s) | Log timeout + suggest longer timeout config |
| **InvalidResponse** | MEDIUM | RETRY_WITH_BACKOFF | YES | 2 | Exponential | Include validation error in LLM prompt for retry |

**Implementation:** All translation errors are **recoverable via retry**. Use structured error context to guide retry logic and provide feedback to LLM.

---

### 3. Bridge/Helper Errors

Errors from embedding bridge, highlighter, scorer, and conversation context components.

| Error Category | Severity | Recovery Strategy | Retry? | Max Retries | Degradation | Diagnostic Action |
|---|---|---|---|---|---|---|
| **ExecutionFailed** | MEDIUM | RETRY_WITH_BACKOFF | YES | 1 | If fails: use simpler feature | Log execution error + reason |
| **EmbeddingGenerationFailed** | MEDIUM | DEGRADE_GRACEFULLY | NO | — | Fall back to non-embedding path | Log embedding provider error |
| **InvalidSchema** | HIGH | FAIL_CLOSED | YES | NO | — | Request valid schema context |
| **ResourceExhausted** | MEDIUM | DEGRADE_GRACEFULLY | NO | — | Reduce batch size / context window | Log memory pressure event |
| **TimeoutExceeded** | LOW | RETRY_WITH_BACKOFF | YES | 1 | Defer operation | Log timeout + suggest async path |
| **ContextBoundExceeded** | MEDIUM | DEGRADE_GRACEFULLY | NO | — | Truncate history / suggest reset | Notify user of context bound exhaustion |

**Implementation:** Bridge errors favor **graceful degradation**. Preserve user-facing functionality while logging diagnostics.

---

### 4. Provider Errors

Errors from external providers (LLM, RAG, embeddings, fine-tuning).

| Error Category | Severity | Recovery Strategy | Retry? | Max Retries | Circuit Breaker | Diagnostic Action |
|---|---|---|---|---|---|---|
| **InferFailed** | HIGH | RETRY_WITH_BACKOFF | YES | 3 | Activate after 5 failures | Log provider error + response code |
| **RAGFailed** | MEDIUM | DEGRADE_GRACEFULLY | NO | — | Check breaker state | Fall back to non-RAG generation |
| **EmbedFailed** | MEDIUM | DEGRADE_GRACEFULLY | NO | — | Check breaker state | Fall back to non-embedding features |
| **FinetuneFailed** | LOW | PROPAGATE_ERROR | NO | — | — | Return error to caller + diagnostics |
| **CircuitBreakerOpen** | MEDIUM | DEGRADE_GRACEFULLY | NO | — | Circuit: OPEN | Return explicit error: provider circuit open |
| **ProviderTimeout** | HIGH | RETRY_WITH_BACKOFF | YES | 1 | Activate after 3 timeouts | Log timeout + request ID for tracing |
| **ProviderUnavailable** | HIGH | RETRY_WITH_BACKOFF | YES | 2 | Activate on first unavailable | Check health endpoint + retry |

**Implementation:** Provider errors use **circuit breaker pattern**. Once breaker opens, return DEGRADE_GRACEFULLY errors immediately.

---

## Recovery Path Specifications

### A. FAIL_CLOSED

**When to use:** Invalid input, security issues, data integrity risks.

**Implementation:**
```cpp
AQLErrorContext ctx("validation", ValidationError::InjectionAttempt, 
                    "aql_validator", "Query contains detected injection pattern");
ctx.addDiagnosticHint("Detected suspicious pattern near position " + std::to_string(pos));
ctx.setRecoverable(false);
// Throw exception or return error immediately
```

**User-facing behavior:** Return explicit error with diagnostic; query rejected.

---

### B. RETRY_WITH_BACKOFF

**When to use:** Transient failures, provider timeouts, temporary unavailability.

**Implementation:**
```cpp
AQLErrorContext ctx("translation", TranslationError::GenerationFailed,
                    "llm_handler", "LLM inference request failed");
ctx.setRecoverable(true);
ctx.setRetryCount(attempt);
ctx.addDiagnosticHint("Retry " + std::to_string(attempt) + " of 3; backoff: " 
                      + std::to_string(backoff_ms) + "ms");

// Exponential backoff: 100ms, 500ms, 2000ms
std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
// Retry with same input
```

**User-facing behavior:** Transparent retry; may appear as latency spike; final error if exhausted.

---

### C. DEGRADE_GRACEFULLY

**When to use:** Optional features fail, resource limits, non-critical providers.

**Implementation:**
```cpp
AQLErrorContext ctx("bridge", BridgeError::EmbeddingGenerationFailed,
                    "embedding_bridge", "Embedding generation timeout");
ctx.setRecoverable(true);
ctx.addDiagnosticHint("Embedding unavailable; using keyword-based matching");

// Continue without embedding feature
return generateQueryWithoutEmbeddings();
```

**User-facing behavior:** Reduced functionality; return result without optional feature.

---

### D. CIRCUIT_BREAK

**When to use:** Provider sustained failures; prevent cascading downstream failures.

**Implementation:**
```cpp
sharding::CircuitBreaker& breaker = getCircuitBreakerFor(provider_type);
if (breaker.isClosed()) {
    try {
        // Invoke provider
        result = provider.execute(request);
        breaker.recordSuccess();
    } catch (...) {
        breaker.recordFailure();
        if (breaker.isOpen()) {
            AQLErrorContext ctx("provider", ProviderError::CircuitBreakerOpen,
                              "circuit_breaker", provider_type + " circuit breaker open");
            ctx.addDiagnosticHint("Provider " + provider_type + " experiencing sustained failures");
            // Return error immediately without calling provider
        }
    }
}
```

**User-facing behavior:** Fast-fail with diagnostic; prevent resource waste.

---

## Error Path Validation Checklist

For each error recovery path implemented:

- [ ] Error category defined in `aql_error_types.h`
- [ ] Recovery strategy assigned in `getRecoveryStrategy()`
- [ ] AQLErrorContext created with operation type, component, message, diagnostic hints
- [ ] Metrics/logging instrumented (error type, recovery action, outcome)
- [ ] Unit test created for:
  - [ ] Normal happy path (no error)
  - [ ] Error injection (force error)
  - [ ] Recovery attempt + success
  - [ ] Recovery exhaustion (max retries reached)
  - [ ] Fallback/degradation behavior
- [ ] Integration test created for:
  - [ ] Provider unavailability handling
  - [ ] Circuit breaker state transitions
  - [ ] Cascading error prevention

---

## Per-Component Error Handling

### Validation Component (`validateAQLWithParser`)

**Inputs:** AQL query string, parser service, optional schema context

**Possible Errors:**
- `ValidationError::MalformedAQL` — Parser fails
- `ValidationError::InjectionAttempt` — Regex patterns detected
- `ValidationError::SchemaMismatch` — Referenced collections don't exist
- `ValidationError::TypeMismatch` — Field types don't match operations
- `ValidationError::NullSchemaContext` — Schema required but missing

**Recovery:** FAIL_CLOSED for all errors. Return detailed diagnostic with AST location.

**Implementation Location:** `src/aql/llm_aql_handler.cpp` lines 1554+

---

### Translation Pipeline (`translateNLToAQL` + friends)

**Inputs:** NL query, schema context

**Possible Errors:**
- `ValidationError::NullSchemaContext` → FAIL_CLOSED
- `TranslationError::GenerationFailed` → RETRY_WITH_BACKOFF (3x)
- `TranslationError::TimeoutExceeded` → RETRY_WITH_BACKOFF (1x)
- `TranslationError::InvalidResponse` → RETRY_WITH_BACKOFF (2x) with error feedback in prompt
- `TranslationError::ContextOverflow` → DEGRADE_GRACEFULLY (shorter context window)
- `ProviderError::CircuitBreakerOpen` → DEGRADE_GRACEFULLY (simple template query)

**Recovery:** Structured retry with exponential backoff. Circuit breaker integration.

**Implementation Location:** `src/aql/llm_aql_handler.cpp` lines 1663+

---

### Embedding Bridge (`llm_aql_embedding_bridge.cpp`)

**Inputs:** Query, schema, embedding request

**Possible Errors:**
- `BridgeError::InvalidSchema` → FAIL_CLOSED
- `ProviderError::EmbedFailed` → DEGRADE_GRACEFULLY (use non-embedding path)
- `BridgeError::TimeoutExceeded` → RETRY_WITH_BACKOFF (1x)
- `BridgeError::ContextBoundExceeded` → DEGRADE_GRACEFULLY (reduce batch)

**Recovery:** Non-embedding fallback if provider fails. Reduce batch size on resource exhaustion.

**Implementation Location:** `src/aql/llm_aql_embedding_bridge.cpp` lines TBD

---

### Conversation Context (`aql_conversation_context.cpp`)

**Inputs:** Turn (NL query, AQL result), context config (max_turns, max_tokens)

**Possible Errors:**
- `BridgeError::ContextBoundExceeded` → DEGRADE_GRACEFULLY (truncate history)
- `BridgeError::ResourceExhausted` → DEGRADE_GRACEFULLY (evict oldest turns)

**Recovery:** Automatic history eviction (FIFO when token budget exceeded). Notify user of bound.

**Implementation Location:** `src/aql/aql_conversation_context.cpp` lines TBD

---

## Metrics and Observability

### Error Counters (Prometheus)

```
# Validation errors
aql_validation_errors_total{category="malformed_aql"}
aql_validation_errors_total{category="injection_attempt"}
aql_validation_errors_total{category="schema_mismatch"}

# Translation errors
aql_translation_errors_total{category="generation_failed", retry_count="1"}
aql_translation_errors_total{category="timeout_exceeded", retry_count="1"}
aql_translation_retry_exhausted_total

# Bridge errors
aql_bridge_errors_total{category="context_bound_exceeded"}
aql_bridge_degradation_total{reason="embedding_unavailable"}

# Provider errors + circuit breaker
aql_provider_errors_total{provider_type="infer", error_type="timeout"}
aql_circuit_breaker_state{provider_type="infer", state="open"}
```

### Diagnostics Logging

All errors use `AQLErrorContext::formatForLogging()` for structured logging:

```
[AQLError] Type=validation Category=InjectionAttempt Component=aql_validator Operation=translate_nl_to_aql Line=1 Position=15 SchemaField= Collection=users Hints=[Detected suspicious pattern near position 15] Recoverable=no
```

---

## Testing Strategy

### Unit Tests (new test files in `tests/aql/`)

1. **test_aql_validation_error_handling.cpp** — 8 test cases
   - T4.2.1a: MalformedAQL detection
   - T4.2.1b: InjectionAttempt detection
   - T4.2.1c: SchemaMismatch diagnostic
   - T4.2.1d: TypeMismatch detection
   - T4.2.1e: UnsupportedOperator handling
   - T4.2.1f: NullSchemaContext detection
   - T4.2.1g: MissingFieldMetadata handling
   - T4.2.1h: Error context formatting

2. **test_aql_translation_recovery.cpp** — 8 test cases
   - T4.3.1a: GenerationFailed with retry (success on 2nd attempt)
   - T4.3.1b: GenerationFailed retry exhaustion (3 failures)
   - T4.3.1c: TimeoutExceeded with exponential backoff
   - T4.3.1d: InvalidResponse retry with error feedback in prompt
   - T4.3.1e: ContextOverflow graceful degradation
   - T4.3.1f: CircuitBreakerOpen fast-fail without provider call
   - T4.3.1g: ProviderUnavailable detection
   - T4.3.1h: Retry backoff timing accuracy

3. **test_aql_bridge_degradation.cpp** — 6 test cases
   - T4.4.1a: EmbeddingFailed fallback to non-embedding path
   - T4.4.1b: TimeoutExceeded bridge operation
   - T4.4.1c: ResourceExhausted batch size reduction
   - T4.4.1d: ContextBoundExceeded history truncation
   - T4.4.1e: Multiple errors in bridge (error precedence)
   - T4.4.1f: Error context preservation across fallback

### Integration Tests

1. **test_aql_error_end_to_end.cpp** — 4 scenarios
   - E2E scenario 1: Provider failure → circuit open → subsequent calls fast-fail
   - E2E scenario 2: Schema mismatch in translation → fail-closed with diagnostics
   - E2E scenario 3: Conversation context overflow → graceful truncation
   - E2E scenario 4: Concurrent error handling (race conditions)

---

## Implementation Milestones

### Week 1 (2026-07-22 to 2026-07-26)
- [ ] aql_error_types.h completed + reviewed
- [ ] ERROR_RECOVERY_MATRIX.md completed + reviewed
- [ ] Validation component error handling hardened
- [ ] Initial unit tests created (validation errors)

### Week 2 (2026-07-29 to 2026-08-02)
- [ ] Translation pipeline error handling + retry logic
- [ ] Bridge component error handling
- [ ] Conversation context error handling
- [ ] Unit tests completed (translation + bridge)

### Week 3 (2026-08-05 to 2026-08-09)
- [ ] Integration tests created and passing
- [ ] Metrics instrumentation verified
- [ ] Error handling documentation finalized
- [ ] ROADMAP.md Phase 4 updated

---

## Rollback Plan

If critical issues discovered during Phase 4:

1. Revert to last known good state (git revert HEAD~N)
2. Keep aql_error_types.h (non-breaking additive change)
3. Revert recovery logic changes in llm_aql_handler.cpp and bridge components
4. Keep ERROR_RECOVERY_MATRIX.md for reference
5. Document root cause in ai_working/ for post-mortem

---

## Sign-Off

- **Specification Created:** 2026-07-19
- **Specification Approved:** PENDING
- **Implementation Lead:** copilot-swe-agent
- **Review Status:** PENDING
