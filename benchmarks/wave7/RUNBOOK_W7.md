# Wave 7 Release Performance Runbook

**Wave:** 7 — Final Release Sign-off  
**Scope:** Reproducible execution, gate evaluation, and operational response for W7 benchmarks.  
**Owners:** platform-perf@themisdb  
**Version:** 1.0 (2026-07-16)

---

## 1. Purpose

This runbook defines the standard operating procedure for:

1. Running the Wave 7 benchmark suite before release sign-off.
2. Evaluating hard and soft performance gates.
3. Responding to gate failures.
4. Archiving results for the release audit trail.

---

## 2. Prerequisites

| Requirement | Version / Notes |
|-------------|-----------------|
| CMake preset | `linux-release` (or `windows-release`) |
| Compiler | clang-17, `-O3 -march=native -DNDEBUG` |
| Google Benchmark | ≥ 1.8 |
| Python | ≥ 3.10 |
| Disk | ≥ 2 GB free in `$TMPDIR` |
| RAM | ≥ 4 GB available |

**CI runner baseline:**  Ubuntu 22.04, ci-runner-standard image, single-socket NUMA.

---

## 3. Build

```bash
# Configure
cmake --preset linux-release

# Build wave7 benchmarks only
cmake --build --preset linux-release --parallel 8 \
  --target bench_w7a_release_critical_signoff \
           bench_w7b_endurance_soak \
           bench_w7c_degradation_fault_recovery \
           bench_w7d_guardrails_variance_operability
```

---

## 4. Execution Order

Always run in this order to capture steady-state first:

### Step 1 — W7-A: Release Sign-off (must pass for release)

```bash
mkdir -p benchmarks/results/wave7

./build/linux-release/benchmarks/wave7/bench_w7a_release_critical_signoff \
  --benchmark_out=benchmarks/results/wave7/w7a.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=5 \
  --benchmark_filter="W7A/"
```

### Step 2 — W7-D: Guardrails & Variance (must pass for release)

```bash
./build/linux-release/benchmarks/wave7/bench_w7d_guardrails_variance_operability \
  --benchmark_out=benchmarks/results/wave7/w7d.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=7 \
  --benchmark_filter="W7D/"
```

### Step 3 — W7-B: Endurance/Soak (stability evidence, no hard block)

```bash
./build/linux-release/benchmarks/wave7/bench_w7b_endurance_soak \
  --benchmark_out=benchmarks/results/wave7/w7b.json \
  --benchmark_out_format=json \
  --benchmark_filter="W7B/"
```

### Step 4 — W7-C: Degradation/Fault/Recovery (characterisation, no hard block)

```bash
./build/linux-release/benchmarks/wave7/bench_w7c_degradation_fault_recovery \
  --benchmark_out=benchmarks/results/wave7/w7c.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=5 \
  --benchmark_filter="W7C/"
```

---

## 5. Gate Evaluation

```bash
# Evaluate all hard and soft gates
python3 benchmarks/wave7/report_variance_w7.py \
  --input  benchmarks/results/wave7/w7a.json \
  --manifest benchmarks/wave7/release_gate_manifest_w7.json \
  --output benchmarks/results/wave7/gate_report.json

# Repeat for w7d to validate variance gates
python3 benchmarks/wave7/report_variance_w7.py \
  --input  benchmarks/results/wave7/w7d.json \
  --manifest benchmarks/wave7/release_gate_manifest_w7.json \
  --output benchmarks/results/wave7/gate_report_w7d.json
```

**Exit code 0** = all hard gates passed.  
**Exit code 1** = one or more hard gates failed → release is blocked.

---

## 6. Gate Response Matrix

| Gate ID | Failure Action | Owner |
|---------|---------------|-------|
| GATE-W7-01 (p99 read ≤ 200 µs) | Block release · create regression ticket · escalate to storage lead | Storage team |
| GATE-W7-02 (write ≥ 80k ops/s) | Block release · check compaction settings · profile write path | Storage team |
| GATE-W7-03 (range scan ≤ 500 µs) | Block release · check index structure · re-run 3× to confirm | Query team |
| GATE-W7-04 (batch write ≤ 5 ms) | Block release · check WAL + batch size · measure memtable pressure | Storage team |
| GATE-W7-05/06 (gate self-check) | Block release · verify report_variance_w7.py logic · re-run | Perf team |
| SGATE-W7-01/02 (CV thresholds) | Warn · investigate environment noise · increase repetitions to 10 | Perf team |
| SGATE-W7-03 (drift ≤ 10%) | Warn · inspect compaction logs · re-run soak with longer segments | Storage team |
| SGATE-W7-04 (ANN p99 ≤ 200 µs) | Warn · check vector index configuration | Search team |
| SGATE-W7-05 (determinism) | Warn · review KeyGenerator seed logic | Perf team |

---

## 7. Archival

After a successful release run, archive results:

```bash
# Tag results with release version
cp -r benchmarks/results/wave7 benchmarks/results/wave7-release-$(git describe --tags --abbrev=0)

# Commit baseline snapshot
git add benchmarks/results/wave7-release-*
git commit -m "perf(wave7): archive W7 release sign-off results [skip ci]"
```

---

## 8. Known Limitations

See `release_gate_manifest_w7.json` → `known_limitations` section.

Additional notes:

- **W7-B soak benchmarks** use `Iterations()` (outer segments), not per-op iterations.
  The `items_per_second` counter is set via `SetItemsProcessed()` and reflects total ops.
- **W7-C fault injection** uses in-process delay simulation.  Results characterise
  the effect of I/O jitter but do not require external fault injection tooling.
- **Vector / Graph benchmarks** (RCS-06/07) depend on `VectorIndex` / `GraphIndex`
  implementations.  If those backends are stubs, results reflect stub latency.

---

## 9. Post-Release Follow-Ups

| Item | Target |
|------|--------|
| Re-baseline all W7 gates against the released binary | Q3 2026 |
| Add CPU-affinity pinning for GVO-06 isolation test | Q4 2026 |
| Extend W7-B soak to 30-minute real-time window in staging environment | Q4 2026 |
| Wire W7 gates to GitHub Actions workflow once CI infra is available | Q4 2026 |
