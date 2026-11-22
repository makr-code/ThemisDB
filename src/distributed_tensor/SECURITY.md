# Distributed Tensor Module Security Notes

**Last Updated:** 2026-11-21
**Source level:** Level 1 — module implementation
**SOT domain:** security
**Canonical references:** `include/distributed_tensor/integrity_verification.h`, `include/distributed_tensor/distributed_planner.h`, `include/distributed_tensor/tensor_storage_strategy.h`, `src/distributed_tensor/integrity_verification.cc`, `src/distributed_tensor/CMakeLists.txt`
**Related:** Issue #7341

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
Exact model backend not verifiable from final document text alone.
-->

## Scope and evidence boundary

The public API declares artifact-integrity verification, retrieval planning,
and storage-strategy interfaces. The storage-strategy header explicitly
describes the handled tensor data as advisory and says it must not override
graph-verified query results. The planner header exposes graph-validator and
fallback-related interfaces.

## Integrity implementation limitation

The active module target's CMake source list includes
`src/distributed_tensor/integrity_verification.cc`. It now uses OpenSSL EVP
SHA-256 when OpenSSL Crypto is found at configure time and fails closed when
that dependency is unavailable. Only SHA-256 is accepted; unsupported
algorithms, malformed digests, missing shard digests, duplicate shard IDs, and
legacy demonstration-hash receipts are rejected. Receipt inputs use a
versioned domain tag, length-prefixed fields, and sorted shard IDs.

The receipt binds the artifact ID, manifest content hash and version, and
declared shard digests—not all manifest metadata or the shard bytes themselves.
Callers must use `verify_shard()` to check raw bytes. Neither operation
authenticates who produced the data or receipt. The separate nested
`src/distributed_tensor/src/integrity_verification.cc` remains outside the
module target; its separate EPIC-3 test and benchmark compile it only when
OpenSSL Crypto is found. `ArtifactManifest::compute_manifest_hash()` is a
separate non-cryptographic checksum and is not an integrity or authenticity guarantee.
The SHA-256 changes and their regressions have not been built or tested in this
task, so production security properties remain unverified.

The training coordinator is also limited: `TensorTrainingCoordinator` invokes
registered `ITensorTrainingWorker::processShard` callbacks synchronously
in-process. It does not implement network transport or distributed worker
coordination.

More generally, declarations do not establish that every runtime path enforces
advisory-only behavior or that the module has passed a security review. The
test configurations include integrity-related sources, but no tests or
security checks were executed for this implementation task.

## Risks to validate

- Verify integrity checks against corruption, malformed manifests, and
  inconsistent shard metadata.
- Verify that advisory tensor data cannot override authoritative graph results
  on every consuming path.
- Verify behavior when the graph validator or fragment fetcher is absent or
  reports failure.
- Validate recovery and concurrency behavior under fault injection before
  making availability or fail-closed guarantees.

## Current status

Security properties and production guarantees remain unverified. No tests,
security scans, or runtime experiments were run for the SHA-256 change.
Historical security and phase-gate reports are not current validation evidence
unless their referenced artifacts are checked.

## References

- [`../../include/distributed_tensor/integrity_verification.h`](../../include/distributed_tensor/integrity_verification.h)
- [`../../include/distributed_tensor/distributed_planner.h`](../../include/distributed_tensor/distributed_planner.h)
- [`../../include/distributed_tensor/tensor_storage_strategy.h`](../../include/distributed_tensor/tensor_storage_strategy.h)
- [`ROADMAP.md`](ROADMAP.md)
