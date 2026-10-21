---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "unknown"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Tensor Module API Contract

## Public interfaces in scope

This module exposes a small set of public tensor-facing contracts that are used by other parts of ThemisDB:

### `TensorIndexManager`

Location: `include/tensor/tensor_index_manager.h`

Responsibilities:

- choose a route for a tenant/collection/field using `routeFor()`
- create or retrieve index instances using `createIndex()` and `getIndex()`
- remove and list indexes through `dropIndex()`, `dropTenantIndexes()`, `listIndexes()`
- optionally pin TT-core pages using `mapCores()` and `ggmlCorePtrs()`
- persist indexes with `setDataDir()` and `flushAll()`

Contract notes:

- route decisions are `TENSOR_TRAIN`, `HYBRID`, or `HNSW`
- null returns are valid when no index is found or the route is unsupported
- configuration and cleanup semantics are explicit; no silent fallback is expected for malformed state

### `TensorFingerprintGraph`

Location: `include/tensor/tensor_fingerprint_graph.h`

Responsibilities:

- register adapter fingerprints via `addAdapter()`
- remove them with `removeAdapter()`
- query candidate similarity with `findSimilar()` and `findSimilarByFingerprint()`
- inspect graph state via `entry()`, `size()`, `adapterKeys()`, and `stats()`

Contract notes:

- `findSimilar()` excludes the query key from the result set
- `findSimilarByFingerprint()` returns results sorted by descending similarity
- invalid magnitude, invalid scores, or missing candidate trains result in empty output with diagnostics rather than silent bad values

### `AdapterRepository`

Location: `include/tensor/adapter_repository.h`

Responsibilities:

- persist TT trains for a domain / base-model pair
- load adapter descriptors through `loadAdapter()`
- list domains and adapter keys
- optionally hook a `TensorFingerprintGraph` with `setFingerprintGraph()`

Contract notes:

- storage is tenant-scoped and keyed by domain + base-model metadata
- the repository is responsible for adapter provenance and retrieval orchestration
- missing or malformed adapter state is treated as a fail-safe condition

### `TensorMmapBridge`

Location: `include/tensor/tensor_mmap_bridge.h`

Responsibilities:

- represent pinned or mmap-backed TT-core views
- expose RAII ownership of mapped memory blocks
- keep pointer validity aligned with the lifetime of the bridge object

Contract notes:

- bridge lifetime must be kept alive as long as mapped slices are used
- memory resets and munmap behavior are part of the object contract

## Error and edge-case contract

The module strongly prefers explicit failure to silent recovery. In API terms that means:

- empty or malformed TT cores fail fast
- invalid similarity math does not produce implicit results
- missing index or adapter is signaled as an empty or false response
- data persistence and cleanup must remain paired

This is the public behavior contract for the tensor module, and it is the contract that callers should rely on when integrating with higher-level retrieval flows.
