# ThemisDB - Performance Profiling & Benchmarking Guide

## Profiling Tools Overview

| Tool | Platform | Use Case |
|------|----------|----------|
| NVIDIA Nsight Systems (`nsys`) | GPU | System-wide timeline, CPU+GPU interactions |
| NVIDIA Nsight Compute (`ncu`) | GPU | Per-kernel deep-dive metrics |
| Linux `perf` + FlameGraph | CPU (Linux) | CPU hot-path analysis |
| Valgrind (`callgrind`) | CPU | Instruction-level profiling, no hardware counters |
| `gprof` | CPU | Function-level call graph (requires recompile) |
| Google Benchmark | CPU/GPU | Microbenchmark framework, repeatable results |

---

## Profiling Tools Setup

### NVIDIA Nsight Systems

```bash
# Install (Ubuntu/Debian)
apt-get install -y nsight-systems-2024.1

# Or download from:
# https://developer.nvidia.com/nsight-systems

# Verify installation
nsys --version
```

### NVIDIA Nsight Compute

```bash
# Install alongside CUDA toolkit (Ubuntu)
apt-get install -y nsight-compute

# Or via CUDA installer bundle
# Verify
ncu --version
```

### Linux Perf + FlameGraph

```bash
# Install perf
apt-get install -y linux-tools-common linux-tools-generic

# Install FlameGraph scripts
git clone https://github.com/brendangregg/FlameGraph /opt/FlameGraph

# Verify
perf --version
```

---

## GPU Profiling Workflow

### Step 1: High-Level System Profile

```bash
# Capture CPU + GPU timeline (low overhead)
nsys profile \
    --output=profile_output \
    --trace=cuda,nvtx,osrt \
    ./build/tests/acceleration_tests

# Open in Nsight Systems GUI:
nsys-ui profile_output.nsys-rep
```

### Step 2: Kernel-Level Metrics (Default Set)

```bash
# Profile with default metric set (balanced overhead vs. detail)
ncu --set default \
    --output=kernel_profile \
    ./build/tests/acceleration_tests

# Open in Nsight Compute GUI:
ncu-ui kernel_profile.ncu-rep
```

### Step 3: Full Detailed Profile (High Overhead)

```bash
# Capture all available metrics (slow—use on targeted kernels)
ncu --set full \
    --csv \
    --output=kernel_full \
    ./build/tests/acceleration_tests > profile_full.csv

# Filter to specific kernel
ncu --set full \
    --kernel-name "computeL2Distances" \
    --csv \
    ./build/tests/acceleration_tests > l2_kernel.csv
```

### Step 4: Memory Bandwidth Analysis

```bash
# Focused memory bandwidth metrics
ncu --metrics \
    l1tex__t_bytes_pipe_lsu_mem_global_op_ld.sum,\
    l1tex__t_bytes_pipe_lsu_mem_global_op_st.sum,\
    dram__bytes_read.sum,\
    dram__bytes_write.sum \
    ./build/tests/acceleration_tests
```

### Step 5: Occupancy & Warp Efficiency

```bash
# Occupancy metrics
ncu --metrics \
    sm__warps_active.avg.pct_of_peak_sustained_active,\
    smsp__warps_launched.sum,\
    smsp__inst_executed.sum \
    ./build/tests/acceleration_tests
```

---

## CPU Profiling Workflow

### Linux Perf

```bash
# Record CPU cycles (requires elevated permissions or perf_event_paranoid=1)
perf record -g -F 99 ./build/tests/storage_tests
perf report --stdio | head -50

# Generate FlameGraph
perf script | /opt/FlameGraph/stackcollapse-perf.pl | \
    /opt/FlameGraph/flamegraph.pl > flamegraph.svg
```

### Valgrind Callgrind

```bash
# Collect call graph data (10–50× slowdown—use small inputs)
valgrind --tool=callgrind \
    --callgrind-out-file=callgrind.out \
    ./build/tests/aql_tests

# Visualize with kcachegrind
kcachegrind callgrind.out
```

