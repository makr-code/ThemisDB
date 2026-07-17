# Wave 7 Benchmark Coverage

**Wave:** 7 — Final Release Performance Sign-off, Endurance/Degradation/Recovery & Governance  
**Branch:** develop  
**Status:** Complete  
**Date:** 2026-07-16

---

## Overview

Wave 7 delivers the final release-ready benchmark layer for ThemisDB, covering four orthogonal
dimensions required for a credible release sign-off:

| PR | Benchmark File | Purpose |
|----|---------------|---------|
| B7-A | `bench_w7a_release_critical_signoff.cpp` | Release-critical workload sign-off (hard gates) |
| B7-B | `bench_w7b_endurance_soak.cpp` | Endurance, soak, and peak transition validation |
| B7-C | `bench_w7c_degradation_fault_recovery.cpp` | Degradation, fault & recovery characterisation |
| B7-D | `bench_w7d_guardrails_variance_operability.cpp` | Final guardrails, CV zeroing, operability counters |

---

## B7-A: Release-Critical Workload Sign-Off

### Scenarios

| ID | Name | Gate |
|----|------|------|
| RCS-01 | Point read | p99 ≤ 200 µs (GATE-W7-01) |
| RCS-02 | Upsert throughput | ≥ 80 000 ops/s (GATE-W7-02) |
| RCS-03 | Range scan | p99 ≤ 500 µs (GATE-W7-03) |
| RCS-04 | Batch write (500 records) | p99 ≤ 5 ms (GATE-W7-04) |
| RCS-05 | Mixed OLTP (60% r / 40% w) | Baseline tracking |
| RCS-06 | Vector ANN top-10 | p99 ≤ 200 µs (SGATE-W7-04) |
| RCS-07 | Graph neighbourhood traversal | p99 ≤ 500 µs |
| RCS-08 | Secondary index lookup | p99 ≤ 200 µs |

### Measurement Methodology

- Dataset: 50 000 records pre-loaded (B7-A), 100 000 records (B7-B).
- Warmup: 500 operations before measurement window.
- Repetitions: 5 (B7-A, B7-C, B7-D), outer-segments (B7-B).
- PRNG seed: `kW7CanonicalSeed = 42`.
- Timing: `UseRealTime()` to include I/O wait.
- DB path: OS temp directory with nanosecond timestamp suffix for isolation.

---

## B7-B: Endurance, Soak & Peak Transition

### Scenarios

| ID | Name | Signal |
|----|------|--------|
| SOK-01 | Steady read (6 segments) | Throughput drift ≤ 10% |
| SOK-02 | Steady write (6 segments) | Throughput drift ≤ 10% |
| SOK-03 | Burst → Peak read spike | Tail latency build-up |
| SOK-04 | Peak → Recovery | Latency normalisation |
| SOK-05 | Write-heavy soak (90% write) | Memory accumulation guard |
| SOK-06 | Mixed OLTP soak (60/40) | Combined stability |
| SOK-07 | Concurrent reader soak (4 threads) | Thread-safety under load |
| SOK-08 | Large-value soak (16 KB) | Bandwidth saturation |

---

## B7-C: Degradation, Fault & Recovery

### Scenarios

| ID | Name | Fault Model |
|----|------|------------|
| DFR-01 | Latency injection reads | 10% of reads + 50 µs delay |
| DFR-02 | Partial write failure | 5% write skip (simulated error) |
| DFR-03 | Saturation point (1/2/4/8 threads) | Increasing concurrency |
| DFR-04 | Backpressure write stall | 15% writes + 300 µs delay |
| DFR-05 | Cold-start read latency | Cold block cache (1 MB) |
| DFR-06 | Half-thread resource reduction | 2-worker constraint |
| DFR-07 | Recovery after compaction | 3 heavy-write + 5 recovery segments |
| DFR-08 | Degraded mixed workload | Combined fault pressure |

---

## B7-D: Guardrails, Variance & Operability

### Scenarios

| ID | Name | Gate |
|----|------|------|
| GVO-01 | Read variance CV | CV ≤ 5% (SGATE-W7-01) |
| GVO-02 | Write variance CV | CV ≤ 8% (SGATE-W7-02) |
| GVO-03 | Determinism check | mismatches = 0 (SGATE-W7-05) |
| GVO-04 | p99 read gate assertion | gate_passed = 1.0 (GATE-W7-05) |
| GVO-05 | Write throughput gate assertion | gate_passed = 1.0 (GATE-W7-06) |
| GVO-06 | Isolated read latency | CV measurement with yield isolation |
| GVO-07 | Regression delta baseline | mean/stddev/p50/p95/p99 snapshot |
| GVO-08 | Operability counters | Full structured counter set |

---

## Governance Artifacts

| Artifact | Purpose |
|---------|---------|
| `release_gate_manifest_w7.json` | Machine-readable gate definitions |
| `report_variance_w7.py` | Gate evaluation + variance reporting script |
| `RELEASE_GATE_W7.md` | Human-readable gate specification |
| `RUNBOOK_W7.md` | Execution and response runbook |
| `REPRO_TRIAGE_W7.md` | Reproduction and triage guide |
| `../ci_wave7_final_release_experiments.json` | CI experiment configuration |
| `CMakeLists.txt` | CMake registration for all W7 targets |

---

## Release Sign-off Acceptance Criteria

- [ ] All 6 hard gates pass on the release-pinned environment.
- [ ] All 5 soft gates triaged (pass or documented with accepted risk).
- [ ] W7-B soak shows ≤ 10% throughput drift across steady segments.
- [ ] W7-C fault scenarios produce diagnosable output with no uncharacterised failures.
- [ ] GVO-03 determinism check passes with 0 mismatches.
- [ ] Gate report archived at `benchmarks/results/wave7-release-<tag>/`.
