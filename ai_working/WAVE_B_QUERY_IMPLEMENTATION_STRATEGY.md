# Wave B Query Module: Implementation Strategy & Next Steps

**Date**: 2026-08-17  
**Status**: Wave A Complete, Wave B Ready for Kickoff  
**Author**: Copilot Agent  

---

## Executive Summary

Wave A Query Module implementation is **COMPLETE and VERIFIED**. This document outlines the Wave B execution strategy, including distributed execution baselines, hybrid planner integration, optimization gates, and benchmark consolidation.

### Wave A Final Status
✅ **READY FOR MERGE TO DEVELOP**
- 69+ HIGH-severity gaps fixed (93 total across 4 batches)
- All Wave A exit criteria met
- Code review complete with critical bug fix applied
- ROADMAP.md updated with completion status
- Merge plan documented in `WAVE_A_QUERY_MERGE_AND_WAVE_B_PLAN.md`

### Wave B Scope & Deliverables
- Distributed execution baselines (multi-shard routing profiles, failure recovery)
- Hybrid planner (ANN+graph single-shard integration)
- Optimization gates (vectorization efficiency, cache utilization)
- Performance benchmark consolidation (TPC-H, TPC-DS, reproducibility)

---

## Wave B Strategic Roadmap

### Overview

**Timeline**: Q3–Q4 2026 (16 weeks)  
**Total Effort**: ~115 hours (~29 hours/week)  
**Execution Model**: 4 sequential batches with overlap

### Wave B Exit Criteria (Gate to Wave C)

From ROADMAP.md §106-109:
- ✅ Full 4-layer retrieval chain has stable p95/p99 and bounded memory on representative hardware
- ✅ Access Model benchmark and observability gates are closed with reproducible evidence
- ✅ Release decisions are based on representative hardware baselines, not module-local scaffolding benchmarks

### Batch 2A: Distributed Execution Baselines (3-4 weeks, Sept 2026)

**Objective**: Establish federated query execution performance profiles and validate failure recovery SLAs

**Key Components**:
1. **Multi-shard Routing Profiler**
   - Query plan routing latency to N shards (N=1,3,5,10)
   - Per-shard execution time + aggregation overhead
   - Network serialization/deserialization cost
   - Goal: <10ms per-shard routing overhead

2. **Failure Recovery Baseline**
   - Shard failure detection time
   - Query fallback/retry latency
   - Partial result handling under ≥1 shard failure
   - Goal: <5s failover time, >95% success rate on retry

3. **Cross-Cluster Latency Model**
   - RTT characterization (LAN, WAN, geo-distributed)
   - Plan caching efficiency across clusters
   - Distributed join optimization baseline
   - Goal: Reproducible ±10% latency variance

**Acceptance Criteria**:
- [ ] Single-shard baseline: ≤10ms overhead per remote query (verified on 3+ hardware configs)
- [ ] Cross-shard (3 shards) baseline: ≤50ms RTT + plan overhead (< 100ms total)
- [ ] Partial failure recovery: ≤5s failover time (≥95% success on retry)
- [ ] Latency reproducibility: ±10% variance across 5 independent runs
- [ ] Documented in `src/query/WAVE_B_BASELINES.md` with hardware specs and test methodology

**Test Coverage**:
- 10+ latency profile tests (different shard counts)
- 5+ network fault injection tests (simulate shard unavailability)
- Chaos validation: 10,000+ concurrent queries with 5% failure rate
- Reproducibility: 5x runs per configuration with variance analysis

**Files to Create**:
- `src/query/distributed_execution_baselines.cpp` (profiler implementation, ~200 lines)
- `include/query/distributed_execution_baselines.h` (API, ~50 lines)
- `tests/query/test_query_distributed_baselines.cpp` (test suite, ~300 lines)
- `benchmarks/query/wave_b_distributed_baselines.cpp` (benchmark harness, ~150 lines)
- `src/query/WAVE_B_BASELINES.md` (documentation, ~200 lines)

