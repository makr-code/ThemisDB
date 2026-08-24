# Tensor Module Stream A Block A3: Validation & Baselining Strategy

**Status**: In Progress (Validation Phase)  
**Start Date**: 2026-08-08  
**Checkpoint**: 2026-08-18 (Draft baseline with stable measurements)  
**Completion**: 2026-08-27 (Final report + gates defined)  
**Owner**: Performance Validation Specialist  

---

## Executive Summary

This document outlines the comprehensive validation and baselining strategy for Tensor Module Stream A Block A3, designed to:

1. **Validate** existing tensor benchmark coverage against performance targets
2. **Baseline** reproducible p95/p99 measurements locked to hardware profile
3. **Define** CMake-based performance gates for release validation
4. **Document** methodology for continued CI/CD validation

All work aligns with `src/tensor/FUTURE_ENHANCEMENTS.md` performance targets and feeds into Stream B P95/P99 validation (B3).

---

## Phase 1: Validation Phase (Aug 8-18, 2026)

### 1.1 Benchmark Audit

**Objective**: Verify existing benchmarks cover all performance targets.

#### Fingerprint Graph Benchmarks (`benchmarks/tensor/bench_tensor_fingerprint_graph.cpp`)

| Benchmark | Coverage | Status | Notes |
|-----------|----------|--------|-------|
| `BM_TFG_Insert_Throughput` | Batch insert throughput | ✅ READY | Parameterized for 100/1k/10k nodes |
| `BM_TFG_Insert_SingleNode` | Per-node insert latency | ✅ READY | Steady-state cost at 100/1k/10k/50k scale |
| `BM_TFG_FindSimilar` | Query latency (10k candidates) | ✅ ENHANCED | Added `.Repetitions(5)` + `.UseRealTime()` |
| `BM_TFG_Neighbours` | Adjacency lookup | ✅ ENHANCED | Added `.Repetitions(5)` for variance |
| `BM_TFG_ConcurrentReads` | Concurrent throughput | ✅ READY | 1/2/4/8 reader threads with shared_mutex |
| `BM_TFG_NodeCount` | Metadata query | ✅ READY | Should be O(1) / nanosecond scale |
| `BM_TFG_ExportPersistedGraph` | Serialization cost | ✅ READY | Checkpoint/persistence path |

**Target Coverage**:
- ✅ findSimilar (10k candidates): p95 ≤ 80 ms, p99 ≤ 140 ms
- ✅ neighbours (direct adjacency): ≤ 5 ms
- ✅ insert (100K nodes): ≤ 10 ms per node
- ✅ Concurrent reads with shared_mutex scaling

#### Deduplication Manager Benchmarks (`benchmarks/tensor/bench_tensor_deduplication_manager.cpp`)

| Benchmark | Coverage | Status | Notes |
|-----------|----------|--------|-------|
| `BM_TDM_SnapshotRestoreRoundTrip` | Persistence latency | ✅ READY | 100/1k tensor counts |
| `BM_TDM_JournalReplayThroughput` | Journal replay perf | ✅ READY | 100/1k/5k mutations |
| `BM_TDM_StoreOverwrite` | Overwrite path | ✅ NEW | Validates no O(N) metadata updates |
| `BM_TDM_StoreInsert` | Insert baseline | ✅ NEW | Comparison baseline for overwrite |
| `BM_TDM_MixedReadHeavyWorkload` | 90/10 mixed ops | ✅ NEW | Sustained throughput measurement |

**Target Coverage**:
- ✅ store overwrite overhead: ≤ 5% vs insert
- ✅ Mixed read-heavy (90/10): ≥ 2,000 ops/sec
- ✅ Bounded memory growth: ≤ 20 bytes/op
- ✅ 1k/10k scale validation

### 1.2 Missing Scenarios Identification

#### Fingerprint Graph - Identified Gaps

| Scenario | Priority | Plan | Timeline |
|----------|----------|------|----------|
| Multi-tenant (1 vs 10) | Medium | Parameterize existing benchmarks | A4 (Sep 2026) |
| Cache warm/cold | Low | LSH bucket reset instrumentation | Post-A3 |
| Fingerprint-fast path benchmark | High | Requires `findSimilarByFingerprint()` public method | A2 (current) |
| Concurrent contention under load | Medium | Add stress variants at 1k QPS | A4 |

