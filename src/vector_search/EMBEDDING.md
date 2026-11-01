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

Validation differs by API overload. The current source establishes that:

- initialization rejects an empty namespace or non-positive dimension,
- `searchKnn()` rejects a query whose dimension does not match the index,
- `addEntity()` with `WriteBatchWrapper` or `TransactionWrapper` rejects an
  entity vector whose dimension does not match the index,
- direct-commit `addEntity()` checks for a readable vector field but does not
  currently check its dimension.

The direct-commit dimension check is an open implementation gap. The code does
not establish that NaN/Inf values are rejected; finite-value validation remains
unverified. Metric selection is an initialization setting, not a per-query
argument.

## Storage and persistence model

The runtime uses a RocksDB-backed namespace structure and maintains the vector state in the object namespace plus optional HNSW metadata. The API explicitly exposes:

- `saveIndex()`
- `loadIndex()`
- `shutdown()`
- `rebuildFromStorage()`
- `incrementalReindex()`

This makes persistence and recovery a core operational concern rather than an implementation detail.

## Query semantics

A query must match the index's configured dimension. The index metric determines
the result distance semantics: lower values rank first, with squared L2,
`1 - cosine`, or negative dot product for the corresponding metrics.

The `VectorIndexManager::Result` structure is intentionally small and explicit:

- primary key (`pk`),
- distance score,
- status information in the surrounding operation.

That keeps similarity interpretation stable for both single-vector and multi-vector queries.

## Multi-vector embeddings

`MultiVectorSearch` extends the single embedding model to cases where a retrieval request is composed of multiple vectors or multiple fields.

The API includes:

- multi-vector query sets,
- a `searchMultiField()` entry point, which currently repeats one query vector
  and does not route to field-specific indexes,
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

The high-cardinality stress fixture in
`tests/vector_search/test_vector_search_highcardinality_stress.cpp` uses an
in-process flat index, not `VectorIndexManager`; it exercises the fixture's
insertion and query code rather than the production runtime.

## Known failure domains

The most important embedding-related failure domains include:

- dimension mismatch,
- dimension mismatch (including the unchecked direct-commit insertion path),
- unverified NaN/Inf handling,
- unavailable advanced backend support (with backend-specific error or fallback behavior),
- stale index metadata after updates or deletes,
- resource exhaustion under large batches or long-running ANN rebuilds.

The repository treats these as explicit failure modes rather than hidden degradation paths.

## Recommendation for maintainers

When modifying embedding-related behavior, validate the following invariants:

1. verify dimension compatibility separately for each insertion overload,
2. metric semantics are unchanged or intentionally updated with code and docs,
3. persistence and rebuild behavior remain recoverable,
4. multi-vector fusion remains deterministic for supported strategies,
5. query quality remains measurable via the benchmark gate.

## Conclusion

The shared index stores vectors under a configured dimension and metric, but
not all insertion overloads currently enforce that dimension. The direct-commit
gap and unverified finite-value validation must be resolved or guarded by callers
before documenting strict input validation.
