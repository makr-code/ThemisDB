# ThemisDB - Performance Profiling & Benchmarking Guide

## Profiling Tools Overview

| Tool | Platform | Use Case |
| ---- | -------- | -------- |
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
| ------ | ---------- | ------- | ---- |
| SM Utilization | > 80% | < 50% | `ncu` |
| Memory Bandwidth | > 70% peak | < 40% | `ncu` |
| Warp Efficiency | > 85% | < 60% | `ncu` |
| L1 Cache Hit Rate | > 80% | < 50% | `ncu` |
| L2 Cache Hit Rate | > 60% | < 30% | `ncu` |
| Occupancy | > 50% | < 25% | `ncu` |
| Registers per Thread | < 64 | > 128 | `ncu` |

### CPU Metrics

| Metric | Good Value | Tool |
| ------ | ---------- | ---- |
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
python tools/compare_benchmarks.py results_before.json results_after.json
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
| ------ | ------ | ------ | ----- |
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
# ThemisDB - Performance Profiling Guide

## Overview

This guide covers profiling workflows for ThemisDB using GPU and CPU profiling tools,
benchmark standards, and CI-based regression detection.

## GPU Profiling with Nvidia Tools

### Nsight Compute — Kernel-Level Profiling

Nsight Compute (`ncu`) provides detailed per-kernel metrics including memory
throughput, warp stalls, and instruction throughput.

```bash
# Full metric set — captures all hardware counters (slow, use on a small run)
ncu --set full -o profile.ncu-rep ./build/tests/acceleration_tests

# Targeted: memory and compute utilization only
ncu --metrics \
    l1tex__t_bytes_pipe_lsu_mem_global_op_ld.sum.per_second,\
    sm__throughput.avg.pct_of_peak_sustained_elapsed \
    -o memory_profile.ncu-rep ./build/tests/acceleration_tests

# Interactive analysis
ncu -i profile.ncu-rep

# Export to CSV for scripted analysis
ncu -i profile.ncu-rep --csv > kernel_metrics.csv
```

Key metrics to check:

| Metric                              | Target            | Action if low              |
|-------------------------------------|-------------------|----------------------------|
| `sm__throughput`                    | > 60% peak        | Increase parallelism       |
| `l1tex__throughput`                 | > 50% peak        | Improve memory locality    |
| `lts__t_sectors_srcunit_l1tex_...`  | Low L2 misses     | Use `__ldg`, shared memory |
| `warp_execution_efficiency`         | > 80%             | Reduce warp divergence     |
| `achieved_occupancy`                | > 50%             | Tune block size            |

### Nsight Systems — System-Wide Tracing

Nsight Systems (`nsys`) captures a timeline of CPU threads, CUDA kernels,
API calls, and memory transfers.

```bash
# Capture a full system trace
nsys profile -o trace.nsys-rep ./build/tests/integration_tests

# With CUDA API and kernel tracing, include NVTX markers
nsys profile \
    --trace=cuda,osrt,nvtx \
    --delay=2 \
    -o trace.nsys-rep \
    ./build/tests/acceleration_tests

# Generate a summary report (text)
nsys stats trace.nsys-rep

# Open in GUI
nsys-ui trace.nsys-rep
```

Add NVTX markers in ThemisDB code to annotate regions in the timeline:

```cpp
#include <nvtx3/nvToolsExt.h>

void VectorIndex::search(const float* query, int k, SearchResult* results) {
    nvtxRangePush("VectorIndex::search");

    nvtxRangePush("H2D transfer");
    cudaMemcpyAsync(d_query_, query, dim_ * sizeof(float),
                    cudaMemcpyHostToDevice, stream_);
    nvtxRangePop();

    nvtxRangePush("search kernel");
    launchSearchKernel(d_query_, k, d_results_);
    nvtxRangePop();

    nvtxRangePush("D2H transfer");
    cudaMemcpyAsync(results, d_results_, k * sizeof(SearchResult),
                    cudaMemcpyDeviceToHost, stream_);
    nvtxRangePop();

    cudaStreamSynchronize(stream_);
    nvtxRangePop();
}
```

### CPU-GPU Timeline Analysis

Identify bottlenecks in the CPU-GPU pipeline:

