# Phase-0 CRUD Baseline Architecture

<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 ThemisDB Contributors -->

## Overview

Phase-0 is the initial performance baseline stage for ThemisDB. It establishes deterministic, reproducible baselines for core CRUD operations (Create/Insert, Read, Update, Delete) to enable release-readiness planning and regression detection.

## Purpose

1. **Release-Readiness Gate:** Establish minimum acceptable performance for core operations
2. **Regression Detection:** Compare new builds against baseline to detect regressions early
3. **Hardware Profiling:** Capture performance across different hardware platforms (x86, ARM, GPU)
4. **Foundation for Future Waves:** Phase-0 baseline enables Phase-1+ (network, multi-node, distributed scenarios)

## Architecture

### Directory Structure

```
benchmarks/phase0/
├── CMakeLists.txt                      # Benchmark registration (Phase-0 targets)
├── phase0_fixtures.h                   # Canonical constants, 3-phase warmup helpers
├── bench_p0_crud_baseline.cpp          # Main benchmark: 4 CRUD workloads
├── MEASUREMENT_PROTOCOL.md             # Detailed measurement protocol (3-phase warmup, seeding)
├── RUNBOOK.md                          # Operator manual (baseline capture, regression detection, gates)
├── ARCHITECTURE.md                     # This file
└── baselines/
    └── baseline_p0_v0.json             # Baseline expectations + metrics
```

### Core Components

#### 1. **phase0_fixtures.h** — Canonical Constants

Provides reproducible measurement infrastructure:

```cpp
// Seeding
static constexpr uint64_t kP0CanonicalSeed = 42;

// 3-Phase Warmup
static constexpr int kP0WarmupCold = 50;   // Write buffer priming
static constexpr int kP0WarmupWarm = 100;  // Page cache warmup
static constexpr int kP0WarmupHot = 200;   // Branch predictor stabilization

// Workload Profiles
static constexpr double kP0InsertHeavyInsertRatio = 0.80;
// ... (read_heavy, update_heavy, delete_heavy)
```

**Design Rationale:**
- Constants ensure reproducibility across CI runs and hardware
- 3-phase warmup eliminates cold-start outliers
- Temp directories avoid collision in parallel runs
- Canonical seed (42) guarantees identical data sequences

#### 2. **bench_p0_crud_baseline.cpp** — Benchmark Implementations

Four core workloads measuring baseline performance:

| Workload       | Mix              | Purpose                |
|----------------|------------------|------------------------|
| INSERT_HEAVY   | 80% ins / 20% rd | Write throughput       |
| READ_HEAVY     | 80% rd / 20% wr  | Read throughput        |
| UPDATE_HEAVY   | 80% upd / 20% rd | Update throughput      |
| DELETE_HEAVY   | 80% del / 20% in | Delete throughput      |

**Implementation Notes:**
- Uses temporary in-memory KV store (stub for Phase-0)
- Future phases will integrate real themis_core backend
- Google Benchmark framework for reproducible timing
- Latency reported in microseconds (p50, p99)
- Throughput in operations/second

#### 3. **Measurement Protocol** — 3-Phase Warmup

Every benchmark follows a canonical 3-phase warmup before measurement:

```
Phase 1 (Cold)  Phase 2 (Warm)  Phase 3 (Hot)  Measurement Window
50 writes ----> 100 seq reads -> 200 rnd reads -> [MEASURE]
                                                   ↑ First measurement here
```

**Why 3 phases?**
- **Cold writes:** Fill write buffer, prime compaction
- **Sequential reads:** Populate OS page cache for working set
- **Random reads:** Stabilize CPU branch predictor and instruction cache

This ensures first measured iteration is not outlier-penalized.

#### 4. **baseline_p0_v0.json** — Baseline Expectations

JSON manifest with target metrics:

