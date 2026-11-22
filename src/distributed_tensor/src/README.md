---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-11-21"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "makr-code"
---

# distributed_tensor/src documentation

Exact model backend not verifiable from final document text alone.

## Purpose

This directory holds the internal (`themis::distributed_tensor::detail` or
internal-header) implementations of the distributed tensor module. The public
contract lives in `include/distributed_tensor/*`; internal types that collided
with it were moved to `detail`.

## Source Ownership Map

Ownership is defined by `src/distributed_tensor/CMakeLists.txt`
(`DISTRIBUTED_TENSOR_SOURCES`).

| Source file | In `themis_distributed_tensor` | Role |
|---|---|---|
| `artifact_manifest.cc` | yes | internal `detail` manifest model |
| `distributed_planner.cc` | yes | internal `detail` retrieval planner |
| `tensor_delta_log.cc`, `manifest_store.cpp`, `crash_recovery_checkpoint.cc` | yes | delta log, manifest persistence, checkpoints |
| `tensor_storage_strategy.cc`, `snapshot_update_worker.cc`, `shard_summary_coordinator.cc` | yes | storage strategy, snapshot updates, shard summaries |
| `artifact_invalidation.cc`, `exact_graph_fallback.cc`, `stale_artifact_detector.cc` | yes | lifecycle, fallback, staleness |
| `error_recovery_handler.cc`, `distributed_lock_manager.cc` | yes | recovery and lock management |
| `manifest_store.cc` | no | shim including `manifest_store.cpp` for fallback benchmark targets |
| `integrity_verification.cc` | no | internal receipt-chain/Merkle-proof suite; test/benchmark only |

## Integrity implementations

Two integrity implementations exist with disjoint type names:

- **Production:** `src/distributed_tensor/integrity_verification.cc` implements
  the public `IntegrityVerificationEngine` (OpenSSL EVP SHA-256, fail-closed
  without OpenSSL, versioned receipts).
- **Internal, non-production:** `src/integrity_verification.cc` with
  `src/distributed_tensor/include/integrity_verification.h` (`ReceiptChain`,
  `MerkleProof`, `VerificationAuditTrail`, provenance/recovery hooks). It is
  compiled only into OpenSSL-gated EPIC-3 tests and benchmarks
  (`tests/epic3_distributed_tensor/CMakeLists.txt`,
  `benchmarks/epic3_distributed_tensor/CMakeLists.txt`) and must not be linked
  into production targets. Promotion into the module requires a reviewed API
  decision, build and test evidence, and documentation of its relationship to
  the public engine.

## Usage

Consumers include `include/distributed_tensor/*` and link
`themis_distributed_tensor`. Do not include headers from
`src/distributed_tensor/include/` outside the module and its tests.

## References

- `src/distributed_tensor/README.md`
- `src/distributed_tensor/ARCHITECTURE.md`
- `src/distributed_tensor/ROADMAP.md`
