# Distributed Tensor Public Headers

**Last Updated:** 2026-10-31
**Source level:** Level 1 — public API
**SOT domain:** api-contract
**Canonical references:** `include/distributed_tensor/*.h`, `src/distributed_tensor/CMakeLists.txt`
**Related:** Issue #7341

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
Exact model backend not verifiable from final document text alone.
-->

`include/distributed_tensor/` contains the public C++ interfaces for the
distributed tensor module. The canonical module implementation and status
documentation is in [`../../src/distributed_tensor/README.md`](../../src/distributed_tensor/README.md)
and [`../../src/distributed_tensor/ROADMAP.md`](../../src/distributed_tensor/ROADMAP.md).

## Public headers

| Header | Declared interface area |
|---|---|
| `artifact_manifest.h` | Artifact manifests, shard placements, integrity receipts, reconstruction metadata |
| `distributed_planner.h` | Distributed retrieval plans, summary-first routing and exact-on-demand request interfaces |
| `integrity_verification.h` | Integrity receipts, hash algorithm enumeration, verification engine interfaces |
| `recovery_manager.h` | Recovery strategies and manager interface |
| `shard_placement.h` | Placement constraints, node capacity and placement strategy |
| `tensor_artifact_classes.h` | Artifact classes, lifecycle and artifact metadata |
| `tensor_infrastructure.h` | Node/infrastructure management interfaces |
| `tensor_storage_strategy.h` | Quantization assessment, memory mapping and storage strategy APIs |
| `tensor_training_coordinator.h` | In-process training-job coordinator and worker callback interface |

These are declarations, not a guarantee that every strategy or integration is
enabled in a production deployment. Consult the corresponding header and the
module CMake source list for current details.

**Integrity caveat:** The module CMake target uses the root-level
`src/distributed_tensor/integrity_verification.cc`, whose `simple_sha256_hash()`
is an FNV-1a demonstration hash, not SHA-256. The separate nested
`src/distributed_tensor/src/integrity_verification.cc` with SHA-256 calls is
not part of that target's source list. Do not treat the default target's
Merkle-shaped structures or receipts as cryptographic security.

**Training caveat:** `TensorTrainingCoordinator` runs registered worker
callbacks synchronously in-process; it does not provide networked or
distributed worker execution.

## Minimal API example

```cpp
#include "distributed_tensor/artifact_manifest.h"

themis::distributed_tensor::ArtifactManifest manifest(
    "artifact-1", themis::distributed_tensor::ArtifactClass::PRIMARY);
manifest.set_version("v1");
```

## Build

The module CMake target is `themis_distributed_tensor`; its configured public
headers are listed in `src/distributed_tensor/CMakeLists.txt`. The repository
does not define the previously documented `themis-distributed_tensor` target
alias or the sample `createDistributedPlanner()` API.

After configuring a repository build with this module enabled:

```sh
cmake --build <build-dir> --target themis_distributed_tensor
```

## C++ standard caveat

The project CMake configuration defaults to C++20, while the module target
privately requests `cxx_std_17`. The public
`tensor_storage_strategy.h` includes `<span>` and exposes `std::span`, which
requires C++20. Do not assume the installed public API is C++17-compatible.
Whether the private target feature request propagates the public header's
requirement to external consumers is unresolved; no compatibility build was
performed for this documentation review.

## Related documentation

- [`../../src/distributed_tensor/README.md`](../../src/distributed_tensor/README.md)
- [`../../src/distributed_tensor/ARCHITECTURE.md`](../../src/distributed_tensor/ARCHITECTURE.md)
- [`../../src/distributed_tensor/ROADMAP.md`](../../src/distributed_tensor/ROADMAP.md)
