# Phase 3: Graph Module Optimization & Hardening — Implementation Plan

**Status:** 🔵 PLANNED (Kickoff: 2026-07-22)  
**Timeline:** 2026-07-22 to 2026-08-19 (4 weeks active + 1 week sign-off)  
**Release Target:** v1.9.0-beta (Q4 2026)

---

## Scope & Objectives

### High-Level Scope

Build on the completion of Graph Phase 2.4 (production-ready, 326 tests, 0 CRITICAL gaps) by adding three critical optimization and hardening layers:

1. **Query Optimizer Hardening** — Plan cache + cost model optimization
2. **Cache Efficiency** — LRU + multi-tier eviction strategies
3. **Resource Management** — Connection, thread, and buffer pool orchestration
4. **Load Balancing & Scheduling** — Query distribution across shards/partitions

### Success Criteria

| Criterion | Measurement | Target | Status |
|-----------|-------------|--------|--------|
| **Test Coverage** | New tests passing + no regressions | 130 new + 326 existing = 456 total | 🔵 Planned |
| **Query Latency** | p99 latency on synthetic workloads | ≤ 50ms (baseline TBD week 1) | 🔵 Planned |
| **Cache Efficiency** | Hit ratio on production-like workloads | ≥ 85% | 🔵 Planned |
| **Resource Utilization** | Connection pool saturation + thread efficiency | ≤ 80% peak utilization | 🔵 Planned |
| **Code Quality** | CRITICAL + HIGH scanner findings | 0 new CRITICAL, < 5 new HIGH | 🔵 Planned |
| **Performance Regression** | vs. Phase 2.4 baseline on Wave 7 gates | All gates must stay PASS | 🔵 Planned |

---

## Deliverables Breakdown

### P3-01: Query Optimizer Hardening (2 weeks, Team A)

**Target Files:**
- `src/query/query_planner.h/cc` — Plan cache, cost model
- `src/query/query_optimizer.h/cc` — Optimization rules
- `tests/epic2_evaluation/query_planner_test.cc` — New tests

**Current State (from Phase 2.5):**
- Query planner framework exists with 46 passing tests
- Plan cache: basic skeleton, needs LRU implementation
- Cost model: heuristic-only, no cost vectors

**Tasks:**

| Task | Description | Effort | Owner | Gate |
|------|-------------|--------|-------|------|
| P3-01-A | Implement plan cache with LRU eviction | 3 days | A1 | 6 tests |
| P3-01-B | Add cost model refinement (cardinality + selectivity) | 3 days | A1 | 8 tests |
| P3-01-C | Integrate cache with query execution path | 2 days | A2 | 4 tests |
| P3-01-D | Benchmark + tune cache hit ratio (target 80%+) | 2 days | A2 | 4 tests |
| P3-01-E | Performance regression testing vs. baseline | 2 days | A1 | 6 tests |

**Acceptance Criteria:**
- ✅ 28 new tests (6+8+4+4+6 = 28) all passing
- ✅ Plan cache hit ratio ≥ 80% on YCSB workload
- ✅ Query latency p99 improved by >= 10% (vs. Phase 2.4)
- ✅ No new CRITICAL scanner findings
- ✅ Doxygen complete for new public APIs

**Implementation Notes:**
- Reference: `src/query/query_planner.cc:PlanCache` (existing skeleton)
- Use `std::map<hash, CachedPlan>` with timestamp-based LRU
- Cost model vectors: `vector<SelectivityCost>` per table/join
- Benchmark: `benchmarks/bench_query_optimization.cpp` (to be created)

---

### P3-02: Cache Efficiency — Multi-Tier Eviction (1.5 weeks, Team A)

**Target Files:**
- `src/cache/cache_manager.h/cc` — Multi-tier eviction orchestrator
- `src/cache/lru_eviction_policy.h/cc` — LRU implementation
- `tests/cache/test_cache_efficiency.cpp` — New tests

**Current State:**
- Basic cache manager exists (Phase 4 hardening complete)
- Single-tier LRU: basic FIFO fallback
- No eviction policy composition

**Tasks:**

| Task | Description | Effort | Owner | Gate |
|------|-------------|--------|-------|------|
| P3-02-A | Design multi-tier eviction (hot/warm/cold tiers) | 1 day | A1 | Design doc |
| P3-02-B | Implement LRU eviction with tier awareness | 3 days | A2 | 10 tests |
| P3-02-C | Add weighted scoring (frequency + recency) | 2 days | A1 | 8 tests |
| P3-02-D | Eviction trigger + adaptive threshold tuning | 2 days | A2 | 6 tests |
| P3-02-E | Integration testing (cache + executor pipeline) | 2 days | A1 | 8 tests |

