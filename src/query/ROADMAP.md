> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase A–B), §7 (risk) -->

# Query Module Roadmap

## Current Status
Production-ready multi-model query stack with parser, optimizer, execution, federation, caching, and compatibility layers in active use.

**Hybrid Retrieval Rollout Readiness**: 55% 🟡 (issue #5468).
- Phase A (single-shard exact): ✅ Ready with error-path fixes (return-value checks, exception handling).
- Phase B (hybrid planning): 🟡 Q3 2026 — thread-safety in parallel plan optimization required.
- Phase C (parallel optimization): 🟡 Q3 2026+ after thread-safety fixes (140 gaps).
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

## In Progress
- [~] Query hardening wave for safety, resilience, and predictable performance (Target: Q3 2026)
  - [ ] Complete remaining performance/regression benchmark gates for vectorized and federated paths (Target: Q3 2026)
  - [ ] Continue reliability hardening for cancellation, limits, and distributed query failure behavior (Target: Q3 2026)
- [~] approved next implementation block: support Graph Phase 3 with query-planner and optimizer hardening before follow-on scheduling/load-balancing work (Target: Q3 2026)
- [~] **AQL LLM Integration Consolidation** — Phase 1-4 (2026-06-18 → ongoing)
  - Formalize dependency contract between src/query/ (Query Engine) and src/aql/ (LLM Integration)
  - Define canonical parser validation pipeline and SLA for LLM-generated AQL
  - [x] Phase 1: Define integration boundary (12 hrs) ✅ 2026-06-18
    - Created `src/query/AQL_LLM_INTEGRATION_CONTRACT.md` (canonical specification)
    - Updated `src/query/ARCHITECTURE.md` with LLM integration section
    - Updated `src/aql/ARCHITECTURE.md` with dependency documentation
  - [x] Phase 2: Wire parser validation + metrics (20 hrs) ✅ 2026-06-18
    - ✅ validateAQLWithParser() implemented in llm_aql_handler.cpp:1553
    - ✅ translateNLToAQL() calls validation with retry-on-error logic
    - ✅ Created integration test suite: test_aql_llm_integration.cpp (16 test cases)
    - ✅ Added Prometheus metrics instrumentation for validation tracking
    - ✅ Verified Prometheus counters/histograms bound to LLMMetricsCollector
    - Updated src/aql/ROADMAP.md to cross-reference consolidation work
  - [ ] Phase 3: Consolidate documentation (12 hrs) 📋 PENDING
    - Identify and unify duplicate AQL roadmaps across modules
    - Create cross-module reference guide
  - [~] Phase 4: Validation SLA performance tests (20 hrs) 🔄 IN PROGRESS
    - ✅ Created test_aql_validation_performance.cpp (8 performance test cases)
    - ✅ Tests verify SLA: ≤500ms per parse, ≥100 q/s throughput, <50ms error enrichment
    - ✅ Registered in tests/query/CMakeLists.txt with performance tier/labels
    - Pending: Build verification (blocked by pre-existing LLM linker errors)
  - Full detailed roadmap: [AQL_CONSOLIDATION_AUDIT_2026_06_18.md](./AQL_CONSOLIDATION_AUDIT_2026_06_18.md)
- [x] **AQL Mutations Language Extension** — Phase 1-5 Complete (v2.0.0, 2026-07-15)
  - INSERT, UPDATE, REPLACE, REMOVE, UPSERT statements implemented and tested
  - Transaction block integration (BEGIN...COMMIT) with atomic multi-statement batching
  - Full detailed roadmap + delivery evidence: [AQL_MUTATIONS_ROADMAP.md](./AQL_MUTATIONS_ROADMAP.md)
  - [x] Phase 1: Parser & Tokenizer Enhancement ✅ 2026-07-15
  - [x] Phase 2: Safety & Semantic Validation ✅ 2026-07-15
  - [x] Phase 3: Translation & Execution Plan ✅ 2026-07-15
  - [x] Phase 4: Transaction Support & Atomicity ✅ 2026-07-15
  - [x] Phase 5: Testing, Performance & Documentation ✅ 2026-07-15
  - Open: migration guide (Target: Q4 2026), security audit (Target: Q4 Week 1), production checklist final sign-off
- [~] **AQL v2.0.0 Remaining Work** — Geospatial + FTS wiring (Target: Q3–Q4 2026)
  - Full language roadmap: [AQL_V2_0_0_COMPLETE_ROADMAP.md](./AQL_V2_0_0_COMPLETE_ROADMAP.md)
  - [x] DDL (CREATE/DROP COLLECTION/INDEX/VIEW) — parser + executor + 32 tests delivered 2026-07-22
  - [~] Geospatial parser wiring (ST_* functions exist in let_evaluator, need FILTER/SORT/RETURN context) (Target: Q3 2026)
  - [ ] FTS query enhancement (phrase/proximity queries; ≤100ms on 100K documents) (Target: Q3–Q4 2026)
  - [ ] Cross-feature integration tests (1000+ tests, zero v1.x regressions) (Target: Q4 2026)

## Planned Features

### Hybrid Retrieval Rollout Gates (issue #5468)
- [ ] Phase A gate: fix 50% of return-value check gaps (340 → 170) in optimizer (Target: Q3 2026)
- [ ] Phase A gate: fix 50% of exception-handling gaps (180 → 90) in optimizer (Target: Q3 2026)
- [ ] Phase A ctest gate: `test_query_planner_fallback` with degraded-mode injection (Target: Q3 2026)
- [ ] Phase B gate: fix thread-safety gaps in parallel plan optimization (140 → 56) (Target: Q3 2026)
- [ ] Phase B gate: hybrid planner (ANN + graph) enabled with single-shard scope (Target: Q3 2026)
- [ ] Phase B gate: `query_planner_fallback_total` Prometheus metric wired (Target: Q3 2026)
- [ ] Phase C gate: parallel optimization enabled after thread-safety gate passed (Target: Q3 2026+)

### Short-term (3-6 months)
- [ ] **AQL Mutations** — INSERT/UPDATE/REPLACE/REMOVE/UPSERT for data manipulation (Target: v2.0.0-beta Q3 2026)
- [ ] Harden optimizer decision quality under skewed statistics and changing workloads (Target: Q4 2026)
- [ ] Expand federated query failure handling with deterministic partial-result policies (Target: Q4 2026)
- [ ] Strengthen query resource-limit enforcement diagnostics and operator-facing observability (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Advance approximate query processing from baseline implementations to production-suitable coverage (Target: Q1 2027)
- [ ] Extend ML-assisted optimization with strict fallback guarantees and reproducibility checks (Target: Q1 2027)
- [ ] Expand continuous-query backpressure and persistence hardening for long-lived subscriptions (Target: Q1 2027)

## Implementation Phases

### Phase 1: Safety and Access Hardening
- [ ] Keep parser/translation safety checks complete for edge-case query inputs (Target: Q3 2026)
- [ ] Ensure collection/access validation paths are enforced consistently across all execute entry points (Target: Q3 2026)

### Phase 2: Optimizer and Planning Hardening
- [ ] Improve plan-selection robustness under stale or partial statistics (Target: Q4 2026)
- [ ] Add deterministic regression packs for rewrite, cost, and adaptive plan switches (Target: Q4 2026)
- [ ] sequence optimizer/cache hardening ahead of broader runtime and federation work in the current execution block (Target: Q3 2026)

### Phase 3: Federation and Distributed Query Hardening
- [ ] Expand cross-cluster/federated timeout and retry envelopes with bounded memory behavior (Target: Q4 2026)
- [ ] Validate shard routing and partial-failure semantics under fault-injection (Target: Q4 2026)

### Phase 4: Runtime and Performance Hardening
- [ ] Re-baseline vectorized execution performance and memory envelopes on representative datasets (Target: Q1 2027)
- [ ] Tighten JIT fallback and equivalence checks for hot-query compilation paths (Target: Q1 2027)

### Phase 5: Documentation and Release Readiness
- [x] Keep query docs source-aligned with explicit sourcecode verification evidence per update cycle (Target: ongoing)
- [x] Keep completed roadmap items exclusively in changelog (Target: ongoing)

## Production Readiness Checklist
- Status: Tracking in progress
- Nachweise: query focused tests, federation tests, optimizer tests, performance suites
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Some long-horizon performance guarantees for federation/vectorized paths still require broader benchmark evidence.
- Advanced approximate/ML optimizer paths need additional production hardening and fallback verification.
- Continuous-query scale and backpressure behavior needs further validation under sustained load.

## Breaking Changes
- Query public APIs and language compatibility paths remain additive-first in active major lines.
- Any future behavioral change requiring client migration must be versioned and documented in changelog/migration notes.
