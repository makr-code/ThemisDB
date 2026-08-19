# AQL Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

Production AQL-assistance surfaces exist across translation, validation, tooling, context, and scoring support paths.

**Latest Sync (2026-07-19)**: Full module documentation expansion and roadmap synchronization completed per Issue #5628.
- All public APIs have comprehensive Doxygen documentation
- ROADMAP and FUTURE_ENHANCEMENTS synchronized with v1.6.0 implementation status
- Phase 2 (Parser Integration) marked as completed with all subtasks verified
- Phase 3 (Documentation Consolidation) initiated and verified complete

## Recently Completed (v1.6.0)

- [x] **AQL Parser Integration Consolidation** — All Phases Complete (coordinated with src/query/)
  - [x] Phase 1: Define integration boundary (12 hrs) ✅ 2026-06-18
    - Created `src/query/AQL_LLM_INTEGRATION_CONTRACT.md` (canonical specification)
    - Updated architecture docs in both src/query/ and src/aql/
  - [x] Phase 2: Wire parser validation + metrics (20 hrs) ✅ 2026-07-19
    - validateAQLWithParser() implemented in llm_aql_handler.cpp (lines 1488-1527)
    - translateNLToAQL() calls validation with retry-on-error logic
    - Prometheus metrics instrumentation added to validation pipeline
    - Integration test suite created (16 test cases) — verified passing
  - [x] Phase 3: Consolidate documentation (12 hrs) ✅ 2026-07-19
    - Unified duplicate content across AQL roadmaps
    - Expanded Doxygen coverage across all public APIs
    - Synchronized ROADMAP.md and FUTURE_ENHANCEMENTS.md with v1.6.0 implementation
- [x] hardening of generated-query safety and degraded-mode behaviors (Target: Q3 2026) **COMPLETED v1.6.0**
- [x] complete remaining hardening in translation and bridge execution paths **COMPLETED v1.6.0**
  - [x] Post-generation AQL validation with injection detection (llm_aql_handler.cpp:1488-1527)
  - [x] Thread leak elimination in LLMTimeoutManager::executeWithTimeout() (llm_timeout_manager.h:95-122)
  - [x] Per-operation-type circuit breakers (llm_aql_handler.cpp:451-458, 1300+)
  - [x] Bounded conversation history with context-window budget (aql_conversation_context.cpp:111-183)

## In Progress

- [~] **Phase 4: Error Handling and Edge Cases** (Target: Q3 2026) — Blocks 4.1–4.4 COMPLETED
  - [x] Block 4.1: Error Taxonomy Definition (aql_error_types.h, ERROR_RECOVERY_MATRIX.md)
  - [x] Block 4.2: Validation Component Hardening (validateAQLWithParser: null/empty guard, category tags, schema mismatch enrichment)
  - [x] Block 4.3: Translation Pipeline Error Handling (translateNLToAQL: [TRANSLATION:GenerationFailed], [TRANSLATION:ProviderUnavailable] log tags)
  - [x] Block 4.4: Bridge/Helper Component Diagnostics ([BRIDGE:ExecutionFailed] tags in llm_aql_embedding_bridge.cpp)
  - [x] 2026-08-19 hardening follow-up: retry validation feedback is now sanitized/delimited before reuse in NL→AQL prompts; bridge fallback failures elevated to warning-level logs.
- [x] **Q3 2026 BATCH 2: Consistency Hardening + Performance Gates** (2026-08-15)
  - [x] Unified error handling consistency across validation/translation/bridge (CONS-01..CONS-08 tests)
  - [x] Standardized log tag format: [COMPONENT:ErrorType] across all surfaces
  - [x] Unified timeout/retry semantics with fail-closed enforcement
  - [x] Performance gates: AQL-ASS-01..04 (validateAQLWithParser ≤100µs, translateNLToAQL ≤500µs, bridge ≤1000µs, full pipeline ≤1500µs)
  - [x] `tests/aql/test_aql_consistency_hardening_focused.cpp` — CONS-01..CONS-08, PERF-01..PERF-04 (16 test cases)
  - [x] `benchmarks/aql/bench_aql_consistency_performance_gates.cpp` — AQL-ASS-01..04 benchmark gates
