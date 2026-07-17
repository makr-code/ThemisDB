# Wave 8 Benchmark Runbook

<!-- ThemisDB | RUNBOOK_W8.md | Version: 0.0.1 -->

## Overview

This runbook covers the Wave 8 benchmark suite for ThemisDB. Wave 8 tightens
performance thresholds established in Wave 7 and introduces operability and
triage-coverage gates.

---

## Quick Reference

| Sub-wave | Binary | Gate file |
|----------|--------|-----------|
| W8-A Incident Regression | `bench_w8a_incident_regression_shielding` | `GATE-W8-01`, `GATE-W8-02` |
| W8-B Threshold Hardening | `bench_w8b_threshold_hardening_drift_detection` | `GATE-W8-03`, `GATE-W8-04` |
| W8-C Deterministic CI    | `bench_w8c_deterministic_ci_harness` | (soft gates only) |
| W8-D Operability         | `bench_w8d_operability_runbooks_ownership` | `GATE-W8-05`, `GATE-W8-06` |

---

## Running the Full Suite

```bash
# Build (from repository root)
cmake --preset linux-release
cmake --build --preset linux-release --parallel 16

# Run all W8 benchmarks with JSON output
cd build/linux-release

./bench_w8a_incident_regression_shielding \
    --benchmark_format=json \
    --benchmark_out=w8a_results.json \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true

./bench_w8b_threshold_hardening_drift_detection \
    --benchmark_format=json \
    --benchmark_out=w8b_results.json \
    --benchmark_repetitions=5

./bench_w8c_deterministic_ci_harness \
    --benchmark_format=json \
    --benchmark_out=w8c_results.json \
    --benchmark_repetitions=7

./bench_w8d_operability_runbooks_ownership \
    --benchmark_format=json \
    --benchmark_out=w8d_results.json

# Evaluate gates
python3 benchmarks/wave8/report_variance_w8.py --input w8a_results.json
python3 benchmarks/wave8/report_variance_w8.py --input w8b_results.json
```

---

## Hard Gates

| Gate ID | Benchmark | Metric | Threshold | Direction |
|---------|-----------|--------|-----------|-----------|
| GATE-W8-01 | IRS-07 Large-value read | p99 latency | ≤ 175 µs | lower_is_better |
| GATE-W8-02 | IRS-08 Audit throughput | ops/s | ≥ 90 000 | higher_is_better |
| GATE-W8-03 | THD-07 Read threshold | p99 latency | ≤ 175 µs | lower_is_better |
| GATE-W8-04 | THD-08 Write-storm | ops/s | ≥ 60 000 | higher_is_better |
| GATE-W8-05 | ORP-07 Triage completeness | fraction | = 1.0 | higher_is_better |
| GATE-W8-06 | ORP-08 Coverage | % | ≥ 80% | higher_is_better |

---

## Failure Investigation

### GATE-W8-01 / GATE-W8-03 (p99 > 175 µs)

1. Re-run with `--benchmark_repetitions=15` to reduce variance.
2. Check background compaction activity during the run.
3. Compare against W7 baseline (p99 ≤ 200 µs) — if W7 also fails, it is a
   storage regression, not a threshold-tightening regression.
4. Run with TSAN to rule out lock contention (`-fsanitize=thread`).
5. Escalate to @storage-team if regression persists across 3 consecutive runs.

### GATE-W8-02 (audit throughput < 90 000 ops/s)

1. Verify the mutex critical section in `AuditLogCapture::Record()` has not grown.
2. Check if `lock_guard` was accidentally replaced with a higher-overhead lock.
3. Run single-threaded first to establish baseline; then add threads.

### GATE-W8-04 (write-storm < 60 000 ops/s)

1. Run `THD08_WriteStorm_ThroughputGate_60k` with `--benchmark_filter=THD08`
   and reduce thread count to 1 to isolate per-thread cost.
2. Profile with `perf record` to identify compaction or WAL flush bottleneck.

### GATE-W8-05 (triage_completeness < 1.0)

1. Check that all four W8 benchmark binaries built and ran successfully.
2. Verify each binary reports `gate_passed` counter (grep JSON output).
3. Add missing `state.counters["gate_passed"]` to any benchmark that omits it.

### GATE-W8-06 (operability coverage < 80%)

1. Add missing runbook entries to `RUNBOOK_W8.md` for any uncovered gate ID.
2. Re-run `ORP08_OperabilitySelfCheck_OverallScorePassAll` after updating the
   registry in `bench_w8d_operability_runbooks_ownership.cpp`.

---

## Variance Interpretation

| CV range | Assessment | Action |
|----------|------------|--------|
| < 5% | Excellent | No action required |
| 5–8% | Acceptable | Monitor trend |
| 8–15% | Noisy | Investigate environment (CPU governor, disk I/O) |
| > 15% | Unacceptable | Block release; root-cause before re-run |

---

## Contacts

| Area | Owner |
|------|-------|
| Storage performance | @storage-team |
| CI harness | @ci-team |
| Audit logging | @observability |
| Release sign-off | @platform-perf |
