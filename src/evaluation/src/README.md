# evaluation/src documentation

<!-- Status: current | aligned with status issue #5643 | validated: 2026-07-29 -->

## Purpose

This directory now contains runtime-owned EPIC 2 implementations for hardware profile,
benchmark matrix, retrieval metrics, ablation, approximation rules, query planning,
and artifact lifecycle handling.

## Source Ownership Map

| Source file | EPIC sub-issue | Planned runtime responsibility |
|---|---|---|
| `hardware_profile.cc` | 2.1 | hardware profile registration/lookup behavior |
| `benchmark_matrix.cc` | 2.2 | benchmark scenario orchestration and result collection |
| `evaluation_metrics.cc` | 2.3 | metric aggregation/report generation behavior |
| `approximation_rules.cc` | 2.4 | approximation policy evaluation and violation reporting |
| `query_planner.cc` | 2.5 | hybrid path selection from planner requests |
| `artifact_lifecycle.cc` | 2.6 | lifecycle state transitions and freshness policy handling |
| `storage_strategy.cc` | 2.7 | storage recommendation selection and trade-off scoring |

## Delivery Expectations

- delivered at source level: the current EPIC 2 runtime files listed above
- next: broader Phase 3 behavior definitions for validation failures, policy conflicts, and fallback paths
- after: refreshed executable test/benchmark evidence and measured acceptance gates
- later: downstream workflow integration per roadmap

## Integration Notes

- local `CMakeLists.txt` keeps selected targets behind availability gates and environment prerequisites
- dependency order should follow `docs/EPIC1_2_3_DEPENDENCIES.md`

## Installation

No standalone installation step is required for the current source set.

## Usage

Use this README to track source ownership and phase readiness while additional EPIC 2 sources are added.

## References

- `docs/IMPLEMENTATION_ROADMAP.md`
- `docs/EPIC1_2_3_DEPENDENCIES.md`
- `src/evaluation/README.md`
