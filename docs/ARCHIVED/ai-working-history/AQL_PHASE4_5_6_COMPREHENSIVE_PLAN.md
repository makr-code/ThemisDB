# AQL Module — Phases 4-6 Comprehensive Implementation Plan

**Status:** 🚀 PLANNING → EXECUTION  
**Timeline:** 2026-07-19 → 2027-Q1  
**Target Release:** v1.7.0 (Phase 4-5) → v1.8.0 (Phase 6)

---

## Executive Summary

All three remaining phases (4-6) will be implemented sequentially to complete the AQL module hardening roadmap:

- **Phase 4 (Q4 2026):** Error Handling & Edge Cases - Unified error taxonomy + fail-closed behavior standardization
- **Phase 5 (Q4 2026):** Comprehensive Test Suite - Concurrency, degraded-mode, policy-edge coverage + deterministic fixtures
- **Phase 6 (Q1 2027):** Performance & Benchmarking - Release gates + p95/p99 validation

---

## Phase 4: Error Handling and Edge Cases (Target: Q4 2026)

### Scope
Standardize fail-closed behavior, unify error taxonomy, and ensure consistent diagnostics across all assistance components.

### Work Blocks

#### Block 4.1: Error Taxonomy Definition (1 week)
- **Task 4.1.1:** Define canonical error types across validation, translation, bridge, and provider surfaces
  - [ ] Create `include/aql/aql_error_types.h` with enum classes:
    - `ValidationError` (MalformedAQL, InjectionAttempt, SchemaMismatch, UnsupportedOperator)
    - `TranslationError` (GenerationFailed, RetryExhausted, ContextOverflow, ProviderUnavailable)
    - `BridgeError` (ExecutionFailed, TimeoutExceeded, InvalidSchema, ResourceExhausted)
    - `ProviderError` (InferFailed, RAGFailed, EmbedFailed, FinetuneFailed)
  - [ ] Add structured error context (operation_type, component, timestamp, user_id, diagnostic_hint)
  - [ ] Update ROADMAP.md Phase 4.1 status

- **Task 4.1.2:** Document error recovery paths per error type
  - [ ] Create `src/aql/ERROR_RECOVERY_MATRIX.md` mapping error → recovery action
  - [ ] Define explicit fail-closed vs fail-open decisions per operation type
  - [ ] Add recovery-attempt limits and backoff strategies

#### Block 4.2: Validation Component Hardening (1.5 weeks)
- **Task 4.2.1:** Standardize error handling in `llm_aql_handler.cpp::validateAQLWithParser()`
  - [ ] Replace generic error strings with structured error types
  - [ ] Add detailed error context (line number, token position, schema field involved)
  - [ ] Implement diagnostic hints for production triage (e.g., "field 'age' missing from schema")
  - [ ] Add metrics for error type distribution (Prometheus)

- **Task 4.2.2:** Harden edge cases in schema validation
  - [ ] Handle missing collection metadata gracefully (fail-closed with diagnostic)
  - [ ] Handle null/empty schema context (explicit error, not silent)
  - [ ] Handle type mismatches in field definitions (detect + report)
  - [ ] Test with property-based fuzzing (empty schema, null fields, malformed metadata)

#### Block 4.3: Translation Pipeline Error Handling (1.5 weeks)
- **Task 4.3.1:** Standardize error handling in `translateNLToAQL()` path
  - [ ] Wrap LLM generation with structured error context
  - [ ] Replace current "validation failed" with specific error reason (schema mismatch, unsupported syntax)
  - [ ] Implement retry-on-error with detailed logging per retry (attempt count, error reason)
  - [ ] Add circuit breaker error propagation (fail-closed when breaker trips)

- **Task 4.3.2:** Standardize provider unavailability handling
  - [ ] Document explicit fallback behavior (e.g., return error or degrade to simpler query)
  - [ ] Add diagnostics for provider state transitions (available → unavailable → available)
  - [ ] Implement timeout error vs provider-error disambiguation

