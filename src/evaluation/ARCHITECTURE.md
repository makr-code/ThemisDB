# Evaluation Module Architecture

<!-- Status: current | validated: 2026-07-17 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

`src/evaluation` provides EPIC 2 contracts for hardware-aware planning, benchmark matrix
definition, evaluation metrics, approximation governance, query planning, artifact
lifecycle management, and storage strategy selection.

Current architecture is mixed-stage: the hardware-profile contract and the EPIC 2.5
hybrid query planner are live, while the remaining EPIC 2 surfaces stay staged behind
their local roadmap gates.

## Component Map

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
- hybrid planner routing and fallback semantics for ANN, tensor, graph, and distributed paths

Out of scope at current stage:
- learned or adaptive planner cost models
- finalized benchmark and policy tuning outcomes beyond the shipped query-planner baselines
- default pipeline enablement for integrations still marked deferred

## Integration Surfaces

- Planning: `docs/EPIC2_ARCHITECTURE.md` and EPIC 2 sub-issue docs
- Implemented contracts: `src/evaluation/include/hardware_profile.h`, `src/evaluation/include/query_planner.h`
- Implemented runtimes: `src/evaluation/src/hardware_profile.cc`, `src/evaluation/src/query_planner.cc`
- Active planner verification: `tests/epic2_evaluation/query_planner_test.cc`
- Active planner hardening bench: `benchmarks/epic2_evaluation/planner_decision_bench.cc`
- Remaining contracts/implementations: still phase-gated per local module docs
- Dependency sequencing: `docs/EPIC1_2_3_DEPENDENCIES.md`
