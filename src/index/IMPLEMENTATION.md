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
SOT_Domain: module-behavior
Canonical_Sources:
  - "include/index/index_manager.h"
  - "src/index/index_manager.cpp"
  - "include/index/vector_index.h"
  - "src/index/vector_index.cpp"
  - "include/index/index_manifest_v1.h"
  - "src/index/index_manifest_v1.cpp"
  - "tests/index/test_index_manager_di.cpp"
  - "tests/index/test_index_manifest_v1.cpp"
Related: "Issue #7125; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

# Index implementation guide

## Purpose and reader

This guide is for developers implementing index behavior or integrating the
index manager with storage, evaluators, retrieval, and lifecycle code. It
describes current ownership and call contracts visible in source; it is not a
prescription to add behavior to an unrelated storage or embedding subsystem.

## Component boundaries

- `IndexManager` is a dependency-injected facade. It owns registry/adapters and
  delegates vector, secondary, and graph work to concrete managers. The
  evaluator and storage are optional constructor inputs; `setRocksDB()` wires
  the backing managers. Creating index instances before storage is wired can
  return `ERR_INDEX_NOT_INITIALIZED`, as tested by
  [`test_index_manager_di.cpp`](../../tests/index/test_index_manager_di.cpp).
- `VectorIndexManager` persists entity vectors using the RocksDB wrapper and
  `KeySchema::makeVectorKey`, with optional HNSW state/cache and a search
  fallback path. Its behavior depends on compile-time support and configured
  backends.
- `AnnFrontdoor` is an ANN routing/execution boundary over registered
  `IAnnIndex` backends and optional vector/tier managers. Registration is
  setup-time work and must finish before concurrent searches.
- `IndexManifestV1` converts manifest data to/from JSON strings. The class
  itself is not a RocksDB persistence adapter and does not provide
  synchronization.

## Vector lifecycle and invariants

1. Apply `setAdvancedIndexConfig()` before `init()` when an advanced config is
   required. The implementation rejects enabling it after initialization;
   unsupported feature builds may return an error.
2. Initialize the namespace with a positive dimension and an explicit metric.
3. Add/update/remove entity vectors through direct operations or their
   `WriteBatchWrapper`/`TransactionWrapper` overloads when the caller owns a
   transaction boundary. Check every returned `Status`.
4. Use `incrementalReindex()` to reconcile stored entities with in-memory
   HNSW state; its reported counts distinguish added, removed, updated, and
   unchanged vectors. It may trigger a full rebuild when its configured
   deleted-label threshold is exceeded.
5. Use `saveIndex()`/`loadIndex()` for the explicit HNSW directory
   serialization API. Auto-save applies only when enabled and the relevant
   HNSW state/path are configured; it is not a substitute for validating the
   storage data.

The API details and defaults are in
[`vector_index.h`](../../include/index/vector_index.h); the implementation is
in [`vector_index.cpp`](vector_index.cpp). Vector operations can fail for
uninitialized managers, missing/wrongly typed vector fields, dimension
mismatch, persistence errors, or backend errors. On failure, callers must not
publish success or assume the in-memory and persisted states are equivalent.

## Routing and concurrent access

`AnnFrontdoor::search()` requires a non-null query pointer and positive
dimension. A non-positive `k` uses `Config::default_k`; an invalid resulting
`k` is returned as an empty result with configuration-error metadata. Backend
registration methods mutate internal maps and are not safe to race with search:
complete setup before concurrent readers. Search result metadata communicates
the route, fallback, correlation/policy identifiers, and distributed shard
outcomes; downstream code must inspect `partial_results` and not mistake a
degraded result for a complete one.

The frontdoor currently validates result count and filters NaN/negative
distances. It does not verify candidate completeness or reject positive
infinity. Its brute-force helper is currently a stub returning an empty list;
do not implement a caller expectation from the route label alone. See
[`ann_frontdoor.cpp`](ann_frontdoor.cpp) and
[`ann_frontdoor.h`](../../include/index/ann_frontdoor.h).

## Index manager and tenant namespace

The facade provides create/get/drop/list/type/capability/statistics methods for
secondary, vector, and graph interfaces. Tenant-aware overloads construct
logical index names using `tenant:<id>:<name>` and reject empty components,
components longer than 512 bytes, `:` and NUL. The prefix is a namespace
mechanism, not an authorization decision; the calling service must authorize
the tenant and index operation independently.

## Manifest behavior

Manifest serialization is minified JSON; parse failures are logged and
re-thrown. `add_version_history()` mutates the vector and timestamp and is not
thread-safe. The C++ structs and JSON schema currently differ; no schema
validator is called by these methods. See
[`index_manifest_v1.cpp`](index_manifest_v1.cpp) and
[`EMBEDDING.md`](EMBEDDING.md) before changing version metadata.

## Validation path

Start with focused tests:

- ANN route/result behavior:
  [`test_ann_frontdoor.cpp`](../../tests/index/test_ann_frontdoor.cpp)
- Manager dependency wiring and adapters:
  [`test_index_manager_di.cpp`](../../tests/index/test_index_manager_di.cpp)
- Manifest serialization/history behavior:
  [`test_index_manifest_v1.cpp`](../../tests/index/test_index_manifest_v1.cpp)
- GPU fallback boundaries:
  [`test_gpu_vector_index_llm_paths_focused.cpp`](../../tests/index/test_gpu_vector_index_llm_paths_focused.cpp)

The test names and targets are registered in
[`tests/index/CMakeLists.txt`](../../tests/index/CMakeLists.txt). Hardware-only
GPU parity remains an open validation gate; stub and CPU tests do not close it.