**Acceptance Criteria:**
- ✅ 32 new tests (10+8+6+8 = 32) all passing
- ✅ Cache hit ratio maintained ≥ 85% across workload phases
- ✅ Memory usage stable (no unbounded growth)
- ✅ Eviction latency < 1ms (p99)
- ✅ Doxygen + architecture doc (CACHE_EFFICIENCY.md)

**Implementation Notes:**
- Tier scoring: `score = freq_weight * freq + recency_weight * recency` (tunable weights)
- Eviction threshold: 70% capacity (trigger at this point)
- Benchmark: Profile on graph workload (nodes + edges iteration)

---

### P3-03: Resource Pooling — Connection/Thread/Buffer Managers (2 weeks, Team B)

**Target Files:**
- `src/base/resource_pool_manager.h/cc` — Unified pool orchestration
- `src/network/connection_pool.h/cc` — Connection pool hardening
- `src/execution/thread_pool_manager.h/cc` — Thread pool tuning
- `src/base/buffer_pool.h/cc` — Buffer allocation strategy
- `tests/integration/test_resource_pooling.cpp` — New tests

**Current State:**
- Connection pool: basic implementation, no adaptive sizing
- Thread pool: fixed-size thread pool with queue
- Buffer pool: naive malloc (not pooled)

**Tasks:**

| Task | Description | Effort | Owner | Gate |
|------|-------------|--------|-------|------|
| P3-03-A | Design unified resource pool interface + orchestrator | 1 day | B1 | Design doc |
| P3-03-B | Implement adaptive connection pool (min/max dynamic adjustment) | 3 days | B1 | 8 tests |
| P3-03-C | Implement thread pool tuning (work-stealing + backpressure) | 3 days | B2 | 10 tests |
| P3-03-D | Implement buffer pool (slab allocator for fixed-size blocks) | 2 days | B1 | 6 tests |
| P3-03-E | Integration + saturation testing (concurrent load) | 2 days | B2 | 8 tests |
| P3-03-F | Performance tuning + profiling (peak utilization ≤ 80%) | 1 day | B1 | 4 tests |

**Acceptance Criteria:**
- ✅ 36 new tests (8+10+6+8+4 = 36) all passing
- ✅ Connection pool: min=5, max=50, scale-up latency < 10ms
- ✅ Thread pool: work-stealing queue, latency p99 < 5ms
- ✅ Buffer pool: > 90% reuse rate (vs. malloc/free baseline)
- ✅ Peak resource utilization ≤ 80% under synthetic load
- ✅ Doxygen + RESOURCE_POOLING.md architecture doc

**Implementation Notes:**
- Adaptive sizing: Measure pool wait times; grow if wait > 1ms consistently
- Thread pool: Use `tbb::task_scheduler_init` for dynamic work stealing
- Buffer pool: Segregated-fit allocator (128B, 256B, 512B, 1KB, 2KB, 4KB classes)
- Benchmark: Concurrent query load (100 concurrent clients, ramp-up test)

---

### P3-04: Load Balancing & Query Scheduling (1.5 weeks, Team B)

**Target Files:**
- `src/execution/query_scheduler.h/cc` — Query distribution orchestrator
- `src/sharding/shard_load_balancer.h/cc` — Load balancing strategy
- `tests/integration/test_load_balancing.cpp` — New tests

**Current State:**
- No centralized scheduler (queries routed immediately)
- Shard selection: round-robin (not load-aware)
- No query prioritization

**Tasks:**

| Task | Description | Effort | Owner | Gate |
|------|-------------|--------|-------|------|
| P3-04-A | Design query scheduler with priority queue | 1 day | B2 | Design doc |
| P3-04-B | Implement load-aware shard selection (via shard metrics) | 2 days | B1 | 6 tests |
| P3-04-C | Implement query prioritization (SLA-aware scheduling) | 2 days | B2 | 8 tests |
| P3-04-D | Integrate scheduler with executor pipeline | 2 days | B1 | 4 tests |
| P3-04-E | Load-balancing benchmarks (uneven workload scenarios) | 1 day | B2 | 6 tests |

**Acceptance Criteria:**
- ✅ 24 new tests (6+8+4+6 = 24) all passing
- ✅ Query latency p99: ≤ 50ms (vs. Phase 2.4 baseline)
- ✅ Cross-shard query distribution: ≤ 10% variance in response times
- ✅ SLA compliance: 99% of queries complete within SLA window
- ✅ Doxygen + QUERY_SCHEDULING.md architecture doc

