---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: level1
SOT_Domain: api-contract
Canonical_Sources:
  - "include/index/ann_frontdoor.h"
  - "src/index/ann_frontdoor.cpp"
  - "include/index/vector_index.h"
  - "include/index/gpu_vector_index.h"
  - "include/index/index_manager.h"
  - "include/index/index_manifest_v1.h"
  - "include/index/index_error_codes.h"
  - "tests/index/test_ann_frontdoor.cpp"
Related: "Issue #7125; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

# Index API reference

## Purpose and reader

This page is for developers calling public interfaces declared under
`include/index/`. It summarizes the currently declared module contracts.
Declarations and implementation take precedence over a generic `IIndex`
summary or a stale contract narrative when they disagree.

## `AnnFrontdoor`

Declared in [`ann_frontdoor.h`](../../include/index/ann_frontdoor.h).

- `registerBackend(scope_id, backend, kind)` installs a non-null `IAnnIndex`;
  null is rejected with `std::invalid_argument`. Registration and scope
  registration must complete before concurrent queries.
- `registerVectorIndexManager()` requires a non-null pointer. A null tiered
  manager is allowed and disables tier routing.
- `planStrategy()`, `planRetrieval()`, and `explainStrategy()` expose the
  planned route; planning is not a guarantee of implementation availability
  or target recall/latency.
- `search(query_vector, dim, k, context)` rejects a null pointer or `dim == 0`
  with `std::invalid_argument`. `k <= 0` selects `Config::default_k`; if that
  is still invalid, search returns an empty result carrying configuration
  error fields. Results include route/fallback metadata and distributed shard
  counts.
- Search caps the result count at `k` and removes negative and NaN distances.
  Consumers must still check errors/degradation and partial-result metadata.
  Positive infinity is not filtered by the current validation code.

The declared strategies are HNSW, ScaNN, DiskANN, distributed, and flat brute
force; scope kinds include Generic, Adapter, Package, ShardSummary, Document,
Chunk, and Entity. Config defaults are source-defined in the header and are
not deployment SLAs.

## `VectorIndexManager`

Declared in [`vector_index.h`](../../include/index/vector_index.h). It
initializes an entity namespace with a dimension and `L2`, `COSINE`, or `DOT`
metric. Main operations include entity add/update/remove (direct, write-batch,
or transaction overloads), `searchKnn()`, radius and filtered searches,
incremental reindex, storage rebuild, explicit save/load, statistics, and
shutdown. Check the returned `Status` (or status/statistics pair); do not
assume an error leaves a newly requested state available.

The exact distance/result semantics, preconditions, overloads, and defaults
are declared in the header. Search filter forms differ: whitelist IDs,
post-filter attributes, secondary-index prefilters, and evaluator-aware
methods are not interchangeable. Evaluator-aware behavior is restricted by
the header's JSON-context contract; unsupported evaluator types can fall back
to the unfiltered search path. Do not use a search filter as an authorization
boundary.

## `GPUVectorIndex`

Declared in [`gpu_vector_index.h`](../../include/index/gpu_vector_index.h).
`initialize(dimension)` fails for a non-positive dimension. Requested backend
availability is build/runtime dependent. `Config::allowCPUFallback=true`
allows CPU failover on backend failure; `false` fails closed when the requested
backend cannot initialize. Operation methods return booleans or result
vectors, so callers should distinguish empty/no-result outcomes from
successful retrieval via available lifecycle/status information. The
configuration and statistics include active backend and memory/oversubscription
observations.

CPU fallback tests do not establish numerical parity on GPU hardware.
Representative-device parity remains pending in the
[module roadmap](ROADMAP.md).

## `IndexManager`

Declared in [`index_manager.h`](../../include/index/index_manager.h). The
facade injects optional evaluator/storage dependencies and wires concrete
managers when RocksDB is set. It exposes create/get/drop/list/type,
capability/statistics, and tenant-scoped variants for secondary, vector, and
graph indexes. Index creation can fail when backing storage has not been
initialized. Tenant-prefixed logical names are isolation namespaces, not
authentication or authorization.

## Manifest and error interfaces

[`IndexManifestV1`](../../include/index/index_manifest_v1.h) offers JSON
serialization/parsing, history rotation, previous-version lookup, and a
rollback-availability predicate. It is explicitly not thread-safe. It does
not itself persist into RocksDB or execute rollback. See
[`index_manifest_v1.cpp`](index_manifest_v1.cpp) for the actual serialization.

[`index_error_codes.h`](../../include/index/index_error_codes.h) defines
backend, rebuild, and distribution error categories. Use a structured error
when the calling interface returns one; do not map a partial result to full
success without checking its metadata.

## Contract validation and canonical references

Focused contract tests: [`test_ann_frontdoor.cpp`](../../tests/index/test_ann_frontdoor.cpp),
[`test_index_manager_di.cpp`](../../tests/index/test_index_manager_di.cpp),
[`test_index_manifest_v1.cpp`](../../tests/index/test_index_manifest_v1.cpp),
and [`test_gpu_vector_index_llm_paths_focused.cpp`](../../tests/index/test_gpu_vector_index_llm_paths_focused.cpp).
Their CTest registrations are in
[`tests/index/CMakeLists.txt`](../../tests/index/CMakeLists.txt).
The generic [`INDEX_CONTRACT.md`](../../include/index/INDEX_CONTRACT.md)
contains descriptions that do not all match current header fields; verify
claims against the declarations and implementation linked above before relying
on it as a contract.