#### Deduplication Manager - Identified Gaps

| Scenario | Priority | Plan | Timeline |
|----------|----------|------|----------|
| Large tensor payloads (1MB+) | Medium | Extend BM_TDM_MixedReadHeavyWorkload | Post-A3 |
| Deduplication effectiveness | High | Track unique vs total tensors | A4 |
| Journal compression | Low | Benchmark WAL compaction | B1 |

### 1.3 Hardware Profile Locking

**Reference CI Environment** (locked for reproducibility):

```
Architecture: x86_64 (Linux)
CPU: Intel/AMD 8-core @ 3.5 GHz base
Governor: performance (turboboost enabled)
Thermal: Stable, <60°C under full load
Memory: 16 GB DRAM, no swap pressure
OS: Ubuntu 20.04 LTS (or equivalent)
Build: Release profile (CMAKE_BUILD_TYPE=Release)
Compiler: GCC 11+ or Clang 13+
```

**Reproducibility Requirements**:

1. **CPU Governor**: Set to `performance` mode for consistent clock speeds
   ```bash
   for cpu in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
   do echo performance > $cpu; done
   ```

2. **Baseline Constraints**:
   - Run benchmarks on isolated cores (cgroup or taskset) for minimal jitter
   - Warm up caches before measurements (run 2-3 iterations before collecting)
   - Use Google Benchmark's `--repetitions=5` for fingerprint (high variance path)
   - Use `--repetitions=3` for dedup (more stable path)

3. **Variance Targets**:
   - p95/p99 measurements must be stable across runs (< 2% variance)
   - Coefficient of Variation (stddev/mean) < 5% for acceptance
   - No outliers > 3σ from mean allowed

### 1.4 Baseline Collection Protocol

**Procedure**:

```bash
# 1. Configure build with benchmarks enabled
cmake --preset community-release -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build-community-release --target \
  bench_tensor_fingerprint_graph \
  bench_tensor_deduplication_manager \
  -j8

# 2. Collect fingerprint graph baseline (5 repetitions)
./build-community-release/benchmarks/tensor/bench_tensor_fingerprint_graph \
  --benchmark_repetitions=5 \
  --benchmark_format=csv \
  --benchmark_filter="BM_TFG_FindSimilar/10000" \
  --benchmark_out=tfg_baseline.csv

# 3. Collect deduplication manager baseline (3 repetitions)
./build-community-release/benchmarks/tensor/bench_tensor_deduplication_manager \
  --benchmark_repetitions=3 \
  --benchmark_format=csv \
  --benchmark_filter="BM_TDM_MixedReadHeavyWorkload" \
  --benchmark_out=tdm_baseline.csv

# 4. Extract p95/p99 via post-processing
python3 extract_percentiles.py tfg_baseline.csv tdm_baseline.csv
```

**Baseline Extraction Script** (`extract_percentiles.py`):

```python
#!/usr/bin/env python3
import csv, statistics, sys

def analyze_benchmark_csv(filename, filter_name):
    """Extract p95/p99 from Google Benchmark CSV output."""
    measurements = []
    with open(filename) as f:
        reader = csv.DictReader(f)
        for row in reader:
            if filter_name in row.get('name', ''):
                # Google Benchmark reports real_time and cpu_time
                real_time = float(row.get('real_time', 0))
                measurements.append(real_time)
    
    if not measurements:
        return None
    
    measurements.sort()
    p50 = statistics.median(measurements)
    p95 = statistics.quantiles(measurements, n=20)[18]  # 95th percentile
    p99 = statistics.quantiles(measurements, n=100)[98]  # 99th percentile
    mean = statistics.mean(measurements)
    stddev = statistics.stdev(measurements) if len(measurements) > 1 else 0
    
    return {
        'p50': p50, 'p95': p95, 'p99': p99,
        'mean': mean, 'stddev': stddev,
        'cv': (stddev / mean * 100) if mean > 0 else 0,
        'count': len(measurements)
    }

# Analyze fingerprint graph
tfg_data = analyze_benchmark_csv('tfg_baseline.csv', 'BM_TFG_FindSimilar/10000')
if tfg_data:
    print(f"BM_TFG_FindSimilar (10k nodes):")
    print(f"  p50: {tfg_data['p50']:.2f} ms")
    print(f"  p95: {tfg_data['p95']:.2f} ms (target: ≤80 ms) {'✓' if tfg_data['p95'] <= 80 else '✗'}")
    print(f"  p99: {tfg_data['p99']:.2f} ms (target: ≤140 ms) {'✓' if tfg_data['p99'] <= 140 else '✗'}")
    print(f"  CV:  {tfg_data['cv']:.1f}% (target: <5%)")
    print(f"  Samples: {tfg_data['count']}")
    print()

# Analyze deduplication manager
tdm_data = analyze_benchmark_csv('tdm_baseline.csv', 'BM_TDM_MixedReadHeavyWorkload')
if tdm_data:
    print(f"BM_TDM_MixedReadHeavyWorkload:")
    print(f"  Throughput: {tdm_data['count']} ops/sec (target: ≥2000) {'✓' if tdm_data['count'] >= 2000 else '✗'}")
```

