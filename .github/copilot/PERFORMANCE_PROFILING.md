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
