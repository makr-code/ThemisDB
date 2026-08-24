# Access Model Phase 6 — Release Gate Verification Checklist

**Document Version:** 1.0.0  
**Status:** Verification Framework  
**Date:** 2026-08-17  

---

## Release Gate Verification

### Gate GATE-ACM-01: L1→L2 Promotion Latency

**Target:** ≤50µs p99  
**Hardware:** Intel Xeon  
**Test:** Single-shard, no I/O

**Verification Procedure:**

1. **Baseline Capture**
   ```bash
   cmake --preset windows-release
   cmake --build . --target bench_access_coordinator_gates --parallel 8
   ./benchmarks/access_model/bench_access_coordinator_gates \
     --benchmark_filter=GATE_ACM_01 \
     --benchmark_repetitions=5 \
     --benchmark_display_aggregates_only=true \
     --benchmark_out=gate_acm_01_baseline.json
   ```

2. **Baseline Verification**
   ```bash
   # Parse JSON output
   jq '.benchmarks[] | {name, cpu_time, wall_time}' gate_acm_01_baseline.json
   
   # Verify: wall_time p99 <= 50µs (0.000050 seconds)
   # Expected: "wall_time": "0.000045"  (45µs)
   ```

3. **Regression Check**
   ```bash
   # Store baseline
   cp gate_acm_01_baseline.json /var/lib/themisdb/baselines/acm_01.baseline.json
   
   # Run gate after changes
   ./benchmarks/access_model/bench_access_coordinator_gates \
     --benchmark_filter=GATE_ACM_01 > gate_acm_01_current.json
   
   # Compare: current should be within 10% of baseline
   python3 scripts/benchmark_compare.py \
     /var/lib/themisdb/baselines/acm_01.baseline.json \
     gate_acm_01_current.json \
     --tolerance_percent=10 \
     --fail_if_regressed=true
   ```

**Pass Criteria:**
- [ ] p99 latency ≤50µs
- [ ] No regression >10% vs baseline
- [ ] 5+ repetitions consistent within 5% variance
- [ ] Benchmark executable, no errors

---

### Gate GATE-ACM-02: Cache Eviction→Storage Feedback (End-to-End)

**Target:** ≤100µs p99  
**Hardware:** Intel Xeon  
**Test:** Round-trip: eviction event → coordinator → storage feedback

**Verification Procedure:**

1. **Setup Mock Tiers**
   ```cpp
   // Ensure test fixtures in benchmark include:
   // - MockCacheTier with EvictionListener
   // - MockStorageTier with PromotionListener
   // - Real AccessCoordinator
   ```

2. **Capture Round-Trip Latency**
   ```bash
   ./benchmarks/access_model/bench_access_coordinator_gates \
     --benchmark_filter=GATE_ACM_02 \
     --benchmark_min_time=0.1 \
     --benchmark_out=gate_acm_02.json
   
   jq '.benchmarks[] | .wall_time' gate_acm_02.json | sort -n | tail -1
   # Last value should be ≤ 0.0001 (100µs)
   ```

3. **Regression Validation**
   ```bash
   # Same as ACM-01 comparison procedure
   ```

**Pass Criteria:**
- [ ] p99 latency ≤100µs
- [ ] No regression >10% vs baseline
- [ ] E2E path verified (eviction → promotion → storage)

---

### Gate GATE-ACM-03: Cold→Warm Promotion (with Storage I/O)

**Target:** ≤100ms p99  
**Hardware:** Intel Xeon  
**Test:** ≤1MB object, includes storage I/O

**Verification Procedure:**

1. **Setup Storage I/O**
   ```bash
   # Use real or simulated storage (not mock)
   # Create 1000 x 1MB objects in cold tier
   
   ./benchmarks/access_model/bench_access_coordinator_gates \
     --benchmark_filter=GATE_ACM_03 \
     --benchmark_out=gate_acm_03.json
   ```