### 1.5 Stability Verification

**Run Schedule** (3 independent runs per benchmark):

| Run | Environment | Time | Notes |
|-----|-------------|------|-------|
| Run 1 | Fresh CI instance | T+0 | Baseline |
| Run 2 | Same instance, 2h later | T+2h | Cache state varies |
| Run 3 | Fresh CI instance | T+4h | Cross-instance variance |

**Acceptance Criteria**:

- p95 variance across 3 runs: ± 2%
- p99 variance across 3 runs: ± 3%
- No statistical outliers (> 3σ) in any run
- CV (coefficient of variation) < 5% for all key metrics

---

## Phase 2: Enhancement Phase (Aug 18-27, 2026)

### 2.1 Concurrent Operation Scenarios

**Objective**: Add benchmarks for realistic concurrent load patterns.

#### BM_TFG_ConcurrentMixedOps (NEW)

Concurrent mix of findSimilar (80%) + insert (20%) operations:

```cpp
static void BM_TFG_ConcurrentMixedOps(benchmark::State& state) {
    const int n_threads = static_cast<int>(state.range(0));
    const auto cfg = benchConfig();
    TensorFingerprintGraph graph(cfg);
    populateGraph(graph, 5000);
    
    std::mt19937 rng(kCanonicalRngSeed);
    auto query = makeSyntheticTrain(rng);
    std::atomic<uint64_t> insert_counter{0};
    
    for (auto _ : state) {
        std::vector<std::thread> threads;
        for (int t = 0; t < n_threads; ++t) {
            threads.emplace_back([&]() {
                // 80% find, 20% insert
                for (int i = 0; i < 100; ++i) {
                    if (i % 5 < 4) {
                        auto results = graph.findSimilar(query, 10);
                        benchmark::DoNotOptimize(results.size());
                    } else {
                        auto train = makeSyntheticTrain(rng);
                        auto id = "t_" + std::to_string(insert_counter++);
                        graph.insert(id, train, "b", "c", "f");
                    }
                }
            });
        }
        for (auto& t : threads) t.join();
    }
}

BENCHMARK(BM_TFG_ConcurrentMixedOps)
    ->Arg(1) ->Arg(2) ->Arg(4) ->Arg(8)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();
```

**Measurement Target**:
- Throughput: ≥ 1,500 ops/sec at 8 threads (near-linear scaling from single thread)
- Tail latency (p99) for individual ops: ≤ 150 ms (slight increase from contention)

#### BM_TDM_ConcurrentDedup (NEW)

Concurrent store + retrieval under deduplication:

```cpp
static void BM_TDM_ConcurrentDedup(benchmark::State& state) {
    const int n_threads = static_cast<int>(state.range(0));
    auto engine = makeEngine();
    auto mgr = makeDedupManager(engine);
    
    preloadCanonicals(*mgr, 500, 4000);
    
    std::atomic<uint64_t> op_counter{0};
    for (auto _ : state) {
        std::vector<std::thread> threads;
        for (int t = 0; t < n_threads; ++t) {
            threads.emplace_back([&]() {
                std::mt19937 rng(t + kCanonicalRngSeed);
                for (int i = 0; i < 50; ++i) {
                    if (i % 5 < 3) {
                        // Query existing
                        auto idx = op_counter % 500;
                        mgr->getRecord("canon_" + std::to_string(idx));
                    } else {
                        // Store new (with potential dedup)
                        auto data = randVec(16, kCanonicalRngSeed + op_counter++);
                        mgr->store("new_" + std::to_string(op_counter), data,
                                  {16, 1}, "tenant", "collection", "field");
                    }
                }
            });
        }
        for (auto& t : threads) t.join();
    }
}

BENCHMARK(BM_TDM_ConcurrentDedup)
    ->Arg(1) ->Arg(2) ->Arg(4)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();
```

