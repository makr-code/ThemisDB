# Tensor Fingerprint Graph: Stress Coverage Documentation

**Module**: tensor  
**Component**: TensorFingerprintGraph concurrent stress testing  
**Duration**: 2-3 weeks (Sept 1-21, 2026)  
**Branch**: feature/tensor-q4-determinism  
**Coordination**: Stream B Block B2  

---

## Overview

This document describes the comprehensive stress testing strategy for the `TensorFingerprintGraph` under high-concurrency, high-throughput, and chaotic failure scenarios.

### Objectives

1. **Throughput Validation**: Maintain >= 2,000 ops/sec under all workload profiles
2. **Memory Stability**: Ensure < 5% memory growth per 1M operations
3. **Latency Predictability**: Track P99 latency stability under sustained load
4. **Failure Resilience**: Validate graph consistency under chaos injection (random failures, delays, resource exhaustion)
5. **48h+ Design**: Demonstrate framework capability for extended stress runs

---

## Workload Profiles (8+ Scenarios)

All profiles use deterministic seeds (kCanonicalRngSeed = 42) for reproducibility.

### Profile 1: Query-Heavy (95% read, 5% write)

| Attribute | Value |
|-----------|-------|
| Name | QueryHeavy |
| Query Ratio | 95% |
| Store Ratio | 4% |
| Remove Ratio | 1% |
| Concurrency Level | 4 threads |
| Operation Count | 10,000 |
| Enable Chaos | No |
| Expected Throughput | >= 2,500 ops/sec |
| Expected P99 Latency | < 100ms |
| Use Case | Production query pattern (OLAP-dominated) |

**Description**: Simulates a typical production scenario where the majority of operations are read-only queries against the fingerprint graph. Heavy query load reveals lock contention and cache efficiency.

---

### Profile 2: Mixed (90% read, 10% write)

| Attribute | Value |
|-----------|-------|
| Name | Mixed |
| Query Ratio | 90% |
| Store Ratio | 7% |
| Remove Ratio | 3% |
| Concurrency Level | 8 threads |
| Operation Count | 50,000 |
| Enable Chaos | No |
| Expected Throughput | >= 2,200 ops/sec |
| Expected P99 Latency | < 500ms |
| Use Case | Typical OLTP workload |

**Description**: Balanced read-write workload representative of typical OLTP scenarios (adapter registration + frequent similarity queries). Tests fairness under mixed operations.

---

### Profile 3: Store-Heavy (75% read, 25% write)

| Attribute | Value |
|-----------|-------|
| Name | StoreHeavy |
| Query Ratio | 75% |
| Store Ratio | 20% |
| Remove Ratio | 5% |
| Concurrency Level | 8 threads |
| Operation Count | 50,000 |
| Enable Chaos | No |
| Expected Throughput | >= 1,800 ops/sec |
| Expected P99 Latency | < 1s |
| Use Case | Ingestion-dominated scenarios |

**Description**: Write-heavy pattern (adapter ingestion). Validates performance during bulk registration phases where insert load temporarily dominates.

---

### Profile 4: Saturated Reads (99% read, 1% write)

| Attribute | Value |
|-----------|-------|
| Name | SaturatedReads |
| Query Ratio | 99% |
| Store Ratio | 1% |
| Remove Ratio | 0% |
| Concurrency Level | 16 threads |
| Operation Count | 100,000 |
| Enable Chaos | No |
| Expected Throughput | >= 2,800 ops/sec |
| Expected P99 Latency | < 150ms |
| Use Case | Maximum read concurrency |

**Description**: Extreme read-dominant scenario. Tests shared-lock fairness and read-path scalability under maximum concurrency.

---

### Profile 5: High Concurrency Mixed (90% read, 8 threads)

| Attribute | Value |
|-----------|-------|
| Name | HighConcurrency |
| Query Ratio | 90% |
| Store Ratio | 7% |
| Remove Ratio | 3% |
| Concurrency Level | 16 threads |
| Operation Count | 100,000 |
| Enable Chaos | No |
| Expected Throughput | >= 2,000 ops/sec |
| Expected P99 Latency | < 800ms |
| Use Case | Lock contention analysis |