2. **Measure End-to-End**
   ```bash
   jq '.benchmarks[] | select(.name | contains("GATE_ACM_03"))
     | .wall_time' gate_acm_03.json | sort -n | tail -1
   # Should be ≤ 0.1 (100ms)
   ```

**Pass Criteria:**
- [ ] p99 latency ≤100ms
- [ ] Includes actual storage I/O (not mocked)
- [ ] Object size ≤1MB validated
- [ ] No regression >10%

---

### Gate GATE-ACM-04: Event Processing Throughput

**Target:** ≥10K events/sec  
**Hardware:** Intel Xeon  
**Test:** With logging + metrics enabled

**Verification Procedure:**

1. **Enable Instrumentation**
   ```cpp
   // Benchmark must run with:
   // - Structured logging enabled
   // - Metrics collection enabled
   // - Correlation ID propagation active
   ```

2. **Measure Throughput**
   ```bash
   ./benchmarks/access_model/bench_access_coordinator_gates \
     --benchmark_filter=GATE_ACM_04 \
     --benchmark_out=gate_acm_04.json
   
   # Extract: events_per_second
   jq '.benchmarks[] | .label' gate_acm_04.json
   # Expected: >= 10000 ops/sec
   ```

3. **Regression Check**
   ```bash
   # Same comparison procedure
   # Ensure throughput doesn't drop >10%
   ```

**Pass Criteria:**
- [ ] ≥10K events/sec (ops measured in millions per second in benchmark)
- [ ] All instrumentation active during benchmark
- [ ] No regression >10%

---

### Gate GATE-ACM-05: Memory Overhead

**Target:** ≤50MB  
**Hardware:** Intel Xeon  
**Test:** Coordinator + 1M pending events

**Verification Procedure:**

1. **Baseline Memory Measurement**
   ```bash
   # Before test
   ps aux | grep themisdb | awk '{print $6}' > /tmp/mem_before.txt
   
   # Run coordinator
   ./benchmarks/access_model/bench_access_coordinator_gates \
     --benchmark_filter=GATE_ACM_05
   
   # After test
   ps aux | grep themisdb | awk '{print $6}' > /tmp/mem_after.txt
   
   # Calculate delta
   python3 -c "
   before = int(open('/tmp/mem_before.txt').read().strip())
   after = int(open('/tmp/mem_after.txt').read().strip())
   print(f'Memory overhead: {(after - before) / 1024}MB')
   "
   ```

2. **Alternative: Valgrind Profiling**
   ```bash
   valgrind --tool=massif --massif-out-file=massif.out \
     ./benchmarks/access_model/bench_access_coordinator_gates \
     --benchmark_filter=GATE_ACM_05
   
   ms_print massif.out | tail -30
   # Look for peak memory usage
   ```

**Pass Criteria:**
- [ ] Peak memory ≤50MB
- [ ] Capacity: 1M+ pending events
- [ ] No unbounded growth detected

---

### Gate GATE-ACM-06: Policy Decision Overhead

**Target:** ≤10µs  
**Hardware:** Intel Xeon  
**Test:** Per eviction event

**Verification Procedure:**

1. **Isolated Policy Execution**
   ```bash
   ./benchmarks/access_model/bench_access_coordinator_gates \
     --benchmark_filter=GATE_ACM_06 \
     --benchmark_out=gate_acm_06.json
   
   jq '.benchmarks[] | .wall_time' gate_acm_06.json | sort -n | tail -1
   # Should be ≤ 0.00001 (10µs)
   ```

2. **Regression Check**
   ```bash
   # Same comparison procedure
   ```

**Pass Criteria:**
- [ ] p99 latency ≤10µs
- [ ] No regression >10%
- [ ] Policy decision isolated from I/O

---

## Overall Release Gate Checklist

### Pre-Gate Verification