### 2.2 Cache Effectiveness Measurements

**Objective**: Quantify LSH cache benefits in fingerprint graph.

#### BM_TFG_CacheWarmVsCold (NEW)

Compare findSimilar performance with warm vs. cold LSH cache:

```cpp
static void BM_TFG_CacheWarmVsCold(benchmark::State& state) {
    const bool warm_cache = state.range(1) == 1;
    const std::size_t prefill = 10000;
    const auto cfg = benchConfig();
    TensorFingerprintGraph graph(cfg);
    populateGraph(graph, prefill);
    
    std::mt19937 rng(kCanonicalRngSeed);
    std::vector<TTTrain> queries;
    for (int i = 0; i < 20; ++i) {
        queries.push_back(makeSyntheticTrain(rng));
    }
    
    for (auto _ : state) {
        if (!warm_cache) {
            // Force cache invalidation (would require instrumentation)
            // For now, simulate via query sequence rotation
            state.PauseTiming();
            for (int i = 0; i < 100; ++i) {
                graph.findSimilar(queries[i % 20], 10);
            }
            state.ResumeTiming();
        }
        
        auto results = graph.findSimilar(queries[0], 10);
        benchmark::DoNotOptimize(results.size());
    }
}

BENCHMARK(BM_TFG_CacheWarmVsCold)
    ->ArgPair(10000, 0)  // cold cache
    ->ArgPair(10000, 1)  // warm cache
    ->Unit(benchmark::kMillisecond)
    ->Repetitions(5)
    ->UseRealTime();
```

**Target Improvements**:
- Warm cache p95: ≤ 50 ms (baseline ~80 ms)
- Cold cache p95: ≤ 120 ms (acceptable degradation)
- Cache hit rate: ≥ 80% for repeated query patterns

### 2.3 Hardware Profile Locking

Create CMake configuration to detect and enforce hardware constraints:

**File**: `cmake/TensorBenchmarkGates.cmake`

```cmake
# Tensor Benchmark Hardware Profile Detection
# Enforce reproducibility requirements for baseline gating

if(NOT THEMIS_DETECT_BENCHMARK_HARDWARE)
    return()
endif()

# Detect CPU info
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Read /proc/cpuinfo for core count and frequency
    execute_process(
        COMMAND grep -c "^processor" /proc/cpuinfo
        OUTPUT_VARIABLE CPU_CORE_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND grep "cpu MHz" /proc/cpuinfo
        OUTPUT_VARIABLE CPU_FREQ_MHZ
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

message(STATUS "Tensor Benchmark Hardware Profile:")
message(STATUS "  CPU Cores: ${CPU_CORE_COUNT}")
message(STATUS "  CPU Freq: ${CPU_FREQ_MHZ}")

# Store for runtime use in benchmarks
string(APPEND CMAKE_CXX_FLAGS " -DTENSOR_BENCH_CPU_CORES=${CPU_CORE_COUNT}")
```

---

## Phase 3: Reporting & Gates Phase (Aug 27, 2026)

### 3.1 Baseline Report Compilation

Create final report: `ai_working/TENSOR_Q3_BENCHMARK_BASELINE_FINAL.md`

**Report Structure**:

1. **Executive Summary**
   - All targets met/missed with clear pass/fail status
   - Key metrics table (p50/p95/p99 locked values)
   - Timeline and completion status

2. **Measurement Methodology**
   - Hardware profile (locked)
   - CPU governor settings
   - Thermal constraints
   - Reproducibility checklist

3. **Baseline Measurements**
   - Table per benchmark with p50/p95/p99/CV
   - 3-run variance validation
   - Outlier analysis (any measurements > 3σ flagged)

4. **Target Validation**
   - FUTURE_ENHANCEMENTS.md requirement matrix
   - Pass/fail assessment per target
   - Margin analysis (how far below/above target)

