# distributed_tensor/src documentation

<!-- Status: current | aligned with docs/IMPLEMENTATION_ROADMAP.md | validated: 2026-06-01 -->

## Purpose

This directory contains EPIC 3 skeleton sources that reserve ownership and integration
order for distributed tensor runtime behavior without enabling production logic yet.

## Source Ownership Map

> Most source files remain deferred to future implementation PRs. `integrity_verification.cc`
> is now the active Phase-2 baseline for EPIC 3.4.

| Source file | EPIC sub-issue | Planned runtime responsibility |
|---|---|---|
| `tensor_artifact_classes.cc` | 3.1 | artifact classification and metadata registry behavior |
| `artifact_manifest.cc` | 3.2 | manifest persistence/lookup semantics |
| `shard_placement.cc` | 3.3 | placement planning under capacity/topology constraints |
| `integrity_verification.cc` | 3.4 | checksum/Merkle verification and receipt generation |
| `recovery_manager.cc` | 3.5 | recovery scheduling, retries, and status transitions |
| `distributed_planner.cc` | 3.6 | distributed retrieval planning across shard targets |
| `tensor_infrastructure.cc` | 3.7 | node registry and stripe transport orchestration points |

## Delivery Expectations

- next: extend the EPIC 3.4 Phase-2 baseline into runtime recovery/planner integration
- after: Phase-3 behavior for placement conflicts, verification failures, and recovery escalation
- later: fault-injection tests, performance hardening, acceptance docs, and integration

## Integration Notes

- local `CMakeLists.txt` keeps targets disabled until acceptance gates are met
- dependency sequencing must follow `docs/EPIC1_2_3_DEPENDENCIES.md`

## Installation

No standalone installation step is required while files remain phase-2 skeletons.

## Usage

Use this README to track implementation ownership and readiness for EPIC 3 source files.

## References

- `docs/IMPLEMENTATION_ROADMAP.md`
- `docs/EPIC1_2_3_DEPENDENCIES.md`
- `src/distributed_tensor/README.md`
