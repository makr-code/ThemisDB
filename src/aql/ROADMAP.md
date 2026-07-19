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

- [~] performance gate consolidation for AQL assistance benchmark paths (Target: Q3 2026)
- [~] consistency hardening across helper and bridge integration surfaces (Target: Q3 2026)

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
- [ ] freeze assistance contract semantics for translation/validation outputs (Target: Q3 2026)
- [ ] define explicit failure contracts for unsupported provider/capability modes (Target: Q3 2026)

### Phase 2: Core Implementation
- [x] complete remaining hardening in translation and bridge execution paths (Target: Q4 2026) **COMPLETED v1.6.0**
  - [x] Post-generation AQL validation with injection detection
  - [x] Thread leak elimination in LLMTimeoutManager
  - [x] Per-operation-type circuit breakers
  - [x] Bounded conversation history with token budget
- [~] align helper components to shared bounded runtime contracts (Target: Q4 2026)

### Phase 3: Error Handling and Edge Cases
- [ ] standardize fail-closed behavior for malformed/generated query edge cases (Target: Q4 2026)
- [ ] unify error taxonomy and diagnostics across assistance components (Target: Q4 2026)

### Phase 4: Tests
- [ ] expand focused regressions for concurrency, degraded-mode, and policy-edge behavior (Target: Q4 2026)
- [ ] extend deterministic fixture coverage for provider and schema-context variability (Target: Q4 2026)

### Phase 5: Performance and Hardening
- [ ] lock benchmark-backed release gates for translation/highlighter/scorer/few-shot paths (Target: Q4 2026)
- [ ] validate p95/p99 behavior against release baselines under representative workloads (Target: Q4 2026)

### Phase 6: Documentation and Acceptance
- [x] core module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] comprehensive Doxygen API documentation for all public interfaces
- [x] ROADMAP.md and FUTURE_ENHANCEMENTS.md synchronized with implementation (2026-07-19)

## Production Readiness Checklist

- [x] core assistance surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] all public APIs have comprehensive Doxygen documentation with @brief/@param/@return/@throws
- [x] ROADMAP/FUTURE_ENHANCEMENTS synchronized with v1.6.0 implementation
- [ ] remaining hardening items closed across translation/bridge edges
- [ ] release-gate benchmark stabilization complete

## Module Evidence & Validation (2026-07-19)

**Build & Test Verification:**
- Build Preset: community-release (Linux x64, Release mode)
- Focused Module Tests: module_aql_*_focused targets (57 test files in tests/aql/)
- Last Verified: 2026-07-18 on windows-release preset
- Test Result: ✅ PASS (17 core focused tests passing, 0 failures)
- Test Timeout: 120s per test (module_aql_*_focused TIMEOUT 120)

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