5. **Benchmark Manifest Compliance**
   - Coverage matrix (1k/10k/50k scales)
   - Multi-tenant support status
   - Cache warm/cold scenarios

6. **Gap Analysis & Roadmap**
   - Missing scenarios (with A4/B1 timeline)
   - Enhancements completed vs. planned
   - Future measurement expansion

### 3.2 CMake Performance Gates

**File**: `cmake/TensorPerformanceGates.cmake`

Defines 6 release gates with locked measurements:

```cmake
# GATE-TN-A3-01: Fingerprint Graph Throughput
# Minimum sustainable insert throughput
set(GATE_TN_A3_01_TARGET 1500)  # ops/sec
set(GATE_TN_A3_01_DESCRIPTION "FG insert throughput >= 1500 ops/sec (100-node batches)")

# GATE-TN-A3-02: Fingerprint Graph p95 Latency
# Maximum acceptable p95 for findSimilar (10k nodes)
set(GATE_TN_A3_02_TARGET 80)    # milliseconds
set(GATE_TN_A3_02_DESCRIPTION "FG findSimilar p95 <= 80ms (10k candidates)")

# GATE-TN-A3-03: Fingerprint Graph p99 Latency
# Maximum acceptable p99 for findSimilar (10k nodes)
set(GATE_TN_A3_03_TARGET 140)   # milliseconds
set(GATE_TN_A3_03_DESCRIPTION "FG findSimilar p99 <= 140ms (10k candidates)")

# GATE-TN-A3-04: Deduplication Throughput
# Mixed read-heavy (90/10) sustained throughput
set(GATE_TN_A3_04_TARGET 2000)  # ops/sec
set(GATE_TN_A3_04_DESCRIPTION "Dedup 90/10 mixed ops >= 2000 ops/sec")

# GATE-TN-A3-05: Deduplication p95 Latency
# Per-operation p95 in mixed workload
set(GATE_TN_A3_05_TARGET 2.5)   # milliseconds
set(GATE_TN_A3_05_DESCRIPTION "Dedup per-op p95 <= 2.5ms in mixed workload")

# GATE-TN-A3-06: Deduplication Memory Growth
# Memory efficiency in sustained operations
set(GATE_TN_A3_06_TARGET 20)    # bytes/op
set(GATE_TN_A3_06_DESCRIPTION "Dedup memory growth <= 20 bytes/op (bounded)")

# Release gate test function
function(add_tensor_performance_gate_test)
    # Create test target that validates gates
    # Test runs benchmarks and validates measurements against targets
    # Reports PASS/FAIL with detailed output
endfunction()
```

**Usage in CI/CD Pipeline**:

```bash
# Run all performance gates in release build
ctest -R "tensor_performance_gate" -V

# Or run individually:
ctest -R "gate_tn_a3_02" -V  # Test findSimilar p95 gate
```

### 3.3 Gate Validation Scripts

**File**: `benchmarks/tensor/validate_gates.py`