#### Block 4.4: Bridge and Helper Component Diagnostics (1.5 weeks)
- **Task 4.4.1:** Standardize error handling in `llm_aql_embedding_bridge.cpp`
  - [ ] Add error categorization (generation failure, schema mismatch, execution timeout)
  - [ ] Implement diagnostic context for each error type (what failed, where, why)
  - [ ] Add tracing hooks for high-level debugging (request ID, operation type, timestamp)

- **Task 4.4.2:** Standardize error handling in conversation context
  - [ ] Handle context-overflow errors explicitly (not silent eviction)
  - [ ] Add diagnostic for token budget exhaustion (context size, actual usage)
  - [ ] Implement error recovery (pause conversation, suggest context reset)

### Acceptance Criteria
- ✅ All error types defined and documented in `aql_error_types.h`
- ✅ Error recovery matrix completed with explicit fail-closed decisions per type
- ✅ All assistance components (validation, translation, bridge, conversation) use structured error types
- ✅ Diagnostic hints included in 100% of error paths (production triage actionable)
- ✅ Metrics added for error distribution and recovery success rates
- ✅ ROADMAP.md Phase 4 updated with implementation references

---

## Phase 5: Tests (Target: Q4 2026)

### Scope
Expand focused regressions for concurrency, degraded-mode, and policy-edge behavior. Introduce deterministic fixtures for provider/schema variability.

### Work Blocks

#### Block 5.1: Concurrency Test Suite (2 weeks)
- **Task 5.1.1:** Multi-turn conversation concurrency tests
  - [ ] Create `tests/aql/test_aql_conversation_concurrency.cpp` with 8 test cases:
    - T5.1.1a: Parallel conversation turns (4 threads, shared context)
    - T5.1.1b: Concurrent circuit breaker state transitions
    - T5.1.1c: Race condition in token budget exhaustion
    - T5.1.1d: Concurrent context eviction and new turn insertion
    - T5.1.1e: Stress test (100 concurrent turns with random delays)
    - T5.1.1f: Deadlock detection (verify no circular lock patterns)
    - T5.1.1g: Memory safety under concurrent access (AddressSanitizer)
    - T5.1.1h: Conversation history consistency under interleaved access
  - [ ] Update `tests/aql/CMakeLists.txt` to register as `module_aql_test_aql_conversation_concurrency_focused`
  - [ ] Verify all tests pass with timeout 120s

- **Task 5.1.2:** Validation pipeline concurrency tests
  - [ ] Create `tests/aql/test_aql_validation_concurrency.cpp` with 6 test cases:
    - T5.1.2a: Concurrent validation calls on same AQL query
    - T5.1.2b: Concurrent parser updates during validation
    - T5.1.2c: Race condition in circuit breaker state during validation
    - T5.1.2d: Memory leak detection during concurrent validation
    - T5.1.2e: Performance degradation under contention (p99 latency benchmark)
    - T5.1.2f: Error propagation correctness under concurrent failures

#### Block 5.2: Degraded-Mode Test Suite (2 weeks)
- **Task 5.2.1:** Provider unavailability degradation tests
  - [ ] Create `tests/aql/test_aql_provider_degradation.cpp` with 8 test cases:
    - T5.2.1a: Infer provider unavailable (should fail-closed with diagnostic)
    - T5.2.1b: RAG provider timeout (should fall back to non-RAG query)
    - T5.2.1c: Embed provider failure (should degrade embedding features)
    - T5.2.1d: Multiple providers unavailable (error recovery priority)
    - T5.2.1e: Provider recovery during operation (state transition handling)
    - T5.2.1f: Circuit breaker activation under sustained provider failures
    - T5.2.1g: Graceful degradation of few-shot examples (fallback to template library)
    - T5.2.1h: Diagnostic accuracy (user-facing error messages are actionable)

