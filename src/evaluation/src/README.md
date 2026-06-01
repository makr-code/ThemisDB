# evaluation/src documentation

<!-- Status: current | aligned with docs/IMPLEMENTATION_ROADMAP.md | validated: 2026-06-01 -->

## Purpose

This directory provides EPIC 2 skeleton implementation files that reserve ownership,
module boundaries, and integration order while production policy logic is still pending.

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

- current: Phase-2 scaffold baseline only
- next: Phase-3 behavior definitions for validation failures, policy conflicts, and fallback paths
- later: tests, benchmark evidence, acceptance docs, and product integration per roadmap

## Integration Notes

- local `CMakeLists.txt` keeps targets disabled until acceptance gates are approved
- dependency order should follow `docs/EPIC1_2_3_DEPENDENCIES.md`

## Installation

No standalone installation step is required while these files remain skeleton sources.

## Usage

Use this README to track source ownership and phase readiness before enabling targets.

## References

- `docs/IMPLEMENTATION_ROADMAP.md`
- `docs/EPIC1_2_3_DEPENDENCIES.md`
- `src/evaluation/README.md`
