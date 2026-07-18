# Phase 3 Week 1 Baseline Measurements — 2026-07-22 to 2026-07-26

**Status:** 🔵 PLANNED  
**Timeline:** Week 1 of Phase 3 (2026-07-22 to 2026-07-26)  
**Objective:** Establish baseline metrics for Phase 2.4 before Phase 3 optimizations begin

---

## Overview

This document captures baseline measurements taken during Phase 3 Week 1. These measurements serve as the reference point for validating performance improvements in Phase 3-01 through P3-04 (query optimization, cache efficiency, resource pooling, and load balancing).

---

## Week 1 Schedule & Measurement Plan

### Monday 2026-07-22: Query Planner Baseline (P3-01)

**Objective:** Measure query planning latency (p50, p95, p99) before LRU cache implementation.

| Metric | Measurement Method | Target | Status |
|--------|-------------------|--------|--------|
| **Query Planning p50** | Measure 1000 queries on baseline workload | TBD | 🔵 Pending |
| **Query Planning p95** | 95th percentile of 1000 queries | TBD | 🔵 Pending |
| **Query Planning p99** | 99th percentile of 1000 queries | TBD | 🔵 Pending |
| **Plan Cache Hit Ratio** | Baseline single-tier LRU (if exists) | TBD | 🔵 Pending |
| **Cost Model Accuracy** | vs. actual execution time | TBD | 🔵 Pending |

**Workload:** Synthetic query set (100 distinct query templates, 10 parameter variations each)

**Output:** `phase3_baseline_query_planning.csv`

---

### Tuesday 2026-07-23: Cache Hit Ratio Baseline (P3-02)

**Objective:** Measure cache hit ratio on Phase 2.4 cache implementation (single-tier).

| Metric | Measurement Method | Target | Status |
|--------|-------------------|--------|--------|
| **Cache Hit Ratio** | YCSB uniform workload (10k operations) | ~40% (est.) | 🔵 Pending |
| **Cache Eviction Latency p99** | Single eviction latency measurement | TBD | 🔵 Pending |
| **Memory Footprint** | Peak memory used by cache | TBD | 🔵 Pending |
| **Tier Distribution** | Distribution across hot/warm/cold (if applicable) | N/A (Phase 2.4 single-tier) | 🔵 Pending |

**Workload:** YCSB uniform distribution (50% read, 50% write, 1000 entries)

**Output:** `phase3_baseline_cache_efficiency.csv`

---

### Wednesday 2026-07-24: Resource Pool Saturation Baseline (P3-03)

**Objective:** Measure resource pool saturation and contention under baseline workload.

| Metric | Measurement Method | Target | Status |
|--------|-------------------|--------|--------|
| **Connection Pool Peak Utilization** | 100 concurrent clients; track max active connections | TBD | 🔵 Pending |
| **Thread Pool Queue Depth** | Peak work queue depth during sustained load | TBD | 🔵 Pending |
| **Thread Pool Latency p99** | Work pickup latency from queue | TBD | 🔵 Pending |
| **Buffer Pool Reuse Rate** | Allocated vs. freed buffers | TBD | 🔵 Pending |
| **Resource Contention Score** | Composite metric (lock contention, wait times) | TBD | 🔵 Pending |

**Workload:** 100 concurrent query clients (connection pool stress)

**Output:** `phase3_baseline_resource_pooling.csv`

---

### Thursday 2026-07-25: Load Balancing Skew Baseline (P3-04)

**Objective:** Measure cross-shard load distribution and latency variance under baseline scheduler.

| Metric | Measurement Method | Target | Status |
|--------|-------------------|--------|--------|
| **Shard Response Time p99** | Per-shard latency p99 (3 shards, 100 concurrent queries) | TBD | 🔵 Pending |
| **Variance Coefficient** | Std dev / mean of response times across shards | TBD | 🔵 Pending |
| **Skew Ratio** | Max latency / min latency across shards | TBD | 🔵 Pending |
| **SLA Compliance** | % queries completed within 50ms | TBD | 🔵 Pending |
| **Scheduler Queue Depth** | Peak depth of centralized query queue (if exists) | TBD | 🔵 Pending |

**Workload:** Zipf-distributed query distribution (hot/cold shards, 100 concurrent clients)

**Output:** `phase3_baseline_load_balancing.csv`

---

### Friday 2026-07-26: Baseline Consolidation & Wave 7 Re-Pass (P3-05)

**Objective:** Consolidate all baselines and verify Wave 7 gates still pass (regression detector).

| Metric | Measurement Method | Gate Threshold | Status |
|--------|-------------------|-----------------|--------|
| **Wave 7: Read p99** | Single-shard point read latency p99 | <= 200µs | 🔵 Pending |
| **Wave 7: Write Throughput** | Write ops/sec | >= 80k ops/s | 🔵 Pending |
| **Wave 7: Range p99** | Range query latency p99 | <= 500µs | 🔵 Pending |
| **Wave 7: Batch p99** | Batch query latency p99 | <= 5ms | 🔵 Pending |

**Output:** `PHASE3_BASELINE.md` (this file, updated with measurements)

---

## Baseline Summary Table (to be filled in Week 1)

