# Distributed Tensor Module Architecture

**Last Updated:** 2026-11-21
**Source level:** Level 1 — module implementation
**SOT domain:** architecture-governance
**Canonical references:** `src/distributed_tensor/CMakeLists.txt`, `include/distributed_tensor/*.h`, `cmake/CMakeLists.txt`
**Related:** Issue #7341

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: ThemisDB Team
Exact model backend not verifiable from final document text alone.
-->

## Kontext

The module's public contracts are in `include/distributed_tensor/`; its
implementation sources are listed in `src/distributed_tensor/CMakeLists.txt`.
That CMake file builds the static target `themis_distributed_tensor`, links
`themis_base`, and conditionally links `themis_core` and `themis_tensor` if
those targets are available. The project CMake default is C++20, but the module
target publicly requests `cxx_std_20`. The public
`tensor_storage_strategy.h` includes `<span>` and exposes `std::span`, so C++20
is a public-header requirement. Consumers linking the CMake target inherit the
feature requirement; direct users of the installed headers must select C++20
themselves.

## Komponenten

| Area | Public interface | Implementation sources in the module target |
|---|---|---|
| Artifact classes and manifests | `tensor_artifact_classes.h`, `artifact_manifest.h` | `tensor_artifact_classes.cc`, `artifact_manifest.cc`, `src/artifact_manifest.cc` |
| Placement and retrieval | `shard_placement.h`, `distributed_planner.h` | `shard_placement.cc`, `distributed_planner.cc`, `src/distributed_planner.cc` |
| Integrity and recovery | `integrity_verification.h`, `recovery_manager.h` | `integrity_verification.cc`, `recovery_manager.cc` |
| Infrastructure and in-process training jobs | `tensor_infrastructure.h`, `tensor_training_coordinator.h` | `tensor_infrastructure.cc`, `tensor_training_coordinator.cc` |
| Storage strategy | `tensor_storage_strategy.h` | `src/tensor_storage_strategy.cc` |
| Update and coordination support | Private headers under `src/distributed_tensor/include/` | Delta log, RocksDB-backed manifest store, checkpoint, snapshot-update worker, shard-summary, artifact invalidation, exact-graph fallback, staleness, recovery, and lock-manager sources listed in CMake |

This table highlights component groups; the CMake file is the authoritative
complete target source list.

The public declarations in `include/distributed_tensor/` remain in
`themis::distributed_tensor`. The colliding source-private manifest/class
definitions and summary-first planner model under
`src/distributed_tensor/include/` are isolated in
`themis::distributed_tensor::detail`; their `ArtifactManifest`,
`ArtifactClass`, planner, routing, retrieval, and fragment types are distinct
from the public API types.

## Schnittstellen

The module exposes placement, retrieval, integrity, recovery, manifest-store,
and training-coordinator interfaces described below. It does not own a shard
payload transport or distributed manifest consensus interface.

## Datenfluss

The public interfaces expose these composable steps; they do not establish an
end-to-end network execution pipeline:

1. A caller creates an `ArtifactManifest` and supplies metadata and shard
   placements.
2. `DefaultShardPlacementStrategy::compute_placement()` accepts artifact/shard
   sizes, node-capacity records, and placement constraints; it returns a
   `PlacementPlan`. The caller is responsible for applying any returned
   placements to a manifest.
3. `DefaultDistributedTensorPlanner::plan_tensor_retrieval()` validates
   manifest completeness, any invalid integrity receipt, lifecycle requirements,
   and dependencies, then returns a retrieval plan with shard IDs and estimates.
   It does not fetch shard data.
4. The separate `DistributedTensorPlanner` builds summary-first routing and
   exact-on-demand requests. `executeFragmentLoads()` delegates to an injected
   `IFragmentFetcher`; without one it returns failed results. It has no built-in
   network transport.
5. Integrity and recovery APIs operate on the supplied manifest/receipt and
   recovery request; they do not turn placement plans into network operations.
6. `ManifestStore` persists validated manifest metadata in the caller-provided
   RocksDB database. WAL-backed synchronous batches make acknowledged manifest
   writes durable across process restart; corrupt records fail store opening.
   The store does not replicate metadata across nodes, and does not persist
   shard payloads or provide a network recovery protocol.

