# Wave 9 Reproduction & Triage Guide

<!-- ThemisDB | REPRO_TRIAGE_W9.md | Version: 0.0.1 -->

## Purpose

Step-by-step instructions for reproducing Wave 9 benchmark failures and
triaging regressions before escalating to the on-call team.

---

## Prerequisites

```bash
# Ensure release build exists
cmake --preset linux-release
cmake --build --preset linux-release --parallel 16

# Verify W9 binaries are present
ls build/linux-release/bench_w9*
```

---

## Step 1 — Reproduce the Failure

```bash
# Run the failing sub-wave with verbose output
cd build/linux-release

# W9-A Security Overhead & Audit
./bench_w9a_security_overhead_audit \
    --benchmark_format=json \
    --benchmark_out=/tmp/w9a_repro.json \
    --benchmark_repetitions=10 \
    --benchmark_report_aggregates_only=false

# W9-B SLA Measurement & Compliance
./bench_w9b_sla_measurement_compliance \
    --benchmark_format=json \
    --benchmark_out=/tmp/w9b_repro.json \
    --benchmark_repetitions=10

# W9-C Chaos & Fault Recovery
./bench_w9c_chaos_fault_recovery \
    --benchmark_format=json \
    --benchmark_out=/tmp/w9c_repro.json \
    --benchmark_repetitions=10

# W9-D Multi-Tenant Isolation
./bench_w9d_multi_tenant_isolation \
    --benchmark_format=json \
    --benchmark_out=/tmp/w9d_repro.json \
    --benchmark_repetitions=10
```

---

## Step 2 — Run the Variance Report

```bash
# Check gates for each sub-wave
python3 benchmarks/wave9/report_variance_w9.py --input /tmp/w9a_repro.json
python3 benchmarks/wave9/report_variance_w9.py --input /tmp/w9b_repro.json
python3 benchmarks/wave9/report_variance_w9.py --input /tmp/w9c_repro.json
python3 benchmarks/wave9/report_variance_w9.py --input /tmp/w9d_repro.json
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
./bench_w9a_security_overhead_audit \
    --benchmark_filter="SOA08" \
    --benchmark_repetitions=15 \
    --benchmark_format=json \
    --benchmark_out=/tmp/soa08_isolated.json

# Compare against W8 baseline (if available)
python3 tools/compare.py --alpha 0.05 \
    benchmarks/wave8/baselines/w8a_baseline.json \
    /tmp/soa08_isolated.json
```

---

## Step 5 — ThreadSanitizer Rerun (SOA-03, SOA-08, MTI-07)

```bash
# Rebuild with TSAN
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1" \
      -DCMAKE_BUILD_TYPE=Debug \
      -B build/tsan -G Ninja .
cmake --build build/tsan --parallel 8

# Run targeted check
./build/tsan/bench_w9a_security_overhead_audit \
    --benchmark_filter="SOA03|SOA08" \
    --benchmark_repetitions=3

./build/tsan/bench_w9d_multi_tenant_isolation \
    --benchmark_filter="MTI07" \
    --benchmark_repetitions=3
```

---

## Step 6 — Escalation Decision Tree

```
Gate failure detected?
├── GATE-W9-01 (audit < 100 k ops/s)
│   ├── Single-threaded regression? → mutex overhead → @observability
│   └── Multi-thread regression only? → contention → @reliability
├── GATE-W9-02 (auth p99 > 150 µs)
│   └── → check hash implementation → page @security-team
├── GATE-W9-03 (node rejoin > 2000 µs)
│   └── → profile Stop/Start cycle → page @storage-team
├── GATE-W9-04 (RTO cycle > 5000 µs)
│   └── → check for real sleep calls → page @reliability
├── GATE-W9-05 (triage_completeness < 1.0)
│   └── → fix missing gate_passed counter, no escalation needed
└── GATE-W9-06 (cross-tenant throughput < 60 k ops/s)
    └── → profile mutex in multi-tenant store → page @platform-perf
```

---

## Known Flaky Patterns

| Benchmark | Flake type | Mitigation |
|-----------|------------|------------|
| SOA-03 concurrent audit | Thread scheduling | Increase repetitions to 15; use CPU affinity if available |
| CFR-05 write storm | Thread scheduling variance | Run in isolation; pin threads if supported |
| MTI-07 concurrent writes | Mutex contention on shared VMs | Only valid on bare-metal with performance governor |

---

## Baseline References

| Wave | Baseline location | Key metric |
|------|------------------|------------|
| W8 | `benchmarks/wave8/baselines/` | Audit ≥ 90 000 ops/s, Read p99 ≤ 175 µs |
| W9 | `benchmarks/wave9/baselines/` | Audit ≥ 100 000 ops/s, Auth p99 ≤ 150 µs |
