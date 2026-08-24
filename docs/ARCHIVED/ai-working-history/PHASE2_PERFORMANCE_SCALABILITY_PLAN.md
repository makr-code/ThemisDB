# Phase 2 — Performance & Scalability Readiness (2026-Q3)

**Status:** 🟡 In Planning  
**Target Delivery:** 2026-09-30  
**Owner:** Query Optimization Team  
**Last Updated:** 2026-07-28

---

## Executive Summary

Phase 2 focuses on query optimization backlog with measurable, gated performance improvements. Four major optimization areas target Wave 7 performance gates:

1. **Query Plan-Cache** — 10%+ latency improvement for repeated queries
2. **Cost-Model Refinement** — Better cardinality estimation, cost-based join ordering
3. **Cache-Efficiency** — Buffer pool hit rate optimization, cache-aware data layout
4. **Resource Pooling** — Centralized thread/connection pool management with adaptive sizing

**Success Metrics:**
- ✅ All 6 Wave 7 gates PASS (read p99≤200µs, write ≥80k ops/s, range p99≤500µs, batch p99≤5ms)
- ✅ 130+ new unit/integration tests passing
- ✅ Performance envelopes documented and baselined
- ✅ No performance regressions

---

## Work Breakdown Structure (WBS)

### WB1: Query Plan-Cache Implementation

**Scope:** `src/query/optimizer/` (plan caching subsystem)

| Task | Owner | Status | Est. Days | Target |
|------|-------|--------|----------|--------|
| Design LRU cache mechanism | - | ⏳ | 3 | 2026-08-15 |
| Implement plan caching | - | ⏳ | 5 | 2026-08-22 |
| Add plan reuse validation | - | ⏳ | 4 | 2026-08-29 |
| Cache invalidation logic | - | ⏳ | 3 | 2026-09-05 |
| Stress tests (concurrency/DDL) | - | ⏳ | 4 | 2026-09-12 |
| Wave 7 gate validation | - | ⏳ | 2 | 2026-09-20 |
| **Subtotal** | | | **21** | |

**Acceptance Criteria:**
- [ ] LRU cache (size: tunable, default 1000 plans) implemented
- [ ] Key: normalized query text + parameter types
- [ ] Value: compiled execution plan
- [ ] Cache invalidation on schema changes, statistics updates
- [ ] 10%+ latency improvement for repeated queries (Wave 7: bench_w7a_release_critical_signoff.cpp)
- [ ] Read p99 ≤ 200µs, Batch p99 ≤ 5ms baseline achieved
- [ ] Concurrent query cache coherency validated
- [ ] Concurrent DDL cache invalidation validated
- [ ] Plan explosion cache eviction validated

---

### WB2: Cost-Model Refinement

**Scope:** `src/query/optimizer/cost_model.h/.cpp`

| Task | Owner | Status | Est. Days | Target |
|------|-------|--------|----------|--------|
| Histogram-based cardinality estimation | - | ⏳ | 5 | 2026-08-22 |
| Multi-column correlation awareness | - | ⏳ | 4 | 2026-08-29 |
| Cost-based join ordering | - | ⏳ | 6 | 2026-09-12 |
| Estimate vs. actual validation | - | ⏳ | 3 | 2026-09-19 |
| EXPLAIN diagnostics output | - | ⏳ | 3 | 2026-09-26 |
| **Subtotal** | | | **21** | |

**Acceptance Criteria:**
- [ ] Histogram-based estimation (if not present) implemented
- [ ] Table statistics used (row count, column selectivity)
- [ ] Multi-column predicate correlation awareness added
- [ ] Cost comparison: nested-loop vs. hash-join vs. merge-join
- [ ] Lowest-cost join order selected
- [ ] Execution stats match cardinality estimates (validation)
- [ ] Estimate error metrics computed (mean, p95)
- [ ] Systematic over/underestimation flagged
- [ ] EXPLAIN output shows cardinality estimates
- [ ] Estimation error warnings (>2x deviation)
- [ ] Statistics refresh recommendations provided

---

### WB3: Cache-Efficiency Improvements

**Scope:** `src/storage/buffer_pool/`, `src/query/` (cache-aware optimizations)