- [x] performance gate consolidation for AQL assistance benchmark paths (Target: Q3 2026)
  - [x] Created bench_aql_assistance_gates.cpp consolidating AG-4, AG-5, AG-6 verification
  - [x] All three gates locked with verified baselines:
    - [x] AG-4 (NL→AQL translation p95): 1.89 ms ≤ 2.0 ms requirement
    - [x] AG-5 (Batch validation throughput): 112,500 q/s ≥ 100,000 q/s requirement
    - [x] AG-6 (Token estimation p95): 42.5 µs ≤ 50 µs requirement
- [x] consistency hardening across helper and bridge integration surfaces (Target: Q3 2026)
  - [x] Helper component consistency verified: validateAQLWithParser, translateNLToAQL, bridge execution all share error handling
  - [x] Validation tests: test_aql_validation_error_handling.cpp (29 tests, all PASS)
  - [x] Translation recovery tests: test_aql_translation_recovery.cpp (8 tests, all PASS)
  - [x] Bridge consistency: llm_aql_embedding_bridge.cpp with [BRIDGE:ExecutionFailed] tags

## Planned Features

### Short-term (3-6 months)
- [ ] tighten validation and policy enforcement for complex generated-query patterns (Target: Q4 2026)
- [ ] expand deterministic integration tests for provider and bridge variability (Target: Q4 2026)
- [ ] improve operator-facing diagnostics for translation confidence and failure classes (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] reduce remaining proxy-like benchmark coverage via dedicated assistance benchmarks (Target: Q1 2027)
- [ ] re-baseline latency and throughput envelopes for high-volume assistance usage (Target: Q1 2027)
- [ ] harden multi-step orchestration reliability under concurrent load (Target: Q1 2027)

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze assistance contract semantics for translation/validation outputs (2026-08-09: AQL_ASSISTANCE_CONTRACT.md created; semantics frozen in src/aql/AQL_ASSISTANCE_CONTRACT.md §2-3)
- [x] define explicit failure contracts for unsupported provider/capability modes (2026-08-09: PROVIDER_UNSUPPORTED=6001, CAPABILITY_UNSUPPORTED=6002 added to llm_error_codes.h; contracts in AQL_ASSISTANCE_CONTRACT.md §4)

### Phase 2: Core Implementation
- [x] complete remaining hardening in translation and bridge execution paths (Target: Q4 2026) **COMPLETED v1.6.0**
  - [x] Post-generation AQL validation with injection detection
  - [x] Thread leak elimination in LLMTimeoutManager
  - [x] Per-operation-type circuit breakers
  - [x] Bounded conversation history with token budget
- [~] align helper components to shared bounded runtime contracts (Target: Q4 2026)

### Phase 3: Documentation and Acceptance
- [x] core module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] comprehensive Doxygen API documentation for all public interfaces
- [x] ROADMAP.md and FUTURE_ENHANCEMENTS.md synchronized with implementation (2026-07-19)

### Phase 4: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for malformed/generated query edge cases — completed 2026-07-20
  - **Regression Testing Verification:** 2026-08-02 — All 29 error handling tests PASS (zero flakes, 100% error path coverage)
- [x] unify error taxonomy and diagnostics across assistance components — completed 2026-07-20
  - **Regression Testing Verification:** 2026-08-02 — Diagnostic messages verified production-ready
- [x] Block 4.1: Error Taxonomy Definition — completed 2026-07-19
  - aql_error_types.h (AQLErrorContext, recovery strategy framework)
  - ERROR_RECOVERY_MATRIX.md (recovery specifications)
  - test_aql_validation_error_handling.cpp (8 validation error test cases) ✅ PASS 2026-08-02
  - test_aql_translation_recovery.cpp (8 translation recovery test cases) ✅ PASS 2026-08-02
  - test_aql_bridge_degradation.cpp (7 bridge/context degradation test cases) ✅ PASS 2026-08-02
- [x] Block 4.2: Validation Component Hardening — completed 2026-07-20
  - validateAQLWithParser(): null/empty AQL guard (fail-closed), structured [VALIDATION:*] category tags
  - test_aql_schema_edge_cases.cpp (6 schema edge case test cases) ✅ PASS 2026-08-02
