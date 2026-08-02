# Phase 5 Block P5.1: Concurrency Performance Baseline

**Report Date:** 2026-08-02  
**Test Coverage:** test_aql_conversation_concurrency.cpp (8 test cases)  
**Status:** DRAFT - Ready for execution

## Executive Summary

Block P5.1 establishes performance baselines for concurrent AQL conversation handling. This block profiles:
1. Parallel conversation turns under shared context
2. Concurrent circuit breaker state transitions
3. Token budget exhaustion under concurrent load
4. Concurrent context eviction under memory pressure

All measurements are taken on Release-mode preset with CPU profiling enabled.

## Test Inventory

### test_aql_conversation_concurrency.cpp (8 Test Cases)

| Test Case | Scenario | Measurement | Expected p95 | Status |
|-----------|----------|-------------|---------------|--------|
| T5.1.1a | ParallelTurns_4ThreadsSharedContext | Latency (ms) | ≤ 50 ms | ⏳ Pending |
| T5.1.1b | ConversationTurnLatency_SingleThread | Latency (ms) | ≤ 5 ms | ⏳ Pending |
| T5.1.1c | CircuitBreakerStateTransition_Concurrent | Latency (µs) | ≤ 500 µs | ⏳ Pending |
| T5.1.1d | TokenBudgetExhaustion_ConcurrentAttempts | Latency (ms) | ≤ 20 ms | ⏳ Pending |
| T5.1.1e | ContextEviction_MemoryPressure | Latency (ms) | ≤ 100 ms | ⏳ Pending |
| T5.1.1f | RaceConditionDetection_SharedContext | Latency (µs) | ≤ 1000 µs | ⏳ Pending |
| T5.1.1g | ThreadSafety_ConcurrentValidation | Latency (µs) | ≤ 200 µs | ⏳ Pending |
| T5.1.1h | ContentionUnderLoad_10ThreadPool | Throughput (ops/sec) | ≥ 1000 ops/s | ⏳ Pending |

## Concurrency Baseline Measurements

### Parallel Conversation Turns (4 Threads, Shared Context)

**Measurement:** Latency distribution for parallel turn processing

```
Operation: ParallelTurns_4ThreadsSharedContext
Iterations: 10,000 samples
Concurrency: 4 threads
Context Sharing: Shared (synchronized access)

Expected Results:
  p50:  8 ms (50th percentile)
  p95: 45 ms (95th percentile - gate)
  p99: 75 ms (99th percentile)

Status: ⏳ PENDING EXECUTION
```

### Single Thread Conversation Turn Latency

**Measurement:** Baseline single-threaded latency for comparison

```
Operation: ConversationTurnLatency_SingleThread
Iterations: 50,000 samples
Concurrency: 1 thread
Context Sharing: N/A

Expected Results:
  p50:  1.5 ms
  p95:  4 ms   (gate)
  p99:  8 ms

Status: ⏳ PENDING EXECUTION
```

### Circuit Breaker State Transitions (Concurrent)

**Measurement:** Latency for circuit breaker state changes under concurrent access

```
Operation: CircuitBreakerStateTransition_Concurrent
Iterations: 100,000 samples
Concurrency: 8 threads triggering state changes
State Transitions: Open → Half-Open → Closed

Expected Results:
  p50:   50 µs
  p95:  400 µs (gate)
  p99:  800 µs

Status: ⏳ PENDING EXECUTION
```

### Token Budget Exhaustion Under Concurrent Load

**Measurement:** Latency when multiple threads attempt allocation at budget limit

```
Operation: TokenBudgetExhaustion_ConcurrentAttempts
Iterations: 10,000 samples
Concurrency: 4 threads
Budget Scenario: 1000 tokens total, 4 threads requesting 300 tokens each

Expected Results:
  p50:   3 ms
  p95:  18 ms (gate)
  p99:  35 ms

Status: ⏳ PENDING EXECUTION
```

### Context Eviction Under Memory Pressure

**Measurement:** Latency for context cleanup when memory limit approached

```
Operation: ContextEviction_MemoryPressure
Iterations: 1,000 samples
Memory Limit: 512 MB
Threads: 4
Contention: High (all threads filling context)

Expected Results:
  p50:  20 ms
  p95:  80 ms  (gate)
  p99: 150 ms

Status: ⏳ PENDING EXECUTION
```

### Race Condition Detection (Shared Context)

**Measurement:** Latency when synchronized access prevents race conditions

```
Operation: RaceConditionDetection_SharedContext
Iterations: 50,000 samples
Concurrency: 8 threads
Critical Section: Context update

Expected Results:
  p50:  100 µs
  p95:  800 µs (gate)
  p99: 1500 µs

Status: ⏳ PENDING EXECUTION
```

### Thread-Safe Validation

**Measurement:** Latency for AQL validation under concurrent calls

```
Operation: ThreadSafety_ConcurrentValidation
Iterations: 100,000 samples
Concurrency: 8 threads
Validator Sharing: Shared (thread-safe)

Expected Results:
  p50:   50 µs
  p95:  180 µs (gate)
  p99:  400 µs

Status: ⏳ PENDING EXECUTION
```

### Contention Under Load (10-Thread Pool)

**Measurement:** Throughput (operations per second) with thread pool contention

```
Operation: ContentionUnderLoad_10ThreadPool
Duration: 30 seconds
Thread Pool Size: 10
Operations: Concurrent turn processing
Memory Pressure: Moderate

Expected Results:
  p50:  1500 ops/s
  p95:  1200 ops/s (gate - minimum throughput under contention)
  p99:   900 ops/s

Status: ⏳ PENDING EXECUTION
```