```json
{
  "baseline_metrics": {
    "insert_heavy": {
      "min_ops_per_sec": 100000,
      "target_ops_per_sec": 150000,
      "max_latency_p99_us": 1000
    },
    ...
  },
  "gate_rules": {
    "regression_threshold_percent": 10,
    "variance_cv_max": 0.15,
    "sample_count_min": 30
  }
}
```

**Decision Logic:**
- Regression detected if throughput drops >10% or P99 latency increases >10%
- Flakiness detected if CV (coefficient of variation) >15%
- Go/No-Go decision: all 4 workloads pass or fail

## Data Flow

### Baseline Capture (First Run)

```
1. Configure: cmake --preset linux-release -DTHEMIS_BUILD_BENCHMARKS=ON
2. Build:     cmake --build --preset linux-release --target bench_p0_crud_baseline
3. Run:       ./bench_p0_crud_baseline --benchmark_out=results_v1.json
4. Archive:   cp results_v1.json baselines/hardware_profiles/$(date)/
```

### Regression Detection (Subsequent Runs)

```
1. Run new benchmark
2. Extract metrics (ops/sec, latency p50/p99)
3. Compare against baseline_p0_v0.json
4. If any metric regresses >10%: NO-GO
5. If all pass: GO
```

## Measurement Hygiene

### Canonical Seeding

All RNG-based operations use seed=42 to ensure:
- Identical data across machines and CI runs
- No pollution from random distributions
- Deterministic baseline comparisons

### Temporary Directories

```cpp
auto tmp_path = std::filesystem::temp_directory_path() / 
                (std::string(prefix) + "_" + timestamp_suffix);
```

Avoids:
- Collisions in parallel benchmark runs
- Artefacts in working directory
- Permission issues from relative paths

### Warmup Protocol

3-phase warmup ensures:
- Deterministic cache state before measurement
- Elimination of cold-start outliers
- Reproducible latency measurements

## Integration Points

### CMake

Phase-0 benchmarks registered in main `benchmarks/CMakeLists.txt`:

```cmake
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/phase0/CMakeLists.txt")
    add_subdirectory(phase0)
endif()
```

Uses `themis_add_standard_benchmark()` macro for standard link libraries and flags.

### CI (Future)

Phase-0 gates will be integrated into GitHub Actions:

```yaml
# .github/workflows/benchmarks_p0.yml (future)
- name: Phase-0 Baseline
  run: |
    ./build/Release/bin/benchmarks/bench_p0_crud_baseline \
      --benchmark_out=p0_result.json
    python3 compare_baseline.py --baseline baselines/baseline_p0_v0.json \
                                 --results p0_result.json \
                                 --threshold 10
```

## Known Limitations

1. **In-Memory KV Store:** Current implementation uses memory-based stub
   - Production Phase-1: Link against real themis_core backend

2. **Single-Machine:** No distributed/sharding scenarios
   - Future: Add multi-node Phase-1+ benchmarks

3. **No Network:** Local operations only
   - Future: Network latency injection in Phase-1+

4. **Hardware-Dependent:** Baselines vary by CPU, RAM, storage
   - Mitigation: Capture hardware profiles for each platform

## Future Phases

### Phase-0 (Current)
- ✓ Single-machine CRUD baselines
- ✓ Canonical seeding and warmup protocol
- ✓ Baseline expectations and decision gates

### Phase-1 (Q3/Q4 2026)
- Network latency injection
- Multi-threaded scenarios
- Sharding and multi-node basics

### Phase-2+ (2026+)
- Distributed consensus scenarios
- Failure injection
- Chaos engineering
- Cost models and hardware profiling

## References

- **Measurement Protocol:** `phase0/MEASUREMENT_PROTOCOL.md`
- **Operator Runbook:** `phase0/RUNBOOK.md`
- **Baseline Manifest:** `phase0/baselines/baseline_p0_v0.json`
- **Wave 4 Benchmark Governance:** `wave4/RUNBOOK.md`
- **Measurement Hygiene:** `benchmarks/MEASUREMENT_HYGIENE.md`
