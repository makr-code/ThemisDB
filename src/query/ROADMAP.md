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
  - [x] Geospatial parser Phase 1 COMPLETE 2026-07-27: ST_* already work in FILTER/SORT/RETURN via qe_evalFunction; 26 tests in test_aql_st_predicates.cpp; Phase 2 (optimizer hints) next (Target: Q3 2026)
  - [ ] FTS query enhancement (phrase/proximity queries; ≤100ms on 100K documents) (Target: Q3–Q4 2026)
  - [ ] Cross-feature integration tests (1000+ tests, zero v1.x regressions) (Target: Q4 2026)

## Phase 2 — Performance & Scalability Readiness (Target: 2026-09-30)

Performance and scalability optimization with measurable, gated performance improvements validated against Wave 7 gates.

### Graph/Query Optimization Backlog

**Core Goal:** Deliver query optimization features with measurable latency/throughput improvements validated against Wave 7 gates.

#### 1. Query Plan-Cache Implementation (Target: Q3 2026)

**Scope:** src/query/optimizer/ (plan caching subsystem)

- [ ] Implement query plan caching mechanism (Target: Q3 2026)
  - Design: LRU cache for query plans (size: tunable, default 1000 plans)
  - Key: normalized query text + parameter types
  - Value: compiled execution plan
  - Invalidation: on schema changes, statistics updates

- [x] Add plan reuse validation (Target: Q3 2026)
  - Detect plan invalidation conditions (schema DDL, index drops)
  - Implement cache invalidation with precise granularity (table-level)
  - Log cache invalidations for diagnostics

- [ ] Performance gate: 10%+ latency improvement (Target: Q3 2026)
  - Measure: 10%+ latency improvement for repeated queries
  - Validate: Run Wave 7 bench_w7a_release_critical_signoff.cpp
  - Baseline: read p99≤200µs, batch p99≤5ms

- [ ] Add stress tests (Target: Q3 2026)
  - Cache coherency under concurrent queries (CMX-style tests)
  - Cache invalidation under concurrent DDL
  - Cache eviction under plan explosion

#### 2. Cost-Model Refinement (Target: Q3 2026)

**Scope:** src/query/optimizer/cost_model.h/.cpp

- [ ] Improve cardinality estimation (Target: Q3 2026)
  - Implement histogram-based estimation (if not present)
  - Use table statistics (row count, column selectivity)
  - Add correlation awareness (multi-column predicates)

- [ ] Implement cost-based join ordering (Target: Q3 2026)
  - Compare: nested-loop vs. hash-join vs. merge-join costs
  - Select: lowest-cost join order
  - Validate: execution stats match estimates

- [ ] Add estimate validation (Target: Q3 2026)
  - Log estimate vs. actual cardinality
  - Compute estimation error metrics (mean, p95)
  - Flag systematic underestimation/overestimation

- [ ] Create diagnostics (Target: Q3 2026)
  - EXPLAIN output shows cardinality estimates
  - Warning: large estimation errors (>2x deviation)
  - Recommend: statistics refresh if needed

#### 3. Cache-Efficiency Improvements (Target: Q3 2026)

**Scope:** src/storage/buffer_pool/, src/query/ (cache-aware optimizations)

- [ ] Profile and optimize buffer-pool hit rate (Target: Q3 2026)
  - Measure: cache hit rate before/after optimization
  - Implement: cache-aware prefetching (sequential access patterns)
  - Optimize: LRU eviction policy tuning

- [ ] Implement cache-aware data layout (Target: Q3 2026)
  - Co-locate frequently-joined columns (reduce cache misses)
  - Minimize cache line waste (column grouping)
  - Validate: latency improvement under scan workloads

- [ ] Add cache-pressure telemetry (Target: Q3 2026)
  - Track: cache miss rate, eviction rate, pressure level
  - Implement: adaptive eviction (age-based, frequency-based)
  - Alert: high pressure conditions

- [ ] Gate on performance (Target: Q3 2026)
  - Measure: p99 latency for range queries < 500µs (Wave 7)
  - Validate: no regression under scan workloads
  - Document: cache efficiency metrics in ROADMAP.md