- **Task 5.2.2:** Schema unavailability and invalid schema tests
  - [ ] Create `tests/aql/test_aql_schema_degradation.cpp` with 6 test cases:
    - T5.2.2a: Missing collection metadata (should error early, not cascade)
    - T5.2.2b: Incomplete field definitions (should detect and report)
    - T5.2.2c: Invalid type annotations (should fail validation, not cause runtime crashes)
    - T5.2.2d: Null schema context (should not segfault)
    - T5.2.2e: Schema evolution (field added/removed mid-conversation)
    - T5.2.2f: Very large schema (memory efficiency under many collections)

- **Task 5.2.3:** Resource exhaustion tests
  - [ ] Create `tests/aql/test_aql_resource_exhaustion.cpp` with 6 test cases:
    - T5.2.3a: Memory pressure (context-bounded conversation under OOM simulation)
    - T5.2.3b: Token budget exhaustion (conversation history truncation)
    - T5.2.3c: LLM request timeout (should not leak threads)
    - T5.2.3d: Concurrent request storm (load shedding or queueing behavior)
    - T5.2.3e: Circuit breaker open state (requests rejected explicitly)
    - T5.2.3f: Recovery from exhaustion (state reset and resumption)

#### Block 5.3: Policy-Edge Test Suite (1.5 weeks)
- **Task 5.3.1:** Token budget policy enforcement tests
  - [ ] Create `tests/aql/test_aql_token_policy.cpp` with 6 test cases:
    - T5.3.1a: Token budget exactly exhausted (boundary condition)
    - T5.3.1b: Single turn exceeds token budget (should error early)
    - T5.3.1c: Conversation history truncated when exceeding budget (oldest pairs removed first)
    - T5.3.1d: Max-turns limit enforced (conversation capped at configured max)
    - T5.3.1e: Token counting accuracy (verify actual vs reported token usage)
    - T5.3.1f: Policy override behavior (when allowed by admin config)

- **Task 5.3.2:** Circuit breaker policy enforcement tests
  - [ ] Create `tests/aql/test_aql_circuit_breaker_policy.cpp` with 6 test cases:
    - T5.3.2a: Failure threshold triggers open state (error_count >= threshold)
    - T5.3.2b: Success transitions from half-open to closed
    - T5.3.2c: Timeout transitions from half-open to open
    - T5.3.2d: Per-operation-type isolation (one operation fails, others still available)
    - T5.3.2e: Half-open state allows limited requests (success_threshold met)
    - T5.3.2f: Explicit reset behavior (admin can manually reset breaker state)

#### Block 5.4: Deterministic Fixture Coverage (1.5 weeks)
- **Task 5.4.1:** Create mock provider fixtures
  - [ ] Create `tests/aql/fixtures/mock_provider_factory.h` with:
    - MockInferProvider (configurable success/failure, latency, response pattern)
    - MockRAGProvider (configurable relevance scores, document results)
    - MockEmbedProvider (deterministic embeddings based on input hash)
    - MockFinetuneProvider (immediate completion, status tracking)
  - [ ] Implement provider-state-machine for simulating provider failures
  - [ ] Add failure injection support (fail_after_n_requests, fail_on_pattern)

- **Task 5.4.2:** Create schema-context fixtures
  - [ ] Create `tests/aql/fixtures/schema_context_builder.h` with:
    - Builder pattern for dynamic schema construction
    - Preset schemas (users, products, orders, complex_nested)
    - Invalid schema variants (missing fields, type mismatches)
    - Large schema fixtures (100+ collections, 1000+ fields)
  - [ ] Implement schema-mutation helpers (add/remove/modify field on-the-fly)

- **Task 5.4.3:** Create conversation-history fixtures
  - [ ] Create `tests/aql/fixtures/conversation_history_builder.h` with:
    - Builder for realistic multi-turn conversations
    - Edge cases (null turns, very long turns, many short turns)
    - Token budget variance (10%, 50%, 90% of budget used)
  - [ ] Implement history-mutation helpers (truncate, insert, replace turns)

