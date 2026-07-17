# Wave 7 Reproduction & Triage Guide

**Wave:** 7 — Final Release Sign-off  
**Audience:** On-call engineers, release managers, performance reviewers  
**Version:** 1.0 (2026-07-16)

---

## 1. Repro Checklist (Start Here)

Before raising a regression ticket, run through this checklist:

- [ ] Confirm you are running on the pinned baseline environment (see `release_gate_manifest_w7.json → baseline_freeze`).
- [ ] Confirm `TMPDIR` has ≥ 2 GB free space.
- [ ] Confirm no other disk-intensive processes are running concurrently.
- [ ] Confirm compiler and flags match: `clang-17 -O3 -march=native -DNDEBUG`.
- [ ] Run the failing benchmark **3 times** and compare results.  If all 3 runs fail the gate → confirmed regression.  If only 1/3 fails → likely noise (see §4).
- [ ] Check `gate_passed` counter in JSON output.  A value of `0.0` is definitive failure.
- [ ] Run W7-D GVO-03 determinism check to verify PRNG is stable.

---

## 2. Reproducing a Specific Benchmark

### Isolate a single benchmark

```bash
# Replace <FILTER> with the exact benchmark name, e.g. "W7A/RCS01_PointRead_p99_gate_200us"
./build/linux-release/benchmarks/wave7/bench_w7a_release_critical_signoff \
  --benchmark_filter="<FILTER>" \
  --benchmark_repetitions=7 \
  --benchmark_out=/tmp/repro_run.json \
  --benchmark_out_format=json
```

### Increase repetitions to confirm variance

```bash
--benchmark_repetitions=15
```

### Run with verbose output

```bash
--benchmark_report_aggregates_only=false \
--benchmark_display_aggregates_only=false
```

---

## 3. Triage Decision Tree

```
Gate failure reported
        │
        ├─► Is it a hard gate (GATE-W7-xx)?
        │         YES → Block release → File regression ticket → See §5
        │         NO  → Soft gate (SGATE-W7-xx) → Warn → See §4
        │
        └─► Confirm with 3 independent runs
                  │
                  ├─► All 3 fail → Confirmed regression → §5
                  └─► 1/3 fails  → Environment noise → §4
```

---

## 4. Noise / False-Positive Investigation

### Symptom: CV% too high (SGATE-W7-01 or SGATE-W7-02)

1. Identify noisy runs in `--benchmark_out` JSON: look for individual `real_time` values that are outliers.
2. Check for competing disk I/O: `iostat -x 1 10` during the benchmark run.
3. Check for CPU frequency scaling: `cpupower frequency-info`.
4. Increase repetitions to 15 and re-evaluate CV.
5. If CV remains high with 15 repetitions on an isolated machine → escalate to Perf team.

### Symptom: One run fails p99 gate, others pass

1. This is expected behaviour on a shared CI runner.
2. The gate policy requires the **mean p99 across all repetitions** to breach the threshold.
3. Single-repetition outliers are expected and tolerated within the 5% budget.

### Symptom: Determinism check (GVO-03) shows mismatches

1. This should never happen.  Verify that `KeyGenerator::Reset()` is called at the start of each iteration.
2. Check for unintentional shared state between benchmark fixtures.
3. If mismatches persist → file a critical bug against the benchmark framework.

---

## 5. Regression Ticket Template

When a hard gate fails, file a ticket with this structure:

```
Title: [PERF-REGRESSION] <Gate ID> failed — <benchmark name> — <date>

## Environment
- Branch: <branch>
- Commit: <sha>
- OS: ubuntu-22.04
- Compiler: clang-17 -O3 -march=native
- Hardware: <ci-runner type>

## Gate
- Gate ID: GATE-W7-xx
- Metric: <metric name>
- Threshold: <value>
- Measured: <value>
- Repetitions run: 5 (or more)

## Repro Steps
1. Build: cmake --preset linux-release --target <target>
2. Run: <exact command>
3. Evaluate: python3 report_variance_w7.py --input ...

## JSON Evidence
Attach: repro_run.json

## Root Cause Hypothesis
<initial hypothesis>

## Proposed Fix
<proposed fix or investigation step>

## Rollback Plan
Revert <commit sha> if no fix is available within 48 hours.
```

---

## 6. Saturation & Recovery Analysis (W7-C)

When `W7C/DFR03_SaturationPoint_Concurrency` shows throughput collapse at N threads:

1. Note the thread count at which ops/s first drops below 80% of single-thread performance.
2. Check whether the collapse is due to:
   - Lock contention (use `perf lock` or `pstack`)
   - I/O saturation (`iostat`, `blktrace`)
   - Memory bandwidth (`perf mem`)
3. Document the saturation point in the release residual risk register.

When `W7C/DFR07_RecoveryAfterCompaction` shows slow latency normalisation:

1. Compare `phase=0` (compaction) and `phase=1` (recovery) segment timings in the JSON.
2. Expected: recovery latency returns to ≤ 1.2× healthy baseline within 2 recovery segments.
3. If recovery takes > 4 segments → file a soft regression; document in residual risk register.

---

## 7. Residual Risk Register

| Risk | Severity | Mitigation | Owner | Target |
|------|----------|-----------|-------|--------|
| Vector/Graph benchmarks run against stub implementations | Low | Accept stub latency; re-run when real backend wired | Search team | Q4 2026 |
| W7-B soak only covers seconds-scale segments on CI | Medium | Validate with 30-min soak in staging before final release | Storage team | Q4 2026 |
| Fault injection (W7-C) uses in-process delays, not real I/O faults | Medium | Acceptable for characterisation; real fault injection deferred | Perf team | Q1 2027 |
| GVO-06 isolation uses thread yield, not CPU affinity | Low | Results informative; not used for hard gate decisions | Perf team | Q4 2026 |
| CI runner may share resources during nightly runs | Medium | Run sign-off benchmarks on dedicated runner before tagging release | DevOps | Per-release |

---

## 8. Monitoring Recommendations (Post-Release)

After the release ships, configure the following alerting:

| Metric | Alert Threshold | Alert Channel |
|--------|----------------|--------------|
| Single-key read p99 | > 250 µs (1.25× gate) | PagerDuty P2 |
| Write throughput | < 64 000 ops/s (0.8× gate) | PagerDuty P2 |
| Range scan p99 | > 750 µs (1.5× gate) | Slack #perf-alerts |
| Batch write p99 | > 8 ms (1.6× gate) | Slack #perf-alerts |
| Read CV (rolling 7-day) | > 8% | Slack #perf-alerts |

Re-run W7 benchmarks on the next release candidate and compare against the archived baseline.