**Description**: Mixed workload under high concurrency (16 threads). Stress-tests shared_mutex fairness and wake-up semantics under contention.

---

### Profile 6: Chaos Injection (90% read with failures)

| Attribute | Value |
|-----------|-------|
| Name | ChaosInjection |
| Query Ratio | 90% |
| Store Ratio | 7% |
| Remove Ratio | 3% |
| Concurrency Level | 8 threads |
| Operation Count | 50,000 |
| Enable Chaos | Yes (5% failure rate) |
| Injected Delays | Random [0, 100µs] |
| Expected Throughput | >= 1,500 ops/sec |
| Expected P99 Latency | < 2s |
| Use Case | Failure resilience |

**Description**: Mixed workload with random operation failures (5% rate) and random delays (0-100µs). Validates graph remains consistent despite transient failures.

---

### Profile 7: Extreme Churn (40% read, 50% write, 10% remove)

| Attribute | Value |
|-----------|-------|
| Name | ExtremeChurn |
| Query Ratio | 40% |
| Store Ratio | 50% |
| Remove Ratio | 10% |
| Concurrency Level | 12 threads |
| Operation Count | 100,000 |
| Enable Chaos | No |
| Expected Throughput | >= 1,500 ops/sec |
| Expected P99 Latency | < 2s |
| Use Case | Memory pressure / high insert-delete rate |

**Description**: High insert-delete rate scenario. Validates memory stability when graph size fluctuates rapidly due to continuous registration and removal.

---

### Profile 8: Sustained Load (90% read, 8 threads, 1M+ ops)

| Attribute | Value |
|-----------|-------|
| Name | SustainedLoad |
| Query Ratio | 90% |
| Store Ratio | 7% |
| Remove Ratio | 3% |
| Concurrency Level | 8 threads |
| Operation Count | 500,000+ (48h design goal: 10M+) |
| Enable Chaos | No |
| Expected Throughput | >= 2,000 ops/sec |
| Expected P99 Latency | Stable (document per run) |
| Use Case | 48h+ continuous operation |

**Description**: Extended-duration workload for 48h+ stress capability. Designed as async-capable for CI integration. Validates memory stability over millions of operations and throughput consistency over time.

---

## Stress Test Suite (TSTRESS-01..20+)

All tests use deterministic fixtures (kCanonicalRngSeed = 42) for reproducibility.

### TSTRESS-01..03: Basic Throughput Validation

| Test | Operations | Threads | Profile | Expected Throughput | Success Criteria |
|------|-----------|---------|---------|-------------------|-----------------|
| TSTRESS-01 | 10k | 1 | QueryHeavy | >= 2,000 ops/sec | Latency < 10s |
| TSTRESS-02 | 50k | 4 | QueryHeavy | >= 2,000 ops/sec | Completion < 25s |
| TSTRESS-03 | 100k | 4 | QueryHeavy | >= 2,000 ops/sec | Completion < 60s |

**Purpose**: Establish throughput baseline across different operation scales.

---

### TSTRESS-04..06: Memory Stability

| Test | Operations | Profile | Memory Target | Success Criteria |
|------|-----------|---------|----------------|-----------------|
| TSTRESS-04 | 100k | Mixed | < 5% growth | Graph size < 100k |
| TSTRESS-05 | 500k | Mixed | < 5% growth | Graph size bounded |
| TSTRESS-06 | 100k | ExtremeChurn | < 10% growth | Graph size < 20k (high churn) |

**Purpose**: Validate memory doesn't grow unboundedly under sustained operations. Particularly important for Profile 7 (ExtremeChurn) where rapid insert-delete cycles can leak if cleanup is incomplete.

---

### TSTRESS-07..09: P99 Latency Tracking

| Test | Operations | Profile | Expected P99 | Validation |
|------|-----------|---------|--------------|------------|
| TSTRESS-07 | 50k | QueryHeavy | < 100ms | Read-path baseline |
| TSTRESS-08 | 50k | Mixed | < 500ms | Mixed ops P95 < P99 |
| TSTRESS-09 | 50k | StoreHeavy | < 1s | Write ops slower |

**Purpose**: Establish P99 latency baselines for each profile. Detect performance regressions.

---

### TSTRESS-10..12: Concurrent Mixed Workloads

