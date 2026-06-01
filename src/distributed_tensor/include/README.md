# distributed_tensor/include documentation

<!-- Status: current | aligned with docs/IMPLEMENTATION_ROADMAP.md | validated: 2026-06-01 -->

## Purpose

This directory defines EPIC 3 public contracts for distributed tensor artifact
portability, placement, integrity, and recovery semantics.

## Header Ownership Map

| Header | EPIC sub-issue | Contract focus |
|---|---|---|
| `tensor_artifact_classes.h` | 3.1 | artifact classes, metadata, and class registry API |
| `artifact_manifest.h` | 3.2 | manifest entry/snapshot lifecycle contracts |
| `shard_placement.h` | 3.3 | placement constraints, assignments, and planning interface |
| `integrity_verification.h` | 3.4 | stripe verification and Merkle-based integrity receipts |
| `recovery_manager.h` | 3.5 | recovery jobs, strategy selection, and status reporting |
| `distributed_planner.h` | 3.6 | distributed retrieval request and execution plan contracts |
| `tensor_infrastructure.h` | 3.7 | node health registry and stripe transport interfaces |

## Contract Governance Rules

- align type semantics with the matching `docs/EPIC3_*.md` document before header edits
- keep interfaces implementation-agnostic until Phase 3 behavior sign-off
- register dependency impacts with EPIC 1 and EPIC 2 when contract changes cross module boundaries

## Transition Checklist (toward hardening)

- define explicit error/recovery expectations for each interface
- document invariants for artifact identity, placement determinism, and integrity receipts
- map each contract to planned distributed/fault-injection tests

## Installation

No separate installation step is required for this header-only scaffold surface.

## Usage

Use this README as the contract review index before editing EPIC 3 interfaces.

## References

- `docs/EPIC3_ARCHITECTURE.md`
- `docs/EPIC3_ARTIFACT_CLASSES.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
- `docs/EPIC3_SHARD_PLACEMENT.md`
- `docs/EPIC3_INTEGRITY_MODEL.md`
- `docs/EPIC3_RECOVERY_STRATEGY.md`
- `docs/EPIC3_DISTRIBUTED_RETRIEVAL.md`
- `docs/EPIC3_ARCHITECTURE.md`
