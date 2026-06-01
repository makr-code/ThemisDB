# Distributed Tensor Module Documentation

<!-- Status: current | aligned with docs/IMPLEMENTATION_ROADMAP.md | validated: 2026-06-01 -->

## Purpose

`src/distributed_tensor` is the EPIC 3 surface for portable tensor artifacts,
manifest semantics, shard placement, integrity verification, and recovery planning.
The module currently provides contract-first scaffolding aligned to the seven-phase rule.

## Development-Plan Alignment (EPIC 3)

> Source files (`*.h`, `*.cc`) are deferred to the implementation PR.

| Sub-issue | Planned contract | Planned source | Primary planning docs |
|---|---|---|---|
| 3.1 Artifact classes | `include/tensor_artifact_classes.h` | `src/tensor_artifact_classes.cc` | `docs/EPIC3_ARTIFACT_CLASSES.md` |
| 3.2 Manifest schema | `include/artifact_manifest.h` | `src/artifact_manifest.cc` | `docs/EPIC3_MANIFEST_SCHEMA.md` |
| 3.3 Shard placement | `include/shard_placement.h` | `src/shard_placement.cc` | `docs/EPIC3_SHARD_PLACEMENT.md` |
| 3.4 Integrity model | `include/integrity_verification.h` | `src/integrity_verification.cc` | `docs/EPIC3_INTEGRITY_MODEL.md` |
| 3.5 Recovery strategy | `include/recovery_manager.h` | `src/recovery_manager.cc` | `docs/EPIC3_RECOVERY_STRATEGY.md` |
| 3.6 Distributed retrieval | `include/distributed_planner.h` | `src/distributed_planner.cc` | `docs/EPIC3_DISTRIBUTED_RETRIEVAL.md` |
| 3.7 Tensor infrastructure | `include/tensor_infrastructure.h` | `src/tensor_infrastructure.cc` | `docs/EPIC3_ARCHITECTURE.md` |

## Current Delivery State

- Wave A complete: architecture and sub-issue planning docs available in `docs/EPIC3_*.md`.
- Wave B partial: module structure (`README.md`, `include/README.md`, `src/README.md`, `CMakeLists.txt`) is in place. Header and source files (`*.h`, `*.cc`) are out of scope for this PR and will be added in a dedicated implementation PR.
- Wave C pending: distributed correctness tests/benchmarks remain for
  `tests/epic3_distributed_tensor/` and `benchmarks/epic3_distributed_tensor/`.

## Seven-Phase Gate (module view)

- [x] Phase 1: artifact and infrastructure contracts documented
- [ ] Phase 2: file-level skeleton surfaces created (deferred to implementation PR)
- [ ] Phase 3: failure handling for placement/integrity/recovery paths
- [ ] Phase 4: distributed contract tests and fault-injection scenarios
- [ ] Phase 5: scale/performance hardening for multi-node environments
- [ ] Phase 6: acceptance documentation tied to recovery/integrity evidence
- [ ] Phase 7: integration with production retrieval/evaluation pipelines

## Module Boundaries

In scope:
- portable artifact taxonomy and manifest contracts
- placement, integrity, and recovery interfaces for distributed tensor operations
- infrastructure interfaces for node health and stripe transport

Out of scope at scaffold stage:
- production distributed scheduling logic and transport implementation
- final resiliency SLO enforcement and operational automation
- default build/test enablement

## Installation

No standalone installation step is required while the module remains in scaffold mode.
Build targets are intentionally held behind phase-gate approval.

## Usage

Use this documentation to:
- align EPIC 3 sub-issue planning with concrete file ownership
- track wave/phase readiness for distributed tensor work
- prepare test and benchmark rollout without enabling production targets

## References

- `docs/IMPLEMENTATION_ROADMAP.md`
- `docs/EPIC3_ARCHITECTURE.md`
- `docs/EPIC1_2_3_DEPENDENCIES.md`
- `src/distributed_tensor/include/README.md`
- `src/distributed_tensor/src/README.md`