| Test | Concurrency | Operations | Scaling |
|------|------------|-----------|---------|
| TSTRESS-10 | 4 threads | 50k | Linear scaling check |
| TSTRESS-11 | 8 threads | 80k | Contention at medium scale |
| TSTRESS-12 | 16 threads | 100k | Lock fairness under high contention |

**Purpose**: Verify performance scales (or degrades gracefully) with increasing thread count.

---

### TSTRESS-13..15: Chaos Injection (Failure Resilience)

| Test | Chaos Mode | Operations | Validation |
|------|-----------|-----------|-----------|
| TSTRESS-13 | 5% random failures | 50k | No crashes; graph consistent |
| TSTRESS-14 | 0-100µs random delays | 20k | P99 bounded < 5s |
| TSTRESS-15 | Combined failures + delays | 50k | Throughput >= 1500 ops/sec |

**Purpose**: Verify graph remains consistent under transient failures. Chaos injection strategies include:
- **Random Failures**: 5% of operations silently fail (simulating temporary backend errors)
- **Latency Injection**: Random 0-100µs delays (simulating network jitter)
- **Combined**: Both failures and delays active (realistic failure scenario)

---

### TSTRESS-16..18: Edge Stress Patterns

| Test | Profile | Description |
|------|---------|------------|
| TSTRESS-16 | ExtremeChurn | High insert-delete rate stress |
| TSTRESS-17 | SaturatedReads | 99% query load, 16 threads |
| TSTRESS-18 | SustainedLoad | 500k ops (scaled 48h design) |

**Purpose**: Validate edge cases and extended-duration capability.

---

### TSTRESS-19: Profile Consistency

**Purpose**: Meta-test validating all profiles are correctly configured.

---

### TSTRESS-20: Long-Duration Design Test

**Purpose**: Demonstrates framework capability for 48h+ runs (scaled in CI to 50k ops).

---

## Failure Injection Strategies

### 1. Random Operation Failures (5% rate)

```cpp
// Simulate transient backend errors
std::uniform_real_distribution<double> dis(0.0, 1.0);
if (dis(rng) < 0.05) {
    // Mark operation as failed
    stats.failed_operations++;
}
```

**Rationale**: Models real-world transient failures (network flakes, temporary lock contention).

---

### 2. Artificial Latency Injection (0-100µs random delays)

```cpp
// Simulate network/scheduler jitter
std::uniform_int_distribution<uint64_t> dis(0, 100);
auto delay_us = dis(rng);
std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
```

**Rationale**: Tests P99 latency stability under jitter and preemption.

---

### 3. Resource Exhaustion (Memory Pressure)

**Not Implemented in Base Test**: Advanced chaos (cgroup limits, malloc interception) deferred to future enhancement. Base suite focuses on operation-level failures.

---

## Memory Budget & Enforcement

### Budget Target
- **Baseline**: Empty graph ≈ 0 MB
- **Per 1M Operations**: < 5% growth (typically < 50 MB for 1M mixed ops)
- **Per Profile**:
  - **QueryHeavy**: Minimal growth (few store ops)
  - **ExtremeChurn**: Must remain < 50 MB due to high remove rate
  - **SustainedLoad**: Bounded at < 500 MB for 1M ops

### Monitoring Strategy

1. **Pre-Test**: Record graph.size() and estimate heap usage
2. **Per-Batch**: After each 1M ops, check size() and compare growth
3. **Post-Test**: Final size() must be consistent with operation counts

### Regression Detection

If memory growth exceeds 5% threshold:
1. Check for memory leaks in removeAdapter() path
2. Verify fingerprint vector cleanup
3. Inspect exact_train storage lifecycle

---

## Measurement Methodology

### Timing

- **Framework**: std::chrono::steady_clock (monotonic, unaffected by system time changes)
- **Granularity**: Nanoseconds (converted to ms/sec for reporting)
- **Per-Operation**: Record start/end times in LatencyTracker

### Throughput Calculation

```
Throughput (ops/sec) = (total_operations * 1e9) / elapsed_ns
```

### Percentile Calculation

```
P50 = value at 50% position when sorted
P95 = value at 95% position when sorted
P99 = value at 99% position when sorted
```

### Reproducibility