1. **Transfer-bound**: PCIe bandwidth saturated → use pinned memory, compression
2. **Launch-overhead-bound**: Many small kernels → batch launches, persistent kernels
3. **Kernel-compute-bound**: SM utilization > 80% → optimize arithmetic intensity
4. **Kernel-memory-bound**: High L2/DRAM traffic → improve data reuse

```bash
# Check if workload is transfer-bound vs compute-bound
nsys stats --report cuda_gpu_trace trace.nsys-rep | grep "Duration"
```

## CPU Profiling

### perf (Linux)

`perf` provides CPU counter-based profiling with call graph support:

```bash
# Record CPU cycles with call graph (dwarf for C++ with inlining)
perf record -g --call-graph dwarf -F 1000 ./build/tests/aql_tests

# Generate annotated report
perf report --stdio | head -80

# Flame graph (requires FlameGraph tools)
perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg

# Cache miss analysis
perf stat -e cache-references,cache-misses,instructions,cycles \
    ./build/tests/storage_tests
```

### VTune (Cross-Platform)

Intel VTune Profiler provides advanced CPU analysis:

```bash
# Hotspot analysis
vtune -collect hotspots -result-dir vtune_hotspots \
    -- ./build/tests/aql_tests

# Memory access analysis
vtune -collect memory-access -result-dir vtune_memory \
    -- ./build/tests/storage_tests

# Concurrency analysis (thread contention)
vtune -collect threading -result-dir vtune_threading \
    -- ./build/tests/integration_tests

# Command-line report
vtune -report hotspots -result-dir vtune_hotspots
```

### Callgrind with Cachegrind

Valgrind's Callgrind tool provides instruction-level profiling; Cachegrind
simulates cache behavior:

```bash
# Callgrind: instruction and call counts
valgrind --tool=callgrind --callgrind-out-file=callgrind.out \
    ./build/tests/aql_tests

# Annotate source with counts
callgrind_annotate callgrind.out | head -100

# Cachegrind: L1/L2 cache simulation
valgrind --tool=cachegrind --cachegrind-out-file=cachegrind.out \
    ./build/tests/storage_tests

cachegrind_annotate cachegrind.out
```

## Memory Profiling

### Memory Access Patterns

Check for memory access anti-patterns using Nsight Compute:

```bash
# Memory workload analysis
ncu --section MemoryWorkloadAnalysis \
    -o memory_analysis.ncu-rep \
    ./build/tests/acceleration_tests

# Identify uncoalesced global loads/stores
ncu --metrics \
    l1tex__t_bytes_pipe_lsu_mem_global_op_ld_uncached.sum,\
    l1tex__t_bytes_pipe_lsu_mem_global_op_st.sum \
    ./build/tests/acceleration_tests
```

### Cache Hit/Miss Rates

```bash
# GPU L1/L2 hit rates
ncu --metrics \
    l1tex__t_sector_hit_rate.pct,\
    lts__t_sector_hit_rate.pct \
    -o cache_rates.ncu-rep \
    ./build/tests/acceleration_tests

# CPU cache analysis with perf
perf stat -e \
    L1-dcache-loads,L1-dcache-load-misses,\
    LLC-loads,LLC-load-misses \
    ./build/tests/storage_tests
```

Target cache rates for ThemisDB workloads:

| Cache Level | Target Hit Rate | Impact if Below Target         |
|-------------|-----------------|-------------------------------|
| GPU L1      | > 60%           | Add shared memory tiling       |
| GPU L2      | > 80%           | Reduce working set or batch    |
| CPU L1      | > 85%           | Improve spatial locality       |
| CPU LLC     | > 75%           | Reduce data set or use NUMA    |

### Bandwidth Utilization

```bash
# Achieved vs peak memory bandwidth
ncu --metrics \
    dram__bytes_read.sum.per_second,\
    dram__bytes_write.sum.per_second \
    ./build/tests/acceleration_tests

# Compare to theoretical peak (A100: ~2.0 TB/s HBM2e, RTX 4090: ~1.0 TB/s GDDR6X; verify per GPU spec)
```

## Benchmark Standards

### Google Benchmark Setup

ThemisDB uses Google Benchmark for micro-benchmarks in `benchmarks/`:

