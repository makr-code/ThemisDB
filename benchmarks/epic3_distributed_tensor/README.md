# EPIC 3 distributed tensor benchmarks

<!-- Status: current | phase-5 scaffold | validated: 2026-07-17 -->

## Scope

Phase 5 defines deterministic benchmark governance for `src/distributed_tensor`
without claiming measured production behavior before the runtime implementation
exists. The files in this directory reserve workload identities, hard gates, and
triage rules for the later execution phase.

## Planned benchmark files

- `manifest_coordination_bench.cc`
- `placement_strategy_bench.cc`
- `integrity_verification_bench.cc`
- `recovery_rebuild_bench.cc`
- `distributed_retrieval_bench.cc`
- `infrastructure_bench.cc`

## Deterministic benchmark assets

- `phase5_workload_profiles.json` — canonical workload inventory and seed usage
- `release_gate_manifest_epic3.json` — measurable Phase 5 gates and pass/fail rules
- `report_variance_epic3.py` — compare captured result bundles against the gate manifest
- `RUNBOOK_EPIC3_PHASE5.md` — execution checklist once runtime benchmarks land
- `REPRO_TRIAGE_EPIC3_PHASE5.md` — failure triage for degraded-mode and recovery regressions

## Measurement goals

- Capture planner latency, placement throughput, integrity verification cost, and recovery budgets
- Keep topology, dataset, and degraded-node assumptions explicit for every workload profile
- Use canonical seed `42` for reproducible artifact layouts and retry schedules
- Carry variance and gate output forward into Phase 6 acceptance documentation

## Hardening scenarios reserved in this scaffold

- degraded-mode retrieval with one unavailable node
- retry/recovery convergence after shard rebuild initiation
- integrity verification under sustained manifest churn
- infrastructure control-plane stability under placement pressure

## Evidence status

- Gate definitions: present
- Deterministic workload profiles: present
- Runtime measurements: pending the distributed tensor implementation PR
- Phase 6 acceptance promotion: blocked until measured evidence is attached per gate

## Reference

- `docs/EPIC2_BENCHMARK_FRAMEWORK.md`
- `src/distributed_tensor/PERFORMANCE_EXPECTATIONS.md`
- `src/distributed_tensor/ROADMAP.md`

## Installation

No standalone installation step is required for the metadata scaffold.
Executable benchmark targets remain intentionally disabled until the runtime
module sources are available.

## Usage

Use this directory to:

- lock workload names and benchmark ownership before runtime code lands
- review measurable gate thresholds without asserting unmeasured claims
- prepare Phase 6 evidence collection and variance reporting inputs
