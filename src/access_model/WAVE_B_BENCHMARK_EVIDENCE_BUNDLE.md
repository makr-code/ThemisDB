# Access Model Module — Wave B Benchmark Gate Evidence Bundle

<!-- Status: FRAMEWORK COMPLETE — hardware re-capture pending | validated: 2026-09-21 -->
<!-- Wave: B — Performance Consolidation -->
<!-- Branch: develop -->
<!-- Links: ROADMAP.md · PERFORMANCE_EXPECTATIONS.md · PHASE_5_6_ACCEPTANCE_REPORT.md -->

**Module:** `src/access_model/`  
**Wave:** B — Performance Consolidation  
**Evidence Date:** 2026-08-17 (framework defined) / hardware baseline pending  
**Status:** 🟡 FRAMEWORK COMPLETE — representative-hardware baseline re-capture required for Wave B GA promotion

---

## Summary

The access_model module delivered all six benchmark gates (GATE-ACM-01..06) as part of
Phase 6 (2026-08-17). The gate definitions, thresholds, violation reporting, and
regression tolerance framework are fully implemented in
`benchmarks/access_model/bench_access_coordinator_gates.cpp`.

**Current state:**
- Gate framework: ✅ defined, source-verified, violation-reporting operational
- Scaffold/CI measurements: ✅ functional (no PERF_GATE violations reported on develop CI)
- Representative-hardware baselines: ⏳ pending — required before Wave B GA promotion

---

## Gate Definitions

| Gate ID | Description | Threshold | Direction | Benchmark Case |
|---------|-------------|-----------|-----------|----------------|
| GATE-ACM-01 | L1→L2 promotion latency | ≤ 50 µs p99 | max | `GATE_ACM_01_L1ToL2Promotion` |
| GATE-ACM-02 | Cache eviction → storage feedback round-trip | ≤ 100 µs p99 | max | `GATE_ACM_02_CacheEvictionToStorageFeedback` |
| GATE-ACM-03 | Cold→warm promotion latency | ≤ 100 ms p99 | max | `GATE_ACM_03_ColdToWarmPromotion` |
| GATE-ACM-04 | Event processing throughput | ≥ 10 K events/sec | min | `GATE_ACM_04_EventProcessingThroughput` |
| GATE-ACM-05 | Coordinator memory overhead (1 M events) | ≤ 50 MB | max | `GATE_ACM_05_MemoryOverhead` |
| GATE-ACM-06 | Age-based policy decision overhead | ≤ 10 µs per event | max | `GATE_ACM_06_PolicyDecisionOverhead` |

Regression tolerance: ±10% vs representative-hardware baseline for all gates.  
Violation reporting: `[PERF_GATE]` prefix to stderr, checked at end of each benchmark case.  
Source: `benchmarks/access_model/bench_access_coordinator_gates.cpp` lines 47–52, 214–415.

---

## GATE-ACM-01: L1→L2 Promotion Latency

**Threshold:** ≤ 50 µs p99  
**Measurement method:** Wall-clock per-iteration via `UseRealTime()`;
`onStorageAccess(STORAGE_COLD)` → coordinator event processing.  
**Benchmark fixture:** `BenchAccessCoordinator`, canonical RNG seed 42.

| Evidence Type | Status |
|---|---|
| Gate threshold defined in source | ✅ `kL1ToL2PromotionUs = 50.0` |
| Violation reporting implemented | ✅ `reportViolation("GATE-ACM-01", ...)` |
| CI scaffold run (no violation) | ✅ develop CI functional |
| Representative-hardware baseline | ⏳ pending |

---

## GATE-ACM-02: Cache Eviction → Storage Feedback

**Threshold:** ≤ 100 µs p99  
**Measurement method:** Wall-clock per-iteration; `onCacheEvicted(L1_WORKING)` →
coordinator processes and logs EvictionEventLog with correlation ID.

| Evidence Type | Status |
|---|---|
| Gate threshold defined in source | ✅ `kCacheEvictionRoundtripUs = 100.0` |
| Violation reporting implemented | ✅ `reportViolation("GATE-ACM-02", ...)` |
| CI scaffold run (no violation) | ✅ develop CI functional |
| Representative-hardware baseline | ⏳ pending |

---

## GATE-ACM-03: Cold→Warm Promotion Latency

**Threshold:** ≤ 100 ms p99  
**Measurement method:** Wall-clock per-iteration; 3× `onStorageAccess(STORAGE_COLD)`
calls per key to trigger cold→warm promotion decision path.  
**Note:** CPU-only measurement; actual I/O latency from backing store not included
in this gate — a separate integration benchmark is recommended for full-path validation.

| Evidence Type | Status |
|---|---|
| Gate threshold defined in source | ✅ `kColdToWarmPromotionMs = 100.0` |
| Violation reporting implemented | ✅ `reportViolation("GATE-ACM-03", ...)` |
| CI scaffold run (no violation) | ✅ develop CI functional |
| Representative-hardware baseline | ⏳ pending |

---

## GATE-ACM-04: Event Processing Throughput

**Threshold:** ≥ 10 000 events/sec sustained  
**Measurement method:** Mixed `onStorageAccess` / `onCacheEvicted` stream;
throughput = `iterations / elapsed_real_time`.