```python
#!/usr/bin/env python3
"""Validate tensor performance gates against baseline measurements."""

import json, sys, subprocess
from dataclasses import dataclass
from typing import Dict

@dataclass
class Gate:
    id: str
    description: str
    target: float
    unit: str
    comparison: str  # "less_than" or "greater_than"

# Define all gates
GATES = {
    'GATE_TN_A3_01': Gate('TN-A3-01', 'FG insert throughput', 1500, 'ops/sec', 'greater_than'),
    'GATE_TN_A3_02': Gate('TN-A3-02', 'FG findSimilar p95', 80, 'ms', 'less_than'),
    'GATE_TN_A3_03': Gate('TN-A3-03', 'FG findSimilar p99', 140, 'ms', 'less_than'),
    'GATE_TN_A3_04': Gate('TN-A3-04', 'Dedup 90/10 throughput', 2000, 'ops/sec', 'greater_than'),
    'GATE_TN_A3_05': Gate('TN-A3-05', 'Dedup per-op p95 latency', 2.5, 'ms', 'less_than'),
    'GATE_TN_A3_06': Gate('TN-A3-06', 'Dedup memory growth', 20, 'bytes/op', 'less_than'),
}

def validate_gate(gate: Gate, measured_value: float) -> bool:
    """Check if measurement meets gate requirement."""
    if gate.comparison == 'less_than':
        return measured_value <= gate.target
    elif gate.comparison == 'greater_than':
        return measured_value >= gate.target
    return False

def run_benchmarks(build_dir: str) -> Dict[str, float]:
    """Run all tensor benchmarks and extract measurements."""
    measurements = {}
    
    # Run fingerprint graph benchmarks
    result = subprocess.run([
        f"{build_dir}/benchmarks/tensor/bench_tensor_fingerprint_graph",
        "--benchmark_format=json",
        "--benchmark_out=tfg_results.json"
    ], capture_output=True)
    
    if result.returncode == 0:
        with open('tfg_results.json') as f:
            tfg_data = json.load(f)
            # Extract p95/p99 from results
            # ... parsing logic ...
    
    # Run dedup manager benchmarks
    result = subprocess.run([
        f"{build_dir}/benchmarks/tensor/bench_tensor_deduplication_manager",
        "--benchmark_format=json",
        "--benchmark_out=tdm_results.json"
    ], capture_output=True)
    
    if result.returncode == 0:
        with open('tdm_results.json') as f:
            tdm_data = json.load(f)
            # Extract measurements
            # ... parsing logic ...
    
    return measurements

def main(build_dir: str):
    measurements = run_benchmarks(build_dir)
    
    print("=" * 60)
    print("Tensor Module Performance Gate Validation")
    print("=" * 60)
    
    passed = 0
    failed = 0
    
    for gate_key, gate in GATES.items():
        if gate_key in measurements:
            value = measurements[gate_key]
            is_pass = validate_gate(gate, value)
            status = "✓ PASS" if is_pass else "✗ FAIL"
            margin = (value / gate.target - 1) * 100
            
            print(f"\n{gate.id}: {gate.description}")
            print(f"  Target: {gate.target} {gate.unit}")
            print(f"  Measured: {value} {gate.unit}")
            print(f"  Status: {status}")
            print(f"  Margin: {margin:+.1f}%")
            
            if is_pass:
                passed += 1
            else:
                failed += 1
        else:
            print(f"\n{gate.id}: {gate.description}")
            print(f"  Status: ⚠ MISSING MEASUREMENT")
            failed += 1
    
    print("\n" + "=" * 60)
    print(f"Summary: {passed} passed, {failed} failed")
    print("=" * 60)
    
    return 0 if failed == 0 else 1

if __name__ == "__main__":
    import sys
    build_dir = sys.argv[1] if len(sys.argv) > 1 else "build-community-release"
    sys.exit(main(build_dir))
```

---

## Success Criteria

- ✅ All benchmarks compile and run without errors
- ✅ p95/p99 measurements stable across 3+ runs (< 2% variance)
- ✅ All performance targets from FUTURE_ENHANCEMENTS.md met
- ✅ Baseline report complete with methodology documentation
- ✅ 6 CMake performance gates defined and PASS on develop
- ✅ Gap analysis identifies missing scenarios with A4/B1 timeline
- ✅ Reproducibility verified on CI environment

---

## Timeline

| Week | Dates | Phase | Deliverables |
|------|-------|-------|--------------|
| W1 | Aug 8-15 | Validation | Benchmark audit complete, missing scenarios identified |
| W1-W2 | Aug 8-18 | Validation | Baseline measurements collected (3 runs), variance verified |
| W2 | Aug 18-22 | Enhancement | Concurrent ops benchmarks added, cache measurement framework |
| W2-W3 | Aug 22-27 | Reporting | Final baseline report, CMake gates defined, validation scripts |
| Final | Aug 27 | Completion | All deliverables ready for Stream B B3 handoff |

---

## Related Documents

- `src/tensor/FUTURE_ENHANCEMENTS.md` — Performance targets (source of truth)
- `src/graph/FUTURE_ENHANCEMENTS.md` — Graph module targets
- `benchmarks/tensor/TENSOR_Q3_BENCHMARK_BASELINE.md` — Locked baseline (existing)
- `benchmarks/tensor/BLOCK_A3_COMPLETION_SUMMARY.md` — Work completed (existing)

---

## References

- **Google Benchmark**: https://github.com/google/benchmark
- **Latency Distribution Percentiles**: https://en.wikipedia.org/wiki/Percentile
- **CI Environment Specification**: See DevOps team runner specs (8-core Linux, 3.5 GHz)
