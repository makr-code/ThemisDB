# Tensor Module Stream C: Q1 2027 Performance Baselining & Expansion - Planning

**Issue**: #5674 Follow-up Implementation (Stream C)  
**Target Completion**: 2027-01-31  
**Branch**: feature/tensor-q1-baseline  
**Status**: PLANNED (Start: Nov 1, 2026)  
**Dependencies**: Stream A+B completion (Oct 31, 2026)

## Overview

Stream C focuses on re-baselining performance envelopes across hardware variants, expanding benchmark diversity, and validating long-run reliability. All work is done in parallel using specialized sub-agents.

---

## Block C1: Re-Baseline P95/P99 Hot Paths

**Agent**: research (tensor-c1-profiling)  
**Duration**: 2-3 weeks (Nov 1-21)  
**Prerequisites**: Stream A+B stable; code optimized

### Deliverables

- [ ] **Profiling & Hotspot Analysis**
  - Identify top 5-8 hottest code paths via flame graphs
  - Key candidates: findSimilar, fingerprint extraction, index rotation, bridge transition
  - Collect CPU cycle counts, memory access patterns

- [ ] **Stable P50/P95/P99 Bands**
  - 100+ samples per hotspot path on release build
  - Calculate statistical bands (±1 std deviation)
  - Validate < 2% variance across 10 measurement runs
  - Hardware-normalized comparison (normalize to reference CI box)

- [ ] **PERFORMANCE_ENVELOPES_Q1_2027.md** — Comprehensive report
  - 5-8 hotspot analyses with profiling data
  - P50/P95/P99 bands (in µs, ms, or ns as appropriate)
  - SLA gate recommendations (20+ proposed gates)
  - Historical comparison: Q3 baseline → Q1 baseline trends
  - Marginal improvement analysis (if optimizations applied)

### Success Criteria

- ✅ 5-8 hotspots profiled and measured
- ✅ P95/P99 stable (< 2% variance)
- ✅ SLA gates defensible and documented
- ✅ Historical trends tracked

### Integration

```markdown
# Envelope recommendations feed into:
# - FUTURE_ENHANCEMENTS.md (update performance targets)
# - bench_tensor_release_gates.cpp (lock new gate values)
```

---

## Block C2: Benchmark Diversity Expansion

**Agent**: task runner (tensor-c2-diversity)  
**Duration**: 2-3 weeks (Nov 1-21)  
**Prerequisites**: Block C1 hotspot identification; baseline locked

### Deliverables

- [ ] **Workload Profile Library** (6-8 new profiles)

  1. **Graph Density Profiles**
     - Sparse fingerprints (< 10% fingerprint width utilization)
     - Dense fingerprints (> 90% fingerprint width utilization)
     
  2. **Adapter Size Profiles**
     - Small vectors (32-64 dims, typical for embeddings)
     - Large vectors (1024+ dims, for rich embeddings)
     - Mixed sizes within single graph
     
  3. **Workload Ratio Profiles**
     - Read-heavy (99% query, 1% update)
     - Write-heavy (50% query, 50% update)
     - Mixed (90% query, 10% update)
     
  4. **Tenant/Federation Profiles**
     - Single-tenant (typical)
     - 10-tenant split (cross-tenant queries)
     - Federation scenario (distributed graphs)

- [ ] **bench_tensor_workload_profiles_q1_2027.cpp** — 20+ benchmark variants
  - Parametrized benchmarks covering all 8 profiles
  - Each benchmark measures p50/p95/p99 latency and throughput
  - Fixed random seeds (kCanonicalRngSeed variants)
  - 1k/10k/50k candidate scales per FUTURE_ENHANCEMENTS.md

- [ ] **BENCHMARK_DIVERSITY.md**
  - 20+ new benchmark variants catalogued
  - Expected performance ranges per profile
  - Rationale for each profile (why it matters)
  - Links to workload characteristics in docs

### Success Criteria

