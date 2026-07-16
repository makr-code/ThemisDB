# Wave 7 Release Gate Specification

**Wave:** 7 — Final Release Sign-off  
**Status:** Active  
**Version:** 1.0 (2026-07-16)

---

## Hard Gates (Release Blocking)

Hard gate failures **block merge and release**.  All must pass before a release candidate is tagged.

| Gate ID | Benchmark | Metric | Threshold | Direction |
|---------|-----------|--------|-----------|-----------|
| GATE-W7-01 | `W7A/RCS01_PointRead_p99_gate_200us` | p99 latency | ≤ 200 µs | lower_is_better |
| GATE-W7-02 | `W7A/RCS02_UpsertThroughput_gate_80k_ops_s` | throughput | ≥ 80 000 ops/s | higher_is_better |
| GATE-W7-03 | `W7A/RCS03_RangeScan_p99_gate_500us` | p99 latency | ≤ 500 µs | lower_is_better |
| GATE-W7-04 | `W7A/RCS04_BatchWrite_p99_gate_5ms` | p99 latency | ≤ 5 ms | lower_is_better |
| GATE-W7-05 | `W7D/GVO04_P99Read_HardGate_200us` | gate counter | = 1.0 | higher_is_better |
| GATE-W7-06 | `W7D/GVO05_WriteThroughput_HardGate_80k_ops_s` | gate counter | = 1.0 | higher_is_better |

---

## Soft Gates (Warning — Do Not Block)

Soft gate failures produce a warning and must be triaged but do not block release alone.

| Gate ID | Benchmark | Metric | Threshold | Direction |
|---------|-----------|--------|-----------|-----------|
| SGATE-W7-01 | `W7D/GVO01_ReadVariance_CV_gate_5pct` | CV% | ≤ 5% | lower_is_better |
| SGATE-W7-02 | `W7D/GVO02_WriteVariance_CV_gate_8pct` | CV% | ≤ 8% | lower_is_better |
| SGATE-W7-03 | `W7B/SOK01_SteadyRead_NoThroughputDrift` | drift% | ≤ 10% | lower_is_better |
| SGATE-W7-04 | `W7A/RCS06_AnnSearch_p99_gate_200us` | p99 latency | ≤ 200 µs | lower_is_better |
| SGATE-W7-05 | `W7D/GVO03_Determinism_KeySequenceCheck` | mismatches | = 0 | lower_is_better |

---

## Regression Tolerance Policy

| Parameter | Value |
|-----------|-------|
| Regression tolerance (all hard gates) | 5% vs prior release baseline |
| Minimum repetitions for valid measurement | 5 |
| Minimum sample size per repetition | 30 |
| Action on hard gate failure | Block merge · auto-create regression ticket |
| Action on soft gate failure | Warn · triage required before release |

---

## Baseline Freeze

All measurements must be taken with the following pinned environment:

```
compiler:        clang-17
compiler_flags:  -O3 -march=native -DNDEBUG
preset:          linux-release
hardware_profile: ci-runner-standard
os_image:        ubuntu-22.04
seed:            42
```

Deviations from this environment must be documented in the release notes.

---

## Evaluation Command

```bash
python3 benchmarks/wave7/report_variance_w7.py \
  --input  benchmarks/results/wave7/w7a.json \
  --manifest benchmarks/wave7/release_gate_manifest_w7.json \
  --output benchmarks/results/wave7/gate_report.json
```

Exit code `0` = all hard gates passed. Exit code `1` = release blocked.

---

## Ownership

| Gate Group | Owner Team |
|------------|-----------|
| GATE-W7-01 to W7-04 | Storage team |
| GATE-W7-05 to W7-06 | Perf team |
| SGATE-W7-01 to W7-02 | Perf team |
| SGATE-W7-03 | Storage team |
| SGATE-W7-04 | Search team |
| SGATE-W7-05 | Perf team |
