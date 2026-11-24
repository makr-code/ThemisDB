# Distributed Tensor Module Security Notes

**Last Updated:** 2026-11-24
**Source level:** Level 1 — module implementation
**SOT domain:** security
**Canonical references:** `include/distributed_tensor/integrity_verification.h`, `include/distributed_tensor/distributed_planner.h`, `include/distributed_tensor/graph_summary_projection.h`, `include/distributed_tensor/shard_rpc_fragment_fetcher.h`, `include/sharding/shard_rpc_client.h`, `include/sharding/shard_rpc_server.h`, `proto/sharding/shard_rpc.proto`, `include/distributed_tensor/tensor_storage_strategy.h`, `src/distributed_tensor/integrity_verification.cc`, `src/distributed_tensor/CMakeLists.txt`, `src/sharding/shard_rpc_client.cpp`, `src/sharding/shard_rpc_server.cpp`
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

The optional `ShardRPCFragmentFetcher` path uses bounded gRPC streaming over
verified mTLS. The client checks the configured peer ID and DNS SAN, stream
identities and sequence, a single final frame, size/digest metadata, and the
requested byte/deadline limits. The adapter then recomputes SHA-256 and compares
the bytes with the trusted manifest digest. This detects mismatch against that
manifest; it does not authenticate the manifest producer or grant authorization
to its contents.

The server requires an explicit X.509 identity-to-principal binding and an
explicit local shard ID. When
`sharding.tensor_fragment_rpc.enabled` is set, `main_server.cpp` supplies a
thread-safe RocksDB reader backed by `ShardArtifactFragmentStore`. Its key is
the versioned `(principal_id, shard_id, artifact_id)` tuple, separate from
`TensorNetworkStorageEngine` field keys. The store checks size metadata before
loading payload bytes and verifies the stored SHA-256; the manifest-bound
adapter performs the final comparison against the trusted manifest. The
principal must be tenant-bound by host policy, and publishers must persist
bytes before advertising the matching placement. Missing TLS credentials or
identity bindings fail startup closed. The host integration was built and a
local minimal-profile startup smoke exercised publication, but production
configuration, graceful shutdown, and power-loss behavior remain unverified.
`LocalProjectionPublisher` requires a trusted caller to supply an authenticated
tenant, tenant-bound principal, authoritative shard/node and summary provenance.
`loadRemote()` binds the record to the configured mTLS peer and validates its
content-bound artifact ID and expected tenant, principal, shard and node. It
also verifies the fetched projection's digest, scope and candidate keys. It
does not authenticate the original producer, discover artifacts, or establish
freshness; these remain caller responsibilities. The publisher
checks the scope tenant prefix and requires each candidate key to match its
tenant, domain and base-model fields; these checks do not establish provenance
or caller authorization.
`publishFromGraph()` also checks query metadata from the tenant-scoped graph
snapshot; the graph, scoring callbacks, principal binding, freshness and
exclusion of concurrent graph mutations remain trusted host responsibilities.
The Linux mTLS test suite passed 8/8 directly and 1/1 in CTest on retry. Its
fixture case composes the RocksDB fragment reader and manifest-bound adapter,
including wrong-digest and unbound-client rejection; it does not verify
production host startup composition.

Graph validation accepts only the bounded, versioned
`themis.tensor.adapter-similarity/v1` projection. The decoder rejects unknown
versions, malformed or non-canonical payloads, oversized fields, invalid
UTF-8/control characters, and non-finite or out-of-range scores. The planner
also checks request/result identity and the fetcher's manifest-digest
verification claim, merges only matching scopes, and requires validated
evidence meeting the configured graph-score threshold with supporting graph
nodes. Arbitrary tensor payloads and transport success alone never establish
graph truth.

This boundary still depends on a trusted manifest and a conforming fetcher;
the format does not authenticate its publisher or authorize graph assertions.
No automatic publisher emits the projection today, and the implementation and
new tests have not been built or run. These controls are source-level design,
not verified production guarantees.

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