- ✅ 6-8 distinct workload profiles implemented
- ✅ 20+ benchmark variants locked and reproducible
- ✅ All variants runnable in standard CI pipeline
- ✅ Performance expectations documented

### Integration

```cmake
# benchmarks/tensor/CMakeLists.txt — Register new benchmarks
# Each variant as separate executable:
# module_tensor_bench_workload_profiles_q1_2027
```

---

## Block C3: Long-Run Reliability Validation

**Agent**: task runner (tensor-c3-longrun)  
**Duration**: 1-2 weeks (Nov 22 - Dec 5, with async 48h+ runs)  
**Prerequisites**: Stream A+B stable; stress suite passing

### Deliverables

- [ ] **Long-Run Test Design**
  - 48-72 hour sustained graph mutation/query traffic
  - Workload: 90% query, 10% store/remove (per FUTURE_ENHANCEMENTS.md)
  - Scale: 50k-100k adapters in graph
  - Measurement interval: every 1M operations

- [ ] **Memory & Stability Validation**
  - Valgrind + AddressSanitizer passes (0 leaks detected)
  - No unbounded growth in:
    - Index metadata size
    - Fingerprint graph structure size
    - Bridge ingestion queue depth
  - Memory budget enforcement: < 10% growth over 72h

- [ ] **SLA Violation Tracking**
  - Count operations missing p99 deadline
  - Acceptable: < 0.1% of operations
  - Track and categorize SLA violations by subsystem

- [ ] **LONG_RUN_RELIABILITY_REPORT.md**
  - 48-72 hour run summary (start/end timestamps)
  - Operation count: X million
  - Sanitizer results (memcheck, helgrind if applicable)
  - Memory growth graph (with bounds)
  - SLA violation statistics
  - Any crashes/hangs/assertions triggered

### Success Criteria

- ✅ 48-72 hour runs complete without crashes
- ✅ No memory leaks (Valgrind 0 leaks)
- ✅ No unbounded growth (< 10% over 72h)
- ✅ SLA violations < 0.1% of operations
- ✅ Reproducible (if rerun, similar results ±5%)

### Integration

```cmake
# tests/tensor/CMakeLists.txt — Long-run test harness
# Note: Requires TIMEOUT 432000+ (72 hours in seconds)
# May run on separate CI tier (nightly or weekly)
```

---

## Merge & Integration Timeline

| Date | Event | Merge Target |
|------|-------|--------------|
| Nov 1-21 | Block C1 profiling & envelopes | C1 complete |
| Nov 1-21 | Block C2 benchmark diversity | C2 complete |
| Nov 22 - Dec 5 | Block C3 long-run async runs | C3 ongoing |
| Dec 6-19 | Integration & final validation | feature/tensor-q1-baseline complete |
| Jan 20-31 | Final review & merge to develop | develop branch |

---

## Risk Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Profiling variance in C1 | MEDIUM | MEDIUM | 100+ samples, normalized hardware, statistical testing |
| Benchmark explosion in C2 | LOW | MEDIUM | Start with 6-8 core profiles, expand incrementally |
| Long-run instability in C3 | LOW | HIGH | Run on dedicated hardware; enable all sanitizers |
| Regression to Q3 baseline in C1 | LOW | MEDIUM | Compare trends; investigate any > 10% drift |

---

## Success Criteria (Overall Stream C)

✅ **Performance Envelopes**: 20+ SLA gates defined, defensible, and locked  
✅ **Benchmark Diversity**: 20+ new variants, covering 6-8 workload profiles  
✅ **Long-Run Reliability**: 72h sustained run passing, no leaks, bounded memory growth  
✅ **Production Readiness**: All envelopes validated, no regressions, reproducible

---

## Notes

- **Async Long-Run**: C3 runs can execute in parallel with C1+C2 (they have separate dependencies)
- **Performance Targets**: Reference values from FUTURE_ENHANCEMENTS.md remain stable
- **Traceability**: All envelopes and benchmarks traceable to ROADMAP.md Phase 5/6 blocks
- **Future Maintenance**: New workload profiles become regression test baseline for Q2 2027+

---

