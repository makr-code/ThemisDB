---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-21"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/distributed_tensor/*.h; src/distributed_tensor/CMakeLists.txt; src/distributed_tensor/README.md; src/distributed_tensor/ARCHITECTURE.md; tests/distributed_tensor/CMakeLists.txt; tests/epic3_distributed_tensor/CMakeLists.txt; benchmarks/distributed_tensor/CMakeLists.txt; benchmarks/epic3_distributed_tensor/CMakeLists.txt"
---

> Exact model backend not verifiable from final document text alone.

# Distributed Tensor — Developer LLM Wiki

Curated orientation for implementation and review in `distributed_tensor`.
This is secondary context, not a source of release status: public headers and
implementation establish behavior, CMake files establish registration, and
recorded test/benchmark output establishes execution evidence.

## Module shape and flow

The module builds the static `themis_distributed_tensor` library. Its public
headers are in [`include/distributed_tensor/`](../../include/distributed_tensor/);
implementation and internal support code are in
[`src/distributed_tensor/`](../../src/distributed_tensor/). The target links
`themis_base` and conditionally links `themis_core` and `themis_tensor` when
those targets exist.

The public surfaces compose, but do not by themselves establish a networked
end-to-end service:

1. A caller creates an `ArtifactManifest` with metadata and shard placements.
2. `DefaultShardPlacementStrategy::compute_placement()` returns a plan from
   artifact/shard sizes, node capacity, and constraints; the caller applies the
   placements.
3. `DefaultDistributedTensorPlanner::plan_tensor_retrieval()` validates
   manifest and lifecycle conditions and returns shard IDs and estimates; it
   does not fetch shard data.
4. The separate `DistributedTensorPlanner` supports summary-first routing and
   exact-on-demand requests. `executeFragmentLoads()` delegates to an injected
   `IFragmentFetcher`; no built-in network transport is provided.
5. Integrity and recovery APIs operate on the supplied manifest, receipt, and
   recovery request; they do not execute placement or network operations.
6. The internal `ManifestStore` persists validated manifest metadata through
   the RocksDB-backed `RocksDBWrapper`. It requires a caller-provided database
   path, writes WAL-backed synchronous batches, and rejects corrupt records on
   open. This persists local placement metadata only; it does not replicate
   records or shard payloads to other nodes. Lifecycle invalidation, exact-graph
   fallback, staleness, recovery, lock, and snapshot-update implementations are
   part of the module target's private source set.

`TensorTrainingCoordinator` invokes worker callbacks synchronously in-process.
It does not implement distributed worker execution. Configuration is supplied
through API value types, including planner, transport, storage strategy, and
placement options; there is no module configuration file. See the
[architecture guide](../../src/distributed_tensor/ARCHITECTURE.md) for the
component map and configuration inventory.

The public declarations under `include/distributed_tensor/` remain in
`themis::distributed_tensor`. Source-private manifest/class definitions and
summary-first planner types under `src/distributed_tensor/include/` use
`themis::distributed_tensor::detail`; these similarly named manifest, class,
planner, routing, retrieval, and fragment types are distinct from the public
API types.

## Public contract and limitations

- The public API requires C++20: `tensor_storage_strategy.h` includes
  `<span>` and exposes `std::span`. The CMake target propagates
  `cxx_std_20` to consumers linking the target. Direct users of the installed
  headers must also compile as C++20.
- The default module target compiles root-level `integrity_verification.cc`,
  which now uses OpenSSL EVP SHA-256 when OpenSSL Crypto is available and fails
  closed otherwise. The versioned receipt binds the artifact ID, manifest
  content hash/version, and declared shard digests, not all metadata or raw
  shard bytes; callers must use `verify_shard()` for raw bytes. Neither
  operation authenticates the producer. The nested implementation remains
  outside the product target and is compiled into EPIC-3 test/benchmark
  targets only when OpenSSL is found. `ArtifactManifest::compute_manifest_hash()`
  remains non-cryptographic. The
  SHA-256 implementation and new regression sources have not been built or
  tested; the generated `WIKI_STATUS.json` has not been refreshed.
- Placement plans are not automatically persisted or applied; callers may
  persist placement metadata through `ManifestStore`. Retrieval planning does
  not fetch fragments. Caller-supplied fetchers and transport implementations
  remain outside this module's built-in behavior.

Follow the public headers for exact signatures and edge behavior. The module
[README](../../src/distributed_tensor/README.md),
[public-header guide](../../include/distributed_tensor/README.md), and
[security guide](../../src/distributed_tensor/SECURITY.md) provide additional
source-linked context.

## Build, tests, and benchmarks

The root CMake configuration now registers `themis_tensor` before tests and
benchmarks, independently of `THEMIS_BUILD_ENTERPRISE_PLUGINS`. The
distributed tensor target can link that tensor module in non-enterprise
configurations too. Static owners and shared owners with exported symbols are
de-duplicated against `themis_tensor`, which links those owners privately.
Tensor test targets filter direct source additions against the same owners.
Non-exporting shared owners still overlap. These changes have not been
configured, built, or tested; the generated wiki manifest remains unchanged.

The module-local focused test CMake registers three executables and CTest cases
from allocation, operations, and performance test sources; see
[`tests/distributed_tensor/CMakeLists.txt`](../../tests/distributed_tensor/CMakeLists.txt).
The separately included EPIC 3 directory registers contract and phase tests,
including durable manifest restart, corrupt-record rejection, WAL recovery, and
concurrent update coverage. Integration CMake may include the distributed-tensor
soak test. Registration alone is not evidence that a target built or passed.
The nightly coverage workflow runs `distributed_tensor`-labelled tests,
publishes module-level summaries and LCOV data, and enforces 70% line / 60%
branch coverage for `manifest_store.cpp` and `src/artifact_manifest.cc`.
Generated evidence records the per-path measurements. Workflow configuration
does not imply a measured passing result; no path-specific exception is
approved.

Build the module library in an existing configured build tree with:

```sh
cmake --build <build-dir> --target themis_distributed_tensor
```

For exact target names, labels, and benchmark guards, consult the module CMake
files and [test evidence guide](../../src/distributed_tensor/TEST_EVIDENCE_COLLECTION.md).
The local benchmark under `benchmarks/distributed_tensor/` uses in-process
stubs; its output is not distributed-system, GPU, or production performance
evidence. EPIC 3 benchmarks are separate and only configured when benchmark
support is enabled.

For the 2026-10-31 validation attempt, the module configure stopped before
generating targets because the required `fmt` library was unavailable. Thus no
module test or benchmark was run. The public storage-strategy header passed a
standalone C++20 syntax check and was rejected by C++17 at `std::span`; this
confirms the header language requirement but not a full target build. The wiki
sync regression tests passed, but they do not exercise this module.

Current test and benchmark execution evidence is maintained in the module test
evidence guide, not inferred from source registration. Production enablement
requires the gates and human sign-offs in the module's
[production enable gates](../../src/distributed_tensor/PRODUCTION_ENABLE_GATES.md)
and canonical root governance. No representative-hardware performance or
production-release approval should be claimed without recorded evidence.

## Source priority and freshness

Use the module [README](../../src/distributed_tensor/README.md) as the
documentation entry point. For contract changes, prefer the public header and
implementation source, then current module documentation and CMake/test
registration, then this summary. Root governance is authoritative for release
status. If this page conflicts with those sources, correct it from source and
retain unresolved evidence gaps explicitly.