---

## Performance Metrics Reference

### GPU Metrics

| Metric | Good Value | Warning | Tool |
|--------|-----------|---------|------|
| SM Utilization | > 80% | < 50% | `ncu` |
| Memory Bandwidth | > 70% peak | < 40% | `ncu` |
| Warp Efficiency | > 85% | < 60% | `ncu` |
| L1 Cache Hit Rate | > 80% | < 50% | `ncu` |
| L2 Cache Hit Rate | > 60% | < 30% | `ncu` |
| Occupancy | > 50% | < 25% | `ncu` |
| Registers per Thread | < 64 | > 128 | `ncu` |

### CPU Metrics

| Metric | Good Value | Tool |
|--------|-----------|------|
| IPC (Instructions/Cycle) | > 2.0 | `perf stat` |
| L1D Cache Miss Rate | < 5% | `perf stat` |
| Branch Misprediction Rate | < 2% | `perf stat` |
| Memory BW Utilization | > 60% | `perf mem` |

---

## Benchmark Best Practices

### Warmup Iterations

```cpp
// ✅ Required: Prime GPU caches before measuring
static void BM_VectorSearch(benchmark::State& state) {
    constexpr int WARMUP_ITERS = 5;
    
    // Warmup: discard results, prime caches and JIT compilation
    for (int i = 0; i < WARMUP_ITERS; ++i) {
        auto result = index->search(query, k);
        benchmark::DoNotOptimize(result);
    }

    // Measurement loop
    for (auto _ : state) {
        auto result = index->search(query, k);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}
```

### Multiple Runs & Variance Reduction

```cpp
// ✅ Good: Sufficient iterations to reduce measurement noise
BENCHMARK(BM_VectorSearch)
    ->Iterations(100)          // Minimum iterations
    ->Repetitions(5)           // Repeat 5 times, report min/mean/max
    ->ReportAggregatesOnly();  // Show aggregated statistics
```

### Prevent Dead Code Elimination

```cpp
// ✅ Required: Prevent compiler from optimizing away benchmark code
for (auto _ : state) {
    float result = compute(data);
    benchmark::DoNotOptimize(result);  // Prevents DCE
    benchmark::ClobberMemory();        // Flushes memory state
}
```

### Baseline Comparisons

```cpp
// ✅ Good: Always define a CPU baseline for GPU benchmarks
BENCHMARK(BM_VectorSearchCPU)->Arg(1000)->Arg(10000)->Arg(100000);
BENCHMARK(BM_VectorSearchGPU)->Arg(1000)->Arg(10000)->Arg(100000);
```

---

## Benchmark Suite Integration

### CMake Target

```cmake
# In benchmarks/CMakeLists.txt
add_executable(themis_benchmarks
    vector_search_bench.cpp
    storage_bench.cpp
    aql_bench.cpp
)
target_link_libraries(themis_benchmarks
    PRIVATE
        themis_core
        benchmark::benchmark
        benchmark::benchmark_main
)

# Register as cmake target
add_custom_target(benchmark
    COMMAND themis_benchmarks --benchmark_format=json
                              --benchmark_out=benchmark_results.json
    DEPENDS themis_benchmarks
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
```

### Running the Benchmark Suite

```bash
# Build and run benchmarks
cmake --build build --target benchmark

# Run with specific filter
./build/benchmarks/themis_benchmarks \
    --benchmark_filter="BM_VectorSearch" \
    --benchmark_format=json \
    --benchmark_out=results.json

# Compare two result files
python tools/compare.py results_before.json results_after.json
```

### Google Benchmark Output Format

```bash
# Human-readable (default)
./themis_benchmarks

# JSON output for CI integration
./themis_benchmarks --benchmark_format=json --benchmark_out=results.json

# CSV for spreadsheet analysis
./themis_benchmarks --benchmark_format=csv --benchmark_out=results.csv
```

---

## NVTX Annotations for Profiling