- **Seed**: kCanonicalRngSeed = 42 for all random generation
- **Hash-Based Seeding**: Per-adapter seeds derived from deterministic string hash
- **No System Randomness**: All RNG explicitly seeded; no std::random_device

---

## Determinism & Flakiness Prevention

### Test Isolation

1. **Per-Test Setup**: Fresh TensorFingerprintGraph instance (no shared state)
2. **Deterministic Profiles**: Fixed operation counts and thread counts
3. **No Timing Assertions**: Throughput assertions use >= threshold (not exact equality)
4. **Bounded Latency Assertions**: P99 assertions use < upper bound (not < p50)

### Thread Safety

- **shared_mutex**: Used in TensorFingerprintGraph
- **Memory Barriers**: Implicit in thread creation/joining (std::thread)
- **Atomic Counters**: Used for operation counting in workload mixer

---

## CI Integration Notes

### Test Timeout

- **TSTRESS-01..03, 07..09**: ~30s each (TIMEOUT 60)
- **TSTRESS-04..06, 10..12**: ~60s each (TIMEOUT 120)
- **TSTRESS-13..15**: ~90s each (TIMEOUT 180)
- **TSTRESS-16..18**: ~120s each (TIMEOUT 300+)
- **TSTRESS-20**: ~60s (TIMEOUT 120)

**Default**: Set TIMEOUT 300+ for safety on slower CI runners.

### Parallelization

All 20+ tests are independent and can run in parallel on multi-core CI runners. Recommended parallel execution: 4-8 workers.

### Memory Requirements

- **Per-Process**: ~200 MB (base + single test)
- **All Tests in Parallel**: ~2 GB (8 parallel tests × 250 MB each)

### Skipping Long Tests

For rapid iteration, skip TSTRESS-18 (SustainedLoad 500k):
```bash
ctest -E "TSTRESS18" --output-on-failure
```

---

## Performance Targets Summary

| Scenario | Requirement | Validation |
|----------|-------------|-----------|
| Basic Throughput (10k ops) | >= 2,000 ops/sec | TSTRESS-01 |
| Mixed Workload (90/10, 50k ops) | >= 2,200 ops/sec | TSTRESS-10 |
| Saturated Reads (99% query, 16t) | >= 2,800 ops/sec | TSTRESS-17 |
| Memory Growth (1M ops) | < 5% | TSTRESS-04, TSTRESS-05 |
| P99 Latency (Query-Heavy) | < 100ms | TSTRESS-07 |
| P99 Latency (Mixed) | < 500ms | TSTRESS-08 |
| Chaos Resilience (5% failures) | No crashes | TSTRESS-13 |
| Extended Run (500k ops) | >= 2,000 ops/sec | TSTRESS-18 |

---

## Reporting Template

After each test run, collect:

```markdown
# Stress Test Run Report
**Date**: 2026-09-XX
**Branch**: feature/tensor-q4-determinism
**Hardware**: [CPU model, RAM, OS]

## Summary
- Total Tests: 20
- Passed: XX/20
- Failed: 0/20
- Regressions: None

## Performance Metrics
| Test | Throughput (ops/sec) | P99 Latency (ms) | Memory Delta |
|------|-------------------|----------------|--------------|
| TSTRESS-01 | 2,450 | 42 | +5 MB |
| TSTRESS-02 | 2,380 | 56 | +12 MB |
| ... | ... | ... | ... |

## Observations
- [Any notable results or anomalies]
```

---

## Future Enhancements

1. **GPU-Accelerated Fingerprints**: Extended profiles for CUDA TT computations
2. **Distribution Fairness**: Verify shared_mutex doesn't favor readers (priority inversion)
3. **Cache Efficiency**: Profile CPU cache misses and TLB behavior
4. **Bulk Operations**: Test batch addAdapter/removeAdapter operations
5. **48h+ Extended Runs**: Full-duration runs on dedicated CI workers

---

## References

- **Workload Design**: Based on production query patterns from STREAM_B_Q4_2026_PLANNING.md
- **Measurement Hygiene**: benchmarks/MEASUREMENT_HYGIENE.md
- **Tensor API**: include/tensor/tensor_fingerprint_graph.h
- **Implementation**: src/tensor/tensor_fingerprint_graph.cpp
