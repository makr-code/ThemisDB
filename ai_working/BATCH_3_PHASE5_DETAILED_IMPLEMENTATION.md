# Batch 3 Implementation Guide: Phase 5-Performance & Benchmarking

**Target Weeks:** 9-14 (Weeks after Batch 1-2 completion)  
**Status:** 📋 Ready to launch (awaiting Batch 2 completion)

---

## Phase 5 Completion Roadmap

### Current State (Batch 1-2 completion)
- ✅ Native benchmark suite exists (bench_toolbox_native_workloads.cpp, 260+ lines)
- ✅ 6 primary gates defined (GATE-TBX-P1..P6)
- ✅ Test data and fixtures prepared (kShortText, kMediumText, MakeMediumText)
- ✅ Benchmark infrastructure operational (Google Benchmark + steady_clock)
- 📋 Baseline measurements: NOT YET COLLECTED
- 📋 p95/p99 envelopes: NOT YET ESTABLISHED

### Phase 5 Objectives (Batch 3)

| Objective | Owner | Target | Status |
|-----------|-------|--------|--------|
| Collect Q3 2026 baseline measurements | Batch 3 Agent | Week 9 | [ ] TODO |
| Analyze proxy-to-native performance delta | Batch 3 Agent | Week 10 | [ ] TODO |
| Establish p95/p99 envelopes | Batch 3 Agent | Week 11 | [ ] TODO |
| Validate no regression > 10% | Batch 3 Agent | Week 12 | [ ] TODO |
| Certify all GATE-TBX-P1..P6 gates | Batch 3 Agent | Week 13 | [ ] TODO |
| Update PERFORMANCE_EXPECTATIONS.md | Batch 3 Agent | Week 14 | [ ] TODO |

---

## Benchmark Gate Specifications

### Gate Table (from bench_toolbox_native_workloads.cpp)

| Gate ID | Operation | Measurement | Threshold | Benchmark Case |
|---------|-----------|------------|-----------|-----------------|
| GATE-TBX-P1 | extractEntities throughput | ops/second | ≥ 100K ops/s | BM_ExtractEntities_Throughput |
| GATE-TBX-P2 | extractEntitySet latency | p95 ms | ≤ 50ms | BM_ExtractEntitySet_Latency |
| GATE-TBX-P3 | Text normalization | p95 µs | ≤ 10ms | BM_TextNormalization_Latency |
| GATE-TBX-P4 | Language detection | p95 µs | ≤ 15ms | BM_LanguageDetection_Latency |
| GATE-TBX-P5 | Fingerprinting throughput | ops/second | ≥ 1M ops/s | BM_ContentFingerprinting_Throughput |
| GATE-TBX-P6 | Bridge enrichment | p95 ms | ≤ 100ms | BM_BridgeEnrichment_Placeholder |

### Release Gates (TBXG-1..3)

| Gate ID | Expectation | Measurement | Threshold |
|---------|------------|-------------|-----------|
| TBXG-1 | Regression vs baseline | (current - baseline) / baseline | ≤ 10% |
| TBXG-2 | p99 latency ceiling | p99 from benchmark | release-specific |
| TBXG-3 | Benchmark manifest | 6+ primary cases | all operational |

---

## Implementation Strategy

### Phase 5.1: Baseline Collection (Week 9-10)

**Objective:** Run all 6 gates on Q3 2026 release profile hardware

**Procedure:**
```bash
# 1. Configure release build
cmake --preset linux-release
cmake --build --preset linux-release

# 2. Run all 6 benchmark gates
./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
  --benchmark_min_time=5s \
  --benchmark_out_format=csv \
  --benchmark_out=q3_2026_baseline.csv

# 3. Capture results per gate (GATE-TBX-P1..P6)
# - GATE-TBX-P1: BM_ExtractEntities_Throughput/real_time (ops/s)
# - GATE-TBX-P2: BM_ExtractEntitySet_Latency/real_time (ms p95)
# - GATE-TBX-P3: BM_TextNormalization_Latency/real_time (µs p95)
# - GATE-TBX-P4: BM_LanguageDetection_Latency/real_time (µs p95)
# - GATE-TBX-P5: BM_ContentFingerprinting_Throughput/real_time (ops/s)
# - GATE-TBX-P6: BM_BridgeEnrichment_Placeholder/real_time (ms p95)
```

**Expected Outputs:**
- `q3_2026_baseline.csv` with all 6 gates
- p50, p95, p99 percentiles for latency gates
- ops/s throughput for throughput gates
- Document hardware/OS (CPU model, RAM, OS kernel)

### Phase 5.2: Proxy-to-Native Delta Analysis (Week 10-11)

**Objective:** Compare proxy benchmarks vs native suite

**Procedure:**
1. Identify proxy benchmarks for each gate:
   - GATE-TBX-P1 (extraction): bench_ingestion_extraction.cpp
   - GATE-TBX-P2 (entity_set): bench_ingestion_quality_judge.cpp
   - GATE-TBX-P3 (text): bench_text_extraction.cpp
   - GATE-TBX-P4 (language): (no proxy, direct native only)
   - GATE-TBX-P5 (fingerprinting): bench_content_processor_paths.cpp
   - GATE-TBX-P6 (bridge): (placeholder, measure enrichment)

2. Run proxy benchmarks on same hardware
3. Compare throughput/latency (calculate % delta)
4. Document divergence points

**Expected Analysis:**
- Proxy vs native delta: ±5-15% typical for extraction/text paths
- Expected divergence sources:
  - Proxy: includes ingestion engine overhead
  - Native: pure toolbox operations only
- Delta <= 20% acceptable for proxy/native equivalence