```cpp
// ✅ Good: Add NVTX ranges to identify regions in the timeline
#ifdef THEMIS_ENABLE_PROFILING
#include <nvtx3/nvToolsExt.h>
#define THEMIS_PROFILE_RANGE(name) nvtxRangePushA(name)
#define THEMIS_PROFILE_END()       nvtxRangePop()
#else
#define THEMIS_PROFILE_RANGE(name) ((void)0)
#define THEMIS_PROFILE_END()       ((void)0)
#endif

// Usage
void VectorIndex::search(const float* query, int k) {
    THEMIS_PROFILE_RANGE("VectorIndex::search");
    
    THEMIS_PROFILE_RANGE("distance_computation");
    computeDistances(query);
    THEMIS_PROFILE_END();

    THEMIS_PROFILE_RANGE("top_k_selection");
    selectTopK(k);
    THEMIS_PROFILE_END();

    THEMIS_PROFILE_END();
}
```

---

## CI/CD Performance Validation

### Regression Detection Workflow

```yaml
# .github/workflows/performance-ci.yml (excerpt)
- name: Run benchmarks
  run: |
    cmake --build build --target benchmark
    ./build/benchmarks/themis_benchmarks \
      --benchmark_format=json \
      --benchmark_out=benchmark_results.json

- name: Compare with baseline
  run: |
    python tools/compare_benchmarks.py \
      --baseline benchmark_baseline.json \
      --current  benchmark_results.json \
      --threshold 0.10  # Fail if > 10% regression
```

### Performance Alerting Thresholds

```python
# tools/compare_benchmarks.py (excerpt)
REGRESSION_THRESHOLDS = {
    "BM_VectorSearch":    0.10,   # 10% regression allowed
    "BM_StorageWrite":    0.15,   # 15% regression allowed
    "BM_AQLParse":        0.05,   # 5%  regression allowed (CPU bound)
}
```

---

## Performance Targets by Module

| Module | Metric | Target | Notes |
|--------|--------|--------|-------|
| Vector Search (GPU) | Search latency (1K queries) | < 5 ms | HNSW, float32, dim=768 |
| Vector Search (CPU) | Search latency (1K queries) | < 50 ms | AVX2 baseline |
| Storage Write | Bulk insert (100K records) | < 2 s | RocksDB backend |
| AQL Parser | Parse throughput | > 10K queries/s | Single thread |
| MVCC Transaction | Commit latency (p99) | < 1 ms | Read-write mix |

### SLO Definitions

```cpp
// Encode SLOs as test assertions
TEST(PerformanceSLO, VectorSearchLatency) {
    auto start = std::chrono::high_resolution_clock::now();
    index->search(query, k);
    auto elapsed_ms = std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - start).count();

    // SLO: p50 < 5ms for GPU path
    EXPECT_LT(elapsed_ms, 5.0) << "Vector search SLO violated";
}
```

---

## Trade-off Analysis Template

When evaluating performance vs. accuracy trade-offs, document:

```markdown
### Decision: HNSW efConstruction vs. Build Time

| efConstruction | Recall@10 | Build Time | Memory |
|----------------|-----------|------------|--------|
| 100            | 0.92      | 45 s       | 1.2 GB |
| 200            | 0.97      | 90 s       | 1.2 GB |
| 400            | 0.99      | 180 s      | 1.2 GB |

**Chosen:** efConstruction=200 (balanced recall/build time for production)
**Trade-off:** 5% accuracy improvement for 2× build time is acceptable
```

---

## Additional Resources

- [CUDA_OPTIMIZATION.md](CUDA_OPTIMIZATION.md) - GPU kernel optimization techniques
- [TESTING_GUIDE.md](TESTING_GUIDE.md) - Test framework and benchmark integration
- NVIDIA Nsight Systems docs: https://docs.nvidia.com/nsight-systems/
- NVIDIA Nsight Compute docs: https://docs.nvidia.com/nsight-compute/
- Google Benchmark docs: https://github.com/google/benchmark
- Brendan Gregg's FlameGraph: https://github.com/brendangregg/FlameGraph