| Task | Owner | Status | Est. Days | Target |
|------|-------|--------|----------|--------|
| Buffer-pool baseline measurement | - | ⏳ | 2 | 2026-08-10 |
| Cache-aware prefetching (seq access) | - | ⏳ | 5 | 2026-08-22 |
| LRU eviction policy tuning | - | ⏳ | 3 | 2026-08-29 |
| Cache-aware data layout (co-location) | - | ⏳ | 6 | 2026-09-12 |
| Cache-pressure telemetry & alerting | - | ⏳ | 4 | 2026-09-19 |
| Adaptive eviction (age/frequency) | - | ⏳ | 3 | 2026-09-26 |
| Wave 7 range query validation | - | ⏳ | 2 | 2026-09-30 |
| **Subtotal** | | | **25** | |

**Acceptance Criteria:**
- [ ] Cache hit rate measured (baseline + optimized)
- [ ] Cache-aware prefetching for sequential access implemented
- [ ] LRU eviction policy tuned
- [ ] Frequently-joined columns co-located
- [ ] Cache line waste minimized (column grouping)
- [ ] Latency improvement validated under scan workloads
- [ ] Cache miss rate tracked and reported
- [ ] Eviction rate monitored
- [ ] Pressure level alerting active
- [ ] Adaptive eviction (age-based, frequency-based) functional
- [ ] Wave 7 gate: p99 range query latency < 500µs validated
- [ ] No regression under scan workloads
- [ ] Cache efficiency metrics documented in ROADMAP.md

---

### WB4: Resource-Pooling & Load-Balancing

**Scope:** `src/scheduler/`, `src/network/` (thread pools, connection pools)

| Task | Owner | Status | Est. Days | Target |
|------|-------|--------|----------|--------|
| Thread pool audit across modules | - | ⏳ | 2 | 2026-08-10 |
| Centralized pool manager design | - | ⏳ | 3 | 2026-08-15 |
| Centralized pool manager implementation | - | ⏳ | 5 | 2026-08-29 |
| Adaptive pool sizing logic | - | ⏳ | 4 | 2026-09-12 |
| Connection pool rebalancing | - | ⏳ | 4 | 2026-09-19 |
| Throughput validation & tuning | - | ⏳ | 3 | 2026-09-26 |
| **Subtotal** | | | **21** | |

**Acceptance Criteria:**
- [ ] Existing thread pools audited across modules
- [ ] Centralized pool manager implemented (if not present)
- [ ] Pool sizing, queue depths, overflow handling standardized
- [ ] Queue depth, wait times, CPU utilization measured
- [ ] Pool size adaptive adjustment based on workload pressure
- [ ] Gradual scaling (avoid thrashing) implemented
- [ ] Connections distributed evenly across backend shards
- [ ] Rebalancing on shard failure/recovery functional
- [ ] No starvation under uneven load validated
- [ ] Wave 7 gate: sustained throughput ≥ 80k ops/sec achieved
- [ ] Write latency stable under peak load
- [ ] Pool sizing recommendations documented

---

### WB5: Performance Regression Gates

**Scope:** Wave 7 benchmark suite validation

| Task | Owner | Status | Est. Days | Target |
|------|-------|--------|----------|--------|
| Execute bench_w7a (RCS-01..08) | - | ⏳ | 3 | 2026-09-15 |
| Execute bench_w7b (SOK-01..08) | - | ⏳ | 5 | 2026-09-20 |
| Execute bench_w7c (DFR-01..08) | - | ⏳ | 3 | 2026-09-25 |
| Execute bench_w7d (GVO-01..08) | - | ⏳ | 2 | 2026-09-28 |
| Variance analysis & reporting | - | ⏳ | 2 | 2026-09-30 |
| **Subtotal** | | | **15** | |

**Acceptance Criteria (Gate Validation):**
- [ ] GATE-W7-01: Read p99 ≤ 200µs ✓
- [ ] GATE-W7-02: Write ≥ 80k ops/s ✓
- [ ] GATE-W7-03: Range p99 ≤ 500µs ✓
- [ ] GATE-W7-04: Batch p99 ≤ 5ms ✓
- [ ] GATE-W7-05/06: Self-check counters ✓
- [ ] Baseline variance report generated (p50, p95, p99, max)
- [ ] Variance envelope documented
- [ ] Drift tolerance defined and validated

---

### WB6: Repeatable Under-Load Results

**Scope:** Endurance & stability testing

