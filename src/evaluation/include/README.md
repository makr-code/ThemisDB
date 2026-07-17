# evaluation/include documentation

<!-- Status: current | aligned with docs/IMPLEMENTATION_ROADMAP.md | validated: 2026-07-17 -->

## Purpose

This directory hosts EPIC 2 public contracts for evaluation policy, measurement,
and query path planning. `hardware_profile.h` and `query_planner.h` are now active;
the remaining contracts continue as phased-delivery scaffolds.

## Header Ownership Map

| Header | EPIC sub-issue | Contract focus |
|---|---|---|
| `hardware_profile.h` | 2.1 | hardware capability/tier descriptors and registry API |
| `benchmark_matrix.h` | 2.2 | scenario catalog and benchmark result contract |
| `evaluation_metrics.h` | 2.3 | metric dimensions, observations, and report interfaces |
| `approximation_rules.h` | 2.4 | exactness zones, violations, and policy checks |
| `query_planner.h` | 2.5 | path-selection request/plan schema and planner config |
| `artifact_lifecycle.h` | 2.6 | freshness state, staleness triggers, and lifecycle policy |
| `storage_strategy.h` | 2.7 | storage mode/quantization strategy recommendation contracts |

## Contract Governance Rules

- keep naming and semantics synchronized with `docs/EPIC2_*.md`
- avoid introducing runtime-coupled behavior into headers before Phase 3 design sign-off
- record cross-epic dependencies when contracts influence retrieval or distributed tensor plans
- preserve advisory-only tensor semantics and CPU-only graph-truth gates in planner-facing APIs

## Transition Checklist (toward implementation hardening)

- define explicit failure modes for each policy interface
- document expected invariants and value ranges for each request/result type
- keep `query_planner.h` synchronized with `tests/epic2_evaluation/query_planner_test.cc`

## Installation

No separate installation step is required for contract headers.
Keep non-active contracts documentation-first until their runtime behavior is approved.

## Usage

Use this README as the contract review index before modifying EPIC 2 headers.

## References

- `docs/EPIC2_ARCHITECTURE.md`
- `docs/EPIC2_HARDWARE_PROFILES.md`
- `docs/EPIC2_BENCHMARK_FRAMEWORK.md`
- `docs/EPIC2_EVALUATION_METRICS.md`
- `docs/EPIC2_APPROXIMATION_GOVERNANCE.md`
- `docs/EPIC2_QUERY_PLANNER.md`
- `docs/EPIC2_ARTIFACT_LIFECYCLE.md`
- `docs/EPIC2_ARCHITECTURE.md`
