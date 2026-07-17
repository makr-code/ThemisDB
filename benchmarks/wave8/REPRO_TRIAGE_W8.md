# Wave 8 Repro & Triage Guide

**Wave:** 8 — Post-Release Performance Reliability  
**Scope:** Minimal reproduction steps, escalation paths, and owner mapping for W8 benchmark failures.  
**Owners:** platform-perf@themisdb  
**Version:** 1.0 (2026-07-17)

---

## 1. Purpose

This guide enables any on-call engineer to:
1. Reproduce a W8 benchmark gate failure with minimal setup.
2. Isolate the affected component using the structured triage checklist.
3. Route the issue to the correct owner with all required diagnostic context.

---

## 2. Minimal Reproduction (All Scenarios)

### 2.1 Environment setup

```bash
# Use the linux-release preset (same as CI)
cmake --preset linux-release
cmake --build --preset linux-release --target bench_w8a_incident_regression_shielding \
    bench_w8b_threshold_hardening_drift_detection \
    bench_w8c_deterministic_ci_harness \
    bench_w8d_operability_runbooks_ownership \
    --parallel 8

mkdir -p /tmp/w8_repro_results
```

### 2.2 Run the failing benchmark in isolation

Replace `<BENCH_BINARY>` and `<FILTER>` with the failing target:

```bash
./build/linux-release/benchmarks/wave8/<BENCH_BINARY> \
    --benchmark_out=/tmp/w8_repro_results/repro.json \
    --benchmark_out_format=json \
    --benchmark_repetitions=10 \
    --benchmark_filter="<FILTER>" \
    --benchmark_report_aggregates_only=false
```

### 2.3 Evaluate gates

```bash
python3 benchmarks/wave8/report_variance_w8.py \
    --input /tmp/w8_repro_results/repro.json \
    --manifest benchmarks/wave8/release_gate_manifest_w8.json \
    --output /tmp/w8_repro_results/repro_report.json
```

---

## 3. Scenario-Specific Triage Checklists

### GATE-W8-01: Read p99 > 175 µs (THD-01)

| Step | Check                                            | Command                                    |
|------|--------------------------------------------------|--------------------------------------------|
| 1    | Reproduce with 10 repetitions                    | `--benchmark_filter="W8B/THD01.*" --benchmark_repetitions=10` |
| 2    | Check block cache size config                    | Inspect `DefaultConfig()` block_cache_size_mb |
| 3    | Compare against W7 baseline                      | `--baseline benchmarks/baselines/wave7/bench_w7a_baseline.json` |
| 4    | Isolate to disk vs CPU                           | Re-run on tmpfs: `export TMPDIR=/dev/shm && ./bench_w8b...` |
| 5    | File ticket if p99 > 190 µs on 3 consecutive runs | Owner: platform-perf |

### GATE-W8-02: Write throughput < 90k ops/s (THD-02)

| Step | Check                                            |
|------|--------------------------------------------------|
| 1    | Reproduce with `--benchmark_filter="W8B/THD02.*"` |
| 2    | Check for compaction in progress (`rocksdb.compaction-pending` stat) |
| 3    | Check memtable_size_mb (should be 128 for this bench) |
| 4    | Compare against W7 baseline (W7 gate was 80k) |
| 5    | If > 5% regression vs W7 baseline: escalate to storage-team |

### GATE-W8-03: Batch write p99 > 4 ms (THD-07)

| Step | Check                                            |
|------|--------------------------------------------------|
| 1    | Reproduce: `--benchmark_filter="W8B/THD07.*" --benchmark_repetitions=10` |
| 2    | Check batch size (should be 500 records) |
| 3    | Verify `putBatch()` uses a RocksDB WriteBatch (not N single puts) |
| 4    | Owner: platform-perf |

### GATE-W8-04: Write storm throughput < 60k ops/s (IRS-02)

| Step | Check                                            |
|------|--------------------------------------------------|
| 1    | Reproduce: `--benchmark_filter="W8A/IRS02.*"` |
| 2    | Reduce writer threads from 4 to 1 and compare — if 1-thread is fast, lock contention is likely |
| 3    | Check `allow_concurrent_memtable_write` is true |
| 4    | Owner: storage-team |

### GATE-W8-05: Triage completeness < 1.0 (ORP-01)

| Step | Check                                            |
|------|--------------------------------------------------|
| 1    | Reproduce: `--benchmark_filter="W8D/ORP01.*"` |
| 2    | Inspect raw JSON output for missing counter fields |
| 3    | Verify `mean_latency_us`, `p99_latency_us`, `throughput_ops_s`, `gate_passed` are all > 0 |
| 4    | Owner: platform-perf |

