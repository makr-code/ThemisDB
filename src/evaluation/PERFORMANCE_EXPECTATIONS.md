# Evaluation Module Performance Expectations

<!-- Status: current | validated: 2026-07-29 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_EVIDENCE.md -->

## Current Baseline

Evaluation module behavior is no longer documentation-first only: the repository now
contains benchmark entry points for planner decision overhead, benchmark-matrix
scenarios, artifact staleness, and storage-strategy follow-up paths. However, no new
measured benchmark results were captured in the current issue cycle, so Phase 5 remains
open until executable benchmark evidence is refreshed.

## Phase-Gated Performance Expectations

### Phase 3-4 (behavior + tests)
- establish deterministic correctness and stability baselines for EPIC 2 contracts
- validate planner and policy semantics with contract-focused regressions

### Phase 5 (performance hardening)
- define latency/throughput budgets for planner and evaluation paths
- define storage and hardware-profile evaluation budget expectations
- lock benchmark matrices for representative workload classes
- capture measured baselines and fallback-rate guardrails for the current validation cycle

### Phase 6-7 (acceptance + integration)
- publish only benchmark-backed performance claims
- enforce regression gates before default integration
- keep evidence blockers explicit when benchmark execution is not possible in the current environment

## Benchmark Work Items

- keep `benchmarks/epic2_evaluation/` aligned with issue #5428 workload classes
- map benchmark scenarios to hardware profile classes and policy modes
- maintain release-baseline tracking for phase-gate promotion
- preserve source-to-evidence traceability through `src/evaluation/MODULE_EVIDENCE.md`

## Non-Goals (Current Stage)

- no production performance numbers are asserted yet
- no optimization claims are made without benchmark artifacts
