# retrieval/src documentation

<!-- Status: current | aligned with docs/IMPLEMENTATION_ROADMAP.md | validated: 2026-06-01 -->

## Purpose

This directory contains EPIC 1 skeleton translation units that mirror the public headers.
They exist to lock file ownership and integration order, not to provide production retrieval behavior yet.

## Source Ownership Map

> These source files are deferred to the implementation PR. The table documents planned ownership.

| Source file | EPIC sub-issue | Planned runtime responsibility |
|---|---|---|
| `ann_frontdoor.cc` | 1.1 | backend routing and ANN candidate collection entry flow |
| `tensor_midlayer.cc` | 1.2 | tensor-context retrieval bridge and compression-aware dispatch |
| `graph_validator.cc` | 1.3 | graph-backed evidence validation and confidence shaping |
| `lora_package.cc` | 1.4 | adapter artifact packaging and provenance verification hooks |
| `model_switch.cc` | 1.5 | safe model version transitions with policy controls |
| `federated_summaries.cc` | 1.6 | federated summary orchestration across sources |
| `retrieval_observability.cc` | 1.7 | retrieval tracing, governance capture, and audit surfaces |

## Phase Progress Expectations

- Phase 2 (next): minimal skeleton surfaces will be added in the implementation PR.
- Phase 3 (after): explicit failure semantics, fallback paths, and edge-case handling per source.
- Phase 4+: tests, hardening, docs acceptance, and integration follow the seven-phase rule.

## Integration Notes

- Build targets remain intentionally disabled in local `CMakeLists.txt` until acceptance criteria are met.
- Downstream module coupling must follow `docs/EPIC1_2_3_DEPENDENCIES.md` to avoid cross-epic rework.

## Installation

No standalone installation step is required while this directory contains phase-2 skeleton sources.

## Usage

Use this README to track ownership and implementation readiness for each EPIC 1 source file.

## References

- `docs/IMPLEMENTATION_ROADMAP.md`
- `docs/EPIC1_2_3_DEPENDENCIES.md`
- `src/retrieval/README.md`
