---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

> Original creation date was not recorded; this provenance block was first recorded on 2026-10-31.
> Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `../../ROADMAP.md`, `../../RELEASE_STRATEGY.md`, `ROADMAP.md`, `ARCHITECTURE.md`, `FUTURE_ENHANCEMENTS.md`, `PERFORMANCE_EXPECTATIONS.md`, `../../include/query/optimizer_cost_model.h`, `../../include/query/query_resource_limits.h`, `../../include/query/query_federation.h`, `../../tests/query/CMakeLists.txt`
Related issue: #7444
Related milestone: `v2.5.0 → v3.0.0 module rollout`

> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Rollout Plan: ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 (Phase A–B), §7 (risk) -->

# Query Module Roadmap

## Current Status
Production-ready multi-model query stack with parser, optimizer, execution, federation, caching, and compatibility layers in active use.

**Wave Alignment (see root ROADMAP.md § Program Execution Model):**
- **Wave A (Q3–Q4 2026):** Query planning determinism, timeout behavior consistency, cancellation semantics, distributed write stress
- **Wave A Exit Criteria:** Deterministic chaos evidence (query timeout, cancellation) + release-critical CI GREEN + federated execution baselines
- **Wave B (Q3–Q4 2026):** Distributed execution baselines, hybrid planner (ANN+graph), benchmark gates, parallel optimization
- **Wave B Exit Criteria:** Hybrid planner single-shard stable + federated execution p95/p99 on representative hardware
- **Tier 2 Functional Completeness:** Query planning is foundational for all workloads

