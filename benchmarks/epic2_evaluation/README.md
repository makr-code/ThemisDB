# EPIC 2 Evaluation Benchmarks

<!-- Status: Phase 5 guardrails + benchmark documentation | validated: 2026-08-18 -->

## Overview

This directory contains benchmark sources for EPIC 2 performance measurement and regression detection. Benchmarks are designed to run in CI without GPU infrastructure and to establish measurable guardrails for release promotion.

## Implemented Benchmark Files

- `planner_decision_bench.cc` — Planner decision latency and fallback rate measurement
- `benchmark_matrix_bench.cc` — Benchmark matrix throughput (scenario × dimension operations)
- `artifact_staleness_bench.cc` — Artifact lifecycle staleness detection overhead
- `storage_strategy_bench.cc` — Distributed placement strategy computation efficiency

**Planned (Phase 5):**
- `planner_error_path_bench.cc` — Error handling path overhead measurement

## Performance Guardrails

See `src/evaluation/PERFORMANCE_EXPECTATIONS.md` for complete guardrail definitions, including:
1. Planner Decision Latency (p50/p95/p99 targets)
2. Planner Fallback Rate (Category A/B/C acceptance rates)
3. Benchmark Matrix Throughput (ops/sec targets)
4. Artifact Staleness Detection Overhead (latency budget)
5. Storage Strategy Efficiency (placement computation time)
6. Planner Error Path Overhead (error handling latency)

## How to Build

### Prerequisites
- CMake 3.20+
- C++17 compiler (GCC 11+ or Clang 14+)
- Google Benchmark library (auto-detected if available)
- RocksDB development headers (optional; for full integration testing)

### Build Commands

**Release build with all available benchmarks:**
```bash
cmake --preset linux-release -DTHEMIS_BUILD_BENCHMARKS=ON -DTHEMIS_BUILD_EPIC2=ON
cmake --build --preset linux-release
```

**Build individual benchmark targets:**
```bash
# Planner decision benchmark (requires EPIC 2 eval lib)
cmake --build --preset linux-release --target planner_decision_bench

# Benchmark matrix scenarios (requires Google Benchmark)
cmake --build --preset linux-release --target benchmark_matrix_bench

# Storage strategy placement (requires distributed_tensor)
cmake --build --preset linux-release --target bench_epic2_evaluation_storage_strategy_bench

# Artifact staleness detection (requires artifact_lifecycle)
cmake --build --preset linux-release --target bench_epic2_evaluation_artifact_staleness_bench
```

## How to Run

### Individual Benchmark Execution

**Planner decision latency (manual harness — no Google Benchmark dependency):**
```bash
./benchmarks/epic2_evaluation/planner_decision_bench

# Example output:
# [Path 1 — ANN Only]             avg 75.3 ns / call
# [Path 2 — ANN + Tensor Summary] avg 125.4 ns / call
# [Path 4 — Stale Tensor]         avg 95.2 ns / call
# [Path 4 — force_exact]          avg 88.1 ns / call
# [Path 5 — Distributed]          avg 287.6 ns / call
# 
# Observer call count: 500000
# Path distribution: P1=100000 P2=100000 P3=0 P4=200000 P5=100000
# ModuleGapThreshold blocks: 0
#
# All paths within 10000 ns soft threshold — PASS
```

**Benchmark matrix throughput (Google Benchmark format):**
```bash
./build/benchmarks/epic2_evaluation/benchmark_matrix_bench \
  --benchmark_min_time=0.1 \
  --benchmark_filter="BM_Matrix_Record|BM_Matrix_Lookup|BM_Matrix_FillAll"

# Expected output format:
# Benchmark                                      Time             CPU   Iterations
# BM_Matrix_Record_FullScenario               800 ns          799 ns       875000
# BM_Matrix_Lookup_FullScenario              1200 ns         1199 ns       581000
# BM_Matrix_FillAll_Full                     4800 ns         4799 ns       145000
```

**Artifact staleness detection (Google Benchmark format):**
```bash
./build/benchmarks/epic2_evaluation/bench_epic2_evaluation_artifact_staleness_bench \
  --benchmark_min_time=0.1 \
  --benchmark_filter="BM_ComputeState"
```

**Storage strategy placement (Google Benchmark format):**
```bash
./build/benchmarks/epic2_evaluation/bench_epic2_evaluation_storage_strategy_bench \
  --benchmark_min_time=0.1
```

### Running Full Benchmark Suite in CI/CD

**Via CTest (recommended for CI integration):**
```bash
# Run only benchmark targets
ctest --preset linux-release -R "epic2_evaluation_bench"

# Run with verbose output
ctest --preset linux-release -R "epic2_evaluation_bench" -VV

# Run with specific labels
ctest --preset linux-release -L "benchmark"
```

**Via CMake custom targets:**
```bash
cmake --build --preset linux-release --target run_benchmark_matrix
```

## Measurement Methodology

### Baseline Capture

Benchmarks establish **release baselines** on representative hardware before GA promotion:

**Baseline Hardware (Target):**
- CPU: 2+ GHz x86_64 (Intel/AMD or ARM with equivalent single-threaded performance)
- RAM: 16GB+ available
- OS: Linux (Ubuntu 20.04 LTS or equivalent)
- Thermal: No active CPU throttling during measurement

**Baseline Capture Process:**
1. Clean build: `cmake --preset linux-release -DTHEMIS_BUILD_BENCHMARKS=ON`
2. Build all benchmarks
3. Warm-up runs (discard first 2-3 iterations per benchmark)
4. Capture 3+ independent runs on idle system
5. Record median and p95/p99 from captured runs
6. Store baseline in `src/evaluation/BASELINES.md` (Phase 6 deliverable)

