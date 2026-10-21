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

# Vector Search Modeling Notes

## Purpose

This document describes the model assumptions represented by the vector-search implementation in ThemisDB. It is grounded in the shared-index API layer under `include/index/` and the runtime behavior in `src/index/`, not in a separate duplicate implementation under `src/vector_search/`.

## Core model

The model is a namespace-backed vector index over dense floating-point vectors. The canonical representation is `VectorIndexManager` and the multi-query enhancement is `MultiVectorSearch`.

At the model level, each indexed item is represented as:

- a primary key (`pk`),
- a vector payload,
- a metric that defines distance or similarity semantics,
- optional advanced ANN backend configuration.

The runtime contract is stated directly in `include/index/vector_index.h` and `include/index/multi_vector_search.h`.

## Assumed vector semantics

### Dimension

The dimension is part of the index identity. The implementation does not treat the dimension as an optional property of individual vectors; it is configured at index creation time. This creates a strict invariant for all inserted, updated, and queried vectors.

### Metric semantics

The supported distance metrics are:

- `Metric::L2`
- `Metric::COSINE`
- `Metric::DOT`

These are not interchangeable. This means all callers must choose the intended semantic before indexing and searching. The source code requires explicit configuration and fails on unsupported or invalid states rather than silently switching metrics.

### Query and result behavior

Within `VectorIndexManager`, result objects carry:

- the primary key,
- the computed distance,
- the status message and success state.

The search result semantics are determined by index configuration, query vector, and the chosen metric. The implementation makes the result ordering explicit and allows callers to treat the distance as a ranking signal, not as a hidden side effect.

## Multi-vector model

`MultiVectorSearch` extends the single-index model to accommodate multiple vectors, multiple fields, or multi-query retrieval.

The fusion model includes several strategies:

- linear combination,
- max/min/avg score fusion,
- rank fusion,
- reciprocal rank fusion,
- learned fusion (documented as future work rather than current production guarantee).

This allows ThemisDB to model real retrieval problems such as:

- multi-field search over title, summary, and body embeddings,
- query expansion with multiple reformulations,
- hybrid ranking that mixes vector relevance with keyword scores,
- multi-modal retrieval built on multiple embedding types or fields.

## Indexing lifecycle

The model presents an index lifecycle with a clear set of operations:

1. initialize the namespace and dimension,
2. configure the metric and optional ANN backend,
3. add or update entities,
4. execute search or multi-vector search,
5. reindex or rebuild as state drifts from storage,
6. persist or recover on shutdown and restart.

This lifecycle is visible in the API methods stored in `include/index/vector_index.h`, including `init()`, `setEfSearch()`, `rebuildFromStorage()`, `incrementalReindex()`, `saveIndex()`, and `loadIndex()`.

## Operational model

The implementation expects the following operational realities:

- search and indexing operate on an evolving set of vectors in storage,
- some vectors may be updated or removed while a graph index remains warm,
- index consistency must be recomputed or repaired under drift,
- large workloads require explicit tuning of ANN parameters instead of accidental defaults.

This is why the runtime exposes reindex and rebuild behavior as explicit operations rather than an implicit background synchronization layer.

## Fail-closed design

The modeling assumption is that invalid inputs and unsupported runtime states are not recoverable by heuristics. The code treats them as explicit failures. Examples include:

- invalid dimension mismatch,
- unsupported metric,
- advanced index configuration requested without required support,
- rebuild operations required when the index has drifted beyond acceptable thresholds.

This is a stronger and safer modeling choice than trying to complete an approximate search on malformed state.

## Performance model

The document uses the existing benchmark gate language from `src/vector_search/FUTURE_ENHANCEMENTS.md` and `src/vector_search/PERFORMANCE_EXPECTATIONS.md`:

- insertion throughput and query latency remain key operational indicators,
- query latency is expected to remain bounded for the configured search profile,
- concurrency must remain stable under write and read mixing,
- advanced backend selection should increase capacity only when proven under the release gate.

The runtime preserves these measurable expectations through `tests/vector_search/test_vector_search_highcardinality_stress.cpp` and the benchmark gates under `benchmarks/vector_search/`.

## Domain fit

This model is a fit for ThemisDB’s retrieval and knowledge-graph workloads, especially where the system needs:

- nearest-neighbor search over dense vectors,
- explicit similarity semantics,
- fast candidate generation for relevance ranking,
- bounded operational behavior when data volume increases.

It is less suited for a model that assumes an unbounded, schema-free search surface without validation; those scenarios would require additional design work for sharding, persistence, and large-scale index lifecycle management.

## Conclusion

The vector-search model in ThemisDB is a dense-vector retrieval layer with explicit metric semantics, predictable lifecycle control, and failure-closed validation rules. The implementation favors correctness and operational transparency over hidden fallback logic, and any improvements to the index should preserve that contract.
