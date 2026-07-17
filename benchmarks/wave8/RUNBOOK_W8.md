# Wave 8 Post-Release Performance Runbook

**Wave:** 8 — Post-Release Performance Reliability  
**Scope:** Incident shielding, threshold hardening, drift detection, deterministic CI harness, operability.  
**Owners:** platform-perf@themisdb  
**Version:** 1.0 (2026-07-17)

---

## 1. Purpose

This runbook defines the standard operating procedure for:

1. Running the Wave 8 benchmark suite as part of post-release reliability checks.
2. Evaluating hardened and soft performance gates.
3. Detecting and triaging performance drift before it crosses hard gate limits.
4. Responding to gate failures with actionable escalation paths.
5. Archiving results for the release audit trail.

Wave 8 differs from prior waves in that it is **post-release** rather than pre-release.
Its primary objectives are:
- Shield known incident patterns from regressing silently.
- Detect gradual drift (creeping degradation) before it becomes an incident.
- Maintain a deterministic, low-noise CI benchmark environment.
- Provide on-call engineers with structured, actionable triage metrics.

---

## 2. Prerequisites

| Requirement        | Version / Notes                                      |
|--------------------|------------------------------------------------------|
| CMake preset       | `linux-release` (or `windows-release`)               |
| Compiler           | clang-17, `-O3 -march=native -DNDEBUG`               |
| Google Benchmark   | ≥ 1.8                                                |
| Python             | ≥ 3.10                                               |
| Disk               | ≥ 2 GB free in `$TMPDIR`                             |
| RAM                | ≥ 4 GB available                                     |

**CI runner baseline:** Ubuntu 22.04, ci-runner-standard image, single-socket NUMA.

---

## 3. Build

```bash
# Configure (linux-release preset)
cmake --preset linux-release

# Build Wave 8 targets only
cmake --build --preset linux-release --target \
    bench_w8a_incident_regression_shielding \
    bench_w8b_threshold_hardening_drift_detection \
    bench_w8c_deterministic_ci_harness \
    bench_w8d_operability_runbooks_ownership \
    --parallel 8
```

---

## 4. Execution Order

Create output directory first:

```bash
mkdir -p benchmarks/results/wave8
```

### Step 1 — W8-A: Incident Regression Shielding

```bash
./build/linux-release/benchmarks/wave8/bench_w8a_incident_regression_shielding \
  --benchmark_out=benchmarks/results/wave8/w8a.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=5 \
  --benchmark_filter="W8A/"
```

### Step 2 — W8-B: Threshold Hardening & Drift Detection (run first for gate evaluation)

```bash
./build/linux-release/benchmarks/wave8/bench_w8b_threshold_hardening_drift_detection \
  --benchmark_out=benchmarks/results/wave8/w8b.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=7 \
  --benchmark_filter="W8B/"
```

### Step 3 — W8-C: Deterministic CI Harness

```bash
./build/linux-release/benchmarks/wave8/bench_w8c_deterministic_ci_harness \
  --benchmark_out=benchmarks/results/wave8/w8c.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=7 \
  --benchmark_filter="W8C/"
```

### Step 4 — W8-D: Operability, Runbooks & Ownership

```bash
./build/linux-release/benchmarks/wave8/bench_w8d_operability_runbooks_ownership \
  --benchmark_out=benchmarks/results/wave8/w8d.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=5 \
  --benchmark_filter="W8D/"
```

---

## 5. Gate Evaluation

Run the variance/drift reporter against each output file:

```bash
# W8-B: hardened gates + drift detection (primary gate file)
python3 benchmarks/wave8/report_variance_w8.py \
    --input benchmarks/results/wave8/w8b.json \
    --manifest benchmarks/wave8/release_gate_manifest_w8.json \
    --baseline benchmarks/baselines/wave8/bench_w8b_baseline.json \
    --output benchmarks/results/wave8/w8b_report.json

# W8-A: incident shielding
python3 benchmarks/wave8/report_variance_w8.py \
    --input benchmarks/results/wave8/w8a.json \
    --manifest benchmarks/wave8/release_gate_manifest_w8.json \
    --output benchmarks/results/wave8/w8a_report.json

# W8-D: operability gates
python3 benchmarks/wave8/report_variance_w8.py \
    --input benchmarks/results/wave8/w8d.json \
    --manifest benchmarks/wave8/release_gate_manifest_w8.json \
    --output benchmarks/results/wave8/w8d_report.json
```

Exit code 0 = all hard gates passed.  
Exit code 1 = at least one hard gate failed → **CI blocks, file escalation ticket**.

---

## 6. Gate Response Matrix

