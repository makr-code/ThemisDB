# Wave 4 Benchmark Runbook

> **Current CI status:** Wave 4 is a smoke/diagnostic lane, not a release or
> performance gate. Gate evaluation in `report_variance.py` is disabled.
> The historical baseline does not provide verified matching hardware/build
> identity; its OS description also conflicts with the manifest freeze.
> The manifest's gate definitions and thresholds are historical proposals,
> not release evidence. Use `--no-gates` for variance diagnostics only.

**Scope:** Benchmark smoke, variance diagnostics, resilience scenarios,
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

Wave 4 contains benchmark scenarios and historical gate proposals. The CI lane
and reporter currently provide smoke and diagnostic output only:

| PR   | Area                               | Key Deliverables                                    |
|------|------------------------------------|-----------------------------------------------------|
| B4-A | Governance scenarios               | 6 proposed gates (W4A-01..06), manifest JSON        |
| B4-B | Resilience/Degradation Scenarios   | 6 degraded-state benchmarks (W4B-01..06)            |
| B4-C | Determinism & Variance Control     | 6 determinism benchmarks (W4C-01..06), CV ≤ 15%    |
| B4-D | Diagnostics & Maintainability      | 5 diagnostic benchmarks, report_variance.py tool    |

---

## Quick Start

```bash
# 1. Configure and build the Wave 4 benchmarks
cmake --preset linux-release -B build-linux-release -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build-linux-release --target \
  bench_w4a_release_gates bench_w4b_resilience \
  bench_w4c_determinism bench_w4d_diagnostics --parallel 2

# 2. Run all Wave 4 benchmarks
./build-linux-release/bin/bench_w4a_release_gates \
  --benchmark_out=results_w4a.json --benchmark_out_format=json

./build-linux-release/bin/bench_w4b_resilience \
  --benchmark_out=results_w4b.json --benchmark_out_format=json

./build-linux-release/bin/bench_w4c_determinism \
  --benchmark_out=results_w4c.json --benchmark_out_format=json

./build-linux-release/bin/bench_w4d_diagnostics \
  --benchmark_out=results_w4d.json --benchmark_out_format=json

# 3. Analyse variance (diagnostic only)
python3 benchmarks/wave4/report_variance.py \
  --input results_w4a.json \
  --no-gates \
  --output variance_report.json
```

---

## Benchmark Catalogue

### B4-A: Release-Critical Governance (`bench_w4a_release_gates`)

These are historical manifest thresholds, not active CI performance gates.

| Gate ID | Benchmark                              | Metric                 | Threshold |
|---------|----------------------------------------|------------------------|-----------|
| W4A-01  | `W4A_01_WriteThroughput/500`           | `items_per_second`     | −10% max  |
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

`release_gate_manifest.json` describes proposed thresholds and criticality
levels, but neither is enforced as a release or performance decision:

| Manifest criticality | Intended historical classification |
|----------------------|------------------------------------|
| `release-blocking`   | Proposed release-blocking check    |
| `advisory`           | Proposed non-blocking check        |

**Proposed release-blocking:** W4A-01, W4A-02, W4A-03, W4A-04, W4A-06.

**Proposed advisory:** W4A-05, all W4B gates.

### Gate Evaluation

Gate evaluation is disabled: requesting it exits `2`, including when a
baseline and manifest are supplied. `--baseline` is also rejected with
`--no-gates`. A successful `--no-gates` invocation exits `0` for diagnostic
analysis only; missing or invalid input exits nonzero. Its JSON report has
`release_blocked: null`, not a release pass.

---

## Running Wave 4 Benchmarks

### W4A Smoke Completeness

The CI smoke invocation selects `W4A` and requests five Google Benchmark
repetitions. Its explicit case list and repetition count live in
`smoke_contract.json`; the contract is checked against the registered source
arguments by `benchmarks/tests/test_wave4_smoke_completeness.py`. After the
run, `validate_smoke.py` requires exactly one raw `run_type: "iteration"` row
for every expected case and each `repetition_index` from 0 through 4.
`run_type: "aggregate"` rows are not repetitions. Missing, duplicate, extra,
skipped, errored, or malformed cases fail validation.

This validates execution completeness only. A passing smoke is not a
performance result, does not validate metrics or hardware identity, and does
not approve or update a baseline. Performance comparison remains a separate
reporting operation.

### Reproducibility Requirements

1. Use the `linux-release` or `windows-release` CMake preset.
2. Pin the `BENCHMARK_SEED` environment variable to `42`.
3. Close other background processes to reduce OS noise.
4. Record the actual machine and build identity; a shared CI runner class is
   not proof of baseline equivalence.
5. Archive `--benchmark_out` JSON files as build artifacts.

