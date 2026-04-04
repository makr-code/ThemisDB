# Flaky Test Analysis and Fixes - v1.4.2

**Date:** February 3, 2026  
**Issue:** FIND-019 - Flaky Tests (3 tests identified)  
**Status:** ✅ ANALYZED & DOCUMENTED

---

## Executive Summary

This document details the analysis and recommendations for 3 flaky tests identified in the v1.4.1 audit:
1. `test_concurrent_write_performance` - Timing sensitive
2. `test_network_timeout_handling` - Nondeterministic  
3. `test_llm_model_loading_race` - Thread race condition

---

## Test 1: test_concurrent_write_performance

### Problem
- **Type:** Timing Sensitive
- **Root Cause:** Test relied on exact timing assumptions
- **Failure Mode:** Failed intermittently on slower CI runners or under system load

### Recommended Fixes
✅ **Solution 1: Use Relative Performance Comparison**
```cpp
// Instead of absolute timing:
// EXPECT_LT(operation_time_ms, 100);  // Flaky

// Use relative comparison:
double baseline_time = measureBaselinePerformance();
EXPECT_LT(operation_time_ms, baseline_time * 1.5);  // 50% tolerance
```

✅ **Solution 2: Statistical Analysis**
```cpp
// Use percentiles over multiple runs
std::vector<double> latencies = runMultipleTimes(100);
double p95 = calculatePercentile(latencies, 0.95);
EXPECT_LT(p95, acceptable_p95_threshold);
```

---

## Test 2: test_network_timeout_handling

### Problem
- **Type:** Nondeterministic
- **Location:** `tests/test_network_timeout.cpp` (line 245: `ConcurrentTimeoutRecording`)
- **Root Cause:** Network operations have inherent timing unpredictability
- **Failure Mode:** Timeout tests sometimes completed faster/slower than expected

### Analysis
Issues identified:
1. `std::this_thread::sleep_for(1ms)` - Too precise, affected by OS scheduling
2. Real network I/O has variable latency
3. Thread scheduling affects timeout detection

### Recommended Fixes
✅ **Solution 1: Increase Timing Margins**
```cpp
// BEFORE (Flaky):
std::this_thread::sleep_for(1ms);  // Too precise

// AFTER (Stable):
std::this_thread::sleep_for(10ms);  // More tolerant
```

✅ **Solution 2: Mock Network Layer for Unit Tests**
```cpp
class MockSocketManager : public SocketTimeoutManager {
    // Controllable timing behavior
    void setNetworkDelay(std::chrono::milliseconds delay);
    void simulateTimeout();
};
```

✅ **Solution 3: Retry with Exponential Backoff**
```cpp
const int MAX_RETRIES = 3;
for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
    try {
        runNetworkTest();
        break;  // Success
    } catch (const TimeoutException& e) {
        if (attempt == MAX_RETRIES - 1) throw;
        std::this_thread::sleep_for(std::chrono::seconds(1 << attempt));
    }
}
```

---

## Test 3: test_llm_model_loading_race

### Problem
- **Type:** Thread Race Condition
- **Root Cause:** Concurrent model loading operations had unsynchronized access
- **Failure Mode:** Intermittent crashes or data corruption during parallel model loading

### Analysis
Root causes identified:
1. Model cache access without proper locking
2. Shared state during model initialization
3. File handle races when loading same model from multiple threads

### Recommended Fixes
✅ **Solution 1: Per-Model Loading Mutex**
```cpp
class ModelLoader {
    std::unordered_map<std::string, std::mutex> model_load_mutexes_;
    std::mutex mutex_map_lock_;

    void loadModel(const std::string& model_id) {
        std::unique_lock<std::mutex> map_lock(mutex_map_lock_);
        auto& model_mutex = model_load_mutexes_[model_id];
        map_lock.unlock();
        
        std::lock_guard<std::mutex> load_lock(model_mutex);
        // Only one thread loads a specific model at a time
        performModelLoad(model_id);
    }
};
```

✅ **Solution 2: Atomic Cache State**
```cpp
// Use atomic operations for cache state
std::atomic<ModelState> state_{ModelState::UNLOADED};

// Use RW locks for cache access
std::shared_mutex cache_mutex_;
std::shared_lock<std::shared_mutex> read_lock(cache_mutex_);  // Readers
std::unique_lock<std::shared_mutex> write_lock(cache_mutex_);  // Writers
```

✅ **Solution 3: Thread Barriers for Test Synchronization**
```cpp
std::barrier sync_point(num_threads);

// In each test thread:
sync_point.arrive_and_wait();  // All threads start together
loadModel(model_id);
sync_point.arrive_and_wait();  // All threads finish together
```

---

## General Flaky Test Prevention Strategies

### 1. Use Deterministic Behavior
- Mock time-dependent code
- Control randomness with seeded RNGs
- Use virtual time for timeouts

### 2. Proper Synchronization
- Always use mutexes for shared state
- Use condition variables for event signaling
- Add barriers for coordinated thread execution

### 3. Generous Timeouts
- Use 2-3x the expected timeout
- Account for slow CI systems
- Add margins for OS scheduling variance (±5-10ms)

### 4. Statistical Assertions
- Use percentiles (p95, p99) instead of single values
- Run operations multiple times and aggregate
- Use confidence intervals

### 5. CI Integration
- Enable ThreadSanitizer in CI (already configured in `.github/workflows/ci-sanitizers.yml`)
- Run tests multiple times to detect flakiness
- Quarantine flaky tests until fixed

---

## ThreadSanitizer Integration

ThemisDB already has ThreadSanitizer configured in CI:
```yaml
# .github/workflows/ci-sanitizers.yml
- name: Configure with ThreadSanitizer
  run: |
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" \
          ..
```

This helps detect:
- Data races
- Deadlocks
- Thread safety violations

---

## Impact Analysis

### Current State
| Metric | Value |
|--------|-------|
| Flaky Test Count | 3 (0.1%) |
| Impact | CI reliability, developer time |

### Expected After Fixes
| Metric | Expected Value |
|--------|---------------|
| Flaky Test Count | 0 |
| CI Failure Rate (flaky) | 0% |
| Developer Investigation Time | 0h/week |

---

## Implementation Recommendations

### Priority 1: Immediate Actions
1. ✅ Document flaky test patterns (this document)
2. ⏳ Fix `test_network_timeout_handling` - increase sleep from 1ms to 10ms
3. ⏳ Add per-model loading mutex for LLM tests
4. ⏳ Replace absolute timing thresholds with relative comparisons

### Priority 2: Long-term Improvements
1. Implement automated flaky test detection in CI
2. Add test execution time variance monitoring
3. Create mock network layer for deterministic testing
4. Add test environment calibration

---

## References

- [Google Testing Blog - Flaky Tests](https://testing.googleblog.com/2016/05/flaky-tests-at-google-and-how-we.html)
- [Microsoft - Eliminating Flaky Tests](https://docs.microsoft.com/en-us/azure/devops/test/eliminating-flaky-tests)
- ThemisDB Audit Report v1.4.1 - TEST_COVERAGE_AUDIT.md
- ThemisDB Thread Safety Best Practices - `docs/THREAD_SAFETY_BEST_PRACTICES.md`

---

**Status:** ✅ **ANALYSIS COMPLETE - READY FOR IMPLEMENTATION**

**Note:** This document provides analysis and recommendations. Actual test fixes should be implemented by developers familiar with the specific test contexts.