**Implementation Notes:**
- Scheduler: Priority queue with SLA expiry timestamps
- Shard load: Metric = CPU% + pending_queries + response_time_p99
- Load balancing: Select shard with lowest score (min_score = 0, max = 100)
- Benchmark: Zipf-distributed workload (some shards hot, others cold)

---

### P3-05: Integration, Performance Benchmarks & Sign-Off (1 week, Team A+B)

**Target Files:**
- `benchmarks/bench_phase3_optimization.cpp` — New Phase 3 benchmarks
- `ai_working/PHASE3_OPTIMIZATION_REPORT.md` — Final verification report

**Tasks:**

| Task | Description | Effort | Owner | Gate |
|------|-------------|--------|-------|------|
| P3-05-A | Re-run Wave 7 gates (all 6 must stay PASS) | 2 days | A1 | All gates ✅ |
| P3-05-B | Performance regression testing (vs. Phase 2.4) | 2 days | A2+B1 | No regressions |
| P3-05-C | Integration testing (Phase 3 components together) | 2 days | B2 | 12 integration tests |
| P3-05-D | Code review + scanner audit (zero new CRITICAL) | 2 days | Lead review | Scanner PASS |
| P3-05-E | Doxygen audit + documentation completeness | 1 day | A1 | Doxygen clean |
| P3-05-F | Performance report + optimization insights | 1 day | A2 | Insights doc |

**Acceptance Criteria:**
- ✅ All 456 graph tests passing (326 + 130 new)
- ✅ Wave 7 gates re-verified (read, write, range, batch all ≤ thresholds)
- ✅ No performance regressions vs. Phase 2.4
- ✅ CRITICAL/HIGH scanner findings: 0 new CRITICAL, < 5 new HIGH
- ✅ Doxygen audit: 0 warnings, 100% API documentation
- ✅ Final sign-off from tech lead + release manager

**Deliverables:**
- `PHASE3_OPTIMIZATION_REPORT.md` — Full results, timings, recommendations
- Benchmark data: CSV files with performance profiles
- Performance graphs: Query latency, cache hit ratio, resource utilization

---

## Test Infrastructure

### New Test Files (by component)

| File | Component | Tests | Responsibility |
|------|-----------|-------|-----------------|
| `tests/epic2_evaluation/test_query_planner_cache.cpp` | P3-01 | 28 | Team A |
| `tests/cache/test_cache_efficiency_multilevel.cpp` | P3-02 | 32 | Team A |
| `tests/integration/test_resource_pooling.cpp` | P3-03 | 36 | Team B |
| `tests/integration/test_load_balancing.cpp` | P3-04 | 24 | Team B |
| `benchmarks/bench_phase3_optimization.cpp` | P3-05 | 12 integration | A+B |

**Total: 132 tests** (exceeds 130 target by 2 tests)

### Test Execution Strategy

```bash
# Phase 3 focused tests
ctest --preset community-release -L phase3 --output-on-failure -j 4 --timeout 120

# Full graph test suite (regression check)
ctest --preset community-release -L graph --output-on-failure -j 4 --timeout 120

# Wave 7 gates re-verification
cd benchmarks && python benchmarks/wave7/report_variance_w7.py --gate-only
```

### CI Integration

- Add GitHub Actions workflow: `.github/workflows/phase3-optimization-tests.yml`
- Trigger on PR to `feature/graph-phase3-optimization` branch
- Run all 456 graph tests + Wave 7 re-pass
- Fail if any CRITICAL scanner findings detected

---

## Performance Baselines (to be measured Week 1)

### Baseline Measurement Schedule

**Week 1 (2026-07-22 to 2026-07-26):**

```
MON: Measure query planner (p50, p95, p99) before optimization
     Current baseline TBD via benchmarks/bench_query_optimization.cpp
     
TUE: Measure cache hit ratio (synthetic workload)
     Current baseline TBD via synthetic YCSB workload simulation
     
WED: Measure resource pool saturation (connection, thread, buffer)
     Connection pool: target <= 80% utilization
     Thread pool: latency p99 < 5ms
     
THU: Measure load balancing skew (across shards)
     Target: <= 10% variance in response times
     
FRI: Consolidate all baselines in PHASE3_BASELINE.md
```

### Target Improvements (vs. Phase 2.4)

| Metric | Phase 2.4 Baseline | Phase 3 Target | Target Delta |
|--------|-------------------|----------------|--------------|
| Query latency p99 | TBD (measure week 1) | ≤ 50ms | TBD |
| Cache hit ratio | ~40% (estimated) | ≥ 85% | +45 pct points |
| Resource pool utilization | Peak 95% | ≤ 80% | -15 pct points |
| Cross-shard latency variance | TBD (measure week 1) | ≤ 10% | TBD |

