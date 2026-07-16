# Wave 5 Benchmark Runbook
<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- Copyright (c) 2026 ThemisDB Contributors -->

**Wave**: 5 — Pre-Production Performance Confidence  
**Scope**: Production-critical workload coverage, degradation validation,  
           determinism / variance control, governance & diagnostics  
**Owner**: ThemisDB Performance Team  
**Last Reviewed**: 2026-07-16

---

## Table of Contents

1. [Overview](#1-overview)
2. [Benchmark Targets](#2-benchmark-targets)
3. [Repro Steps](#3-repro-steps)
4. [Release Gate Checks](#4-release-gate-checks)
5. [Baseline Strategy](#5-baseline-strategy)
6. [Variance & CV Monitoring](#6-variance--cv-monitoring)
7. [Degradation Scenario Interpretation](#7-degradation-scenario-interpretation)
8. [Known Gaps & Limitations](#8-known-gaps--limitations)
9. [Extension Guide](#9-extension-guide)

---

## 1. Overview

Wave 5 establishes four complementary benchmark layers that together provide
**pre-production performance confidence**:

| PR    | Target              | Goal                                              |
|-------|---------------------|---------------------------------------------------|
| B5-A  | `bench_w5a_*`       | E2E production workloads; burst + sustained modes |
| B5-B  | `bench_w5b_*`       | Failure/degradation scenarios; throughput cliffs  |
| B5-C  | `bench_w5c_*`       | Variance control; CV ≤ 5%; RNG determinism        |
| B5-D  | `bench_w5d_*`       | Hard release gates; structured diagnostic output  |

All benchmarks use **kW5CanonicalSeed = 42** and **OS temp dir** paths to
ensure reproducible, collision-free runs.

---

## 2. Benchmark Targets

| Binary                          | Source file                          | CTest labels   |
|---------------------------------|--------------------------------------|----------------|
| `bench_w5a_production_workloads`| `bench_w5a_production_workloads.cpp` | `wave5;w5a`    |
| `bench_w5b_degradation`         | `bench_w5b_degradation.cpp`          | `wave5;w5b`    |
| `bench_w5c_determinism`         | `bench_w5c_determinism.cpp`          | `wave5;w5c`    |
| `bench_w5d_governance`          | `bench_w5d_governance.cpp`           | `wave5;w5d`    |

---

## 3. Repro Steps

### 3.1 Build (Windows — canonical preset)

```powershell
cmake --preset windows-release
cmake --build --preset windows-release --parallel 16 `
    --target bench_w5a_production_workloads `
             bench_w5b_degradation `
             bench_w5c_determinism `
             bench_w5d_governance
```

### 3.2 Build (Linux)

```bash
cmake --preset linux-release
cmake --build --preset linux-release --parallel 16 \
    --target bench_w5a_production_workloads \
             bench_w5b_degradation \
             bench_w5c_determinism \
             bench_w5d_governance
```

### 3.3 Run all Wave 5 benchmarks via CTest

```bash
ctest --preset windows-release -L wave5 --output-on-failure -j 1 --timeout 120
```

### 3.4 Run a single target with JSON output

```bash
./bench_w5d_governance \
    --benchmark_out=bench_w5d.json \
    --benchmark_out_format=json \
    --benchmark_filter=BM_W5D \
    --benchmark_min_time=0.2s
```

### 3.5 Run regression report

```bash
python benchmarks/wave5/report_variance_w5.py \
    --input    bench_w5d.json \
    --baseline benchmarks/baselines/wave5/bench_w5d_baseline.json
```

### 3.6 Multi-run variance check (5 runs)

```bash
for i in 1 2 3 4 5; do
    ./bench_w5c_determinism \
        --benchmark_out=run${i}.json \
        --benchmark_out_format=json \
        --benchmark_filter=BM_W5C_ReadVariance
done

python benchmarks/wave5/report_variance_w5.py \
    --multi-run run1.json run2.json run3.json run4.json run5.json
```

---

## 4. Release Gate Checks

Gates are defined in `release_gate_manifest_w5.json`.
All gates must **PASS** before a release branch is cut.

| Gate   | Benchmark                         | Criterion                          | Severity |
|--------|-----------------------------------|------------------------------------|----------|
| W5D-1  | `BM_W5D_Gate1_ReadLatency`        | p50 ≤ 20 µs AND p99 ≤ 200 µs      | FAIL     |
| W5D-2  | `BM_W5D_Gate2_WriteThroughput`    | throughput ≥ 80 000 ops/s          | FAIL     |
| W5D-3  | `BM_W5D_Gate3_RangeScanLatency`   | range-scan p99 ≤ 500 µs            | FAIL     |
| W5D-4  | `BM_W5D_Gate4_BatchCommitLatency` | 100-rec batch p99 ≤ 5 ms           | FAIL     |
| W5C-CV | `BM_W5C_ReadVariance`             | CV ≤ 5% (CV_ok == 1)               | WARN     |
| W5C-RNG| `BM_W5C_RngDeterminism/1000`      | deterministic == 1                 | FAIL     |

**How to interpret gate_pass = 0.0**:

1. Check the benchmark label for the observed p50/p99 values.
2. Compare against the baseline: `report_variance_w5.py --input <file> --baseline <baseline>`.
3. Identify the root cause: hardware variance, compaction stall, or genuine regression.
4. If hardware variance: re-run on the canonical CI runner (isolated, no other load).
5. If genuine regression: open a performance issue and block the release.

---

## 5. Baseline Strategy

### 5.1 Baseline files

| File                                              | Description                  |
|---------------------------------------------------|------------------------------|
| `baselines/wave5/bench_w5a_baseline.json`         | B5-A workload baselines      |
| `baselines/wave5/bench_w5b_baseline.json`         | B5-B degradation baselines   |
| `baselines/wave5/bench_w5c_baseline.json`         | B5-C determinism baselines   |
| `baselines/wave5/bench_w5d_baseline.json`         | B5-D gate baselines          |

### 5.2 Creating an initial baseline

```bash
./bench_w5d_governance \
    --benchmark_out=bench_w5d.json \
    --benchmark_out_format=json

python benchmarks/wave5/report_variance_w5.py \
    --input    bench_w5d.json \
    --baseline benchmarks/baselines/wave5/bench_w5d_baseline.json \
    --update-baseline
```

### 5.3 Updating baselines after intentional improvements

After a confirmed performance improvement (not a regression):

```bash
python benchmarks/wave5/report_variance_w5.py \
    --input    bench_w5d.json \
    --baseline benchmarks/baselines/wave5/bench_w5d_baseline.json \
    --update-baseline
```

Commit the updated baseline file with the message:
```
perf(baselines/wave5): update W5D baseline after <improvement description>
```

### 5.4 Regression thresholds

| Change       | Action                     |
|--------------|----------------------------|
| < 10%        | OK — no action required    |
| 10% – 20%    | WARNING — investigate      |
| ≥ 20%        | FAIL — block release, open issue |

---

## 6. Variance & CV Monitoring

Wave 5 benchmarks track **Coefficient of Variation (CV)** as the primary
stability metric.  A CV > 5% indicates measurement noise that reduces
baseline comparability.

**Causes of high CV and remediation**:

| Cause                          | Remediation                                    |
|--------------------------------|------------------------------------------------|
| Insufficient warmup            | Ensure 3-phase warmup (cold → warm → hot)      |
| Shared CI runner load          | Use dedicated runner; set CPU governor=performance |
| NUMA effects                   | Pin benchmark to a single socket (numactl)     |
| RocksDB background compaction  | Increase block_cache; run after explicit flush |
| OS jitter (THP, ASLR)          | Disable THP; use `--benchmark_enable_random_interleaving=false` |

**How to measure CV across 5 runs**:

```bash
for i in 1 2 3 4 5; do
    ./bench_w5c_determinism --benchmark_out=run${i}.json \
        --benchmark_out_format=json \
        --benchmark_filter=BM_W5C_ReadVariance
done
python benchmarks/wave5/report_variance_w5.py --multi-run run*.json
```

---

## 7. Degradation Scenario Interpretation

### BM_W5B_ReadLatencyInjection

- **Arg(0) = 0**: baseline read latency, no injection
- **Arg(0) = 50**: simulates one-hop network latency (50 µs RTT)
- **Arg(0) = 100**: simulates degraded storage tier
- **Arg(0) = 500**: simulates heavily degraded path (disk queue saturation)

**Expected outcome**: throughput should degrade linearly with injected delay.
A superlinear drop indicates queueing or scheduling overhead.

### BM_W5B_WriteFlood_Throughput

Tracks the point at which write throughput plateaus due to write-buffer
saturation.  Compare 1-thread vs. 4-thread runs; if 4-thread throughput
< 2× single-thread, investigate lock contention.

### BM_W5B_IndexRebuild_Cost

Represents the time-to-recover-index after a partial failure.  Arg = 500, 1000,
2000 records; extrapolate to production corpus size to estimate recovery SLA.

### BM_W5B_MixedContention_80_20

An 80/20 read/write mix at 2, 4, and 8 threads.  Expected: throughput scales
sub-linearly.  A regression here indicates lock hot spots; compare thread_index
% 5 assignment vs. actual writer distribution in production.

---

## 8. Known Gaps & Limitations

| Gap                                              | Priority | Planned Resolution            |
|--------------------------------------------------|----------|-------------------------------|
| No AQL query benchmarks in W5A                   | Medium   | Wave 6 — AQL E2E workloads    |
| Degradation tests use simulated delay (not real)  | Medium   | Wave 6 — chaos framework hook |
| No GPU/CUDA path in W5A vector benchmarks        | Low      | After CUDA kernel integration |
| W5B index rebuild uses `dropIndex` API (proxy)   | Low      | Replace with `rebuildIndex`   |
| No distributed/shard variant in W5D gates        | Low      | Wave 6 — distributed gates    |
| Baselines captured on dev box (not CI runner)    | High     | Re-capture on dedicated runner |

---

## 9. Extension Guide

### Adding a new workload to B5-A

1. Add a `BENCHMARK_DEFINE_F` / `BENCHMARK_REGISTER_F` block to
   `bench_w5a_production_workloads.cpp`.
2. Follow the fixture pattern: `SetUp` warmup → measurement window.
3. Use `kW5CanonicalSeed` and `w5a::tempPath()`.
4. Register the benchmark name in `release_gate_manifest_w5.json`
   under `workload_profiles.W5A.benchmarks`.

### Adding a new release gate to B5-D

1. Add a `BENCHMARK_DEFINE_F` block to `bench_w5d_governance.cpp`.
2. Emit `gate_pass`, `p50_us`/`p99_us` or `throughput` counters.
3. Add a gate entry to `release_gate_manifest_w5.json`.
4. Capture an initial baseline: `--update-baseline`.

### Ownership

- Wave 5 benchmark code: `benchmarks/wave5/` — ThemisDB Performance Team
- Baselines: `benchmarks/baselines/wave5/` — must be updated via PR with
  reviewer sign-off from a maintainer
- Gate manifest: `release_gate_manifest_w5.json` — changes require 2 approvals
