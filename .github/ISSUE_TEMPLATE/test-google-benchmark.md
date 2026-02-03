---
name: 🏃 Google Benchmark Implementation
about: Implement or improve performance benchmarks using Google Benchmark framework
title: '[BENCHMARK] '
labels: ['type:testing', 'area:performance', 'tool:google-benchmark', 'needs-triage']
assignees: ''
---

## 🎯 Benchmark Objective / Benchmark-Ziel

**Component to Benchmark:** <!-- z.B. Storage Layer, Query Engine, Vector Search -->
**Component Path:** <!-- z.B. src/storage/, src/query/, src/index/ -->
**Benchmark Type:** 
- [ ] Throughput (ops/sec)
- [ ] Latency (p50, p95, p99)
- [ ] Resource usage (CPU, Memory)
- [ ] Scalability (varying data sizes)
- [ ] Comparison (before/after optimization)

---

## 📊 Benchmark Specification

### What to Measure / Was soll gemessen werden

**Primary Metrics:**
- [ ] **Throughput**: <!-- e.g., queries/sec, writes/sec, inserts/sec -->
- [ ] **Latency**: <!-- e.g., query response time, write latency -->
- [ ] **CPU Time**: <!-- e.g., user time, system time -->
- [ ] **Memory**: <!-- e.g., peak RSS, allocations -->
- [ ] **I/O**: <!-- e.g., reads/writes, IOPS -->
- [ ] **Other**: <!-- specify -->

**Benchmark Parameters:**
- **Data Size Range**: <!-- e.g., 1K to 1M records -->
- **Workload Type**: <!-- e.g., Random reads, Sequential writes, Mixed -->
- **Thread Count**: <!-- e.g., 1, 4, 8, 16 threads -->
- **Iterations**: <!-- e.g., 1000, 10000 iterations -->

### Expected Performance Baseline / Erwartete Performance-Baseline

**Target Metrics:**
- Throughput: <!-- e.g., > 50K ops/sec -->
- Latency (p99): <!-- e.g., < 10ms -->
- CPU Usage: <!-- e.g., < 80% on 4 cores -->
- Memory: <!-- e.g., < 500MB for 100K records -->

**Comparison:**
- [ ] New implementation vs current implementation
- [ ] ThemisDB vs competitor (specify: _______)
- [ ] Before optimization vs after optimization
- [ ] Single-threaded vs multi-threaded

---

## 🔬 Implementation Details

### Benchmark Function / Benchmark-Funktion

**File Location:** `benchmarks/benchmark_<component>.cpp`

**Function Signature:**
```cpp
// Example structure
static void BM_ComponentOperation(benchmark::State& state) {
    // Setup
    auto component = SetupComponent(state.range(0));
    
    // Benchmark loop
    for (auto _ : state) {
        // Operation to benchmark
        benchmark::DoNotOptimize(component.Operation());
    }
    
    // Report metrics
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
```

### Benchmark Registration / Benchmark-Registrierung

```cpp
// Register benchmark with parameters
BENCHMARK(BM_ComponentOperation)
    ->Range(1<<10, 1<<20)        // Data size: 1K to 1M
    ->ThreadRange(1, 8)           // Threads: 1 to 8
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10000);

// Multiple benchmarks for different scenarios
BENCHMARK(BM_ComponentOperation_Sequential);
BENCHMARK(BM_ComponentOperation_Random);
BENCHMARK(BM_ComponentOperation_Mixed);
```

### Setup and Teardown / Setup und Teardown

```cpp
// Fixture for complex setup
class ComponentFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Initialize component, load data, etc.
    }
    
    void TearDown(const ::benchmark::State& state) override {
        // Cleanup
    }
};

BENCHMARK_F(ComponentFixture, BM_Operation)(benchmark::State& state) {
    for (auto _ : state) {
        // Benchmark code
    }
}
```

---

## 🛠️ Build Integration / Build-Integration

### CMakeLists.txt Entry

