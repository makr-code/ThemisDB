# Evaluation Module Documentation

<!-- Status: current | aligned with docs/IMPLEMENTATION_ROADMAP.md | validated: 2026-07-13 -->

## Purpose

`src/evaluation` is the EPIC 2 surface for making retrieval decisions measurable,
hardware-aware, and policy-governed. The hardware-profile component is now implemented;
the remaining EPIC 2 components stay documentation-first until their phase gates land.

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
- Wave B partial: `hardware_profile.h/.cc` now provide the first EPIC 2 runtime-owned contract.
  Remaining headers and sources stay deferred to later implementation PRs.
- Wave C partial: focused contract coverage exists for hardware profiles in
  `tests/epic2_evaluation/hardware_profile_test.cc`; broader regression and benchmark suites remain pending.
- Wave B partial: module structure (`README.md`, `include/README.md`, `src/README.md`, `CMakeLists.txt`) is in place. Header and source files (`*.h`, `*.cc`) are out of scope for this PR and will be added in a dedicated implementation PR.
- Wave C partial: benchmark baselines now exist in `benchmarks/epic2_evaluation/`
  for issue #5428 planner/placement/recovery evaluation paths; dedicated EPIC 2
  contract tests remain pending.

## Seven-Phase Gate (module view)

- [x] Phase 1: design and API contract baselines captured
- [~] Phase 2: hardware-profile implementation landed; other EPIC 2 surfaces remain deferred
- [~] Phase 3: hardware-profile validation and transition guards implemented; broader policy hooks pending
- [~] Phase 4: hardware-profile contract tests implemented; wider matrix coverage pending
- [ ] Phase 5: hardening against drift/cost regressions
- [ ] Phase 2: skeleton code surfaces established (deferred to implementation PR)
- [ ] Phase 3: policy enforcement behavior and failure semantics
- [ ] Phase 4: verification matrix and regression tests
- [x] Phase 5: hardening against drift/cost regressions
- [ ] Phase 6: acceptance documentation tied to measured results
- [ ] Phase 7: integration into default product workflows

## Module Boundaries

In scope:
- evaluation contracts for hardware, metrics, planner policy, and artifact lifecycle decisions
- governance hooks for approximation and storage strategy selection
- interfaces consumed by EPIC 1 retrieval and EPIC 3 distributed artifact planning

Out of scope (current scaffold stage):
- final scoring implementations beyond the current benchmark baselines
- production tuning policy and rollout thresholds
- default build/test enablement

## Installation

No standalone installation step is required for this module at scaffold stage.
Implementation targets stay disabled until phase gates are approved.

## Usage

Use this module documentation to:
- map EPIC 2 sub-issues to owned files
- plan implementation and test sequencing
- review whether phase gates are met before enabling build targets

## References

- `docs/IMPLEMENTATION_ROADMAP.md`
- `docs/EPIC2_ARCHITECTURE.md`
- `docs/EPIC1_2_3_DEPENDENCIES.md`
- `src/evaluation/include/README.md`
- `src/evaluation/src/README.md`
