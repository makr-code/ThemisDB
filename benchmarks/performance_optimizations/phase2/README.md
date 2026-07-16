> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Phase 2 Performance Benchmarks

Comprehensive benchmarks for Phase 2 optimizations based on scientific research.

## Overview

This directory contains Google Benchmark performance tests for all Phase 2 optimizations:

1. **WiscKey** - Key/value separation (FAST'16)
2. **Dostoevsky** - Adaptive LSM trees (SIGMOD'18)
3. **Cicada** - Optimistic concurrency control (SIGMOD'17)
4. **Ligra** - Parallel graph processing (PPoPP'13)
5. **RaBitQ** - 2-bit vector quantization (SIGMOD'24)

## Building

```bash
# Configure with benchmarks enabled
cmake -B build -S . \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DTHEMIS_ENABLE_WISCKEY=ON \
  -DTHEMIS_ENABLE_DOSTOEVSKY=ON \
  -DTHEMIS_ENABLE_CICADA=ON \
  -DTHEMIS_ENABLE_LIGRA=ON \
  -DTHEMIS_ENABLE_RABITQ=ON

# Build
cmake --build build --config Release
```

## Running Benchmarks

### All Phase 2 Benchmarks

```bash
./build/benchmarks/performance_optimizations/phase2/benchmark_phase2
```

### Specific Benchmark Categories

```bash
# WiscKey benchmarks only
./build/benchmarks/performance_optimizations/phase2/benchmark_phase2 --benchmark_filter=WiscKey

# Dostoevsky benchmarks only
./build/benchmarks/performance_optimizations/phase2/benchmark_phase2 --benchmark_filter=Dostoevsky

# Cicada benchmarks only
./build/benchmarks/performance_optimizations/phase2/benchmark_phase2 --benchmark_filter=Cicada

# Ligra benchmarks only
./build/benchmarks/performance_optimizations/phase2/benchmark_phase2 --benchmark_filter=Ligra

# RaBitQ benchmarks only
./build/benchmarks/performance_optimizations/phase2/benchmark_phase2 --benchmark_filter=RaBitQ
```

### Output Options

```bash
# JSON output
./build/benchmarks/performance_optimizations/phase2/benchmark_phase2 --benchmark_format=json > results.json

# CSV output
./build/benchmarks/performance_optimizations/phase2/benchmark_phase2 --benchmark_format=csv > results.csv

# Console output (default)
./build/benchmarks/performance_optimizations/phase2/benchmark_phase2 --benchmark_format=console
```

## Benchmark Categories

### WiscKey Benchmarks

- `BM_WiscKey_SmallValueWrite` - Write performance for values <1KB (inline)
- `BM_WiscKey_LargeValueWrite` - Write performance for values >1KB (separated)
- `BM_WiscKey_MixedWorkload` - Mixed read/write with varying value sizes

**Expected:** +40-60% write throughput for large values

### Dostoevsky Benchmarks

- `BM_Dostoevsky_PolicyComputation` - Merge policy selection overhead
- `BM_Dostoevsky_WorkloadMonitoring` - Workload statistics tracking overhead

**Expected:** +25-35% for mixed workloads

### Cicada Benchmarks

- `BM_Cicada_RecordLocking` - Lock acquisition/release performance
- `BM_Cicada_TransactionCommit` - Full transaction commit protocol
- `BM_Cicada_HighContention` - Performance under high contention (multi-threaded)

**Expected:** +100-150% transaction throughput

### Ligra Benchmarks

- `BM_Ligra_FrontierOperations` - Frontier add/contains operations
- `BM_Ligra_BFS` - Parallel breadth-first search (100/1000/10000 nodes)
- `BM_Ligra_PageRank` - Parallel PageRank computation

**Expected:** +200-300% graph operation throughput

### RaBitQ Benchmarks

- `BM_RaBitQ_Encoding` - Vector quantization (128/512/1024 dimensions)
- `BM_RaBitQ_Decoding` - Dequantization performance
- `BM_RaBitQ_DistanceComputation` - Asymmetric distance computation
- `BM_RaBitQ_IndexSearch` - k-NN search (1K/10K/100K vectors)
- `BM_RaBitQ_MemoryCompression` - Memory compression ratio

**Expected:** 16x memory reduction, +50-80% throughput

## Interpreting Results

### Sample Output

```
-------------------------------------------------------------------
Benchmark                            Time             CPU   Iterations
-------------------------------------------------------------------
BM_WiscKey_SmallValueWrite        45.2 ns         45.2 ns     15478963
BM_WiscKey_LargeValueWrite        3240 ns         3238 ns       216034
BM_Dostoevsky_PolicyComputation   12.4 ns         12.4 ns     56432109
BM_Cicada_RecordLocking           8.32 ns         8.32 ns     84123456
BM_Ligra_BFS/100                  2340 ns         2338 ns       299123
BM_RaBitQ_Encoding/128            234 ns           234 ns      2987654
```

### Key Metrics

- **Time/CPU:** Lower is better (nanoseconds per operation)
- **Iterations:** More iterations = more reliable measurement
- **Bytes/s:** Higher is better (for I/O benchmarks)

## Comparing with Baseline

To validate Phase 2 gains:

1. Run benchmarks **without** Phase 2 optimizations:
   ```bash
   cmake -B build-baseline -S . -DTHEMIS_BUILD_BENCHMARKS=ON
   ./build-baseline/benchmarks/...
   ```

2. Run benchmarks **with** Phase 2 optimizations:
   ```bash
   cmake -B build-phase2 -S . -DTHEMIS_BUILD_BENCHMARKS=ON \
     -DTHEMIS_ENABLE_WISCKEY=ON \
     -DTHEMIS_ENABLE_DOSTOEVSKY=ON \
     -DTHEMIS_ENABLE_CICADA=ON \
     -DTHEMIS_ENABLE_LIGRA=ON \
     -DTHEMIS_ENABLE_RABITQ=ON
   ./build-phase2/benchmarks/...
   ```

3. Compare results:
   ```bash
   # Compare two benchmark runs
   python3 compare.py build-baseline/results.json build-phase2/results.json
   ```

## Troubleshooting

### Benchmarks Take Too Long

```bash
# Reduce iterations
./benchmark_phase2 --benchmark_min_time=0.1

# Run specific benchmark
./benchmark_phase2 --benchmark_filter=WiscKey
```

### Out of Memory

```bash
# Reduce problem size for RaBitQ
./benchmark_phase2 --benchmark_filter=RaBitQ/1000  # Only 1K vectors
```

### Inconsistent Results

```bash
# More stable CPU frequency
sudo cpupower frequency-set --governor performance

# Disable CPU turbo
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo

# Run multiple times
./benchmark_phase2 --benchmark_repetitions=10
```

## References

- **Google Benchmark:** https://github.com/google/benchmark
- **Phase 2 Research:** See `docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md`
- **Implementation Guide:** See `docs/PHASE2_IMPLEMENTATION_GUIDE.md`

---

**Last Updated:** April 2026  
**Status:** ✅ Complete