**Effort**: 20 hours
**Deliverable**: Quantified federated execution profiles + failure recovery validation

---

### Batch 2B: Hybrid Planner Integration (5-6 weeks, Oct-Nov 2026)

**Objective**: Deliver single-shard ANN+graph hybrid planner with stable cost modeling and optimal plan selection

**Key Components**:
1. **ANN Index Costing**
   - Index lookup cost (time complexity: log N for balanced trees, sqrt N for HNSW)
   - Memory footprint estimation (vector + index structure overhead)
   - Cache hit rates on repeated lookups
   - Comparison baseline: full table scan cost

2. **Graph Traversal Costing**
   - Vertex/edge access patterns
   - Transitive closure estimation
   - Distributed join costing for graph edges
   - Cardinality estimation accuracy

3. **Hybrid Plan Selection**
   - ANN vs graph vs table scan comparison
   - Multi-level planning: local shard → cross-shard → aggregation
   - Cost model calibration on representative workloads
   - Deterministic plan selection (reproducible decisions)

**Acceptance Criteria**:
- [ ] ANN plans produce valid results (correctness: 100% match to baseline)
- [ ] ANN plans faster than table scan for ≥95% of ANN queries (TP50 latency)
- [ ] Graph plans correctly handle transitive closure (logic tests: acyclic + cyclic graphs)
- [ ] Hybrid plan selection deterministic across 100+ runs
- [ ] Single-shard TP99 latency ≤50ms on TPC-H-like workload (scale factor 1)
- [ ] Cost model variance <±15% vs observed execution time

**Test Coverage**:
- 20+ unit tests: cost model components (ANN, graph, table scan)
- 50+ integration tests: plan selection scenarios (different selectivities, workload mixes)
- 10+ chaos/stress tests: determinism under load (10,000+ concurrent queries)
- Benchmark against Wave 7 gates (TPC-H Queries 1-22)

**Files to Create/Modify**:
- `src/query/hybrid_planner_ann.cpp` (ANN costing, ~300 lines)
- `src/query/hybrid_planner_cost_model.cpp` (unified cost model, ~400 lines)
- `include/query/hybrid_planner.h` (extend with 2B APIs, +50 lines)
- `tests/query/test_query_hybrid_planner.cpp` (test suite, ~600 lines)
- `src/query/HYBRID_PLANNER_DESIGN_2B.md` (design doc, ~300 lines)

**Effort**: 40 hours
**Deliverable**: Single-shard hybrid planner with ≥95% query speedup and deterministic behavior

---

### Batch 2C: Parallel Optimization Gates (4-5 weeks, Oct-Nov 2026)

**Objective**: Enforce performance gates on vectorization efficiency, federated parallelization, and cache utilization

**Key Components**:
1. **Vectorization Profiler**
   - SIMD lane utilization (target: ≥50% active lanes)
   - Loop vectorization coverage analysis
   - Scalar fallback detection and measurement
   - Branch misprediction rates

2. **Federation Efficiency Gates**
   - Shard parallelization efficiency (target: ≥75% ideal speedup)
   - Network bandwidth utilization
   - Query plan decomposition overhead
   - Comparison: single-shard vs N-shard execution

3. **Cache Efficiency Analysis**
   - L1/L2/L3 cache miss rates (target: <5% L3 miss)
   - Memory access patterns (spatial/temporal locality)
   - Cache-oblivious algorithm compliance
   - NUMA effects on multi-socket systems

**Acceptance Criteria**:
- [ ] Vectorized paths: ≥50% SIMD lane utilization (measured on representative hardware)
- [ ] Federated plans: ≥75% shard parallelization efficiency (ideal = 100%)
- [ ] Cache efficiency: <5% L3 cache miss rate on 1M-row queries
- [ ] Memory peak: <500MB for 1M-row queries (validation/profiling)
- [ ] Gate compliance: 100% of query optimizer outputs pass all gates
- [ ] Regression detection: CI gates alert on 5%+ efficiency drop

