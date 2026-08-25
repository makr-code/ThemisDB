# Server Phase 2 Performance Benchmarking Specification

**Project**: ThemisDB v2.4.0 - Wave B Server Performance Hardening  
**Version**: 2.4.0  
**Date**: 2026-08-18  
**Status**: 🟢 READY FOR EXECUTION  

---

## Benchmarking Overview

This document defines the performance benchmarking methodology to validate the 34 performance gaps closed in Wave B Server Phase 2.

**Benchmarking Goals**:
1. Establish baseline metrics (before optimization)
2. Measure metrics after optimization
3. Calculate improvement percentages
4. Verify +5-15% throughput target achievement
5. Identify any performance regressions (P99 latency, memory)

**Test Environment**:
- CPU: Intel Xeon (8+ cores, 2.5+ GHz)
- RAM: 16GB+ 
- Network: 10Gbps Ethernet
- OS: Linux 5.10+
- Build: Release mode (-O3), no sanitizers for performance tests

---

## Benchmark 1: Connection Pool Acquisition Rate

**Purpose**: Validate Gap S-001, S-002, S-008

**Benchmark Code** (from test_server_phase2_focused.cpp):
```cpp
BENCHMARK_F(ServerPhase2Benchmark, ConnectionPoolAcquisitionRate)
    (benchmark::State& state) {
    using Pool = GenericConnectionPool<MockConnection>;
    Pool pool(32, 256);
    
    for (auto _ : state) {
        auto conn = pool.acquire(1s);
        benchmark::DoNotOptimize(conn);
        if (conn) {
            pool.release(std::move(*conn));
        }
    }
    
    state.SetLabel("Pool acquire/release rate");
}
```

**Metrics to Capture**:
- Throughput: acquisitions/sec (target: > 100K/sec with pre-allocation)
- Latency: p50, p90, p99 acquisition time
- Memory: Peak memory usage of pool
- Allocations: Total heap allocations during test

**Baseline Expectations** (before):
- Throughput: ~30K acquisitions/sec (repeated malloc/free)
- P99 latency: 50-100 µs
- Allocations per acquire: 1 (malloc) + 1 (free)

**Target Expectations** (after):
- Throughput: >= 100K acquisitions/sec (50-70% improvement)
- P99 latency: < 5 µs
- Allocations per acquire: 0 (reuse from pool)

**Pass Criteria**:
- ✅ Throughput improvement >= 50% (from 30K to > 45K/sec)
- ✅ P99 latency < 10 µs
- ✅ Zero allocation spikes

---

## Benchmark 2: Buffer Append Efficiency

**Purpose**: Validate Gaps S-003, S-004, S-012, S-013

**Benchmark Code**:
```cpp
BENCHMARK_F(ServerPhase2Benchmark, BufferAppendEfficiency)
    (benchmark::State& state) {
    PreallocatedBuffer buffer(8192);
    std::vector<uint8_t> data(256, 0xFF);
    
    for (auto _ : state) {
        buffer.clear();
        for (int i = 0; i < 32; ++i) {
            buffer.append(data.data(), data.size());
            benchmark::DoNotOptimize(buffer);
        }
    }
    
    state.SetLabel("Buffer append (32x 256-byte appends)");
}
```

**Metrics to Capture**:
- Throughput: iterations/sec (32 appends per iteration = 8KB/iter)
- Effective throughput: MB/sec
- Latency: per-append time
- Reallocation count: number of reserve() calls per iteration

**Baseline Expectations** (before):
- Throughput: ~10K iterations/sec (80MB/sec)
- Append latency: 25-50 ns per append
- Reallocations per iteration: 3-5 (1.25-2.0x growth)

**Target Expectations** (after):
- Throughput: >= 15K iterations/sec (120MB/sec, +50%)
- Append latency: < 20 ns per append
- Reallocations per iteration: 0-1 (1.5x exponential growth)