- [ ] **Correctness:** All unit tests PASS (ACM-01..ACM-08)
- [ ] **Integration:** test_cache_storage_integration.cpp CAI-01..10 PASS
- [ ] **E2E:** test_access_model_e2e.cpp T1..T15 PASS (15 tests, <5s)
- [ ] **Concurrency:** test_coordination_concurrency.cpp C1..C10 PASS (TSan-clean)

### Gate Verification

- [ ] **GATE-ACM-01:** L1→L2 promotion ≤50µs p99 ✅
- [ ] **GATE-ACM-02:** Cache eviction→storage feedback ≤100µs p99 ✅
- [ ] **GATE-ACM-03:** Cold→warm promotion ≤100ms p99 ✅
- [ ] **GATE-ACM-04:** Event processing ≥10K events/sec ✅
- [ ] **GATE-ACM-05:** Memory overhead ≤50MB ✅
- [ ] **GATE-ACM-06:** Policy decision ≤10µs p99 ✅

### Diagnostic Verification

- [ ] Structured logging operational (10+ instrumentation points)
- [ ] Correlation IDs propagate end-to-end
- [ ] Metrics collection enabled (all 15+ metrics)
- [ ] Operator dashboard queries execute <1s

### Regression Validation

- [ ] Cache module benchmarks: ≤5% latency delta vs baseline
- [ ] Storage module benchmarks: ≤5% latency delta vs baseline
- [ ] Integration benchmarks: ≤5% latency delta vs baseline
- [ ] No new flaky tests introduced

### Sanitizer Validation

- [ ] **ASan:** 0 memory errors
- [ ] **UBSan:** 0 undefined behavior
- [ ] **TSan:** 0 data races
- [ ] **MSan:** 0 uninitialized reads

### Documentation Verification

- [ ] ROADMAP.md updated (Phase 5-6 complete)
- [ ] API documentation (Doxygen) complete
- [ ] Operator runbooks (5+ scenarios) complete
- [ ] Dashboard guide complete
- [ ] Performance baseline captured

---

## Gate Failure Resolution

**If any gate FAILS:**

1. **Immediate Escalation**
   - Notify core team (performance impact)
   - Document failure signature (metrics, logs)
   - Disable non-critical features if needed

2. **Root Cause Analysis**
   - Benchmark profiling (CPU, memory, I/O)
   - Code review of recent changes
   - Dependency version check

3. **Remediation**
   - Fix code or configuration
   - Re-run gate in isolation
   - Verify no regression in supporting tests

4. **Re-verification**
   - All gates must pass before release
   - Capture new baseline
   - Document all changes in PR

---

## Sign-Off Template

```
GATE VERIFICATION SIGN-OFF — Access Model Phase 6

Date: [YYYY-MM-DD]
Verifier: [Name / Team]
Environment: [Intel Xeon / AWS c5.2xlarge / Similar]

All Acceptance Criteria: [PASS / FAIL]

✅ GATE-ACM-01 (L1→L2 Promotion):      ≤50µs p99 ✓
✅ GATE-ACM-02 (Eviction→Storage):     ≤100µs p99 ✓
✅ GATE-ACM-03 (Cold→Warm Promotion): ≤100ms p99 ✓
✅ GATE-ACM-04 (Event Throughput):     ≥10K/sec ✓
✅ GATE-ACM-05 (Memory Overhead):      ≤50MB ✓
✅ GATE-ACM-06 (Policy Decision):      ≤10µs p99 ✓

Test Results:
  - E2E Tests: 15/15 PASS
  - Concurrency Tests: 10/10 PASS (TSan-clean)
  - Sanitizer: ASan/UBSan/TSan clean
  - Regressions: None detected

Approved For: v2.4.0 GA Release

Signature: [Name]
Date: [YYYY-MM-DD HH:MM UTC]
```

---

**Status:** Verification Framework Ready  
**Next Steps:** Execute gates after Phase 5-6 implementation complete

