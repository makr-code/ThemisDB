# Evaluation Module Documentation

<!-- Status: PRODUCTION_CANDIDATE | Phase 1-3 complete | aligned with status issue #5643 | validated: 2026-08-10 -->

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
- Wave B complete at source level: runtime-owned contracts and sources exist for
  hardware profile, benchmark matrix, retrieval metrics, ablation, approximation,
  query planning, and artifact lifecycle.
- Wave C partial: focused tests and benchmark entry points exist in source, but
  current-cycle executable evidence is still incomplete for status issue #5643.
- Phase 3-6 closure remains open until runtime policy hardening, refreshed build/test
  evidence, and benchmark-backed acceptance gates are all recorded.

## Seven-Phase Gate (module view)

- [x] Phase 1: design and API contract baselines captured
- [x] Phase 2: core implementation surfaces landed in source
- [~] Phase 3: planner-centric policy behavior exists, but module-wide runtime hardening remains open
- [~] Phase 4: source-level focused tests exist, but refreshed executable evidence is still pending
- [~] Phase 5: benchmark entry points exist, but measured gates are not yet captured for this issue cycle
- [~] Phase 6: acceptance docs exist, but closure evidence remains incomplete
- [ ] Phase 7: default workflow integration remains gated

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
- review whether phase gates are met before enabling default workflow integration
- trace the shipped planner contract across `include/query_planner.h`,
  `src/query_planner.cc`, tests, and benchmarks
- review current-cycle blockers in `MODULE_EVIDENCE.md`

## References

- `docs/IMPLEMENTATION_ROADMAP.md`
- `docs/EPIC2_ARCHITECTURE.md`
- `docs/EPIC1_2_3_DEPENDENCIES.md`
- `src/evaluation/include/README.md`
- `src/evaluation/src/README.md`