**Pass Criteria**:
- ✅ Throughput improvement >= 30% (from 10K to > 13K iter/sec)
- ✅ Reallocation count <= 1 per 32 appends
- ✅ Latency per append < 30 ns

---

## Benchmark 3: HTTP/2 Stream Buffer Operations

**Purpose**: Validate Gaps H-001, H-003, H-007, H-008

**Benchmark Code**:
```cpp
BENCHMARK_F(ServerPhase2Benchmark, HTTP2StreamBufferOperations)
    (benchmark::State& state) {
    HTTP2StreamBuffer buffer;
    std::vector<uint8_t> data(512, 0x42);
    
    for (auto _ : state) {
        buffer.clear();
        for (int i = 0; i < 100; ++i) {
            buffer.append(data.data(), data.size());
            benchmark::DoNotOptimize(buffer);
        }
    }
    
    state.SetLabel("Stream buffer accumulation (100x 512-byte appends)");
}
```

**Metrics to Capture**:
- Throughput: iterations/sec (100 appends per iteration = 50KB/iter)
- Effective throughput: MB/sec
- Latency: per-append time
- Reallocation count: reserve() calls
- Cache efficiency: last_access_time updates

**Baseline Expectations** (before):
- Throughput: ~8K iterations/sec (400MB/sec)
- Append latency: 50-100 ns per append
- Reallocations per iteration: 5-7 (multiple resizes)
- Cache misses: frequent state lookups

**Target Expectations** (after):
- Throughput: >= 12K iterations/sec (600MB/sec, +50%)
- Append latency: < 30 ns per append
- Reallocations per iteration: 1-2 (exponential growth)
- Cache hits: timeout tracking efficient

**Pass Criteria**:
- ✅ Throughput improvement >= 40% (from 8K to > 11.2K iter/sec)
- ✅ Reallocation count <= 2 per 100 appends
- ✅ Append latency < 50 ns

---

## Benchmark 4: Query API Handler End-to-End

**Purpose**: Validate integrated Query API improvement (all 15 gaps)

**Benchmark Specification**:
```
Operation: Execute 1000 sequential queries
Query Size: Small (10 results), Medium (100 results), Large (1000 results)
Connection Pool: Pre-allocated 32 connections
Thread Count: 1, 4, 8, 16
Duration: 60 seconds each

Metrics:
- Throughput: queries/sec
- Latency: p50, p90, p99 query time
- Memory: Peak RSS, allocation rate
- CPU: User time, system time
- Timeout rate: % queries exceeding timeout
```

**Baseline Expectations** (before):
- Throughput: 100-200 QPS
- P99 latency: 10-50ms
- Memory: 500MB-1GB
- CPU: 80-90% user time

**Target Expectations** (after):
- Throughput: 150-300 QPS (+50-100%)
- P99 latency: 5-25ms (-50%)
- Memory: 300-600MB (-40%)
- CPU: 75-85% user time (better cache locality)

**Pass Criteria**:
- ✅ Throughput improvement >= 25% (on average across query sizes)
- ✅ P99 latency reduction >= 20%
- ✅ Memory usage reduction >= 20%
- ✅ Zero timeout errors

---

## Benchmark 5: HTTP/2 Session High Concurrency

**Purpose**: Validate HTTP/2 improvements (9 gaps) under concurrent load

**Benchmark Specification**:
```
Operation: Concurrent HTTP/2 stream writes and flushes
Stream Count: 32, 64, 128
Write Size: 1KB, 4KB, 16KB
Concurrency: 1-16 threads
Duration: 60 seconds each

Metrics:
- Throughput: MB/sec across all streams
- Latency: p50, p90, p99 stream flush time
- Memory: Peak RSS, allocation rate
- Synchronization: Lock contention (futex count)
- Cache efficiency: Buffer reallocation count
```

**Baseline Expectations** (before):
- Throughput: 100-200 MB/sec
- P99 latency: 1-5ms
- Memory: 256-512MB
- Buffer reallocations: 1000s per second