**Test Coverage**:
- 15+ vectorization profiling tests (different query types)
- 10+ parallelization efficiency tests (1-10 shards)
- 8+ cache efficiency tests (different workload patterns)
- Integration with CI gates (`.github/workflows/query-performance-gates.yml`)

**Files to Create/Modify**:
- `src/query/vectorization_profiler.cpp` (profiler, ~250 lines)
- `src/query/federation_efficiency_gates.cpp` (parallel efficiency, ~300 lines)
- `src/query/cache_efficiency_analyzer.cpp` (cache metrics, ~200 lines)
- `tests/query/test_query_optimization_gates.cpp` (test suite, ~400 lines)
- `.github/workflows/query-performance-gates.yml` (CI gates, new or extend)

**Effort**: 25 hours
**Deliverable**: Automated performance gates for vectorization, parallelization, and cache efficiency

---

### Batch 2D: Performance Benchmark Consolidation (5-6 weeks, Nov-Dec 2026)

**Objective**: Close performance gates on representative hardware with reproducible benchmark results

**Key Components**:
1. **Workload Profiling**
   - TPC-H (22 queries, scale factors 0.1, 1, 10)
   - TPC-DS (99 queries, scale factors 0.1, 1)
   - Custom synthetic workloads (random joins, aggregations, complex filters)

2. **Latency & Throughput Measurement**
   - Per-query latency (p50, p95, p99)
   - Concurrent throughput (queries/sec at different load levels)
   - Tail latency analysis (repeated measurements)
   - Comparison to Wave 7 baseline

3. **Reproducibility & Hardware Qualification**
   - Variance analysis (target: ±5% across runs)
   - Hardware platform validation (CPU, memory, network specs)
   - CI integration: automated gate checks on every build
   - Baseline version control (pin to specific commit + hardware config)

**Acceptance Criteria**:
- [ ] TPC-H Queries 1-22: p99 latency ≤2s single-shard (scale factor 1)
- [ ] TPC-H Queries 1-22: p99 latency ≤5s multi-shard (3 shards, scale factor 1)
- [ ] Throughput: ≥100 concurrent queries/sec (mixed workload)
- [ ] Memory peak: <2GB for full TPC-H scale factor 1
- [ ] Reproducibility: Results within ±5% of baseline across 5+ runs
- [ ] Regression detection: CI alerts if any benchmark degrades >10%

**Test Coverage**:
- 22 TPC-H query benchmarks (all standard queries)
- 20+ TPC-DS query benchmarks (high-complexity queries)
- 30+ custom synthetic benchmarks (edge cases, stress patterns)
- Reproducibility validation: 5+ runs per configuration

**Files to Create/Modify**:
- `benchmarks/query/wave_b_tpch_suite.cpp` (TPC-H, ~300 lines)
- `benchmarks/query/wave_b_tpcds_suite.cpp` (TPC-DS, ~250 lines)
- `benchmarks/query/wave_b_synthetic_workloads.cpp` (synthetic, ~200 lines)
- `benchmarks/query/results/wave_b_baseline_*.json` (baseline data)
- `.github/workflows/query-performance-benchmark.yml` (CI benchmark runner, new or extend)
- `docs/WAVE_B_PERFORMANCE_GATES.md` (gate documentation, ~300 lines)

**Effort**: 30 hours
**Deliverable**: Reproducible performance baselines with automated gate enforcement

---

## Interdependency & Scheduling

### Critical Path