**Hybrid Retrieval Rollout Readiness**: 65% 🟢 (issue #5468).
- Phase A (single-shard exact): ✅ Ready with error-path fixes (return-value checks, exception handling).
- Phase B (thread-safety): ✅ COMPLETE 2026-08-08 — QueryOptimizer per_query_cost_model_mutex_, advisor_cost_model_mutex_, adaptive_init_flag_ added; PlanCache stats atomics + deadline propagation wired; 9 tests in test_query_optimizer_thread_safety.cpp.
- Phase B (hybrid planner): 🟡 Q3 2026 — ANN + graph hybrid planner single-shard scope in progress.
- Phase C (parallel optimization): 🟡 Q3 2026+ (thread-safety gate now passed).
- Rollout risk detail: `ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §7`

## In Progress
- [~] Query hardening wave for safety, resilience, and predictable performance (Target: Q3 2026)
  - [ ] Complete remaining performance/regression benchmark gates for vectorized and federated paths (Target: Q3 2026)
  - [ ] Continue reliability hardening for cancellation, limits, and distributed query failure behavior (Target: Q3 2026)
  - [x] Canonical retry metadata envelope integrated into federation timeout paths (`retry_count`, exhaustion reason, timeout source, correlation id) with focused coverage in `test_query_federation_hardness.cpp` (Delivered: 2026-08-09)
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
  - [x] Phase 3: Consolidate documentation (12 hrs) ✅ 2026-08-05
    - ✅ Created AQL_LLM_INTEGRATION_PHASE3_PARSER_CHANGES.md (3h, comprehensive parser evolution doc)
    - ✅ Created AQL_LLM_INTEGRATION_PHASE3_METRICS.md (3h, metrics interpretation guide)
    - ✅ Created AQL_LLM_INTEGRATION_PHASE3_API_CONTRACT.md (2h, public API definition)
    - ✅ Created AQL_LLM_INTEGRATION_MIGRATION_GUIDE.md (2h, user adoption guide)
    - ✅ Updated AQL_CONSOLIDATION_INDEX.md with Phase 3 links (1h)
    - ✅ All cross-references verified; no broken links (1h, QA validation)
  - [x] Phase 4: Validation SLA performance tests (20 hrs) ✅ COMPLETE (2026-08-05)
    - ✅ Created test_aql_validation_performance.cpp (8 performance test cases)
    - ✅ Tests verify SLA: ≤500ms per parse, ≥100 q/s throughput, <50ms error enrichment
    - ✅ Registered in tests/query/CMakeLists.txt with performance tier/labels
    - ✅ Build verification: All dependencies resolved (fmt, boost, spdlog, etc.)
    - ✅ SLA validation: All 8 tests pass; performance targets confirmed met
    - ✅ Detailed results: PHASE_6B_SLA_VALIDATION_REPORT.md
  - Historical consolidation audit: [AQL_CONSOLIDATION_AUDIT_2026_06_18.md](../../docs/ARCHIVED/ai-working-history/reports/root_cleanup/AQL_CONSOLIDATION_AUDIT_2026_06_18.md)
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
  - [x] Geospatial parser Phase 1 COMPLETE 2026-07-27: ST_* already work in FILTER/SORT/RETURN via qe_evalFunction; 26 cases are recorded in `tests/aql/test_aql_st_predicates.cpp`; Phase 2 (optimizer hints) next (Target: Q3 2026)
  - [x] FTS query enhancement (phrase/proximity queries; ≤100ms on 100K documents) (Target: Q3–Q4 2026)
    - [x] SEARCH/PHRASE/NEAR/STARTS_WITH/BOOST/ANALYZER tokens added to lexer (2026-08-09)
    - [x] FtsPredType enum, FtsPredicateNode, SearchClauseNode structs added to aql_parser.h (2026-08-09)
    - [x] parseSearchClause() implemented; wired into parseQuery() (2026-08-09)
    - [x] search_clause field added to Query struct (2026-08-09)
    - [x] Executor backend wiring (FTS index lookup, scoring) (Target: Q4 2026) — delivered 2026-09-03 (Block 2)
    - [x] Phrase/proximity executor behavior is now source-covered with focused `FtsExecutor` tests (exact phrase, case-insensitive phrase, bounded proximity, boolean reuse, timeout, OOM) and production-backed matching over posting-list positions (2026-09-09)
    - [x] Benchmark harness now exercises the real `FtsExecutor` on an on-disk 10K/100K corpus in `benchmarks/rag/bench_fts_phase_b.cpp` instead of the former synthetic token simulation (2026-09-09)
    - [x] Performance gate: ≤100ms on 100K documents (Target: Q4 2026) — local benchmark evidence captured 2026-09-09 with `BM_FtsPhraseQuery/100000 p95_ms=1.3638` and `BM_FtsProximityQuery/100000 p95_ms=1.53029`
    - [~] Broad baseline refresh automation now includes the FTS benchmark artifact path on `Build: Benchmarks` run `34345454063`; representative-hardware capture remains open. (Target: Q1 2027)
  - [~] Cross-feature integration tests (1000+ tests, zero v1.x regressions) (Target: Q4 2026)

## Phase 2 — Performance & Scalability Readiness (Target: 2026-09-30)

Performance and scalability optimization with measurable, gated performance improvements validated against Wave 7 gates.

### Graph/Query Optimization Backlog

**Core Goal:** Deliver query optimization features with measurable latency/throughput improvements validated against Wave 7 gates.

#### 1. Query Plan-Cache Implementation (Target: Q3 2026)

**Scope:** src/query/optimizer/ (plan caching subsystem)

- [x] Implement query plan caching mechanism (Target: Q3 2026) ✅ COMPLETE
  - Design: LRU cache for query plans (size: tunable, default 1000 plans)
  - Key: normalized query text + parameter types
  - Value: compiled execution plan
  - Invalidation: on schema changes, statistics updates
  - **Delivery**: plan_cache.cpp (existing, verified production-ready)

- [x] Add plan reuse validation (Target: Q3 2026) ✅ COMPLETE
  - Detect plan invalidation conditions (schema DDL, index drops)
  - Implement cache invalidation with precise granularity (table-level)
  - Log cache invalidations for diagnostics

- [x] Performance gate: 10%+ latency improvement (Target: Q3 2026) ✅ COMPLETE
  - Measure: 10%+ latency improvement for repeated queries
  - Validate: Regression tests PG1, PG2 in test_query_optimizer_regression.cpp
  - Baseline: read p99≤200µs, batch p99≤5ms

- [x] Add stress tests (Target: Q3 2026) ✅ COMPLETE
  - Cache coherency under concurrent queries (CMX-style tests)
  - Cache invalidation under concurrent DDL (test STRESS1_CacheCoherencyConcurrentDDL)
  - Cache eviction under plan explosion (test PC5_CacheLRUEviction)
  - **Delivery**: test_query_optimizer_regression.cpp (33 total test cases)

#### 2. Cost-Model Refinement (Target: Q3 2026) ✅ COMPLETE

**Scope:** src/query/optimizer_cost_model_enhancements.h/.cpp

- [x] Improve cardinality estimation (Target: Q3 2026) ✅ COMPLETE
  - Implement histogram-based estimation (ColumnHistogram struct)
  - Use table statistics (row count, column selectivity)
  - Add correlation awareness (ColumnCorrelation struct)
  - **Delivery**: optimizer_cost_model_enhancements.h/.cpp

- [x] Implement cost-based join ordering (Target: Q3 2026) ✅ COMPLETE
  - Compare: nested-loop vs. hash-join vs. merge-join costs
  - Select: lowest-cost join order
  - Validate: execution stats match estimates (tests CM1-CM3)
  - **Delivery**: optimizer_cost_model.cpp (existing joint cost estimation methods)

- [x] Add estimate validation (Target: Q3 2026) ✅ COMPLETE
  - Log estimate vs. actual cardinality (EstimateValidation struct)
  - Compute estimation error metrics (computeMAPE, computeP95Error)
  - Flag systematic underestimation/overestimation (tests EV3, EV4)
  - **Delivery**: CostModelEnhancements static methods + tests

- [x] Create diagnostics (Target: Q3 2026) ✅ COMPLETE
  - Cardinality estimates logged in CostModelEnhancements::recordEstimate
  - Warning: large estimation errors (>50%, flagged as WARN level)
  - Systematic bias detection with recommendations
  - **Delivery**: optimizer_cost_model_enhancements.cpp logging integration

#### 3. Cache-Efficiency Improvements (Target: Q3 2026)

**Scope:** src/storage/buffer_pool/, src/query/ (cache-aware optimizations)

- [x] Profile and optimize buffer-pool hit rate (Target: Q3 2026)
  - Measure: cache hit rate before/after optimization
  - Implement: cache-aware prefetching (sequential access patterns)
  - Optimize: LRU eviction policy tuning

- [~] Implement cache-aware data layout (Target: Q3 2026)
  - Co-locate frequently-joined columns (reduce cache misses)
  - Minimize cache line waste (column grouping)
  - Validate: latency improvement under scan workloads

- [x] Add cache-pressure telemetry (Target: Q3 2026)
  - Track: cache miss rate, eviction rate, pressure level
  - Implement: adaptive eviction (age-based, frequency-based)
  - Alert: high pressure conditions

- [~] Gate on performance (Target: Q3 2026)
  - Validate: no regression under scan workloads
  - Document: cache efficiency metrics in ROADMAP.md

#### 4. Resource-Pooling & Load-Balancing (Target: Q3 2026)

**Scope:** src/scheduler/, src/network/ (thread pools, connection pools)

- [~] Consolidate thread-pool management (Target: Q3 2026)
  - Audit: existing thread pools across modules
  - Implement: centralized pool manager (if not present)
  - Standardize: pool sizing, queue depths, overflow handling

- [~] Implement adaptive pool sizing (Target: Q3 2026)
  - Measure: queue depth, wait times, CPU utilization
  - Adjust: pool size based on workload pressure
  - Implement: gradual scaling (avoid thrashing)

- [~] Add connection pool rebalancing (Target: Q3 2026)
  - Distribute: connections evenly across backend shards
  - Rebalance: on shard failure/recovery
  - Validate: no starvation under uneven load

- [~] Gate on throughput (Target: Q3 2026)
  - Validate: write latency stable under peak load
  - Document: pool sizing recommendations

### Performance Regression Gates (Target: 2026-09)

1. **Execute Wave-7 Full Suite:**
   - [~] bench_w7a_release_critical_signoff (RCS-01..08) (Target: 2026-09)
   - [~] bench_w7b_endurance_soak (SOK-01..08) (Target: 2026-09)
   - [~] bench_w7c_degradation_fault_recovery (DFR-01..08) (Target: 2026-09)
   - [~] bench_w7d_guardrails_variance_operability (GVO-01..08) (Target: 2026-09)

2. **Validate Gate Status:** (Target: 2026-09)
   - [~] GATE-W7-01: Read p99 ≤ 200µs ✓ (Target: 2026-09)
   - [~] GATE-W7-02: Write ≥ 80k ops/s ✓ (Target: 2026-09)
   - [~] GATE-W7-03: Range p99 ≤ 500µs ✓ (Target: 2026-09)
   - [~] GATE-W7-04: Batch p99 ≤ 5ms ✓ (Target: 2026-09)
   - [~] GATE-W7-05/06: Self-check counters ✓ (Target: 2026-09)

3. **Baseline Variance Report:** (Target: 2026-09)
   - [~] Use: benchmarks/wave7/report_variance_w7.py (Target: 2026-09)
   - [~] Measure: p50, p95, p99, max latencies (Target: 2026-09)
   - [~] Document: variance envelope and drift tolerance (Target: 2026-09)

### Repeatable Under-Load Results (Target: 2026-09)

1. **Endurance Benchmark Suite:** (Target: 2026-09)
   - [~] Duration: 8+ hour soak tests (Target: 2026-09)
   - [~] Workload: Mix of read/write/range/batch operations (Target: 2026-09)
   - [~] Load: Realistic production intensity (Target: 2026-09)

2. **Stability Measurements:** (Target: 2026-09)
   - [~] Track: p95/p99 latency over time (Target: 2026-09)
   - [~] Detect: performance drift or degradation (Target: 2026-09)
   - [~] Alert: significant variance (>10% change) (Target: 2026-09)

3. **Archive Results:** (Target: 2026-09)
   - [~] Save: benchmark data for release sign-off (Target: 2026-09)
   - [~] Document: latency/throughput envelopes (Target: 2026-09)
   - [~] Baseline: for regression detection (Target: 2026-09)

### Phase 2 Exit Criteria

- [~] Query plan-cache: ✓ implemented, 10%+ improvement gated (Target: 2026-09)
- [~] Cost-model: ✓ cardinality estimation validated, joins optimized (Target: 2026-09)
- [~] Cache-efficiency: ✓ hit rate improved, prefetching added (Target: 2026-09)
- [~] Resource pooling: ✓ consolidated, adaptive sizing functional (Target: 2026-09)
- [~] Wave 7 gates: ✓ all 6 PASS, no regressions (Target: 2026-09)
- [~] 130+ new unit/integration tests created and passing (Target: 2026-09)
- [~] Performance envelopes documented and baselined (Target: 2026-09)

### Deliverables

1. Query optimization implementations (plan-cache, cost-model, cache-efficiency, pooling)
2. 130+ focused tests (CACHE-01..20, OPT-01..30, POOL-01..20, E2E-QUERY-01..40)
3. Wave-7 baseline validation report
4. Performance envelope documentation
5. Updated src/query/ROADMAP.md with delivery status

### Notes

- Prioritize improvements with measurable Wave-7 gate validation
- No optimization without baseline measurement
- Document all performance tuning assumptions
- If gates regress, investigate and remediate before declaring phase complete

---

## Planned Features

### Wave 9 Block 3 Delivery (2026-08-26)

- [x] **W9-10**: 7 HIGH gaps closed — catch_all_swallow, memory_leak, null_dereference, todo_as_productionlogic, uncaught_exception, unchecked_result, string_concat_loop (Target: Q3 2026) ✅ COMPLETE
- [x] **W9-11**: AQL FunctionCall compat shim deprecation warning added; callers documented; **W10-C: compat shim removed** — no callers existed; canonical `SimilarityCall`/`ProximityCall` paths are sole active dispatch (Target: Q4 2026) ✅ COMPLETE
- [x] **W9-12**: Hybrid ANN+graph planner `planAnnGraphHybrid()` delivered in `tensor_aware_query_optimizer.cpp`; `AnnFrontdoor` + `IKnowledgeGraph::neighbours()` + RRF fusion; 500ms gate; 14 regression tests (Target: Q3 2026) ✅ COMPLETE

### Hybrid Retrieval Rollout Gates (issue #5468)
- [~] Phase A gate: fix 50% of return-value check gaps (340 → 170) in optimizer (Target: Q3 2026)
- [~] Phase A gate: fix 50% of exception-handling gaps (180 → 90) in optimizer (Target: Q3 2026)
- [x] Phase A ctest gate: `test_query_planner_fallback` with degraded-mode injection (tests/query/test_query_planner_fallback.cpp: degraded mode + query_planner_fallback_total check present)
- [x] Phase B gate: fix thread-safety gaps in parallel plan optimization (140 → 56) (COMPLETE 2026-08-08: QueryOptimizer per_query_cost_model_mutex_, advisor_cost_model_mutex_, adaptive_init_flag_ added; PlanCache stats atomics + deadline propagation wired; test_query_optimizer_thread_safety.cpp — 9 tests)
- [x] Phase B gate: hybrid planner (ANN + graph) enabled with single-shard scope (planAnnGraphHybrid delivered W9-12 2026-08-26)
- [x] Phase B gate: `query_planner_fallback_total` Prometheus metric wired (src/query/query_optimizer.cpp:177)
- [ ] Phase C gate: parallel optimization enabled after thread-safety gate passed (Target: Q3 2026+)

### Short-term (3-6 months)
- [x] Replace `makeNotImplemented()`/stub-backed process-mining AQL gaps with context-backed trace, pattern-match, and ideal-comparison execution in `src/query/functions/process_mining_functions.cpp` (Target: Q4 2026) — delivered 2026-08-31
- [x] Replace the three generic runtime-error paths in `src/query/functions/ethics_functions.cpp` (`ETHICS_GET_ARGUMENTS`, `ETHICS_FIND_SIMILAR_DILEMMAS`, `ETHICS_TRAVERSE_CHAIN`) with collection/graph/vector-backed execution over `FunctionContext` scanners (Target: Q4 2026) — delivered 2026-08-31
- [x] **AQL Mutations** — INSERT/UPDATE/REPLACE/REMOVE/UPSERT for data manipulation (UPSERT/INSERT/UPDATE/REMOVE/REPLACE nodes already in include/query/aql_parser.h; DML in parse loop)
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

## Release Roadmap — v2.5.0 Near-term and v3.0.0 Strategic (Issue #7444)

This is a planning proposal, not a release commitment. Root release/version authority remains [`../../ROADMAP.md`](../../ROADMAP.md) and [`../../RELEASE_STRATEGY.md`](../../RELEASE_STRATEGY.md); v2.5.0 is the upcoming minor line there. No release date is inferred here. Planning windows are relative to approval, and owner assignment, milestone timing, and scope approval remain open.

The P-labels below are proposed relative priorities for release readiness, not approved commitments or capacity assignments. Within each release line, P1 precedes P2; the v2.5.0 work is near-term and the v3.0.0 work is strategic.

### Maturity assessment and evidence boundary

- **Assessment:** Query has substantial implemented breadth across parsing, planning/optimization, execution coordination, limits/cancellation, and optional federation. It is not evidence that every path is production-validated: optimizer quality under changing distributions, bounded failure behavior, and representative end-to-end performance remain material gaps. This roadmap does not assign a new maturity score.
- **Ownership boundary:** [`ARCHITECTURE.md`](ARCHITECTURE.md) defines query as owning parsing/planning/execution coordination, not storage/index implementation or network ingress. The work below stays within that boundary.
- **Source evidence:** [`../../include/query/optimizer_cost_model.h`](../../include/query/optimizer_cost_model.h) exposes scan/join/selectivity estimates and table/column/index statistics, including stale-state information. [`../../include/query/query_resource_limits.h`](../../include/query/query_resource_limits.h) has row, memory, and timeout limits; [`../../include/query/query_federation.h`](../../include/query/query_federation.h) defines shard fan-out, result-byte, and timeout defaults.
- **Test evidence:** [`../../tests/query/CMakeLists.txt`](../../tests/query/CMakeLists.txt) registers focused core tests including `QueryEngine` with `release_critical` labeling and selected query test modules. Source coverage includes parser edge cases, [`../../tests/query/test_query_resource_limits.cpp`](../../tests/query/test_query_resource_limits.cpp), [`../../tests/query/test_query_cancellation.cpp`](../../tests/query/test_query_cancellation.cpp), optimizer statistics/thread-safety, [`../../tests/query/test_query_jit_equivalence.cpp`](../../tests/query/test_query_jit_equivalence.cpp), federation hardness/routing, and continuous-query hardening. Presence and registration do **not** prove the current CI/runtime result; require run artifacts before calling a gate passed.
- **Benchmark evidence and limitation:** [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) records QG-1..4 and generic NG targets, while representative-hardware p95/p99 refresh remains outstanding. [`../../benchmarks/query/bench_query_dedicated_gates.cpp`](../../benchmarks/query/bench_query_dedicated_gates.cpp) explicitly uses in-process parser/planner/executor stubs: it is harness evidence, **not real-engine performance evidence**. [`../../benchmarks/query/bench_query_limit_scaling.cpp`](../../benchmarks/query/bench_query_limit_scaling.cpp) invokes `VectorizedExecutionEngine` with a synthetic row source; it is useful for that scoped limit-scaling path, not a production storage/backend workload.

| Research dimension | Repository evidence | Remaining gap / planned response |
|---|---|---|
| Performance / scalability | [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`FEATURES.md`](FEATURES.md) document multi-model query/execution paths; the [cost-model interfaces](../../include/query/optimizer_cost_model.h) cover estimates and stale statistics. The dedicated gate uses stubs; limit-scaling uses a synthetic source. | Representative-hardware refresh remains open. Wave 2.5-B adds reproducible, non-stub measurements; Wave 3.0-A evaluates adaptive plans against fixed baselines. |
| Application scenarios / usability | The same module docs describe document, graph, vector, geospatial, and timeseries paths and the query flow. | Repository documentation is not validation of representative user/application scenarios. Wave 2.5-B defines workload cases; Wave 3.0-B covers sustained and federated operation and completion semantics. |
| Security / resilience | [Resource-limit](../../include/query/query_resource_limits.h), [cancellation](../../include/query/query_canceller.h), and [federation](../../include/query/query_federation.h) interfaces expose controls; focused tests exist and are selectively registered in [`tests/query/CMakeLists.txt`](../../tests/query/CMakeLists.txt). | End-to-end bounds, input-boundary review, and deterministic failure evidence remain to be established. Wave 2.5-A closes focused safety gaps; Wave 3.0-B addresses slow, failed, and recovering peers. |
| Governance / compliance / maintainability | [`ARCHITECTURE.md`](ARCHITECTURE.md) states ownership boundaries; test registration includes selected focused suites, including `QueryEngine` labeled `release_critical`. | Registration is not a CI result or compliance finding. Wave 2.5-A records focused safety evidence and input-boundary review; Wave 3.0-B reviews module boundaries and migration compatibility. |
| Architecture / operational maturity | The documented module boundary excludes storage/index ownership and network ingress; query execution and federation are represented by module docs and interfaces. Research cited below is context only and does not validate ThemisDB. | Sustained-run behavior and representative operational evidence remain open. Wave 3.0-A retains guarded planning fallback; Wave 3.0-B covers sustained/federated behavior, metrics, and representative-duration evidence. |

### Research grounding (context, not ThemisDB validation)

- Leis et al., [“How Good Are Query Optimizers, Really?” (VLDB 2016)](https://www.vldb.org/pvldb/vol9/p204-leis.pdf), motivates measuring optimizer plan quality against execution outcomes rather than assuming cost-model estimates imply good plans. Its benchmark systems, operators, and workloads are not ThemisDB evidence.
- Mu et al., [“Systematic Evaluation of Plan-based Adaptive Query Processing” (arXiv:2511.16455)](https://arxiv.org/abs/2511.16455), is a recent reference for evaluating adaptive, plan-based query processing systematically. Its evaluated designs and experimental conditions do not establish which adaptation policy is best for ThemisDB.
- Begoli et al., [Apache Calcite (SIGMOD 2018)](https://arxiv.org/abs/1802.10233), describes an extensible query-processing framework; its modularity is useful design context, not a performance result or a requirement to adopt Calcite.
- The [official TPC-H benchmark overview](https://www.tpc.org/tpch/) is a reference for standardized decision-support workload evaluation. Any use here must state dataset, query set, scale, and execution conditions; no TPC-H compliance or result is claimed.
- The [OWASP SQL Injection Prevention Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/SQL_Injection_Prevention_Cheat_Sheet.html) provides defensive guidance for untrusted query inputs and parameterization. It is not proof of ThemisDB injection resistance, and applicability must be checked per frontend/API rather than assuming SQL semantics for AQL.
- The 2024 [reproducibility article](https://link.springer.com/article/10.1007/s10586-024-04533-0) is general context for reproducible computational evaluation. It does not validate any ThemisDB experiment; record environment, inputs, repetitions, and raw results for our own claims.

These references motivate hypotheses and evaluation discipline only. Results from other database systems, query languages, workloads, and hardware must not be presented as validated ThemisDB outcomes.

### v2.5.0 — near-term release hardening

Scope: close bounded-safety and evidence gaps without broad query-language or public-API redesign. Target is v2.5.0; schedule and accountable owners require maintainer/release-manager approval. Planning effort is an estimate, not committed capacity.

#### Wave 2.5-A — safety and deterministic behavior (P1 — highest v2.5.0 release-readiness priority)

- **Goal:** Demonstrate predictable resource, cancellation, and federated failure behavior on focused query paths before expanding performance claims.
- **Concrete tasks:**
  - [ ] Audit execute entry points against row/memory/timeout enforcement; close any uncovered paths and add boundary regressions for configured limits (Target: v2.5.0).
  - [ ] Run and, where needed, extend parser edge-case and cancellation tests; require evidence for malformed input, cancellation, and timeout outcomes (Target: v2.5.0).
  - [ ] Exercise federation fan-out/result-byte/timeout bounds and define deterministic partial-failure outcomes; document any deliberately unsupported retry/partial-result behavior (Target: v2.5.0).
  - [ ] Review external-query input boundaries against OWASP guidance, without treating that guidance as a substitute for frontend-specific tests (Target: v2.5.0).
- **Dependencies:** Existing APIs and focused tests above; CI/build availability; shard-router and executor test doubles for failure injection. Any storage or ingress change must be owned and reviewed by its canonical module.
- **Risks / trade-offs:** Enforcing limits consistently can change behavior for callers relying on effectively unbounded execution; retries/partial results can amplify load or obscure incomplete answers. Prefer explicit defaults and compatibility review over silent semantic changes.
- **Effort estimate:** 2–4 engineering-weeks, excluding cross-module changes and release sign-off.
- **Measurable success metrics:** All focused safety tests registered for the intended profile and passing in recorded CI runs; boundary cases cover below/at/above each configured limit; timeout/cancel completes within the test’s bounded deadline; federation tests assert fan-out and result-byte ceilings and explicit failure outcomes. No claim of zero security defects without a scoped review.
- **Validation strategy:** Build/run the focused registered tests on `develop`, record CI identifiers and failures, then rerun the `release_critical` profile. Use fault injection for timeout, cancellation, and peer failure; inspect results for truncation/partial-success ambiguity. Do not infer pass status from source presence.

#### Wave 2.5-B — optimizer and performance evidence (P2 — next v2.5.0 release-readiness priority)

- **Goal:** Replace proxy/stub performance assertions with repeatable, representative engine-path measurements and regression evidence.
- **Concrete tasks:**
  - [ ] Add or designate a non-stub parse/plan/execute benchmark path and clearly label mocks, synthetic sources, and real backends in reports (Target: v2.5.0).
  - [ ] Build a reproducible workload pack spanning simple filters, joins, pagination, skew/statistics drift, and vectorized limit scaling; document data generation, warm-up, repetitions, and environment (Target: v2.5.0).
  - [ ] Compare estimated versus observed cardinalities and plan outcomes under fresh, missing, stale, and skewed statistics; retain stable fallback behavior where evidence is insufficient (Target: v2.5.0).
  - [ ] Refresh QG-1..4 and p95/p99 baselines on agreed representative hardware; report variance and regressions against the prior comparable baseline (Target: v2.5.0).
- **Dependencies:** Wave 2.5-A safety gate; benchmark-capable build/runner; representative hardware and dataset; release owner approval of workload and baseline policy.
- **Risks / trade-offs:** Hardware, cache state, and workload drift can make comparisons misleading; more statistics/adaptation may increase planning latency or memory. Preserve raw runs and disclose environment rather than tuning to one score.
- **Effort estimate:** 2–5 engineering-weeks, plus runner/hardware scheduling and repeated measurement time.
- **Measurable success metrics:** 100% of reported cases identify engine/stub status, dataset, hardware, and run configuration; QG-1..4 have repeatable measured outcomes or an explicitly tracked blocked/proxy status; p95/p99 refresh is attached to representative-hardware runs; regression is reported against the existing QG-4 policy (<=8%) without silently redefining gates.
- **Validation strategy:** Run repeated benchmark trials on the same recorded environment and compare distributions, not one-off best values. Review variance and raw artifacts; separately verify `bench_query_limit_scaling.cpp` as vectorized-engine/synthetic-source evidence. TPC-H may inform workload design only if its actual inputs/protocol are identified; do not claim official benchmark compliance absent that process.

### v3.0.0 — strategic query evolution

Scope: major-line opportunities that require measured safety/quality gains and explicit compatibility review; not a promise to ship every candidate. Any public language/API change needs versioned migration documentation. Sequencing and release date remain unapproved.

#### Wave 3.0-A — adaptive planning with guarded fallback (P1 — first v3.0.0 priority)

- **Goal:** Improve plan robustness across changing and skewed workloads while preserving explainable, bounded fallback behavior.
- **Concrete tasks:**
  - [ ] Define a workload-backed evaluation matrix for cardinality-estimation error, stale-statistics detection, plan quality, and planning overhead; publish reproducible cases and thresholds (Target: v3.0.0).
  - [ ] Evaluate adaptive re-optimization triggers against fixed-plan baselines; require deterministic fallback when statistics or runtime signals are missing or unreliable (Target: v3.0.0).
  - [ ] Extend semantic-equivalence and concurrency tests for rewrites, adaptive plan switches, and JIT/interpreter execution; make any unsupported operator/path explicit (Target: v3.0.0).
  - [ ] Promote only optimizations that improve pre-agreed workload metrics without violating correctness, resource budgets, or planning-latency limits (Target: v3.0.0).
- **Dependencies:** Wave 2.5-B reproducible baselines; optimizer/runtime instrumentation; maintainers approve metric definitions and compatibility policy.
- **Risks / trade-offs:** Adaptive decisions can cause plan instability, nondeterminism, planning overhead, or regressions on workloads unlike the evaluation set. Guard with bounded retries, observable decisions, and opt-out/fallback policy where appropriate.
- **Effort estimate:** 4–8 engineering-weeks for evaluation and implementation, with additional workload-specific tuning subject to evidence.
- **Measurable success metrics:** Every accepted adaptive path has a deterministic fallback and equivalence regression; evaluation reports estimate error and planning/execution latency for each workload class; improvement threshold is agreed before tuning and no correctness/resource gate regresses.
- **Validation strategy:** Compare adaptive and fixed plans on identical data and environment, include skew/drift and missing-statistics cases, repeat trials, and retain raw traces. Independent reproduction/review is required before generalizing findings; research results are not a substitute.

#### Wave 3.0-B — sustained and federated query operability (P2 — after v3.0-A)

- **Goal:** Extend safe long-running and distributed query operation only where boundedness and user-visible completion semantics can be demonstrated.
- **Concrete tasks:**
  - [ ] Specify and test federation cancellation, deadlines, bounded result accumulation, and partial-result/error contracts under slow, failed, and recovering peers (Target: v3.0.0).
  - [ ] Validate continuous-query backpressure, queue/memory bounds, and recovery behavior under sustained and burst workloads; document persistence guarantees separately from query-module ownership (Target: v3.0.0).
  - [ ] Add representative-duration federation/continuous-query benchmarks and operational metrics, separating synthetic test sources from production-backed runs (Target: v3.0.0).
  - [ ] Review any proposed API or language changes for major-version compatibility and update canonical migration/release documents before adoption (Target: v3.0.0).
- **Dependencies:** Wave 2.5-A bounded-failure evidence; federation/router test environments; continuous-query stream and persistence owners; observability support.
- **Risks / trade-offs:** Backpressure and stricter bounds may reduce throughput or expose previously hidden partial results; retries and long retention increase resource cost. Persistence and network-ingress guarantees belong to their respective modules.
- **Effort estimate:** 4–8 engineering-weeks, excluding cross-module persistence/ingress work and sustained-run wall time.
- **Measurable success metrics:** Fault matrix covers slow/failing/recovering peers and asserts bounded fan-out/result bytes/deadline; sustained tests report memory/queue high-water marks and no unbounded growth; completion or partial-failure status is unambiguous in every tested case; release-critical tests remain green.
- **Validation strategy:** Execute focused fault-injection tests plus sustained runs with recorded configuration, artifacts, and resource samples; repeat on agreed representative hardware for performance claims. Review module boundaries and migration compatibility before release sign-off.

### Implementation-phase mapping

The phase tasks below map the existing wave work to the repository implementation phases; they do not add scope beyond those wave tasks.

#### Phase 1 — Design/API contract

- [ ] Define the workload-backed evaluation matrix, measures, and thresholds for cardinality error, plan quality, and planning overhead (Wave 3.0-A; Target: v3.0.0).
- [ ] Specify federation cancellation, deadlines, bounded result accumulation, and partial-result/error contracts, and review proposed API or language changes for major-version compatibility (Wave 3.0-B; Target: v3.0.0).

#### Phase 2 — Core implementation

- [ ] Close uncovered execute paths for row, memory, and timeout enforcement (Wave 2.5-A; Target: v2.5.0).
- [ ] Implement adaptive re-optimization only for evaluation-backed triggers, retaining the fixed-plan fallback (Wave 3.0-A; Target: v3.0.0).
- [ ] Validate continuous-query backpressure, queue/memory bounds, and recovery behavior under sustained and burst workloads (Wave 3.0-B; Target: v3.0.0).

#### Phase 3 — Error handling & edge cases

- [ ] Define deterministic federation partial-failure outcomes and exercise fan-out, result-byte, and timeout bounds (Wave 2.5-A; Target: v2.5.0).
- [ ] Add deterministic adaptive-plan fallback for missing or unreliable statistics and cover slow, failed, and recovering federation peers (Waves 3.0-A and 3.0-B; Target: v3.0.0).

#### Phase 4 — Tests

- [ ] Run and extend parser edge-case and cancellation tests for malformed input, cancellation, and timeout outcomes (Wave 2.5-A; Target: v2.5.0).
- [ ] Extend semantic-equivalence and concurrency tests for rewrites, adaptive plan switches, and JIT/interpreter execution (Wave 3.0-A; Target: v3.0.0).
- [ ] Test federation fault cases and sustained continuous-query behavior, including bounded resources and unambiguous completion/failure (Wave 3.0-B; Target: v3.0.0).

#### Phase 5 — Performance/hardening

- [ ] Add or designate a non-stub parse/plan/execute benchmark path and build the reproducible workload pack (Wave 2.5-B; Target: v2.5.0).
- [ ] Refresh QG-1..4 and p95/p99 baselines on agreed representative hardware, reporting variance and comparable regressions (Wave 2.5-B; Target: v2.5.0).
- [ ] Compare adaptive and fixed plans on identical data, including skew, drift, and missing-statistics cases (Wave 3.0-A; Target: v3.0.0).
- [ ] Add representative-duration federation/continuous-query benchmarks and operational metrics, separating synthetic sources from production-backed runs (Wave 3.0-B; Target: v3.0.0).

#### Phase 6 — Documentation & acceptance

- [ ] Document deliberately unsupported retry/partial-result behavior and record focused safety-test CI evidence against the wave success metrics (Wave 2.5-A; Target: v2.5.0).
- [ ] Record benchmark environment, run configuration, variance, and raw artifacts; assess QG-1..4 and p95/p99 outcomes against the existing policy (Wave 2.5-B; Target: v2.5.0).
- [ ] Publish reproducible adaptive-planning evaluation results and retain traces for independent review before generalizing findings (Wave 3.0-A; Target: v3.0.0).
- [ ] Update canonical migration/release documents for any adopted API or language changes, and review module boundaries before release sign-off (Wave 3.0-B; Target: v3.0.0).

### Assumptions and open gaps

- Planning targets name release lines, not dates. Maintainers must approve ownership, capacity, milestone windows, acceptance thresholds, and whether each wave belongs in the corresponding release.
- Existing source/tests are evidence of implementation and intended coverage only. No tests, benchmarks, security review, or TPC-H runs were executed for this roadmap update; runtime/CI status must be attached by the owning implementation work.
- Representative hardware, production-like datasets, baseline artifacts, and a reproducible real-engine parse/plan/execute benchmark remain open evidence needs. The dedicated stub benchmark is not an acceptable substitute.
- Public API compatibility, federation partial-result semantics, cross-module dependencies, and the precise v3.0.0 feature boundary require design/owner review before implementation commitments.
- Query receives storage/index and ingress dependencies; this plan does not assign query ownership for persistence, index maintenance, or network admission.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `query`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module — `tests/query/test_query_highcardinality_stress.cpp` (HighCardinalityQueryParsing, ConcurrentPlannerStress, ComplexJoinQueryStress) delivered; `docs/operability/RUNBOOK_QUERY_ENGINE.md` (5 scenarios: PlannerOOM, ExecutorDeadlock, PlanCacheCorruption, SlowQuery, JoinExplosion) delivered (Delivered: 2026-09-16)
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths — `tests/integration/test_query_engine_soak.cpp` (QuerySoak_ParsePlanExecuteThroughput ≥2000 q/s, QuerySoak_PlannerStability, QuerySoak_ConcurrentExecutionReliability) delivered (Delivered: 2026-09-16)
- [x] Ensure runbook coverage for operator-critical scenarios in this module — `docs/operability/RUNBOOK_QUERY_ENGINE.md` with 5 incident classes and log patterns (Delivered: 2026-09-16)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed — existing error handling confirmed; `[QUERY:PlannerOOM]`, `[QUERY:ExecutorDeadlock]`, `[QUERY:JoinExplosion]` runbook paths documented (Delivered: 2026-09-16)
- [ ] Benchmark-backed p95/p99 baselines on representative hardware — open; `benchmarks/query/bench_query_dedicated_gates.cpp` (QE-BM-01..04) uses in-process stubs and is not engine-performance evidence. Refresh with a real-engine path (Target: Q1 2027)
- [x] Operator-critical paths have diagnostics, alerts, and runbooks — `docs/operability/RUNBOOK_QUERY_ENGINE.md` with 5 scenarios, log patterns, and remediation tables (Delivered: 2026-09-16)
