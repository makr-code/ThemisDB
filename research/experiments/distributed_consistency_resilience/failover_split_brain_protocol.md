# Experiment Protocol: Failover Split-Brain and Disaster Recovery

**Manuscript**: `research/manuscripts/distributed_consistency_resilience/FAILOVER_SPLIT_BRAIN_DISASTER_RECOVERY_PAPER_DRAFT.md`  
**Status**: PROTOCOL_DRAFT  
**Last Updated**: 2026-08-10

---

## Objective

Produce reproducible latency and correctness data for the six failover hot paths (FP23-01..FP23-06) and eight focused behavioral tests (P23-01..P23-08), enabling quantitative evidence for the VLDB 2027 submission.

---

## Experiment Suite

### Suite F1 — Hot-Path Latency (Gates FP23-01..FP23-06)

| ID | Operation | Gate threshold | Repetitions |
|---|---|---|---|
| F1-01 | `canTransition()` state table | p99 ≤ 100 µs | ≥ 10 benchmark runs |
| F1-02 | `preventSplitBrain()` fail-closed | p99 ≤ 200 µs | ≥ 10 |
| F1-03 | `executePlan()` try_to_lock uncontested | p99 ≤ 100 µs | ≥ 10 |
| F1-04 | `attemptRecovery()` batch stats flush | p99 ≤ 200 µs | ≥ 10 |
| F1-05 | `emitDiagnostic()` no-subscriber path | p99 ≤ 100 µs | ≥ 10 |
| F1-06 | Queue-full rejection | p99 ≤ 200 µs | ≥ 10 |

**Benchmark target**: `module_failover_bench_failover_phase2_phase3_gates_focused`  
**Seed**: 42 (per `benchmarks/MEASUREMENT_HYGIENE.md`)  
**Report format**: JSON via `--benchmark_format=json --benchmark_out=results/F1_<timestamp>.json`

### Suite F2 — Behavioral Correctness (P23-01..P23-08)

Run all focused tests and record pass/fail per test ID.

**Test target**: `module_failover_test_failover_phase2_phase3_focused`  
**Contract target**: `module_failover_test_failover_contract_hardening_focused`

### Suite F3 — Dependency-Degraded Scenarios (Planned)

Multi-node chaos harness: inject fencing manager failure during active failover cycle.

- Dependency: cluster harness (not yet available in CI)
- Target: demonstrate fail-closed behavior under real manager failure, not just null-pointer injection

---

## Environment

- Build preset: `linux-release` or `windows-release`
- Minimum hardware: 4 physical cores, 16 GB RAM (per `PERFORMANCE_EXPECTATIONS.md`)
- OS isolation: no other database workloads during measurement windows

---

## Artifact Checklist

- [ ] F1 JSON results committed under `research/experiments/distributed_consistency_resilience/results/F1_<timestamp>.json`
- [ ] F2 test output log committed
- [ ] Gate table updated in manuscript §IV.E4
- [ ] Regression history: ≤10% threshold tracked across runs
- [ ] F3 protocol description completed (even if not executed before submission)