---

## Team Structure & Responsibilities

### Team A: Query Optimizer + Cache Efficiency (2 engineers)
- **A1 (Lead):** P3-01 (Query optimizer), P3-02 (Cache efficiency lead)
- **A2 (Member):** P3-01 (cache integration), P3-02 (cache tuning), P3-05 (benchmarks)

### Team B: Resource Management + Load Balancing (2 engineers)
- **B1 (Lead):** P3-03 (Resource pools lead), P3-04 (shard selection)
- **B2 (Member):** P3-03 (thread pool + buffer pool), P3-04 (priority queue), P3-05 (integration)

### Collaboration Points
- **Weekly sync:** Mondays 10am (all hands) — status + blockers
- **Shared resources:** Graph module test infrastructure (existing)
- **Cross-team review:** All PRs reviewed by member of other team

---

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Query optimizer regression (slower queries) | Medium | High | Intensive perf testing; Wave 7 gates as regression detector; revert capability in < 1h |
| Cache eviction thrashing (high CPU) | Medium | Medium | Profiling during Week 3; adaptive thresholds tuned via benchmark |
| Thread pool deadlock under contention | Low | Critical | Deadlock detection tests; chaos engineering (thread cancellation) |
| Resource pool exhaustion (OOM) | Low | Critical | Memory limits enforced; pool saturation alerts in code |
| Load balancing bimodal latency | Low | Medium | Continuous monitoring; cross-shard latency anomaly detection |

---

## Acceptance & Sign-Off Gate

### Release Gate Checklist (all must be ✅)

- [ ] All 456 graph tests passing (0 flakes in 3 consecutive runs)
- [ ] Wave 7 gates re-verified: read p99 ≤ 200µs, write ≥ 80k ops/s, range p99 ≤ 500µs, batch p99 ≤ 5ms
- [ ] Query latency p99 ≤ 50ms (vs. baseline)
- [ ] Cache hit ratio ≥ 85% on production-like workload
- [ ] Resource pool utilization ≤ 80% peak
- [ ] Cross-shard load balancing variance ≤ 10%
- [ ] Scanner audit: 0 new CRITICAL, < 5 new HIGH findings
- [ ] Doxygen audit: 0 warnings
- [ ] Code review: >= 1 peer review from different team
- [ ] Security review: No vulnerabilities introduced
- [ ] Performance report delivered (with insights + recommendations)

### Sign-Off Authorities
- **Tech Lead:** Code quality + architecture
- **Release Manager:** Timeline + acceptance criteria
- **Performance Lead:** Benchmark results + improvements

---

## References & Evidence

### Current State Documentation
- Phase 2.4 completion: [ROADMAP.md](../../ROADMAP.md) (lines 224-352)
- Graph module status: [src/graph/ROADMAP.md](../../src/graph/ROADMAP.md)
- Query planner: [src/evaluation/ROADMAP.md](../../src/evaluation/ROADMAP.md) (EPIC 2.5)

### Benchmark References
- Wave 7 suite: [benchmarks/wave7/](../../benchmarks/wave7/)
- Measurement hygiene: [benchmarks/MEASUREMENT_HYGIENE.md](../../benchmarks/MEASUREMENT_HYGIENE.md)

### Related Issues
- GitHub Issue (to be created): #XXXX — Phase 3: Graph Optimization & Hardening Epic
- Sub-issues (to be created): #XXXX-01 (Query Optimizer), #XXXX-02 (Cache), #XXXX-03 (Resource Pools), #XXXX-04 (Load Balancing), #XXXX-05 (Sign-Off)

---

## Next Steps

1. **Week 1 (2026-07-22):** Kickoff meeting, baseline measurements, create GitHub issues & branches
2. **Week 2-3:** Parallel implementation (Teams A+B working independently)
3. **Week 4:** Integration + performance regression testing
4. **Week 5:** Final verification + sign-off

**Kickoff Actions (Monday 2026-07-22):**
- [ ] Assign Team A & B members
- [ ] Create `feature/graph-phase3-optimization` branch
- [ ] Create GitHub issues #XXXX-01 through #XXXX-05
- [ ] Setup benchmark environment (Wave 7 re-run capability)
- [ ] Schedule weekly sync meetings (Mondays 10am)

---

**Document Owner:** Tech Lead  
**Last Updated:** 2026-07-18  
**Approval:** Pending architecture review (scheduled 2026-07-22)
