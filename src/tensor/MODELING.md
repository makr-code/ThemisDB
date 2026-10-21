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

# Tensor Module Modeling Notes

## Model boundaries

The tensor module is best understood as a set of connected runtime models around `TTTrain`-based compression and retrieval:

- `TensorIndexManager` chooses the storage route and manages index lifecycle
- `AdapterRepository` stores and loads adapter-level TT graphs
- `TensorFingerprintGraph` maintains a compact fingerprint graph for nearest-neighbour style scoring
- `TensorMmapBridge` and related bridge surfaces expose a pinned-memory integration point for tensor-heavy runtime consumers

This is a structural model rather than a single monolithic class; each component owns a specific runtime responsibility.

## Primary data model

### Tensor train representation

`storage::TTTrain` is the canonical tensor representation. It packages:

- `mode_sizes`: original tensor dimensions
- `cores`: list of TT cores, each with `r_left`, `n`, `r_right`, and flattened `data`
- `original_norm` and `achieved_eps`: reconstruction and quality metadata

The decomposition behavior is implemented in `TensorTrainDecomposer`, and validation is built around the contract that each TT core must have valid dimensions and consistent `numElements()` length. The code rejects malformed cores before registering them in a fingerprint graph or adapter repository.

### Adapter lifecycle

`AdapterRepository::store()` serializes an adapter and writes it under a tenant-scoped key. `loadAdapter()` reconstructs the TT data for later runtime injection. In the current code, this is a heap-backed or bridge-backed descriptor flow, not yet a fully zero-copy memory-pinned runtime.

The repository also keeps a fingerprint graph delegate, which lets the system register adapter fingerprints when a backend proves valid and remove them when an adapter is deleted.

### Query and ranking model

`TensorFingerprintGraph` computes similarity in two ways:

- exact key-based similarity via `findSimilar()`
- approximate fingerprint similarity via `findSimilarByFingerprint()`

The exact path uses `TensorTrainDecomposer::innerProduct()` when available, while the approximate path uses cached squared norms and zero-padding semantics for dimension mismatches. This creates a clear model split: exactness for registered trains and bounded approximation for lightweight fingerprint lookups.

## Control flow

1. A tenant creates or loads an index via `TensorIndexManager::createIndex()` or `getIndex()`.
2. A route is chosen using `routeFor()` and `TensorRouter::decide()` based on dimension / `kappa_estimate`.
3. The adapter or tensor data is persisted and optionally registered in the fingerprint graph.
4. Queries use either the exact train path or the fingerprint path according to the operation.
5. Result sets are sorted, trimmed to `k`, and counters are updated in the graph stats.

## Failure model

The module treats invalid data, mismatch, or corrupted state as explicit failure conditions rather than silent success. Examples from the source include:

- invalid TT core dimensions cause early return in `TensorFingerprintGraph::addAdapter()`
- invalid self inner product or scalar score values trigger `emitFingerprintDiagnostic()`
- missing adapter or candidate train entries produce no result and a diagnostic instead of proceeding
- `TensorIndexManager::dropIndex()` removes persisted index files and clears internal legacy bridge cache entries

This is consistent with the module’s design emphasis on explicit lifecycle and recovery semantics.

## Design implications

The tensor model is intentionally conservative:

- route early,
- keep a compact fingerprint proxy,
- use TT decomposition for the parts that benefit from compression,
- and fail closed around malformed or invalid state.

That is why the module is a good fit for vector-heavy or adapter-heavy workloads but not ideal for opaque or unbounded surrogate pipelines.