## AddressSanitizer Memory Safety Verification

### Thread Safety Checks

When running with AddressSanitizer and ThreadSanitizer enabled:

```bash
cmake -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON \
       -DBUILD_TESTS=ON \
       -DCMAKE_BUILD_TYPE=Release \
       -DTHEMIS_EDITION=COMMUNITY \
       -DTHEMIS_ENABLE_ASAN=ON \
       -DTHEMIS_ENABLE_TSAN=ON \
       .
```

### Expected Results

- ✅ ZERO memory leaks detected
- ✅ ZERO use-after-free errors
- ✅ ZERO data races detected
- ✅ ZERO thread safety violations
- ✅ All synchronization primitives correctly used

### Specific Memory Safety Checks

| Check | Expected | Verified |
|-------|----------|----------|
| Context memory properly freed | No leaks | ⏳ Pending |
| No double-delete in cleanup | No UaF | ⏳ Pending |
| Proper mutex/lock usage | No races | ⏳ Pending |
| Stack allocation safety | No overflows | ⏳ Pending |
| Heap allocation safety | No UAF | ⏳ Pending |

## Performance Variance Analysis

### Baseline Variance Target

**Goal:** < 5% variance across repeated runs to ensure stability

| Metric | Target Variance | Measurement Method |
|--------|-----------------|-------------------|
| p50 Latency | ± 2% | (max - min) / avg × 100 |
| p95 Latency | ± 3% | (max - min) / avg × 100 |
| p99 Latency | ± 5% | (max - min) / avg × 100 |
| Throughput | ± 3% | (max - min) / avg × 100 |

### Variance Analysis Procedure

1. Run each test 5 times on the same system
2. Collect p50/p95/p99 metrics for each run
3. Calculate standard deviation and coefficient of variation
4. Verify all measurements within 5% variance band

**Example:**
```
Run 1 p95: 45.2 ms
Run 2 p95: 44.8 ms
Run 3 p95: 45.5 ms
Run 4 p95: 44.9 ms
Run 5 p95: 45.1 ms

Mean: 45.1 ms
Variance: ±0.24 ms (0.53%) ✅ PASS (< 5%)
```

## Hardware Baseline Configuration

**Measurement Environment:**
- CPU: x86-64, ≥ 3 GHz, ≥ 8 cores
- Memory: ≥ 16 GB RAM
- Build: Release mode (-O3)
- Concurrency: 4-10 threads (as per test specification)

**CPU Frequency Scaling (if applicable):**
- Disable frequency scaling for consistent measurements
- Pin threads to specific cores if possible
- Verify CPU frequency stable throughout run

## Execution Instructions

### Build Phase 5.1 Tests

```bash
cd /home/runner/work/ThemisDB/ThemisDB

# Configure
cmake -DTHEMIS_AUTO_BOOTSTRAP_DEPS=ON \
       -DTHEMIS_ALLOW_MISSING_ROCKSDB=ON \
       -DBUILD_TESTS=ON \
       -DCMAKE_BUILD_TYPE=Release \
       -DTHEMIS_EDITION=COMMUNITY \
       -DTHEMIS_ENABLE_MIMALLOC=OFF \
       .

# Build concurrency tests
cmake --build . --target module_aql_test_aql_conversation_concurrency_focused --parallel 4
```

### Run With Profiling

```bash
# Run with verbose output to capture timing
ctest --verbose -R "AQLConversationConcurrency" --timeout 300

# Run with TSAN for thread safety verification
TSAN_OPTIONS="suppress_equal_stacks=true:suppress_equal_addresses=true" \
  ctest --verbose -R "AQLConversationConcurrency" --timeout 300

# Run with perf (Linux only)
perf stat -e cycles,instructions,cache-references,cache-misses \
  ./bin/test_aql_conversation_concurrency
```

### Collect Latency Distributions

Tests should output timing data in format:
```
[ParallelTurns_4ThreadsSharedContext]
  p50:    8.2 ms
  p95:   45.1 ms  ← Gate measurement
  p99:   74.8 ms
  count:  10000
  duration: 125.3 s
```

## Success Criteria

### All Baselines Must Be Established

- [ ] All 8 tests execute successfully
- [ ] All latency measurements collected
- [ ] All p95 gates within expected ranges
- [ ] Variance analysis < 5% on all metrics
- [ ] AddressSanitizer verifies zero memory issues
- [ ] ThreadSanitizer detects zero data races
- [ ] All results documented in this report

### Phase 5.1 Exit Criteria

When all above criteria met:
- ✅ Concurrency baseline established
- ✅ Concurrent access patterns verified thread-safe
- ✅ No memory safety issues under concurrent load
- ✅ Performance meets gate thresholds
- ✅ Ready to proceed to Block P5.2

## Recommendations

1. **Immediate**: Run all 8 concurrency tests on Release preset
2. **Validation**: Compare p95 measurements to gates; adjust if needed
3. **Hardening**: Any variance > 5% indicates potential tuning needed
4. **Documentation**: Archive detailed profiling output for future reference

## Next Steps

- After P5.1 completion: Proceed to Block P5.2 (Degraded-Mode Performance)
- Integrate baselines into CI/CD performance regression detection
- Use gate thresholds for ongoing release gating

---

**Report Status:** DRAFT  
**Report Date:** 2026-08-02  
**Next Update:** After test execution (Week 3 end)

