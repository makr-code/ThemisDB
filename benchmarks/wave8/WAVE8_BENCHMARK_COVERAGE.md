# Wave 8 Benchmark Coverage

<!-- ThemisDB | WAVE8_BENCHMARK_COVERAGE.md | Version: 0.0.1 -->

## Overview

Wave 8 introduces four benchmark sub-suites that harden and extend the Wave 7
performance gates.  The key changes from W7 are:

- **Read p99 threshold tightened**: 175 µs (down from 200 µs)
- **Write throughput gate raised**: 90 000 ops/s for audit log (new gate)
- **Write-storm gate added**: 60 000 ops/s under 8-thread peak load
- **Operability gates added**: triage_completeness = 1.0 and coverage ≥ 80%

---

## Sub-Wave Coverage Table

| Sub-wave | File | Tests | Hard Gates | Soft Gates |
|----------|------|-------|------------|------------|
| W8-A | `bench_w8a_incident_regression_shielding.cpp` | IRS-01..IRS-08 | GATE-W8-01, GATE-W8-02 | — |
| W8-B | `bench_w8b_threshold_hardening_drift_detection.cpp` | THD-01..THD-08 | GATE-W8-03, GATE-W8-04 | SGATE-W8-01, SGATE-W8-02 |
| W8-C | `bench_w8c_deterministic_ci_harness.cpp` | DCH-01..DCH-08 | — | SGATE-W8-03 |
| W8-D | `bench_w8d_operability_runbooks_ownership.cpp` | ORP-01..ORP-08 | GATE-W8-05, GATE-W8-06 | — |

---

## W8-A Coverage — Incident Regression Shielding

| ID | Scenario | Gate | Threshold |
|----|----------|------|-----------|
| IRS-01 | Concurrent ingest/delete throughput | advisory | — |
| IRS-02 | WAL append + commit latency | advisory | — |
| IRS-03 | Retry back-off bounded execution | advisory | — |
| IRS-04 | Batch rollback latency (100 records) | advisory | — |
| IRS-05 | WAL replay throughput (10 k entries) | advisory | — |
| IRS-06 | Double-delete no-op latency | advisory | — |
| IRS-07 | Large-value read p99 (512 KiB) | **GATE-W8-01** | p99 ≤ 175 µs |
| IRS-08 | Concurrent audit write throughput | **GATE-W8-02** | ≥ 90 000 ops/s |

---

## W8-B Coverage — Threshold Hardening & Drift Detection

| ID | Scenario | Gate | Threshold |
|----|----------|------|-----------|
| THD-01 | Point-read baseline re-measurement | advisory | — |
| THD-02 | Upsert baseline re-measurement | advisory | — |
| THD-03 | Range-scan baseline re-measurement | advisory | — |
| THD-04 | Batch-write baseline re-measurement | advisory | — |
| THD-05 | Memory drift detection | **SGATE-W8-02** | drift < 5% |
| THD-06 | Read p99 with concurrent write load | advisory | — |
| THD-07 | Threshold tightening gate (175 µs) | **GATE-W8-03** | p99 ≤ 175 µs |
| THD-08 | Write-storm ceiling (8 threads) | **GATE-W8-04** | ≥ 60 000 ops/s |

---

## W8-C Coverage — Deterministic CI Harness

| ID | Scenario | Gate | Threshold |
|----|----------|------|-----------|
| DCH-01 | Seed reproducibility | **SGATE-W8-03** | mismatches = 0 |
| DCH-02 | CV < 5% for synthetic read workload | **SGATE-W8-01** | CV < 5% |
| DCH-03 | Deterministic key generation cross-run | advisory | — |
| DCH-04 | Idle-loop overhead baseline | advisory | — |
| DCH-05 | Clock resolution (steady_clock) | advisory | — |
| DCH-06 | Thread-count independence | advisory | — |
| DCH-07 | Warmup effectiveness | advisory | — |
| DCH-08 | Self-check pass/fail counter | advisory | — |

---

## W8-D Coverage — Operability, Runbooks & Ownership

| ID | Scenario | Gate | Threshold |
|----|----------|------|-----------|
| ORP-01 | Triage coverage — all gate IDs have runbook entries | advisory | — |
| ORP-02 | Runbook lookup latency simulation | advisory | — |
| ORP-03 | Ownership counter presence | advisory | — |
| ORP-04 | Gate manifest completeness | advisory | — |
| ORP-05 | Incident replay (IRS-07 under triage) | advisory | — |
| ORP-06 | Alert-threshold detection | advisory | — |
| ORP-07 | CI coverage gauge (triage_completeness) | **GATE-W8-05** | = 1.0 |
| ORP-08 | Operability self-check (coverage %) | **GATE-W8-06** | ≥ 80% |

---

## Threshold Comparison: W7 vs W8

| Metric | W7 Threshold | W8 Threshold | Change |
|--------|-------------|-------------|--------|
| Read p99 | 200 µs | 175 µs | −12.5% (tighter) |
| Write throughput | 80 000 ops/s | N/A (IRS-08 = 90 k, different path) | new gate |
| Range scan p99 | 500 µs | (baseline re-measured, not gated) | advisory only |
| Batch write p99 | 5 ms | (baseline re-measured, not gated) | advisory only |
| Write-storm | N/A | 60 000 ops/s (8 threads) | new gate |
| Triage completeness | N/A | 1.0 | new gate |
| Runbook coverage | N/A | ≥ 80% | new gate |

---

## Files

| File | Purpose |
|------|---------|
| `bench_w8a_incident_regression_shielding.cpp` | IRS benchmark suite |
| `bench_w8b_threshold_hardening_drift_detection.cpp` | THD benchmark suite |
| `bench_w8c_deterministic_ci_harness.cpp` | DCH benchmark suite |
| `bench_w8d_operability_runbooks_ownership.cpp` | ORP benchmark suite |
| `CMakeLists.txt` | Build registration |
| `release_gate_manifest_w8.json` | Gate definitions and thresholds |
| `report_variance_w8.py` | Variance and gate evaluation tool |
| `RUNBOOK_W8.md` | Operational runbook |
| `REPRO_TRIAGE_W8.md` | Reproduction and triage guide |
| `WAVE8_BENCHMARK_COVERAGE.md` | This file |
