# Evaluation Module Architecture

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

`src/evaluation` provides EPIC 2 contracts for hardware-aware planning, benchmark matrix
definition, evaluation metrics, approximation governance, query planning, artifact
lifecycle management, and storage strategy selection.

Current architecture is scaffold-first: headers define interface contracts while paired
translation units maintain implementation ownership boundaries.

## Component Map

> Source files (`*.h`, `*.cc`) are deferred to the implementation PR.
> The table below documents planned file ownership; no code files exist yet.

| Component | Planned contract | Planned implementation |
|---|---|---|
| Hardware profile | `include/hardware_profile.h` | `src/hardware_profile.cc` |
| Benchmark matrix | `include/benchmark_matrix.h` | `src/benchmark_matrix.cc` |
| Evaluation metrics | `include/evaluation_metrics.h` | `src/evaluation_metrics.cc` |
| Approximation rules | `include/approximation_rules.h` | `src/approximation_rules.cc` |
| Query planner | `include/query_planner.h` | `src/query_planner.cc` |
| Artifact lifecycle | `include/artifact_lifecycle.h` | `src/artifact_lifecycle.cc` |
| Storage strategy | `include/storage_strategy.h` | `src/storage_strategy.cc` |

## Boundaries

In scope:
- EPIC 2 contract ownership and sequencing
- integration seams with EPIC 1 retrieval and EPIC 3 distributed artifacts
- phase-gated readiness documentation

Out of scope at scaffold stage:
- production planner/scoring behavior claims
- finalized benchmark and policy tuning outcomes
- default pipeline enablement

## Integration Surfaces

- Planning: `docs/EPIC2_ARCHITECTURE.md` and EPIC 2 sub-issue docs
- Planned contracts: `src/evaluation/include/*.h` (deferred to implementation PR)
- Planned implementation: `src/evaluation/src/*.cc` (deferred to implementation PR)
- Dependency sequencing: `docs/EPIC1_2_3_DEPENDENCIES.md`
