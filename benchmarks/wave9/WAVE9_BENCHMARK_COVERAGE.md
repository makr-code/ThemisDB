# Wave 9 Benchmark Coverage

<!-- ThemisDB | WAVE9_BENCHMARK_COVERAGE.md | Version: 0.0.1 -->

## Overview

Wave 9 introduces four benchmark sub-suites covering security hardening,
SLA compliance measurement, chaos fault recovery, and multi-tenant isolation.
Key additions from W8:

- **Auth validation gate added**: p99 ≤ 150 µs for every request auth check
- **Audit throughput gate raised**: 100 000 ops/s (up from W8's 90 000)
- **Node rejoin gate added**: cluster recovery ≤ 2000 µs
- **RTO simulation gate added**: recovery cycle ≤ 5000 µs
- **Multi-tenant throughput gate added**: ≥ 60 000 ops/s cross-tenant writes
- **Triage completeness gate carried forward**: 1.0 (all benchmarks report gate_passed)

---

## Sub-Wave Coverage Table

| Sub-wave | File | Tests | Hard Gates | Soft Gates |
|----------|------|-------|------------|------------|
| W9-A | `bench_w9a_security_overhead_audit.cpp` | SOA-01..SOA-08 | GATE-W9-01, GATE-W9-02 | SGATE-W9-03 |
| W9-B | `bench_w9b_sla_measurement_compliance.cpp` | SMC-01..SMC-08 | GATE-W9-04 | SGATE-W9-01 |
| W9-C | `bench_w9c_chaos_fault_recovery.cpp` | CFR-01..CFR-08 | GATE-W9-03 | SGATE-W9-02 |
| W9-D | `bench_w9d_multi_tenant_isolation.cpp` | MTI-01..MTI-08 | GATE-W9-05, GATE-W9-06 | — |

---

## W9-A Coverage — Security Overhead & Audit

| ID | Scenario | Gate | Threshold |
|----|----------|------|-----------|
| SOA-01 | Auth token validation throughput (FNV-1a hash) | **GATE-W9-02** | p99 ≤ 150 µs |
| SOA-02 | Rate-limiter enforcement latency (token bucket) | advisory | — |
| SOA-03 | Audit event write throughput (concurrent) | advisory | — |
| SOA-04 | Input sanitisation throughput (injection reject) | advisory | — |
| SOA-05 | Credential rotation latency | advisory | — |
| SOA-06 | Nonce store lookup throughput | **SGATE-W9-03** | mismatches = 0 |
| SOA-07 | Tamper-detection scan throughput | advisory | — |
| SOA-08 | Concurrent audit write p99 gate | **GATE-W9-01** | ≥ 100 000 ops/s |

---

## W9-B Coverage — SLA Measurement & Compliance

| ID | Scenario | Gate | Threshold |
|----|----------|------|-----------|
| SMC-01 | Availability window computation throughput | advisory | — |
| SMC-02 | p99 latency measurement overhead | **SGATE-W9-01** | CV < 5% |
| SMC-03 | Graceful rejection latency (overload path) | advisory | — |
| SMC-04 | RTO simulation — recovery cycle latency | **GATE-W9-04** | p99 ≤ 5000 µs |
| SMC-05 | RPO simulation — data-loss accounting | advisory | — |
| SMC-06 | Rolling-window availability tracker throughput | advisory | — |
| SMC-07 | Degraded-mode throughput floor (50% workers) | advisory | — |
| SMC-08 | SLA gate counter emission throughput | advisory | — |

---

## W9-C Coverage — Chaos & Fault Recovery

| ID | Scenario | Gate | Threshold |
|----|----------|------|-----------|
| CFR-01 | Network partition inject/heal cycle latency | advisory | — |
| CFR-02 | Batch rollback latency (100-record batch) | advisory | — |
| CFR-03 | Bulkhead isolation overhead | advisory | — |
| CFR-04 | Node restart + rejoin cycle latency | **GATE-W9-03** | p99 ≤ 2000 µs |
| CFR-05 | Write storm completion time (8 threads × 50) | **SGATE-W9-02** | drift < 8% |
| CFR-06 | Degraded read fallback latency | advisory | — |
| CFR-07 | Timeout enforcement accuracy | advisory | — |
| CFR-08 | Chaos gate counter emission | advisory | — |

---

## W9-D Coverage — Multi-Tenant Isolation

| ID | Scenario | Gate | Threshold |
|----|----------|------|-----------|
| MTI-01 | Tenant namespace isolation throughput | advisory | — |
| MTI-02 | Per-tenant quota enforcement latency | advisory | — |
| MTI-03 | Tenant eviction latency | advisory | — |
| MTI-04 | Cross-tenant read isolation throughput | advisory | — |
| MTI-05 | Tenant creation throughput | advisory | — |
| MTI-06 | Tenant resource accounting throughput | advisory | — |
| MTI-07 | Tenant-aware concurrent write throughput | **GATE-W9-06** | ≥ 60 000 ops/s |
| MTI-08 | Multi-tenant gate self-check | **GATE-W9-05** | triage_completeness = 1.0 |

---

## Threshold Comparison: W8 vs W9

| Metric | W8 Threshold | W9 Threshold | Change |
|--------|-------------|-------------|--------|
| Audit log throughput | 90 000 ops/s | 100 000 ops/s | +11% (tighter) |
| Read p99 | 175 µs | N/A (replaced by auth p99) | new auth gate |
| Auth token validation p99 | N/A | 150 µs | new gate |
| Node rejoin latency | N/A | 2000 µs | new gate |
| RTO recovery cycle | N/A | 5000 µs | new gate |
| Cross-tenant throughput | N/A | 60 000 ops/s | new gate |
| Triage completeness | 1.0 | 1.0 | carried forward |

---

## Files

| File | Purpose |
|------|---------|
| `bench_w9a_security_overhead_audit.cpp` | SOA benchmark suite |
| `bench_w9b_sla_measurement_compliance.cpp` | SMC benchmark suite |
| `bench_w9c_chaos_fault_recovery.cpp` | CFR benchmark suite |
| `bench_w9d_multi_tenant_isolation.cpp` | MTI benchmark suite |
| `CMakeLists.txt` | Build registration |
| `release_gate_manifest_w9.json` | Gate definitions and thresholds |
| `report_variance_w9.py` | Variance and gate evaluation tool |
| `RUNBOOK_W9.md` | Operational runbook |
| `REPRO_TRIAGE_W9.md` | Reproduction and triage guide |
| `WAVE9_BENCHMARK_COVERAGE.md` | This file |
