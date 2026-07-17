# Evaluation Module Performance Expectations

<!-- Status: current | validated: 2026-07-13 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Current Baseline

Evaluation module behavior remains documentation-first for EPIC 2 interfaces, but
Phase 5 baseline evidence now exists through source-verifiable benchmark runners in
`benchmarks/epic2_evaluation/` that exercise planner/placement/recovery paths tied
to issue #5428.

## Phase-Gated Performance Expectations

### Phase 3-4 (behavior + tests)
- establish deterministic correctness and stability baselines for EPIC 2 contracts
- validate planner and policy semantics with contract-focused regressions

### Phase 5 (performance hardening)
- define latency/throughput budgets for planner and evaluation paths
- define storage and hardware-profile evaluation budget expectations
- lock benchmark matrices for representative workload classes

### Phase 6-7 (acceptance + integration)
- publish only benchmark-backed performance claims
- enforce regression gates before default integration

## Benchmark Work Items

- keep `benchmarks/epic2_evaluation/` aligned with issue #5428 workload classes
- map benchmark scenarios to hardware profile classes and policy modes
- maintain release-baseline tracking for phase-gate promotion

## Non-Goals (Current Stage)

- no production performance numbers are asserted yet
- no optimization claims are made without benchmark artifacts