```cpp
#include <benchmark/benchmark.h>

static void BM_VectorSearch(benchmark::State& state) {
    const int numVectors = state.range(0);
    const int dimension  = 128;

    VectorIndex index(dimension);
    prepareIndex(index, numVectors);
    auto query = generateRandomVector(dimension);

    for (auto _ : state) {
        auto results = index.search(query, 10);
        benchmark::DoNotOptimize(results);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(state.iterations() * numVectors);
    state.SetBytesProcessed(state.iterations() * numVectors *
                            dimension * sizeof(float));
}

// Parameterize over dataset sizes
BENCHMARK(BM_VectorSearch)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Unit(benchmark::kMicrosecond)
    ->MinTime(2.0);

BENCHMARK_MAIN();
```

Run benchmarks:

```bash
# Build benchmarks
cmake --build build --target themis_benchmarks

# Run with JSON output for regression tracking
./build/benchmarks/themis_benchmarks \
    --benchmark_format=json \
    --benchmark_out=baseline.json

# Compare two runs
./build/benchmarks/themis_benchmarks \
    --benchmark_format=json \
    --benchmark_out=new.json

# Use benchmark compare tool
python tools/compare.py benchmarks baseline.json new.json
```

### Performance Regression Detection

Establish baselines per branch and alert on regressions:

```bash
# Store baseline on main
./build/benchmarks/themis_benchmarks \
    --benchmark_out=benchmarks/baseline_$(git rev-parse --short HEAD).json \
    --benchmark_out_format=json

# Detect regression: > 5% slowdown is a failure
python tools/compare.py benchmarks \
    benchmarks/baseline_<ref>.json \
    new.json \
    --threshold 5
```

Regression detection thresholds for ThemisDB:

| Component          | Regression Threshold | Justification                  |
|--------------------|---------------------|-------------------------------|
| Vector search      | 5%                  | Tight latency SLA              |
| AQL query parsing  | 10%                 | Less latency-sensitive         |
| Storage write      | 10%                 | Batch-oriented workload        |
| GPU kernel         | 5%                  | GPU performance is deterministic |

### Baseline Tracking

Store baselines alongside code:

```
benchmarks/
├── baselines/
│   ├── vector_search_baseline.json
│   ├── storage_baseline.json
│   └── aql_baseline.json
└── results/          # .gitignore'd — generated locally
```

## CI Integration

### Automated Benchmark Runs

```yaml
# .github/workflows/benchmark-regression.yml (excerpt)
- name: Run benchmarks
  run: |
    ./build/benchmarks/themis_benchmarks \
      --benchmark_format=json \
      --benchmark_out=results/new.json

- name: Check for regressions
  run: |
    python tools/compare.py benchmarks \
      benchmarks/baselines/vector_search_baseline.json \
      results/new.json \
      --threshold 5
```

### Regression Alerts

The CI pipeline fails the build if any benchmark regresses by more than the
threshold defined above. To suppress a known regression:

```bash
# Mark a known regression as accepted (requires code review)
# Add to benchmarks/baselines/<component>_exceptions.json:
{
  "BM_VectorSearch/10000": {
    "accepted_regression_pct": 8,
    "reason": "Accuracy improvement trade-off — see PR #1234"
  }
}
```

## Profiling Workflow Summary

```
1. Identify bottleneck (CPU vs GPU vs memory vs transfer)
   → nsys profile (system view)

2. Drill into GPU kernels
   → ncu --set full (kernel metrics)

3. Drill into CPU hotspots
   → perf record -g / vtune -collect hotspots

4. Measure memory behavior
   → ncu MemoryWorkloadAnalysis / cachegrind

5. Quantify with benchmarks
   → Google Benchmark + regression comparison

6. Fix → re-profile → verify improvement
```

## Additional Resources

- Nsight Compute documentation: https://docs.nvidia.com/nsight-compute/
- Nsight Systems documentation: https://docs.nvidia.com/nsight-systems/
- Google Benchmark: https://github.com/google/benchmark
- CUDA Optimization guide: [CUDA_OPTIMIZATION.md](CUDA_OPTIMIZATION.md)
- Testing Guide: [TESTING_GUIDE.md](TESTING_GUIDE.md)
