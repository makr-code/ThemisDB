# Wave 9 Benchmark Runbook

<!-- ThemisDB | RUNBOOK_W9.md | Version: 0.0.1 -->

## Overview

This runbook covers the Wave 9 benchmark suite for ThemisDB. Wave 9 introduces
security-hardening, SLA compliance, chaos fault-tolerance, and multi-tenant
isolation performance gates.

---

## Quick Reference

| Sub-wave | Binary | Gate file |
|----------|--------|-----------|
| W9-A Security Overhead | `bench_w9a_security_overhead_audit` | `GATE-W9-01`, `GATE-W9-02` |
| W9-B SLA Measurement   | `bench_w9b_sla_measurement_compliance` | `GATE-W9-04` |
| W9-C Chaos Recovery    | `bench_w9c_chaos_fault_recovery` | `GATE-W9-03` |
| W9-D Multi-Tenant      | `bench_w9d_multi_tenant_isolation` | `GATE-W9-05`, `GATE-W9-06` |

---

## Running the Full Suite

```bash
# Build (from repository root)
cmake --preset linux-release
cmake --build --preset linux-release --parallel 16

# Run all W9 benchmarks with JSON output
cd build/linux-release

./bench_w9a_security_overhead_audit \
    --benchmark_format=json \
    --benchmark_out=w9a_results.json \
    --benchmark_repetitions=5 \
    --benchmark_report_aggregates_only=true

./bench_w9b_sla_measurement_compliance \
    --benchmark_format=json \
    --benchmark_out=w9b_results.json \
    --benchmark_repetitions=5

./bench_w9c_chaos_fault_recovery \
    --benchmark_format=json \
    --benchmark_out=w9c_results.json \
    --benchmark_repetitions=5

./bench_w9d_multi_tenant_isolation \
    --benchmark_format=json \
    --benchmark_out=w9d_results.json \
    --benchmark_repetitions=5

# Evaluate gates
python3 benchmarks/wave9/report_variance_w9.py --input w9a_results.json
python3 benchmarks/wave9/report_variance_w9.py --input w9b_results.json
python3 benchmarks/wave9/report_variance_w9.py --input w9c_results.json
python3 benchmarks/wave9/report_variance_w9.py --input w9d_results.json
```

---

## Hard Gates

| Gate ID | Benchmark | Metric | Threshold | Direction |
|---------|-----------|--------|-----------|-----------|
| GATE-W9-01 | SOA-08 Concurrent audit write | ops/s | ≥ 100 000 | higher_is_better |
| GATE-W9-02 | SOA-01 Auth token validation | p99 latency | ≤ 150 µs | lower_is_better |
| GATE-W9-03 | CFR-04 Node restart + rejoin | p99 latency | ≤ 2000 µs | lower_is_better |
| GATE-W9-04 | SMC-04 RTO recovery cycle | p99 latency | ≤ 5000 µs | lower_is_better |
| GATE-W9-05 | MTI-08 Triage completeness | fraction | = 1.0 | higher_is_better |
| GATE-W9-06 | MTI-07 Cross-tenant throughput | ops/s | ≥ 60 000 | higher_is_better |

---

## Failure Investigation

### GATE-W9-01 (audit throughput < 100 000 ops/s)

1. Re-run `SOA08_ConcurrentAuditWrite_P99Gate_100k` with `--benchmark_repetitions=15`.
2. Verify the mutex critical section in the audit log append path has not grown.
3. Check if `lock_guard` was replaced with a higher-overhead lock.
4. Compare against W8 baseline (audit ≥ 90 000 ops/s) — if W8 also fails it is a
   regression in the mutex implementation, not a threshold change.
5. Escalate to @observability if regression persists across 3 consecutive runs.

### GATE-W9-02 (auth validation p99 > 150 µs)

1. Isolate with `--benchmark_filter=SOA01 --benchmark_repetitions=15`.
2. Check if the FNV-1a hash computation was replaced with a more expensive primitive.
3. Run with TSAN to rule out contention (`-fsanitize=thread`).
4. Escalate to @security-team if regression persists.

### GATE-W9-03 (node rejoin > 2000 µs)

1. Profile `CFR04_NodeRestartRejoin_CycleLatency` — isolate Stop() vs Start() cost.
2. Check if map copy in node restart has grown due to snapshot size increase.
3. Escalate to @storage-team with heap profile.

### GATE-W9-04 (RTO recovery cycle > 5000 µs)

1. Verify the recovery cycle logic has not introduced real sleep calls.
2. Run `SMC04_RTOSimulation_RecoveryCycleLatency` single-threaded to isolate overhead.
3. Escalate to @reliability.

### GATE-W9-05 (triage_completeness < 1.0)

1. Check that all four W9 benchmark binaries built and ran successfully.
2. Verify each binary reports `gate_passed` counter (grep JSON output).
3. Add missing `state.counters["gate_passed"]` to any benchmark that omits it.

### GATE-W9-06 (cross-tenant throughput < 60 000 ops/s)

1. Run `MTI07_TenantAwareConcurrentWrite_Throughput` with `--benchmark_filter=MTI07`
   and reduce thread count to 1 to isolate per-tenant overhead.
2. Profile mutex contention in the multi-tenant store.
3. Escalate to @platform-perf with contention profile.

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
| Security performance | @security-team |
| SLA compliance | @reliability |
| Chaos / fault tolerance | @chaos-team |
| Multi-tenant isolation | @platform-perf |
| CI harness | @ci-team |
| Release sign-off | @platform-perf |