**Target Expectations** (after):
- Throughput: 150-300 MB/sec (+50-100%)
- P99 latency: 0.5-2ms (-60%)
- Memory: 150-300MB (-40%)
- Buffer reallocations: 100s per second (-90%)

**Pass Criteria**:
- ✅ Throughput improvement >= 50% (baseline-dependent)
- ✅ P99 latency reduction >= 40%
- ✅ Buffer reallocation reduction >= 80%
- ✅ Zero deadlocks/hangs under concurrency

---

## Benchmark 6: Export API Streaming

**Purpose**: Validate Export API improvements (4 gaps)

**Benchmark Specification**:
```
Operation: Export large datasets with backpressure
Dataset Size: 10MB, 50MB, 100MB, 500MB
Record Count: 100K, 500K, 1M, 5M
Export Format: JSONL
Backpressure: Simulate slow write (1MB/sec)
Timeout: 60 seconds

Metrics:
- Throughput: MB/sec
- Latency: p50, p90, p99 record write time
- Memory: Peak RSS (critical for large exports)
- Backpressure events: % time throttled
- Chunk efficiency: avg chunk size, count
```

**Baseline Expectations** (before):
- Throughput: 10-20 MB/sec
- P99 latency: 50-200ms per record
- Memory: 1-2GB (full buffer pre-allocation)
- Backpressure: None (potential OOM)

**Target Expectations** (after):
- Throughput: 15-30 MB/sec (+50%)
- P99 latency: 10-50ms per record (-60%)
- Memory: 400-800MB (-50-60%)
- Backpressure: Active, no OOM

**Pass Criteria**:
- ✅ Throughput improvement >= 30%
- ✅ Memory reduction >= 50%
- ✅ P99 latency reduction >= 50%
- ✅ Zero OOM conditions (backpressure active)
- ✅ Export timeout not exceeded

---

## Benchmark 7: Memory Allocation Impact

**Purpose**: Validate allocation reduction (Gaps S-002, S-004, E-001, E-002)

**Benchmark Specification** (using jemalloc stats):
```
Operation: Run all 4 handler workloads simultaneously
Duration: 300 seconds
Metrics captured:
- Total allocations
- Total deallocations
- Peak heap size
- Fragmentation ratio
- GC pause times
- Cache miss rate
```

**Baseline Expectations** (before):
- Total allocations: 10M+
- Fragmentation: 20-30%
- Peak heap: 1-2GB
- GC pauses: 10-50ms

**Target Expectations** (after):
- Total allocations: 5-7M (-40-50%)
- Fragmentation: 10-15%
- Peak heap: 400-600MB (-50%)
- GC pauses: 2-10ms

**Pass Criteria**:
- ✅ Allocation count reduction >= 30%
- ✅ Fragmentation ratio <= 15%
- ✅ Peak heap reduction >= 40%
- ✅ GC pause reduction >= 50%

---

## Benchmark Execution Plan

### Phase 1: Baseline Measurement (Before Optimization)
1. Build without performance_helpers integration
2. Run all 7 benchmarks, capture results to `baseline_metrics.json`
3. Record environment info (CPU, RAM, compiler, OS version)
4. Generate baseline report

### Phase 2: Apply Optimization
1. Integrate performance_helpers.h
2. Update handlers to use pool, buffers, guards
3. Clean rebuild in Release mode
4. Run smoke tests to verify functionality

### Phase 3: Optimized Measurement (After Optimization)
1. Run all 7 benchmarks, capture results to `optimized_metrics.json`
2. Verify environment consistency
3. Run 3 times to ensure reproducibility

### Phase 4: Comparison & Analysis
1. Calculate improvement % for each metric
2. Generate comparison charts (matplotlib/gnuplot)
3. Create summary table with pass/fail status
4. Document any surprises or regressions
5. Generate final report

