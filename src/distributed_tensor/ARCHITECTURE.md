# Distributed Tensor Module Architecture

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

`src/distributed_tensor` defines EPIC 3 contracts for portable tensor artifacts, manifest
management, shard placement, integrity verification, recovery management, distributed
planning, and tensor infrastructure coordination.

Current architecture is implementation-first for Phases 1-3: headers define the
public contracts, translation units provide the default runtime behavior, and the
focused EPIC 3 regression suite verifies the new failure/degraded-mode semantics.

## Component Map

> Source files and public headers are now implemented. The table below documents
> the active ownership map used by the current build.

| Component | Active contract | Active implementation |
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
- EPIC 3 contract ownership, sequencing, and default runtime behavior
- integration seams with EPIC 1 retrieval and EPIC 2 evaluation
- phase-gated readiness documentation and focused regression coverage

Out of scope at the current stage:
- finalized scale and fault-injection benchmark outcomes
- default pipeline enablement

## Integration Surfaces

- Planning: `docs/EPIC3_ARCHITECTURE.md` and EPIC 3 sub-issue docs
- Public contracts: `include/distributed_tensor/*.h`
- Runtime implementation: `src/distributed_tensor/*.cc`
- Focused verification: `tests/epic3_distributed_tensor/test_phase3_failure_semantics.cpp`
- Dependency sequencing: `docs/EPIC1_2_3_DEPENDENCIES.md`

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| acceleration | `include/acceleration/` | GPU compute backend for distributed tensor operations |
| storage | `include/storage/` | Artifact persistence, shard placement, and recovery state |
| sharding | `include/sharding/` | Shard routing and placement coordination |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| llm | `include/distributed_tensor/` | LLM training pipeline uses distributed tensor for large-model weight distribution |

## Integration Points

### Critical Integration: LLM Training Pipeline
**Files:** `src/distributed_tensor/distributed_planner.cc`, `shard_placement.cc` ↔ `llm/` (training)
**Contract:** LLM training requests a distributed placement plan from `DistributedPlanner`; plan results are deterministic for fixed cluster topology and artifact sizes.
**Thread Safety:** Planner is stateless per request; concurrent planning calls are safe.

### Critical Integration: Storage Artifact Persistence
**Files:** `src/distributed_tensor/artifact_manifest.cc`, `integrity_verification.cc` ↔ `storage/`
**Contract:** Artifact manifests and integrity hashes are persisted through storage interfaces; integrity verification reads must be idempotent.
**Thread Safety:** Manifest writes are serialised per artifact ID; verification reads are concurrent-safe.
