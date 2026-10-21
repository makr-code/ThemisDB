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

# Vector Search API Contract

## Scope

This page documents the public contract visible in the shared index layer, not a separate code path under `src/vector_search/`. The authoritative interfaces are in:

- `include/index/vector_index.h`
- `include/index/multi_vector_search.h`

## `VectorIndexManager`

### Lifecycle

`VectorIndexManager` is constructed from a `RocksDBWrapper&` and then initialized with an object namespace and vector dimension.

Core lifecycle functions:

- `init(objectName, dim, metric, M, efConstruction, efSearch, savePath)`
- `setAdvancedIndexConfig(config)`
- `setEfSearch(efSearch)`
- `rebuildFromStorage()`
- `incrementalReindex(rebuild_threshold, vectorField)`
- `shutdown()`
- `saveIndex(directory)`
- `loadIndex(directory)`

### Status model

`VectorIndexManager::Status` is the canonical error/status object. It contains:

- `ok` (boolean)
- `message` (string)

The implementation uses explicit statuses instead of silently adjusting invalid state.

### Metric model

`VectorIndexManager::Metric` is an enum with:

- `L2`
- `COSINE`
- `DOT`

This must be set consistently for the index lifecycle. A mismatch is treated as an operational failure path, not an accepted compatibility scenario.

### Result model

`VectorIndexManager::Result` is a struct with:

- `pk`
- `distance`

This keeps the search result narrow and predictable. It is a ranking object, not a hidden stateful result container.

## `MultiVectorSearch`

`MultiVectorSearch` is the higher-level aggregation layer around one or more vector queries.

### Core search types

- `search(query, config)`
- `searchMultiField(query_vector, field_names, config)`
- `searchWithExpansion(query_variants, config)`
- `hybridSearch(query_vector, keyword_scores, config)`
- `batchSearch(queries, config)`

### Configuration surface

`SearchConfig` contains:

- `fusion` strategy,
- `weights`,
- `top_k`,
- `rrf_k`,
- `normalize_scores`,
- optional index name,
- `ef_search`.

### Fusion strategies

The implementation documents the following strategies:

- `LINEAR_COMBINATION`
- `MAX_SCORE`
- `MIN_SCORE`
- `AVG_SCORE`
- `RANK_FUSION`
- `RECIPROCAL_RANK`
- `LEARNED_FUSION` (documented as future work rather than a guaranteed production mode)

### Result envelope

`MultiSearchResult` returns:

- vector of `SearchResult` entries,
- the strategy used,
- total candidate count,
- computation time,
- the weights actually applied.

## Contract invariants

The API expects the following invariants to hold:

- each index must have a fixed dimension,
- each query must respect that dimension and metric semantics,
- invalid states surface as status failures,
- advanced backends require support and should not silently degrade,
- reindex, rebuild, and persistence paths remain explicit and recoverable.

## Edge cases and failure expectations

A caller should treat the following as expected failure cases rather than as hidden non-failures:

- invalid dimension mismatch,
- invalid metric requests,
- malformed input vectors,
- backend support missing for a selected advanced mode,
- storage drift requiring rebuild or reindex,
- resource exhaustion under high cardinality or sustained concurrency.

## Conclusion

The vector-search API is explicit and conservative. It favors stable lifecycle behavior, measurable ranking, and fail-closed validation over magic defaults. The public contract is intentionally narrow and operationally explainable.