#### 4. Resource-Pooling & Load-Balancing (Target: Q3 2026)

**Scope:** src/scheduler/, src/network/ (thread pools, connection pools)

- [ ] Consolidate thread-pool management (Target: Q3 2026)
  - Audit: existing thread pools across modules
  - Implement: centralized pool manager (if not present)
  - Standardize: pool sizing, queue depths, overflow handling

- [ ] Implement adaptive pool sizing (Target: Q3 2026)
  - Measure: queue depth, wait times, CPU utilization
  - Adjust: pool size based on workload pressure
  - Implement: gradual scaling (avoid thrashing)

- [ ] Add connection pool rebalancing (Target: Q3 2026)
  - Distribute: connections evenly across backend shards
  - Rebalance: on shard failure/recovery
  - Validate: no starvation under uneven load

- [ ] Gate on throughput (Target: Q3 2026)
  - Measure: sustained throughput ≥ 80k ops/sec (Wave 7)
  - Validate: write latency stable under peak load
  - Document: pool sizing recommendations

### Performance Regression Gates (Target: 2026-09)

1. **Execute Wave-7 Full Suite:**
   - [ ] bench_w7a_release_critical_signoff (RCS-01..08) (Target: 2026-09)
   - [ ] bench_w7b_endurance_soak (SOK-01..08) (Target: 2026-09)
   - [ ] bench_w7c_degradation_fault_recovery (DFR-01..08) (Target: 2026-09)
   - [ ] bench_w7d_guardrails_variance_operability (GVO-01..08) (Target: 2026-09)

2. **Validate Gate Status:** (Target: 2026-09)
   - [ ] GATE-W7-01: Read p99 ≤ 200µs ✓ (Target: 2026-09)
   - [ ] GATE-W7-02: Write ≥ 80k ops/s ✓ (Target: 2026-09)
   - [ ] GATE-W7-03: Range p99 ≤ 500µs ✓ (Target: 2026-09)
   - [ ] GATE-W7-04: Batch p99 ≤ 5ms ✓ (Target: 2026-09)
   - [ ] GATE-W7-05/06: Self-check counters ✓ (Target: 2026-09)

3. **Baseline Variance Report:** (Target: 2026-09)
   - [ ] Use: benchmarks/wave7/report_variance_w7.py (Target: 2026-09)
   - [ ] Measure: p50, p95, p99, max latencies (Target: 2026-09)
   - [ ] Document: variance envelope and drift tolerance (Target: 2026-09)

### Repeatable Under-Load Results (Target: 2026-09)

1. **Endurance Benchmark Suite:** (Target: 2026-09)
   - [ ] Duration: 8+ hour soak tests (Target: 2026-09)
   - [ ] Workload: Mix of read/write/range/batch operations (Target: 2026-09)
   - [ ] Load: Realistic production intensity (Target: 2026-09)

2. **Stability Measurements:** (Target: 2026-09)
   - [ ] Track: p95/p99 latency over time (Target: 2026-09)
   - [ ] Detect: performance drift or degradation (Target: 2026-09)
   - [ ] Alert: significant variance (>10% change) (Target: 2026-09)

3. **Archive Results:** (Target: 2026-09)
   - [ ] Save: benchmark data for release sign-off (Target: 2026-09)
   - [ ] Document: latency/throughput envelopes (Target: 2026-09)
   - [ ] Baseline: for regression detection (Target: 2026-09)

### Phase 2 Exit Criteria

- [ ] Query plan-cache: ✓ implemented, 10%+ improvement gated (Target: 2026-09)
- [ ] Cost-model: ✓ cardinality estimation validated, joins optimized (Target: 2026-09)
- [ ] Cache-efficiency: ✓ hit rate improved, prefetching added (Target: 2026-09)
- [ ] Resource pooling: ✓ consolidated, adaptive sizing functional (Target: 2026-09)
- [ ] Wave 7 gates: ✓ all 6 PASS, no regressions (Target: 2026-09)
- [ ] 130+ new unit/integration tests created and passing (Target: 2026-09)
- [ ] Performance envelopes documented and baselined (Target: 2026-09)

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
