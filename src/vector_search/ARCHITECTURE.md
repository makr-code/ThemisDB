---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Architecture — Vector Search Module

## Context and boundary

`src/vector_search/` is a documentation boundary only. The runtime belongs to
the shared index subsystem in `include/index/` and `src/index/`. The public
contracts most directly relevant to this module are
`VectorIndexManager` and `MultiVectorSearch`; neither is implemented in this
directory.

## Components

- **VectorIndexManager:** owns a vector namespace backed by RocksDB. It exposes
  entity mutation, KNN and filtered search, statistics, reindex/rebuild, and
  save/load operations. HNSW acceleration is optional and build-dependent;
  brute-force search is also supported.
- **MultiVectorSearch:** combines results from vector queries, fields, query
  expansions, or auxiliary scores using configured fusion strategies.
- **AdvancedVectorIndex:** optional backend integration configured through the
  shared index API. Unsupported build capabilities are reported through the
  API's `Status`; do not describe them as guaranteed runtime features.
- **DistributedVectorIndex:** a separate in-process, sharded ANN implementation
  in `src/index/`. Its presence does not imply durable replication or a
  production multi-node service.

## Runtime flow

```text
caller
  -> VectorIndexManager / MultiVectorSearch (include/index/)
  -> RocksDB-backed namespace, in-memory cache, optional ANN backend
  -> Status and ranked results
```

Vector inputs and index configuration are passed through the shared public
interfaces. The manager checks initialization, dimensions, and operation
results and reports failures through `Status`; the API does not define the
numeric E5400-series error taxonomy claimed by older module documents.

## Failure and capability boundaries

- Initialization rejects an empty namespace or a non-positive dimension.
- Search rejects a query dimension that does not match the initialized index.
- Entity operations can fail when the manager is uninitialized, the vector
  field is absent or malformed, storage operations fail, or optional encryption
  fails.
- Advanced index configuration can return an error when the requested support
  is unavailable in the build.
- HNSW availability is conditional; callers must not assume a particular
  backend solely from the module name.
- Persistence APIs exist for the HNSW index, and vector entities are stored
  through the RocksDB-backed namespace. This does not establish distributed
  persistence or replication.

## Validation evidence

The module-specific high-cardinality stress and integration soak fixtures use
flat in-process indexes. The dedicated benchmark also uses a synthetic flat
index. These are useful fixture-level checks but do not validate the production
runtime's ANN performance or release readiness. See
[performance expectations](PERFORMANCE_EXPECTATIONS.md) and
[module gaps](MODULE_GAPS.md).

## Non-goals

- Duplicating the index runtime under `src/vector_search/`.
- Treating optional backends or in-process sharding as production guarantees.
- Publishing benchmark thresholds as measured production baselines without
  runtime-backed measurements.

## References

- [README](README.md) · [API](API.md) · [Implementation](IMPLEMENTATION.md)
- [Roadmap](ROADMAP.md) · [Security](SECURITY.md)
- [`VectorIndexManager`](../../include/index/vector_index.h)
- [`MultiVectorSearch`](../../include/index/multi_vector_search.h)
- [`DistributedVectorIndex`](../../include/index/distributed_vector_index.h)