```
Phase A (parallel with other Wave A work)
  ↓
Batch 2A: Distributed Baselines (3 weeks)
  ↓
Batch 2B: Hybrid Planner (begins week 4, overlaps with 2A final 2 weeks)
  ├─ Depends on: 2A baselines established
  ├─ Overlap: 2 weeks with 2A
  └─ Duration: 5-6 weeks
  ↓
Batch 2C: Optimization Gates (begins week 6, overlaps with 2B final 4 weeks)
  ├─ Depends on: 2B planner stable (≥95% speedup confirmed)
  ├─ Overlap: 4 weeks with 2B
  └─ Duration: 4-5 weeks
  ↓
Batch 2D: Benchmark Closure (begins week 9, overlaps with 2C final 2 weeks)
  ├─ Depends on: 2C gates passing
  ├─ Overlap: 2 weeks with 2C
  └─ Duration: 5-6 weeks
  ↓
Wave B Exit Criteria Verification (week 16)
```

**Total Duration**: 16 weeks (Q3–Q4 2026)

### Effort Distribution

| Week | Batch 2A | Batch 2B | Batch 2C | Batch 2D | Total |
|------|----------|----------|----------|----------|-------|
| 1-3  | 20h      | —        | —        | —        | 20h   |
| 4-5  | 4h       | 12h      | —        | —        | 16h   |
| 6-8  | —        | 16h      | 8h       | —        | 24h   |
| 9-11 | —        | 12h      | 12h      | 10h      | 34h   |
| 12-14| —        | —        | 5h       | 15h      | 20h   |
| 15-16| —        | —        | —        | 5h       | 5h    |
| **Total** | **20h** | **40h** | **25h** | **30h** | **115h** |

---

## Quality Gates & Success Metrics

### Performance Gates (Batch 2D)

| Gate | Metric | Target | Pass/Fail |
|------|--------|--------|-----------|
| **Latency** | TPC-H Q1-22 p99 (single-shard) | ≤2s | Auto-fail if >2.2s |
| **Latency** | TPC-H Q1-22 p99 (3 shards) | ≤5s | Auto-fail if >5.5s |
| **Throughput** | Mixed workload concurrent | ≥100 q/s | Auto-fail if <95 q/s |
| **Memory** | TPC-H SF=1 peak | <2GB | Manual review if >2.2GB |
| **Reproducibility** | Variance across runs | ±5% | Manual review if >±7% |

### Regression Detection

```yaml
CI Performance Gate Workflow:
  - Run baseline benchmarks on every commit
  - Compare to pinned baseline commit
  - Alert if regression > 10%
  - Block merge if TPC-H TP99 increases > 15%
```

### Exit Criteria Verification Checklist

**Wave B Exit Gate (Week 16)**:
- [ ] Batch 2A: Distributed baselines established (≤10ms overhead, <5s failover)
- [ ] Batch 2B: Hybrid planner validated (≥95% speedup, deterministic)
- [ ] Batch 2C: Optimization gates passing (≥50% vectorization, ≥75% parallelization, <5% L3 miss)
- [ ] Batch 2D: Performance benchmarks reproducible (±5%, >100 concurrent q/s, p99 ≤2s single-shard)
- [ ] All tests passing (70+ new tests, ≥95% pass rate)
- [ ] Documentation complete (design docs, gate specs, benchmark results)
- [ ] No regressions vs Wave 7 baseline
- [ ] PR ready for merge to `develop` with Wave B completion

---

## Risk Mitigation

### High-Risk Areas

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Hybrid planner cost model inaccuracy | Medium (50%) | High | Extensive profiling + calibration on diverse workloads |
| Hardware variability in benchmarks | Medium (60%) | Medium | Pin to specific hardware; document in baseline metadata |
| Network latency unpredictability | Medium (50%) | Medium | Model as distribution, not single value; test on LAN + WAN |
| Vectorization SIMD limitations | Low (30%) | Medium | Keep scalar fallback; profile fallback costs separately |

### Contingency Plans

- **If Batch 2B slips**: Defer hybrid planner to Wave C; run Wave B with graph+ANN as separate paths
- **If benchmarks miss variance target**: Extend wave to Q1 2027; use provisional baselines
- **If optimization gates expose major regressions**: Iterate on query optimizer; defer complex optimizations to Wave C

---

## Implementation Resources

### Files to Create (Summary)