```cmake
# Add Google Benchmark executable
add_executable(benchmark_<component>
    benchmarks/benchmark_<component>.cpp
)

target_link_libraries(benchmark_<component>
    PRIVATE
        themisdb_<component>
        benchmark::benchmark
        benchmark::benchmark_main
)

# Register with CTest
add_test(
    NAME benchmark_<component>
    COMMAND benchmark_<component> --benchmark_min_time=1.0
)
```

### Dependencies / Abhängigkeiten

- [ ] Google Benchmark library installed
- [ ] Component library available
- [ ] Test data generators (if needed)
- [ ] Compiler optimization flags: `-O3 -DNDEBUG`

---

## 📈 Expected Output / Erwartete Ausgabe

### Console Output Format

```
-----------------------------------------------------------------
Benchmark                        Time        CPU   Iterations
-----------------------------------------------------------------
BM_ComponentOperation/1024      345 ns    344 ns     2034567
BM_ComponentOperation/4096      678 ns    677 ns     1034234
BM_ComponentOperation/16384    1234 ns   1233 ns      567890
BM_ComponentOperation_Threads/1  100 ns     99 ns   7000000
BM_ComponentOperation_Threads/4   28 ns     28 ns  25000000
```

### JSON Output (for CI/CD)

```bash
# Generate JSON output for tracking
./benchmark_<component> --benchmark_format=json > benchmark_results.json
```

### Regression Detection / Regressions-Erkennung

```bash
# Compare with baseline
google-benchmark compare baseline.json current.json
```

---

## ✅ Acceptance Criteria / Akzeptanzkriterien

- [ ] Benchmark compiles without warnings
- [ ] Benchmark runs successfully in CI/CD
- [ ] Baseline metrics documented
- [ ] Performance targets met
- [ ] Multiple scenarios covered (best/average/worst case)
- [ ] Thread scalability tested
- [ ] Memory usage profiled (if applicable)
- [ ] Results reproducible (low variance < 5%)
- [ ] Documentation updated in `benchmarks/README.md`
- [ ] Integrated into `benchmarks/complete_benchmark_suite.py`

---

## 📊 Results Tracking / Ergebnis-Tracking

### Initial Baseline

| Metric | Value | Date | Commit |
|--------|-------|------|--------|
| Throughput | | | |
| Latency p50 | | | |
| Latency p99 | | | |
| Memory (RSS) | | | |

### Performance Evolution

| Date | Version | Throughput | Latency p99 | Notes |
|------|---------|------------|-------------|-------|
| | | | | Initial implementation |
| | | | | After optimization X |
| | | | | After optimization Y |

---

## 🔗 References / Referenzen

### Google Benchmark Documentation
- [User Guide](https://github.com/google/benchmark/blob/main/docs/user_guide.md)
- [Assembly Tests](https://github.com/google/benchmark#disabling-cpu-frequency-scaling)
- [Perf Counters](https://github.com/google/benchmark/blob/main/docs/perf_counters.md)

### Internal Documentation
- [Benchmark Best Practices](../../BENCHMARK_BEST_PRACTICES.md)
- [Performance Testing Guide](../../docs/testing/performance.md)
- [CHIMERA Suite Integration](../../benchmarks/chimera/README.md)

### Related Benchmarks
- <!-- Link to related benchmark issues/files -->

---

## 🎓 Benchmark Best Practices / Best Practices

- [ ] **Minimize Setup Cost**: Move setup outside benchmark loop
- [ ] **Use `DoNotOptimize()`**: Prevent compiler from optimizing away code
- [ ] **Disable CPU Frequency Scaling**: For stable results
- [ ] **Run with Sufficient Iterations**: Let benchmark framework auto-adjust
- [ ] **Measure What Matters**: Focus on user-visible metrics
- [ ] **Test Multiple Data Sizes**: Understand scaling behavior
- [ ] **Profile First**: Identify bottlenecks before optimizing
- [ ] **Compare Apples to Apples**: Same compiler flags, same hardware

---

**Created:** <!-- YYYY-MM-DD -->
**Owner:** <!-- Team/Person -->
**Priority:** <!-- P0/P1/P2/P3 -->
**Target Version:** <!-- v1.x.x -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-03  
**Maintained by:** ThemisDB Performance Team