### Acceptance Criteria
- ✅ 34 new focused test cases across concurrency, degradation, policy, and fixtures
- ✅ All tests pass (0 failures, 0 timeouts) with timeout 120s per test
- ✅ AddressSanitizer/ThreadSanitizer green (no memory leaks, no race conditions)
- ✅ CMake integration verified (all targets registered correctly)
- ✅ Test suite documented in `src/aql/TESTING_COVERAGE.md`
- ✅ ROADMAP.md Phase 5 updated with test references

---

## Phase 6: Performance and Benchmarking (Target: Q1 2027)

### Scope
Lock benchmark-backed release gates and validate p95/p99 behavior against baselines under representative workloads.

### Work Blocks

#### Block 6.1: Benchmark Baseline Establishment (1.5 weeks)
- **Task 6.1.1:** Establish translation path performance baseline
  - [ ] Create `benchmarks/aql/bench_translation_baseline.cpp` with:
    - `BM_TranslationSimple` (10-word NL query, straightforward AQL)
    - `BM_TranslationComplex` (50-word NL query, complex filters/joins)
    - `BM_TranslationEdgeCase` (malformed NL, requires retry)
    - `BM_TranslationVariance` (measure p50, p90, p95, p99)
  - [ ] Run baseline on reference hardware (Linux x64, Release build)
  - [ ] Record baseline metrics in `benchmarks/aql/baseline_translation.json`

- **Task 6.1.2:** Establish helper path performance baselines
  - [ ] Create `benchmarks/aql/bench_highlighter_scorer_fewshot.cpp` with:
    - `BM_HighlighterSimple` (small result set, few matches)
    - `BM_HighlighterLarge` (large result set, many matches)
    - `BM_ScorerSimple` (score 5 queries)
    - `BM_ScorerLarge` (score 100 queries)
    - `BM_FeWShotLibraryLookup` (retrieve examples by pattern)
  - [ ] Record baseline metrics in `benchmarks/aql/baseline_helpers.json`

#### Block 6.2: Release-Gate Definition (1 week)
- **Task 6.2.1:** Define release gates with hard SLAs
  - [ ] Create `benchmarks/aql/release_gates_aql.json` with:
    - GATE-AQL-01: Translation latency p95 ≤ 150ms (simple), ≤ 500ms (complex)
    - GATE-AQL-02: Translation latency p99 ≤ 300ms (simple), ≤ 1000ms (complex)
    - GATE-AQL-03: Highlighter latency p95 ≤ 50ms
    - GATE-AQL-04: Scorer latency p95 ≤ 100ms
    - GATE-AQL-05: FeWShotLibrary lookup p95 ≤ 10ms
    - GATE-AQL-06: Memory usage under sustained load ≤ 500MB (conversation context)
  - [ ] Document gate definitions and rationale in `src/aql/PERFORMANCE_EXPECTATIONS.md`

- **Task 6.2.2:** Implement gate-validation automation
  - [ ] Create `benchmarks/aql/validate_release_gates.py` script
  - [ ] Integrate with CI/CD (check gates on every release branch merge)
  - [ ] Add gate-failure notifications (email, GitHub check status)

#### Block 6.3: Representative Workload Benchmarking (2 weeks)
- **Task 6.3.1:** Create realistic workload profiles
  - [ ] Workload W1: Light translation load (10 req/sec, p50 query complexity)
    - [ ] Benchmark for 1 hour, capture p95/p99 variance
    - [ ] Validate against GATE-AQL-01, GATE-AQL-02
  - [ ] Workload W2: Medium mixed load (50 req/sec, mix of translation/highlighter/scorer)
    - [ ] Sustained load test, memory pressure monitoring
    - [ ] Validate against all gates
  - [ ] Workload W3: High load + degradation (100 req/sec, simulated provider failures)
    - [ ] Verify circuit breaker doesn't cascade failures
    - [ ] Validate graceful degradation (latency spike < 2x baseline)

