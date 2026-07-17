# Wave 4 Benchmark Runbook

**Scope:** Release-readiness, performance governance, resilience scenarios,
determinism/variance control, and diagnostics for ThemisDB benchmarks.

**Applies to:** Branch `develop` / release `v1.5.0+`

---

## Table of Contents

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Benchmark Catalogue](#benchmark-catalogue)
4. [Release Gates](#release-gates)
5. [Running Wave 4 Benchmarks](#running-wave-4-benchmarks)
6. [Variance Analysis & Reporting](#variance-analysis--reporting)
7. [Baseline Strategy](#baseline-strategy)
8. [Resilience Scenarios (B4-B)](#resilience-scenarios-b4-b)
9. [Variance Acceptance Criteria (B4-C)](#variance-acceptance-criteria-b4-c)
10. [Diagnostics (B4-D)](#diagnostics-b4-d)
11. [Known Limitations](#known-limitations)
12. [Extension Guide](#extension-guide)

---

## Overview

Wave 4 lifts the ThemisDB benchmark pipeline to **release-decision level**.  It
introduces:

| PR   | Area                               | Key Deliverables                                    |
|------|------------------------------------|-----------------------------------------------------|
| B4-A | Release-Critical Governance        | 6 release gates (W4A-01..06), gate manifest JSON    |
| B4-B | Resilience/Degradation Scenarios   | 6 degraded-state benchmarks (W4B-01..06)            |
| B4-C | Determinism & Variance Control     | 6 determinism benchmarks (W4C-01..06), CV ≤ 15%    |
| B4-D | Diagnostics & Maintainability      | 5 diagnostic benchmarks, report_variance.py tool    |

---

## Quick Start

```bash
# 1. Build (linux-release preset recommended for stable numbers)
cmake --preset linux-release
cmake --build --preset linux-release --parallel 16

# 2. Run all Wave 4 benchmarks
./build/linux-release/bin/benchmarks/bench_w4a_release_gates \
  --benchmark_out=results_w4a.json --benchmark_out_format=json

./build/linux-release/bin/benchmarks/bench_w4b_resilience \
  --benchmark_out=results_w4b.json --benchmark_out_format=json

./build/linux-release/bin/benchmarks/bench_w4c_determinism \
  --benchmark_out=results_w4c.json --benchmark_out_format=json

./build/linux-release/bin/benchmarks/bench_w4d_diagnostics \
  --benchmark_out=results_w4d.json --benchmark_out_format=json

# 3. Analyse variance
python3 benchmarks/wave4/report_variance.py \
  --input results_w4a.json \
  --manifest benchmarks/wave4/release_gate_manifest.json

# 4. Compare against baseline (if available)
python3 benchmarks/wave4/report_variance.py \
  --input results_w4a.json \
  --baseline benchmarks/wave4/baselines/baseline_v1.5.0.json \
  --manifest benchmarks/wave4/release_gate_manifest.json \
  --output release_gate_report.json
```

---

## Benchmark Catalogue

### B4-A: Release-Critical Governance (`bench_w4a_release_gates`)

| Gate ID | Benchmark                              | Metric                 | Threshold |
|---------|----------------------------------------|------------------------|-----------|
| W4A-01  | `W4A_01_WriteThoughput/500`            | `items_per_second`     | −10% max  |
| W4A-02  | `W4A_02_ReadLatencyP99/10000`          | `p99_ns`               | +15% max  |
| W4A-03  | `W4A_03_VectorInsertThroughput/200`    | `items_per_second`     | −10% max  |
| W4A-04  | `W4A_04_VectorSearchLatency/10000`     | `p99_ns`               | +15% max  |
| W4A-05  | `W4A_05_GraphBFSDepth2/5000`           | `p99_ns`               | +20% max  |
| W4A-06  | `BM_W4A_06_MixedConcurrent/500`        | `items_per_second`     | −15% max  |

### B4-B: Resilience/Degradation (`bench_w4b_resilience`)

| Benchmark      | Degradation Mode          | Baseline Reference |
|----------------|---------------------------|--------------------|
| W4B-01         | None / Mild / Severe latency | W4A-01 (writes) |
| W4B-02         | Backpressure 500 ops/s    | W4A-02 (reads)     |
| W4B-03         | CPU contention (0/2/4 threads) | W4A-04 (vector) |
| W4B-04         | Throughput cap (uncapped/1000/200 ops/s) | W4A-01 |
| W4B-05         | 0/10/20% node unavailability | W4A-05 (graph) |
| W4B-06         | 10/25/50% data loss recovery | W4A-01 (writes) |

### B4-C: Determinism / Variance Control (`bench_w4c_determinism`)

| Benchmark | Purpose                                  | Acceptance Criterion      |
|-----------|------------------------------------------|---------------------------|
| W4C-01    | Seed stability                           | `stability_ok = 1.0`      |
| W4C-02    | Warmup effectiveness                     | `warm_ns < cold_ns` (I/O) |
| W4C-03    | Storage write CV                         | `cv ≤ 0.15`               |
| W4C-04    | Vector search CV                         | `cv ≤ 0.15`               |
| W4C-05    | Teardown isolation                       | `isolation_ok = 1.0`      |
| W4C-06    | Clock precision                          | `min_tick_ns < 1000`      |

### B4-D: Diagnostics (`bench_w4d_diagnostics`)

| Benchmark | Purpose                                          |
|-----------|--------------------------------------------------|
| W4D-01    | Storage throughput summary with structured counters |
| W4D-02    | Vector search regression sentinel (hash fingerprint) |
| W4D-03    | Pipeline health check (bitmask, `health_ok`)    |
| W4D-04    | Fixture setup/teardown overhead measurement     |
| W4D-05    | Consolidated workload profile (all 3 paths)     |

---

## Release Gates

Release gates are defined in `release_gate_manifest.json`.  The manifest
distinguishes two criticality levels:

| Criticality         | CI Effect                              |
|---------------------|----------------------------------------|
| `release-blocking`  | Gate failure **blocks** release/merge  |
| `advisory`          | Gate failure emits a warning; non-blocking |

**Release-blocking gates:** W4A-01, W4A-02, W4A-03, W4A-04, W4A-06.

**Advisory gates:** W4A-05, all W4B gates.

### Gate Evaluation

```bash
python3 benchmarks/wave4/report_variance.py \
  --input results_w4a.json \
  --baseline baselines/baseline_v1.5.0.json \
  --manifest benchmarks/wave4/release_gate_manifest.json \
  --output gate_report.json
```

Exit codes:
- `0` — all release-blocking gates passed.
- `1` — one or more release-blocking gates failed.
- `2` — input file not found or invalid.

---

## Running Wave 4 Benchmarks

### Reproducibility Requirements

1. Use the `linux-release` or `windows-release` CMake preset.
2. Pin the `BENCHMARK_SEED` environment variable to `42`.
3. Close other background processes to reduce OS noise.
4. Run on the same machine (or CI runner class) as the baseline.
5. Archive `--benchmark_out` JSON files as build artifacts.

### Single-gate Repro

```bash
# Example: Reproduce W4A-02 (read latency p99)
./bin/benchmarks/bench_w4a_release_gates \
  --benchmark_filter='StorageBenchFixture/W4A_02_ReadLatencyP99/10000' \
  --benchmark_repetitions=5 \
  --benchmark_out=repro_w4a02.json \
  --benchmark_out_format=json
```

### CI Integration (GitHub Actions)

When GitHub Actions workflows are enabled, add a job to
`.github/workflows/benchmark-gate.yml`:

```yaml
- name: Run Wave 4 Release Gates
  run: |
    ./build/linux-release/bin/benchmarks/bench_w4a_release_gates \
      --benchmark_filter='W4A' \
      --benchmark_out=results_w4a.json \
      --benchmark_out_format=json
    python3 benchmarks/wave4/report_variance.py \
      --input results_w4a.json \
      --baseline benchmarks/wave4/baselines/baseline_v1.5.0.json \
      --manifest benchmarks/wave4/release_gate_manifest.json \
      --output gate_report.json
  env:
    BENCHMARK_SEED: 42
```

See `benchmarks/docs/CI_GATE.md` for the full CI gate specification.

---

## Variance Analysis & Reporting

`report_variance.py` provides:

1. **Variance table** — p50/p95/p99/CV for all Wave 4 benchmarks that
   emit VarianceTracker counters.

2. **Gate evaluation** — compares current metrics against a baseline JSON
   and the thresholds in `release_gate_manifest.json`.

3. **Regression hints** — for each failing gate, emits a repro command and
   the exact pct-change to help triage.

4. **Structured JSON output** — machine-readable report for CI artifacts.

### Inspecting the CV

```bash
python3 benchmarks/wave4/report_variance.py --input results_w4c.json --no-gates
```

Look for `cv` values in the variance table.  If `cv > 0.15` for W4C-03 or
W4C-04, the measurement environment is too noisy for reliable regression
detection.  Common causes:

- Background processes competing for CPU/I/O.
- Debug/non-release build mode.
- Insufficient warmup iterations (increase `kDefaultWarmupIterations`).
- Clock granularity too coarse (`min_tick_ns > 1000` in W4C-06).

---

## Baseline Strategy

### Recording a New Baseline

```bash
# Run the full W4A suite with high repetitions.
./bin/benchmarks/bench_w4a_release_gates \
  --benchmark_repetitions=10 \
  --benchmark_out=benchmarks/wave4/baselines/baseline_v1.5.0.json \
  --benchmark_out_format=json

git add benchmarks/wave4/baselines/baseline_v1.5.0.json
git commit -m "chore(bench): record W4A baseline for v1.5.0"
```

### Baseline Freeze Policy

A new baseline is recorded when:
- A deliberate performance improvement is merged and verified.
- The CI runner hardware is upgraded.
- A new release branch is cut.

Baselines are versioned by release tag.  The active baseline for the
`develop` branch is `baseline_v1.5.0.json`.

---

## Resilience Scenarios (B4-B)

B4-B benchmarks run the same workloads as B4-A but with one of four
degradation modes applied:

| Mode | Description                         | LatencyInjector / BackpressureSimulator |
|------|-------------------------------------|-----------------------------------------|
| 0    | Baseline (no degradation)           | None                                    |
| 1    | Mild latency injection              | 50–200 µs uniform delay                 |
| 2    | Severe latency injection            | 500–2000 µs uniform delay               |
| 3    | Backpressure                        | 500 ops/s token bucket                  |

**Interpreting results:**

Compare the `p99_ns` counter from the degraded run (mode ≥ 1) against
mode 0 (baseline).  Document the degradation ratio in release notes if
it exceeds the advisory threshold in the manifest
(`max_degradation_pct_vs_baseline`).

**Deviation patterns to investigate:**

- **p99 spike without p50/p95 change:** Tail-latency outlier; suspect GC,
  OS preemption, or lock contention rather than median path.
- **CV increase under backpressure:** Token-bucket sleep jitter — expected;
  check that `min_tick_ns` (W4C-06) is < 1 µs on the platform.
- **Throughput collapse under mode 3:** 500 ops/s cap is intentional;
  verify that actual throughput is ≈ 500 ops/s (sanity check).

---

## Variance Acceptance Criteria (B4-C)

| Criterion     | Gate    | Limit   | Meaning                                    |
|---------------|---------|---------|--------------------------------------------|
| CV ≤ 0.15     | W4C-03  | 15%     | Storage write variance acceptable           |
| CV ≤ 0.15     | W4C-04  | 15%     | Vector search variance acceptable           |
| stability_ok  | W4C-01  | 1.0     | RNG seed produces identical sequences       |
| isolation_ok  | W4C-05  | 1.0     | No state leak between fixture iterations   |
| min_tick_ns   | W4C-06  | < 1000  | Sub-µs timing resolution available         |

If any criterion fails in CI, investigate the environment before
recording a new baseline.

---

## Diagnostics (B4-D)

### Pipeline Health Check (W4D-03)

```bash
./bin/benchmarks/bench_w4d_diagnostics \
  --benchmark_filter='BM_W4D_03_PipelineHealthCheck' \
  --benchmark_out=health.json --benchmark_out_format=json

# health_ok = 1.0 → all three components (storage/vector/graph) are healthy
```

The `component` counter is a bitmask:
- Bit 0 (1) = storage read succeeded.
- Bit 1 (2) = vector search returned results.
- Bit 2 (4) = graph node lookup succeeded.

A value of `7` means all three passed.

### Regression Sentinel (W4D-02)

The `sentinel_hash` counter must match the recorded baseline value for
the same seed and index size.  If it differs, the index algorithm has
changed in a way that affects result sets — investigate before release.

### Fixture Overhead (W4D-04)

`p99_ns` from W4D-04 is the upper bound of fixture overhead.  It should
be subtracted from per-operation latencies when the fixture setup/teardown
cost dominates (typically only for very fast micro-benchmarks).

---

## Known Limitations

1. **No live CI activation yet:** GitHub Actions workflow definitions are
   stored in `.github/no_workflows`.  Gate enforcement requires manual
   or downstream pipeline invocation until workflows are activated.
   See `benchmarks/docs/CI_GATE.md` for the intended gate contract.

2. **Degradation modes are synthetic:** B4-B latency injection uses
   `std::this_thread::sleep_for` which has OS scheduling jitter.  Results
   are deterministic in expectation but not per-sample.  Use `p99_ns`
   rather than `real_time` for comparisons.

3. **Backpressure simulator is single-threaded:** The
   `BackpressureSimulator` in `wave4_fixtures.h` is not safe for
   concurrent benchmark bodies.  For multi-threaded backpressure tests,
   use per-thread instances.

4. **Baseline directory is initially empty:** The
   `benchmarks/wave4/baselines/` directory must be populated by running
   the baseline recording procedure (see [Baseline Strategy](#baseline-strategy))
   before gate comparison becomes meaningful.

5. **CV criterion applies to release builds only:** In debug builds,
   CV > 0.15 is expected due to additional instrumentation.

---

## Extension Guide

### Adding a New Release Gate

1. Add a new `BENCHMARK_DEFINE_F` / `BENCHMARK` in
   `bench_w4a_release_gates.cpp` with the next `W4A-NN` ID.
2. Register via `BENCHMARK_REGISTER_F` / `BENCHMARK(...)` with
   `UseRealTime()` for I/O paths.
3. Add a gate entry to `release_gate_manifest.json` following the
   existing schema.
4. Update `benchmarks/scripts/audit_benchmark_registration.py`'s
   `INTENTIONAL_EXCLUSIONS` if needed.
5. Run `python3 benchmarks/scripts/audit_benchmark_registration.py`
   to verify all sources are registered.
6. Record a new baseline after the gate is validated.

### Adding a New Resilience Scenario

1. Add a benchmark in `bench_w4b_resilience.cpp` using
   `DegradedStorageFixture` or standalone with `LatencyInjector` /
   `BackpressureSimulator`.
2. Set `gate_ref` counter to the corresponding W4A gate ID.
3. Add a `resilience_reference_gates` entry in the manifest.

### Modifying Variance Thresholds

Edit `release_gate_manifest.json` → `variance_acceptance_criteria`.
Update this runbook and `ROADMAP.md` accordingly.

---

*Runbook version: 1.0 | Last updated: 2026-07-16 | Owner: ThemisDB Performance Team*
