---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

# Vector Embedding and Indexing Notes

## Purpose

This document explains how ThemisDB treats vectors as first-class retrieval objects and how the shared index layer integrates them with insert, update, query, and persistence flows. It is grounded in the code under `include/index/vector_index.h`, `include/index/multi_vector_search.h`, and `src/index/vector_index.cpp`.

## Embedding contract

The index manages dense vector embeddings as floating-point payloads assigned to a primary key. Each index instance is configured with:

- a namespace / object name,
- a fixed vector dimension,
- a metric (`L2`, `COSINE`, or `DOT`),
- optional HNSW/ANN tuning settings.

This means the embedding representation is treated as a schema-level property of the index, not as a best-effort or partially typed payload.

## Input validation boundaries

The implementation makes validation part of the API contract. This includes ensuring that:

- vectors match the configured dimension,
- invalid floating-point values are not accepted silently,
- metric selection is supported before query/index use,
- advanced backend configuration is requested only when the required runtime support exists.

These checks appear in the source as explicit status returns and logging around advanced-index setup and vector operations.

## Storage and persistence model

The runtime uses a RocksDB-backed namespace structure and maintains the vector state in the object namespace plus optional HNSW metadata. The API explicitly exposes:

- `saveIndex()`
- `loadIndex()`
- `shutdown()`
- `rebuildFromStorage()`
- `incrementalReindex()`

This makes persistence and recovery a core operational concern rather than an implementation detail.

## Query semantics

A query is only valid when it matches the index's configured dimension and metric semantics. Search results are not just candidate IDs; they include the distance ranking data used by the caller to decide how relevant a result is.

The `VectorIndexManager::Result` structure is intentionally small and explicit:

- primary key (`pk`),
- distance score,
- status information in the surrounding operation.

That keeps similarity interpretation stable for both single-vector and multi-vector queries.

## Multi-vector embeddings

`MultiVectorSearch` extends the single embedding model to cases where a retrieval request is composed of multiple vectors or multiple fields.

Examples described in the code include:

- multi-vector query sets,
- multi-field search over title/content/body embeddings,
- query expansion across several variants,
- hybrid ranking using vector similarity plus auxiliary signal sources.

The important boundary is that these are still built on top of the same vector index semantics; they only aggregate or combine results differently.

## Operational notes

The embedding subsystem is expected to operate under realistic load, including:

- insert-heavy workloads,
- query-heavy workloads,
- mixed read-write concurrency,
- reindex or rebuild during drift recovery,
- long-lived index persistence across restarts.

The stress test in `tests/vector_search/test_vector_search_highcardinality_stress.cpp` validates the behavior expected under high-cardinality insertion and concurrent query/test conditions.

## Known failure domains

The most important embedding-related failure domains include:

- dimension mismatch,
- invalid float values and NaN/Inf content,
- unsupported metric or backend selection,
- stale index metadata after updates or deletes,
- resource exhaustion under large batches or long-running ANN rebuilds.

The repository treats these as explicit failure modes rather than hidden degradation paths.

## Recommendation for maintainers

When modifying embedding-related behavior, validate the following invariants:

1. dimension compatibility stays strict,
2. metric semantics are unchanged or intentionally updated with code and docs,
3. persistence and rebuild behavior remain recoverable,
4. multi-vector fusion remains deterministic for supported strategies,
5. query quality remains measurable via the benchmark gate.

## Conclusion

The vector-search embedding model is a controlled, schema-aware retrieval system. It assumes a fixed dimensionality and an explicit metric, requires validation at the boundary, and expects persistence, reindex, and multi-vector aggregation to remain explicit and measurable operations.
