---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
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

`TensorTrainingCoordinator` invokes worker callbacks synchronously in-process.
It does not implement distributed worker execution. Configuration is supplied
through API value types, including planner, transport, storage strategy, and
placement options; there is no module configuration file. See the
[architecture guide](../../src/distributed_tensor/ARCHITECTURE.md) for the
component map and configuration inventory.

## Public contract and limitations

- The public API requires C++20: `tensor_storage_strategy.h` includes
  `<span>` and exposes `std::span`. The CMake target propagates
  `cxx_std_20` to consumers linking the target. Direct users of the installed
  headers must also compile as C++20.
- The default module target compiles root-level `integrity_verification.cc`,
  whose `simple_sha256_hash()` is an FNV-1a demonstration hash. The separate
  nested implementation containing SHA-256 calls is not in this target's
  source list. Do not treat the active hash or Merkle-shaped data structures as
  cryptographic integrity evidence.
- Placement plans are not persisted or applied automatically. Retrieval
  planning does not fetch fragments. Caller-supplied fetchers and transport
  implementations remain outside this module's built-in behavior.

Follow the public headers for exact signatures and edge behavior. The module
[README](../../src/distributed_tensor/README.md),
[public-header guide](../../include/distributed_tensor/README.md), and
[security guide](../../src/distributed_tensor/SECURITY.md) provide additional
source-linked context.

## Build, tests, and benchmarks

The module-local focused test CMake registers three executables and CTest cases
from allocation, operations, and performance test sources; see
[`tests/distributed_tensor/CMakeLists.txt`](../../tests/distributed_tensor/CMakeLists.txt).
The separately included EPIC 3 directory registers contract and phase tests,
and integration CMake may include the distributed-tensor soak test. Registration
alone is not evidence that a target built or passed.

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
