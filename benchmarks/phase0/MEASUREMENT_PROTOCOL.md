# Phase-0 Baseline Measurement Protocol

<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 ThemisDB Contributors -->

## Overview

This document defines the measurement protocol for Phase-0 CRUD baseline benchmarks. The goal is to establish reproducible, deterministic performance baselines for core ThemisDB operations (Insert, Read, Update, Delete).

## Prerequisites

1. **Build Environment:**
   - CMake 3.20+
   - Ninja build system
   - C++17 or later compiler (GCC 9+, Clang 10+, MSVC 2019+)
   - Google Benchmark library (`-DTHEMIS_BUILD_BENCHMARKS=ON`)

2. **System Dependencies:**
   - RocksDB (via vcpkg or `librocksdb-dev`)
   - spdlog for logging
   - nlohmann_json for JSON processing

3. **Machine Configuration:**
   - Isolation: run benchmarks on a quiet machine with no background load
   - CPU frequency: set to performance mode (no dynamic scaling if possible)
   - Thermal: ensure system is thermally stable (no throttling during runs)
   - Memory: use consistent NUMA settings (e.g., numactl --preferred=0)

## Build and Execution

### Build

```bash
# Configure with benchmark support enabled
cmake --preset linux-release \
  -DTHEMIS_BUILD_BENCHMARKS=ON \
  -DCMAKE_BUILD_TYPE=Release

# Build Phase-0 benchmark target
cmake --build --preset linux-release --target bench_p0_crud_baseline
```

### Run Baseline

```bash
# Run all Phase-0 CRUD benchmarks
./build/Release/bin/benchmarks/bench_p0_crud_baseline

# Run single workload (example: insert_heavy)
./build/Release/bin/benchmarks/bench_p0_crud_baseline --benchmark_filter="BM_P0_InsertHeavy"

# Capture results to JSON
./build/Release/bin/benchmarks/bench_p0_crud_baseline \
  --benchmark_out=results_p0_baseline_run1.json \
  --benchmark_out_format=json
```

## Measurement Protocol

### 3-Phase Warmup

Every Phase-0 benchmark uses a canonical 3-phase warmup before the measurement window:

1. **Cold Phase (50 ops):**
   - Fill write buffer, prime OS I/O path
   - Ensures clean state for measurement

2. **Warm Phase (100 ops):**
   - Sequential reads to populate OS page cache
   - Stabilizes memory access patterns

3. **Hot Phase (200 ops):**
   - Random reads to stabilize CPU branch predictor and instruction cache
   - Prevents outlier penalties on first measured iteration

### Canonical Seeding

All benchmarks use **seed = 42** for reproducible data sequences:

```cpp
static constexpr uint64_t kP0CanonicalSeed = 42;
```

This ensures:
- Identical data across machines
- No pollution from random distributions
- Baseline comparisons are deterministic

### Workloads

Four core workloads, each with 80/20 split:

| Workload       | Operations        | Purpose                      |
|----------------|-------------------|------------------------------|
| INSERT_HEAVY   | 80% inserts, 20% reads | Write throughput focus |
| READ_HEAVY     | 80% reads, 20% writes  | Read throughput focus  |
| UPDATE_HEAVY   | 80% updates, 20% reads | Update throughput focus |
| DELETE_HEAVY   | 80% deletes, 20% inserts | Delete throughput focus |

## Baseline Expectations

See `baseline_p0_v0.json` for numeric targets. Key categories:

- **Minimum**: absolute floor for regression detection
- **Target**: expected performance range
- **Max P99 Latency**: maximum acceptable tail latency
- **Expected P50 Latency**: median expected latency

## Regression Detection

A benchmark is flagged as regressed if:

1. **Throughput drops > 10%** compared to baseline, **OR**
2. **P99 latency increases > 10%**, **OR**
3. **Coefficient of variation (CV) > 15%** (flakiness)

## Reporting Results

After running benchmarks, generate a report:

```bash
# Compare run against baseline
python3 /benchmarks/phase0/compare_baseline.py \
  --baseline baselines/baseline_p0_v0.json \
  --results results_p0_baseline_run1.json \
  --output report_p0_run1.md
```

## Known Limitations

1. **In-Memory KV Store:** Current implementation uses a memory-based stub. Production will link against `themis_core`.
2. **No Network Overhead:** Benchmarks measure local operations only.
3. **No Multi-Node:** Sharding and distributed scenarios not included in Phase-0.
4. **CPU-Specific Tuning:** SIMD vectorization and cache affinity optimizations pending.

## Next Steps (Phase 1+)

- [ ] Integrate with real themis_core KV store backend
- [ ] Add network latency injection scenarios
- [ ] Measure multi-node and sharding performance
- [ ] Collect hardware-specific baselines (CPU types, RAM configs, storage)
- [ ] Implement continuous baseline tracking in CI