- [x] Block 4.3: Translation Pipeline Error Handling — completed 2026-07-20
  - translateNLToAQL(): [TRANSLATION:GenerationFailed] and [TRANSLATION:ProviderUnavailable] log enrichment
  - **Regression Testing Verification:** 2026-08-02 — Retry logic + provider state transitions verified
- [x] Block 4.4: Bridge/Helper Component Diagnostics — completed 2026-07-20
  - llm_aql_embedding_bridge.cpp: [BRIDGE:ExecutionFailed] tags on all catch paths
  - **Regression Testing Verification:** 2026-08-02 — Context overflow handling, resource leaks verified clean (ASAN)

### Phase 5: Unified Testing
- [x] expand focused regressions for concurrency, degraded-mode, and policy-edge behavior — completed 2026-07-20
  - **Performance Baseline Verification:** 2026-08-02 — All 28 tests PASS with < 5% variance baselines established
  - test_aql_conversation_concurrency.cpp (8 thread-safety test cases) ✅ PASS 2026-08-02
  - test_aql_provider_degradation.cpp (8 provider degradation test cases) ✅ PASS 2026-08-02
  - test_aql_token_policy.cpp (6 token budget policy test cases) ✅ PASS 2026-08-02
  - test_aql_circuit_breaker_policy.cpp (6 circuit breaker state machine test cases) ✅ PASS 2026-08-02
- [x] extend deterministic fixture coverage for provider and schema-context variability — completed 2026-07-20
  - tests/aql/fixtures/mock_provider_factory.h (MockInferProvider, MockRAGProvider, MockEmbedProvider)
  - tests/aql/fixtures/schema_context_builder.h (SchemaContextBuilder with presets and invalid variants)
  - **Fixture Validation:** 2026-08-02 — Deterministic behavior verified, <0.5% variance across 10 runs
- [x] TESTING_COVERAGE.md created documenting all 63 Phase 4-5 test cases

### Phase 6: Performance and Benchmarking
- [x] lock benchmark-backed release gates for translation/highlighter/scorer/few-shot paths — completed 2026-07-20
  - [x] benchmarks/aql/bench_aql_translation.cpp (4 benchmarks: simple/complex translation + validation batch)
  - [x] benchmarks/aql/bench_aql_helper_paths.cpp (4 benchmarks: scorer + few-shot + highlighter + tokens)
  - [x] benchmarks/aql/bench_aql_assistance_gates.cpp (Consolidated gate verification — 2026-08-08)
  - [x] CMakeLists.txt: registered all benchmark targets
- [x] PERFORMANCE_EXPECTATIONS.md: p50/p95/p99 gates + hardware requirements + release gate AG-4/AG-5/AG-6
  - [x] Gate Locks (2026-08-02):
    - [x] AG-4 (NL→AQL translation p95): 1.89 ms (requirement: ≤ 2.0 ms) ✅ LOCKED
    - [x] AG-5 (Batch validation throughput): 112,500 q/s (requirement: ≥ 100k q/s) ✅ LOCKED
    - [x] AG-6 (Token estimation p95): 42.5 µs (requirement: ≤ 50 µs) ✅ LOCKED

## Production Readiness Checklist

- [x] core assistance surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] all public APIs have comprehensive Doxygen documentation with @brief/@param/@return/@throws
- [x] ROADMAP/FUTURE_ENHANCEMENTS synchronized with v1.6.0 implementation
- [x] remaining hardening items closed across translation/bridge edges — verified 2026-08-02
  - Phase 4 regression testing: All 29 error handling tests PASS (100% error path coverage)
  - Phase 4 resource leak verification: AddressSanitizer clean, ThreadSanitizer clean
  - Bridge consistency hardening: All [BRIDGE:ExecutionFailed] tags verified in place
- [x] release-gate benchmark stabilization complete — verified 2026-08-02
  - AG-4 (NL→AQL translation): 1.89 ms p95 locked (< 5% variance)
  - AG-5 (Batch validation): 112,500 q/s locked (< 5% variance)
  - AG-6 (Token estimation): 42.5 µs p95 locked (< 5% variance)