| Evidence Type | Status |
|---|---|
| Gate threshold defined in source | ✅ `kEventThroughputPerSec = 10000.0` |
| Violation reporting implemented | ✅ `reportViolation("GATE-ACM-04", ..., is_minimum=true)` |
| CI scaffold run (no violation) | ✅ develop CI functional |
| Representative-hardware baseline | ⏳ pending |

---

## GATE-ACM-05: Memory Overhead

**Threshold:** ≤ 50 MB for coordinator + 1 M events  
**Measurement method:** 1 000-event batches per iteration with 10 ms drain sleep;
RSS-level measurement requires an external profiling run.  
**Note:** The in-benchmark proxy (bounded allocation pattern) is a structural gate;
definitive RSS validation requires a dedicated profiling run on representative hardware.

| Evidence Type | Status |
|---|---|
| Gate threshold defined in source | ✅ `kMemoryOverheadMb = 50.0` |
| Structural gate (bounded allocation) | ✅ passes in CI |
| RSS profiling run | ⏳ pending representative hardware |
| Representative-hardware baseline | ⏳ pending |

---

## GATE-ACM-06: Policy Decision Overhead

**Threshold:** ≤ 10 µs per event  
**Measurement method:** Wall-clock per-iteration; `onCacheEvicted` → coordinator
invokes `AgeBasedPolicy` decision path per event.

| Evidence Type | Status |
|---|---|
| Gate threshold defined in source | ✅ `kPolicyDecisionOverheadUs = 10.0` |
| Violation reporting implemented | ✅ `reportViolation("GATE-ACM-06", ...)` |
| CI scaffold run (no violation) | ✅ develop CI functional |
| Representative-hardware baseline | ⏳ pending |

---

## Hardware Profile Requirements

For Wave B GA promotion, baselines must be captured on representative production-class hardware.

| Requirement | Target |
|---|---|
| CPU class | ≥ Intel Xeon E-series or equivalent server-grade multi-core |
| Memory | ≥ 16 GB DDR4 |
| OS | Linux (x86_64), production kernel (no debug/profiling overhead) |
| Build type | Release profile (`CMAKE_BUILD_TYPE=Release`) |
| Benchmark isolation | No background load; CPU governor set to `performance` |
| Iterations | ≥ 10 000 iterations per gate to achieve stable p99 |
| Runs | ≥ 3 independent runs; report min/median/max across runs |
| Benchmark binary | `bench_access_coordinator_gates` from release build |

Run command:

```bash
./bench_access_coordinator_gates \
  --benchmark_filter="GATE_ACM_0[1-6]" \
  --benchmark_repetitions=5 \
  --benchmark_report_aggregates_only=true \
  --benchmark_out=gate_acm_results.json \
  --benchmark_out_format=json
```

---

## Regression Policy

| Condition | Action |
|---|---|
| Measured > gate threshold + 10% | BLOCKER — do not promote to GA |
| Measured > gate threshold but ≤ +10% | WARNING — document and track |
| Measured ≤ gate threshold | PASS |
| GATE-ACM-04 measured < 10K / sec | BLOCKER — do not promote to GA |
| Measured < 10K / sec but ≥ −10% of 10K | WARNING — document and track |

---

## Wave B Exit Criteria Checklist

- [x] All 6 gate thresholds defined and source-validated
  (`benchmarks/access_model/bench_access_coordinator_gates.cpp`)
- [x] Violation reporting operational (`[PERF_GATE]` to stderr, all 6 gates)
- [x] Scaffold/CI benchmark runs clean (no PERF_GATE violations on develop)
- [x] Regression tolerance documented (±10% for all gates)
- [x] Hardware profile requirements documented (this file)
- [~] Representative-hardware baseline captured and attached (**pending — required for GA**)
- [ ] All 6 gates PASS on representative hardware (≥ 3 reproducible runs)
- [ ] Hardware run results attached to this document or linked sign-off issue
- [ ] Wave B sign-off issue created referencing this evidence bundle

---

## Dependency on Wave A

Wave B entry gate requires Wave A closure in the Transaction and GPU modules.
The following are external to access_model:

- `src/transaction/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` — focused suites registered;
  authoritative hardware CI pending Q4 2026.
- `src/gpu/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` — RAII guards confirmed;
  hardware baselines pending Q4 2026.

The access_model module has no blocking implementation gaps for Wave B entry.
The hardware baseline re-capture above can proceed in parallel with Wave A closure work.

---

## Source Verification

| Artefact | Path | State |
|---|---|---|
| Benchmark gate source | `benchmarks/access_model/bench_access_coordinator_gates.cpp` | ✅ exists, all 6 gates implemented |
| Phase 5-6 acceptance report | `src/access_model/PHASE_5_6_ACCEPTANCE_REPORT.md` | ✅ complete 2026-08-17 |
| Performance expectations | `src/access_model/PERFORMANCE_EXPECTATIONS.md` | ✅ aligned to GATE-ACM-01..06 |
| ROADMAP Wave B exit criteria | `src/access_model/ROADMAP.md` §Wave B Exit Criteria | ✅ references this bundle |
