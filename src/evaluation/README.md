# Evaluation Module Documentation

<!-- Status: current | aligned with docs/IMPLEMENTATION_ROADMAP.md | validated: 2026-07-17 -->

## Purpose

`src/evaluation` is the EPIC 2 surface for making retrieval decisions measurable,
hardware-aware, and policy-governed. The hardware-profile component and the EPIC 2.5
hybrid query planner are implemented. Other EPIC 2 surfaces remain mixed-stage and
continue to advance behind their own phase gates.

## Development-Plan Alignment (EPIC 2)

| Sub-issue | Planned contract | Planned source | Primary planning docs |
|---|---|---|---|
| 2.1 Hardware profiles | `include/hardware_profile.h` | `src/hardware_profile.cc` | `docs/EPIC2_HARDWARE_PROFILES.md` |
| 2.2 Benchmark framework | `include/benchmark_matrix.h` | `src/benchmark_matrix.cc` | `docs/EPIC2_BENCHMARK_FRAMEWORK.md` |
| 2.3 Evaluation metrics | `include/evaluation_metrics.h` | `src/evaluation_metrics.cc` | `docs/EPIC2_EVALUATION_METRICS.md` |
| 2.4 Approximation governance | `include/approximation_rules.h` | `src/approximation_rules.cc` | `docs/EPIC2_APPROXIMATION_GOVERNANCE.md` |
| 2.5 Hybrid query planner | `include/query_planner.h` | `src/query_planner.cc` | `docs/EPIC2_QUERY_PLANNER.md` |
| 2.6 Artifact lifecycle | `include/artifact_lifecycle.h` | `src/artifact_lifecycle.cc` | `docs/EPIC2_ARTIFACT_LIFECYCLE.md` |
| 2.7 Storage strategy | `include/storage_strategy.h` | `src/storage_strategy.cc` | `docs/EPIC2_ARCHITECTURE.md` |

## Current Delivery State

- Wave A complete: EPIC 2 architecture and sub-issue docs are in `docs/EPIC2_*.md`.
- Wave B active: `hardware_profile.h/.cc` and `query_planner.h/.cc` provide live EPIC 2 runtime-owned contracts.
- Wave C active: focused contract coverage exists for hardware profiles in
  `tests/epic2_evaluation/hardware_profile_test.cc` and for hybrid planning in
  `tests/epic2_evaluation/query_planner_test.cc`.
- Wave C active: benchmark baselines now exist in `benchmarks/epic2_evaluation/`,
  including `planner_decision_bench.cc` for tracking issue #5441 hardening work.
- Remaining EPIC 2 surfaces continue to be phased independently and should not be
  treated as implemented unless their local docs and source files say so.

## Seven-Phase Gate (module view)

- [x] Phase 1: design and API contract baselines captured
- [x] Phase 2: hardware-profile and hybrid-planner implementation landed
- [x] Phase 3: hybrid planner policy enforcement and fallback semantics implemented
- [x] Phase 4: hybrid planner verification matrix and regression tests implemented
- [x] Phase 5: hardening against drift/cost regressions
- [x] Phase 6: planner acceptance documentation tied to source-verifiable behavior
- [~] Phase 7: planner build/test wiring landed; downstream default-workflow integration remains pending

## Module Boundaries

In scope:
- evaluation contracts for hardware, metrics, planner policy, and artifact lifecycle decisions
- governance hooks for approximation and storage strategy selection
- interfaces consumed by EPIC 1 retrieval and EPIC 3 distributed artifact planning

Out of scope (current delivery stage):
- learned or cost-model-based planner selection
- production tuning policy beyond the current typed thresholds and benchmarks
- downstream workflow integrations that are explicitly marked deferred in `docs/EPIC2_QUERY_PLANNER.md`

## Installation

No standalone installation step is required for this module. Active planner and
hardware-profile targets are wired through the local CMake surfaces.

## Usage

Use this module documentation to:
- map EPIC 2 sub-issues to owned files
- plan implementation and test sequencing
- review whether phase gates are met before enabling build targets
- trace the shipped planner contract across `include/query_planner.h`,
  `src/query_planner.cc`, tests, and benchmarks

## References

- `docs/IMPLEMENTATION_ROADMAP.md`
- `docs/EPIC2_ARCHITECTURE.md`
- `docs/EPIC1_2_3_DEPENDENCIES.md`
- `src/evaluation/include/README.md`
- `src/evaluation/src/README.md`