| File | Lines | Purpose |
|------|-------|---------|
| `distributed_execution_baselines.cpp` | 200 | Federated latency profiler |
| `hybrid_planner_ann.cpp` | 300 | ANN cost estimation |
| `hybrid_planner_cost_model.cpp` | 400 | Unified cost model |
| `vectorization_profiler.cpp` | 250 | SIMD utilization tracking |
| `federation_efficiency_gates.cpp` | 300 | Parallel efficiency validation |
| `cache_efficiency_analyzer.cpp` | 200 | Cache metrics collection |
| `test_query_distributed_baselines.cpp` | 300 | Baseline tests |
| `test_query_hybrid_planner.cpp` | 600 | Planner tests |
| `test_query_optimization_gates.cpp` | 400 | Gate tests |
| `wave_b_tpch_suite.cpp` | 300 | TPC-H benchmarks |
| `wave_b_tpcds_suite.cpp` | 250 | TPC-DS benchmarks |
| `wave_b_synthetic_workloads.cpp` | 200 | Synthetic workloads |
| Documentation (6 files) | 1,200 | Design + gates + results |
| **Total** | **5,000+** | — |

### CI/CD Integration

**New/Updated Workflows**:
- `.github/workflows/query-performance-gates.yml` — Automated performance gate checks
- `.github/workflows/query-performance-benchmark.yml` — Benchmark result tracking
- Extensions to `09-pr-gates_release-critical-tests.yml` — Query module performance gates

---

## Next Steps (Immediate)

### Week 1 (2026-08-17 – 2026-08-24)

1. **Merge Wave A to develop**
   - Execute merge workflow from WAVE_A_QUERY_MERGE_AND_WAVE_B_PLAN.md
   - Verify merge success in GitHub

2. **Create Wave B Infrastructure**
   - Set up benchmark directories (`benchmarks/query/results/`)
   - Create CI workflow skeletons
   - Initialize Wave B documentation

3. **Begin Batch 2A Kickoff**
   - Sketch `distributed_execution_baselines.cpp` API
   - Identify hardware configs for profiling
   - Plan chaos test scenarios

### Week 2-3 (2026-08-25 – 2026-09-07)

- Implement Batch 2A profiler
- Begin profiling runs
- Establish baseline results

### Week 4+ 

- Continue per-batch execution per schedule
- Weekly status updates
- Escalate blockers to project stakeholders

---

## Success Definition

✅ **Wave B Complete** when:
1. All performance gates PASS on representative hardware
2. Hybrid planner produces valid, optimal plans (single-shard)
3. Distributed execution baselines quantified with <±5% variance
4. Optimization gates enforce ≥50% vectorization, ≥75% parallelization
5. All 70+ new tests passing with ≥95% pass rate
6. Reproducible benchmark results (TPC-H + TPC-DS)
7. No regressions vs Wave 7 baseline
8. Documentation complete and linked from root ROADMAP.md

---

## Appendix: Command Reference

### Batch 2A Baseline Collection
```bash
cd /home/runner/work/ThemisDB/ThemisDB/build
cmake --build . --target themis_query_benchmarks
ctest -R "distributed_baselines" --output-on-failure
```

### Batch 2B Planner Validation
```bash
ctest -R "hybrid_planner" --output-on-failure
# Should see: ✅ 100% plan correctness, ≥95% speedup on ANN queries
```

### Batch 2C Gate Checks
```bash
ctest -R "optimization_gates" --output-on-failure
# Should see: ✅ ≥50% SIMD, ≥75% parallelization, <5% L3 miss
```

### Batch 2D TPC-H Benchmark
```bash
cd benchmarks/query && ./wave_b_tpch_suite --scale 1 --queries 1-22
# Expected: p99 ≤2s single-shard, ≤5s multi-shard (3 shards)
```

---

**Report Generated**: 2026-08-17  
**Wave A Status**: ✅ COMPLETE  
**Wave B Status**: 🟡 READY FOR KICKOFF (awaiting merge approval)
