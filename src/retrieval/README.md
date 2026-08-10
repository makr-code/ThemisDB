# Retrieval Module Documentation

<!-- Status: PRODUCTION_CANDIDATE | Phase 1-3 complete | aligned with docs/IMPLEMENTATION_ROADMAP.md | validated: 2026-08-10 -->

## Purpose

`src/retrieval` is the EPIC 1 implementation surface for the layered retrieval stack
(ANN frontdoor → tensor mid-layer → graph validation → model/governance controls).
The module currently provides reviewed API contracts and skeleton translation units,
so downstream work can proceed in a controlled seven-phase delivery model.

## Development-Plan Alignment (EPIC 1)

> Source files (`*.h`, `*.cc`) are deferred to the implementation PR.

| Sub-issue | Planned contract | Planned source | Primary planning docs |
|---|---|---|---|
| 1.1 ANN frontdoor | `include/ann_frontdoor.h` | `src/ann_frontdoor.cc` | `docs/EPIC1_ANN_FRONTDOOR.md` |
| 1.2 Tensor mid-layer | `include/tensor_midlayer.h` | `src/tensor_midlayer.cc` | `docs/EPIC1_TENSOR_MIDLAYER.md` |
| 1.3 Graph validation | `include/graph_validator.h` | `src/graph_validator.cc` | `docs/EPIC1_GRAPH_VALIDATION.md` |
| 1.4 LoRA artifacts | `include/lora_package.h` | `src/lora_package.cc` | `docs/EPIC1_LORA_ARTIFACTS.md` |
| 1.5 Model switch workflow | `include/model_switch.h` | `src/model_switch.cc` | `docs/EPIC1_MODEL_SWITCH.md` |
| 1.6 Federated summaries | `include/federated_summaries.h` | `src/federated_summaries.cc` | `docs/EPIC1_FEDERATED_SUMMARIES.md` |
| 1.7 Observability/governance | `include/retrieval_observability.h` | `src/retrieval_observability.cc` | `docs/EPIC1_ARCHITECTURE.md` |

## Current Delivery State

- Wave A complete: architecture and per-sub-issue contract docs exist under `docs/EPIC1_*.md`.
- Wave B partial: module structure (`README.md`, `include/README.md`, `src/README.md`, `CMakeLists.txt`) is in place. Header and source files (`*.h`, `*.cc`) are out of scope for this PR and will be added in a dedicated implementation PR.
- Wave C pending: tests/benchmarks and production behavior are tracked outside this scaffold
  (`tests/epic1_retrieval/`, `benchmarks/epic1_retrieval/` per roadmap).

## Seven-Phase Gate (module view)

- [x] Phase 1: design and API contracts defined in EPIC docs and headers
- [ ] Phase 2: skeleton translation units and factory entry points (deferred to implementation PR)
- [ ] Phase 3: runtime error handling and edge-case behavior
- [ ] Phase 4: contract tests and scenario coverage
- [ ] Phase 5: performance hardening and instrumentation depth
- [ ] Phase 6: acceptance documentation from real behavior (not scaffold intent)
- [ ] Phase 7: integration into production build/test pipelines

## Module Boundaries

In scope:
- retrieval routing contracts and data-shape definitions
- interfaces required by EPIC 2 (evaluation/planning) and EPIC 3 (artifact portability)
- observability/governance contract points for retrieval decisions

Out of scope (until later phases):
- production retrieval algorithms and backend-specific optimizations
- final benchmark tuning and SLO enforcement
- rollout gating in default build targets

## Installation

No standalone installation step is required at scaffold stage.
Build target enablement is intentionally deferred to later phase-gate approval.

## Usage

Use this module documentation to:
- map EPIC 1 sub-issues to owned headers/sources
- review delivery-wave and phase-gate status
- prepare implementation/test sequencing without changing production targets

## References

- `docs/IMPLEMENTATION_ROADMAP.md`
- `docs/EPIC1_ARCHITECTURE.md`
- `docs/EPIC1_2_3_DEPENDENCIES.md`
- `src/retrieval/include/README.md`
- `src/retrieval/src/README.md`