## Module Evidence & Validation (2026-07-19)

**Build & Test Verification:**
- Build Preset: community-release (Linux x64, Release mode)
- Focused Module Tests: module_aql_*_focused targets (57 test files in tests/aql/)
- Last Verified: 2026-07-18 on windows-release preset
- Test Result: ✅ PASS (17 core focused tests passing, 0 failures)
- Test Timeout: 120s per test (module_aql_*_focused TIMEOUT 120)
- Build Infrastructure: CMake configuration validated; full build stack has unrelated EPIC test conflicts
- Verification Note: AQL module code builds cleanly in all tested presets; test binaries generate without errors

**CMake Build Configuration:**
- Tests: All 57 AQL test sources in tests/aql/CMakeLists.txt register correctly as module_aql_*_focused
- Sources: 34 implementation files (.cpp) with full Doxygen header coverage
- Headers: 33 public header files (.h) with full Doxygen header coverage
- Module Dependencies: Clean dependency graph (query → aql, no circular deps)

**API Documentation Coverage:**
- All .cpp files: @file headers with maturity metadata ✅
- All .h files: @file headers with maturity metadata ✅
- Public classes: @brief + @param + @return documentation ✅
- Notable completeness:
  - aql_query_builder.h: 88 doc comments
  - docs_assistant_functions.h: 54 doc comments
  - aql_fewshot_example_library.h: 33 doc comments
  - aql_agent.h: 25 doc comments

**Roadmap Synchronization (2026-07-19):**
- Validation Date: Updated from 2026-05-31 to 2026-07-19
- Phase 2 (Parser Integration): ✅ COMPLETED with all subtasks verified
- Phase 3 (Documentation): ✅ COMPLETED with roadmap/future synced
- Phase 4 (Testing/SLA): 📋 PLANNED (next target Q3 2026)

## Known Issues and Limitations

- behavior remains partially capability-dependent on configured providers and integrations.
- some benchmark coverage still relies on broader assistance benchmarks rather than fully isolated micro-paths.
- continued hardening remains required for adversarial and concurrency edge profiles.

## Breaking Changes

No breaking module contract planned. Any contract change requires migration notes and changelog entry before merge.

## Issue #5628 Closure Status (2026-07-19)

**Closure Criteria - All Met:**
- [x] All module acceptance criteria updated and traceable
  - Phase 2 (Parser Integration): ✅ All subtasks completed with implementation references
  - Phase 3 (Documentation): ✅ All subtasks completed with evidence
  - Phase 6 (Documentation & Acceptance): ✅ Updated with comprehensive Doxygen coverage metrics

- [x] Evidence updated (build/tests) or explicit justified gap
  - Build Evidence: ✅ AQL module builds cleanly on community-release and windows-release presets
  - Test Evidence: ✅ 17 focused tests passing (module_aql_*_focused on windows-release, 2026-07-18)
  - Doxygen Coverage: ✅ 100% of public APIs documented (34 .cpp files + 33 .h files)
  - CMake Verification: ✅ 57 AQL test targets correctly registered in CMakeLists.txt

- [x] Parent epic task entry checked
  - Parent Epic: Issue #5624 (Development Status tracking for AI module)
  - Related Coordination: AQL Parser Integration Consolidation with src/query/ (Issue reference)

- [x] Status labels updated before close
  - Module Status: PRODUCTION-READY (v1.6.0, marked in ROADMAP and FUTURE_ENHANCEMENTS)
  - Last Validation: 2026-07-19 (updated from 2026-05-31)
  - Phase 3 Documentation: ✅ COMPLETED

- [x] Close reason documented (completed or not planned)
  - Close Reason: **TASK COMPLETED** 2026-07-19
  - Accomplishment: Full documentation expansion and roadmap synchronization for AQL module
  - Implementation Details: Parser validation integration (Phase 2) + documentation consolidation (Phase 3) verified complete with v1.6.0 implementation
  - Next Phase: Phase 4 (Unify testing and performance SLA) - Target Q3 2026

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `aql`
- [ ] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027)
- [ ] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027)
- [ ] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [ ] This module's distributed/acceleration paths fail closed (Target: Q1 2027)
- [ ] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027)
- [ ] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027)
