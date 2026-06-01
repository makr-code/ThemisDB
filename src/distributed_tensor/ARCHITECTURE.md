# Distributed Tensor Module Architecture

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

`src/distributed_tensor` defines EPIC 3 contracts for portable tensor artifacts, manifest
management, shard placement, integrity verification, recovery management, distributed
planning, and tensor infrastructure coordination.

Current architecture is scaffold-first: headers capture contract boundaries and matching
translation units anchor implementation ownership for phased delivery.

## Component Map

> Source files (`*.h`, `*.cc`) are deferred to the implementation PR.
> The table below documents planned file ownership; no code files exist yet.

| Component | Planned contract | Planned implementation |
|---|---|---|
| Artifact classes | `include/tensor_artifact_classes.h` | `src/tensor_artifact_classes.cc` |
| Manifest schema | `include/artifact_manifest.h` | `src/artifact_manifest.cc` |
| Shard placement | `include/shard_placement.h` | `src/shard_placement.cc` |
| Integrity verification | `include/integrity_verification.h` | `src/integrity_verification.cc` |
| Recovery manager | `include/recovery_manager.h` | `src/recovery_manager.cc` |
| Distributed planner | `include/distributed_planner.h` | `src/distributed_planner.cc` |
| Tensor infrastructure | `include/tensor_infrastructure.h` | `src/tensor_infrastructure.cc` |

## Boundaries

In scope:
- EPIC 3 contract ownership and sequencing
- integration seams with EPIC 1 retrieval and EPIC 2 evaluation
- phase-gated readiness documentation

Out of scope at scaffold stage:
- production distributed-placement/recovery runtime behavior claims
- finalized scale and fault-injection benchmark outcomes
- default pipeline enablement

## Integration Surfaces

- Planning: `docs/EPIC3_ARCHITECTURE.md` and EPIC 3 sub-issue docs
- Planned contracts: `src/distributed_tensor/include/*.h` (deferred to implementation PR)
- Planned implementation: `src/distributed_tensor/src/*.cc` (deferred to implementation PR)
- Dependency sequencing: `docs/EPIC1_2_3_DEPENDENCIES.md`