### Execution Commands
```bash
# Build baseline
cmake -B build_baseline -DCMAKE_BUILD_TYPE=Release
cd build_baseline && make -j8

# Run baseline benchmarks
./module_server_test_server_phase2_focused_focused \
  --benchmark_out=baseline_metrics.json \
  --benchmark_out_format=json \
  --benchmark_time_unit=ms

# Build optimized
cmake -B build_optimized -DCMAKE_BUILD_TYPE=Release
cd build_optimized && make -j8

# Run optimized benchmarks
./module_server_test_server_phase2_focused_focused \
  --benchmark_out=optimized_metrics.json \
  --benchmark_out_format=json \
  --benchmark_time_unit=ms

# Compare results
python3 scripts/compare_benchmarks.py \
  baseline_metrics.json \
  optimized_metrics.json \
  --output comparison_report.html
```

---

## Performance Regression Criteria

**Critical Regressions** (would fail PR):
- ✗ Throughput regression > 5% on any benchmark
- ✗ P99 latency regression > 10%
- ✗ Memory regression > 5%
- ✗ Deadlock or hang on any concurrent test

**Acceptable Trade-offs** (OK for code simplicity):
- ✓ P50 latency regression <= 5% (if P99 improves)
- ✓ CPU usage increase <= 5% (if throughput improves)
- ✓ Context switch increase <= 10% (if throughput improves)

---

## Success Criteria Summary

| Metric | Target | Pass If |
|--------|--------|---------|
| Overall throughput | +5-15% | >= +5% improvement |
| Query API throughput | +5-8% | >= +4% improvement |
| HTTP/2 throughput | +2-4% | >= +1% improvement |
| Stream buffer reallocations | -70% | <= 30% of baseline |
| Export memory usage | -40% | <= 60% of baseline |
| Rope throughput | +1-3% | >= +0.5% improvement |
| P99 latency regression | < 1% | No regression OR improve |
| Allocation count reduction | -30% | >= -25% reduction |

**Overall Result**: ✅ PASS if at least 6/8 metrics meet targets

---

## Troubleshooting Guide

### If Throughput Doesn't Improve
1. Verify pre-allocation is actually active (add logging)
2. Check if contention is limiting factor (reduce thread count)
3. Profile with perf to identify hotspots
4. Consider if workload doesn't exercise optimizations

### If P99 Latency Regresses
1. Check for lock contention (futex instrumentation)
2. Verify RAII overhead is not significant
3. Profile allocation patterns
4. Consider increasing pool size if timeout-related

### If Memory Usage Increases
1. Verify pre-allocated buffers are being reused
2. Check for memory leaks (valgrind)
3. Monitor cache hit rate
4. Ensure buffers are cleared between uses

### If Tests Hang
1. Check for deadlock (timeout should prevent)
2. Verify condition_variable usage
3. Test with ThreadSanitizer
4. Reduce concurrency to isolate issue

---

## Final Report Template

**Performance Benchmark Report - v2.4.0 Phase 2**

```
Execution Date: [Date]
Environment: [CPU, RAM, OS, Compiler]

Benchmark Results:
1. Connection Pool Rate:       [X]K acq/sec (target: >100K)  ✅/❌
2. Buffer Append Efficiency:   [X]K iter/sec (target: >13K)  ✅/❌
3. HTTP/2 Stream Operations:   [X]K iter/sec (target: >11K)  ✅/❌
4. Query API E2E:              [X]% improvement (target: +25%) ✅/❌
5. HTTP/2 High Concurrency:    [X]% improvement (target: +50%) ✅/❌
6. Export Streaming:           [X]% improvement (target: +30%) ✅/❌
7. Memory Allocation Impact:   [X]% reduction (target: -30%)  ✅/❌

Overall Performance Improvement: [X]% (Target: +5-15%) ✅/❌

Regressions: [None / List any]
Recommendations: [List any follow-up optimizations]
```

---

**Document**: SERVER_PHASE2_BENCHMARKING_SPEC.md  
**Status**: 🟢 READY FOR EXECUTION  
**Version**: 2.4.0  

Ready to execute benchmarks after code integration and review approval.
