# Wave 8 Reproduction & Triage Guide

<!-- ThemisDB | REPRO_TRIAGE_W8.md | Version: 0.0.1 -->

## Purpose

Step-by-step instructions for reproducing Wave 8 benchmark failures and
triaging regressions before escalating to the on-call team.

---

## Prerequisites

```bash
# Ensure release build exists
cmake --preset linux-release
cmake --build --preset linux-release --parallel 16

# Verify W8 binaries are present
ls build/linux-release/bench_w8*
```

---

## Step 1 — Reproduce the Failure

```bash
# Run the failing sub-wave with verbose output
cd build/linux-release

# W8-A
./bench_w8a_incident_regression_shielding \
    --benchmark_format=json \
    --benchmark_out=/tmp/w8a_repro.json \
    --benchmark_repetitions=10 \
    --benchmark_report_aggregates_only=false

# W8-B
./bench_w8b_threshold_hardening_drift_detection \
    --benchmark_format=json \
    --benchmark_out=/tmp/w8b_repro.json \
    --benchmark_repetitions=10

# W8-C
./bench_w8c_deterministic_ci_harness \
    --benchmark_format=json \
    --benchmark_out=/tmp/w8c_repro.json \
    --benchmark_repetitions=10

# W8-D
./bench_w8d_operability_runbooks_ownership \
    --benchmark_format=json \
    --benchmark_out=/tmp/w8d_repro.json
```

---

## Step 2 — Run the Variance Report

```bash
# Check gates for each sub-wave
python3 benchmarks/wave8/report_variance_w8.py --input /tmp/w8a_repro.json
python3 benchmarks/wave8/report_variance_w8.py --input /tmp/w8b_repro.json
python3 benchmarks/wave8/report_variance_w8.py --input /tmp/w8c_repro.json
python3 benchmarks/wave8/report_variance_w8.py --input /tmp/w8d_repro.json
```

Exit code 0 = all hard gates passed. Exit code 1 = one or more failures.

---

## Step 3 — Check for Environment Noise

```bash
# CPU governor (must be 'performance' for consistent results)
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor | sort -u

# Set performance governor (requires root)
echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Check for competing processes
top -b -n1 | head -20

# Disable CPU frequency scaling during measurement
sudo cpupower frequency-set -g performance
```

---

## Step 4 — Isolate the Regression

```bash
# Run only the failing benchmark by name
./bench_w8a_incident_regression_shielding \
    --benchmark_filter="IRS07" \
    --benchmark_repetitions=15 \
    --benchmark_format=json \
    --benchmark_out=/tmp/irs07_isolated.json

# Compare against W7 baseline (if available)
python3 tools/compare.py --alpha 0.05 \
    benchmarks/wave7/baselines/w7a_baseline.json \
    /tmp/irs07_isolated.json
```

---

## Step 5 — ThreadSanitizer Rerun (IRS-01, IRS-08, CCR-04)

```bash
# Rebuild with TSAN
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1" \
      -DCMAKE_BUILD_TYPE=Debug \
      -B build/tsan -G Ninja .
cmake --build build/tsan --parallel 8

# Run targeted check
./build/tsan/bench_w8a_incident_regression_shielding \
    --benchmark_filter="IRS01|IRS08" \
    --benchmark_repetitions=3
```

---

## Step 6 — Escalation Decision Tree

```
Gate failure detected?
├── GATE-W8-01 / GATE-W8-03 (p99 > 175 µs)
│   ├── Fails in isolation? → storage regression → page @storage-team
│   └── Fails only under load? → contention issue → page @reliability
├── GATE-W8-02 (audit < 90 k ops/s)
│   ├── Single-threaded regression? → lock overhead → @observability
│   └── Multi-thread regression only? → contention → @reliability
├── GATE-W8-04 (write-storm < 60 k ops/s)
│   └── → page @storage-team, attach perf report
├── GATE-W8-05 (triage_completeness < 1.0)
│   └── → fix missing gate_passed counter, no escalation needed
└── GATE-W8-06 (coverage < 80%)
    └── → add runbook entries, no escalation needed
```

---

## Known Flaky Patterns

| Benchmark | Flake type | Mitigation |
|-----------|------------|------------|
| IRS-01 concurrent race | Thread scheduling | Increase repetitions to 15; use CPU affinity if available |
| THD-06 concurrent writes | Compaction interference | Run in isolation; disable background compaction during measurement |
| DCH-05 clock resolution | VM granularity | Only valid on bare-metal; skip on shared CI VMs |

---

## Baseline References

| Wave | Baseline location | Key metric |
|------|------------------|------------|
| W7 | `benchmarks/wave7/baselines/` | Read p99 ≤ 200 µs |
| W8 | `benchmarks/wave8/baselines/` | Read p99 ≤ 175 µs |
