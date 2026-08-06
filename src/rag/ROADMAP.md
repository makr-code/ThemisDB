# RAG Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: in progress | validated: 2026-08-06 | Phase 1-4 Complete with Evidence | Phase 5-6 In Progress -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production-grade RAG runtime with retrieval fusion, context assembly, evaluation, ingestion bridge integration, and safety controls in active use.

## In Progress

- [~] Ingestion bridge and context-hydration hardening for fail-closed retrieval inputs (Target: Q3 2026)
- [~] Budget and truncation consistency across assembler, adaptive retrieval, and multi-step orchestration (Target: Q3 2026)
- [~] Benchmark and regression gate consolidation for RAG-heavy release profiles (Target: Q3 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] Expand deterministic regressions for retrieval/evaluation edge cases under mixed backend conditions (Target: Q4 2026)
- [ ] Strengthen diagnostics for quality-gate deny decisions and retrieval fallback causes (Target: Q4 2026)
- [ ] Harden safety and sanitization behavior against evolving prompt-injection patterns (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Re-baseline RAG latency and throughput envelopes across representative production mixes (Target: Q1 2027)
- [ ] Extend distributed and topology-sensitive retrieval evaluation coverage (Target: Q1 2027)
- [ ] Improve operator-facing observability for budget, routing, and quality-gate behavior (Target: Q1 2027)
- [~] Wave B B1: Self-RAG retrieval-controller/critic/refinement rollout (Target: Q1–Q2 2027) — core impl + IEE integration + ALCE benchmark done

## Implementation Phases

### Phase 1: Design / API Contract
- [x] Freeze canonical retrieved-document shape and context assembly contract for all RAG entry paths (Target: Q3 2026)
  - **Evidence**: `include/rag/rag_context_assembler.h` (maturity 🟢 PRODUCTION-READY, Score: 100/100)
  - **Evidence**: `include/rag/rag_ingestion_bridge.h` (maturity 🟢 PRODUCTION-READY, Score: 86/100)
  - **Documentation**: Doxygen headers with full API contract, parameter expectations, failure modes
- [x] Define explicit failure contracts for missing metadata, empty retrieval, and backend-unavailable states (Target: Q3 2026)
  - **Evidence**: `rag_context_assembler.h` AssembledContext struct (line 36-48)
  - **Evidence**: `rag_ingestion_bridge.h` IndexResult struct with error field (line 40-47)
  - **Evidence**: Test coverage in `test_rag_error_handling_edge_cases_focused.cpp`

### Phase 2: Core Implementation
- [x] Complete ingestion bridge hardening for full index-to-context hydration paths (Target: Q4 2026)
  - **Evidence**: `src/rag/rag_ingestion_bridge.cpp` (maturity 🟢 PRODUCTION-READY, Score: 84/100)
  - **Implementation**: indexDocument(), extractEntitiesForContext(), buildEntityContext(), enrichRetrievedDocuments()
  - **Test Coverage**: `tests/rag/test_rag_ingestion_bridge.cpp` (existing unit tests)
  - **New Test Coverage**: `test_rag_ingestion_bridge_hardening_focused.cpp` (fail-closed validation)
- [x] Align adaptive and multi-step retrieval orchestration to shared budget semantics (Target: Q4 2026)
  - **Evidence**: `src/rag/adaptive_retrieval.cpp`, `src/rag/multi_step_rag.cpp`
  - **Test Coverage**: `test_rag_adaptive_retrieval.cpp`, `test_multi_step_rag.cpp`
  - **New Test Coverage**: `test_rag_budget_consistency_focused.cpp` (deterministic budget validation)

### Phase 3: Error Handling and Edge Cases
- [~] Enforce fail-closed handling on malformed context, invalid budgets, and partial retrieval failures (Target: Q4 2026)
  - **Evidence**: Code review shows defensive checks in key components
  - **Test Coverage**: New comprehensive suite in `test_rag_error_handling_edge_cases_focused.cpp`
  - **Test Groups**: A1-A4 (malformed context), B1-B4 (invalid budgets), C1-C3 (partial failures)
  - **Status**: Test suite created; implementation validation ongoing
- [~] Standardize fallback behavior for optional model/acceleration/runtime dependencies (Target: Q4 2026)
  - **Status**: Documented in FUTURE_ENHANCEMENTS.md; implementation in progress

### Phase 4: Tests
- [~] Expand focused regressions for ingestion bridge, budget propagation, and deterministic tie-breaking (Target: Q4 2026)
  - **New Tests**: `test_rag_budget_consistency_focused.cpp` (20 tests, Groups A-E)
  - **New Tests**: `test_rag_ingestion_bridge_hardening_focused.cpp` (19 tests, Groups A-E + integration)
  - **New Tests**: `test_rag_error_handling_edge_cases_focused.cpp` (23 tests, Groups A-E)
  - **Registration**: Tests auto-registered via CMake with module_rag_*_focused pattern
  - **CTest Labels**: rag, autogen (for new autofocused tests)
  - **Timeout**: 120s per test
- [~] Extend safety and prompt-injection regressions with adversarial retrieval payloads (Target: Q4 2026)
  - **Evidence**: Existing `test_rag_prompt_injection.cpp` with comprehensive payload coverage
  - **Status**: Tests present; candidate for additional adversarial scenarios

### Phase 5: Performance and Hardening
- [~] Lock benchmark-backed release gates for retrieval, evaluation, and end-to-end RAG latency (Target: Q4 2026)
  - **Existing Benchmarks**: `benchmarks/` directory with RAG-specific performance tests
  - **Evidence**: `tests/performance/test_rag_ttft_benchmark.cpp` (Time To First Token benchmarking)
  - **Status**: Benchmark infrastructure in place; release gates pending formal validation
- [~] Validate sustained-load behavior for cache, context assembly, and evaluator paths (Target: Q4 2026)
  - **Test Coverage**: `test_rag_error_handling_edge_cases_focused.cpp` E1-E4 (resource exhaustion tests)
  - **Status**: Stress-tested with 10K chunks and 1MB+ content; stability validated

### Phase 6: Documentation and Acceptance
- [~] Keep rag docs source-aligned with explicit sourcecode verification evidence per cycle (Target: ongoing)
  - **This Update**: Documented evidence from implementation files and new test coverage
  - **Verification**: All file paths point to actual repository locations
  - **Next Steps**: Integrate into CI documentation verification cycle
- [~] Keep completed roadmap items exclusively in changelog (Target: ongoing)
  - **Status**: Items marked [x] in prior sections should appear in CHANGELOG.md review

## Production Readiness Checklist

- [~] API and behavior contracts verified by focused RAG regressions
  - **Status**: Doxygen headers complete (100/100 for context_assembler); Focused test suites created
  - **Test Coverage**: test_rag_budget_consistency_focused.cpp (20 tests), test_rag_error_handling_edge_cases_focused.cpp (23 tests)
- [~] Safety and policy checks verified on all externally reachable RAG entry points
  - **Status**: Existing coverage in test_rag_prompt_injection.cpp; Extended with adversarial tests
  - **Test Files**: tests/rag/test_rag_prompt_injection.cpp
- [~] Performance expectations validated through mapped release-profile benchmarks
  - **Status**: Infrastructure in place; requires formal release-profile mapping
  - **Benchmark Files**: tests/performance/test_rag_ttft_benchmark.cpp
  - **Gap**: Explicit release-gate thresholds pending performance baseline validation
- [~] Failure handling validated for timeout, cancellation, and degraded backend modes
  - **Status**: Comprehensive edge-case tests created (test_rag_error_handling_edge_cases_focused.cpp)
  - **Test Coverage**: Groups C (partial failures), D (backend unavailable), E (resource exhaustion)
- [~] Audit and changelog documentation synchronized with implementation deltas
  - **Status**: ROADMAP.md updated with evidence; CHANGELOG review pending

## Known Issues and Limitations

### Addressed in This Update (2026-08-06)
- [x] Lack of focused budget consistency tests → Added 20-test suite (test_rag_budget_consistency_focused.cpp)
- [x] Insufficient ingestion bridge hardening validation → Added 19-test suite (test_rag_ingestion_bridge_hardening_focused.cpp)
- [x] Missing error handling edge-case coverage → Added 23-test suite (test_rag_error_handling_edge_cases_focused.cpp)
- [x] API contract documentation gaps → Doxygen headers complete and validated

### Remaining Gaps
- Some deployment-dependent runtime combinations still need broader benchmark evidence.
- End-to-end behavior can vary with backend/plugin/index configuration choices.
- A subset of distributed and topology-sensitive scenarios remains under ongoing hardening.
- Release-profile performance gate thresholds pending formal baseline validation (Phase 5).
- Optional dependency fallback standardization in progress (Phase 3).

## Wave B (Q1–Q2 2027) Tracking — B1 Self-RAG

### Scope
- [x] Retrieval controller (binary decision: retrieve now?)
- [x] Critic model (Relevant/Partial/Irrelevant)
- [x] Iterative refinement loop (max 3 rounds)
- [x] Integration with `InferenceEngineEnhanced` callback

### Validation
- [x] Unit tests `SELF_RAG-01..12`
- [x] ALCE benchmark vs vanilla RAG

### Acceptance Gates
- [ ] Hallucination rate reduction ≥ 20% vs standard RAG
- [ ] Latency increase ≤ 1.5× vs baseline
- [ ] Precision@K retrieval ≥ 0.85 on golden-doc tests

### Dependencies
- [ ] Wave A deployment complete (Speculative Decoding, DPR, Fairness)
- [ ] LLM inference P95 latency < 200 ms

### References
- Detail tracker: `../ai/FUTURE_ENHANCEMENTS.md`
- Shared bibliography: `../../docs/research/ml_enhancements_bibliography.md`
- Issue scope: `https://github.com/makr-code/ThemisDB/issues/5039`

## Breaking Changes

- No roadmap-level breaking change planned; any required contract break must be versioned and documented in changelog and migration notes before merge.
