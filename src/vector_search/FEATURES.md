---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Vector Search Capabilities

## Role

Vector search is implemented by the shared `index` subsystem. This module
documents its contracts and validation evidence; it does not contain a
standalone runtime.

## Problem and operational value

Applications that store embeddings need nearest-neighbour retrieval and, where
supported by the caller, a way to combine several query results. The shared
index API provides the persistence and search boundary so consumers do not need
to own a separate vector-storage implementation.

## Capabilities represented by the public interfaces

- `VectorIndexManager` provides a RocksDB-backed vector namespace, entity
  mutation, KNN and filtered search, reindex/rebuild operations, and index
  save/load APIs. Optional HNSW and advanced-index behavior depends on build
  support.
- `MultiVectorSearch` combines results from multiple vectors, query expansions,
  or keyword scores using the configured fusion strategy. The current
  `searchMultiField()` implementation does not route to field-specific indexes;
  see [API contract](API.md).
- `DistributedVectorIndex` provides in-process shard routing and scatter-gather
  search. It is not a durable or replicated multi-node deployment service.

## Target scenarios

- KNN retrieval over vectors stored in a RocksDB-backed namespace.
- Filtered vector search and index rebuild/reindex operations.
- Query expansion or hybrid vector/keyword score fusion through
  `MultiVectorSearch`.

## Trade-offs and limits

Backend capability is build-dependent. Module-specific stress/soak tests and the
dedicated benchmark use synthetic flat indexes, so their results must not be
presented as production-runtime performance evidence.

## Source references

- [`VectorIndexManager`](../../include/index/vector_index.h)
- [`MultiVectorSearch`](../../include/index/multi_vector_search.h)
- [`DistributedVectorIndex`](../../include/index/distributed_vector_index.h)
- [Architecture](ARCHITECTURE.md) · [Performance expectations](PERFORMANCE_EXPECTATIONS.md)