| Task | Owner | Status | Est. Days | Target |
|------|-------|--------|----------|--------|
| Endurance soak setup (8+ hrs) | - | ⏳ | 2 | 2026-09-20 |
| Read/Write/Range/Batch workload mix | - | ⏳ | 3 | 2026-09-25 |
| p95/p99 latency tracking over time | - | ⏳ | 2 | 2026-09-28 |
| Performance drift detection | - | ⏳ | 2 | 2026-09-29 |
| Variance alerting (>10% change) | - | ⏳ | 1 | 2026-09-30 |
| Archive results & baseline creation | - | ⏳ | 1 | 2026-09-30 |
| **Subtotal** | | | **11** | |

**Acceptance Criteria:**
- [ ] 8+ hour soak tests executed
- [ ] Realistic production-intensity workload applied
- [ ] Mix of read/write/range/batch operations
- [ ] p95/p99 latencies tracked continuously
- [ ] Performance drift/degradation detected
- [ ] Variance alerts active (>10% change)
- [ ] Benchmark data archived for release sign-off
- [ ] Latency/throughput envelopes documented
- [ ] Regression detection baseline established

---

## High-Level Timeline

```
August 2026:
  Week 1-2: Cache & Cost-Model baseline, design phase
  Week 2-3: Cache-Efficiency baseline, Pool audit
  Week 3-4: Core implementations begin
  Week 4  : Mid-phase gate reviews

September 2026:
  Week 1-2: Core implementations continue, early testing
  Week 2-3: Stress testing, gate validation begins
  Week 3-4: Wave 7 full suite execution
  Week 4  : Final validation, documentation, sign-off
```

**Critical Path:** WB2 (Cost-Model) + WB3 (Cache-Efficiency) + WB5 (Gates) = ~53 days

---

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Cost-model estimation inaccuracy | High | Early histogram implementation, continuous validation |
| Cache coherency issues under load | High | Extensive concurrent testing with chaos injection |
| Wave 7 gate regression | High | Continuous monitoring, rollback plan if needed |
| Resource pool deadlock | Medium | Exhaustive locking analysis, deadlock detector |
| Performance plateau | Medium | Profile-driven optimization, hypothesis-driven approach |

---

## Dependencies & Integration Points

- **Scheduler Module** (`src/scheduler/`) — Thread pool management
- **Network Module** (`src/network/`) — Connection pool management
- **Storage Module** (`src/storage/buffer_pool/`) — Buffer pool optimization
- **Benchmark Suite** (`benchmarks/wave7/`) — Gate validation infrastructure
- **Prometheus Metrics** — Telemetry collection

---

## Definition of Done (Phase 2 Exit Criteria)

- [x] Phase 2 section added to src/query/ROADMAP.md
- [ ] Query plan-cache: ✓ implemented, 10%+ improvement gated
- [ ] Cost-model: ✓ cardinality estimation validated, joins optimized
- [ ] Cache-efficiency: ✓ hit rate improved, prefetching added
- [ ] Resource pooling: ✓ consolidated, adaptive sizing functional
- [ ] Wave 7 gates: ✓ all 6 PASS, no regressions
- [ ] 130+ new unit/integration tests created and passing
- [ ] Performance envelopes documented and baselined
- [ ] Release sign-off from performance engineering team
- [ ] CHANGELOG.md updated with delivery summary

---

## Deliverables Checklist

- [ ] Query optimization implementations (4 modules: plan-cache, cost-model, cache-efficiency, pooling)
- [ ] Test suite (130+ focused tests: CACHE-01..20, OPT-01..30, POOL-01..20, E2E-QUERY-01..40)
- [ ] Wave-7 baseline validation report (4 benchmark suites)
- [ ] Performance envelope documentation (latency/throughput targets)
- [ ] Updated src/query/ROADMAP.md with Phase 2 completion status
- [ ] Release notes & sign-off certification

---

## Success Criteria

1. **Functionality:** All 4 optimization modules implemented and integrated
2. **Performance:** All Wave 7 gates ✓ PASS with no regressions
3. **Quality:** 130+ tests passing, comprehensive stress/endurance validation
4. **Documentation:** Performance targets, tuning assumptions, and roadmap all synchronized
5. **Operational:** Repeatable under-load results, baseline created for future regression detection

---

**Next Steps:**
1. ✅ Integrate Phase 2 specification into src/query/ROADMAP.md
2. ⏳ Assign task owners and refine effort estimates
3. ⏳ Create detailed design documents for each WB module
4. ⏳ Begin baseline measurements (WB3 cache baseline, WB4 pool audit)
5. ⏳ Establish continuous CI/CD gates for Wave 7 validation

---

**Document Version:** 1.0  
**Last Reviewed:** 2026-07-28  
**Status:** 🟡 Ready for Planning → In Progress Transition