| Gate ID        | Benchmark                         | Failure Response                           |
|---------------|------------------------------------|--------------------------------------------|
| GATE-W8-01    | THD01 read p99 > 175 µs            | 1. Profile read path. 2. Check block cache hit rate. 3. Escalate to platform-perf. |
| GATE-W8-02    | THD02 write throughput < 90k ops/s | 1. Check compaction backlog. 2. Review memtable config. 3. Escalate to platform-perf. |
| GATE-W8-03    | THD07 batch p99 > 4 ms             | 1. Check batch atomicity overhead. 2. Review write-buffer size. |
| GATE-W8-04    | IRS02 write storm < 60k ops/s      | 1. Check concurrent write-path locks. 2. Review memtable stall config. |
| GATE-W8-05    | ORP01 triage completeness < 1.0    | 1. Verify all required counters are emitted. 2. Check benchmark code changes. |
| GATE-W8-06    | ORP08 coverage < 80%               | 1. Add missing gates to release_gate_manifest_w8.json. 2. Review hot-path registry. |
| SGATE-W8-01   | THD03 read drift CV > 8%           | Investigate memory leak or background I/O growth. |
| SGATE-W8-02   | THD04 write drift CV > 10%         | Investigate compaction scheduling or WAL rotation. |
| SGATE-W8-03   | THD05 latency trend rising          | Compare with prior 7 days of CI results; file drift ticket if trend persists. |
| SGATE-W8-04   | DCH08 flake CV > 3%                | Re-run on a dedicated CI runner; check for background noise sources. |
| SGATE-W8-05   | ORP07 baseline stale               | Update frozen baseline JSON using the baseline renewal procedure (§8). |

---

## 7. Drift Detection Interpretation

### Rolling-window drift (THD-03/04)

The `read_drift_slope_ops_s_per_seg` and `write_drift_slope_ops_s_per_seg` counters
represent the linear regression slope across kDriftSegments=6 rolling windows.

- **Slope near 0**: stable throughput across segments → healthy.
- **Positive slope**: throughput increasing across windows → warm-up effect, expected for first run.
- **Negative slope with |slope| > 8% of first-segment throughput**: degrading throughput → investigate.

### Latency trend (THD-05)

`latency_trend_rising = 1.0` means the per-operation latency was rising across the measurement
window.  A single instance is a warning; three consecutive CI runs with a rising slope require
a drift investigation ticket.

---

## 8. Baseline Renewal Procedure

**When to renew the baseline:**
- After a hardware change on the CI runner.
- After a deliberate, approved performance improvement that changes the mean by > 5%.
- When the baseline age exceeds 30 days (ORP07 will warn).

**How to renew:**

```bash
# 1. Run the target benchmark with extra repetitions for a stable mean
./build/linux-release/benchmarks/wave8/bench_w8b_threshold_hardening_drift_detection \
    --benchmark_out=benchmarks/baselines/wave8/bench_w8b_baseline_candidate.json \
    --benchmark_out_format=json \
    --benchmark_repetitions=10

# 2. Review the candidate — confirm the new mean is plausible
python3 benchmarks/wave8/report_variance_w8.py \
    --input benchmarks/baselines/wave8/bench_w8b_baseline_candidate.json \
    --manifest benchmarks/wave8/release_gate_manifest_w8.json

# 3. If OK, promote to stable baseline (requires maintainer approval):
cp benchmarks/baselines/wave8/bench_w8b_baseline_candidate.json \
   benchmarks/baselines/wave8/bench_w8b_baseline.json
git add benchmarks/baselines/wave8/bench_w8b_baseline.json
git commit -m "perf(baselines/wave8): renew W8B baseline <YYYY-MM-DD>"
```

**Guardrail:** A baseline may only be loosened (threshold weakened) with an explicit
maintenance window approval from a senior platform-perf member.  The approval must be
recorded in the PR description and referenced in a comment in `release_gate_manifest_w8.json`.

---

## 9. Archival

After each release cycle:

```bash
# Archive all W8 results under a date-stamped directory
RELEASE_DATE=$(date +%Y-%m-%d)
mkdir -p benchmarks/results/archive/wave8/${RELEASE_DATE}
cp benchmarks/results/wave8/*.json \
   benchmarks/results/archive/wave8/${RELEASE_DATE}/
```

Archived results feed the **drift-trend** dashboard and are used to detect multi-release
degradation patterns that single-run comparisons miss.

---

## 10. Known Limitations

See `release_gate_manifest_w8.json` → `known_limitations` section.

Additional operational notes:

- **IRS-05 concurrent ingest+read**: the background reader thread runs until the benchmark
  main loop exits; on shared CI runners with CPU contention, occasional SGATE-W8-01/04 warnings
  are expected and do not require escalation.
- **THD-03/04 drift segments**: each iteration runs kDriftSegments=6 × kOpsPerSegment=3000 ops.
  On slow CI machines, a single repetition can take > 60 s.  The `--benchmark_min_time=1`
  flag can be omitted for drift benchmarks to let Google Benchmark control iteration count.
- **ORP-07 baseline age**: the benchmark currently uses a hardcoded 14-day simulated age.
  In a future sprint, this should be replaced with a live read from the baseline JSON's
  `created_at` field.

---

## 11. Escalation Path

| Severity | Condition                              | Owner              | SLA        |
|----------|----------------------------------------|--------------------|------------|
| P1       | Any GATE-W8-0N blocking failure        | platform-perf lead | 4 h        |
| P2       | ≥ 3 consecutive SGATE-W8 warnings      | platform-perf      | 1 business day |
| P3       | Rising latency trend (THD-05) 3+ runs  | platform-perf      | 3 business days |
| P4       | Baseline age > 30 days                 | assigned maintainer| Next sprint |
