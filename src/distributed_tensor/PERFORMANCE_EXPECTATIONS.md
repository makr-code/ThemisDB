# Distributed Tensor Module Performance Expectations

<!-- Status: current | validated: 2026-07-17 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Current Baseline

Distributed tensor module behavior is at documentation scaffold stage. Module structure
and planning docs exist; contract headers (`*.h`), skeleton translation units (`*.cc`),
production distributed runtime behavior, and measured benchmark evidence are deferred to
the implementation PR.

## Phase 5 Governance Assets

- `benchmarks/epic3_distributed_tensor/phase5_workload_profiles.json`
- `benchmarks/epic3_distributed_tensor/release_gate_manifest_epic3.json`
- `benchmarks/epic3_distributed_tensor/report_variance_epic3.py`
- `benchmarks/epic3_distributed_tensor/RUNBOOK_EPIC3_PHASE5.md`
- `benchmarks/epic3_distributed_tensor/REPRO_TRIAGE_EPIC3_PHASE5.md`

## Phase-Gated Performance Expectations

### Phase 3-4 (behavior + tests)
- establish deterministic correctness and fault-handling baselines
- validate placement, integrity, and recovery contract semantics

### Phase 5 (performance hardening)
- define distributed latency/throughput targets for placement and retrieval planning paths
- define recovery-time and integrity-check budget expectations
- lock deterministic benchmark profiles and representative topologies before runtime execution
- require canonical seed `42` across planner, placement, integrity, recovery, and control-plane scenarios

### Phase 6-7 (acceptance + integration)
- publish only benchmark-backed and fault-tested performance claims
- enforce regression gates before default integration

## Phase 5 Gate Targets

| Gate | Profile | Metric | Threshold |
|---|---|---|---|
| `GATE-E3-P5-01` | `planner_latency_healthy` | planner latency p99 | `<= 750 µs` |
| `GATE-E3-P5-02` | `placement_throughput_balanced` | placement throughput | `>= 25,000 ops/s` |
| `GATE-E3-P5-03` | `integrity_under_manifest_churn` | verification success ratio | `>= 0.999` |
| `GATE-E3-P5-04` | `recovery_rebuild_degraded` | rebuild convergence | `<= 30,000 ms` |
| `GATE-E3-P5-05` | `infrastructure_control_plane_stability` | degraded availability ratio | `>= 0.99` |
| `GATE-E3-P5-06` | `planner_latency_healthy` | coefficient of variation | `<= 5.0%` |

These are gate definitions, not measured results. Pass/fail status remains pending
until benchmark result bundles are produced from the runtime implementation.

## Benchmark Work Items

- maintain `benchmarks/epic3_distributed_tensor/` workload profiles, runbooks, and gate manifest
- implement distributed/fault-path test evidence in `tests/epic3_distributed_tensor/`
- capture runtime benchmark results and maintain release-baseline tracking for phase-gate promotion

## Phase 6 Acceptance Preconditions

- every Phase 5 profile has a result bundle entry
- every gate records an explicit pass/fail decision
- failed gates include triage notes and follow-up ownership
- no acceptance claim is published without attached runtime evidence

## Non-Goals (Current Stage)

- no production distributed performance numbers are asserted yet
- no resiliency/SLO claims are made without benchmark and fault-injection evidence