Configuration is supplied through API values rather than a module configuration
file:

| Surface | Declared settings |
|---|---|
| `DistributedTensorPlanner::Config` | Summary TTL, minimum fragment success rate, fallback routing, fragment-load timeout, strict graph validation |
| `StripeTransport` | Protocol, compression/encryption flags and algorithms, streams, timeout, retries, backoff |
| `StorageStrategyAssessor::Config` | Quantization constraints, NVMe, multi-consumer, mmap support, and page-lock availability |
| `ShardPlacementStrategy::compute_placement()` | Node capacities, optional factorization hints, and placement constraints |

These settings describe the public configuration surface. The source does not
establish that every option is exercised by an end-to-end runtime path; for
example, no transport implementation is supplied by this module.

The module target compiles the root-level `integrity_verification.cc`, which
uses OpenSSL EVP SHA-256 when OpenSSL Crypto is found at configure time and
fails closed when it is unavailable. The engine accepts only SHA-256 and uses
versioned, length-prefixed receipt inputs with sorted shard IDs. Receipts bind
the artifact ID, manifest content hash and version, and declared shard
digests—not all manifest metadata or the shard bytes themselves. Callers must
use `verify_shard()` to check raw bytes. Neither operation authenticates the
producer. The separate nested
`src/integrity_verification.cc` implementation remains outside this target and
is compiled only into its EPIC-3 test/benchmark targets when OpenSSL is found.
`ArtifactManifest::compute_manifest_hash()` is a separate non-cryptographic
checksum and must not be used as an integrity or authenticity guarantee.

`TensorTrainingCoordinator::runNextJob()` calls registered
`ITensorTrainingWorker::processShard()` callbacks synchronously in-process.
No network transport or distributed training-worker protocol is implemented
by this coordinator.

## Integration surfaces established by source

- The public planner header includes tensor summary types and the graph-truth
  validator interface.
- The target's direct CMake dependency is `themis_base`; `themis_core` and
  `themis_tensor` are optional target dependencies.
- `cmake/CMakeLists.txt` conditionally adds the module and conditionally links
  selected benchmark targets to it.
- The `tests/distributed_tensor/` and `benchmarks/distributed_tensor/`
  directories each register their local module-focused sources. Separate
  `tests/epic3_distributed_tensor/` and
  `benchmarks/epic3_distributed_tensor/` CMake files are also added by their
  parent configurations. `tests/integration/CMakeLists.txt` includes the
  distributed-tensor soak source when present.

The earlier architecture text described a direct LLM-training pipeline,
storage persistence, and sharding integration with specific behavior and
thread-safety guarantees. Those connections are not established by the current
module CMake configuration or the reviewed public headers; treat them as
unverified until a canonical integration contract or implementation reference
is identified.

## Sequenz (kritischer Ablauf)

```mermaid
sequenceDiagram
  participant Caller
  participant Store as ManifestStore
  participant DB as RocksDB
  Caller->>Store: store(valid manifest)
  Store->>DB: version check + synchronous WAL batch
  DB-->>Store: durable commit status
  Store-->>Caller: success only after commit
  Caller->>Store: reopen after restart
  Store->>DB: scan and validate records and keys
  DB-->>Store: persisted state or corruption/error
```

**Kurzinterpretation:** The successful return follows a synchronous local
commit; startup rejects unreadable or invalid persisted records. Neither step
replicates state or shard payloads to another node.

## Fehlerpfade und Resilienz

An empty database path is rejected. Database-open failures and persisted
records that fail schema, manifest, or key validation prevent store creation.
Writes return failure on database read/commit errors, and malformed records
are not served by read paths. These guarantees are local to the configured
RocksDB instance.

## Nicht-Ziele

This module currently does not implement remote shard fetching, manifest
consensus, multi-node replication, or distributed training execution. Placement
plans must be applied by the caller, and RocksDB durability alone does not
establish cluster recovery.

## Verweise

Source registration is not evidence that tests passed or that the module is
enabled in a production workflow. See [`README.md`](README.md) for the current
test/benchmark inventory and [`ROADMAP.md`](ROADMAP.md) for phase planning.
