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

# Tensor Module Implementation Guide

## Implementation map

The tensor module is implemented as a small set of tightly related runtime surfaces:

- `tensor_index_manager.cpp` — lifecycle, route selection, persistence, and manager state
- `tensor_index.cpp` — concrete flat index implementation and index-level operations
- `tensor_fingerprint_graph.cpp` — compact graph and similarity scoring
- `adapter_repository.cpp` — TT adapter storage and retrieval
- `tensor_mmap_bridge.cpp` — mmap-backed zero-copy bridge implementation
- `tensor_core_bridge.cpp` and `tensor_ingestion_bridge.cpp` — translation and ingestion boundaries
- `hnsw_tt_bridge.cpp` and `ht_index.cpp` — hybrid indexing paths
- `tensor_error_handling.cpp` and related helpers — diagnostics and failure handling

## Implementation invariants

### 1. Index manager lifecycle is registry-based and bounded

`TensorIndexManager` keeps a registry keyed by `(tenant_id, collection, field)` and stores a `std::unique_ptr<ITensorIndex>`. `createIndex()` serializes the registry lookup and insertion path with `registry_mutex_`, and it bounds concurrent creates with `pending_operations_`.

This means the manager is designed to prevent duplicate index instances from racing each other under concurrency, while also limiting the maximum number of simultaneous creates in-flight.

### 2. Route decisions are explicit and deterministic

`routeFor()` computes a `DataProfile` and calls `storage::TensorRouter::decide()`. The final decision is encoded as `TENSOR_TRAIN`, `HYBRID`, or `HNSW`. The logic is deterministic and does not silently reinterpret the route after selection.

### 3. Fingerprints are compact summaries, not authoritative tensor state

The graph stores a column-mean fingerprint derived from the first TT core and keeps a squared-norm cache. This is intentionally a compact proxy for index membership and approximate retrieval, not a replacement for the full TT train.

### 4. Invalid state is rejected before ranking

The fingerprint graph rejects empty or malformed TT-core data before insertion, and it rejects invalid scores or missing train entries before returning results. Silent partial results are considered unacceptable for the module.

### 5. Persistence and cleanup are paired

`dropIndex()` removes the in-memory entry, clears the legacy bridge cache, and also removes the persisted index file if configured. The implementation is explicit about cleanup obligations and keeps the cleanup sequence within the relevant manager operations.

## Key edge cases

- `train.cores.empty()` -> reject insertion
- `core.n == 0 || core.r_left == 0 || core.r_right == 0` -> reject insertion
- `fingerprint.empty()` -> skip query candidate in approximate search
- invalid `query_self_ip` / `other_self_ip` -> emit diagnostic and return empty set
- exact similarity callback throws or returns non-finite values -> skip candidate and emit diagnostic
- adapter keys can be replaced idempotently without inflating aggregate stats unexpectedly

## Operational guidance

When changing tensor logic, validate the path that is actually affected:

1. Check route selection semantics and `TensorRouter` thresholds
2. Validate index/adapter creation and persistence behavior
3. Validate fingerprint query behaviour and diagnostics
4. Confirm cleanup/removal paths are consistent with state invariants

The module’s correctness is driven by explicit contracts and bounded failure handling rather than implicit auto-recovery.