### Regression Detection

**Automated regression gates:**
- If measured p95 > baseline p95 × 1.2 (>20% regression), flag as regression alert
- If measured p99 > baseline p99 × 1.3 (>30% regression), block promotion to release
- If any path changes by > 50%, require engineering review and root-cause analysis

**Manual regression assessment:**
- Compare results against section 9.2 of `src/evaluation/MODULE_EVIDENCE.md`
- Check for correlation with code changes in `src/evaluation/`, `src/query_planner/`, or `src/distributed_tensor/`
- Document root cause in commit message if regression is expected and justified

### Reproducibility Requirements

**Assumptions that must hold for valid measurement:**

1. **Isolation:** Run benchmarks on idle system with no competing workloads
2. **Thermal stability:** CPU frequency scaling disabled or pinned to baseline frequency
3. **Data stability:** Same dataset/configuration as baseline run
4. **Shard topology (for storage strategy):** Use standard 4/12/24-node cluster sizes
5. **Policy configuration (for planner):** Use default policy from `EPIC2_POLICY.md`
6. **Hardware profile class:** Record CPU model, RAM, and OS version in baseline metadata

**Hardware Profile Compatibility Matrix:**

| Path | Min CPU | Min RAM | GPU Optional | Test Note |
|------|---------|---------|--------------|-----------|
| P1 (ANN Only) | 2 GHz | 4 GB | No | Always enabled |
| P2 (ANN + Tensor) | 2 GHz | 8 GB | No | Policy-dependent |
| P4 (Stale/Exact) | 2 GHz | 8 GB | No | Always enabled |
| P5 (Distributed) | 2+ GHz | 16 GB | No | Shard manifest required |
| Error paths | 2 GHz | 4 GB | No | Fault injection enabled |

## Expected Baseline Results (Placeholder)

**To be filled after first successful benchmark run on reference hardware (Phase 6):**

### Planner Decision Latency
```
Path 1 (ANN Only)             p50: 75 ns,  p95: 120 ns,  p99: 180 ns
Path 2 (ANN + Tensor)         p50: 100 ns, p95: 200 ns,  p99: 350 ns
Path 4 (Stale Tensor)         p50: 85 ns,  p95: 180 ns,  p99: 280 ns
Path 4 (force_exact)          p50: 80 ns,  p95: 150 ns,  p99: 250 ns
Path 5 (Distributed)          p50: 250 ns, p95: 420 ns,  p99: 650 ns

Fallback Rate (full eligibility):  ≤ 2%
Fallback Rate (GPU unavailable):   ≤ 8%
Fallback Rate (distributed):       ≤ 5%
```

### Benchmark Matrix Throughput
```
record() operation:      ~800 ns/op (1.25M ops/sec)
lookup() operation:      ~1200 ns/op (833K ops/sec)
Full fill cycle:         ~4800 ns/op (208K ops/sec batch)
Scenario slice:          ~400 ns/op (2.5M ops/sec)

Memory footprint:        < 8 MB total
```

### Artifact Staleness Detection
```
Single artifact (full policy):   ~40 µs (p95)
Batch of 10 artifacts:           ~400 µs total (40 µs amortized)
Batch of 100 artifacts:          ~3500 µs total (35 µs amortized)
Rebuild identification:          ~8 µs per artifact
```

### Storage Strategy Placement
```
compute_placement() (4 nodes):   ~350 µs (p95)
compute_placement() (12 nodes):  ~450 µs (p95)
compute_placement() (24 nodes):  ~500 µs (p95)

validate_placement():            ~80 µs (p95)
optimize_placement():            ~180 µs (p95)

Full cycle (24 nodes):           ~760 µs (p95)
```

## Build Environment Status

**Current blocker (Phase 5 evidence capture):**
- vcpkg initialization: not available in current CI environment
- RocksDB: not available system-wide (community-release preset requires vcpkg)
- Status: Benchmarks compile in linux-release preset; measured baseline evidence deferred

**Workaround:**
- Run `cmake --preset linux-release` to build all available benchmarks
- Measured baselines will be captured when vcpkg/RocksDB environment becomes available
- See `src/evaluation/MODULE_EVIDENCE.md` §Justified Gap for full context

## Reference Documents

- `src/evaluation/PERFORMANCE_EXPECTATIONS.md` — Complete guardrail definitions
- `src/evaluation/ROADMAP.md` §Phase 5 — Benchmark closure status
- `docs/EPIC2_BENCHMARK_FRAMEWORK.md` — Benchmark design principles
- `src/evaluation/MODULE_EVIDENCE.md` §Current Build Status — Environment constraints

## Installation

Benchmark targets are installed under `bin/benchmarks/` when full install is performed:

```bash
cmake --install build --config Release --component benchmarks
ls /usr/local/bin/benchmarks/
# Output:
# benchmark_matrix_bench
# planner_decision_bench
# bench_epic2_evaluation_storage_strategy_bench
# bench_epic2_evaluation_artifact_staleness_bench
```

## Usage in Continuous Integration

**For release-critical CI gate:**
1. Build benchmarks with `THEMIS_BUILD_BENCHMARKS=ON`
2. Run via `ctest -R epic2_evaluation_bench`
3. Capture stdout/stderr to benchmark log
4. Compare against baseline thresholds from `src/evaluation/BASELINES.md` (Phase 6)
5. Fail CI if any guardrail exceeded by > 20%

**For local development:**
1. Run full benchmark suite before committing changes to `src/evaluation/`
2. Document expected latency impact in PR description if > 5% change
3. Provide regression justification if guardrail exceeded