### GATE-W8-06: Guardrail coverage < 80% (ORP-08)

| Step | Check                                            |
|------|--------------------------------------------------|
| 1    | Run: `--benchmark_filter="W8D/ORP08.*"` |
| 2    | Check `guardrail_covered_paths` and `guardrail_total_paths` in output |
| 3    | Add missing gates to `release_gate_manifest_w8.json` |
| 4    | Update the `HotPathRegistry()` in `bench_w8d_operability_runbooks_ownership.cpp` |
| 5    | Owner: platform-perf |

---

## 4. Drift Triage

### SGATE-W8-01/02: High drift CV (THD-03/04)

```bash
# Run drift detection with 10 repetitions for a stable CV estimate
./build/linux-release/benchmarks/wave8/bench_w8b_threshold_hardening_drift_detection \
    --benchmark_out=/tmp/w8_repro_results/drift.json \
    --benchmark_out_format=json \
    --benchmark_repetitions=10 \
    --benchmark_filter="W8B/THD0[34].*"

python3 benchmarks/wave8/report_variance_w8.py \
    --input /tmp/w8_repro_results/drift.json \
    --manifest benchmarks/wave8/release_gate_manifest_w8.json
```

**Triage steps:**
1. If CV > 8% on read (THD-03): check for background I/O contention (`iostat -x 1 10`).
2. If CV > 10% on write (THD-04): check compaction scheduling (`ROCKSDB_COMPACTION_PRIORITY`).
3. If CV is stable across 3 runs but consistently high: file a drift investigation ticket.

### SGATE-W8-03: Rising latency trend (THD-05)

A single rising trend is a warning.  Escalate if:
- Trend rises in ≥ 3 consecutive CI runs, OR
- The slope exceeds 1 µs/segment.

```bash
# Extract latency_trend_slope from report JSON
python3 -c "
import json, sys
r = json.load(open('/tmp/w8_repro_results/drift_report.json'))
for b in r.get('drift_results', []):
    print(b['benchmark'], 'slope=', b['slope'], 'alert=', b['alert'])
"
```

---

## 5. Owner Mapping

| Benchmark Family | Primary Owner       | Escalation Owner    |
|-----------------|---------------------|---------------------|
| W8A (IRS-*)     | platform-perf       | storage-team        |
| W8B (THD-*)     | platform-perf       | platform-perf lead  |
| W8C (DCH-*)     | platform-perf       | ci-infra            |
| W8D (ORP-*)     | platform-perf       | platform-perf lead  |
| Write-storm (IRS-02) | storage-team   | platform-perf lead  |
| Vector/Graph (if added) | ml-platform | platform-perf      |

---

## 6. Threshold Change Policy

Thresholds in `release_gate_manifest_w8.json` may only be changed under these conditions:

| Change Type                | Required Approval            | Process                           |
|---------------------------|------------------------------|-----------------------------------|
| Tighten (stricter)         | platform-perf team           | PR with `perf(gates)` prefix      |
| Loosen (weaker) ≤ 10%     | platform-perf lead           | PR + maintenance window ticket    |
| Loosen > 10%               | Engineering director         | RFC process required              |
| Baseline renewal           | assigned maintainer          | `perf(baselines/wave8)` PR        |

All threshold changes must include:
- Measured evidence (benchmark output JSON attached to PR).
- Rationale for the change.
- Impact assessment on downstream SLAs.

---

## 7. Guardrail Governance

The following rules govern the W8 guardrail set:

1. **No new unprotected hot paths**: any new performance-critical code path added to the
   storage, query, or index layer must have a corresponding gate added within the same sprint.
2. **Drift signals are non-optional**: THD-03/04/05 must run in every CI benchmark job.
3. **Triage counters are mandatory**: ORP-01 (triage completeness) must pass before a release
   is considered performance-qualified.
4. **Baseline age limit**: baselines older than 30 days trigger ORP-07 soft-gate warning.
   Baselines older than 60 days must be renewed before the next release.

---

## 8. Post-Incident Checklist

After a production performance incident related to a W8-covered path:

- [ ] Verify the incident pattern is covered by an IRS-* benchmark.
- [ ] If not covered: add a new IRS scenario and update `WAVE8_BENCHMARK_COVERAGE.md`.
- [ ] Check whether the incident would have been caught by W8 drift detection (THD-03/04/05).
- [ ] Update the escalation SLA table in this document if the incident response time exceeded the target.
- [ ] File a follow-up ticket if a new threshold tightening is warranted.
- [ ] Update the `known_limitations` section in `release_gate_manifest_w8.json` if the incident
      revealed a gap in the current fault model.