### Phase 5.3: p95/p99 Envelope Establishment (Week 11-12)

**Objective:** Define performance envelopes and baseline regression limits

**Procedure:**
1. Analyze percentile data from baseline runs:
   - p50 (median)
   - p95 (tail latency, release critical)
   - p99 (extreme tail)

2. Calculate baseline + 10% regression budget:
   - For latency: baseline_p95 * 1.10
   - For throughput: baseline_ops * 0.90

3. Document in PERFORMANCE_EXPECTATIONS.md:
   ```markdown
   ### Baseline Measurements (Q3 2026)
   
   | Gate | Measurement | Baseline p95 | Baseline p99 | Regression Budget (±10%) |
   |------|-------------|-------------|-------------|------------------------|
   | TBX-P1 | ops/s | 125K | 135K | 112.5K - 137.5K |
   | TBX-P2 | ms latency | 42ms | 58ms | 37.8ms - 46.2ms |
   ```

### Phase 5.4: Regression Validation (Week 12-13)

**Objective:** Verify no regression > 10% vs baseline

**Procedure:**
```bash
# Run gates 3-5 times and capture min/max/avg
for run in {1..5}; do
  ./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
    --benchmark_out_format=csv \
    --benchmark_out=run_${run}.csv
done

# Calculate regression vs baseline
python3 tools/benchmark_regression_check.py \
  --baseline q3_2026_baseline.csv \
  --runs run_*.csv \
  --threshold 10
```

**Success Criteria:**
- All 6 gates: regression <= 10%
- Consistent results across 5 runs (low variance)
- No outliers or anomalies

### Phase 5.5: Gate Certification (Week 13-14)

**Objective:** Formally certify all GATE-TBX-P1..P6

**Procedure:**
1. Verify all 6 gates meet thresholds:
   - ✅ GATE-TBX-P1: ≥ 100K ops/s
   - ✅ GATE-TBX-P2: p95 ≤ 50ms
   - ✅ GATE-TBX-P3: p95 ≤ 10ms
   - ✅ GATE-TBX-P4: p95 ≤ 15ms
   - ✅ GATE-TBX-P5: ≥ 1M ops/s
   - ✅ GATE-TBX-P6: p95 ≤ 100ms

2. Verify regression gates:
   - ✅ TBXG-1: regression ≤ 10%
   - ✅ TBXG-2: p99 ≤ release ceiling
   - ✅ TBXG-3: 6 cases operational

3. Document certification in PERFORMANCE_EXPECTATIONS.md

---

## Files to Modify (Phase 5)

### Primary
- `src/toolbox/PERFORMANCE_EXPECTATIONS.md` - Add baseline measurements, p95/p99 envelopes, regression limits
- `src/toolbox/ROADMAP.md` - Update Phase 5 progress to 85%

### Benchmarks (Verify, may need minor tweaks)
- `benchmarks/toolbox/bench_toolbox_native_workloads.cpp` - Verify all 6 gates operational
- `benchmarks/toolbox/bench_toolbox_release_gates.cpp` - Verify 4 legacy gates (TBX-01..04)

### Documentation
- `benchmarks/toolbox/README.md` - Add baseline reference and regression testing guide

---

## Benchmark Hygiene Rules (Critical)

Per `benchmarks/MEASUREMENT_HYGIENE.md`:

✅ **Determinism:**
- Use `kCanonicalRngSeed = 42` for any randomization
- Steady_clock for all measurements (not UseRealTime)
- No timing-dependent assertions
- 5+ repetitions with aggregate reporting

✅ **Data Collection:**
- CSV output format for easy analysis
- Capture p50, p95, p99 percentiles
- Document hardware specs (CPU model, RAM, kernel version)

✅ **Regression Detection:**
- Compare current vs baseline in same environment
- 10% regression threshold (configurable)
- Flag outliers for investigation

---

## Expected Baseline Values (Estimates)

These are *estimates* for typical release hardware. Actual values will be measured:

| Gate | Expected Baseline | Typical Range |
|------|------------------|---------------|
| GATE-TBX-P1 (throughput) | 125K ops/s | 100K - 200K |
| GATE-TBX-P2 (latency) | 42ms p95 | 30ms - 60ms |
| GATE-TBX-P3 (latency) | 8ms p95 | 5ms - 15ms |
| GATE-TBX-P4 (latency) | 12ms p95 | 8ms - 20ms |
| GATE-TBX-P5 (throughput) | 1.2M ops/s | 1M - 2M |
| GATE-TBX-P6 (latency) | 78ms p95 | 50ms - 120ms |

---

## Stress Testing Plan (Phase 5 Optional)

If time permits, add stress tests to Phase 5:

1. **Long-Run Benchmark:** 100k+ iterations
2. **Concurrent Benchmark:** 8+ threads concurrent extraction
3. **Mixed Workload:** Combine extraction + fingerprinting + bridge

These stress benchmarks verify stability under sustained load.

---

## Success Criteria (Phase 5 Complete)

✅ Q3 2026 baseline collected on reference hardware  
✅ All 6 gates measured and documented  
✅ p95/p99 envelopes established  
✅ Proxy-to-native delta analyzed and documented  
✅ Regression budget (±10%) defined  
✅ All gates certified against thresholds  
✅ PERFORMANCE_EXPECTATIONS.md updated with measurements  
✅ Phase 5 progress: 50% → 85%  
✅ ROADMAP.md reflects completed items  
✅ No regressions identified  

---

## Next Steps After Phase 5

1. **Batch 3 Completion:** Deliver certified performance baseline
2. **Batch 4 Launch:** Phase 6 documentation finalization
3. **Q1 2027 Plan:** Direct suite maintenance + baseline updates