### Single-case Diagnostic Repro

```bash
# Example: Reproduce W4A-02 (read latency p99)
./build-linux-release/bin/bench_w4a_release_gates \
  --benchmark_filter='StorageBenchFixture/W4A_02_ReadLatencyP99/10000' \
  --benchmark_repetitions=5 \
  --benchmark_out=repro_w4a02.json \
  --benchmark_out_format=json
```

### CI Integration (GitHub Actions)

The active `.github/workflows/benchmark-gate.yml` builds and runs W4A as a
smoke. The contract test checks the declared cases against source registration;
validation requires all five raw repetitions for all 13 cases and rejects
reported failures and skips before running diagnostics:

```yaml
- name: Validate Wave 4 smoke
  run: |
    python3 benchmarks/wave4/validate_smoke.py --input results_w4a.json \
      --contract benchmarks/wave4/smoke_contract.json
    python3 tools/ci/validate_benchmark_smoke.py results_w4a.json \
      --wave4-manifest benchmarks/wave4/release_gate_manifest.json
    python3 benchmarks/wave4/report_variance.py \
      --input results_w4a.json \
      --no-gates \
      --output variance_report.json
```

This verifies execution completeness, not metric correctness or performance
evidence. The separate scientific framework specification in
`benchmarks/docs/CI_GATE.md` does not activate a Wave 4 gate.

---

## Variance Analysis & Reporting

`report_variance.py` provides:

1. **Variance table** — p50/p95/p99/CV for all Wave 4 benchmarks that
   emit VarianceTracker counters.

2. **Structured JSON output** — machine-readable diagnostic report for CI
   artifacts. Gate evaluation and baseline comparison are disabled.

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

The checked-in `baseline_v1.5.0.json` is historical, not an active CI
baseline. Before a gate can be enabled, maintainers must review provenance,
establish matching hardware/build and measurement configuration, validate
complete successful case/repetition evidence and the comparison policy, and
approve a suitable baseline. Repetitions alone do not establish readiness.

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
mode 0 (baseline) only as a diagnostic under comparable conditions.
The historical manifest's advisory thresholds are not evaluated by the CLI.

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

These are historical criteria, not active CI decisions. Investigate variance
and environment before proposing a baseline.

---

## Diagnostics (B4-D)

### Pipeline Health Check (W4D-03)

```bash
./build-linux-release/bin/bench_w4d_diagnostics \
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

The `sentinel_hash` counter can be inspected with a separately verified
reference using the same seed and index size. A difference warrants
investigation; no such check is performed by the reporter or CI smoke.

### Fixture Overhead (W4D-04)

`p99_ns` from W4D-04 is the upper bound of fixture overhead.  It should
be subtracted from per-operation latencies when the fixture setup/teardown
cost dominates (typically only for very fast micro-benchmarks).

---

## Known Limitations

1. **Smoke-only CI:** `.github/workflows/benchmark-gate.yml` is active as a
   smoke, not as performance gate enforcement. The reporter CLI rejects gate
   evaluation, even when called outside CI. Legacy evaluation functions remain
   importable and are not validated release-decision APIs.

2. **Degradation modes are synthetic:** B4-B latency injection uses
   `std::this_thread::sleep_for` which has OS scheduling jitter.  Results
   are deterministic in expectation but not per-sample.  Use `p99_ns`
   rather than `real_time` for comparisons.

3. **Backpressure simulator is single-threaded:** The
   `BackpressureSimulator` in `wave4_fixtures.h` is not safe for
   concurrent benchmark bodies.  For multi-threaded backpressure tests,
   use per-thread instances.

4. **No verified comparable baseline:** The checked-in historical baseline
   lacks established hardware/build identity and conflicts with the manifest
   OS freeze. A new measurement alone cannot authorize gate activation.

5. **CV criterion applies to release builds only:** In debug builds,
   CV > 0.15 is expected due to additional instrumentation.

---

## Extension Guide

### Proposing a New Release Gate

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
6. Review baseline provenance and gate readiness before any activation; the
   current reporter cannot evaluate the proposed gate.

### Adding a New Resilience Scenario

1. Add a benchmark in `bench_w4b_resilience.cpp` using
   `DegradedStorageFixture` or standalone with `LatencyInjector` /
   `BackpressureSimulator`.
2. Set `gate_ref` counter to the corresponding W4A gate ID.
3. Add a `resilience_reference_gates` entry in the manifest.

### Modifying Variance Thresholds

Changes to `variance_acceptance_criteria` require review and do not activate
enforcement. Update the runbook to describe any approved changes.

---

*Runbook version: 1.1 | Last updated: 2026-11-23 | Owner: ThemisDB Performance Team*