| Component | Metric | Baseline Value | Phase 3 Target | Improvement Path |
|-----------|--------|-----------------|-----------------|------------------|
| **Query Planning (P3-01)** | p99 latency | TBD | -10% | LRU cache + cost model tuning |
| **Cache Efficiency (P3-02)** | Hit ratio | ~40% | >=85% | Multi-tier eviction + weighted scoring |
| **Resource Pooling (P3-03)** | Peak utilization | TBD | <=80% | Adaptive connection/thread/buffer pooling |
| **Load Balancing (P3-04)** | Variance coeff | TBD | <=10% | Load-aware shard selection + SLA scheduling |
| **Wave 7: Read** | p99 latency | TBD | <=200µs | (No regression target) |
| **Wave 7: Write** | Throughput | TBD | >=80k ops/s | (No regression target) |
| **Wave 7: Range** | p99 latency | TBD | <=500µs | (No regression target) |
| **Wave 7: Batch** | p99 latency | TBD | <=5ms | (No regression target) |

---

## Test Infrastructure Status

### Test Files Created (Week 1, P3-05-C infrastructure)

| Test File | Component | Test Count | Status |
|-----------|-----------|-----------|--------|
| `tests/epic2_evaluation/test_query_planner_cache.cc` | P3-01 | 28 tests | ✅ Created (stubs) |
| `tests/cache/test_cache_efficiency_multilevel.cpp` | P3-02 | 32 tests | ✅ Created (stubs) |
| `tests/integration/test_resource_pooling.cpp` | P3-03 | 36 tests | ✅ Created (stubs) |
| `tests/integration/test_load_balancing.cpp` | P3-04 | 24 tests | ✅ Created (stubs) |
| `benchmarks/bench_phase3_optimization.cpp` | P3-05 | 12 benchmarks | ✅ Created (stubs) |

**Total:** 132 test stubs (target: 130+ tests) ✅

### CMakeLists.txt Integration (Week 1 end)

- [ ] Update `tests/epic2_evaluation/CMakeLists.txt` to register `test_query_planner_cache`
- [ ] Update `tests/cache/CMakeLists.txt` to register `test_cache_efficiency_multilevel`
- [ ] Update `tests/integration/CMakeLists.txt` to register resource/load-balancing tests
- [ ] Update `benchmarks/CMakeLists.txt` to register `bench_phase3_optimization`
- [ ] Configure CTest labels: `phase3`, `p3-01`, `p3-02`, `p3-03`, `p3-04`, `p3-05`

---

## Measurement Collection Commands

### Week 1 Baseline Collection (to be executed)

```bash
# Monday: Query Planning Baseline
cd /home/runner/work/ThemisDB/ThemisDB
cmake --preset community-release -DTHEMIS_PHASE3_ENABLE=ON
cmake --build --preset community-release --target phase3_baseline_query_planning
benchmarks/bench_phase3_optimization --benchmark_filter="Planning" \
  --benchmark_out=phase3_baseline_query_planning.csv \
  --benchmark_out_format=csv

# Tuesday: Cache Hit Ratio Baseline
benchmarks/bench_phase3_optimization --benchmark_filter="CacheHitRatio" \
  --benchmark_out=phase3_baseline_cache_efficiency.csv \
  --benchmark_out_format=csv

# Wednesday: Resource Pool Saturation Baseline
benchmarks/bench_phase3_optimization --benchmark_filter="PoolUtilization" \
  --benchmark_out=phase3_baseline_resource_pooling.csv \
  --benchmark_out_format=csv

# Thursday: Load Balancing Baseline
benchmarks/bench_phase3_optimization --benchmark_filter="BalancingVariance" \
  --benchmark_out=phase3_baseline_load_balancing.csv \
  --benchmark_out_format=csv

# Friday: Wave 7 Re-Pass
cd benchmarks/wave7
python report_variance_w7.py --gate-only

# Consolidate all results
cat phase3_baseline_*.csv > PHASE3_BASELINE_WEEK1_SUMMARY.csv
```

---

## Acceptance Criteria (Week 1)

- [ ] All baseline measurements collected (Monday-Friday)
- [ ] Wave 7 gates re-verified PASS (all 6 gates)
- [ ] Test scaffolding created (132 test stubs)
- [ ] CMakeLists.txt updated with Phase 3 test targets
- [ ] Baseline CSV files committed to repository
- [ ] `PHASE3_BASELINE.md` finalized with measurements

---

## Next Steps (Week 2+)

1. **Week 2-3:** Implement P3-01 through P3-04 (parallel teams)
2. **Week 4:** Integration testing + performance regression validation
3. **Week 5:** Final sign-off + documentation completion

---

## References

- Main Plan: [ai_working/PHASE3_OPTIMIZATION_DETAILED_PLAN.md](ai_working/PHASE3_OPTIMIZATION_DETAILED_PLAN.md)
- Wave 7 Gates: [benchmarks/wave7/release_gate_manifest_w7.json](benchmarks/wave7/release_gate_manifest_w7.json)
- Measurement Hygiene: [benchmarks/MEASUREMENT_HYGIENE.md](benchmarks/MEASUREMENT_HYGIENE.md)

---

**Document Owner:** Phase 3 Tech Lead  
**Last Updated:** 2026-07-18 (Planning phase)  
**Measurement Period:** 2026-07-22 to 2026-07-26 (Week 1)