- **Task 6.3.2:** Implement workload execution and reporting
  - [ ] Create `benchmarks/aql/run_representative_workloads.py` script
  - [ ] Generate detailed report: baseline vs current, gate status, variance analysis
  - [ ] Create `benchmarks/aql/PERFORMANCE_REPORT_v1.0.md` template for release sign-off

#### Block 6.4: Performance Documentation (1 week)
- **Task 6.4.1:** Document performance expectations
  - [ ] Update `src/aql/ROADMAP.md` Phase 6 with:
    - Release-gate definitions and rationale
    - Workload profiles and expected performance
    - Hardware requirements for baseline
  - [ ] Create `src/aql/PERFORMANCE_EXPECTATIONS.md` with:
    - Expected p95/p99 profiles per operation
    - Performance degradation expectations (provider unavailable, schema mismatch)
    - Memory expectations (conversation context, batch operations)

- **Task 6.4.2:** Create release-sign-off checklist
  - [ ] Create `benchmarks/aql/RELEASE_SIGN_OFF_v1.0.md` with:
    - Pre-release gate validation (all gates PASS)
    - Workload performance validation (variance acceptable)
    - Known regressions documented (if any)
    - Reviewer sign-off line

### Acceptance Criteria
- ✅ All 6 release gates defined with hard SLAs in `release_gates_aql.json`
- ✅ Performance baselines established for translation, highlighter, scorer, FeWShotLibrary
- ✅ Representative workloads (W1, W2, W3) execute successfully with variance < 15%
- ✅ All gates PASS on v1.7.0-rc1 baseline
- ✅ Gate-validation automation integrated in CI/CD
- ✅ Performance documentation complete and reviewed
- ✅ ROADMAP.md Phase 6 updated with all metrics and sign-off status

---

## Integration and Rollout Timeline

### Phase 4 Execution (Weeks 1-4, Q4 2026)
- Week 1: Error taxonomy + recovery matrix
- Week 2: Validation component hardening (parallel: translation pipeline)
- Week 3: Bridge/helper diagnostics (parallel: provider unavailability handling)
- Week 4: Integration testing + ROADMAP update

### Phase 5 Execution (Weeks 5-8, Q4 2026)
- Week 5: Concurrency test suite (parallel: deterministic fixtures design)
- Week 6: Degradation test suite (parallel: fixtures implementation)
- Week 7: Policy-edge test suite + fixture coverage
- Week 8: Full test suite validation + documentation

### Phase 6 Execution (Weeks 1-4, Q1 2027)
- Week 1: Baseline establishment + gate definition
- Week 2: Workload profiling + gate-validation automation
- Week 3: Representative workload benchmarking
- Week 4: Performance documentation + release sign-off

---

## Risk Mitigation

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|-----------|
| Phase 4 error taxonomy changes break existing error handling | Medium | High | Design error type compatibility layer; validate with existing tests |
| Phase 5 concurrency tests reveal race conditions in production code | High | High | Fix races immediately; use AddressSanitizer in CI |
| Phase 6 benchmarks show perf regressions | Medium | High | Establish baseline early; profile bottlenecks weekly |
| Timeline slip due to provider integration dependencies | Medium | Medium | Use mock fixtures to decouple provider availability |

---

## Success Metrics

- ✅ All tasks completed on schedule (8 weeks Phase 4-5, 4 weeks Phase 6)
- ✅ Zero critical bugs discovered post-Phase 6 release
- ✅ All gates PASS on release branch
- ✅ No performance regressions (p95 latency within 10% of baseline)
- ✅ Comprehensive test coverage (100% of error paths, concurrency patterns)
- ✅ Production-ready diagnostics (operators can triage failures independently)

---

## Next Actions

1. ✅ Confirm Phase 4-6 execution (user approval needed)
2. ⏳ Create detailed task cards for Phase 4 Block 4.1
3. ⏳ Set up benchmark infrastructure for Phase 6
4. ⏳ Create GitHub issues for phase tracking (one issue per block)
